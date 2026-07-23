#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS

#include "compass_screen.hpp"

#include "compass_ui_common.hpp"
#include "ui/core/ui_events.h"
#include "ui/core/ht_meta_data.hpp"
#include <Arduino.h>
#include <lvgl.h>
#include <string.h>

#ifndef COMPASS_HEADING_OFFSET_DEG
#define COMPASS_HEADING_OFFSET_DEG 0
#endif

#ifndef COMPASS_INFO_COL_WIDTH_PCT
#define COMPASS_INFO_COL_WIDTH_PCT 32
#endif

#ifndef COMPASS_INFO_COL_MARGIN_RIGHT
#define COMPASS_INFO_COL_MARGIN_RIGHT 0
#endif

#ifndef COMPASS_INFO_COL_PAD_LEFT
#define COMPASS_INFO_COL_PAD_LEFT 0
#endif

namespace heltec::meshcore::ui {

_lv_obj_t* CompassScreen::createRoot(_lv_obj_t* parent) {
  return ht_obj_create(parent, meta_id::CompassScreenRoot);
}

namespace {

constexpr int16_t kHeadingInvalid = -10000;
constexpr int16_t kMagInvalid = 10000;
constexpr uint8_t kFLastHw = 0x02;
constexpr uint8_t kFLastValid = 0x04;

int mag_centi(float gauss) {
  return (int)(gauss * 100.f + (gauss >= 0.f ? 0.5f : -0.5f));
}

int clamp_q(int q) {
  if (q < 0) return 0;
  if (q > 3) return 3;
  return q;
}

void style_q_row_label(lv_obj_t* lb, const char* text) {
  if (!lb) return;
  lv_label_set_long_mode(lb, LV_LABEL_LONG_CLIP);
  lv_label_set_text(lb, text ? text : "");
}

void set_q_color_state(lv_obj_t* lb, int state) {
  if (!lb) return;
  lv_obj_clear_state(lb, LV_STATE_USER_1 | LV_STATE_USER_2);
  if (state == 1) {
    lv_obj_add_state(lb, LV_STATE_USER_1);
  } else if (state == 2) {
    lv_obj_add_state(lb, LV_STATE_USER_2);
  }
}

void set_q_label(lv_obj_t* prefix, lv_obj_t* val, const char* val_text, int quality, bool active) {
  if (!prefix || !val) return;
  lv_label_set_text(prefix, "Q:");
  if (!active) {
    lv_label_set_text(val, "--");
    set_q_color_state(prefix, 0);
    set_q_color_state(val, 0);
    return;
  }
  const int q = clamp_q(quality);
  set_q_color_state(prefix, 0);
  lv_label_set_text(val, val_text ? val_text : "--");
  set_q_color_state(val, q >= 3 ? 1 : 2);
}

}  // namespace

void CompassScreen::clearMagLabels() {
  for (int i = 0; i < 3; ++i) {
    if (!_lbl_mag[i]) continue;
    char line[20];
    lv_snprintf(line, sizeof(line), "%c:--", "XYZ"[i]);
    lv_label_set_text(_lbl_mag[i], line);
    _last_mag_centi[i] = kMagInvalid;
  }
}

void CompassScreen::showUnavailable(const biz::CompassUi& c, const char* hdg_text) {
  _dial.setDialHidden(true);
  _dial.setLayersVisible(false);
  _dial.invalidateNeedle();
  if (_lbl_hdg) lv_label_set_text(_lbl_hdg, hdg_text);
  set_q_label(_lbl_q_prefix, _lbl_q_val, "--", 0, false);
  clearMagLabels();
  _heading_dial_track_tenths = kHeadingInvalid;
  _heading_label_tenths = kHeadingInvalid;
  _last_quality = c.quality;
}

void CompassScreen::updateDialHeading(float heading_deg) {
  _last_heading_deg = heading_deg;
  const float hdg = compass_screen_heading(heading_deg, COMPASS_HEADING_OFFSET_DEG);
  const int16_t hdg_t = (int16_t)compass_heading_tenths(hdg);
  const int16_t dial_t = compass_dial_redraw_tenths(hdg_t);
  _dial.ring_heading_tenths = dial_t;
  _dial.needle_heading_tenths = dial_t;
  if (_dial.side > 0) {
    _dial.invalidateRing();
    _dial.invalidateNeedle();
  }
}

_lv_obj_t* CompassScreen::create(_lv_obj_t* parent) {
  if (!AbstractScreen::create(parent)) return nullptr;

  compass_init_dial_row(_root, (lv_coord_t)COMPASS_INFO_COL_MARGIN_RIGHT);

  _dial.min_side = 20;
  _dial.needle_kind = CompassDialNeedleKind::BicolorHeading;
  if (!_dial.create(_root)) return nullptr;

  lv_obj_t* info = compass_create_info_column(_root, (lv_coord_t)COMPASS_INFO_COL_WIDTH_PCT,
                                              (lv_coord_t)COMPASS_INFO_COL_PAD_LEFT, 0);
  if (info) {
    _lbl_hdg = ht_label_create(info, meta_id::CompassInfoLabel);
    compass_style_info_label(_lbl_hdg, "HDG:--", LV_LABEL_LONG_CLIP);

    _lbl_q_row = ht_obj_create(info, meta_id::CompassQRow);
    if (_lbl_q_row) {
      lv_obj_set_size(_lbl_q_row, lv_pct(100), LV_SIZE_CONTENT);
      lv_obj_set_flex_flow(_lbl_q_row, LV_FLEX_FLOW_ROW);
      lv_obj_set_flex_align(_lbl_q_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                            LV_FLEX_ALIGN_START);
      lv_obj_set_style_pad_all(_lbl_q_row, 0, LV_PART_MAIN);
      lv_obj_clear_flag(_lbl_q_row, LV_OBJ_FLAG_SCROLLABLE);
      lv_obj_add_flag(_lbl_q_row, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    }

    _lbl_q_prefix = ht_label_create(_lbl_q_row, meta_id::CompassQLabel);
    if (_lbl_q_prefix) lv_obj_set_width(_lbl_q_prefix, LV_SIZE_CONTENT);
    style_q_row_label(_lbl_q_prefix, "Q:");
    _lbl_q_val = ht_label_create(_lbl_q_row, meta_id::CompassQLabel);
    if (_lbl_q_val) lv_obj_set_width(_lbl_q_val, LV_SIZE_CONTENT);
    style_q_row_label(_lbl_q_val, "--");

    for (int i = 0; i < 3; ++i) {
      _lbl_mag[i] = ht_label_create(info, meta_id::CompassInfoLabel);
      if (_lbl_mag[i]) {
        lv_obj_set_width(_lbl_mag[i], lv_pct(100));
        char b[20];
        lv_snprintf(b, sizeof(b), "%c:--", "XYZ"[i]);
        compass_style_info_label(_lbl_mag[i], b, LV_LABEL_LONG_CLIP);
      }
    }
    lv_obj_move_foreground(info);
  }

  lv_obj_update_layout(_root);
  _dial.layoutSize();
  updateDialHeading(0.f);
  return _root;
}

void CompassScreen::refresh_ui(const biz::CompassUi& c) {
  const uint8_t prev = _state_flags & (kFLastHw | kFLastValid);
  uint8_t next = 0;
  if (c.has_hardware) next |= kFLastHw;
  if (c.heading_valid) next |= kFLastValid;
  const bool state_changed = (prev != next) || (c.quality != _last_quality);

  if (!c.has_hardware) {
    if (!state_changed) return;
    _state_flags = 0;
    showUnavailable(c, "No compass");
    return;
  }

  if (!c.heading_valid) {
    if (!state_changed) return;
    _state_flags = kFLastHw;
    showUnavailable(c, "Starting...");
    return;
  }

  _state_flags = kFLastHw | kFLastValid;
  _dial.setDialHidden(false);
  _dial.setLayersVisible(true);

  const float hdg = compass_screen_heading(c.heading_deg, COMPASS_HEADING_OFFSET_DEG);
  const int16_t hdg_t = (int16_t)compass_heading_tenths(hdg);
  const int16_t dial_t = compass_dial_redraw_tenths(hdg_t);
  const bool dial_moved =
      (_heading_dial_track_tenths == kHeadingInvalid) || dial_t != _heading_dial_track_tenths;
  const bool label_moved =
      (_heading_label_tenths == kHeadingInvalid) || hdg_t != _heading_label_tenths;

  if (state_changed) {
    _dial.invalidateRing();
    _dial.invalidateNeedle();
  }

  if (dial_moved) {
    _heading_dial_track_tenths = dial_t;
    updateDialHeading(c.heading_deg);
  }

  if (state_changed || label_moved) {
    _heading_label_tenths = hdg_t;
    if (_lbl_hdg) {
      char buf[20];
      lv_snprintf(buf, sizeof(buf), "HDG:%3d.%d", hdg_t / 10, hdg_t % 10);
      lv_label_set_text(_lbl_hdg, buf);
    }
  }

  if (state_changed) {
    char buf[8];
    lv_snprintf(buf, sizeof(buf), "%d", clamp_q(c.quality));
    set_q_label(_lbl_q_prefix, _lbl_q_val, buf, c.quality, true);
    _last_quality = c.quality;
  }

  bool mag_changed = state_changed || label_moved;
  if (!mag_changed) {
    for (int i = 0; i < 3; ++i) {
      if (mag_centi(c.mag_xyz[i]) != _last_mag_centi[i]) {
        mag_changed = true;
        break;
      }
    }
  }
  if (mag_changed) {
    for (int i = 0; i < 3; ++i) {
      if (!_lbl_mag[i]) continue;
      const int cval = mag_centi(c.mag_xyz[i]);
      char line[20];
      const int w = cval / 100;
      const int f = (cval < 0 ? -cval : cval) % 100;
      lv_snprintf(line, sizeof(line), "%c:%3d.%02d", "XYZ"[i], w, f);
      lv_label_set_text(_lbl_mag[i], line);
      _last_mag_centi[i] = (int16_t)cval;
    }
  }
}

void CompassScreen::maybeScheduleAutoCal() {
  if (_skip_auto_cal_once) {
    _skip_auto_cal_once = false;
    return;
  }

  const biz::CompassUi& c = _biz.compassUi();
  if (!c.has_hardware || !c.heading_valid) return;
  if (_biz.compassHasStoredCalibration() && c.quality >= 3) return;
  _pending_auto_cal = true;
  _auto_cal_due_ms = millis() + 200U;
}

void CompassScreen::skipAutoCalibrationOnce() {
  _skip_auto_cal_once = true;
}

void CompassScreen::onEnter() {
  AbstractScreen::onEnter();
  _pending_auto_cal = false;
  _auto_cal_due_ms = 0;

  if (_root) lv_obj_update_layout(_root);
  _dial.layoutSize();
  if (_focus_group && _lbl_hdg) lv_group_focus_obj(_lbl_hdg);
}

void CompassScreen::onExit() {
  _pending_auto_cal = false;
  _auto_cal_due_ms = 0;
  _dial.setLayersVisible(false);
  _dial.setDialHidden(true);
  AbstractScreen::onExit();
}

void CompassScreen::refresh() {
  if (_dial.side == 0) {
    if (_root) lv_obj_update_layout(_root);
    _dial.layoutSize();
  }

  const uint32_t now = millis();
  if (_pending_auto_cal && (int32_t)(now - _auto_cal_due_ms) >= 0) {
    _pending_auto_cal = false;
    _auto_cal_due_ms = 0;
    _biz.requestCompassCalibration();
    return;
  }

  _biz.syncCompassCache();
  refresh_ui(_biz.compassUi());
}

void CompassScreen::onAppStateChanged(const AppStateEvent& event) {
  if (event.type == AppStateEventType::CompassChanged ||
      event.type == AppStateEventType::ConfigChanged) {
    refresh();
  }
}

void CompassScreen::onRefreshRequested() {
  refresh();
  maybeScheduleAutoCal();
}

}  // namespace heltec::meshcore::ui

#endif  // ENV_INCLUDE_COMPASS
