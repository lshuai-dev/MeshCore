#include "system_screen.hpp"

#include "ui/app/ui_theme.hpp"
#include "ui/core/ht_meta_data.hpp"
#include "ui/core/ui_deferred_queue.hpp"
#include <Arduino.h>

namespace heltec::meshcore::ui {
namespace {
#if defined(HELTEC_V4_R8_TFT)
constexpr lv_coord_t kSystemDropdownHeight = 36;
constexpr lv_coord_t kSystemDropdownListPadVer = 2;
#endif
void setup_focus_row(_lv_obj_t* row, _lv_obj_t* control) { ui_theme_apply_switch_row_focus(row, control); }
void configure_system_row(_lv_obj_t* row, lv_flex_flow_t flow, lv_flex_align_t main, lv_flex_align_t cross) {
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
  lv_obj_clear_flag(label, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ON_FOCUS);
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
_lv_obj_t* focus_row_for(_lv_obj_t* obj) {
#if LV_USE_SWITCH != 0
  if (obj && lv_obj_check_type(obj, &lv_switch_class)) return lv_obj_get_parent(obj);
#endif
#if LV_USE_DROPDOWN != 0
  if (obj && lv_obj_check_type(obj, &lv_dropdown_class)) return lv_obj_get_parent(obj);
#endif
#if LV_USE_SLIDER != 0
  if (obj && lv_obj_check_type(obj, &lv_slider_class)) { _lv_obj_t* controls=lv_obj_get_parent(obj); return controls ? lv_obj_get_parent(controls) : obj; }
#endif
  return obj;
}
void clear_group_widget_states(_lv_obj_t* obj, lv_group_t* g) {
  if (!obj) return;
  if (lv_obj_get_group(obj) == g) { lv_obj_clear_state(obj, LV_STATE_FOCUSED); lv_obj_clear_state(obj, LV_STATE_FOCUS_KEY); lv_obj_invalidate(obj); }
  const uint32_t n=lv_obj_get_child_cnt(obj); for(uint32_t i=0;i<n;++i) clear_group_widget_states(lv_obj_get_child(obj,i),g);
}
#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
bool point_hits_obj(const _lv_obj_t* obj, lv_coord_t x, lv_coord_t y) {
  if (!obj || !lv_obj_is_valid(obj) || lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN)) return false;
  lv_area_t area; lv_obj_get_coords(const_cast<_lv_obj_t*>(obj), &area);
  return x >= area.x1 && x <= area.x2 && y >= area.y1 && y <= area.y2;
}
#endif
#if LV_USE_DROPDOWN != 0
void realign_dropdown_list_async(void* user_data) {
  _lv_obj_t* dd=static_cast<_lv_obj_t*>(user_data); if(!dd || !lv_dropdown_is_open(dd)) return;
  _lv_obj_t* list=lv_dropdown_get_list(dd); if(!list) return;
  lv_obj_update_layout(dd); const lv_coord_t w=lv_obj_get_width(dd); if(w>0) lv_obj_set_width(list,w);
  lv_obj_set_scrollbar_mode(list,LV_SCROLLBAR_MODE_OFF); ui_theme_apply_dropdown_list(list); ui_theme_match_dropdown_list_padding(dd,list);
#if defined(HELTEC_V4_R8_TFT)
  lv_obj_set_style_pad_top(list,kSystemDropdownListPadVer,LV_PART_MAIN); lv_obj_set_style_pad_bottom(list,kSystemDropdownListPadVer,LV_PART_MAIN);
  lv_obj_set_style_pad_top(list,kSystemDropdownListPadVer,LV_PART_SELECTED); lv_obj_set_style_pad_bottom(list,kSystemDropdownListPadVer,LV_PART_SELECTED);
#endif
  lv_obj_update_layout(list); const lv_dir_t dir=lv_dropdown_get_dir(dd);
  if(dir==LV_DIR_BOTTOM) lv_obj_align_to(list,dd,LV_ALIGN_OUT_BOTTOM_LEFT,0,0); else if(dir==LV_DIR_TOP) lv_obj_align_to(list,dd,LV_ALIGN_OUT_TOP_LEFT,0,0); else if(dir==LV_DIR_LEFT) lv_obj_align_to(list,dd,LV_ALIGN_OUT_LEFT_TOP,0,0); else if(dir==LV_DIR_RIGHT) lv_obj_align_to(list,dd,LV_ALIGN_OUT_RIGHT_TOP,0,0);
}
bool cycle_open_dropdown(_lv_obj_t* dd,uint32_t key) {
  if(!dd || !lv_obj_check_type(dd,&lv_dropdown_class) || !lv_dropdown_is_open(dd)) return false; const uint16_t count=lv_dropdown_get_option_cnt(dd); if(count<=1) return false;
  const uint16_t cur=lv_dropdown_get_selected(dd); uint16_t next=cur;
  if(key==LV_KEY_DOWN||key==LV_KEY_RIGHT||key==LV_KEY_NEXT) next=(uint16_t)((cur+1u)%count); else if(key==LV_KEY_UP||key==LV_KEY_LEFT||key==LV_KEY_PREV) next=cur==0?(uint16_t)(count-1u):(uint16_t)(cur-1u); else return false;
  lv_dropdown_set_selected(dd,next); _lv_obj_t* list=lv_dropdown_get_list(dd); if(list) lv_obj_invalidate(list); if(!ui_defer(realign_dropdown_list_async,dd)) realign_dropdown_list_async(dd); return true;
}
#endif
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

#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH

bool SystemScreen::hitScrollableContent(lv_coord_t x, lv_coord_t y) const {
  return point_hits_obj(_root, x, y);
}

#endif

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

}  // namespace heltec::meshcore::ui
