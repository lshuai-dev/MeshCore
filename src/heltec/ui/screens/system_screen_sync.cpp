#include "system_screen.hpp"

#include "ui/app/ui_theme.hpp"
#include "ui/core/ui_deferred_queue.hpp"
#include <Arduino.h>

namespace heltec::meshcore::ui {
namespace {
void sync_switch(_lv_obj_t* sw,bool on,bool* syncing){if(!sw)return;const bool cur=lv_obj_has_state(sw,LV_STATE_CHECKED);if(cur==on)return;if(syncing)*syncing=true;if(on)lv_obj_add_state(sw,LV_STATE_CHECKED);else lv_obj_clear_state(sw,LV_STATE_CHECKED);if(syncing)*syncing=false;}

const char* forwarding_error_text(biz::IBizFacade::ForwardingApplyResult result) {
  switch (result) {
    case biz::IBizFacade::ForwardingApplyResult::InvalidSelection:
      return "Invalid frequency";
    case biz::IBizFacade::ForwardingApplyResult::UnsupportedFrequency:
      return "Frequency unsupported";
    case biz::IBizFacade::ForwardingApplyResult::InvalidRadioParams:
      return "Invalid radio params";
    case biz::IBizFacade::ForwardingApplyResult::Unavailable:
      return "Forwarding unavailable";
    case biz::IBizFacade::ForwardingApplyResult::Ok:
      return nullptr;
  }
  return "Forwarding failed";
}
}  // namespace

void SystemScreen::onSwitchValueChanged(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;
  auto* self = static_cast<SystemScreen*>(lv_event_get_user_data(e));
  if (!self || self->_syncing_switch) return;

  _lv_obj_t* const sw = lv_event_get_target(e);
  const bool on = lv_obj_has_state(sw, LV_STATE_CHECKED);
  biz::IBizFacade& app = self->_biz;

  if (sw == self->_swForwarding) {
    if (on) {
      // Opening the picker is not the commit point. Keep the switch bound to
      // the persisted state until the frequency page applies a selection.
      self->setSwitchState(self->_swForwarding, app.forwardingEnabled());
      if (app.forwardingEnabled()) return;
      if (app.forwardingFrequencyCount() <= 0) {
        self->_feedback.showAlert("No forwarding frequency", 2000);
        return;
      }
      if (!self->emitEvent(UiEventType::RepeatModeOpen)) {
        self->_feedback.showAlert("Unable to open frequency picker", 2000);
      }
      return;
    }

    const biz::IBizFacade::ForwardingApplyResult result =
        app.setForwardingEnabled(false, -1);
    self->setSwitchState(self->_swForwarding, app.forwardingEnabled());
    const char* error = forwarding_error_text(result);
    self->_feedback.showAlert(error ? error : "Repeat mode disabled",
                              error ? 2000 : 1000);
  } else if (sw == self->_swBle) {
    app.setCompanionLinkEnabled(on);
    self->_feedback.showAlert(on ? "Bluetooth: ON" : "Bluetooth: OFF", 800);
#ifdef PIN_BUZZER
  } else if (sw == self->_swBuzzer) {
    app.setBuzzerEnabled(on);
#if defined(HAS_BUZZER_VOLUME_CONTROL) && HAS_BUZZER_VOLUME_CONTROL
    self->setBuzzerVolumeSlider(app.buzzerVolumeLevel());
#endif
    self->setSwitchState(self->_swBuzzer, app.buzzerEnabled());
    self->_feedback.showAlert(on ? "Buzzer: ON" : "Buzzer: OFF", 800);
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
  self->syncDropdownLayout(lv_event_get_target(e));
}

void SystemScreen::onDropdownStateEvent(lv_event_t* e) {
  auto* self = static_cast<SystemScreen*>(lv_event_get_user_data(e));
  if (!self) return;
  _lv_obj_t* const dropdown = lv_event_get_target(e);
  if (!self->isDropdownRow(dropdown)) return;

  const lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_READY) {
    if (self->_open_dropdown && self->_open_dropdown != dropdown) {
      self->closeOpenDropdown();
    }
    self->_open_dropdown = dropdown;
    self->_open_dropdown_original_index = lv_dropdown_get_selected(dropdown);
    if (_lv_obj_t* const list = lv_dropdown_get_list(dropdown)) {
      ui_theme_apply_dropdown_list(list);
      ui_theme_match_dropdown_list_padding(dropdown, list);
    }
    lv_obj_add_state(dropdown, LV_STATE_EDITED);
    if (self->group()) lv_group_set_editing(self->group(), true);
    self->syncDropdownLayout(dropdown);
    if (!ui_defer(realignDropdownListAsync, dropdown)) {
      self->realignDropdownList(dropdown);
    }
  } else if (code == LV_EVENT_CANCEL) {
    if (self->_open_dropdown == dropdown) self->_open_dropdown = nullptr;
    lv_obj_clear_state(dropdown, LV_STATE_EDITED);
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
    if (sel <= 0) {
      self->setDropdownIndex(self->_dd_region,
                             self->regionDropdownIndex(app), false, true);
      return;
    }
    const bool forwarding_was_enabled = app.forwardingEnabled();
    app.setLoRaBandPresetIndex(sel - 1);
    self->syncControlsFromApp(app);
    self->_feedback.showAlert(forwarding_was_enabled
                                  ? "Region saved; forwarding off"
                                  : "LoRa region saved",
                              2000);
  } else if (dd == self->_dd_screen_off) {
    app.setDisplayAutoOffIndex(sel);
    self->_feedback.showAlert("Screen off saved", 2000);
  }
}

void SystemScreen::onExit() {
  closeActionConfirmation();
  closeOpenDropdown();
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
  return _open_dropdown && lv_obj_is_valid(_open_dropdown) &&
         lv_dropdown_is_open(_open_dropdown);
}

uint16_t SystemScreen::regionDropdownIndex(const biz::IBizFacade& app) const {
  // Forwarding frequencies are not LoRa region presets. Index zero is the
  // non-actionable Custom/fwd status entry prepended by SystemScreen::create().
  if (app.forwardingEnabled()) return 0;
  const int exact = app.currentExactLoRaBandPresetIndex();
  return exact >= 0 ? (uint16_t)(exact + 1) : 0;
}

void SystemScreen::syncDropdownsFromApp(const biz::IBizFacade& app) {
  setDropdownIndex(_dd_region, regionDropdownIndex(app), false);
  setDropdownIndex(_dd_screen_off, (uint16_t)app.displayAutoOffIndex(), false);
}

void SystemScreen::setSwitchState(_lv_obj_t* sw, bool on) {
  sync_switch(sw, on, &_syncing_switch);
}

void SystemScreen::syncSwitchesFromApp(const biz::IBizFacade& app) {
  setSwitchState(_swForwarding, app.forwardingEnabled());
  setSwitchState(_swBle, app.companionLinkEnabled());
#ifdef PIN_BUZZER
  setSwitchState(_swBuzzer, app.buzzerEnabled());
#endif
#if defined(HAS_BUZZER_VOLUME_CONTROL) && HAS_BUZZER_VOLUME_CONTROL
  setBuzzerVolumeSlider(app.buzzerVolumeLevel());
#endif
}

void SystemScreen::syncControlsFromApp(const biz::IBizFacade& app) {
  syncSwitchesFromApp(app);
  syncDropdownsFromApp(app);
}

void SystemScreen::refreshControls() {
  syncSwitchesFromApp(_biz);
  if (!anyDropdownOpen()) {
    syncDropdownsFromApp(_biz);
  }
}

void SystemScreen::onAppStateChanged(const AppStateEvent& event) {
  if (event.type == AppStateEventType::ConfigChanged ||
      event.type == AppStateEventType::RadioChanged) {
    refreshControls();
  }
}

void SystemScreen::onRefreshRequested() {
  syncSwitchesFromApp(_biz);
  syncDropdownsFromApp(_biz);
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
