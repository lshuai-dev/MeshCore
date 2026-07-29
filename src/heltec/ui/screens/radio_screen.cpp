#include "radio_screen.hpp"

#include "ui/core/ui_events.h"
#include "ui/core/ht_meta_data.hpp"
#include "ui/app/ui_theme.hpp"

#include <lvgl.h>

namespace heltec::meshcore::ui {

_lv_obj_t* RadioScreen::createRoot(_lv_obj_t* parent) {
  return ht_obj_create(parent, meta_id::RadioScreenRoot);
}

_lv_obj_t* RadioScreen::create(_lv_obj_t* parent) {
  if (!AbstractScreen::create(parent)) return nullptr;

  _line1 = ht_label_create(_root, meta_id::RadioLineLabel);
  _line2 = ht_label_create(_root, meta_id::RadioLineLabel);
  _line3 = ht_label_create(_root, meta_id::RadioLineLabel);
  _line4 = ht_label_create(_root, meta_id::RadioLineLabel);

  _lv_obj_t* const labels[] = {_line1, _line2, _line3, _line4};
  for (int i = 0; i < 4; ++i) {
    _lv_obj_t* label = labels[i];
    if (!label) continue;
    lv_obj_set_width(label, lv_pct(100));
    lv_obj_clear_flag(label, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
    lv_label_set_text_static(label, _line_text[i]);
  }

#if defined(HAS_LNA_CONTROL) && HAS_LNA_CONTROL
#if defined(HELTEC_V4_R8_TFT)
  const bool show_lna = true;
#else
  const bool show_lna = _biz.isLnaCanControl();
#endif
  if (show_lna) {
    _row_lna = ht_obj_create(_root, meta_id::RadioLnaRow);
    if (_row_lna) {
      lv_obj_set_size(_row_lna, lv_pct(100), ui_settings_row_height());
      lv_obj_set_flex_flow(_row_lna, LV_FLEX_FLOW_ROW);
      lv_obj_set_flex_align(_row_lna, LV_FLEX_ALIGN_SPACE_BETWEEN,
                            LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
      lv_obj_set_style_pad_all(_row_lna, 0, LV_PART_MAIN);
      lv_obj_clear_flag(_row_lna, LV_OBJ_FLAG_SCROLLABLE);
      _lbl_lna = ht_label_create(_row_lna, meta_id::RadioLnaLabel, "LNA");
      _sw_lna = ht_switch_create(_row_lna, meta_id::RadioLnaSwitch);
      if (_lbl_lna) {
        lv_obj_set_width(_lbl_lna, LV_SIZE_CONTENT);
        lv_obj_clear_flag(_lbl_lna,
                          LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ON_FOCUS);
      }
      if (_sw_lna) {
        lv_obj_clear_flag(_sw_lna, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_event_cb(
            _sw_lna, onLnaSwitchKey,
            static_cast<lv_event_code_t>(LV_EVENT_KEY | LV_EVENT_PREPROCESS), this);
        lv_obj_add_event_cb(_sw_lna, onLnaSwitchValueChanged,
                            LV_EVENT_VALUE_CHANGED, this);
      }
    }
  }
#endif

#if defined(HAS_LNA_CONTROL) && HAS_LNA_CONTROL
  addFocusItem(_sw_lna, _row_lna);
#endif

  return _root;
}

#if defined(HAS_LNA_CONTROL) && HAS_LNA_CONTROL
void RadioScreen::setLnaSwitchState(bool on) {
  if (!_sw_lna) return;
  const bool current = lv_obj_has_state(_sw_lna, LV_STATE_CHECKED);
  if (current == on) return;
  _syncing_lna = true;
  if (on) {
    lv_obj_add_state(_sw_lna, LV_STATE_CHECKED);
  } else {
    lv_obj_clear_state(_sw_lna, LV_STATE_CHECKED);
  }
  _syncing_lna = false;
}

void RadioScreen::onLnaSwitchKey(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_KEY) return;
  auto* self = static_cast<RadioScreen*>(lv_event_get_user_data(e));
  if (!self || lv_event_get_target(e) != self->_sw_lna) return;

  const uint32_t key = lv_event_get_key(e);
  if (key == LV_KEY_ESC) {
    (void)self->onKey(LV_KEY_ESC);
    lv_event_stop_processing(e);
    lv_event_stop_bubbling(e);
    return;
  }
  if (key == LV_KEY_ENTER) {
    lv_event_stop_processing(e);
    lv_event_stop_bubbling(e);
  }
}

void RadioScreen::onLnaSwitchValueChanged(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;
  auto* self = static_cast<RadioScreen*>(lv_event_get_user_data(e));
  if (!self || self->_syncing_lna || lv_event_get_target(e) != self->_sw_lna) return;

  const bool requested_on = lv_obj_has_state(self->_sw_lna, LV_STATE_CHECKED);
  if (!self->_biz.setLnaEnabled(requested_on)) {
    self->setLnaSwitchState(self->_biz.lnaEnabled());
    self->_biz.showAlert("LNA unavailable", 2000);
    return;
  }
  const bool actual_on = self->_biz.lnaEnabled();
  self->setLnaSwitchState(actual_on);
  self->_biz.showAlert(actual_on ? "LNA: ON" : "LNA: OFF", 800);
}
#endif

void RadioScreen::updateRadio(const biz::IBizFacade::RadioStatus& s) {
  const int32_t freq_milli = (int32_t)(s.freq_mhz * 1000.0f + 0.5f);
  const int32_t bw_centi = (int32_t)(s.bw_khz * 100.0f + 0.5f);
  if (_line1) {
    lv_snprintf(_line_text[0], sizeof(_line_text[0]), "FQ: %ld.%03ld  SF:%d",
                (long)(freq_milli / 1000), (long)(freq_milli % 1000), s.sf);
    lv_label_set_text_static(_line1, _line_text[0]);
  }
  if (_line2) {
    lv_snprintf(_line_text[1], sizeof(_line_text[1]), "BW:%ld.%02ld   CR:%d",
                (long)(bw_centi / 100), (long)(bw_centi % 100), s.cr);
    lv_label_set_text_static(_line2, _line_text[1]);
  }
  if (_line3) {
    lv_snprintf(_line_text[2], sizeof(_line_text[2]), "TX: %ddBm", s.tx_power_dbm);
    lv_label_set_text_static(_line3, _line_text[2]);
  }
  if (_line4) {
    lv_snprintf(_line_text[3], sizeof(_line_text[3]), "Noise floor: %d", s.noise_floor_dbm);
    lv_label_set_text_static(_line4, _line_text[3]);
  }
}

void RadioScreen::refreshSnapshot() {
  _last_radio = _biz.radioStatus();
  updateRadio(_last_radio);
#if defined(HAS_LNA_CONTROL) && HAS_LNA_CONTROL
  setLnaSwitchState(_biz.lnaEnabled());
#endif
}

void RadioScreen::onAppStateChanged(const AppStateEvent& event) {
  if (event.type == AppStateEventType::RadioChanged ||
      event.type == AppStateEventType::ConfigChanged) {
    refreshSnapshot();
  }
}

void RadioScreen::onRefreshRequested() { refreshSnapshot(); }

}  // namespace heltec::meshcore::ui
