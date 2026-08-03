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

char s_region_options[kRadioParamPresetUiScratchSize + 32];
char s_screen_off_options[64];

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

bool screen_off_label(const biz::IBizFacade& app, int i, char* lab, size_t cap) {
  const char* s = app.displayAutoOffOptionLabel(i);
  lv_snprintf(lab, cap, "%s", s ? s : "?");
  return true;
}

void build_region_options() {
  char* const presets = radioParamPresetUiScratch();
  radioParamPresetDropdownOptions(presets, kRadioParamPresetUiScratchSize);
  lv_snprintf(s_region_options, sizeof(s_region_options),
              "Custom/fwd\n%s", presets);
}

}  // namespace

bool SystemScreen::onKey(uint32_t key) {
  if (handleConfirmationKey(key)) return true;
  return AbstractScreen::onKey(key);
}

_lv_obj_t* SystemScreen::create(_lv_obj_t* parent) {
  if (!AbstractScreen::create(parent)) return nullptr;
  _dd_region = addDropdownRow(_root, _choice_region, "Region");
  if (_dd_region) {
    build_region_options();
    setDropdownOptions(_dd_region, s_region_options);
  }

  addSwitchRow(_root, "Enable Repeat Mode", &_swForwarding);

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
  addFocusItem(_swForwarding,
               _swForwarding ? lv_obj_get_parent(_swForwarding) : nullptr,
               kFocusOnPointerPress);
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
  addFocusItem(_row_factory_reset, nullptr, kFocusOnPointerPress, FocusVisual::Row);
  addFocusItem(_row_clear_data, nullptr, kFocusOnPointerPress, FocusVisual::Row);

  // System owns focus scrolling. Disable LVGL's implicit auto-scroll so focus
  // restore, pointer focus and modal transitions cannot move the page behind us.
  _lv_obj_t* const focus_controls[] = {
      _dd_region,
      _swForwarding,
      _dd_screen_off,
      _swBle,
#ifdef PIN_BUZZER
      _swBuzzer,
#endif
#if defined(HAS_BUZZER_VOLUME_CONTROL) && HAS_BUZZER_VOLUME_CONTROL
      _btnBuzzerVolumeDown,
      _btnBuzzerVolumeUp,
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
  closeOpenDropdown();
}

}  // namespace heltec::meshcore::ui
