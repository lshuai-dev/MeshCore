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

char s_region_dd_options[640];
char s_adv_dd_options[64];
char s_screen_off_options[64];
char s_friend_dd_options[1024];

#if defined(HELTEC_V4_R8_TFT)
constexpr lv_coord_t kSystemDropdownHeight = 36;
constexpr lv_coord_t kSystemDropdownListPadVer = 2;
#endif

void sync_switch(_lv_obj_t* sw, bool on, bool* syncing) {
  if (!sw) return;
  const bool cur = lv_obj_has_state(sw, LV_STATE_CHECKED);
  if (cur == on) return;
  if (syncing) *syncing = true;
  if (on) lv_obj_add_state(sw, LV_STATE_CHECKED);
  else lv_obj_clear_state(sw, LV_STATE_CHECKED);
  if (syncing) *syncing = false;
}

void setup_focus_row(_lv_obj_t* row, _lv_obj_t* control) {
  ui_theme_apply_switch_row_focus(row, control);
}

void apply_action_row_theme(_lv_obj_t* row, _lv_obj_t* sw) {
  ui_theme_apply_switch_row_focus(row, sw);
}

void configure_system_row(_lv_obj_t* row, lv_flex_flow_t flow,
                          lv_flex_align_t main, lv_flex_align_t cross) {
  if (!row) return;
  lv_obj_set_size(row, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(row, flow);
  lv_obj_set_flex_align(row, main, cross, cross);
#if LV_COLOR_DEPTH == 1
  lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_row(row, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_column(row, 2, LV_PART_MAIN);
#else
  lv_obj_set_style_pad_all(row, 2, LV_PART_MAIN);
#if defined(HELTEC_V4_R8_TFT)
  lv_obj_set_style_pad_row(row, LV_DPX(10), LV_PART_MAIN);
  lv_obj_set_style_pad_column(row, LV_DPX(10), LV_PART_MAIN);
#endif
#endif
  lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(row, LV_OBJ_FLAG_EVENT_BUBBLE);
  lv_obj_set_scrollbar_mode(row, LV_SCROLLBAR_MODE_OFF);
}

void configure_system_label(_lv_obj_t* label, lv_coord_t width) {
  if (!label) return;
  lv_obj_set_width(label, width);
  lv_obj_clear_flag(label, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE |
                               LV_OBJ_FLAG_SCROLL_ON_FOCUS);
  lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
}

bool keypad_suppresses_bubble(_lv_obj_t* obj) {
  if (!obj) return false;
  if (ht_id(obj) == meta_id::SystemActionRow) return true;
#if LV_USE_SWITCH != 0
  if (lv_obj_check_type(obj, &lv_switch_class)) return true;
#endif
#if LV_USE_DROPDOWN != 0
  if (lv_obj_check_type(obj, &lv_dropdown_class)) return true;
#endif
#if LV_USE_SLIDER != 0
  if (lv_obj_check_type(obj, &lv_slider_class)) return true;
#endif
  return false;
}

#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
bool point_hits_obj(const _lv_obj_t* obj, lv_coord_t x, lv_coord_t y,
                    lv_coord_t extra = 0) {
  if (!obj || !lv_obj_is_valid(obj) || lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN)) {
    return false;
  }
  lv_area_t area;
  lv_obj_get_coords(const_cast<_lv_obj_t*>(obj), &area);
  area.x1 -= extra;
  area.y1 -= extra;
  area.x2 += extra;
  area.y2 += extra;
  return x >= area.x1 && x <= area.x2 && y >= area.y1 && y <= area.y2;
}
#endif

#if LV_USE_DROPDOWN != 0
void realign_dropdown_list_async(void* user_data) {
  _lv_obj_t* const dd = static_cast<_lv_obj_t*>(user_data);
  if (!dd || !lv_dropdown_is_open(dd)) return;
  _lv_obj_t* const list = lv_dropdown_get_list(dd);
  if (!list) return;

  lv_obj_update_layout(dd);
  const lv_coord_t dropdown_w = lv_obj_get_width(dd);
  if (dropdown_w > 0) lv_obj_set_width(list, dropdown_w);
  lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_OFF);
  ui_theme_apply_dropdown_list(list);
  ui_theme_match_dropdown_list_padding(dd, list);
#if defined(HELTEC_V4_R8_TFT)
  lv_obj_set_style_pad_top(list, kSystemDropdownListPadVer, LV_PART_MAIN);
  lv_obj_set_style_pad_bottom(list, kSystemDropdownListPadVer, LV_PART_MAIN);
  lv_obj_set_style_pad_top(list, kSystemDropdownListPadVer, LV_PART_SELECTED);
  lv_obj_set_style_pad_bottom(list, kSystemDropdownListPadVer, LV_PART_SELECTED);
#endif
  lv_obj_update_layout(list);

  const lv_dir_t dir = lv_dropdown_get_dir(dd);
  if (dir == LV_DIR_BOTTOM) {
    lv_obj_align_to(list, dd, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
  } else if (dir == LV_DIR_TOP) {
    lv_obj_align_to(list, dd, LV_ALIGN_OUT_TOP_LEFT, 0, 0);
  } else if (dir == LV_DIR_LEFT) {
    lv_obj_align_to(list, dd, LV_ALIGN_OUT_LEFT_TOP, 0, 0);
  } else if (dir == LV_DIR_RIGHT) {
    lv_obj_align_to(list, dd, LV_ALIGN_OUT_RIGHT_TOP, 0, 0);
  }
}

bool is_dropdown_next_key(uint32_t key) {
  return key == LV_KEY_DOWN || key == LV_KEY_RIGHT || key == LV_KEY_NEXT;
}

bool is_dropdown_prev_key(uint32_t key) {
  return key == LV_KEY_UP || key == LV_KEY_LEFT || key == LV_KEY_PREV;
}

bool cycle_open_dropdown(_lv_obj_t* dd, uint32_t key) {
  if (!dd || !lv_obj_check_type(dd, &lv_dropdown_class) || !lv_dropdown_is_open(dd)) return false;
  const uint16_t count = lv_dropdown_get_option_cnt(dd);
  if (count <= 1) return false;

  const uint16_t current = lv_dropdown_get_selected(dd);
  uint16_t next = current;
  if (is_dropdown_next_key(key)) {
    next = (uint16_t)((current + 1u) % count);
  } else if (is_dropdown_prev_key(key)) {
    next = current == 0 ? (uint16_t)(count - 1u) : (uint16_t)(current - 1u);
  } else {
    return false;
  }

  lv_dropdown_set_selected(dd, next);
  _lv_obj_t* const list = lv_dropdown_get_list(dd);
  if (list) lv_obj_invalidate(list);
  lv_async_call(realign_dropdown_list_async, dd);
  return true;
}
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

int friend_dropdown_index(const biz::IBizFacade& app, const int* mesh_map, int mesh_map_count) {
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

void clear_group_widget_states(_lv_obj_t* obj, lv_group_t* g) {
  if (!obj) return;
  if (lv_obj_get_group(obj) == g) {
    lv_obj_clear_state(obj, LV_STATE_FOCUSED);
    lv_obj_clear_state(obj, LV_STATE_FOCUS_KEY);
    lv_obj_invalidate(obj);
  }
  const uint32_t n = lv_obj_get_child_cnt(obj);
  for (uint32_t i = 0; i < n; ++i) {
    clear_group_widget_states(lv_obj_get_child(obj, i), g);
  }
}

_lv_obj_t* focus_row_for(_lv_obj_t* obj) {
#if LV_USE_SWITCH != 0
  if (obj && lv_obj_check_type(obj, &lv_switch_class)) return lv_obj_get_parent(obj);
#endif
#if LV_USE_DROPDOWN != 0
  if (obj && lv_obj_check_type(obj, &lv_dropdown_class)) return lv_obj_get_parent(obj);
#endif
#if LV_USE_SLIDER != 0
  if (obj && lv_obj_check_type(obj, &lv_slider_class)) {
    _lv_obj_t* controls = lv_obj_get_parent(obj);
    return controls ? lv_obj_get_parent(controls) : obj;
  }
#endif
  return obj;
}

void set_row_hidden(_lv_obj_t* row, bool hidden) {
  if (!row) return;
  if (hidden) lv_obj_add_flag(row, LV_OBJ_FLAG_HIDDEN);
  else lv_obj_clear_flag(row, LV_OBJ_FLAG_HIDDEN);
}

}  // namespace

SystemScreen::SysAction SystemScreen::actionForRow(_lv_obj_t* obj) const {
  if (!obj) return SysAction::None;
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  if (obj == _row_wp_gps) return SysAction::WpGps;
  if (obj == _row_wp_manual) return SysAction::WpManual;
#endif
  if (obj == _row_factory_reset) return SysAction::FactoryReset;
  if (obj == _row_clear_data) return SysAction::ClearData;
  return SysAction::None;
}

void SystemScreen::addKeypadWidget(_lv_obj_t* obj) {
  if (!obj || !group()) return;
  lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
  if (!keypad_suppresses_bubble(obj)) lv_obj_add_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE);
  lv_group_add_obj(group(), obj);
}

_lv_obj_t* SystemScreen::addActionRow(_lv_obj_t* scroll, const char* title, _lv_obj_t** out_row) {
  _lv_obj_t* row = ht_obj_create(scroll, meta_id::SystemActionRow);
  if (!row) return nullptr;
  configure_system_row(row, LV_FLEX_FLOW_ROW, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
  _lv_obj_t* label = ht_label_create(row, meta_id::SystemActionLabel);
  configure_system_label(label, lv_pct(100));
  if (label) lv_label_set_text_static(label, title ? title : "");
  lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
  bindWidget(row);
  lv_obj_add_event_cb(row, onRowFocus, LV_EVENT_FOCUSED, this);
  lv_obj_add_event_cb(row, onRowFocus, LV_EVENT_DEFOCUSED, this);
  lv_obj_add_event_cb(row, onActionRowEvent, LV_EVENT_CLICKED, this);
  if (out_row) *out_row = row;
  return row;
}

_lv_obj_t* SystemScreen::addSwitchRow(_lv_obj_t* scroll, const char* title, _lv_obj_t** out_sw) {
  _lv_obj_t* row = ht_obj_create(scroll, meta_id::SystemSwitchRow);
  if (!row) return nullptr;
  configure_system_row(row, LV_FLEX_FLOW_ROW, LV_FLEX_ALIGN_SPACE_BETWEEN,
                       LV_FLEX_ALIGN_CENTER);
  _lv_obj_t* label = ht_label_create(row, meta_id::SystemSwitchLabel);
  configure_system_label(label, LV_SIZE_CONTENT);
  if (label) lv_label_set_text_static(label, title ? title : "");
  *out_sw = ht_switch_create(row, meta_id::SystemSwitch);
  if (!*out_sw) return row;
  lv_obj_clear_flag(*out_sw, LV_OBJ_FLAG_SCROLLABLE);
  setup_focus_row(row, *out_sw);
  bindWidget(*out_sw);
  lv_obj_add_event_cb(*out_sw, onSwitchValueChanged, LV_EVENT_VALUE_CHANGED, this);
  return row;
}

_lv_obj_t* SystemScreen::addDropdownRow(_lv_obj_t* scroll, ChoiceRow& choice, const char* title) {
  _lv_obj_t* row = ht_obj_create(scroll, meta_id::SystemDropdownRow);
  if (!row) return nullptr;
  configure_system_row(row, LV_FLEX_FLOW_ROW, LV_FLEX_ALIGN_START,
                       LV_FLEX_ALIGN_CENTER);
  _lv_obj_t* label = ht_label_create(row, meta_id::SystemDropdownLabel);
  configure_system_label(label, LV_SIZE_CONTENT);
  choice.row = row;
  choice.label = label;
  choice.dropdown = nullptr;
  choice.title = title;
  if (label) {
    lv_label_set_text_static(label, title ? title : "");
  }

  _lv_obj_t* dd = ht_dropdown_create(row, meta_id::SystemDropdown);
  choice.dropdown = dd;
  if (!dd) return row;
  // Flex owns the horizontal layout. A zero base width ensures the dropdown
  // does not request space from its option text; it simply fills the row area
  // left after the label.
  lv_obj_set_width(dd, 0);
  lv_obj_set_flex_grow(dd, 1);
#if LV_COLOR_DEPTH == 1
  lv_obj_set_height(dd, 12);
#else
#if defined(HELTEC_V4_R8_TFT)
  lv_obj_set_height(dd, kSystemDropdownHeight);
  ui_theme_center_dropdown_value(dd);
#else
  lv_obj_set_height(dd, LV_SIZE_CONTENT);
#endif
#endif
  lv_obj_clear_flag(dd, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scrollbar_mode(dd, LV_SCROLLBAR_MODE_OFF);
  lv_dropdown_set_dir(dd, LV_DIR_BOTTOM);
  lv_dropdown_set_selected_highlight(dd, true);

  bindWidget(dd);
  lv_obj_add_event_cb(dd, onRowFocus, LV_EVENT_FOCUSED, this);
  lv_obj_add_event_cb(dd, onRowFocus, LV_EVENT_DEFOCUSED, this);
  setup_focus_row(row, dd);
  lv_obj_add_event_cb(dd, onDropdownValueChanged, LV_EVENT_VALUE_CHANGED, this);
  lv_obj_add_event_cb(dd, onDropdownStateEvent, LV_EVENT_READY, this);
  lv_obj_add_event_cb(dd, onDropdownStateEvent, LV_EVENT_CANCEL, this);
  return dd;
}

#if defined(HAS_BUZZER_VOLUME_CONTROL) && HAS_BUZZER_VOLUME_CONTROL
_lv_obj_t* SystemScreen::addBuzzerVolumeRow(_lv_obj_t* scroll) {
  _lv_obj_t* row = ht_obj_create(scroll, meta_id::SystemVolumeRow);
  if (!row) return nullptr;
  configure_system_row(row, LV_FLEX_FLOW_COLUMN, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

  _lv_obj_t* title = ht_label_create(row, meta_id::SystemVolumeLabel, "Volume");
  configure_system_label(title, lv_pct(100));

  _lv_obj_t* controls = ht_obj_create(row, meta_id::SystemVolumeControls);
  if (!controls) return row;
  lv_obj_set_size(controls, lv_pct(100), 26);
  lv_obj_set_flex_flow(controls, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(controls, LV_FLEX_ALIGN_SPACE_BETWEEN,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_all(controls, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_column(controls, 4, LV_PART_MAIN);
  lv_obj_clear_flag(controls, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

  const auto add_button = [this, controls](const char* text, _lv_obj_t** out) {
    _lv_obj_t* btn = ht_btn_create(controls, meta_id::SystemVolumeButton);
    if (!btn) return;
    lv_obj_set_size(btn, 26, 24);
    lv_obj_set_style_pad_all(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(btn, ui_color_panel_border(), LV_PART_MAIN);
    lv_obj_set_style_border_opa(btn, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_text_color(btn, ui_color_fg(), LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn, ui_color_highlight_bg(),
                              LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_text_color(btn, ui_color_highlight_fg(),
                                LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_border_width(btn, 1, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_border_color(btn, lv_color_white(),
                                  LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_border_opa(btn, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_text_color(btn, ui_color_fg(), LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_width(btn, 1, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_color(btn, lv_color_white(),
                                  LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_opa(btn, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_set_style_text_color(btn, ui_color_fg(), LV_PART_MAIN | LV_STATE_FOCUS_KEY);
#if defined(HELTEC_V4_R8_TFT)
    // Pointer focus and modal focus restoration must not reposition the
    // System list. Keypad navigation is scrolled explicitly by applyGroupFocus().
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_set_ext_click_area(btn, 5);
#endif
    _lv_obj_t* label = ht_label_create(btn, meta_id::SystemVolumeButtonLabel, text);
    if (label) lv_obj_center(label);
    bindWidget(btn);
    lv_obj_add_event_cb(btn, onBuzzerVolumeButtonClicked, LV_EVENT_CLICKED, this);
    *out = btn;
  };

  add_button("-", &_btnBuzzerVolumeDown);

#if LV_USE_SLIDER != 0
  _sliderBuzzerVolume = lv_slider_create(controls);
  if (_sliderBuzzerVolume) {
    ht_set_meta_id(_sliderBuzzerVolume, meta_id::SystemVolumeSlider);
    lv_obj_set_size(_sliderBuzzerVolume, 60, 10);
    lv_obj_set_flex_grow(_sliderBuzzerVolume, 1);
    lv_slider_set_range(_sliderBuzzerVolume, 0, 3);
    lv_obj_clear_flag(_sliderBuzzerVolume,
                      LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CLICK_FOCUSABLE);
    lv_obj_set_style_bg_color(_sliderBuzzerVolume, ui_color_switch_bg(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(_sliderBuzzerVolume, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(_sliderBuzzerVolume, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(_sliderBuzzerVolume, ui_color_panel_border(), LV_PART_MAIN);
    lv_obj_set_style_radius(_sliderBuzzerVolume, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(_sliderBuzzerVolume, ui_color_accent(), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(_sliderBuzzerVolume, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(_sliderBuzzerVolume, LV_RADIUS_CIRCLE, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(_sliderBuzzerVolume, ui_color_fg(), LV_PART_KNOB);
    lv_obj_set_style_bg_opa(_sliderBuzzerVolume, LV_OPA_COVER, LV_PART_KNOB);
    lv_obj_set_style_border_width(_sliderBuzzerVolume, 1, LV_PART_KNOB);
    lv_obj_set_style_border_color(_sliderBuzzerVolume, ui_color_panel_border(), LV_PART_KNOB);
    lv_obj_set_style_pad_left(_sliderBuzzerVolume, 0, LV_PART_KNOB);
    lv_obj_set_style_pad_right(_sliderBuzzerVolume, 0, LV_PART_KNOB);
    lv_obj_set_style_pad_top(_sliderBuzzerVolume, 3, LV_PART_KNOB);
    lv_obj_set_style_pad_bottom(_sliderBuzzerVolume, 3, LV_PART_KNOB);
    lv_obj_set_style_transform_width(_sliderBuzzerVolume, -1, LV_PART_KNOB);
  }
#endif

  add_button("+", &_btnBuzzerVolumeUp);
  return row;
}
#endif

void SystemScreen::bindWidget(_lv_obj_t* obj) {
  if (!obj) return;
  if (keypad_suppresses_bubble(obj)) lv_obj_clear_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE);
  lv_obj_add_event_cb(obj, onWidgetKeyPreprocess,
                      static_cast<lv_event_code_t>(LV_EVENT_KEY | LV_EVENT_PREPROCESS), this);
#if LV_USE_DROPDOWN != 0
  if (lv_obj_check_type(obj, &lv_dropdown_class)) {
    lv_obj_add_event_cb(obj, onDropdownReleasedPre,
                        static_cast<lv_event_code_t>(LV_EVENT_RELEASED | LV_EVENT_PREPROCESS), this);
  }
#endif
}

void SystemScreen::scrollFocusedIntoView(_lv_obj_t* focused) const {
  if (!_root || !focused) return;

  _lv_obj_t* row = focused;
  while (row && lv_obj_get_parent(row) != _root) {
    row = lv_obj_get_parent(row);
  }
  if (!row || row == _root) return;

  // Keep the normal top-to-bottom focus motion visible. Forcing every newly
  // focused row to the top makes the whole list jump upward on LV_KEY_NEXT,
  // which looks like reversed key navigation.
  lv_obj_scroll_to_view(row, LV_ANIM_OFF);
}

void SystemScreen::closeOpenDropdowns() {
  if (!_open_dropdown) return;
  _lv_obj_t* const closing = _open_dropdown;
  _open_dropdown = nullptr;
#if LV_USE_DROPDOWN != 0
  if (lv_obj_is_valid(closing) && lv_obj_check_type(closing, &lv_dropdown_class) &&
      lv_dropdown_is_open(closing)) {
    lv_dropdown_set_selected(closing, _open_dropdown_original_index);
    lv_dropdown_close(closing);
  }
#endif
  if (lv_obj_is_valid(closing)) lv_obj_clear_state(closing, LV_STATE_EDITED);
  if (group()) lv_group_set_editing(group(), false);
}

void SystemScreen::applyGroupFocus(_lv_obj_t* focused) {
  if (!focused) return;

  if (!isDropdownRow(focused)) closeOpenDropdowns();

#if defined(HELTEC_V4_R8_TFT) && defined(HAS_BUZZER_VOLUME_CONTROL) && \
    HAS_BUZZER_VOLUME_CONTROL
  if (focused == _btnBuzzerVolumeDown || focused == _btnBuzzerVolumeUp) {
    // A touch click, and the later focus restore after the feedback Alert,
    // must keep the user's current scroll position. Physical keypad focus
    // still brings the row into view.
    lv_indev_t* const indev = lv_indev_get_act();
    if (indev) {
      const lv_indev_type_t type = lv_indev_get_type(indev);
      if (type == LV_INDEV_TYPE_KEYPAD || type == LV_INDEV_TYPE_ENCODER) {
        scrollFocusedIntoView(focused);
      }
    }
    clearFocusRowHighlight();
    return;
  }
#endif

  scrollFocusedIntoView(focused);

#if LV_USE_SWITCH != 0
  if (lv_obj_check_type(focused, &lv_switch_class)) {
    highlightFocusRow(lv_obj_get_parent(focused));
    return;
  }
#endif
#if defined(HAS_BUZZER_VOLUME_CONTROL) && HAS_BUZZER_VOLUME_CONTROL
  if (focused == _btnBuzzerVolumeDown || focused == _btnBuzzerVolumeUp) {
    clearFocusRowHighlight();
    return;
  }
#endif
  if (isDropdownRow(focused)) {
    highlightFocusRow(focus_row_for(focused));
    return;
  }
  if (isActionRow(focused)) {
    highlightFocusRow(focused);
  } else {
    clearFocusRowHighlight();
  }
}

#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
bool SystemScreen::hitScrollableContent(lv_coord_t x, lv_coord_t y) const {
  return point_hits_obj(_root, x, y);
}
#endif

bool SystemScreen::focusKeypadWidget(_lv_obj_t* obj) {
  if (!obj || !group() || !lv_obj_is_valid(obj)) return false;
  if (lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN)) return false;

  if (lv_obj_get_group(obj) != group()) {
    rebuildKeypadGroup(_biz);
  }
  if (lv_obj_get_group(obj) != group()) return false;

  lv_obj_clear_state(obj, LV_STATE_PRESSED);
  lv_group_focus_obj(obj);
  applyGroupFocus(obj);
  return true;
}

void SystemScreen::clearGroupFocusVisual() {
  if (!group() || !_root) return;
  lv_group_t* const g = group();
  lv_obj_t* const foc = lv_group_get_focused(g);
  if (foc) lv_event_send(foc, LV_EVENT_DEFOCUSED, nullptr);
  clear_group_widget_states(_root, g);
  clearFocusRowHighlight();
}

void SystemScreen::syncDropdownLayout(_lv_obj_t* dd) const {
#if LV_USE_DROPDOWN != 0
  if (!dd || !_root) return;
  lv_obj_update_layout(_root);
  for (_lv_obj_t* p = dd; p; p = lv_obj_get_parent(p)) {
    lv_obj_update_layout(p);
  }
#else
  (void)dd;
#endif
}

void SystemScreen::handleAction(SysAction action) {
  if (action == SysAction::FactoryReset || action == SysAction::ClearData) {
    openActionConfirmation(action);
    return;
  }
  executeAction(action);
}

void SystemScreen::executeAction(SysAction action) {
  biz::IBizFacade& app = _biz;
  switch (action) {
    case SysAction::WpGps:
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
    {
      const biz::IBizFacade::GpsStatus gps = app.gpsStatus();
      if (gps.fix_valid && app.setFindFriendWaypoint(gps.lat_deg, gps.lon_deg)) {
        char buf[48];
        app.formatFindFriendWaypointInput(buf, sizeof(buf));
        _feedback.showAlert(buf[0] ? buf : "Saved", 3000);
      } else if (!gps.fix_valid) {
        _feedback.showAlert("Need GPS fix", 3000);
      }
    }
#endif
      break;
    case SysAction::WpManual:
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
      _feedback.requestWaypointManualInput();
#endif
      break;
    case SysAction::FactoryReset:
      if (app.factoryReset()) {
        _feedback.showAlert("Factory reset complete\nRestarting...", 1600);
      } else {
        _feedback.showAlert("Factory reset failed", 2000);
      }
      break;
    case SysAction::ClearData:
      if (app.clearUserData()) {
        syncControlsFromApp(app);
        _feedback.showAlert("Data cleared", 2000);
      } else {
        _feedback.showAlert("Clear failed", 2000);
      }
      break;
    default:
      break;
  }
}

void SystemScreen::openActionConfirmation(SysAction action) {
  if (action != SysAction::FactoryReset && action != SysAction::ClearData) return;
  closeActionConfirmation();

  const char* text = action == SysAction::FactoryReset
                         ? "Erase all settings and data?"
                         : "Clear contacts and user data?";

  _pending_action = action;
  _action_confirm_root = lv_obj_create(lv_layer_top());
  if (!_action_confirm_root) {
    _pending_action = SysAction::None;
    _feedback.showAlert("Unable to open confirmation", 2000);
    return;
  }

  lv_obj_remove_style_all(_action_confirm_root);
  lv_obj_set_size(_action_confirm_root, lv_pct(100), lv_pct(100));
  lv_obj_set_pos(_action_confirm_root, 0, 0);
  lv_obj_add_flag(_action_confirm_root, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(_action_confirm_root, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(_action_confirm_root, lv_color_black(), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(_action_confirm_root, LV_OPA_50, LV_PART_MAIN);
  lv_obj_move_foreground(_action_confirm_root);

  _action_confirm_box = lv_obj_create(_action_confirm_root);
  if (!_action_confirm_box) {
    closeActionConfirmation();
    _feedback.showAlert("Unable to open confirmation", 2000);
    return;
  }

  lv_disp_t* const display = lv_disp_get_default();
  const bool compact = display &&
                       (lv_disp_get_hor_res(display) <= 160 || lv_disp_get_ver_res(display) <= 128);
  const lv_coord_t dialog_pad = compact ? 4 : LV_DPX(12);
  const lv_coord_t dialog_gap = compact ? 4 : LV_DPX(12);

  lv_obj_set_size(_action_confirm_box, compact ? lv_pct(94) : lv_pct(86), LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(_action_confirm_box, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(_action_confirm_box, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_clear_flag(_action_confirm_box, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(_action_confirm_box, ui_color_overlay_bg(), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(_action_confirm_box, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_color(_action_confirm_box, ui_color_overlay_fg(), LV_PART_MAIN);
  lv_obj_set_style_border_width(_action_confirm_box, 1, LV_PART_MAIN);
  lv_obj_set_style_radius(_action_confirm_box, compact ? 2 : LV_DPX(8), LV_PART_MAIN);
  lv_obj_set_style_pad_all(_action_confirm_box, dialog_pad, LV_PART_MAIN);
  lv_obj_set_style_pad_row(_action_confirm_box, dialog_gap, LV_PART_MAIN);
  lv_obj_set_style_text_color(_action_confirm_box, ui_color_overlay_fg(), LV_PART_MAIN);

  lv_obj_t* const title = lv_label_create(_action_confirm_box);
  if (title) {
    lv_obj_set_width(title, lv_pct(100));
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, ui_color_overlay_fg(), LV_PART_MAIN);
    lv_label_set_text_static(title, "Confirm");
  }

  lv_obj_t* const body = lv_label_create(_action_confirm_box);
  if (body) {
    lv_obj_set_width(body, lv_pct(100));
    lv_obj_set_style_text_align(body, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_color(body, ui_color_overlay_fg(), LV_PART_MAIN);
    lv_label_set_long_mode(body, LV_LABEL_LONG_WRAP);
    lv_label_set_text(body, text);
  }

#if defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
  const lv_coord_t button_height = compact ? 20 : LV_DPX(42);
  lv_obj_t* const button_row = lv_obj_create(_action_confirm_box);
  if (!button_row) {
    closeActionConfirmation();
    _feedback.showAlert("Unable to open confirmation", 2000);
    return;
  }
  lv_obj_remove_style_all(button_row);
  lv_obj_set_size(button_row, lv_pct(100), button_height);
  lv_obj_set_flex_flow(button_row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(button_row, LV_FLEX_ALIGN_SPACE_BETWEEN,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(button_row, compact ? 4 : LV_DPX(10), LV_PART_MAIN);
  lv_obj_clear_flag(button_row, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

  const auto create_button = [this, button_row, compact](const char* label_text,
                                                          lv_obj_t** out_button) {
    lv_obj_t* const button = lv_btn_create(button_row);
    if (!button) return;
    lv_obj_set_height(button, lv_pct(100));
    lv_obj_set_width(button, lv_pct(46));
    lv_obj_set_style_bg_color(button, ui_color_panel_bg(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(button, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(button, ui_color_panel_border(), LV_PART_MAIN);
    lv_obj_set_style_border_width(button, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(button, compact ? 1 : LV_DPX(5), LV_PART_MAIN);
    lv_obj_set_style_pad_all(button, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(button, ui_color_highlight_bg(),
                              LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_text_color(button, ui_color_highlight_fg(),
                                LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_add_event_cb(button, onActionConfirmationEvent, LV_EVENT_CLICKED, this);
    lv_obj_t* const label = lv_label_create(button);
    if (label) {
      lv_label_set_text_static(label, label_text);
      lv_obj_set_style_text_color(label, ui_color_overlay_fg(), LV_PART_MAIN);
      lv_obj_center(label);
    }
    *out_button = button;
  };

  create_button("Cancel", &_action_confirm_cancel);
  create_button("Confirm", &_action_confirm_accept);
  if (!_action_confirm_cancel || !_action_confirm_accept) {
    closeActionConfirmation();
    _feedback.showAlert("Unable to open confirmation", 2000);
    return;
  }
#else
  lv_obj_t* const key_hint = lv_label_create(_action_confirm_box);
  if (key_hint) {
    lv_obj_set_width(key_hint, lv_pct(100));
    lv_label_set_long_mode(key_hint, LV_LABEL_LONG_WRAP);
    lv_label_set_text_static(key_hint, "ESC: Cancel\nENTER: Confirm");
    lv_obj_set_style_text_align(key_hint, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_color(key_hint, ui_color_overlay_fg(), LV_PART_MAIN);
  }
#endif

  lv_obj_update_layout(_action_confirm_root);
  lv_obj_center(_action_confirm_box);
  if (lv_disp_t* disp = lv_disp_get_default()) lv_refr_now(disp);
}

void SystemScreen::closeActionConfirmation() {
  _pending_action = SysAction::None;
  lv_obj_t* const root = _action_confirm_root;
  _action_confirm_root = nullptr;
  _action_confirm_box = nullptr;
  _action_confirm_cancel = nullptr;
  _action_confirm_accept = nullptr;
  if (root && lv_obj_is_valid(root)) {
#if defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
    // A touch callback can still be dispatching from a child of this root.
    // Hide it immediately, then defer destruction until the event unwinds.
    lv_obj_add_flag(root, LV_OBJ_FLAG_HIDDEN);
    lv_obj_del_async(root);
#else
    // Physical-key events originate from the underlying System row, so the
    // modal root can be removed synchronously before the result Alert opens.
    lv_obj_del(root);
#endif
  }
}

void SystemScreen::acceptActionConfirmation() {
  if (!_action_confirm_root || _pending_action == SysAction::None) return;
  lv_obj_t* const root = _action_confirm_root;
  _action_confirm_root = nullptr;
  _action_confirm_box = nullptr;
  _action_confirm_cancel = nullptr;
  _action_confirm_accept = nullptr;
  if (lv_obj_is_valid(root)) {
#if defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
    lv_obj_add_flag(root, LV_OBJ_FLAG_HIDDEN);
    lv_obj_del_async(root);
#else
    lv_obj_del(root);
#endif
  }

  if (LV_RES_OK != lv_async_call(executeConfirmedActionAsync, this)) {
    const SysAction action = _pending_action;
    _pending_action = SysAction::None;
    executeAction(action);
  }
}

bool SystemScreen::handleConfirmationKey(uint32_t key) {
  if (!_action_confirm_root) return false;
  if (key == LV_KEY_ESC) {
    closeActionConfirmation();
  } else if (key == LV_KEY_ENTER) {
    // LVGL keypad input emits CLICKED on the still-focused action row when
    // this ENTER is released. Suppress that companion click or it immediately
    // opens the confirmation dialog again over the result Alert.
    _suppress_action_click_until_ms = lv_tick_get() + 500;
    acceptActionConfirmation();
  }
  return true;
}

void SystemScreen::onActionConfirmationEvent(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  auto* self = static_cast<SystemScreen*>(lv_event_get_user_data(e));
  if (!self || !self->_action_confirm_root) return;

  lv_obj_t* const target = lv_event_get_target(e);
  if (target == self->_action_confirm_cancel) {
    self->closeActionConfirmation();
  } else if (target == self->_action_confirm_accept) {
    self->acceptActionConfirmation();
  }
  lv_event_stop_processing(e);
  lv_event_stop_bubbling(e);
}

void SystemScreen::executeConfirmedActionAsync(void* user_data) {
  auto* self = static_cast<SystemScreen*>(user_data);
  if (!self) return;
  const SysAction action = self->_pending_action;
  self->_pending_action = SysAction::None;
  if (action != SysAction::None) self->executeAction(action);
}

bool SystemScreen::onKey(uint32_t key) {
  if (handleConfirmationKey(key)) return true;
  return AbstractScreen::onKey(key);
}

void SystemScreen::onWidgetKeyPreprocess(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_KEY) return;
  auto* self = static_cast<SystemScreen*>(lv_event_get_user_data(e));
  if (!self) return;

  const uint32_t key = lv_event_get_key(e);
  _lv_obj_t* const target = lv_event_get_target(e);

  if (self->handleConfirmationKey(key)) {
    lv_event_stop_processing(e);
    lv_event_stop_bubbling(e);
    return;
  }

#if LV_USE_DROPDOWN != 0
  if (cycle_open_dropdown(target, key)) {
    lv_event_stop_processing(e);
    lv_event_stop_bubbling(e);
    return;
  }

  // cycle_open_dropdown() uses lv_dropdown_set_selected(), which also updates
  // LVGL's internal "original" selection. Without an explicit commit here,
  // LVGL therefore sees no change when ENTER closes the list and never emits
  // LV_EVENT_VALUE_CHANGED. Compare against the index captured when the list
  // opened, then emit the change after closing so settings are saved and the
  // normal confirmation alert is shown.
  if (key == LV_KEY_ENTER && self->_open_dropdown == target &&
      lv_dropdown_is_open(target)) {
    const bool changed =
        lv_dropdown_get_selected(target) != self->_open_dropdown_original_index;
    self->_open_dropdown = nullptr;
    lv_dropdown_close(target);
    lv_obj_clear_state(target, LV_STATE_EDITED);
    if (self->group()) lv_group_set_editing(self->group(), false);
    if (changed) lv_event_send(target, LV_EVENT_VALUE_CHANGED, nullptr);
    lv_event_stop_processing(e);
    lv_event_stop_bubbling(e);
    return;
  }
#endif

  if (self->isDropdownRow(target)) {
    if (key == LV_KEY_ESC && self->_open_dropdown == target) {
      self->closeOpenDropdowns();
      lv_event_stop_processing(e);
      lv_event_stop_bubbling(e);
      return;
    }
  }

  if (key != LV_KEY_ESC) return;
  if (self->_open_dropdown) {
    self->closeOpenDropdowns();
    lv_event_stop_processing(e);
    lv_event_stop_bubbling(e);
    return;
  }
  if (!keypad_suppresses_bubble(target)) return;

  lv_event_stop_processing(e);
  lv_event_stop_bubbling(e);
  _lv_obj_t* const root = self->root();
  if (!root) return;
  uint32_t esc = LV_KEY_ESC;
  lv_event_send(root, LV_EVENT_KEY, &esc);
}

void SystemScreen::onActionRowEvent(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  auto* self = static_cast<SystemScreen*>(lv_event_get_user_data(e));
  if (!self) return;

  const SysAction action = self->actionForRow(lv_event_get_target(e));
  if ((action == SysAction::FactoryReset || action == SysAction::ClearData) &&
      self->_suppress_action_click_until_ms != 0) {
    const uint32_t now = lv_tick_get();
    if ((int32_t)(now - self->_suppress_action_click_until_ms) < 0) {
      lv_event_stop_processing(e);
      lv_event_stop_bubbling(e);
      return;
    }
    self->_suppress_action_click_until_ms = 0;
  }

  if (action == SysAction::None) return;

  if (action == SysAction::WpManual) {
    self->_waypoint_keyboard_return_focus = lv_event_get_target(e);
  }
  self->handleAction(action);
  lv_event_stop_processing(e);
  lv_event_stop_bubbling(e);
}

void SystemScreen::clearFocusRowHighlight() {
  if (!_focus_row) return;
  lv_obj_clear_state(_focus_row, LV_STATE_FOCUS_KEY);
  lv_obj_invalidate(_focus_row);
  _focus_row = nullptr;
}

void SystemScreen::highlightFocusRow(_lv_obj_t* row) {
  if (!row) return;
  clearFocusRowHighlight();
  _focus_row = row;
  lv_obj_add_state(row, LV_STATE_FOCUS_KEY);
  lv_obj_invalidate(row);
}

bool SystemScreen::isActionRow(_lv_obj_t* obj) const {
  return actionForRow(obj) != SysAction::None;
}

void SystemScreen::onRowFocus(lv_event_t* e) {
  auto* self = static_cast<SystemScreen*>(lv_event_get_user_data(e));
  if (!self) return;

  _lv_obj_t* const row = focus_row_for(lv_event_get_target(e));
  const lv_event_code_t code = lv_event_get_code(e);

  if (code == LV_EVENT_FOCUSED) {
    self->highlightFocusRow(row);
  } else if (code == LV_EVENT_DEFOCUSED && self->_focus_row == row) {
    self->clearFocusRowHighlight();
  }
}

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
  lv_async_call(realign_dropdown_list_async, dd);
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
    lv_async_call(realign_dropdown_list_async, dd);
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

#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
void SystemScreen::syncFriendDropdownFromApp(const biz::IBizFacade& app, bool force) {
  if (!_dd_friend) return;

  _friend_mesh_map_count = app.buildFindFriendDropdownOptions(
      s_friend_dd_options, sizeof(s_friend_dd_options), _friend_mesh_map,
      (int)(sizeof(_friend_mesh_map) / sizeof(_friend_mesh_map[0])));

  const bool options_changed = force || _friend_mesh_map_count != _friend_mesh_map_count_applied ||
                               strcmp(_friend_dd_options_applied, s_friend_dd_options) != 0;

  if (options_changed) {
    strncpy(_friend_dd_options_applied, s_friend_dd_options, sizeof(_friend_dd_options_applied) - 1);
    _friend_dd_options_applied[sizeof(_friend_dd_options_applied) - 1] = '\0';
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

_lv_obj_t* SystemScreen::create(_lv_obj_t* parent) {
  if (!AbstractScreen::create(parent)) return nullptr;
  if (group()) lv_group_set_wrap(group(), true);
  lv_obj_add_flag(_root, LV_OBJ_FLAG_EVENT_BUBBLE | LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(_root, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(_root, LV_SCROLLBAR_MODE_OFF);

  _dd_region = addDropdownRow(_root, _choice_region, "Region");
  if (_dd_region) {
    radioParamPresetDropdownOptions(s_region_dd_options, sizeof(s_region_dd_options));
    setDropdownOptions(_dd_region, s_region_dd_options);
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

}  // namespace heltec::meshcore::ui
