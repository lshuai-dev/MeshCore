#include "mesh_app_ui.hpp"

#include "HeltecMesh.h"
#include "config/NodePrefs.h"
#include "ui/core/app_state_notifier.hpp"

#include "target.h"

#include <helpers/SensorManager.h>
#include <helpers/sensors/LocationProvider.h>
#include <math.h>

namespace heltec::meshcore::biz {
namespace {
constexpr uint32_t kLocShareAcquireWindowMs = 30000;
constexpr uint32_t kGpsSpeedMinSampleMs = 200;
constexpr uint32_t kGpsSpeedIdleSampleMs = 1000;
constexpr double kEarthRadiusM = 6371000.0;
constexpr double kMicroDegreeToRad = 3.14159265358979323846 / 180000000.0;
}  // namespace

bool MeshAppUi::applyGpsPowerPolicy() {
#if !defined(ENV_INCLUDE_GPS) || !(ENV_INCLUDE_GPS)
  _power.setGpsAllowed(false);
  _power.setGpsDemand(power::GpsDemand::Track, false);
  _power.setGpsDemand(power::GpsDemand::LocationContinuous, false);
  _power.setGpsDemand(power::GpsDemand::LocationAcquire, false);
  _power.setGpsDemand(power::GpsDemand::FindFriend, false);
  return true;
#else
  NodePrefs* p = the_mesh.getNodePrefs();
  LocationProvider* location = sensors.getLocationProvider();
  if (!p || !location) return false;

  const uint32_t now_ms = millis();
  const bool gps_enabled = p->gps_enabled != 0;

  const bool location_share = HeltecMesh::isLocationShareEnabled(p);
  const uint32_t share_interval_sec = HeltecMesh::locShareAdvertIntervalSec();
  const bool share_continuous =
      location_share && share_interval_sec > 0 && share_interval_sec <= 30;
  int32_t share_due_ms = -1;
  bool share_wakeup_window = false;
  const uint32_t next_advert_ms = HeltecMesh::locShareNextAdvertMillis();
  if (location_share && share_interval_sec > 30 && next_advert_ms != 0) {
    share_due_ms = static_cast<int32_t>(next_advert_ms - now_ms);
    share_wakeup_window = share_due_ms <= static_cast<int32_t>(kLocShareAcquireWindowMs);
  }

  const bool track_active = gpsTrackRecording();
  const bool find_friend_enabled =
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
      findFriendEnabled();
#else
      false;
#endif
  // Active friend navigation needs a continuous fix. Keep GPS powered while
  // this screen is foreground even if the backlight times out.
  const bool find_friend_foreground =
      _find_friend_foreground_active && find_friend_enabled;

  _power.setGpsAllowed(gps_enabled);
  _power.setGpsDemand(power::GpsDemand::Track, track_active);
  _power.setGpsDemand(power::GpsDemand::LocationContinuous, share_continuous);
  _power.setGpsDemand(power::GpsDemand::LocationAcquire, share_wakeup_window);
  _power.setGpsDemand(power::GpsDemand::FindFriend, find_friend_foreground);
  return _power.reconcileGpsPower(now_ms);
#endif
}

void MeshAppUi::reconcileGpsPower() {
  (void)applyGpsPowerPolicy();
}

void MeshAppUi::setFindFriendForegroundActive(bool active) {
  _find_friend_foreground_active = active;
  reconcileGpsPower();
}

void MeshAppUi::setGpsForegroundActive(bool active) {
  _power.setGpsDemand(power::GpsDemand::GpsScreen, active);
  (void)_power.reconcileGpsPower(millis());
}

void MeshAppUi::setMapForegroundActive(bool active) {
  _power.setGpsDemand(power::GpsDemand::MapScreen, active);
  (void)_power.reconcileGpsPower(millis());
}

void MeshAppUi::notifyGpsChanged() {
  const GpsStatus gps = gpsStatus();
  heltec::meshcore::ui::AppStateEvent event{};
  event.type = heltec::meshcore::ui::AppStateEventType::GpsChanged;
  event.gps.enabled = gps.enabled;
  event.gps.available = gps.available;
  event.gps.powered = gps.powered;
  event.gps.fix_valid = gps.fix_valid;
  event.gps.fix_valid_ms = gps.fix_valid_ms;
  event.gps.satellites = gps.satellites;
  event.gps.lat_micro = gps.lat_micro;
  event.gps.lon_micro = gps.lon_micro;
  event.gps.lat_deg = gps.lat_deg;
  event.gps.lon_deg = gps.lon_deg;
  event.gps.alt_m = gps.alt_m;
  event.gps.speed_kph = gps.speed_kph;
  heltec::meshcore::ui::app_state_notifier().notify(event);
}

bool MeshAppUi::toggleGPS() {
  NodePrefs* p = the_mesh.getNodePrefs();
  if (!p) return false;
  setGpsEnabled(p->gps_enabled == 0);
  return p->gps_enabled != 0;
}

void MeshAppUi::setGpsEnabled(bool enabled) {
  NodePrefs* p = the_mesh.getNodePrefs();
  if (!p) return;

  const bool pref_on = p->gps_enabled != 0;
  if (pref_on == enabled) {
    reconcileGpsPower();
    return;
  }

  p->gps_enabled = enabled ? 1 : 0;
  if (!applyGpsPowerPolicy()) {
    p->gps_enabled = pref_on ? 1 : 0;
    applyGpsPowerPolicy();
    return;
  }

#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
  if (!enabled) {
    if (gpsTrackRecording()) {
      setGpsTrackRecording(false);
    } else if (p->gps_track_armed) {
      p->gps_track_armed = 0;
      the_mesh.savePrefs();
    }
  }
#endif

  if (!enabled && HeltecMesh::isLocationShareEnabled(p)) {
    HeltecMesh::setLocationShareEnabled(the_mesh, false, true);
  }

#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  if (!enabled && findFriendEnabled()) {
    (void)setFindFriendEnabled(false);
  }
#endif

  the_mesh.savePrefs();
  notifyAppState(heltec::meshcore::ui::AppStateEventType::ConfigChanged);
  notifyGpsChanged();
}

IBizFacade::GpsStatus MeshAppUi::gpsStatus() const {
  GpsStatus s;
  const auto reset_speed = [this]() {
    _gps_speed_sample_valid = false;
    _gps_speed_sample_lat_e6 = 0;
    _gps_speed_sample_lon_e6 = 0;
    _gps_speed_sample_ms = 0;
    _gps_speed_kph = -1.0f;
  };
  LocationProvider* nmea = sensors.getLocationProvider();
  s.available = (nmea != nullptr);

  NodePrefs* p = the_mesh.getNodePrefs();
  if (p) {
    s.enabled = p->gps_enabled != 0;
  } else if (nmea) {
    s.enabled = nmea->isEnabled();
  }
  s.powered = _power.snapshot().gps_powered;
  if (!s.enabled || !s.powered || !nmea) {
    HeltecMesh::StableGpsFixSnapshot discarded{};
    (void)the_mesh.getStableGpsFix(discarded);
    reset_speed();
    return s;
  }

  HeltecMesh::StableGpsFixSnapshot snapshot{};
  s.fix_valid = the_mesh.getStableGpsFix(snapshot);
  s.fix_valid_ms = snapshot.age_ms;
  if (!s.fix_valid) {
    reset_speed();
    return s;
  }

  s.satellites = snapshot.satellites < 0
                     ? 0
                     : (snapshot.satellites > 255 ? 255 : static_cast<uint8_t>(snapshot.satellites));
  s.lat_micro = snapshot.lat_micro;
  s.lon_micro = snapshot.lon_micro;
  s.alt_m = snapshot.alt_milli / 1000.0;
  s.lat_deg = s.lat_micro / 1000000.0;
  s.lon_deg = s.lon_micro / 1000000.0;

  const uint32_t now_ms = millis();
  if (!_gps_speed_sample_valid) {
    _gps_speed_sample_valid = true;
    _gps_speed_sample_lat_e6 = static_cast<int32_t>(s.lat_micro);
    _gps_speed_sample_lon_e6 = static_cast<int32_t>(s.lon_micro);
    _gps_speed_sample_ms = now_ms;
    _gps_speed_kph = -1.0f;
  } else {
    const uint32_t elapsed_ms = now_ms - _gps_speed_sample_ms;
    const bool position_changed =
        s.lat_micro != _gps_speed_sample_lat_e6 ||
        s.lon_micro != _gps_speed_sample_lon_e6;
    if (elapsed_ms >= kGpsSpeedMinSampleMs &&
        (position_changed || elapsed_ms >= kGpsSpeedIdleSampleMs)) {
      if (position_changed) {
        const double lat1 = _gps_speed_sample_lat_e6 * kMicroDegreeToRad;
        const double lat2 = s.lat_micro * kMicroDegreeToRad;
        const double dlat = lat2 - lat1;
        double dlon = (s.lon_micro - _gps_speed_sample_lon_e6) * kMicroDegreeToRad;
        if (dlon > 3.14159265358979323846) dlon -= 2.0 * 3.14159265358979323846;
        if (dlon < -3.14159265358979323846) dlon += 2.0 * 3.14159265358979323846;
        const double x = dlon * cos((lat1 + lat2) * 0.5);
        const double distance_m = sqrt(dlat * dlat + x * x) * kEarthRadiusM;
        double speed_kph = distance_m * 3600000.0 / elapsed_ms;
        if (speed_kph < 0.0) speed_kph = 0.0;
        if (speed_kph > 999.9) speed_kph = 999.9;
        _gps_speed_kph = static_cast<float>(speed_kph);
      } else {
        _gps_speed_kph = 0.0f;
      }
      _gps_speed_sample_lat_e6 = static_cast<int32_t>(s.lat_micro);
      _gps_speed_sample_lon_e6 = static_cast<int32_t>(s.lon_micro);
      _gps_speed_sample_ms = now_ms;
    }
  }
  s.speed_kph = _gps_speed_kph;

  return s;
}

}  // namespace heltec::meshcore::biz
