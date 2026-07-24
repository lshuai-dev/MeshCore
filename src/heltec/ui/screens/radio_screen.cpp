#include "radio_screen.hpp"

#include "ui/core/ui_events.h"
#include "ui/core/ht_meta_data.hpp"

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

  return _root;
}

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
}

void RadioScreen::onEnter() {
  AbstractScreen::onEnter();
}

void RadioScreen::onAppStateChanged(const AppStateEvent& event) {
  if (event.type == AppStateEventType::RadioChanged ||
      event.type == AppStateEventType::ConfigChanged) {
    refreshSnapshot();
  }
}

void RadioScreen::onRefreshRequested() { refreshSnapshot(); }

}  // namespace heltec::meshcore::ui
