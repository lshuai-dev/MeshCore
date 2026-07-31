#include "system_screen.hpp"

#include "ui/app/ui_theme.hpp"
#include "ui/core/ui_events.h"
#include "ui/core/ht_meta_data.hpp"
#include <lvgl.h>
#include <string.h>

#include "../core/biz_facade.hpp"
#include "config/LoRaBandPresets.h"
#include <Arduino.h>

namespace heltec::meshcore::ui {

_lv_obj_t* SystemScreen::createRoot(_lv_obj_t* parent) {
  return ht_obj_create(parent, meta_id::SystemRoot);
}

namespace {

char s_adv_dd_options[64];
char s_screen_off_options[64];
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
char s_friend_dd_options[384];
#endif

int append_options(char* buf, size_t cap, const biz::IBizFacade& app, int count,
                   bool (*label_fn)(const biz::IBizFacade&, int, char*, size_t)) {
  char* p = buf;
  size_t rem = cap;
  for (int i = 0; i < count; ++i) {
    char lab[32];
    if (!label_fn(app, i, lab, sizeof(lab))) continue;
    const int n = lv_snprintf(p, rem, "%s%s", lab, (i + 1 < count) ? "\n" : "");
    if (n < 0 || (size_t)n >= rem) break;
    p += n;
    rem -= (size_t)n;
  }
  return (int)(p - buf);
}

bool adv_label(const biz::IBizFacade& app, int i, char* lab, size_t cap) {
  const char* s = app.locShareIntervalOptionLabel(i);
  lv_snprintf(lab, cap, "%s", s ? s : "?");
  return true;
}

bool screen_off_label(const biz::IBizFacade& app, int i, char* lab, size_t cap) {
  const char* s = app.displayAutoOffOptionLabel(i);
  lv_snprintf(lab, cap, "%s", s ? s : "?");
  return true;
}

uint32_t options_hash(const char* text) {
  uint32_t hash = 2166136261UL;
  if (!text) return hash;
  for (const uint8_t* p = reinterpret_cast<const uint8_t*>(text); *p; ++p) {
    hash ^= *p;
    hash *= 16777619UL;
  }
  return hash;
}

void set_row_hidden(_lv_obj_t* row, bool hidden) {
  if (!row) return;
  if (hidden) lv_obj_add_flag(row, LV_OBJ_FLAG_HIDDEN);
  else lv_obj_clear_flag(row, LV_OBJ_FLAG_HIDDEN);
}

}  // namespace

bool SystemScreen::onKey(uint32_t key) {
  if (handleConfirmationKey(key)) return true;
  return AbstractScreen::onKey(key);
}

#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
void SystemScreen::syncFriendDropdownFromApp(const biz::IBizFacade& app, bool force) {
  if (!_dd_friend) return;
  biz::IBizFacade::FindFriendContactItem probe[kFriendWindowSize]{};
  int total = 0;
  int selected_rank = -1;
  (void)app.fillFindFriendContacts(_friend_window_start, app.findFriendTargetContactIndex(),
                                   probe, kFriendWindowSize, &total, &selected_rank);

  const int max_start = total > kFriendWindowSize ? total - kFriendWindowSize : 0;
  int start = _friend_window_start;
  if (start > max_start) start = max_start;
  if (selected_rank >= 0) {
    const int local = selected_rank - start;
    if (local < 0 || local >= kFriendWindowSize) {
      start = (selected_rank / kFriendWindowStep) * kFriendWindowStep;
    } else if (local >= kFriendWindowSize - 2 && start < max_start) {
      start += kFriendWindowStep;
    } else if (local <= 1 && start > 0) {
      start -= kFriendWindowStep;
    }
  }
  if (start < 0) start = 0;
  if (start > max_start) start = max_start;
  loadFriendDropdownWindow(app, start, selected_rank, force);
}

void SystemScreen::loadFriendDropdownWindow(const biz::IBizFacade& app, int start,
                                            int selected_rank, bool force) {
  if (!_dd_friend) return;
  biz::IBizFacade::FindFriendContactItem items[kFriendWindowSize]{};
  int total = 0;
  int ignored_rank = -1;
  _friend_mesh_map_count = app.fillFindFriendContacts(
      start, -1, items, kFriendWindowSize, &total, &ignored_rank);
  _friend_total = total;
  const int max_start = total > kFriendWindowSize ? total - kFriendWindowSize : 0;
  if (start < 0) start = 0;
  if (start > max_start) start = max_start;
  _friend_window_start = start;
  _friend_selected_rank = selected_rank;

  lv_snprintf(s_friend_dd_options, sizeof(s_friend_dd_options), "(none)");
  for (int i = 0; i < _friend_mesh_map_count; ++i) {
    _friend_mesh_map[i] = items[i].contact_index;
    const size_t used = strlen(s_friend_dd_options);
    if (used >= sizeof(s_friend_dd_options) - 1) break;
    lv_snprintf(s_friend_dd_options + used, sizeof(s_friend_dd_options) - used,
                "\n%s", items[i].label[0] ? items[i].label : "?");
  }

  const uint32_t hash = options_hash(s_friend_dd_options);
  const bool options_changed = force || _friend_mesh_map_count != _friend_mesh_map_count_applied ||
                               hash != _friend_dd_options_hash_applied;

  if (options_changed) {
    _friend_dd_options_hash_applied = hash;
    _friend_mesh_map_count_applied = _friend_mesh_map_count;
    setDropdownOptions(_dd_friend, s_friend_dd_options);
  }
  int local_selection = 0;
  if (selected_rank >= _friend_window_start &&
      selected_rank < _friend_window_start + _friend_mesh_map_count) {
    local_selection = selected_rank - _friend_window_start + 1;
  }
  setDropdownIndex(_dd_friend, (uint16_t)local_selection, false, options_changed);
}

int SystemScreen::friendMeshIndexForSelection() const {
  if (!_dd_friend) return -1;
  const int selected = (int)lv_dropdown_get_selected(_dd_friend);
  if (selected <= 0 || selected - 1 >= _friend_mesh_map_count) return -1;
  return _friend_mesh_map[selected - 1];
}

#endif

void SystemScreen::updateConditionalVisibility(const biz::IBizFacade& app) {
  const bool loc_share = app.locationShareEnabled();
  set_row_hidden(_row_adv, !loc_share);
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  const bool friend_mode = app.findFriendMode() == 0;
  set_row_hidden(_row_friend, !friend_mode);
  set_row_hidden(_row_wp_gps, friend_mode);
  set_row_hidden(_row_wp_manual, friend_mode);
#endif
}

_lv_obj_t* SystemScreen::create(_lv_obj_t* parent) {
  if (!AbstractScreen::create(parent)) return nullptr;
  _dd_region = addDropdownRow(_root, _choice_region, "Region");
  if (_dd_region) {
    char* const options = radioParamPresetUiScratch();
    radioParamPresetDropdownOptions(options, kRadioParamPresetUiScratchSize);
    setDropdownOptions(_dd_region, options);
  }

  _dd_screen_off = addDropdownRow(_root, _choice_screen_off, "Screen off");
  if (_dd_screen_off) {
    append_options(s_screen_off_options, sizeof(s_screen_off_options), _biz, _biz.displayAutoOffOptionCount(),
                   screen_off_label);
    setDropdownOptions(_dd_screen_off, s_screen_off_options);
  }

  addSwitchRow(_root, "Bluetooth", &_swBle);
#ifdef PIN_BUZZER
  addSwitchRow(_root, "Buzzer", &_swBuzzer);
#endif
#if defined(HAS_BUZZER_VOLUME_CONTROL) && HAS_BUZZER_VOLUME_CONTROL
  addBuzzerVolumeRow(_root);
#endif
  addSwitchRow(_root, "Location share", &_swLocShare);

  _dd_adv = addDropdownRow(_root, _choice_adv, "Adv interval");
  _row_adv = _choice_adv.row;
  if (_dd_adv) {
    append_options(s_adv_dd_options, sizeof(s_adv_dd_options), _biz, _biz.locShareIntervalOptionCount(), adv_label);
    setDropdownOptions(_dd_adv, s_adv_dd_options);
  }

#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  _dd_ff_mode = addDropdownRow(_root, _choice_ff_mode, "Find mode");
  if (_dd_ff_mode) setDropdownOptions(_dd_ff_mode, "Friend\nWaypoint");

  _dd_friend = addDropdownRow(_root, _choice_friend, "Friend");
  _row_friend = _choice_friend.row;
  addActionRow(_root, "> Use current GPS", &_row_wp_gps);
  addActionRow(_root, "> Enter lat,lon", &_row_wp_manual);
#endif

  addActionRow(_root, "> Factory reset", &_row_factory_reset);
  addActionRow(_root, "> Clear data", &_row_clear_data);

#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
  // LVGL does not emit CLICKED after a drag, so defer pointer focus until the
  // gesture has been classified as an actual tap.
  constexpr bool kFocusOnPointerPress = false;
#else
  constexpr bool kFocusOnPointerPress = true;
#endif

  addFocusItem(_dd_region, _choice_region.row, kFocusOnPointerPress);
  addFocusItem(_dd_screen_off, _choice_screen_off.row, kFocusOnPointerPress);
  addFocusItem(_swBle, _swBle ? lv_obj_get_parent(_swBle) : nullptr,
               kFocusOnPointerPress);
#ifdef PIN_BUZZER
  addFocusItem(_swBuzzer, _swBuzzer ? lv_obj_get_parent(_swBuzzer) : nullptr,
               kFocusOnPointerPress);
#endif
#if defined(HAS_BUZZER_VOLUME_CONTROL) && HAS_BUZZER_VOLUME_CONTROL
  addFocusItem(_btnBuzzerVolumeDown, nullptr, kFocusOnPointerPress);
  addFocusItem(_btnBuzzerVolumeUp, nullptr, kFocusOnPointerPress);
#endif
  addFocusItem(_swLocShare, _swLocShare ? lv_obj_get_parent(_swLocShare) : nullptr,
               kFocusOnPointerPress);
  addFocusItem(_dd_adv, _choice_adv.row, kFocusOnPointerPress);
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  addFocusItem(_dd_ff_mode, _choice_ff_mode.row, kFocusOnPointerPress);
  addFocusItem(_dd_friend, _choice_friend.row, kFocusOnPointerPress);
  addFocusItem(_row_wp_gps, nullptr, kFocusOnPointerPress);
  addFocusItem(_row_wp_manual, nullptr, kFocusOnPointerPress);
#endif
  addFocusItem(_row_factory_reset, nullptr, kFocusOnPointerPress);
  addFocusItem(_row_clear_data, nullptr, kFocusOnPointerPress);

  // System owns focus scrolling. Disable LVGL's implicit auto-scroll so focus
  // restore, pointer focus and modal transitions cannot move the page behind us.
  _lv_obj_t* const focus_controls[] = {
      _dd_region,
      _dd_screen_off,
      _swBle,
#ifdef PIN_BUZZER
      _swBuzzer,
#endif
#if defined(HAS_BUZZER_VOLUME_CONTROL) && HAS_BUZZER_VOLUME_CONTROL
      _btnBuzzerVolumeDown,
      _btnBuzzerVolumeUp,
#endif
      _swLocShare,
      _dd_adv,
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
      _dd_ff_mode,
      _dd_friend,
      _row_wp_gps,
      _row_wp_manual,
#endif
      _row_factory_reset,
      _row_clear_data,
  };
  for (_lv_obj_t* control : focus_controls) {
    if (control) lv_obj_clear_flag(control, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
  }

  (void)createActionConfirmation();
  ht_set_user_data(_root, this);

  lv_group_set_focus_cb(
      group(),
      +[](lv_group_t* g) {
        lv_obj_t* foc = lv_group_get_focused(g);
        if (!foc) return;
        for (lv_obj_t* p = foc; p; p = lv_obj_get_parent(p)) {
          void* ud = ht_user_data(p);
          if (!ud) continue;
          auto* self = static_cast<SystemScreen*>(ud);
          if (self->root() == p) {
            self->applyGroupFocus(foc);
            return;
          }
        }
      });

  return _root;
}

void SystemScreen::onEnter() {
  AbstractScreen::onEnter();
  _active_choice = nullptr;
  _choice_picker_return_focus = nullptr;
}

void SystemScreen::onUiEvent(const UiEvent& event) {
  if (event.type == UiEventType::WaypointKeyboardClosed) {
    _lv_obj_t* const return_focus = _waypoint_keyboard_return_focus;
    _waypoint_keyboard_return_focus = nullptr;
    if (focusKeypadWidget(return_focus)) return;
    applyGroupFocus(group() ? lv_group_get_focused(group()) : nullptr);
    return;
  }

  if (event.type != UiEventType::WaypointKeyboardSubmit || !event.payload) return;
  const auto* submit = static_cast<const UiWaypointKeyboardSubmit*>(event.payload);
  if (_biz.setFindFriendWaypoint(submit->lat, submit->lon)) {
    _feedback.showAlert("Waypoint saved", 2000);
  } else {
    _feedback.showAlert("Save failed", 2000);
  }
}

/*
 * Waypoint keyboard results are delivered through onUiEvent so the generic
 * AbstractScreen contract does not need waypoint-specific callbacks.
 */

}  // namespace heltec::meshcore::ui
