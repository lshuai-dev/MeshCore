#include "mesh_app_ui.hpp"

#include "HeltecMesh.h"
#include "config/NodePrefs.h"
#include "heltec/drivers/display/display_port.hpp"
#include "ui/core/app_state_notifier.hpp"

#include "target.h"

#include <helpers/SensorManager.h>
#include <helpers/sensors/LocationProvider.h>
#include <math.h>

namespace heltec::meshcore::biz {
namespace {
constexpr uint32_t kGpsScreenOffGraceMs = 30000;
constexpr uint32_t kLocShareAcquireWindowMs = 30000;
constexpr uint32_t kGpsSpeedMinSampleMs = 200;
constexpr uint32_t kGpsSpeedIdleSampleMs = 1000;
constexpr double kEarthRadiusM = 6371000.0;
constexpr double kMicroDegreeToRad = 3.14159265358979323846 / 180000000.0;
}  // namespace

bool MeshAppUi::externalPowerForGps() {
  const uint32_t now_ms = millis();
  if (!_gps_external_power_known || (int32_t)(now_ms - _gps_external_power_next_poll_ms) >= 0) {
    _gps_external_powered = board.isExternalPowered();
    _gps_external_power_known = true;
    _gps_external_power_next_poll_ms = now_ms + 1000;
  }
  return _gps_external_powered;
}

bool MeshAppUi::applyGpsPowerPolicy(bool* changed) {
  if (changed) *changed = false;
#if !defined(ENV_INCLUDE_GPS) || !(ENV_INCLUDE_GPS)
  return true;
#else
  NodePrefs* p = the_mesh.getNodePrefs();
  LocationProvider* location = sensors.getLocationProvider();
  if (!p || !location) return false;

  const uint32_t now_ms = millis();
  const bool gps_enabled = p->gps_enabled != 0;
  const bool external_powered = externalPowerForGps();
  const bool display_on = heltec::meshcore::dal::display_port::isBacklightOn();
  if (!_gps_display_state_known || display_on != _gps_display_was_on) {
    _gps_display_state_known = true;
    _gps_display_was_on = display_on;
    _gps_display_off_since_ms = display_on ? 0 : now_ms;
  }
  const bool display_grace =
      !display_on && (now_ms - _gps_display_off_since_ms) < kGpsScreenOffGraceMs;

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
  const bool find_friend_foreground =
      _find_friend_foreground_active && find_friend_enabled && (display_on || display_grace);
  const bool gps_screen_foreground =
      _gps_foreground_active && (display_on || display_grace);
  const bool map_screen_foreground =
      _map_foreground_active && (display_on || display_grace);
  const bool desired = gps_enabled &&
                       (external_powered || track_active || share_continuous ||
                        share_wakeup_window || gps_screen_foreground ||
                        map_screen_foreground || find_friend_foreground);
  bool state_matches = location->isEnabled() == desired;
#if defined(HELTEC_SENSOR_MANAGER) && HELTEC_SENSOR_MANAGER
  state_matches = state_matches && sensors.isGpsActive() == desired;
#endif
  if (state_matches) return true;

  if (!sensors.setSettingValue("gps", desired ? "1" : "0")) return false;
  if (changed) *changed = true;
  MESH_DEBUG_PRINTLN("[gps] power=%s external=%u display=%u grace=%u share=%u interval=%lu "
                     "share_cont=%u share_wake=%u due_ms=%ld track=%u gps_fg=%u map_fg=%u "
                     "find=%u ff_fg=%u",
                     desired ? "on" : "off", external_powered ? 1 : 0,
                     display_on ? 1 : 0, display_grace ? 1 : 0,
                     location_share ? 1 : 0, static_cast<unsigned long>(share_interval_sec),
                     share_continuous ? 1 : 0, share_wakeup_window ? 1 : 0,
                     static_cast<long>(share_due_ms), track_active ? 1 : 0,
                     gps_screen_foreground ? 1 : 0,
                     map_screen_foreground ? 1 : 0,
                     find_friend_enabled ? 1 : 0, find_friend_foreground ? 1 : 0);
  return true;
#endif
}

void MeshAppUi::reconcileGpsPower() {
  bool changed = false;
  if (applyGpsPowerPolicy(&changed) && changed) notifyGpsChanged();
}

void MeshAppUi::setFindFriendForegroundActive(bool active) {
  _find_friend_foreground_active = active;
}

void MeshAppUi::setGpsForegroundActive(bool active) {
  _gps_foreground_active = active;
}

void MeshAppUi::setMapForegroundActive(bool active) {
  _map_foreground_active = active;
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
  s.powered = nmea && nmea->isEnabled();
#if defined(HELTEC_SENSOR_MANAGER) && HELTEC_SENSOR_MANAGER
  s.fix_valid_ms = sensors.gpsFixValidMs();
#endif
  if (!s.enabled || !s.powered || !nmea) {
    reset_speed();
    return s;
  }

  s.fix_valid = nmea->isValid();
#if defined(HELTEC_SENSOR_MANAGER) && HELTEC_SENSOR_MANAGER
  s.fix_valid = s.fix_valid && sensors.hasFreshGpsFix();
#endif
  if (!s.fix_valid) {
    reset_speed();
    return s;
  }

  s.satellites = nmea->satellitesCount();
  s.lat_micro = nmea->getLatitude();
  s.lon_micro = nmea->getLongitude();
  s.lat_deg = s.lat_micro / 1000000.0;
  s.lon_deg = s.lon_micro / 1000000.0;
  s.alt_m = nmea->getAltitude() / 1000.0;

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
