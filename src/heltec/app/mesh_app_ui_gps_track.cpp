#include "mesh_app_ui.hpp"

#include "HeltecMesh.h"
#include "config/DataStore.h"
#include "config/NodePrefs.h"
#include "geodesic.hpp"
#include "target.h"

#include <Arduino.h>
#include <helpers/SensorManager.h>
#include <helpers/sensors/LocationProvider.h>

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

bool should_record_track_point(int32_t lat_e6, int32_t lon_e6, int32_t last_lat_e6, int32_t last_lon_e6) {
  if (lat_e6 == last_lat_e6 && lon_e6 == last_lon_e6) return false;

  const double moved_m = geo::geodesic_distance_m(
      (double)last_lat_e6 * 1.0e-6, (double)last_lon_e6 * 1.0e-6,
      (double)lat_e6 * 1.0e-6, (double)lon_e6 * 1.0e-6);
  const double min_m = (double)HELTEC_GPS_TRACK_MIN_DIST_CM / 100.0;
  return moved_m + 1e-6 >= min_m;
}

}  // namespace

bool MeshAppUi::gpsTrackRecording() const {
#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
  return _gps_track_armed;
#else
  return false;
#endif
}

bool MeshAppUi::setGpsTrackRecording(bool enabled) {
#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
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
    reconcileGpsPower();
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
  reconcileGpsPower();
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
#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
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
