#include "gps_screen.hpp"

#include "ui/core/ui_events.h"
#include "ui/core/ht_meta_data.hpp"
#include "ui/core/ui_deferred_queue.hpp"
#include "ui/app/ui_theme.hpp"
#include <lvgl.h>

namespace heltec::meshcore::ui {
namespace {

#if defined(HELTEC_V4_R8_TFT)
constexpr lv_coord_t kGpsDropdownHeight =
    ui_settings_row_height() - 2 * ui_settings_row_pad_ver();
constexpr lv_coord_t kGpsDropdownListPadVer = 2;
#endif

void configure_gps_row(_lv_obj_t* row) {
  if (!row) return;
  lv_obj_set_size(row, lv_pct(100), ui_settings_row_height());
  lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
  lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_hor(row, ui_settings_row_pad_hor(), LV_PART_MAIN);
  lv_obj_set_style_pad_ver(row, ui_settings_row_pad_ver(), LV_PART_MAIN);
  lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
}

bool cycle_open_dropdown(_lv_obj_t* dropdown, uint32_t key) {
  if (!dropdown || !lv_dropdown_is_open(dropdown)) return false;
  const uint16_t count = lv_dropdown_get_option_cnt(dropdown);
  if (count <= 1) return false;

  const uint16_t current = lv_dropdown_get_selected(dropdown);
  uint16_t next = current;
  if (key == LV_KEY_DOWN || key == LV_KEY_RIGHT || key == LV_KEY_NEXT) {
    next = static_cast<uint16_t>((current + 1u) % count);
  } else if (key == LV_KEY_UP || key == LV_KEY_LEFT || key == LV_KEY_PREV) {
    next = current == 0 ? static_cast<uint16_t>(count - 1u)
                        : static_cast<uint16_t>(current - 1u);
  } else {
    return false;
  }
  lv_dropdown_set_selected(dropdown, next);
  if (_lv_obj_t* const list = lv_dropdown_get_list(dropdown)) lv_obj_invalidate(list);
  return true;
}

}  // namespace

_lv_obj_t* GPSScreen::createRoot(_lv_obj_t* parent) {
  return ht_obj_create(parent, meta_id::GpsScreenRoot);
}

void format_micro_degrees(char* buffer, size_t capacity, long value) {
  int64_t signed_value = value;
  const bool negative = signed_value < 0;
  if (negative) signed_value = -signed_value;
  lv_snprintf(buffer, capacity, "%s%ld.%06ld", negative ? "-" : "",
              (long)(signed_value / 1000000LL), (long)(signed_value % 1000000LL));
}

void format_altitude(char* buffer, size_t capacity, double altitude_m) {
  int64_t centi = (int64_t)(altitude_m * 100.0 + (altitude_m >= 0.0 ? 0.5 : -0.5));
  const bool negative = centi < 0;
  if (negative) centi = -centi;
  lv_snprintf(buffer, capacity, "alt %s%ld.%02ld m", negative ? "-" : "",
              (long)(centi / 100), (long)(centi % 100));
}

_lv_obj_t* GPSScreen::create(_lv_obj_t* parent) {
  if (!AbstractScreen::create(parent)) return nullptr;

  _row_power = ht_obj_create(_root, meta_id::GpsPowerRow);
  if (_row_power) {
    configure_gps_row(_row_power);
    _lbl_gps_prefix = ht_label_create(_row_power, meta_id::GpsPowerPrefix, "gps");
    _sw_gps = ht_switch_create(_row_power, meta_id::GpsPowerSwitch);
    if (_lbl_gps_prefix) lv_obj_set_width(_lbl_gps_prefix, LV_SIZE_CONTENT);
    if (_sw_gps) {
      lv_obj_clear_flag(_sw_gps, LV_OBJ_FLAG_SCROLLABLE);
      lv_obj_add_event_cb(
          _sw_gps, onGpsSwitchKey,
          static_cast<lv_event_code_t>(LV_EVENT_KEY | LV_EVENT_PREPROCESS), this);
      lv_obj_add_event_cb(_sw_gps, onGpsSwitchValueChanged,
                          LV_EVENT_VALUE_CHANGED, this);
    }
  }

  _row_location_share = ht_obj_create(_root, meta_id::GpsLocationShareRow);
  if (_row_location_share) {
    configure_gps_row(_row_location_share);
    _lbl_location_share = ht_label_create(
        _row_location_share, meta_id::GpsLocationShareLabel, "location share");
    _sw_location_share = ht_switch_create(
        _row_location_share, meta_id::GpsLocationShareSwitch);
    if (_lbl_location_share) lv_obj_set_width(_lbl_location_share, LV_SIZE_CONTENT);
    if (_sw_location_share) {
      lv_obj_clear_flag(_sw_location_share, LV_OBJ_FLAG_SCROLLABLE);
      lv_obj_add_event_cb(
          _sw_location_share, onGpsSwitchKey,
          static_cast<lv_event_code_t>(LV_EVENT_KEY | LV_EVENT_PREPROCESS), this);
      lv_obj_add_event_cb(_sw_location_share, onGpsSwitchValueChanged,
                          LV_EVENT_VALUE_CHANGED, this);
    }
  }

  _row_adv_interval = ht_obj_create(_root, meta_id::GpsAdvIntervalRow);
  if (_row_adv_interval) {
    configure_gps_row(_row_adv_interval);
    _lbl_adv_interval = ht_label_create(
        _row_adv_interval, meta_id::GpsAdvIntervalLabel, "adv interval");
    if (_lbl_adv_interval) lv_obj_set_width(_lbl_adv_interval, LV_SIZE_CONTENT);
    _dd_adv_interval = ht_dropdown_create(
        _row_adv_interval, meta_id::GpsAdvIntervalDropdown);
    if (_dd_adv_interval) {
      char* out = _adv_interval_options;
      size_t remaining = sizeof(_adv_interval_options);
      const int count = _biz.locShareIntervalOptionCount();
      for (int i = 0; i < count && remaining > 1; ++i) {
        const char* const label = _biz.locShareIntervalOptionLabel(i);
        const int written = lv_snprintf(out, remaining, "%s%s",
                                        label ? label : "?",
                                        i + 1 < count ? "\n" : "");
        if (written < 0 || static_cast<size_t>(written) >= remaining) break;
        out += written;
        remaining -= static_cast<size_t>(written);
      }
      lv_dropdown_set_options_static(_dd_adv_interval, _adv_interval_options);
      lv_obj_set_width(_dd_adv_interval, 0);
      lv_obj_set_flex_grow(_dd_adv_interval, 1);
#if LV_COLOR_DEPTH == 1
      lv_obj_set_height(_dd_adv_interval, 12);
#elif defined(HELTEC_V4_R8_TFT)
      lv_obj_set_height(_dd_adv_interval, kGpsDropdownHeight);
      ui_theme_center_dropdown_value(_dd_adv_interval);
#else
      lv_obj_set_height(_dd_adv_interval, LV_SIZE_CONTENT);
#endif
      lv_obj_clear_flag(_dd_adv_interval, LV_OBJ_FLAG_SCROLLABLE);
      lv_obj_set_scrollbar_mode(_dd_adv_interval, LV_SCROLLBAR_MODE_OFF);
      lv_dropdown_set_dir(_dd_adv_interval, LV_DIR_BOTTOM);
      lv_dropdown_set_selected_highlight(_dd_adv_interval, true);
      lv_obj_add_event_cb(
          _dd_adv_interval, onAdvDropdownKey,
          static_cast<lv_event_code_t>(LV_EVENT_KEY | LV_EVENT_PREPROCESS), this);
      lv_obj_add_event_cb(
          _dd_adv_interval, onAdvDropdownReleasedPre,
          static_cast<lv_event_code_t>(LV_EVENT_RELEASED | LV_EVENT_PREPROCESS), this);
      lv_obj_add_event_cb(_dd_adv_interval, onAdvDropdownValueChanged,
                          LV_EVENT_VALUE_CHANGED, this);
      lv_obj_add_event_cb(_dd_adv_interval, onAdvDropdownStateEvent,
                          LV_EVENT_READY, this);
      lv_obj_add_event_cb(_dd_adv_interval, onAdvDropdownStateEvent,
                          LV_EVENT_CANCEL, this);
    }
  }

#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
  _row_track = ht_obj_create(_root, meta_id::GpsTrackRow);
  if (_row_track) {
    configure_gps_row(_row_track);
    _lbl_track = ht_label_create(_row_track, meta_id::GpsTrackLabel, "gps track");
    _sw_track = ht_switch_create(_row_track, meta_id::GpsTrackSwitch);
    if (_lbl_track) lv_obj_set_width(_lbl_track, LV_SIZE_CONTENT);
    if (_sw_track) {
      lv_obj_clear_flag(_sw_track, LV_OBJ_FLAG_SCROLLABLE);
      lv_obj_add_event_cb(
          _sw_track, onGpsSwitchKey,
          static_cast<lv_event_code_t>(LV_EVENT_KEY | LV_EVENT_PREPROCESS), this);
      lv_obj_add_event_cb(_sw_track, onGpsSwitchValueChanged,
                          LV_EVENT_VALUE_CHANGED, this);
    }
  }
#endif

  _lbl_fix = ht_label_create(_root, meta_id::GpsFixLabel);
  lv_label_set_text_static(_lbl_fix, "no fix");
  _lbl_sat = ht_label_create(_root, meta_id::GpsSatLabel);
  lv_label_set_text_static(_lbl_sat, _sat_text);
  _lbl_latlon = ht_label_create(_root, meta_id::GpsLatLonLabel);
  lv_label_set_text_static(_lbl_latlon, _latlon_text);
  _lbl_alt = ht_label_create(_root, meta_id::GpsAltLabel);
  lv_label_set_text_static(_lbl_alt, _alt_text);
  _lbl_speed = ht_label_create(_root, meta_id::GpsSpeedLabel);
  lv_label_set_text_static(_lbl_speed, _speed_text);

  _lv_obj_t* const labels[] = {
      _lbl_gps_prefix, _lbl_location_share, _lbl_adv_interval,
      _lbl_fix, _lbl_sat, _lbl_latlon, _lbl_alt, _lbl_speed,
#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
      _lbl_track,
#endif
  };
  for (_lv_obj_t* label : labels) {
    if (!label) continue;
    lv_obj_clear_flag(label, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
  }

#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
  constexpr bool kFocusOnPointerPress = false;
#else
  constexpr bool kFocusOnPointerPress = true;
#endif
  addFocusItem(_sw_gps, _row_power, kFocusOnPointerPress);
  addFocusItem(_sw_location_share, _row_location_share, kFocusOnPointerPress);
  addFocusItem(_dd_adv_interval, _row_adv_interval, kFocusOnPointerPress);
#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
  addFocusItem(_sw_track, _row_track, kFocusOnPointerPress);
#endif

  _lv_obj_t* const info_rows[] = {
      _lbl_fix, _lbl_sat, _lbl_latlon, _lbl_alt, _lbl_speed};
  for (_lv_obj_t* row : info_rows) {
    if (!row) continue;
    lv_obj_set_width(row, lv_pct(100));
    addFocusItem(row, nullptr, false, FocusVisual::Row);
    // Read-only rows participate in keypad focus scrolling without consuming
    // pointer gestures intended for the scrollable screen root.
    lv_obj_clear_flag(row, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CLICK_FOCUSABLE);
  }

  return _root;
}

void GPSScreen::setSwitchState(_lv_obj_t* sw, bool on) {
  if (!sw) return;
  const bool current = lv_obj_has_state(sw, LV_STATE_CHECKED);
  if (current == on) return;
  _syncing_switches = true;
  if (on) {
    lv_obj_add_state(sw, LV_STATE_CHECKED);
  } else {
    lv_obj_clear_state(sw, LV_STATE_CHECKED);
  }
  _syncing_switches = false;
}

bool GPSScreen::ensureGpsEnabled(_lv_obj_t* dependent_switch) {
  biz::IBizFacade::GpsStatus gps = _biz.gpsStatus();
  if (gps.enabled) return true;

  _biz.setGpsEnabled(true);
  gps = _biz.gpsStatus();
  _gps = gps;
  updateGps(gps);
  if (gps.enabled) return true;

  setSwitchState(dependent_switch, false);
  _biz.showAlert("gps unavailable", 2000);
  return false;
}

void GPSScreen::setAdvIntervalIndex(uint16_t index) {
  if (!_dd_adv_interval) return;
  const uint16_t count = lv_dropdown_get_option_cnt(_dd_adv_interval);
  if (count == 0) return;
  if (index >= count) index = count - 1;
  if (lv_dropdown_get_selected(_dd_adv_interval) == index) return;
  _syncing_adv_interval = true;
  lv_dropdown_set_selected(_dd_adv_interval, index);
  _syncing_adv_interval = false;
}

void GPSScreen::updateConditionalVisibility() {
  if (!_row_adv_interval) return;
  const bool visible = _biz.locationShareEnabled();
  if (visible) {
    lv_obj_clear_flag(_row_adv_interval, LV_OBJ_FLAG_HIDDEN);
  } else {
    closeAdvDropdown();
    lv_obj_add_flag(_row_adv_interval, LV_OBJ_FLAG_HIDDEN);
  }
}

void GPSScreen::closeAdvDropdown() {
  if (!_dd_adv_interval || !_adv_dropdown_open) return;
  _adv_dropdown_open = false;
  setAdvIntervalIndex(_adv_dropdown_original_index);
  if (lv_dropdown_is_open(_dd_adv_interval)) lv_dropdown_close(_dd_adv_interval);
  lv_obj_clear_state(_dd_adv_interval, LV_STATE_EDITED);
  if (group()) lv_group_set_editing(group(), false);
}

void GPSScreen::realignAdvDropdownList() {
  if (!_dd_adv_interval || !_root || !_adv_dropdown_open ||
      !lv_dropdown_is_open(_dd_adv_interval)) {
    return;
  }
  _lv_obj_t* const list = lv_dropdown_get_list(_dd_adv_interval);
  if (!list || !lv_obj_is_valid(list)) return;

  lv_obj_update_layout(_root);
  lv_obj_update_layout(_row_adv_interval);
  lv_obj_update_layout(_dd_adv_interval);
  const lv_coord_t width = lv_obj_get_width(_dd_adv_interval);
  if (width > 0) lv_obj_set_width(list, width);
  lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_OFF);
  ui_theme_apply_dropdown_list(list);
  ui_theme_match_dropdown_list_padding(_dd_adv_interval, list);
#if defined(HELTEC_V4_R8_TFT)
  lv_obj_set_style_pad_top(list, kGpsDropdownListPadVer, LV_PART_MAIN);
  lv_obj_set_style_pad_bottom(list, kGpsDropdownListPadVer, LV_PART_MAIN);
  lv_obj_set_style_pad_top(list, kGpsDropdownListPadVer, LV_PART_SELECTED);
  lv_obj_set_style_pad_bottom(list, kGpsDropdownListPadVer, LV_PART_SELECTED);
#endif
  _lv_obj_t* const viewport = tile();
  ui_dropdown_fit_list_to_viewport(
      _dd_adv_interval, viewport ? viewport : _root, _root);
}

void GPSScreen::realignAdvDropdownListAsync(void* user_data) {
  auto* self = static_cast<GPSScreen*>(user_data);
  if (self) self->realignAdvDropdownList();
}

void GPSScreen::onGpsSwitchKey(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_KEY) return;

  auto* self = static_cast<GPSScreen*>(lv_event_get_user_data(e));
  _lv_obj_t* const sw = lv_event_get_target(e);
  if (!self || !sw) return;
  bool known_switch = sw == self->_sw_gps || sw == self->_sw_location_share;
#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
  known_switch = known_switch || sw == self->_sw_track;
#endif
  if (!known_switch) return;

  const uint32_t key = lv_event_get_key(e);
  if (key == LV_KEY_ESC) {
    // The switch has no local ESC state. Return directly to the screen Root
    // instead of relying on widget/parent bubble flags.
    (void)self->onKey(LV_KEY_ESC);
    lv_event_stop_processing(e);
    lv_event_stop_bubbling(e);
    return;
  }
  if (key != LV_KEY_ENTER) return;

  // Fully consume the KEY phase so ENTER cannot reach the screen-level
  // context action. The keypad's following press/release phase is left intact
  // so the switch performs its normal single toggle.
  lv_event_stop_processing(e);
  lv_event_stop_bubbling(e);

  // The keypad driver will still emit the normal press/release sequence after
  // this KEY event. The switch toggles once on release and then emits
  // LV_EVENT_VALUE_CHANGED; toggling here as well would apply the change twice.
}

void GPSScreen::onGpsSwitchValueChanged(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;
  auto* self = static_cast<GPSScreen*>(lv_event_get_user_data(e));
  if (!self || self->_syncing_switches) return;

  _lv_obj_t* const sw = lv_event_get_target(e);
  if (!sw) return;
  const bool requested_on = lv_obj_has_state(sw, LV_STATE_CHECKED);

  if (sw == self->_sw_gps) {
    self->_biz.setGpsEnabled(requested_on);
    const biz::IBizFacade::GpsStatus actual = self->_biz.gpsStatus();
    self->_gps = actual;
    self->updateGps(actual);
    self->_biz.showAlert(actual.enabled
                             ? "gps: on"
                             : (requested_on ? "gps unavailable" : "gps: off"),
                         requested_on && !actual.enabled ? 2000 : 800);
    return;
  }

  if (sw == self->_sw_location_share) {
    if (requested_on && !self->ensureGpsEnabled(self->_sw_location_share)) return;
    self->_biz.setLocationShareEnabled(requested_on);
    self->setSwitchState(
        self->_sw_location_share, self->_biz.locationShareEnabled());
    self->updateConditionalVisibility();
    self->_biz.showAlert(
        requested_on ? "location share: on" : "location share: off", 800);
    return;
  }

#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
  if (sw == self->_sw_track) {
    if (requested_on && !self->ensureGpsEnabled(self->_sw_track)) return;
    if (!self->_biz.setGpsTrackRecording(requested_on)) {
      self->setSwitchState(self->_sw_track, self->_biz.gpsTrackRecording());
      return;
    }
    self->setSwitchState(self->_sw_track, self->_biz.gpsTrackRecording());
    self->_biz.showAlert(requested_on ? "gps track on" : "gps track off", 800);
  }
#endif
}

void GPSScreen::onAdvDropdownKey(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_KEY) return;
  auto* self = static_cast<GPSScreen*>(lv_event_get_user_data(e));
  _lv_obj_t* const dropdown = lv_event_get_target(e);
  if (!self || dropdown != self->_dd_adv_interval) return;

  const uint32_t key = lv_event_get_key(e);
  if (self->_adv_dropdown_open && cycle_open_dropdown(dropdown, key)) {
    if (!ui_defer(realignAdvDropdownListAsync, self)) {
      self->realignAdvDropdownList();
    }
    lv_event_stop_processing(e);
    lv_event_stop_bubbling(e);
    return;
  }

  if (key == LV_KEY_ENTER && self->_adv_dropdown_open &&
      lv_dropdown_is_open(dropdown)) {
    const bool changed = lv_dropdown_get_selected(dropdown) !=
                         self->_adv_dropdown_original_index;
    self->_adv_dropdown_open = false;
    lv_dropdown_close(dropdown);
    lv_obj_clear_state(dropdown, LV_STATE_EDITED);
    if (self->group()) lv_group_set_editing(self->group(), false);
    if (changed) lv_event_send(dropdown, LV_EVENT_VALUE_CHANGED, nullptr);
    lv_event_stop_processing(e);
    lv_event_stop_bubbling(e);
    return;
  }

  if (key == LV_KEY_ESC) {
    if (self->_adv_dropdown_open) {
      self->closeAdvDropdown();
    } else {
      (void)self->onKey(LV_KEY_ESC);
    }
    lv_event_stop_processing(e);
    lv_event_stop_bubbling(e);
    return;
  }

  if (!self->_adv_dropdown_open) {
    if (key == LV_KEY_DOWN || key == LV_KEY_RIGHT) {
      if (self->group()) lv_group_focus_next(self->group());
      lv_event_stop_processing(e);
      lv_event_stop_bubbling(e);
    } else if (key == LV_KEY_UP || key == LV_KEY_LEFT) {
      if (self->group()) lv_group_focus_prev(self->group());
      lv_event_stop_processing(e);
      lv_event_stop_bubbling(e);
    }
  }
}

void GPSScreen::onAdvDropdownValueChanged(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;
  auto* self = static_cast<GPSScreen*>(lv_event_get_user_data(e));
  if (!self || self->_syncing_adv_interval) return;
  _lv_obj_t* const dropdown = lv_event_get_target(e);
  if (dropdown != self->_dd_adv_interval) return;

  self->_biz.setLocShareIntervalIndex(
      static_cast<int>(lv_dropdown_get_selected(dropdown)));
  self->setAdvIntervalIndex(
      static_cast<uint16_t>(self->_biz.locShareIntervalIndex()));
  self->_biz.showAlert("adv interval saved", 2000);
}

void GPSScreen::onAdvDropdownStateEvent(lv_event_t* e) {
  auto* self = static_cast<GPSScreen*>(lv_event_get_user_data(e));
  _lv_obj_t* const dropdown = lv_event_get_target(e);
  if (!self || dropdown != self->_dd_adv_interval) return;

  const lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_READY) {
    self->_adv_dropdown_open = true;
    self->_adv_dropdown_original_index = lv_dropdown_get_selected(dropdown);
    if (_lv_obj_t* const list = lv_dropdown_get_list(dropdown)) {
      ui_theme_apply_dropdown_list(list);
      ui_theme_match_dropdown_list_padding(dropdown, list);
    }
    lv_obj_add_state(dropdown, LV_STATE_EDITED);
    if (self->group()) lv_group_set_editing(self->group(), true);
    if (!ui_defer(realignAdvDropdownListAsync, self)) {
      self->realignAdvDropdownList();
    }
  } else if (code == LV_EVENT_CANCEL) {
    self->_adv_dropdown_open = false;
    lv_obj_clear_state(dropdown, LV_STATE_EDITED);
    if (self->group()) lv_group_set_editing(self->group(), false);
  }
}

void GPSScreen::onAdvDropdownReleasedPre(lv_event_t* e) {
  auto* self = static_cast<GPSScreen*>(lv_event_get_user_data(e));
  if (self) self->realignAdvDropdownList();
}

void GPSScreen::refreshSnapshot() {
  _gps = _biz.gpsStatus();
  updateGps(_gps);
}

void GPSScreen::onExit() {
  closeAdvDropdown();
  _biz.setGpsForegroundActive(false);
  AbstractScreen::onExit();
}

void GPSScreen::onEnter() {
  AbstractScreen::onEnter();
  _biz.setGpsForegroundActive(true);
}

void GPSScreen::onAppStateChanged(const AppStateEvent& event) {
  if (event.type == AppStateEventType::GpsChanged) {
    refreshSnapshot();
    return;
  }
  if (event.type == AppStateEventType::ConfigChanged) refreshSnapshot();
}

void GPSScreen::onRefreshRequested() { refreshSnapshot(); }

void GPSScreen::updateGps(const biz::IBizFacade::GpsStatus& s) {
  const bool show_fix_data = s.available && s.enabled && s.fix_valid;

  setSwitchState(_sw_gps, s.enabled);
  setSwitchState(_sw_location_share, _biz.locationShareEnabled());
  if (!_adv_dropdown_open) {
    setAdvIntervalIndex(static_cast<uint16_t>(_biz.locShareIntervalIndex()));
  }
  updateConditionalVisibility();
#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
  setSwitchState(_sw_track, _biz.gpsTrackRecording());
#endif

  if (_lbl_fix) {
    if (!s.available) {
      lv_label_set_text_static(_lbl_fix, "can't access gps");
    } else if (!s.enabled || !s.fix_valid) {
      lv_label_set_text_static(_lbl_fix, "no fix");
    } else {
      lv_label_set_text_static(_lbl_fix, "fix");
    }
  }

  if (_lbl_sat) {
    if (!show_fix_data) {
      lv_snprintf(_sat_text, sizeof(_sat_text), "sat --");
    } else {
      lv_snprintf(_sat_text, sizeof(_sat_text), "sat %d", s.satellites);
    }
    lv_label_set_text_static(_lbl_sat, _sat_text);
  }

  if (_lbl_latlon) {
    if (!show_fix_data) {
      lv_snprintf(_latlon_text, sizeof(_latlon_text), "lat -- lon --");
    } else {
      char lat[20];
      char lon[20];
      format_micro_degrees(lat, sizeof(lat), s.lat_micro);
      format_micro_degrees(lon, sizeof(lon), s.lon_micro);
      lv_snprintf(_latlon_text, sizeof(_latlon_text), "lat %s lon %s", lat, lon);
    }
    lv_label_set_text_static(_lbl_latlon, _latlon_text);
  }

  if (_lbl_alt) {
    if (!show_fix_data) {
      lv_snprintf(_alt_text, sizeof(_alt_text), "alt --");
    } else {
      format_altitude(_alt_text, sizeof(_alt_text), s.alt_m);
    }
    lv_label_set_text_static(_lbl_alt, _alt_text);
  }

  if (_lbl_speed) {
    if (!show_fix_data || s.speed_kph < 0.0f) {
      lv_snprintf(_speed_text, sizeof(_speed_text), "speed -- km/h");
    } else {
      int32_t speed_deci_kph = static_cast<int32_t>(s.speed_kph * 10.0f + 0.5f);
      if (speed_deci_kph < 0) speed_deci_kph = 0;
      if (speed_deci_kph > 9999) speed_deci_kph = 9999;
      lv_snprintf(_speed_text, sizeof(_speed_text), "speed %ld.%ld km/h",
                  static_cast<long>(speed_deci_kph / 10),
                  static_cast<long>(speed_deci_kph % 10));
    }
    lv_label_set_text_static(_lbl_speed, _speed_text);
  }
}

}  // namespace heltec::meshcore::ui
