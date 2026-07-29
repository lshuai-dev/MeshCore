#include "system_screen.hpp"

#include "ui/app/ui_theme.hpp"
#include <Arduino.h>
#include <string.h>

namespace heltec::meshcore::ui {
namespace {
void sync_switch(_lv_obj_t* sw,bool on,bool* syncing){if(!sw)return;const bool cur=lv_obj_has_state(sw,LV_STATE_CHECKED);if(cur==on)return;if(syncing)*syncing=true;if(on)lv_obj_add_state(sw,LV_STATE_CHECKED);else lv_obj_clear_state(sw,LV_STATE_CHECKED);if(syncing)*syncing=false;}

bool copy_dropdown_option(const char* options, int wanted, char* buf, size_t cap) {
  if (!options || !buf || cap == 0 || wanted < 0) return false;
  int index = 0;
  const char* start = options;
  for (const char* p = options;; ++p) {
    if (*p == '\n' || *p == '\0') {
      if (index == wanted) {
        size_t len = static_cast<size_t>(p - start);
        if (len >= cap) len = cap - 1;
        memcpy(buf, start, len);
        buf[len] = '\0';
        return true;
      }
      if (*p == '\0') return false;
      ++index;
      start = p + 1;
    }
  }
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
      if (!gps_enabled) {
        self->_feedback.showAlert("GPS unavailable", 2000);
        self->setSwitchState(self->_swLocShare, false);
        return;
      }
    }
    app.setLocationShareEnabled(on);
    self->_feedback.showAlert(on ? "Loc share: ON" : "Loc share: OFF", 800);
    self->updateConditionalVisibility(app);
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
  lv_indev_t* const indev = lv_indev_get_act();
  if (indev && lv_indev_get_scroll_obj(indev)) return;
  ChoiceRow* const choice = self->dropdownChoice(dd);
  if (!choice || !self->openChoicePicker(choice)) return;
  lv_event_stop_processing(e);
  lv_event_stop_bubbling(e);
}

bool SystemScreen::openChoicePicker(ChoiceRow* choice) {
  if (!choice || !choice->dropdown || _active_choice) return false;
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  if (choice == &_choice_friend) syncFriendDropdownFromApp(_biz, true);
#endif
  if (lv_dropdown_is_open(choice->dropdown)) lv_dropdown_close(choice->dropdown);
  if (group()) lv_group_set_editing(group(), false);
  _choice_picker_return_focus = choice->dropdown;
  _choice_picker_scroll_y = _root ? lv_obj_get_scroll_top(_root) : 0;
  _active_choice = choice;
  if (emitEvent(UiEventType::ChoicePickerOpen, static_cast<IChoicePickerSource*>(this))) {
    return true;
  }
  _active_choice = nullptr;
  _choice_picker_return_focus = nullptr;
  return false;
}

const char* SystemScreen::choicePickerTitle() const {
  return _active_choice && _active_choice->title ? _active_choice->title : "Select";
}

int SystemScreen::choicePickerOptionCount() {
  if (!_active_choice || !_active_choice->dropdown) return 0;
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  if (_active_choice == &_choice_friend) {
    syncFriendDropdownFromApp(_biz, false);
    return _friend_total + 1;
  }
#endif
  return static_cast<int>(lv_dropdown_get_option_cnt(_active_choice->dropdown));
}

int SystemScreen::choicePickerSelectedIndex() const {
  if (!_active_choice || !_active_choice->dropdown) return 0;
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  if (_active_choice == &_choice_friend) {
    return _friend_selected_rank >= 0 ? _friend_selected_rank + 1 : 0;
  }
#endif
  return static_cast<int>(lv_dropdown_get_selected(_active_choice->dropdown));
}

bool SystemScreen::choicePickerOptionLabel(int index, char* buf, size_t cap) {
  if (!_active_choice || !_active_choice->dropdown || !buf || cap == 0) return false;
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  if (_active_choice == &_choice_friend) {
    if (index == 0) {
      lv_snprintf(buf, cap, "(none)");
      return true;
    }
    const int rank = index - 1;
    if (rank < 0 || rank >= _friend_total) return false;
    if (rank < _friend_window_start ||
        rank >= _friend_window_start + _friend_mesh_map_count) {
      int start = rank - kFriendWindowSize / 2;
      const int max_start =
          _friend_total > kFriendWindowSize ? _friend_total - kFriendWindowSize : 0;
      if (start < 0) start = 0;
      if (start > max_start) start = max_start;
      loadFriendDropdownWindow(_biz, start, _friend_selected_rank, true);
    }
    const int local = rank - _friend_window_start;
    if (local < 0 || local >= _friend_mesh_map_count) return false;
    return _biz.findFriendContactLabel(_friend_mesh_map[local], buf, cap);
  }
#endif
  return copy_dropdown_option(lv_dropdown_get_options(_active_choice->dropdown), index, buf, cap);
}

void SystemScreen::choicePickerCommit(int index) {
  ChoiceRow* const choice = _active_choice;
  if (!choice || !choice->dropdown) return;
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  if (choice == &_choice_friend) {
    int mesh_index = -1;
    if (index > 0) {
      biz::IBizFacade::FindFriendContactItem item{};
      int total = 0;
      int selected_rank = -1;
      if (_biz.fillFindFriendContacts(index - 1, -1, &item, 1, &total,
                                      &selected_rank) == 1) {
        mesh_index = item.contact_index;
      }
    }
    _biz.setFindFriendTargetContactIndex(mesh_index);
    syncFriendDropdownFromApp(_biz, true);
    _feedback.showAlert("Friend selected", 2000);
    return;
  }
#endif
  const int count = static_cast<int>(lv_dropdown_get_option_cnt(choice->dropdown));
  if (count <= 0) return;
  if (index < 0) index = 0;
  if (index >= count) index = count - 1;
  const int old = static_cast<int>(lv_dropdown_get_selected(choice->dropdown));
  setDropdownIndex(choice->dropdown, static_cast<uint16_t>(index), false, true);
  if (index != old) lv_event_send(choice->dropdown, LV_EVENT_VALUE_CHANGED, nullptr);
}

void SystemScreen::choicePickerClosed(bool committed) {
  (void)committed;
  ChoiceRow* const closing_choice = _active_choice;
  _lv_obj_t* const return_focus = _choice_picker_return_focus;
  _active_choice = nullptr;
  _choice_picker_return_focus = nullptr;
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  if (closing_choice == &_choice_friend) syncFriendDropdownFromApp(_biz, true);
#endif
  if (return_focus && group() && lv_obj_is_valid(return_focus) &&
      lv_obj_get_group(return_focus) == group() &&
      lv_group_get_focused(group()) != return_focus) {
    lv_group_focus_obj(return_focus);
  }
  if (_root) lv_obj_scroll_to_y(_root, _choice_picker_scroll_y, LV_ANIM_OFF);
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
    const int mesh_idx = self->friendMeshIndexForSelection();
    app.setFindFriendTargetContactIndex(mesh_idx);
    self->syncFriendDropdownFromApp(app, true);
    self->_feedback.showAlert("Friend selected", 2000);
#endif
  }
}

void SystemScreen::onExit() {
  closeActionConfirmation();
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
#ifdef PIN_BUZZER
  setSwitchState(_swBuzzer, app.buzzerEnabled());
#endif
#if defined(HAS_BUZZER_VOLUME_CONTROL) && HAS_BUZZER_VOLUME_CONTROL
  setBuzzerVolumeSlider(app.buzzerVolumeLevel());
#endif
  setSwitchState(_swLocShare, app.locationShareEnabled());
}

void SystemScreen::syncControlsFromApp(const biz::IBizFacade& app) {
  syncSwitchesFromApp(app);
  syncDropdownsFromApp(app);
  updateConditionalVisibility(app);
}

void SystemScreen::refreshControls() {
  syncSwitchesFromApp(_biz);
  if (!_active_choice) {
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
  updateConditionalVisibility(_biz);
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  if (_biz.findFriendMode() == 0) syncFriendDropdownFromApp(_biz, true);
#endif
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
