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
  for (_lv_obj_t* label : labels) {
    if (!label) continue;
    lv_obj_set_width(label, lv_pct(100));
    lv_obj_clear_flag(label, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
  }

  return _root;
}

void RadioScreen::updateRadio(const biz::IBizFacade::RadioStatus& s) {
  char tmp[48];
  if (_line1) {
    lv_snprintf(tmp, sizeof(tmp), "FQ: %06.3f  SF:%d", s.freq_mhz, s.sf);
    lv_label_set_text(_line1, tmp);
  }
  if (_line2) {
    lv_snprintf(tmp, sizeof(tmp), "BW:%03.2f   CR:%d", s.bw_khz, s.cr);
    lv_label_set_text(_line2, tmp);
  }
  if (_line3) {
    lv_snprintf(tmp, sizeof(tmp), "TX: %ddBm", s.tx_power_dbm);
    lv_label_set_text(_line3, tmp);
  }
  if (_line4) {
    lv_snprintf(tmp, sizeof(tmp), "Noise floor: %d", s.noise_floor_dbm);
    lv_label_set_text(_line4, tmp);
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
