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
#if defined(HELTEC_T1)
// T1: 80px panel height - 12px top pane - 2px tile padding on each side.
constexpr lv_coord_t kT1FindFriendContentHeight = 64;
constexpr lv_coord_t kT1FindFriendDialPadding = 4;
constexpr lv_coord_t kT1FindFriendDialColumnWidth =
    kT1FindFriendContentHeight;
#endif

void set_static_text(lv_obj_t* label, char* buffer, size_t capacity, const char* text) {
  if (!label || !buffer || capacity == 0) return;
  lv_snprintf(buffer, capacity, "%s", text ? text : "");
  lv_label_set_text_static(label, buffer);
}
}  // namespace

_lv_obj_t* FindFriendScreen::createRoot(_lv_obj_t* parent) {
  return ht_obj_create(parent, meta_id::FindFriendScreenRoot);
}

void FindFriendScreen::setInfoText(const char* info) {
  set_static_text(_lbl_info, _info_text, sizeof(_info_text), info);
  if (!_lbl_info) return;
  if (_info_text[0]) {
    lv_obj_clear_flag(_lbl_info, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(_lbl_info, LV_OBJ_FLAG_HIDDEN);
  }
}

void FindFriendScreen::showInfoOnly(const char* info) {
  _dial.setDialHidden(true);
  _dial.setLayersVisible(false);
  _dial.invalidateNeedle();
  setInfoText(info);
}

void FindFriendScreen::onEnter() {
  MESH_DEBUG_PRINTLN("[FF] onEnter t=%lu", (unsigned long)millis());
  AbstractScreen::onEnter();
  if (_right_column) lv_obj_scroll_to_y(_right_column, 0, LV_ANIM_OFF);
  _biz.setFindFriendForegroundActive(true);
  closeOpenDropdown();
  syncControls(true);
  _defer_cycle_target = _biz.findFriendEnabled();

  if (_dial_row) lv_obj_update_layout(_dial_row);
  _dial.layoutSize();

  _dial.setDialHidden(true);
  _dial.setLayersVisible(false);
  setInfoText(_biz.findFriendEnabled() ? "starting..." : "");
}

_lv_obj_t* FindFriendScreen::create(_lv_obj_t* parent) {
  if (!AbstractScreen::create(parent)) return nullptr;

  if (!createDial()) return nullptr;
  addSwitchRow("enable", &_sw_enabled);
  // Settings controlled by the switch stay immediately below it. One runtime
  // row shows either an actionable state or the current distance.
  createControls();
  createInfoRows();
  configureFocusItems();
  syncControls(true);

  return _root;
}

bool FindFriendScreen::createDial() {
  _dial_row = ht_obj_create(_root, meta_id::FindFriendDialRow);
  if (!_dial_row) return false;
  // Keep the dial fixed in the left column. Only the settings/info column on
  // the right scrolls when its rows no longer fit on screen.
  lv_obj_clear_flag(_root, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scrollbar_mode(_root, LV_SCROLLBAR_MODE_OFF);
  lv_obj_set_size(_dial_row, lv_pct(100), lv_pct(100));
  lv_obj_set_flex_grow(_dial_row, 1);
#if defined(HELTEC_T1)
  // The T1 viewport is only 64px high. The generic color-display minimum
  // LV_DPX(96) resolves to about 78px and would overflow this screen.
  lv_obj_set_style_min_height(_dial_row, 0, LV_PART_MAIN);
  compass_init_dial_row(_dial_row, 0);
#else
  lv_obj_set_style_min_height(
      _dial_row, LV_COLOR_DEPTH == 1 ? LV_DPX(32) : LV_DPX(96), LV_PART_MAIN);
  compass_init_dial_row(_dial_row, (lv_coord_t)FF_INFO_COL_MARGIN_RIGHT);
#endif

  _dial.min_side = 16;
#if defined(HELTEC_T1)
  // 64px column minus 4px padding on every side gives a 56px dial.
  _dial.edge_margin = kT1FindFriendDialPadding;
#endif
  _dial.needle_kind = CompassDialNeedleKind::FriendTurn;
  if (!_dial.create(_dial_row)) return false;
#if defined(HELTEC_T1)
  // Give the graphic only the square it needs; the settings column receives
  // every remaining horizontal pixel.
  lv_obj_set_width(_dial.center_col, kT1FindFriendDialColumnWidth);
  lv_obj_set_flex_grow(_dial.center_col, 0);
#endif

  _right_column = compass_create_info_column(
      _dial_row, (lv_coord_t)FF_INFO_COL_WIDTH_PCT,
      (lv_coord_t)FF_INFO_COL_PAD_LEFT,
#if defined(HELTEC_T1)
      0
#else
      1
#endif
  );
  if (!_right_column) return false;
#if defined(HELTEC_T1)
  lv_obj_set_width(_right_column, 0);
  lv_obj_set_flex_grow(_right_column, 1);
#endif
  lv_obj_clear_flag(_right_column, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
  lv_obj_add_flag(_right_column, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(_right_column, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(_right_column, LV_SCROLLBAR_MODE_AUTO);
  return true;
}

void FindFriendScreen::createInfoRows() {
  if (!_right_column) return;

  _lbl_info = ht_label_create(_right_column, meta_id::CompassInfoLabel);
  compass_style_info_label(_lbl_info, "starting...", LV_LABEL_LONG_CLIP);
  if (_lbl_info) {
    lv_label_set_text_static(_lbl_info, _info_text);
  }
}

void FindFriendScreen::render(const biz::FindFriendUi& u) {
  if (!_biz.findFriendEnabled()) {
    showInfoOnly("");
    return;
  }

  if (!u.compass_hw) {
    showInfoOnly("no compass");
    return;
  }
  if (!u.heading_valid) {
    if (!u.target_valid) {
      showInfoOnly("");
    } else if (u.arrived) {
      showInfoOnly("arrived");
    } else if (!u.gps_fix) {
      showInfoOnly("need gps");
    } else {
      showInfoOnly("starting...");
    }
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

  if (!u.target_valid) {
    setInfoText("");
  } else if (u.arrived) {
    setInfoText("arrived");
  } else if (!u.gps_fix) {
    setInfoText("need gps");
  } else if (u.bearing_valid) {
    compass_format_distance_m(_info_text, sizeof(_info_text), u.distance_m);
    if (_lbl_info) {
      lv_obj_clear_flag(_lbl_info, LV_OBJ_FLAG_HIDDEN);
      lv_label_set_text_static(_lbl_info, _info_text);
    }
  } else {
    setInfoText("starting...");
  }
}

void FindFriendScreen::runDeferredEnterActions() {
  if (_defer_cycle_target) {
    _defer_cycle_target = false;
    if (_biz.findFriendEnabled() && _biz.findFriendMode() == 0 &&
        _biz.findFriendTargetContactIndex() < 0 &&
        _biz.findFriendContactCount() > 0) {
      MESH_DEBUG_PRINTLN("[FF] tryAutoPick contacts=%d", _biz.findFriendContactCount());
      (void)_biz.tryAutoPickFindFriendTarget();
    }
  }
}

void FindFriendScreen::onExit() {
  _defer_cycle_target = false;
  closeOpenDropdown();
  _waypoint_keyboard_return_focus = nullptr;
  _biz.setFindFriendForegroundActive(false);
  AbstractScreen::onExit();
}

void FindFriendScreen::refresh() {
  syncControls();
  if (_dial.side == 0) {
    if (_dial_row) lv_obj_update_layout(_dial_row);
    _dial.layoutSize();
  }

  if (!_biz.findFriendEnabled()) {
    render(_biz.findFriendUi());
    return;
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

void FindFriendScreen::onUiEvent(const UiEvent& event) {
  if (event.type == UiEventType::WaypointKeyboardClosed) {
    restoreWaypointKeyboardFocus();
    return;
  }
  if (event.type != UiEventType::WaypointKeyboardSubmit || !event.payload) return;

  const auto* submit = static_cast<const UiWaypointKeyboardSubmit*>(event.payload);
  if (_biz.setFindFriendWaypoint(submit->lat, submit->lon)) {
    _biz.showAlert("Waypoint saved", 2000);
    syncControls();
  } else {
    _biz.showAlert("Save failed", 2000);
  }
}

}  // namespace heltec::meshcore::ui

#endif  // ENV_INCLUDE_COMPASS
