#include "mesh_app_ui.hpp"

#include "HeltecMesh.h"
#include "config/DataStore.h"
#include "config/NodePrefs.h"
#include "target.h"

#include <Arduino.h>
#include <helpers/SensorManager.h>
#include <helpers/sensors/LocationProvider.h>
#include <math.h>

namespace heltec::meshcore::biz {
namespace {
uint32_t current_utc_epoch_s() {
  mesh::RTCClock* rtc = the_mesh.getRTCClock();
  return rtc ? rtc->getCurrentTime() : 0;
}

uint32_t gps_utc_epoch_s() {
  LocationProvider* nmea = sensors.getLocationProvider();
  if (!nmea || !nmea->isValid()) return 0;
  const long ts = nmea->getTimestamp();
  return ts > 0 ? static_cast<uint32_t>(ts) : 0;
}

double geodesic_distance_m(double lat1_deg, double lon1_deg, double lat2_deg, double lon2_deg) {
  constexpr double kPi = 3.14159265358979323846;
  constexpr double kDeg2Rad = kPi / 180.0;
  constexpr double kWgs84A = 6378137.0;
  constexpr double kWgs84F = 1.0 / 298.257223563;
  constexpr double kWgs84B = (1.0 - kWgs84F) * kWgs84A;

  if (lat1_deg == lat2_deg && lon1_deg == lon2_deg) return 0.0;

  const double phi1 = lat1_deg * kDeg2Rad;
  const double phi2 = lat2_deg * kDeg2Rad;
  const double L = (lon2_deg - lon1_deg) * kDeg2Rad;

  const double U1 = atan((1.0 - kWgs84F) * tan(phi1));
  const double U2 = atan((1.0 - kWgs84F) * tan(phi2));
  const double sinU1 = sin(U1);
  const double cosU1 = cos(U1);
  const double sinU2 = sin(U2);
  const double cosU2 = cos(U2);

  double lambda = L;
  double lambda_prev = 0.0;
  double sinSigma = 0.0;
  double cosSigma = 0.0;
  double sigma = 0.0;
  double sinAlpha = 0.0;
  double cosSqAlpha = 0.0;
  double cos2SigmaM = 0.0;

  for (int iter = 0; iter < 200; ++iter) {
    const double sinLambda = sin(lambda);
    const double cosLambda = cos(lambda);
    const double t1 = cosU2 * sinLambda;
    const double t2 = cosU1 * sinU2 - sinU1 * cosU2 * cosLambda;
    sinSigma = sqrt(t1 * t1 + t2 * t2);
    if (sinSigma == 0.0) return 0.0;
    cosSigma = sinU1 * sinU2 + cosU1 * cosU2 * cosLambda;
    sigma = atan2(sinSigma, cosSigma);
    sinAlpha = cosU1 * cosU2 * sinLambda / sinSigma;
    cosSqAlpha = 1.0 - sinAlpha * sinAlpha;
    cos2SigmaM = cosSqAlpha != 0.0 ? cosSigma - 2.0 * sinU1 * sinU2 / cosSqAlpha : 0.0;
    const double C = kWgs84F / 16.0 * cosSqAlpha * (4.0 + kWgs84F * (4.0 - 3.0 * cosSqAlpha));
    lambda_prev = lambda;
    lambda = L + (1.0 - C) * kWgs84F * sinAlpha *
                     (sigma + C * sinSigma *
                                  (cos2SigmaM + C * cosSigma *
                                                    (-1.0 + 2.0 * cos2SigmaM * cos2SigmaM)));
    if (fabs(lambda - lambda_prev) < 1e-14) break;
  }

  if (fabs(lambda - lambda_prev) >= 1e-12) {
    const double dphi = (lat2_deg - lat1_deg) * kDeg2Rad;
    const double dlambda = (lon2_deg - lon1_deg) * kDeg2Rad;
    const double a = sin(dphi / 2) * sin(dphi / 2) +
                   cos(phi1) * cos(phi2) * sin(dlambda / 2) * sin(dlambda / 2);
    return kWgs84A * 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
  }

  const double uSq = cosSqAlpha * (kWgs84A * kWgs84A - kWgs84B * kWgs84B) / (kWgs84B * kWgs84B);
  const double A =
      1.0 + uSq / 16384.0 * (4096.0 + uSq * (-768.0 + uSq * (320.0 - 175.0 * uSq)));
  const double B = uSq / 1024.0 * (256.0 + uSq * (-128.0 + uSq * (74.0 - 47.0 * uSq)));
  const double deltaSigma =
      B * sinSigma *
      (cos2SigmaM + B / 4.0 *
                         (cosSigma * (-1.0 + 2.0 * cos2SigmaM * cos2SigmaM) -
                          B / 6.0 * cos2SigmaM * (-3.0 + 4.0 * sinSigma * sinSigma) *
                              (-3.0 + 4.0 * cos2SigmaM * cos2SigmaM)));
  return kWgs84B * A * (sigma - deltaSigma);
}

bool should_record_track_point(int32_t lat_e6, int32_t lon_e6, int32_t last_lat_e6, int32_t last_lon_e6) {
  if (lat_e6 == last_lat_e6 && lon_e6 == last_lon_e6) return false;

  const double moved_m = geodesic_distance_m((double)last_lat_e6 * 1.0e-6, (double)last_lon_e6 * 1.0e-6,
                                             (double)lat_e6 * 1.0e-6, (double)lon_e6 * 1.0e-6);
  const double min_m = (double)HELTEC_GPS_TRACK_MIN_DIST_CM / 100.0;
  return moved_m + 1e-6 >= min_m;
}

}  // namespace

bool MeshAppUi::gpsTrackRecording() const {
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  return _gps_track_armed;
#else
  return false;
#endif
}

bool MeshAppUi::setGpsTrackRecording(bool enabled) {
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  DataStore* ds = the_mesh.getDataStore();
  if (!ds) return false;

  if (!enabled) {
    if (ds->isGpsTrackRecordingOpen()) {
      uint32_t end_ts = gps_utc_epoch_s();
      if (!end_ts) end_ts = current_utc_epoch_s();
      ds->endGpsTrackRecording(end_ts);
    }
    _gps_track_armed = false;
    _gps_track_has_last = false;
    _gps_track_point_count = 0;
    _gps_track_start_ms = 0;
    if (NodePrefs* p = the_mesh.getNodePrefs()) {
      if (p->gps_track_armed != 0) {
        p->gps_track_armed = 0;
        the_mesh.savePrefs();
      }
    }
    notifyAppState(heltec::meshcore::ui::AppStateEventType::ConfigChanged);
    notifyAppState(heltec::meshcore::ui::AppStateEventType::FindFriendChanged);
    return true;
  }

  if (!gpsStatus().enabled) return false;

  _gps_track_armed = true;
  _gps_track_has_last = false;
  _gps_track_point_count = 0;
  _gps_track_start_ms = 0;
  if (NodePrefs* p = the_mesh.getNodePrefs()) {
    if (p->gps_track_armed == 0) {
      p->gps_track_armed = 1;
      the_mesh.savePrefs();
    }
  }
  notifyAppState(heltec::meshcore::ui::AppStateEventType::ConfigChanged);
  notifyAppState(heltec::meshcore::ui::AppStateEventType::FindFriendChanged);
  return true;
#else
  (void)enabled;
  return false;
#endif
}

int MeshAppUi::gpsTrackIntervalIndex() const {
  return 0;
}

bool MeshAppUi::setGpsTrackIntervalIndex(int index) {
  (void)index;
  return false;
}

int MeshAppUi::gpsTrackIntervalOptionCount() const {
  return 0;
}

const char* MeshAppUi::gpsTrackIntervalOptionLabel(int index) const {
  (void)index;
  return nullptr;
}

void MeshAppUi::pollGpsTrack() {
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  if (!_gps_track_armed) return;

  DataStore* ds = the_mesh.getDataStore();
  if (!ds) {
    _gps_track_armed = false;
    _gps_track_has_last = false;
    return;
  }

  const GpsStatus gps = gpsStatus();
  if (!gps.enabled) {
    setGpsTrackRecording(false);
    return;
  }
  if (!gps.fix_valid) return;

  if (!ds->isGpsTrackRecordingOpen()) {
    const uint32_t start_ts = gps_utc_epoch_s();
    if (!start_ts) return;
    if (!ds->beginGpsTrackRecording(start_ts)) {
      showAlert("Track start failed", 2000);
      _gps_track_armed = false;
      _gps_track_has_last = false;
      _gps_track_point_count = 0;
      return;
    }
    _gps_track_start_ms = millis();
    _gps_track_has_last = false;
    _gps_track_point_count = 0;
  }

  const int32_t lat_e6 = gps.lat_micro;
  const int32_t lon_e6 = gps.lon_micro;
  const uint32_t now_ms = millis();
  const uint32_t off_ms = now_ms - _gps_track_start_ms;

  auto remember_last_point = [this, lat_e6, lon_e6, now_ms]() {
    _gps_track_last_lat_e6 = lat_e6;
    _gps_track_last_lon_e6 = lon_e6;
    _gps_track_has_last = true;
  };

  if (!_gps_track_has_last) {
    if (ds->appendGpsTrackPoint(off_ms, lat_e6, lon_e6)) {
      remember_last_point();
      if (++_gps_track_point_count >= HELTEC_GPS_TRACK_MAX_POINTS) setGpsTrackRecording(false);
    }
    return;
  }

  if (should_record_track_point(lat_e6, lon_e6, _gps_track_last_lat_e6, _gps_track_last_lon_e6) &&
      ds->appendGpsTrackPoint(off_ms, lat_e6, lon_e6)) {
    remember_last_point();
    if (++_gps_track_point_count >= HELTEC_GPS_TRACK_MAX_POINTS) setGpsTrackRecording(false);
  }
#endif
}

}  // namespace heltec::meshcore::biz
