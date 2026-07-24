#include "system_screen.hpp"

#include "ui/app/ui_theme.hpp"
#include "ui/core/ui_deferred_queue.hpp"
#include <Arduino.h>

namespace heltec::meshcore::ui {
namespace {
#if defined(HELTEC_V4_R8_TFT)
constexpr lv_coord_t kSystemDropdownListPadVer = 2;
#endif
void sync_switch(_lv_obj_t* sw,bool on,bool* syncing){if(!sw)return;const bool cur=lv_obj_has_state(sw,LV_STATE_CHECKED);if(cur==on)return;if(syncing)*syncing=true;if(on)lv_obj_add_state(sw,LV_STATE_CHECKED);else lv_obj_clear_state(sw,LV_STATE_CHECKED);if(syncing)*syncing=false;}
void apply_action_row_theme(_lv_obj_t* row, _lv_obj_t* sw) {
  ui_theme_apply_switch_row_focus(row, sw);
}
_lv_obj_t* focus_row_for(_lv_obj_t* obj){
#if LV_USE_SWITCH != 0
if(obj&&lv_obj_check_type(obj,&lv_switch_class))return lv_obj_get_parent(obj);
#endif
#if LV_USE_DROPDOWN != 0
if(obj&&lv_obj_check_type(obj,&lv_dropdown_class))return lv_obj_get_parent(obj);
#endif
#if LV_USE_SLIDER != 0
if(obj&&lv_obj_check_type(obj,&lv_slider_class)){_lv_obj_t* controls=lv_obj_get_parent(obj);return controls?lv_obj_get_parent(controls):obj;}
#endif
return obj;}
void realign_dropdown_list_async(void* user_data){
#if LV_USE_DROPDOWN != 0
_lv_obj_t* dd=static_cast<_lv_obj_t*>(user_data);if(!dd||!lv_dropdown_is_open(dd))return;_lv_obj_t* list=lv_dropdown_get_list(dd);if(!list)return;lv_obj_update_layout(dd);const lv_coord_t w=lv_obj_get_width(dd);if(w>0)lv_obj_set_width(list,w);lv_obj_set_scrollbar_mode(list,LV_SCROLLBAR_MODE_OFF);ui_theme_apply_dropdown_list(list);ui_theme_match_dropdown_list_padding(dd,list);
#if defined(HELTEC_V4_R8_TFT)
lv_obj_set_style_pad_top(list,kSystemDropdownListPadVer,LV_PART_MAIN);lv_obj_set_style_pad_bottom(list,kSystemDropdownListPadVer,LV_PART_MAIN);lv_obj_set_style_pad_top(list,kSystemDropdownListPadVer,LV_PART_SELECTED);lv_obj_set_style_pad_bottom(list,kSystemDropdownListPadVer,LV_PART_SELECTED);
#endif
lv_obj_update_layout(list);const lv_dir_t dir=lv_dropdown_get_dir(dd);if(dir==LV_DIR_BOTTOM)lv_obj_align_to(list,dd,LV_ALIGN_OUT_BOTTOM_LEFT,0,0);else if(dir==LV_DIR_TOP)lv_obj_align_to(list,dd,LV_ALIGN_OUT_TOP_LEFT,0,0);else if(dir==LV_DIR_LEFT)lv_obj_align_to(list,dd,LV_ALIGN_OUT_LEFT_TOP,0,0);else if(dir==LV_DIR_RIGHT)lv_obj_align_to(list,dd,LV_ALIGN_OUT_RIGHT_TOP,0,0);
#else
(void)user_data;
#endif
}
}  // namespace

void SystemScreen::onSwitchValueChanged(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;
  auto* self = static_cast<SystemScreen*>(lv_event_get_user_data(e));
  if (!self || self->_syncing_switch) return;

  _lv_obj_t* const sw = lv_event_get_target(e);
  const bool on = lv_obj_has_state(sw, LV_STATE_CHECKED);
  biz::IBizFacade& app = self->_biz;

  if (sw == self->_swBle) {
    app.setCompanionLinkEnabled(on);
    self->_feedback.showAlert(on ? "Bluetooth: ON" : "Bluetooth: OFF", 800);
  } else if (sw == self->_swGps) {
    app.setGpsEnabled(on);
    const bool gps_enabled = app.gpsStatus().enabled;
    self->setSwitchState(self->_swGps, gps_enabled);
    if (!gps_enabled) {
      self->setSwitchState(self->_swLocShare, app.locationShareEnabled());
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
      self->setSwitchState(self->_swGpsTrack, app.gpsTrackRecording());
#endif
      self->updateConditionalVisibility(app);
    }
    self->_feedback.showAlert(gps_enabled ? "GPS: ON" : (on ? "GPS unavailable" : "GPS: OFF"),
                              on && !gps_enabled ? 2000 : 800);
#if defined(HAS_LNA_CONTROL) && HAS_LNA_CONTROL
  } else if (sw == self->_swLna) {
    if (!app.setLnaEnabled(on)) {
      self->setSwitchState(self->_swLna, app.lnaEnabled());
      self->_feedback.showAlert("LNA unavailable", 2000);
      return;
    }
    self->setSwitchState(self->_swLna, app.lnaEnabled());
    self->_feedback.showAlert(on ? "LNA: ON" : "LNA: OFF", 800);
#endif
#ifdef PIN_BUZZER
  } else if (sw == self->_swBuzzer) {
    app.setBuzzerEnabled(on);
#if defined(HAS_BUZZER_VOLUME_CONTROL) && HAS_BUZZER_VOLUME_CONTROL
    self->setBuzzerVolumeSlider(app.buzzerVolumeLevel());
#endif
    self->setSwitchState(self->_swBuzzer, app.buzzerEnabled());
    self->_feedback.showAlert(on ? "Buzzer: ON" : "Buzzer: OFF", 800);
#endif
  } else if (sw == self->_swLocShare) {
    if (on && !app.gpsStatus().enabled) {
      app.setGpsEnabled(true);
      const bool gps_enabled = app.gpsStatus().enabled;
      self->setSwitchState(self->_swGps, gps_enabled);
      if (!gps_enabled) {
        self->_feedback.showAlert("GPS unavailable", 2000);
        self->setSwitchState(self->_swLocShare, false);
        return;
      }
    }
    app.setLocationShareEnabled(on);
    self->_feedback.showAlert(on ? "Loc share: ON" : "Loc share: OFF", 800);
    self->updateConditionalVisibility(app);
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  } else if (sw == self->_swGpsTrack) {
    if (on && !app.gpsStatus().enabled) {
      app.setGpsEnabled(true);
      const bool gps_enabled = app.gpsStatus().enabled;
      self->setSwitchState(self->_swGps, gps_enabled);
      if (!gps_enabled) {
        self->_feedback.showAlert("GPS unavailable", 2000);
        self->setSwitchState(self->_swGpsTrack, false);
        return;
      }
    }
    if (!app.setGpsTrackRecording(on)) {
      self->setSwitchState(self->_swGpsTrack, app.gpsTrackRecording());
      return;
    }
    self->_feedback.showAlert(on ? "GPS track ON" : "GPS track OFF", 800);
    self->updateConditionalVisibility(app);
#endif
  }
}

SystemScreen::ChoiceRow* SystemScreen::dropdownChoice(_lv_obj_t* dd) {
  if (!dd) return nullptr;
  const auto matches = [dd](const ChoiceRow& choice) {
    return dd == choice.dropdown || dd == choice.row;
  };
  if (matches(_choice_region)) return &_choice_region;
  if (matches(_choice_screen_off)) return &_choice_screen_off;
  if (matches(_choice_adv)) return &_choice_adv;
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  if (matches(_choice_ff_mode)) return &_choice_ff_mode;
  if (matches(_choice_friend)) return &_choice_friend;
#endif
  return nullptr;
}

const SystemScreen::ChoiceRow* SystemScreen::dropdownChoice(_lv_obj_t* dd) const {
  return const_cast<SystemScreen*>(this)->dropdownChoice(dd);
}

bool SystemScreen::isDropdownRow(_lv_obj_t* obj) const {
  return dropdownChoice(obj) != nullptr;
}

void SystemScreen::setDropdownOptions(_lv_obj_t* dd, const char* options) {
  ChoiceRow* const choice = dropdownChoice(dd);
  if (!choice || !choice->dropdown) return;
  lv_dropdown_set_options_static(choice->dropdown, options ? options : "");
}

void SystemScreen::onDropdownReleasedPre(lv_event_t* e) {
  auto* self = static_cast<SystemScreen*>(lv_event_get_user_data(e));
  if (!self) return;
  _lv_obj_t* const dd = lv_event_get_target(e);
  self->syncDropdownLayout(dd);
#if LV_USE_DROPDOWN != 0
  if (!ui_defer(realign_dropdown_list_async, dd)) realign_dropdown_list_async(dd);
#endif
}

void SystemScreen::onDropdownStateEvent(lv_event_t* e) {
  auto* self = static_cast<SystemScreen*>(lv_event_get_user_data(e));
  if (!self) return;
  _lv_obj_t* const dd = lv_event_get_target(e);
  if (!self->isDropdownRow(dd)) return;

  const lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_READY) {
    if (self->_open_dropdown && self->_open_dropdown != dd) self->closeOpenDropdowns();
    self->_open_dropdown = dd;
    self->_open_dropdown_original_index = lv_dropdown_get_selected(dd);
    lv_obj_add_state(dd, LV_STATE_EDITED);
    if (self->group()) lv_group_set_editing(self->group(), true);
    self->highlightFocusRow(focus_row_for(dd));
    self->syncDropdownLayout(dd);
#if LV_USE_DROPDOWN != 0
    if (!ui_defer(realign_dropdown_list_async, dd)) realign_dropdown_list_async(dd);
#endif
  } else if (code == LV_EVENT_CANCEL) {
    if (self->_open_dropdown == dd) self->_open_dropdown = nullptr;
    lv_obj_clear_state(dd, LV_STATE_EDITED);
    if (self->group()) lv_group_set_editing(self->group(), false);
  }
}

void SystemScreen::onDropdownValueChanged(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;
  auto* self = static_cast<SystemScreen*>(lv_event_get_user_data(e));
  if (!self || self->_syncing_dropdown) return;

  _lv_obj_t* const dd = lv_event_get_target(e);
  biz::IBizFacade& app = self->_biz;
  const auto* choice = self->dropdownChoice(dd);
  if (!choice) return;
  const int sel = (int)lv_dropdown_get_selected(dd);

  if (dd == self->_dd_region) {
    app.setLoRaBandPresetIndex(sel);
    self->_feedback.showAlert("LoRa region saved", 2000);
  } else if (dd == self->_dd_screen_off) {
    app.setDisplayAutoOffIndex(sel);
    self->_feedback.showAlert("Screen off saved", 2000);
  } else if (dd == self->_dd_adv) {
    app.setLocShareIntervalIndex(sel);
    self->_feedback.showAlert("Adv interval saved", 2000);
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  } else if (dd == self->_dd_ff_mode) {
    if (!app.setFindFriendMode(sel)) {
      self->setDropdownIndex(self->_dd_ff_mode, (uint16_t)app.findFriendMode(), false, true);
      return;
    }
    if (sel == 0) {
      app.syncFindFriendContactList();
      self->syncFriendDropdownFromApp(app, true);
    }
    self->updateConditionalVisibility(app);
    self->_feedback.showAlert("Mode saved", 2000);
  } else if (dd == self->_dd_friend) {
    int mesh_idx = -1;
    if (sel > 0 && sel - 1 < self->_friend_mesh_map_count) {
      mesh_idx = self->_friend_mesh_map[sel - 1];
    }
    app.setFindFriendTargetContactIndex(mesh_idx);
    self->_feedback.showAlert("Friend selected", 2000);
#endif
  }
}

void SystemScreen::onExit() {
  closeActionConfirmation();
  closeOpenDropdowns();
  clearFocusRowHighlight();
  clearGroupFocusVisual();
  AbstractScreen::onExit();
}

void SystemScreen::setDropdownIndex(_lv_obj_t* dd, uint16_t index, bool fire_changed, bool force) {
  ChoiceRow* const choice = dropdownChoice(dd);
  if (!choice || !choice->dropdown) return;
  const uint16_t cnt = lv_dropdown_get_option_cnt(choice->dropdown);
  if (cnt == 0) return;
  if (index >= cnt) index = cnt - 1;
  if (!force && lv_dropdown_get_selected(choice->dropdown) == index && !fire_changed) return;
  _syncing_dropdown = true;
  lv_dropdown_set_selected(choice->dropdown, index);
  _syncing_dropdown = false;
  if (fire_changed) lv_event_send(choice->dropdown, LV_EVENT_VALUE_CHANGED, nullptr);
}

bool SystemScreen::anyDropdownOpen() const {
  return _open_dropdown && lv_obj_is_valid(_open_dropdown) && lv_dropdown_is_open(_open_dropdown);
}

void SystemScreen::ensureKeypadFocus() {
  if (!group()) return;
  lv_group_t* const g = group();
  if (lv_group_get_obj_count(g) == 0) rebuildKeypadGroup(_biz);
  if (lv_group_get_obj_count(g) == 0) return;

  if (lv_group_get_focused(g)) return;

  clearFocusRowHighlight();
  if (_dd_region && lv_obj_get_group(_dd_region) == g) {
    lv_group_focus_obj(_dd_region);
    applyGroupFocus(_dd_region);
    return;
  }
  lv_group_focus_next(g);
  applyGroupFocus(lv_group_get_focused(g));
}

lv_obj_t* SystemScreen::focusedObject() const {
  if (!group()) return nullptr;
  lv_obj_t* foc = lv_group_get_focused(group());
  if (foc) return foc;
  if (lv_group_get_obj_count(group()) == 0) return nullptr;
  const_cast<SystemScreen*>(this)->ensureKeypadFocus();
  return lv_group_get_focused(group());
}

void SystemScreen::syncDropdownsFromApp(const biz::IBizFacade& app) {
  setDropdownIndex(_dd_region, (uint16_t)app.currentLoRaBandPresetIndex(), false);
  setDropdownIndex(_dd_screen_off, (uint16_t)app.displayAutoOffIndex(), false);
  setDropdownIndex(_dd_adv, (uint16_t)app.locShareIntervalIndex(), false);
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  setDropdownIndex(_dd_ff_mode, (uint16_t)app.findFriendMode(), false);
  syncFriendDropdownFromApp(app, false);
#endif
}

void SystemScreen::setSwitchState(_lv_obj_t* sw, bool on) {
  sync_switch(sw, on, &_syncing_switch);
}

void SystemScreen::syncSwitchesFromApp(const biz::IBizFacade& app) {
  setSwitchState(_swBle, app.companionLinkEnabled());
  setSwitchState(_swGps, app.gpsStatus().enabled);
#if defined(HAS_LNA_CONTROL) && HAS_LNA_CONTROL
  setSwitchState(_swLna, app.lnaEnabled());
#endif
#ifdef PIN_BUZZER
  setSwitchState(_swBuzzer, app.buzzerEnabled());
#endif
#if defined(HAS_BUZZER_VOLUME_CONTROL) && HAS_BUZZER_VOLUME_CONTROL
  setBuzzerVolumeSlider(app.buzzerVolumeLevel());
#endif
  setSwitchState(_swLocShare, app.locationShareEnabled());
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  setSwitchState(_swGpsTrack, app.gpsTrackRecording());
#endif
}

void SystemScreen::syncControlsFromApp(const biz::IBizFacade& app) {
  syncSwitchesFromApp(app);
  syncDropdownsFromApp(app);
  updateConditionalVisibility(app);
}

void SystemScreen::applyActionRowThemes() {
  if (!_swBle) return;
  apply_action_row_theme(_row_factory_reset, _swBle);
  apply_action_row_theme(_row_clear_data, _swBle);
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  apply_action_row_theme(_row_wp_gps, _swBle);
  apply_action_row_theme(_row_wp_manual, _swBle);
#endif
}

void SystemScreen::refreshControls() {
  syncSwitchesFromApp(_biz);
  if (!anyDropdownOpen()) {
    setDropdownIndex(_dd_region, (uint16_t)_biz.currentLoRaBandPresetIndex(), false);
    setDropdownIndex(_dd_screen_off, (uint16_t)_biz.displayAutoOffIndex(), false);
    if (_biz.locationShareEnabled()) {
      setDropdownIndex(_dd_adv, (uint16_t)_biz.locShareIntervalIndex(), false);
    }
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
    setDropdownIndex(_dd_ff_mode, (uint16_t)_biz.findFriendMode(), false);
#endif
  }
  updateConditionalVisibility(_biz);
}

void SystemScreen::onAppStateChanged(const AppStateEvent& event) {
  if (event.type == AppStateEventType::ConfigChanged ||
      event.type == AppStateEventType::GpsChanged ||
      event.type == AppStateEventType::RadioChanged ||
      event.type == AppStateEventType::FindFriendChanged) {
    refreshControls();
  }
}

void SystemScreen::onRefreshRequested() {
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  if (_biz.findFriendMode() == 0) _biz.syncFindFriendContactList();
#endif
  syncSwitchesFromApp(_biz);
  syncDropdownsFromApp(_biz);
  _keypad_group_mask = 0xFF;
  updateConditionalVisibility(_biz);
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  if (_biz.findFriendMode() == 0) syncFriendDropdownFromApp(_biz, true);
#endif
  ensureKeypadFocus();
}

#if defined(HAS_BUZZER_VOLUME_CONTROL) && HAS_BUZZER_VOLUME_CONTROL

void SystemScreen::setBuzzerVolumeSlider(uint8_t level) {
  if (!_sliderBuzzerVolume) return;
  if (level > 3) level = 3;
  if ((uint8_t)lv_slider_get_value(_sliderBuzzerVolume) == level) return;
  lv_slider_set_value(_sliderBuzzerVolume, level, LV_ANIM_OFF);
}

void SystemScreen::setBuzzerVolumeLevel(uint8_t level, bool show_feedback) {
  if (level > 3) level = 3;
  setBuzzerVolumeSlider(level);
  _biz.setBuzzerVolumeLevel(level);
  setSwitchState(_swBuzzer, _biz.buzzerEnabled());
  if (!show_feedback) return;

#if defined(HELTEC_BUZZER_PWM_VOLUME_CONTROL) && HELTEC_BUZZER_PWM_VOLUME_CONTROL
  static const char* const labels[] = {"Volume: OFF", "Volume: LOW",
                                       "Volume: MEDIUM", "Volume: HIGH"};
#else
  static const char* const labels[] = {"Volume: OFF", "Volume: 1x",
                                       "Volume: 2x", "Volume: 3x"};
#endif
  _feedback.showAlert(labels[level], 1000);
}

void SystemScreen::onBuzzerVolumeButtonClicked(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  auto* self = static_cast<SystemScreen*>(lv_event_get_user_data(e));
  if (!self) return;

  const _lv_obj_t* target = lv_event_get_target(e);
  const int current_level = self->_biz.buzzerVolumeLevel();
  int level = current_level;
  if (target == self->_btnBuzzerVolumeDown) --level;
  else if (target == self->_btnBuzzerVolumeUp) ++level;
  else return;

  if (level < 0) level = 0;
  if (level > 3) level = 3;
  if (level != current_level) {
    self->setBuzzerVolumeLevel((uint8_t)level, true);
  }
  lv_event_stop_processing(e);
  lv_event_stop_bubbling(e);
}

#endif

}  // namespace heltec::meshcore::ui
