#include "gps_screen.hpp"

#include "ui/core/ui_events.h"
#include "ui/core/ht_meta_data.hpp"
#include <lvgl.h>

namespace heltec::meshcore::ui {

_lv_obj_t* GPSScreen::createRoot(_lv_obj_t* parent) {
  return ht_obj_create(parent, meta_id::GpsScreenRoot);
}

void set_power_state(_lv_obj_t* lb, bool on) {
  if (!lb) return;
  lv_label_set_text(lb, on ? "on" : "off");
  if (on) {
    lv_obj_add_state(lb, LV_STATE_USER_3);
  } else {
    lv_obj_clear_state(lb, LV_STATE_USER_3);
  }
}

_lv_obj_t* GPSScreen::create(_lv_obj_t* parent) {
  if (!AbstractScreen::create(parent)) return nullptr;

  _row_power = ht_obj_create(_root, meta_id::GpsPowerRow);
  if (_row_power) {
    lv_obj_set_size(_row_power, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(_row_power, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(_row_power, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_all(_row_power, 0, LV_PART_MAIN);
    lv_obj_clear_flag(_row_power, LV_OBJ_FLAG_SCROLLABLE);
    _lbl_gps_prefix = ht_label_create(_row_power, meta_id::GpsPowerPrefix, "GPS ");
    _lbl_gps_state = ht_label_create(_row_power, meta_id::GpsPowerState);
    if (_lbl_gps_prefix) lv_obj_set_width(_lbl_gps_prefix, LV_SIZE_CONTENT);
    if (_lbl_gps_state) lv_obj_set_width(_lbl_gps_state, LV_SIZE_CONTENT);
    if (_lbl_gps_state) {
      set_power_state(_lbl_gps_state, false);
    }
  }

  _lbl_fix = ht_label_create(_root, meta_id::GpsFixLabel);
  lv_label_set_text(_lbl_fix, "no fix");
  _lbl_sat = ht_label_create(_root, meta_id::GpsSatLabel);
  lv_label_set_text(_lbl_sat, "sat --");
  _lbl_latlon = ht_label_create(_root, meta_id::GpsLatLonLabel);
  lv_label_set_text(_lbl_latlon, "lat -- lon --");
  _lbl_alt = ht_label_create(_root, meta_id::GpsAltLabel);
  lv_label_set_text(_lbl_alt, "alt --");

  _lv_obj_t* const labels[] = {
      _lbl_gps_prefix, _lbl_gps_state, _lbl_fix, _lbl_sat, _lbl_latlon, _lbl_alt,
  };
  for (_lv_obj_t* label : labels) {
    if (!label) continue;
    lv_obj_clear_flag(label, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
  }

  return _root;
}

void GPSScreen::refreshSnapshot() {
  _gps = _biz.gpsStatus();
  updateGps(_gps);
}

void GPSScreen::onEnter() {
  AbstractScreen::onEnter();
}

void GPSScreen::onExit() {
  AbstractScreen::onExit();
}

void GPSScreen::onAppStateChanged(const AppStateEvent& event) {
  if (event.type == AppStateEventType::GpsChanged) {
    _gps.enabled = event.gps.enabled;
    _gps.available = event.gps.available;
    _gps.fix_valid = event.gps.fix_valid;
    _gps.satellites = event.gps.satellites;
    _gps.lat_micro = event.gps.lat_micro;
    _gps.lon_micro = event.gps.lon_micro;
    _gps.lat_deg = event.gps.lat_deg;
    _gps.lon_deg = event.gps.lon_deg;
    _gps.alt_m = event.gps.alt_m;
    updateGps(_gps);
    return;
  }
  if (event.type == AppStateEventType::ConfigChanged) refreshSnapshot();
}

void GPSScreen::onRefreshRequested() { refreshSnapshot(); }

void GPSScreen::updateGps(const biz::IBizFacade::GpsStatus& s) {
  char tmp[72];
  const bool show_fix_data = s.available && s.enabled && s.fix_valid;

  set_power_state(_lbl_gps_state, s.enabled);

  if (_lbl_fix) {
    if (!s.available) {
      lv_label_set_text(_lbl_fix, "Can't access GPS");
    } else if (!s.enabled || !s.fix_valid) {
      lv_label_set_text(_lbl_fix, "no fix");
    } else {
      lv_label_set_text(_lbl_fix, "fix");
    }
  }

  if (_lbl_sat) {
    if (!show_fix_data) {
      lv_label_set_text(_lbl_sat, "sat --");
    } else {
      lv_snprintf(tmp, sizeof(tmp), "sat %d", s.satellites);
      lv_label_set_text(_lbl_sat, tmp);
    }
  }

  if (_lbl_latlon) {
    if (!show_fix_data) {
      lv_label_set_text(_lbl_latlon, "lat -- lon --");
    } else {
      lv_snprintf(tmp, sizeof(tmp), "lat %.6f lon %.6f", s.lat_deg, s.lon_deg);
      lv_label_set_text(_lbl_latlon, tmp);
    }
  }

  if (_lbl_alt) {
    if (!show_fix_data) {
      lv_label_set_text(_lbl_alt, "alt --");
    } else {
      lv_snprintf(tmp, sizeof(tmp), "alt %.2f m", s.alt_m);
      lv_label_set_text(_lbl_alt, tmp);
    }
  }
}

}  // namespace heltec::meshcore::ui
