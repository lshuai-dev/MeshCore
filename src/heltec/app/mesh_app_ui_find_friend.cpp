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

namespace heltec::meshcore::biz {
namespace {

#if defined(COMPASS_DECLINATION_DEG)
constexpr bool kCompassDeclinationConfigured = true;
#else
constexpr bool kCompassDeclinationConfigured = false;
#endif

constexpr uint32_t kAdvIntervalSec[] = {30, 60, 180, 300, 600};
constexpr const char* kAdvIntervalLabels[] = {"30s", "1m", "3m", "5m", "10m"};

inline double microdeg_to_deg(int32_t micro) { return (double)micro * 1.0e-6; }

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

bool stored_contact_coordinate_valid(const ContactInfo& c) {
  if (c.gps_lat == 0 && c.gps_lon == 0) return false;
  return nav::FindFriendNavEstimator::coordinateValid(microdeg_to_deg(c.gps_lat),
                                                       microdeg_to_deg(c.gps_lon));
}

bool contact_location(const ContactInfo& c, int32_t& lat_micro, int32_t& lon_micro,
                      HeltecMesh::ContactLocationReceipt* receipt_out = nullptr) {
  HeltecMesh::ContactLocationReceipt receipt{};
  const bool receipt_known = the_mesh.getContactLocationReceipt(c.id.pub_key, receipt);
  if (receipt_out) *receipt_out = receipt;
  if (receipt_known) {
    if (!receipt.advertised) return false;
    lat_micro = receipt.lat_micro;
    lon_micro = receipt.lon_micro;
    return nav::FindFriendNavEstimator::coordinateValid(microdeg_to_deg(lat_micro),
                                                         microdeg_to_deg(lon_micro));
  }
  if (!stored_contact_coordinate_valid(c)) return false;
  lat_micro = c.gps_lat;
  lon_micro = c.gps_lon;
  return true;
}

int adv_interval_index(uint32_t sec) {
  for (int i = 0; i < 5; ++i) {
    if (kAdvIntervalSec[i] == sec) return i;
  }
  return 1;
}

bool load_ff_settings(int& mode, int& wp_valid, double& wp_lat, double& wp_lon, uint16_t& track_cm,
                      int* friend_idx_out = nullptr, uint16_t* track_interval_min_out = nullptr,
                      bool* enabled_out = nullptr, uint8_t* friend_pub_key_out = nullptr,
                      bool* friend_pub_key_valid_out = nullptr) {
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  DataStore* ds = the_mesh.getDataStore();
  if (!ds) return false;
  return ds->loadFindFriendCompassSettings(mode, wp_valid, wp_lat, wp_lon, track_cm, friend_idx_out,
                                           track_interval_min_out, enabled_out,
                                           friend_pub_key_out, friend_pub_key_valid_out);
#else
  (void)mode;
  (void)wp_valid;
  (void)wp_lat;
  (void)wp_lon;
  (void)track_cm;
  (void)friend_idx_out;
  (void)track_interval_min_out;
  (void)enabled_out;
  (void)friend_pub_key_out;
  (void)friend_pub_key_valid_out;
  return false;
#endif
}

FindFriendTargetFreshness to_ui_freshness(nav::TargetFreshness freshness) {
  switch (freshness) {
    case nav::TargetFreshness::Unknown:
      return FindFriendTargetFreshness::Unknown;
    case nav::TargetFreshness::Fresh:
      return FindFriendTargetFreshness::Fresh;
    case nav::TargetFreshness::Aging:
      return FindFriendTargetFreshness::Aging;
    case nav::TargetFreshness::Stale:
      return FindFriendTargetFreshness::Stale;
    case nav::TargetFreshness::NotApplicable:
    default:
      return FindFriendTargetFreshness::NotApplicable;
  }
}

FindFriendConfidence to_ui_confidence(nav::Confidence confidence) {
  switch (confidence) {
    case nav::Confidence::Low:
      return FindFriendConfidence::Low;
    case nav::Confidence::Medium:
      return FindFriendConfidence::Medium;
    case nav::Confidence::High:
      return FindFriendConfidence::High;
    case nav::Confidence::Unavailable:
    default:
      return FindFriendConfidence::Unavailable;
  }
}

}  // namespace

void MeshAppUi::persistFfPrefs(int mode, int wp_valid, double wp_lat, double wp_lon) const {
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  DataStore* ds = the_mesh.getDataStore();
  if (!ds) return;
  ds->saveFindFriendCompassSettings(mode, wp_valid, wp_lat, wp_lon, HELTEC_GPS_TRACK_MIN_DIST_CM,
                                    findFriendTargetContactIndex(), _ff_gps_track_interval_min,
                                    _ff_enabled,
                                    _ff_target_pub_key_valid ? _ff_target_pub_key : nullptr);
#else
  (void)mode;
  (void)wp_valid;
  (void)wp_lat;
  (void)wp_lon;
#endif
}

void MeshAppUi::resetFindFriendNavState() const { _ff_nav_estimator.reset(); }

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
  uint8_t friend_pub_key[PUB_KEY_SIZE] = {};
  bool friend_pub_key_valid = false;
  bool enabled = false;
  const bool loaded =
      load_ff_settings(mode, wp_valid, la, lo, track_cm, &friend_idx, &track_interval_min,
                       &enabled, friend_pub_key, &friend_pub_key_valid);
  if (!loaded) return;

  _ff_mode = mode ? 1 : 0;
  _ff_wp_valid = wp_valid && nav::FindFriendNavEstimator::coordinateValid(la, lo);
  _ff_wp_lat_e6 = deg_to_e6(la);
  _ff_wp_lon_e6 = deg_to_e6(lo);
  _ff_gps_track_interval_min = track_interval_min;
  _ff_enabled = enabled;

  if (friend_pub_key_valid) {
    memcpy(_ff_target_pub_key, friend_pub_key, sizeof(_ff_target_pub_key));
    _ff_target_pub_key_valid = true;
    resolveFindFriendTargetContactIndex();
  } else if (setFindFriendTargetKeyFromIndex(friend_idx)) {
    // Rewrite v2/legacy settings once so future contact reordering cannot
    // silently redirect the selected target.
    persistFfPrefs(_ff_mode, _ff_wp_valid, e6_to_deg(_ff_wp_lat_e6),
                   e6_to_deg(_ff_wp_lon_e6));
  }
}

bool MeshAppUi::locationShareEnabled() const {
  const NodePrefs* p = the_mesh.getNodePrefs();
  return HeltecMesh::isLocationShareEnabled(p);
}

void MeshAppUi::setLocationShareEnabled(bool enabled) {
  HeltecMesh::setLocationShareEnabled(the_mesh, enabled, true);
  reconcileGpsPower();
  notifyAppState(heltec::meshcore::ui::AppStateEventType::ConfigChanged);
  notifyAppState(heltec::meshcore::ui::AppStateEventType::FindFriendChanged);
}

int MeshAppUi::locShareIntervalIndex() const {
  return adv_interval_index(HeltecMesh::locShareAdvertIntervalSec());
}

void MeshAppUi::setLocShareIntervalIndex(int index) {
  if (index < 0) index = 0;
  if (index > 4) index = 4;
  const uint32_t sec = kAdvIntervalSec[index];
  HeltecMesh::setLocShareAdvertIntervalSec(sec);

  NodePrefs* p = the_mesh.getNodePrefs();
  if (p && p->loc_share_adv_sec != sec) {
    p->loc_share_adv_sec = sec;
    the_mesh.savePrefs();
  }
  reconcileGpsPower();
  notifyAppState(heltec::meshcore::ui::AppStateEventType::ConfigChanged);
  notifyAppState(heltec::meshcore::ui::AppStateEventType::FindFriendChanged);
}

int MeshAppUi::locShareIntervalOptionCount() const { return 5; }

const char* MeshAppUi::locShareIntervalOptionLabel(int index) const {
  if (index < 0 || index > 4) return "?";
  return kAdvIntervalLabels[index];
}

bool MeshAppUi::findFriendEnabled() const {
  ensureFindFriendPrefsLoaded();
  return _ff_enabled;
}

bool MeshAppUi::setFindFriendEnabled(bool enabled) {
  ensureFindFriendPrefsLoaded();
  if (enabled && !gpsStatus().enabled) {
    setGpsEnabled(true);
    if (!gpsStatus().enabled) return false;
  }

  if (_ff_enabled == enabled) {
    reconcileGpsPower();
    return true;
  }

  _ff_enabled = enabled;
  resetFindFriendNavState();
  persistFfPrefs(_ff_mode, _ff_wp_valid, e6_to_deg(_ff_wp_lat_e6),
                 e6_to_deg(_ff_wp_lon_e6));
  reconcileGpsPower();
  notifyAppState(heltec::meshcore::ui::AppStateEventType::ConfigChanged);
  notifyAppState(heltec::meshcore::ui::AppStateEventType::FindFriendChanged);
  return true;
}

int MeshAppUi::findFriendMode() const {
  ensureFindFriendPrefsLoaded();
  return _ff_mode;
}

bool MeshAppUi::setFindFriendMode(int mode) {
  ensureFindFriendPrefsLoaded();
  _ff_mode = mode ? 1 : 0;
  resetFindFriendNavState();
  persistFfPrefs(_ff_mode, _ff_wp_valid, e6_to_deg(_ff_wp_lat_e6),
                 e6_to_deg(_ff_wp_lon_e6));
  notifyAppState(heltec::meshcore::ui::AppStateEventType::ConfigChanged);
  notifyAppState(heltec::meshcore::ui::AppStateEventType::FindFriendChanged);
  return true;
}

bool MeshAppUi::setFindFriendWaypoint(double lat_deg, double lon_deg) {
  if (!nav::FindFriendNavEstimator::coordinateValid(lat_deg, lon_deg)) return false;
  ensureFindFriendPrefsLoaded();

  setFfWaypointCache(lat_deg, lon_deg);
  persistFfPrefs(_ff_mode, _ff_wp_valid, e6_to_deg(_ff_wp_lat_e6),
                 e6_to_deg(_ff_wp_lon_e6));
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

int MeshAppUi::fillFindFriendContacts(int offset, int selected_contact_index,
                                      FindFriendContactItem* items, int max_items,
                                      int* total_items, int* selected_rank) const {
  if (total_items) *total_items = 0;
  if (selected_rank) *selected_rank = -1;
  if (!items || max_items <= 0) return 0;
  const int n = findFriendContactCount();
  int order[MAX_CONTACTS];
  bool receipt_known[MAX_CONTACTS];
  uint32_t receipt_ages[MAX_CONTACTS];
  uint32_t mods[MAX_CONTACTS];
  int count = 0;
  if (n > MAX_CONTACTS) return 0;

  for (int i = 0; i < n; ++i) {
    ContactInfo c{};
    if (!the_mesh.getContactByIdx((uint32_t)i, c)) continue;
    int32_t lat_micro = 0;
    int32_t lon_micro = 0;
    HeltecMesh::ContactLocationReceipt receipt{};
    const bool has_location = contact_location(c, lat_micro, lon_micro, &receipt);
    if (!has_location && i != selected_contact_index) continue;
    order[count] = i;
    receipt_known[count] = receipt.known;
    receipt_ages[count] = receipt.age_ms;
    mods[count] = c.lastmod;
    ++count;
  }

  for (int i = 0; i < count - 1; ++i) {
    for (int j = i + 1; j < count; ++j) {
      const bool newer = receipt_known[j] != receipt_known[i]
                             ? receipt_known[j]
                             : (receipt_known[i] ? receipt_ages[j] < receipt_ages[i]
                                                 : mods[j] > mods[i]);
      if (newer) {
        const int ti = order[i];
        order[i] = order[j];
        order[j] = ti;
        const bool trk = receipt_known[i];
        receipt_known[i] = receipt_known[j];
        receipt_known[j] = trk;
        const uint32_t tra = receipt_ages[i];
        receipt_ages[i] = receipt_ages[j];
        receipt_ages[j] = tra;
        const uint32_t tm = mods[i];
        mods[i] = mods[j];
        mods[j] = tm;
      }
    }
  }

  if (total_items) *total_items = count;
  for (int k = 0; k < count; ++k) {
    if (order[k] == selected_contact_index && selected_rank) *selected_rank = k;
  }

  if (offset < 0) offset = 0;
  if (offset > count) offset = count;
  int filled = 0;
  for (int k = offset; k < count && filled < max_items; ++k) {
    FindFriendContactItem& item = items[filled];
    item = FindFriendContactItem{};
    item.contact_index = static_cast<int16_t>(order[k]);
    if (!findFriendContactLabel(order[k], item.label, sizeof(item.label))) continue;
    ++filled;
  }
  return filled;
}

bool MeshAppUi::findFriendContactHasGps(int index) const {
  if (index < 0) return false;
  ContactInfo c{};
  if (!the_mesh.getContactByIdx((uint32_t)index, c)) return false;
  int32_t lat_micro = 0;
  int32_t lon_micro = 0;
  return contact_location(c, lat_micro, lon_micro);
}

int MeshAppUi::resolveFindFriendTargetContactIndex() const {
  _ff_target_contact_idx = -1;
  if (!_ff_target_pub_key_valid) return -1;
  const int n = findFriendContactCount();
  for (int i = 0; i < n; ++i) {
    ContactInfo c{};
    if (the_mesh.getContactByIdx((uint32_t)i, c) &&
        memcmp(c.id.pub_key, _ff_target_pub_key, PUB_KEY_SIZE) == 0) {
      _ff_target_contact_idx = i;
      break;
    }
  }
  return _ff_target_contact_idx;
}

bool MeshAppUi::setFindFriendTargetKeyFromIndex(int index) const {
  ContactInfo c{};
  if (index < 0 || !the_mesh.getContactByIdx((uint32_t)index, c)) return false;
  static_assert(sizeof(_ff_target_pub_key) == PUB_KEY_SIZE, "Find Friend key size mismatch");
  memcpy(_ff_target_pub_key, c.id.pub_key, PUB_KEY_SIZE);
  _ff_target_pub_key_valid = true;
  _ff_target_contact_idx = index;
  return true;
}

int MeshAppUi::findFriendTargetContactIndex() const {
  ensureFindFriendPrefsLoaded();
  return resolveFindFriendTargetContactIndex();
}

void MeshAppUi::setFindFriendTargetContactIndex(int index) {
  ensureFindFriendPrefsLoaded();
  const int n = findFriendContactCount();
  if (index < 0 || index >= n || !findFriendContactHasGps(index)) {
    _ff_target_contact_idx = -1;
    _ff_target_pub_key_valid = false;
    memset(_ff_target_pub_key, 0, sizeof(_ff_target_pub_key));
  } else {
    (void)setFindFriendTargetKeyFromIndex(index);
  }

  persistFfPrefs(_ff_mode, _ff_wp_valid, e6_to_deg(_ff_wp_lat_e6),
                 e6_to_deg(_ff_wp_lon_e6));
  resetFindFriendNavState();
  notifyAppState(heltec::meshcore::ui::AppStateEventType::FindFriendChanged);
}

void MeshAppUi::syncFindFriendContactList() {
  ensureFindFriendPrefsLoaded();
  const int previous = _ff_target_contact_idx;
  const int resolved = resolveFindFriendTargetContactIndex();
  if (resolved != previous) {
    notifyAppState(heltec::meshcore::ui::AppStateEventType::FindFriendChanged);
  }
}

bool MeshAppUi::tryAutoPickFindFriendTarget() {
  ensureFindFriendPrefsLoaded();
  const int n = findFriendContactCount();
  if (n <= 0) return false;
  int idx = resolveFindFriendTargetContactIndex();
  if (idx < 0) idx = 0;
  for (int step = 0; step < n; ++step) {
    if (findFriendContactHasGps(idx)) {
      (void)setFindFriendTargetKeyFromIndex(idx);
      persistFfPrefs(_ff_mode, _ff_wp_valid, e6_to_deg(_ff_wp_lat_e6),
                     e6_to_deg(_ff_wp_lon_e6));
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
  int idx = resolveFindFriendTargetContactIndex();
  if (idx < 0) idx = delta >= 0 ? n - 1 : 0;
  for (int step = 0; step < n; ++step) {
    idx = (idx + delta + n) % n;
    if (findFriendContactHasGps(idx)) {
      (void)setFindFriendTargetKeyFromIndex(idx);
      persistFfPrefs(_ff_mode, _ff_wp_valid, e6_to_deg(_ff_wp_lat_e6),
                     e6_to_deg(_ff_wp_lon_e6));
      resetFindFriendNavState();
      return;
    }
  }
  _ff_target_contact_idx = -1;
}

FindFriendUi MeshAppUi::findFriendUi() const {
  ensureFindFriendPrefsLoaded();
  FindFriendUi u{};
  if (!_ff_enabled) {
    resetFindFriendNavState();
    return u;
  }
  const GpsStatus gps = gpsStatus();
  u.mode = _ff_mode;
  u.gps_age_ms = gps.fix_valid_ms;
  u.gps_satellites = gps.satellites;
  u.waypoint_valid = (_ff_wp_valid != 0) &&
      nav::FindFriendNavEstimator::coordinateValid(microdeg_to_deg(_ff_wp_lat_e6),
                                                    microdeg_to_deg(_ff_wp_lon_e6));
  u.waypoint_lat = microdeg_to_deg(_ff_wp_lat_e6);
  u.waypoint_lon = microdeg_to_deg(_ff_wp_lon_e6);

  double tgt_lat = 0;
  double tgt_lon = 0;
  bool have_tgt = false;

  if (_ff_mode == 0) {
    const int target_index = resolveFindFriendTargetContactIndex();
    u.target_selected = target_index >= 0;
    if (target_index >= 0) {
      ContactInfo c{};
      if (the_mesh.getContactByIdx((uint32_t)target_index, c)) {
        lv_snprintf(u.target_label, sizeof(u.target_label), "%s", c.name[0] ? c.name : "?");
        int32_t lat_micro = 0;
        int32_t lon_micro = 0;
        HeltecMesh::ContactLocationReceipt receipt{};
        if (contact_location(c, lat_micro, lon_micro, &receipt)) {
          tgt_lat = microdeg_to_deg(lat_micro);
          tgt_lon = microdeg_to_deg(lon_micro);
          have_tgt = true;
        }
        u.target_age_known = receipt.known;
        u.target_age_ms = receipt.age_ms;
      } else {
        lv_snprintf(u.target_label, sizeof(u.target_label), "--");
      }
    } else {
      lv_snprintf(u.target_label, sizeof(u.target_label), "--");
    }
  } else {
    u.target_selected = u.waypoint_valid;
    have_tgt = u.waypoint_valid;
    if (have_tgt) {
      tgt_lat = microdeg_to_deg(_ff_wp_lat_e6);
      tgt_lon = microdeg_to_deg(_ff_wp_lon_e6);
    }
    lv_snprintf(u.target_label, sizeof(u.target_label), "WP");
  }

  u.target_valid = have_tgt;

  const CompassUi& cm = _compass_ui;
  u.compass_hw = cm.has_hardware;
  u.compass_quality = cm.quality;
  u.declination_configured = kCompassDeclinationConfigured;

  nav::FindFriendNavInput input{};
  input.now_ms = millis();
  input.gps_valid = gps.fix_valid;
  input.gps_lat_deg = gps.lat_deg;
  input.gps_lon_deg = gps.lon_deg;
  input.gps_age_ms = gps.fix_valid_ms;
  input.gps_satellites = gps.satellites;
  input.target_valid = have_tgt;
  input.target_lat_deg = tgt_lat;
  input.target_lon_deg = tgt_lon;
  input.target_is_contact = _ff_mode == 0;
  input.target_age_known = u.target_age_known;
  input.target_age_ms = u.target_age_ms;
  input.heading_valid = cm.heading_valid;
  // ICMCompassProvider has already converted magnetic north using
  // COMPASS_DECLINATION_DEG before this heading reaches the facade.
  input.heading_deg = nav::FindFriendNavEstimator::wrapHeading360(
      cm.heading_deg + static_cast<float>(COMPASS_HEADING_OFFSET_DEG));
  input.compass_quality = static_cast<uint8_t>(cm.quality < 0 ? 0 : cm.quality);
  input.declination_configured = kCompassDeclinationConfigured;

  const nav::FindFriendNavOutput estimated = _ff_nav_estimator.update(input);
  u.gps_fix = estimated.gps_valid;
  u.here_lat = estimated.filtered_lat_deg;
  u.here_lon = estimated.filtered_lon_deg;
  u.estimated_accuracy_m = estimated.estimated_accuracy_m;
  u.gps_low_accuracy = estimated.gps_low_accuracy;
  u.target_usable = estimated.target_usable;
  u.target_freshness = to_ui_freshness(estimated.target_freshness);
  u.heading_valid = estimated.heading_valid;
  u.heading_deg = estimated.filtered_heading_deg;
  u.bearing_valid = estimated.bearing_valid;
  u.bearing_to_waypoint_deg = estimated.bearing_deg;
  u.relative_valid = estimated.relative_valid;
  u.turn_deg = estimated.turn_deg;
  u.distance_m = estimated.distance_m;
  u.near_target = estimated.near_target;
  u.nearby_enter_m = estimated.nearby_enter_m;
  u.confidence = to_ui_confidence(estimated.confidence);

  return u;
}

}  // namespace heltec::meshcore::biz
