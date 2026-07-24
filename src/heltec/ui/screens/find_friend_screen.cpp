#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS

#include "find_friend_screen.hpp"

#include "MeshCore.h"
#include "compass_ui_common.hpp"
#include "ui/core/ui_events.h"
#include "ui/core/ht_meta_data.hpp"
#include <Arduino.h>
#include <lvgl.h>

#ifndef COMPASS_HEADING_OFFSET_DEG
#define COMPASS_HEADING_OFFSET_DEG 0
#endif

#ifndef FF_INFO_COL_WIDTH_PCT
#define FF_INFO_COL_WIDTH_PCT 40
#endif

#ifndef FF_INFO_COL_MARGIN_RIGHT
#define FF_INFO_COL_MARGIN_RIGHT 6
#endif

#ifndef FF_INFO_COL_PAD_LEFT
#define FF_INFO_COL_PAD_LEFT 2
#endif

namespace heltec::meshcore::ui {

namespace {
void set_static_text(lv_obj_t* label, char* buffer, size_t capacity, const char* text) {
  if (!label || !buffer || capacity == 0) return;
  lv_snprintf(buffer, capacity, "%s", text ? text : "");
  lv_label_set_text_static(label, buffer);
}
}  // namespace

_lv_obj_t* FindFriendScreen::createRoot(_lv_obj_t* parent) {
  return ht_obj_create(parent, meta_id::FindFriendScreenRoot);
}

void FindFriendScreen::showInfoOnly(const char* target, const char* dist, const char* status) {
  _dial.setDialHidden(true);
  _dial.setLayersVisible(false);
  _dial.invalidateNeedle();
  set_static_text(_lbl_target, _target_text, sizeof(_target_text), target);
  set_static_text(_lbl_dist, _dist_text, sizeof(_dist_text), dist);
  set_static_text(_lbl_status, _status_text, sizeof(_status_text), status);
}

void FindFriendScreen::onEnter() {
  MESH_DEBUG_PRINTLN("[FF] onEnter t=%lu", (unsigned long)millis());
  AbstractScreen::onEnter();
  _defer_cycle_target = true;

  if (_root) lv_obj_update_layout(_root);
  _dial.layoutSize();

  _dial.setDialHidden(true);
  _dial.setLayersVisible(false);
  set_static_text(_lbl_target, _target_text, sizeof(_target_text), ">--");
  set_static_text(_lbl_dist, _dist_text, sizeof(_dist_text), "");
  set_static_text(_lbl_status, _status_text, sizeof(_status_text), "Starting...");
}

_lv_obj_t* FindFriendScreen::create(_lv_obj_t* parent) {
  if (!AbstractScreen::create(parent)) return nullptr;

  compass_init_dial_row(_root, (lv_coord_t)FF_INFO_COL_MARGIN_RIGHT);

  _dial.min_side = 16;
  _dial.needle_kind = CompassDialNeedleKind::FriendTurn;
  if (!_dial.create(_root)) return nullptr;

  lv_obj_t* info = compass_create_info_column(_root, (lv_coord_t)FF_INFO_COL_WIDTH_PCT,
                                              (lv_coord_t)FF_INFO_COL_PAD_LEFT, 1);
  if (info) {
    _lbl_target = ht_label_create(info, meta_id::CompassInfoLabel);
    compass_style_info_label(_lbl_target, ">--", LV_LABEL_LONG_WRAP);
    if (_lbl_target) lv_label_set_text_static(_lbl_target, _target_text);
    _lbl_dist = ht_label_create(info, meta_id::CompassInfoLabel);
    compass_style_info_label(_lbl_dist, "Dist:--", LV_LABEL_LONG_WRAP);
    if (_lbl_dist) lv_label_set_text_static(_lbl_dist, _dist_text);
    _lbl_status = ht_label_create(info, meta_id::CompassInfoLabel);
    compass_style_info_label(_lbl_status, "", LV_LABEL_LONG_WRAP);
    if (_lbl_status) lv_label_set_text_static(_lbl_status, _status_text);
  }

  return _root;
}

void FindFriendScreen::render(const biz::FindFriendUi& u) {
  const char* target = u.target_label[0] ? u.target_label : ">--";

  if (!u.compass_hw) {
    showInfoOnly("No compass", u.gps_fix ? "GPS ok" : "No GPS", "");
    return;
  }
  if (!u.heading_valid) {
    showInfoOnly(target, "", "Starting...");
    return;
  }

  _dial.setDialHidden(false);
  _dial.setLayersVisible(true);

  const float hdg = compass_screen_heading(u.heading_deg, COMPASS_HEADING_OFFSET_DEG);
  const int16_t new_ring = dial_heading_tenths((int16_t)(hdg * 10.f + 0.5f));
  const float new_turn = (u.bearing_valid && u.heading_valid) ? u.turn_deg : 0.f;
  const bool new_gps = u.gps_fix;
  const bool new_on_target = u.relative_valid && compass_turn_on_target_deg(u.turn_deg);

  if (new_ring != _ring_heading_tenths) {
    _ring_heading_tenths = new_ring;
    _dial.ring_heading_tenths = new_ring;
    _dial.invalidateRing();
  }
  if (new_turn != _turn_show_deg || new_gps != _gps_fix || new_on_target != _on_target) {
    _turn_show_deg = new_turn;
    _gps_fix = new_gps;
    _on_target = new_on_target;
    _dial.friend_turn_deg = new_turn;
    _dial.friend_gps_fix = new_gps;
    _dial.friend_on_target = new_on_target;
    _dial.invalidateNeedle();
  }

  set_static_text(_lbl_target, _target_text, sizeof(_target_text), target);

  if (_lbl_dist) {
    if (u.bearing_valid && u.gps_fix) {
      compass_format_distance_m(_dist_text, sizeof(_dist_text), u.distance_m);
    } else {
      lv_snprintf(_dist_text, sizeof(_dist_text), "Dist:--");
    }
    lv_label_set_text_static(_lbl_dist, _dist_text);
  }

  if (_lbl_status) {
    compass_format_find_friend_status(_status_text, sizeof(_status_text), u.mode, u.arrived, u.relative_valid, u.turn_deg,
                                      u.target_valid, u.gps_fix, u.heading_valid, u.bearing_valid,
                                      u.bearing_to_waypoint_deg);
    lv_label_set_text_static(_lbl_status, _status_text);
  }
}

void FindFriendScreen::runDeferredEnterActions() {
  if (_defer_cycle_target) {
    _defer_cycle_target = false;
    if (_biz.findFriendMode() == 0 && _biz.findFriendTargetContactIndex() < 0 &&
        _biz.findFriendContactCount() > 0) {
      MESH_DEBUG_PRINTLN("[FF] tryAutoPick contacts=%d", _biz.findFriendContactCount());
      (void)_biz.tryAutoPickFindFriendTarget();
    }
  }
}

void FindFriendScreen::onExit() {
  _defer_cycle_target = false;
  AbstractScreen::onExit();
}

void FindFriendScreen::refresh() {
  if (_dial.side == 0) {
    if (_root) lv_obj_update_layout(_root);
    _dial.layoutSize();
  }

  runDeferredEnterActions();

  _biz.syncCompassCache();
  render(_biz.findFriendUi());
}

void FindFriendScreen::onAppStateChanged(const AppStateEvent& event) {
  if (event.type == AppStateEventType::CompassChanged ||
      event.type == AppStateEventType::GpsChanged ||
      event.type == AppStateEventType::FindFriendChanged ||
      event.type == AppStateEventType::ConfigChanged) {
    refresh();
  }
}

void FindFriendScreen::onRefreshRequested() { refresh(); }

}  // namespace heltec::meshcore::ui

#endif  // ENV_INCLUDE_COMPASS
