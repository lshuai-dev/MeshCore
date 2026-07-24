#include "mesh_app_ui.hpp"

#include <lvgl.h>

#include "HeltecMesh.h"
#include "config/DataStore.h"
#include "target.h"

#include <helpers/BaseChatMesh.h>
#include <helpers/ContactInfo.h>

#include <math.h>
#include <string.h>

#ifndef COMPASS_HEADING_OFFSET_DEG
#define COMPASS_HEADING_OFFSET_DEG 0
#endif

#ifndef FF_NAV_ARRIVED_HINT_M
#define FF_NAV_ARRIVED_HINT_M 3.0
#endif

namespace heltec::meshcore::biz {
namespace {

constexpr uint32_t kAdvIntervalSec[] = {30, 60, 180, 300, 600};
constexpr const char* kAdvIntervalLabels[] = {"30s", "1m", "3m", "5m", "10m"};

constexpr double kPi = 3.14159265358979323846;
constexpr double kDeg2Rad = kPi / 180.0;
constexpr double kRad2Deg = 180.0 / kPi;
constexpr double kWgs84A = 6378137.0;
constexpr double kWgs84F = 1.0 / 298.257223563;
constexpr double kWgs84B = (1.0 - kWgs84F) * kWgs84A;

inline double microdeg_to_deg(int32_t micro) { return (double)micro * 1.0e-6; }

float wrap_heading_360(float deg) {
  deg = fmodf(deg, 360.0f);
  if (deg < 0.f) deg += 360.0f;
  return deg;
}

double wrap_heading_360_d(double deg) {
  deg = fmod(deg, 360.0);
  if (deg < 0.0) deg += 360.0;
  return deg;
}

float normalize_turn180(float deg) {
  deg = fmodf(deg + 540.0f, 360.0f) - 180.0f;
  return deg;
}

bool geodesic_inverse_spherical(double lat1_deg, double lon1_deg, double lat2_deg, double lon2_deg,
                                double& distance_m, double& bearing_deg) {
  const double phi1 = lat1_deg * kDeg2Rad;
  const double phi2 = lat2_deg * kDeg2Rad;
  const double dphi = (lat2_deg - lat1_deg) * kDeg2Rad;
  const double dlambda = (lon2_deg - lon1_deg) * kDeg2Rad;
  const double a =
      sin(dphi / 2) * sin(dphi / 2) + cos(phi1) * cos(phi2) * sin(dlambda / 2) * sin(dlambda / 2);
  const double c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
  distance_m = kWgs84A * c;

  const double y = sin(dlambda) * cos(phi2);
  const double x = cos(phi1) * sin(phi2) - sin(phi1) * cos(phi2) * cos(dlambda);
  bearing_deg = wrap_heading_360_d(atan2(y, x) * kRad2Deg);
  return true;
}

bool geodesic_inverse_wgs84(double lat1_deg, double lon1_deg, double lat2_deg, double lon2_deg,
                            double& distance_m, double& bearing_deg) {
  if (lat1_deg == lat2_deg && lon1_deg == lon2_deg) {
    distance_m = 0.0;
    bearing_deg = 0.0;
    return true;
  }

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
    if (sinSigma == 0.0) {
      distance_m = 0.0;
      bearing_deg = wrap_heading_360_d(atan2(t1, t2) * kRad2Deg);
      return true;
    }
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
    return geodesic_inverse_spherical(lat1_deg, lon1_deg, lat2_deg, lon2_deg, distance_m, bearing_deg);
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
  distance_m = kWgs84B * A * (sigma - deltaSigma);

  bearing_deg =
      wrap_heading_360_d(atan2(cosU2 * sin(lambda), cosU1 * sinU2 - sinU1 * cosU2 * cos(lambda)) *
                         kRad2Deg);
  return true;
}

int32_t deg_to_e6(double deg) {
  return (int32_t)(deg * 1000000.0 + (deg >= 0.0 ? 0.5 : -0.5));
}

double e6_to_deg(int32_t e6) { return ((double)e6) / 1000000.0; }

void format_e6_coord(char* buf, size_t len, int32_t e6) {
  if (!buf || len == 0) return;
  const int32_t a = e6 < 0 ? -e6 : e6;
  lv_snprintf(buf, len, "%s%d.%06ld", e6 < 0 ? "-" : "", a / 1000000, (long)(a % 1000000));
}

void format_e6_pair(char* buf, size_t len, int32_t lat_e6, int32_t lon_e6) {
  if (!buf || len == 0) return;
  char la[16];
  char lo[16];
  format_e6_coord(la, sizeof(la), lat_e6);
  format_e6_coord(lo, sizeof(lo), lon_e6);
  lv_snprintf(buf, len, "%s,%s", la, lo);
}

bool contact_has_gps(const ContactInfo& c) {
  return c.gps_lat != 0 || c.gps_lon != 0;
}

int adv_interval_index(uint32_t sec) {
  for (int i = 0; i < 5; ++i) {
    if (kAdvIntervalSec[i] == sec) return i;
  }
  return 1;
}

bool load_ff_settings(int& mode, int& wp_valid, double& wp_lat, double& wp_lon, uint16_t& track_cm,
                      int* friend_idx_out = nullptr, uint32_t* adv_sec_out = nullptr,
                      uint16_t* track_interval_min_out = nullptr) {
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  DataStore* ds = the_mesh.getDataStore();
  if (!ds) return false;
  return ds->loadFindFriendCompassSettings(mode, wp_valid, wp_lat, wp_lon, track_cm, friend_idx_out,
                                           adv_sec_out, track_interval_min_out);
#else
  (void)mode;
  (void)wp_valid;
  (void)wp_lat;
  (void)wp_lon;
  (void)track_cm;
  (void)friend_idx_out;
  (void)adv_sec_out;
  return false;
#endif
}

void save_ff_settings(int mode, int wp_valid, double wp_lat, double wp_lon, uint16_t track_cm,
                      int friend_idx, uint32_t adv_sec, uint16_t track_interval_min) {
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  DataStore* ds = the_mesh.getDataStore();
  if (!ds) return;
  ds->saveFindFriendCompassSettings(mode, wp_valid, wp_lat, wp_lon, track_cm, friend_idx, adv_sec,
                                    track_interval_min);
#else
  (void)mode;
  (void)wp_valid;
  (void)wp_lat;
  (void)wp_lon;
  (void)track_cm;
  (void)friend_idx;
  (void)adv_sec;
  (void)track_interval_min;
#endif
}

}  // namespace

void MeshAppUi::persistFfPrefs(int mode, int wp_valid, double wp_lat, double wp_lon) const {
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  DataStore* ds = the_mesh.getDataStore();
  if (!ds) return;
  ds->saveFindFriendCompassSettings(mode, wp_valid, wp_lat, wp_lon, HELTEC_GPS_TRACK_MIN_DIST_CM,
                                    findFriendTargetContactIndex(),
                                    HeltecMesh::locShareAdvertIntervalSec(), _ff_gps_track_interval_min);
#else
  (void)mode;
  (void)wp_valid;
  (void)wp_lat;
  (void)wp_lon;
#endif
}

void MeshAppUi::resetFindFriendNavState() const {}

void MeshAppUi::syncFfCacheFromStore() const {
  int mode = 0;
  int wp_valid = 0;
  double la = 0;
  double lo = 0;
  uint16_t track_cm = 100;
  if (load_ff_settings(mode, wp_valid, la, lo, track_cm)) {
    _ff_mode = mode;
    _ff_wp_valid = wp_valid;
    _ff_wp_lat_e6 = deg_to_e6(la);
    _ff_wp_lon_e6 = deg_to_e6(lo);
  }
}

void MeshAppUi::setFfWaypointCache(double lat_deg, double lon_deg) {
  _ff_mode = 1;
  _ff_wp_valid = 1;
  _ff_wp_lat_e6 = deg_to_e6(lat_deg);
  _ff_wp_lon_e6 = deg_to_e6(lon_deg);
  resetFindFriendNavState();
}

void MeshAppUi::ensureFindFriendPrefsLoaded() const {
  if (_ff_prefs_loaded) return;
  _ff_prefs_loaded = true;

  int mode = 0;
  int wp_valid = 0;
  double la = 0;
  double lo = 0;
  uint16_t track_cm = 100;
  uint16_t track_interval_min = 1;
  int friend_idx = -1;
  uint32_t adv_sec = 0;
  const bool loaded =
      load_ff_settings(mode, wp_valid, la, lo, track_cm, &friend_idx, &adv_sec, &track_interval_min);
  if (!loaded) return;

  _ff_mode = mode;
  _ff_wp_valid = wp_valid;
  _ff_wp_lat_e6 = deg_to_e6(la);
  _ff_wp_lon_e6 = deg_to_e6(lo);
  _ff_gps_track_interval_min = track_interval_min;

  const int n = findFriendContactCount();
  if (friend_idx >= 0 && friend_idx < n && findFriendContactHasGps(friend_idx)) {
    _ff_target_contact_idx = friend_idx;
  } else {
    _ff_target_contact_idx = -1;
  }
  if (adv_sec > 0) HeltecMesh::setLocShareAdvertIntervalSec(adv_sec);
}

bool MeshAppUi::locationShareEnabled() const {
  const NodePrefs* p = the_mesh.getNodePrefs();
  return HeltecMesh::isLocationShareEnabled(p);
}

void MeshAppUi::setLocationShareEnabled(bool enabled) {
  HeltecMesh::setLocationShareEnabled(the_mesh, enabled, true);
  notifyAppState(heltec::meshcore::ui::AppStateEventType::ConfigChanged);
  notifyAppState(heltec::meshcore::ui::AppStateEventType::FindFriendChanged);
}

int MeshAppUi::locShareIntervalIndex() const {
  ensureFindFriendPrefsLoaded();
  return adv_interval_index(HeltecMesh::locShareAdvertIntervalSec());
}

void MeshAppUi::setLocShareIntervalIndex(int index) {
  ensureFindFriendPrefsLoaded();
  if (index < 0) index = 0;
  if (index > 4) index = 4;
  const uint32_t sec = kAdvIntervalSec[index];
  HeltecMesh::setLocShareAdvertIntervalSec(sec);

  NodePrefs* p = the_mesh.getNodePrefs();
  if (p && p->loc_share_adv_sec != sec) {
    p->loc_share_adv_sec = sec;
    the_mesh.savePrefs();
  }

#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  int mode = 0;
  int wp_valid = 0;
  double la = 0;
  double lo = 0;
  uint16_t track_cm = 100;
  load_ff_settings(mode, wp_valid, la, lo, track_cm);
  persistFfPrefs(mode, wp_valid, la, lo);
#endif
  notifyAppState(heltec::meshcore::ui::AppStateEventType::ConfigChanged);
  notifyAppState(heltec::meshcore::ui::AppStateEventType::FindFriendChanged);
}

int MeshAppUi::locShareIntervalOptionCount() const { return 5; }

const char* MeshAppUi::locShareIntervalOptionLabel(int index) const {
  if (index < 0 || index > 4) return "?";
  return kAdvIntervalLabels[index];
}

int MeshAppUi::findFriendMode() const {
  ensureFindFriendPrefsLoaded();
  return _ff_mode;
}

bool MeshAppUi::setFindFriendMode(int mode) {
  ensureFindFriendPrefsLoaded();

  int wp_valid = 0;
  double la = 0;
  double lo = 0;
  uint16_t track_cm = 100;
  int cur_mode = 0;
  load_ff_settings(cur_mode, wp_valid, la, lo, track_cm);
  _ff_mode = mode ? 1 : 0;
  persistFfPrefs(_ff_mode, wp_valid, la, lo);
  notifyAppState(heltec::meshcore::ui::AppStateEventType::ConfigChanged);
  notifyAppState(heltec::meshcore::ui::AppStateEventType::FindFriendChanged);
  return true;
}

bool MeshAppUi::setFindFriendWaypoint(double lat_deg, double lon_deg) {
  if (lat_deg < -90.0 || lat_deg > 90.0 || lon_deg < -180.0 || lon_deg > 180.0) return false;
  ensureFindFriendPrefsLoaded();

  int mode = 1;
  int wp_valid = 1;
  uint16_t track_cm = 100;
  int cur_mode = 0;
  int cur_wp = 0;
  double dummy_la = 0;
  double dummy_lo = 0;
  load_ff_settings(cur_mode, cur_wp, dummy_la, dummy_lo, track_cm);
  setFfWaypointCache(lat_deg, lon_deg);
  persistFfPrefs(1, wp_valid, e6_to_deg(_ff_wp_lat_e6), e6_to_deg(_ff_wp_lon_e6));
  notifyAppState(heltec::meshcore::ui::AppStateEventType::FindFriendChanged);
  return true;
}

bool MeshAppUi::saveFindFriendWaypointFromGps() {
  ensureFindFriendPrefsLoaded();
  const GpsStatus gps = gpsStatus();
  if (!gps.fix_valid) return false;
  return setFindFriendWaypoint(gps.lat_deg, gps.lon_deg);
}

void MeshAppUi::formatFindFriendWaypointInput(char* buf, size_t buf_len) const {
  if (!buf || buf_len == 0) return;
  buf[0] = '\0';
  ensureFindFriendPrefsLoaded();
  syncFfCacheFromStore();
  if (!_ff_wp_valid) return;
  format_e6_pair(buf, buf_len, _ff_wp_lat_e6, _ff_wp_lon_e6);
}

int MeshAppUi::findFriendContactCount() const { return the_mesh.getNumContacts(); }

bool MeshAppUi::findFriendContactLabel(int index, char* buf, size_t buf_len) const {
  if (!buf || buf_len == 0 || index < 0) return false;
  ContactInfo c{};
  if (!the_mesh.getContactByIdx((uint32_t)index, c)) return false;
  lv_snprintf(buf, buf_len, "%s", c.name[0] ? c.name : "?");
  return true;
}

int MeshAppUi::buildFindFriendDropdownOptions(char* buf, size_t buf_len, int16_t* mesh_map,
                                              int mesh_map_cap) const {
  if (!buf || buf_len == 0) return 0;

  const int n = findFriendContactCount();
  int order[MAX_CONTACTS];
  uint32_t mods[MAX_CONTACTS];
  int count = 0;
  if (n > MAX_CONTACTS) return 0;

  for (int i = 0; i < n; ++i) {
    ContactInfo c{};
    if (!the_mesh.getContactByIdx((uint32_t)i, c)) continue;
    if (!contact_has_gps(c)) continue;
    order[count] = i;
    mods[count] = c.lastmod;
    ++count;
  }

  for (int i = 0; i < count - 1; ++i) {
    for (int j = i + 1; j < count; ++j) {
      if (mods[j] > mods[i]) {
        const int ti = order[i];
        order[i] = order[j];
        order[j] = ti;
        const uint32_t tm = mods[i];
        mods[i] = mods[j];
        mods[j] = tm;
      }
    }
  }

  const int prefix = lv_snprintf(buf, buf_len, "(none)");
  if (prefix < 0 || (size_t)prefix >= buf_len) return 0;
  if (count <= 0) return 0;

  char* p = buf + prefix;
  size_t rem = buf_len - (size_t)prefix;
  int listed = 0;
  for (int k = 0; k < count; ++k) {
    char lab[32];
    if (!findFriendContactLabel(order[k], lab, sizeof(lab))) continue;
    const int written = lv_snprintf(p, rem, "\n%s", lab);
    if (written < 0 || (size_t)written >= rem) break;
    if (mesh_map && listed < mesh_map_cap) mesh_map[listed] = static_cast<int16_t>(order[k]);
    p += written;
    rem -= (size_t)written;
    ++listed;
  }
  return listed;
}

bool MeshAppUi::findFriendContactHasGps(int index) const {
  if (index < 0) return false;
  ContactInfo c{};
  if (!the_mesh.getContactByIdx((uint32_t)index, c)) return false;
  return contact_has_gps(c);
}

int MeshAppUi::findFriendTargetContactIndex() const {
  ensureFindFriendPrefsLoaded();
  if (_ff_target_contact_idx >= 0) {
    ContactInfo c{};
    if (!the_mesh.getContactByIdx((uint32_t)_ff_target_contact_idx, c)) {
      _ff_target_contact_idx = -1;
    }
  }
  return _ff_target_contact_idx;
}

void MeshAppUi::setFindFriendTargetContactIndex(int index) {
  ensureFindFriendPrefsLoaded();
  const int n = findFriendContactCount();
  if (index < 0 || index >= n || !findFriendContactHasGps(index)) {
    _ff_target_contact_idx = -1;
  } else {
    _ff_target_contact_idx = index;
  }

  int mode = 0;
  int wp_valid = 0;
  double la = 0;
  double lo = 0;
  uint16_t track_cm = 100;
  load_ff_settings(mode, wp_valid, la, lo, track_cm);
  persistFfPrefs(mode, wp_valid, la, lo);
  resetFindFriendNavState();
  notifyAppState(heltec::meshcore::ui::AppStateEventType::FindFriendChanged);
}

void MeshAppUi::syncFindFriendContactList() {
  ensureFindFriendPrefsLoaded();
  const int n = findFriendContactCount();
  if (_ff_target_contact_idx >= n ||
      (_ff_target_contact_idx >= 0 && !findFriendContactHasGps(_ff_target_contact_idx))) {
    _ff_target_contact_idx = -1;
    notifyAppState(heltec::meshcore::ui::AppStateEventType::FindFriendChanged);
  }
}

bool MeshAppUi::tryAutoPickFindFriendTarget() {
  ensureFindFriendPrefsLoaded();
  const int n = findFriendContactCount();
  if (n <= 0) return false;
  int idx = _ff_target_contact_idx;
  if (idx < 0) idx = 0;
  for (int step = 0; step < n; ++step) {
    if (findFriendContactHasGps(idx)) {
      _ff_target_contact_idx = idx;
      resetFindFriendNavState();
      return true;
    }
    idx = (idx + 1 + n) % n;
  }
  return false;
}

void MeshAppUi::cycleFindFriendTargetContact(int delta) {
  ensureFindFriendPrefsLoaded();
  const int n = findFriendContactCount();
  if (n <= 0) {
    _ff_target_contact_idx = -1;
    return;
  }
  int idx = _ff_target_contact_idx;
  if (idx < 0) idx = 0;
  for (int step = 0; step < n; ++step) {
    idx = (idx + delta + n) % n;
    if (findFriendContactHasGps(idx)) {
      _ff_target_contact_idx = idx;
      int mode = 0;
      int wp_valid = 0;
      double la = 0;
      double lo = 0;
      uint16_t track_cm = 100;
      load_ff_settings(mode, wp_valid, la, lo, track_cm);
      persistFfPrefs(mode, wp_valid, la, lo);
      resetFindFriendNavState();
      return;
    }
  }
  _ff_target_contact_idx = -1;
}

FindFriendUi MeshAppUi::findFriendUi() const {
  ensureFindFriendPrefsLoaded();
  FindFriendUi u{};
  const GpsStatus gps = gpsStatus();
  u.gps_fix = gps.fix_valid;
  u.here_lat = microdeg_to_deg(gps.lat_micro);
  u.here_lon = microdeg_to_deg(gps.lon_micro);

#if ENV_INCLUDE_COMPASS
  const CompassUi& cm = _compass_ui;
  u.compass_hw = cm.has_hardware;
  u.heading_valid = cm.heading_valid;
  u.heading_deg = cm.heading_deg;

  syncFfCacheFromStore();
  u.mode = _ff_mode;

  u.waypoint_valid = (_ff_wp_valid != 0);
  u.waypoint_lat = microdeg_to_deg(_ff_wp_lat_e6);
  u.waypoint_lon = microdeg_to_deg(_ff_wp_lon_e6);

  double tgt_lat = 0;
  double tgt_lon = 0;
  bool have_tgt = false;

  if (_ff_mode == 0) {
    if (_ff_target_contact_idx >= 0) {
      ContactInfo c{};
      if (the_mesh.getContactByIdx((uint32_t)_ff_target_contact_idx, c)) {
        lv_snprintf(u.target_label, sizeof(u.target_label), "%s", c.name[0] ? c.name : "?");
        if (contact_has_gps(c)) {
          tgt_lat = microdeg_to_deg(c.gps_lat);
          tgt_lon = microdeg_to_deg(c.gps_lon);
          have_tgt = true;
        }
      } else {
        lv_snprintf(u.target_label, sizeof(u.target_label), "--");
      }
    } else {
      lv_snprintf(u.target_label, sizeof(u.target_label), "--");
    }
  } else {
    have_tgt = u.waypoint_valid;
    if (have_tgt) {
      tgt_lat = microdeg_to_deg(_ff_wp_lat_e6);
      tgt_lon = microdeg_to_deg(_ff_wp_lon_e6);
    }
    lv_snprintf(u.target_label, sizeof(u.target_label), "WP");
  }

  u.target_valid = have_tgt;

  if (!have_tgt || !u.gps_fix) {
    u.bearing_valid = false;
    u.relative_valid = false;
  }

  if (have_tgt && u.gps_fix) {
    const double here_lat = u.here_lat;
    const double here_lon = u.here_lon;
    double geodesic_bearing = 0.0;
    geodesic_inverse_wgs84(here_lat, here_lon, tgt_lat, tgt_lon, u.distance_m, geodesic_bearing);
    u.arrived = u.distance_m < FF_NAV_ARRIVED_HINT_M;
    u.bearing_valid = true;
    u.bearing_to_waypoint_deg = (float)geodesic_bearing;

    if (u.heading_valid) {
      const float device_hdg =
          wrap_heading_360(u.heading_deg + (float)COMPASS_HEADING_OFFSET_DEG);
      u.turn_deg = normalize_turn180((float)geodesic_bearing - device_hdg);
      u.relative_valid = !u.arrived;
    }
  }
#endif

  return u;
}

}  // namespace heltec::meshcore::biz
