#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS

#include "calibration_overlay.hpp"

#include "ui/core/ht_meta_data.hpp"
#include "ui/core/ui_events.h"
#include <Arduino.h>
#include <lvgl.h>
#include <stdio.h>

namespace heltec::meshcore::ui {

_lv_obj_t* CalibrationOverlay::createRoot(_lv_obj_t* parent) {
  return ht_obj_create(parent, meta_id::CalibrationOverlayRoot);
}

namespace {

#ifndef COMPASS_CALIB_TIMEOUT_MS
constexpr uint32_t kTimeoutMs = 15000;
#else
constexpr uint32_t kTimeoutMs = COMPASS_CALIB_TIMEOUT_MS;
#endif

#ifndef COMPASS_CALIB_QUALITY_TARGET
constexpr int kQTarget = 3;
#else
constexpr int kQTarget = COMPASS_CALIB_QUALITY_TARGET;
#endif

#ifndef COMPASS_CALIB_QUALITY_STREAK
constexpr uint8_t kStreakNeed = 3;
#else
constexpr uint8_t kStreakNeed = COMPASS_CALIB_QUALITY_STREAK;
#endif

}  // namespace

_lv_obj_t* CalibrationOverlay::create(lv_obj_t* parent) {
  if (!AbstractOverlay::create(parent)) return nullptr;
  lv_obj_set_style_pad_all(_root, 0, LV_PART_MAIN);

  _panel = ht_obj_create(_root, meta_id::CalibrationPanel);
  if (!_panel) return nullptr;
  const lv_coord_t gap =
#if defined(HELTEC_V4_R8_TFT)
      LV_DPX(10);
#else
      3;
#endif
  lv_obj_set_size(_panel, lv_pct(100), lv_pct(100));
  lv_obj_set_flex_flow(_panel, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(_panel, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_all(_panel, 8, LV_PART_MAIN);
  lv_obj_set_style_pad_row(_panel, gap, LV_PART_MAIN);
  lv_obj_clear_flag(_panel, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(_panel, [](lv_event_t* e) {
    auto* self = static_cast<CalibrationOverlay*>(lv_event_get_user_data(e));
    if (!self) return;
    lv_event_stop_bubbling(e);
    lv_event_stop_processing(e);
    self->confirm();
  }, LV_EVENT_CLICKED, this);

  _body = ht_label_create(_panel, meta_id::CalibrationBody);
  if (!_body) return nullptr;
  lv_label_set_text_static(_body, _body_text);
  lv_obj_set_width(_body, lv_pct(100));
  lv_label_set_long_mode(_body, LV_LABEL_LONG_WRAP);

  _footer = ht_label_create(_panel, meta_id::CalibrationFooter);
  if (!_footer) return nullptr;
  lv_label_set_text_static(_footer, _footer_text);
  lv_obj_set_width(_footer, lv_pct(100));
  lv_label_set_long_mode(_footer, LV_LABEL_LONG_WRAP);

  _timeout_timer = lv_timer_create(timeoutTimerCb, 250U, this);
  if (!_timeout_timer) return nullptr;
  lv_timer_set_repeat_count(_timeout_timer, -1);
  lv_timer_pause(_timeout_timer);

  return _root;
}

_lv_obj_t* CalibrationOverlay::focusTarget() const {
  return _panel ? _panel : _root;
}

bool CalibrationOverlay::onKey(uint32_t key) {
  if (key == LV_KEY_ENTER) return true;
  if (key == LV_KEY_ESC) {
    return emitEvent(UiEventType::CalibrationClose);
  }
  return false;
}

void CalibrationOverlay::confirm() {
  if (_phase == Phase::Fail && !_no_sensor) {
    openSession();
    render();
    return;
  }
  if (_phase != Phase::Calibrating) {
    (void)emitEvent(UiEventType::CalibrationClose);
  }
}

void CalibrationOverlay::onEnter() {
  if (!_root || !_panel) return;
  AbstractOverlay::onEnter();
  lv_obj_update_layout(_panel);
  openSession();
  render();
  startTimeoutTimer();
}

void CalibrationOverlay::closeSession() {
  _biz.endCompassCalibration();
}

void CalibrationOverlay::onExit() {
  stopTimeoutTimer();
  closeSession();
  if (!_persisted) {
    (void)_biz.restoreCompassCalibration();
  }
  AbstractOverlay::onExit();
  _phase = Phase::Idle;
}

void CalibrationOverlay::openSession() {
  _no_sensor = !_biz.compassHasHardware();
  _quality_streak = 0;
  _last_q = -1;
  _persisted = false;
  _save_failed = false;
  _deadline_ms = millis() + kTimeoutMs;
  if (_no_sensor) {
    _phase = Phase::Fail;
  } else {
    _phase = Phase::Calibrating;
    _biz.beginCompassCalibration();
  }
}

bool CalibrationOverlay::persistCalibrationIfNeeded() {
  if (_persisted) return true;
  if (!_biz.compassHasHardware()) return false;
  if (!_biz.saveCompassCalibration()) return false;
  _persisted = true;
  return true;
}

void CalibrationOverlay::render() {
  if (!_body || !_footer) return;

  switch (_phase) {
    case Phase::Idle:
      _body_text[0] = '\0';
      _footer_text[0] = '\0';
      break;

    case Phase::Calibrating: {
      const uint32_t now = millis();
      const uint32_t remain_s = (_deadline_ms > now) ? ((_deadline_ms - now) / 1000U) : 0U;
      lv_snprintf(_body_text, sizeof(_body_text),
                  "Compass calibration\nMove in a figure-8");
      if (_last_q >= 0) {
        lv_snprintf(_footer_text, sizeof(_footer_text),
                    "Q:%d  %u/%u  %lus\nESC: cancel", (int)_last_q,
                    (unsigned)_quality_streak, (unsigned)kStreakNeed,
                    (unsigned long)remain_s);
      } else {
        lv_snprintf(_footer_text, sizeof(_footer_text),
                    "Q:-  %u/%u  %lus\nESC: cancel", (unsigned)_quality_streak,
                    (unsigned)kStreakNeed, (unsigned long)remain_s);
      }
      break;
    }

    case Phase::Success:
      lv_snprintf(_body_text, sizeof(_body_text), "Calibration OK");
      lv_snprintf(_footer_text, sizeof(_footer_text), "Enter/ESC: close");
      break;

    case Phase::Fail:
      if (_no_sensor) {
        lv_snprintf(_body_text, sizeof(_body_text),
                    "No compass module\nHardware missing");
        lv_snprintf(_footer_text, sizeof(_footer_text), "Enter/ESC: close");
      } else if (_save_failed) {
        lv_snprintf(_body_text, sizeof(_body_text), "Quality OK\nSave failed");
        lv_snprintf(_footer_text, sizeof(_footer_text),
                    "Enter: retry  ESC: close");
      } else {
        lv_snprintf(_body_text, sizeof(_body_text),
                    "Calibration failed\nQuality still low");
        lv_snprintf(_footer_text, sizeof(_footer_text),
                    "Enter: retry  ESC: close");
      }
      break;
  }
  lv_label_set_text_static(_body, _body_text);
  lv_label_set_text_static(_footer, _footer_text);
}

void CalibrationOverlay::evaluateQuality(int quality) {
  if (_phase != Phase::Calibrating) return;

  if (!_biz.compassHasHardware()) {
    closeSession();
    _no_sensor = true;
    _phase = Phase::Fail;
    render();
    return;
  }

  _last_q = (int8_t)quality;
  if (_last_q == kQTarget) {
    if (++_quality_streak >= kStreakNeed) {
      closeSession();
      if (persistCalibrationIfNeeded()) {
        _phase = Phase::Success;
      } else {
        _save_failed = true;
        _phase = Phase::Fail;
      }
    }
  } else {
    _quality_streak = 0;
  }

  if (_phase == Phase::Calibrating && millis() >= _deadline_ms) {
    closeSession();
    _phase = Phase::Fail;
  }
  render();
}

void CalibrationOverlay::checkTimeout() {
  if (_phase != Phase::Calibrating) return;
  if (millis() < _deadline_ms) {
    render();
    return;
  }
  closeSession();
  _phase = Phase::Fail;
  render();
}

void CalibrationOverlay::startTimeoutTimer() {
  if (!_timeout_timer) return;
  lv_timer_set_period(_timeout_timer, 250U);
  lv_timer_set_repeat_count(_timeout_timer, -1);
  lv_timer_reset(_timeout_timer);
  lv_timer_resume(_timeout_timer);
}

void CalibrationOverlay::stopTimeoutTimer() {
  if (!_timeout_timer) return;
  lv_timer_pause(_timeout_timer);
}

void CalibrationOverlay::timeoutTimerCb(lv_timer_t* timer) {
  auto* self = timer ? static_cast<CalibrationOverlay*>(timer->user_data) : nullptr;
  if (!self) return;
  if (self->_phase != Phase::Calibrating) {
    if (timer) lv_timer_pause(timer);
    return;
  }
  self->checkTimeout();
}

void CalibrationOverlay::onAppStateChanged(const AppStateEvent& event) {
  if (event.type != AppStateEventType::CompassChanged) return;
  evaluateQuality(event.compass.quality);
  checkTimeout();
}

void CalibrationOverlay::onRefreshRequested() { render(); }

}  // namespace heltec::meshcore::ui

#else

#include "calibration_overlay.hpp"

namespace heltec::meshcore::ui {

_lv_obj_t* CalibrationOverlay::createRoot(_lv_obj_t*) { return nullptr; }
_lv_obj_t* CalibrationOverlay::create(_lv_obj_t*) { return nullptr; }
_lv_obj_t* CalibrationOverlay::focusTarget() const { return nullptr; }
bool CalibrationOverlay::onKey(uint32_t) { return false; }
void CalibrationOverlay::onEnter() {}
void CalibrationOverlay::onExit() {}
void CalibrationOverlay::onAppStateChanged(const AppStateEvent&) {}
void CalibrationOverlay::onRefreshRequested() {}
void CalibrationOverlay::startTimeoutTimer() {}
void CalibrationOverlay::stopTimeoutTimer() {}
void CalibrationOverlay::timeoutTimerCb(lv_timer_t*) {}

}  // namespace heltec::meshcore::ui

#endif
