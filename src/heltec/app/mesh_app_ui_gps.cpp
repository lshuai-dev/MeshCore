#include "mesh_app_ui.hpp"

#include "HeltecMesh.h"
#include "config/NodePrefs.h"
#include "ui/core/app_state_notifier.hpp"

#include "target.h"

#include <helpers/SensorManager.h>
#include <helpers/sensors/LocationProvider.h>

namespace heltec::meshcore::biz {

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
    if (!sensors.setSettingValue("gps", enabled ? "1" : "0")) return;
    return;
  }

  p->gps_enabled = enabled ? 1 : 0;
  if (!sensors.setSettingValue("gps", enabled ? "1" : "0")) {
    p->gps_enabled = pref_on ? 1 : 0;
    return;
  }

#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  if (!enabled) {
    if (gpsTrackRecording()) {
      setGpsTrackRecording(false);
    } else if (p->gps_track_armed) {
      p->gps_track_armed = 0;
      the_mesh.savePrefs();
    }
  }
#else
  if (!enabled && p->gps_track_armed) {
    p->gps_track_armed = 0;
    the_mesh.savePrefs();
  }
#endif

  if (!enabled && HeltecMesh::isLocationShareEnabled(p)) {
    HeltecMesh::setLocationShareEnabled(the_mesh, false, true);
  }

  the_mesh.savePrefs();
  notifyAppState(heltec::meshcore::ui::AppStateEventType::ConfigChanged);
  const GpsStatus gps = gpsStatus();
  heltec::meshcore::ui::AppStateEvent event{};
  event.type = heltec::meshcore::ui::AppStateEventType::GpsChanged;
  event.gps.enabled = gps.enabled;
  event.gps.available = gps.available;
  event.gps.fix_valid = gps.fix_valid;
  event.gps.satellites = gps.satellites;
  event.gps.lat_micro = gps.lat_micro;
  event.gps.lon_micro = gps.lon_micro;
  event.gps.lat_deg = gps.lat_deg;
  event.gps.lon_deg = gps.lon_deg;
  event.gps.alt_m = gps.alt_m;
  heltec::meshcore::ui::app_state_notifier().notify(event);
}

IBizFacade::GpsStatus MeshAppUi::gpsStatus() const {
  GpsStatus s;
  LocationProvider* nmea = sensors.getLocationProvider();
  s.available = (nmea != nullptr);

  NodePrefs* p = the_mesh.getNodePrefs();
  if (p) {
    s.enabled = p->gps_enabled != 0;
  } else if (nmea) {
    s.enabled = nmea->isEnabled();
  }
  if (!s.enabled || !nmea) return s;

  s.fix_valid = nmea->isValid();
  if (!s.fix_valid) return s;

  s.satellites = nmea->satellitesCount();
  s.lat_micro = nmea->getLatitude();
  s.lon_micro = nmea->getLongitude();
  s.lat_deg = s.lat_micro / 1000000.0;
  s.lon_deg = s.lon_micro / 1000000.0;
  s.alt_m = nmea->getAltitude() / 1000.0;

  return s;
}

}  // namespace heltec::meshcore::biz
