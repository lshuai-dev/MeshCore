#include "system_screen.hpp"

#include "ui/app/ui_theme.hpp"
#include "ui/core/ui_events.h"
#include "ui/core/ht_meta_data.hpp"
#include "ui/core/ui_deferred_queue.hpp"
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
char s_friend_dd_options[1024];
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

int friend_dropdown_index(const biz::IBizFacade& app, const int16_t* mesh_map,
                          int mesh_map_count) {
  const int idx = app.findFriendTargetContactIndex();
  if (idx < 0) return 0;
  for (int i = 0; i < mesh_map_count; ++i) {
    if (mesh_map[i] == idx) return i + 1;
  }
  return 0;
}

uint8_t keypad_group_mask_for(const biz::IBizFacade& app) {
  const bool loc_share = app.locationShareEnabled();
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  const bool friend_mode = app.findFriendMode() == 0;
  return static_cast<uint8_t>((loc_share ? 1u : 0u) | (friend_mode ? 2u : 0u) | (!friend_mode ? 4u : 0u));
#else
  return static_cast<uint8_t>(loc_share ? 1u : 0u);
#endif
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

  _friend_mesh_map_count = app.buildFindFriendDropdownOptions(
      s_friend_dd_options, sizeof(s_friend_dd_options), _friend_mesh_map,
      (int)(sizeof(_friend_mesh_map) / sizeof(_friend_mesh_map[0])));

  const uint32_t hash = options_hash(s_friend_dd_options);
  const bool options_changed = force || _friend_mesh_map_count != _friend_mesh_map_count_applied ||
                               hash != _friend_dd_options_hash_applied;

  if (options_changed) {
    _friend_dd_options_hash_applied = hash;
    _friend_mesh_map_count_applied = _friend_mesh_map_count;
    setDropdownOptions(_dd_friend, s_friend_dd_options);
    setDropdownIndex(_dd_friend,
                     (uint16_t)friend_dropdown_index(app, _friend_mesh_map, _friend_mesh_map_count), false,
                     true);
  } else {
    setDropdownIndex(_dd_friend,
                     (uint16_t)friend_dropdown_index(app, _friend_mesh_map, _friend_mesh_map_count), false);
  }
}
#endif

void SystemScreen::rebuildKeypadGroup(const biz::IBizFacade& app) {
  if (!group()) return;
  lv_group_t* const g = group();
  lv_obj_t* const prev_focus = lv_group_get_focused(g);

  clearFocusObjects();

  const bool loc_share = app.locationShareEnabled();
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  const bool friend_mode = app.findFriendMode() == 0;
#endif

  addKeypadWidget(_dd_region);
  addKeypadWidget(_dd_screen_off);
  addKeypadWidget(_swBle);
  addKeypadWidget(_swGps);
#if defined(HAS_LNA_CONTROL) && HAS_LNA_CONTROL
  addKeypadWidget(_swLna);
#endif
#ifdef PIN_BUZZER
  addKeypadWidget(_swBuzzer);
#endif
#if defined(HAS_BUZZER_VOLUME_CONTROL) && HAS_BUZZER_VOLUME_CONTROL
  addKeypadWidget(_btnBuzzerVolumeDown);
  addKeypadWidget(_btnBuzzerVolumeUp);
#endif
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  addKeypadWidget(_swGpsTrack);
#endif
  addKeypadWidget(_swLocShare);
  if (loc_share) addKeypadWidget(_dd_adv);
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  addKeypadWidget(_dd_ff_mode);
  if (friend_mode) addKeypadWidget(_dd_friend);
  if (!friend_mode) {
    addKeypadWidget(_row_wp_gps);
    addKeypadWidget(_row_wp_manual);
  }
#endif
  addKeypadWidget(_row_factory_reset);
  addKeypadWidget(_row_clear_data);

  if (prev_focus && lv_obj_is_valid(prev_focus)) {
    lv_group_focus_obj(prev_focus);
  }
  if (!lv_group_get_focused(g) && lv_group_get_obj_count(g) > 0) {
    lv_group_focus_next(g);
  }
}

void SystemScreen::updateConditionalVisibility(const biz::IBizFacade& app) {
  const bool loc_share = app.locationShareEnabled();
  set_row_hidden(_row_adv, !loc_share);
  if (!loc_share && _open_dropdown == _dd_adv) closeOpenDropdowns();
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  const bool friend_mode = app.findFriendMode() == 0;
  set_row_hidden(_row_friend, !friend_mode);
  set_row_hidden(_row_wp_gps, friend_mode);
  set_row_hidden(_row_wp_manual, friend_mode);
  if (!friend_mode && _open_dropdown == _dd_friend) closeOpenDropdowns();
#endif
  const uint8_t mask = keypad_group_mask_for(app);
  if (mask != _keypad_group_mask) {
    rebuildKeypadGroup(app);
    _keypad_group_mask = mask;
  }
}

_lv_obj_t* SystemScreen::create(_lv_obj_t* parent) {
  if (!AbstractScreen::create(parent)) return nullptr;
  if (group()) lv_group_set_wrap(group(), true);
  lv_obj_add_flag(_root, LV_OBJ_FLAG_EVENT_BUBBLE | LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(_root, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(_root, LV_SCROLLBAR_MODE_OFF);

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
  addSwitchRow(_root, "GPS", &_swGps);
#if defined(HAS_LNA_CONTROL) && HAS_LNA_CONTROL
#if defined(HELTEC_V4_R8_TFT)
  addSwitchRow(_root, "LNA", &_swLna);
#else
  if (_biz.isLnaCanControl()) addSwitchRow(_root, "LNA", &_swLna);
#endif
#endif
#ifdef PIN_BUZZER
  addSwitchRow(_root, "Buzzer", &_swBuzzer);
#endif
#if defined(HAS_BUZZER_VOLUME_CONTROL) && HAS_BUZZER_VOLUME_CONTROL
  addBuzzerVolumeRow(_root);
#endif
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  addSwitchRow(_root, "GPS track", &_swGpsTrack);
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

  applyActionRowThemes();
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
  closeOpenDropdowns();
  ensureKeypadFocus();
}

void SystemScreen::onWaypointKeyboardClosed() {
  _lv_obj_t* const return_focus = _waypoint_keyboard_return_focus;
  _waypoint_keyboard_return_focus = nullptr;
  if (focusKeypadWidget(return_focus)) return;
  applyGroupFocus(group() ? lv_group_get_focused(group()) : nullptr);
}

void SystemScreen::onWaypointKeyboardSubmit(double lat, double lon) {
  if (_biz.setFindFriendWaypoint(lat, lon)) {
    _feedback.showAlert("Waypoint saved", 2000);
  } else {
    _feedback.showAlert("Save failed", 2000);
  }
}

}  // namespace heltec::meshcore::ui
