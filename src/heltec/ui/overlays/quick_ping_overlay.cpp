#include "quick_ping_overlay.hpp"

#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH

#include <cstring>

#include <lvgl.h>

#include "heltec/ui/core/biz_facade.hpp"
#include "ui/app/ui_theme.hpp"
#include "ui/core/ht_meta_data.hpp"
#include "ui/core/ui_deferred_queue.hpp"
#include "ui/core/ui_events.h"
#include "ui/core/ui_motion_scheduler.hpp"
#include "ui/theme/ui_theme_metrics.hpp"
#include "ui/theme/ui_widget_theme.hpp"

namespace heltec::meshcore::ui {
namespace {

constexpr const char* kTargetOptions = "group\nbroadcast\npersonal\nadvert";
constexpr const char* kMessageOptions = "hi\nbye\nyes\nno\nok";
constexpr const char* kMessagePresets[] = {
    "hi", "bye", "yes", "no", "ok",
};
constexpr lv_coord_t kPaneInset = 8;
constexpr lv_coord_t kPaneWidth = 224;
constexpr lv_coord_t kPaneHeight = 304;
constexpr lv_coord_t kTitleBarHeight = 39;
constexpr lv_coord_t kContentPadHorizontal = 6;
constexpr lv_coord_t kContentPadBottom = 7;
constexpr lv_coord_t kRowGap = 6;
constexpr lv_coord_t kDropdownRowHeight = 40;
constexpr lv_coord_t kDropdownRowRadius = 6;
constexpr lv_coord_t kDropdownControlX = 81;
constexpr lv_coord_t kDropdownControlY = 3;
constexpr lv_coord_t kDropdownControlWidth = 130;
constexpr lv_coord_t kDropdownControlHeight = 32;
constexpr lv_coord_t kRowLabelX = 12;
constexpr lv_coord_t kRowLabelWidth = 70;
constexpr lv_coord_t kMessageDropdownWidth = 26;

#ifndef QUICK_PING_SLIDE_ANIM_MS
#define QUICK_PING_SLIDE_ANIM_MS 220
#endif

static const char* kMessageCompactKbMap[] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "\n",
    "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "\n",
    "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "\n",
    "u", "v", "w", "x", "y", "z", ".", ",", "-", "/", "\n",
    "@", "#", "?", "!", "ABC", "SP", LV_SYMBOL_BACKSPACE, LV_SYMBOL_OK,
    "",
};

static const lv_btnmatrix_ctrl_t kMessageCompactKbCtrl[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 3, 3, LV_KEYBOARD_CTRL_BTN_FLAGS | 2,
    LV_KEYBOARD_CTRL_BTN_FLAGS | 2,
};

static const char* kMessageCompactUpperKbMap[] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "\n",
    "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "\n",
    "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "\n",
    "U", "V", "W", "X", "Y", "Z", ".", ",", "-", "/", "\n",
    "@", "#", "?", "!", "abc", "SP", LV_SYMBOL_BACKSPACE, LV_SYMBOL_OK,
    "",
};

static const lv_btnmatrix_ctrl_t kMessageCompactUpperKbCtrl[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 3, 3, LV_KEYBOARD_CTRL_BTN_FLAGS | 2,
    LV_KEYBOARD_CTRL_BTN_FLAGS | 2,
};

bool sameKey(const uint8_t lhs[6], const uint8_t rhs[6]) {
  return memcmp(lhs, rhs, 6) == 0;
}

bool focusable(_lv_obj_t* obj) {
  if (!obj || !lv_obj_is_valid(obj) || lv_obj_has_state(obj, LV_STATE_DISABLED)) return false;
  for (_lv_obj_t* p = obj; p; p = lv_obj_get_parent(p)) {
    if (lv_obj_has_flag(p, LV_OBJ_FLAG_HIDDEN)) return false;
  }
  return true;
}

size_t normalizeSingleLineText(const char* source, char* dest, size_t dest_size) {
  if (!dest || dest_size == 0) return 0;
  if (!source) {
    dest[0] = '\0';
    return 0;
  }

  size_t written = 0;
  bool pending_space = false;
  for (size_t i = 0; source[i] != '\0'; ++i) {
    const unsigned char c = static_cast<unsigned char>(source[i]);
    if (c <= 32 || c == 127) {
      if (written > 0) pending_space = true;
      continue;
    }
    if (pending_space) {
      if (written + 1 >= dest_size) break;
      dest[written++] = ' ';
      pending_space = false;
    }
    if (written + 1 >= dest_size) break;
    dest[written++] = static_cast<char>(c);
  }
  dest[written] = '\0';
  return written;
}

void sanitizeOptionLabel(char* label, size_t label_size) {
  if (!label || label_size == 0) return;
  if (normalizeSingleLineText(label, label, label_size) > 0) return;
  strncpy(label, "?", label_size - 1);
  label[label_size - 1] = '\0';
}

}  // namespace

_lv_obj_t* QuickPingOverlay::createRoot(_lv_obj_t* parent) {
  return ht_obj_create(parent, meta_id::QuickPingOverlayRoot);
}

_lv_obj_t* QuickPingOverlay::create(_lv_obj_t* parent) {
  if (_root) return _root;
  if (!AbstractOverlay::create(parent)) return nullptr;

  lv_obj_set_size(_root, kPaneWidth, kPaneHeight);
  lv_obj_align(_root, LV_ALIGN_TOP_LEFT, kPaneInset, kPaneInset);
  lv_obj_set_flex_flow(_root, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(_root, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_START);
  lv_obj_set_style_pad_all(_root, 0, LV_PART_MAIN);
  lv_obj_clear_flag(_root, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(_root, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(_root, onOutsideEvent, LV_EVENT_CLICKED, this);

  _title_bar = ht_obj_create(_root, meta_id::QuickPingTitleBar);
  if (!_title_bar) return nullptr;
  lv_obj_set_size(_title_bar, lv_pct(100), kTitleBarHeight);
  lv_obj_set_flex_flow(_title_bar, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(_title_bar, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_clear_flag(_title_bar, LV_OBJ_FLAG_SCROLLABLE);

  _title = ht_label_create(_title_bar, meta_id::QuickPingTitle, "Quick Ping");
  if (!_title) return nullptr;
  lv_obj_set_size(_title, lv_pct(100), LV_SIZE_CONTENT);
  lv_label_set_long_mode(_title, LV_LABEL_LONG_DOT);

  _content = ht_obj_create(_root, meta_id::QuickPingContent);
  if (!_content) return nullptr;
  lv_obj_set_width(_content, lv_pct(100));
  lv_obj_set_flex_grow(_content, 1);
  lv_obj_set_flex_flow(_content, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(_content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_START);
  lv_obj_set_style_pad_left(_content, kContentPadHorizontal, LV_PART_MAIN);
  lv_obj_set_style_pad_right(_content, kContentPadHorizontal, LV_PART_MAIN);
  lv_obj_set_style_pad_top(_content, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_bottom(_content, kContentPadBottom, LV_PART_MAIN);
  lv_obj_set_style_pad_row(_content, kRowGap, LV_PART_MAIN);
  lv_obj_clear_flag(_content, LV_OBJ_FLAG_SCROLLABLE);

  _row_target = createDropdownRow(_content, "Target :", &_dd_target);
  _row_recipient = createDropdownRow(_content, "to:", &_dd_recipient);
  _row_message = createMessageRow(_content);
  _keyboard = createKeyboard(_content);
  if (!_row_target || !_row_recipient || !_row_message || !_keyboard) {
    return nullptr;
  }
  return _root;
}

void QuickPingOverlay::onEnter() {
  AbstractOverlay::onEnter();
  _message_submit_handled = false;
  _close_animating = false;
  _close_animation_ready = false;
  closeDropdowns();
  syncLists();
  syncDropdownOptions();
  applyState(false);
  lv_obj_update_layout(_root);
  startOpenAnimation();
}

void QuickPingOverlay::onExit() {
  if (_root) {
    ui_motion_cancel(_root, slideYExec);
    lv_obj_set_y(_root, kPaneInset);
  }
  ui_defer_cancel(openPendingKeyboardAsync, this);
  clearRepeatSelection();
  _close_animating = false;
  _close_animation_ready = false;
  _keyboard_open_pending = false;
  _message_submit_handled = false;
  closeDropdowns();
  setKeyboardVisible(false);
  clearFocusObjects();
  AbstractOverlay::onExit();
}

void QuickPingOverlay::slideYExec(void* var, int32_t value) {
  auto* obj = static_cast<_lv_obj_t*>(var);
  if (obj && lv_obj_is_valid(obj)) lv_obj_set_y(obj, static_cast<lv_coord_t>(value));
}

void QuickPingOverlay::startOpenAnimation() {
  if (!_root || !lv_obj_is_valid(_root)) return;
  ui_motion_cancel(_root, slideYExec);
  lv_obj_update_layout(_root);
  lv_coord_t height = lv_obj_get_height(_root);
  if (height <= 0) {
    if (_lv_obj_t* parent = lv_obj_get_parent(_root)) height = lv_obj_get_height(parent);
  }
  if (height <= 0 || QUICK_PING_SLIDE_ANIM_MS == 0) {
    lv_obj_set_y(_root, kPaneInset);
    return;
  }

  UiMotionSpec motion;
  motion.target = _root;
  motion.exec = slideYExec;
  motion.start_value = -height;
  motion.end_value = kPaneInset;
  motion.duration_ms = QUICK_PING_SLIDE_ANIM_MS;
  motion.path = UiMotionPath::EaseOut;
  if (!ui_motion_start(motion)) lv_obj_set_y(_root, kPaneInset);
}

bool QuickPingOverlay::requestCloseAnimation() {
  if (!_root || !lv_obj_is_valid(_root)) return false;
  if (_close_animation_ready) {
    _close_animation_ready = false;
    return false;
  }
  if (_close_animating) return true;

  closeDropdowns();
  clearRepeatSelection();
  ui_motion_cancel(_root, slideYExec);
  lv_obj_update_layout(_root);
  lv_coord_t height = lv_obj_get_height(_root);
  if (height <= 0) {
    if (_lv_obj_t* parent = lv_obj_get_parent(_root)) height = lv_obj_get_height(parent);
  }
  if (height <= 0 || QUICK_PING_SLIDE_ANIM_MS == 0) return false;

  UiMotionSpec motion;
  motion.target = _root;
  motion.exec = slideYExec;
  motion.ready = closeAnimationReady;
  motion.ready_data = this;
  motion.start_value = lv_obj_get_y(_root);
  motion.end_value = -height;
  motion.duration_ms = QUICK_PING_SLIDE_ANIM_MS;
  motion.path = UiMotionPath::EaseIn;
  if (!ui_motion_start(motion)) return false;
  _close_animating = true;
  return true;
}

void QuickPingOverlay::closeAnimationReady(void* user_data) {
  auto* self = static_cast<QuickPingOverlay*>(user_data);
  if (!self) return;
  self->_close_animating = false;
  self->_close_animation_ready = true;
  (void)self->emitEvent(UiEventType::QuickPingClose);
}

bool QuickPingOverlay::onKey(uint32_t key) {
  if (key != LV_KEY_ESC) return false;
  clearRepeatSelection();
  if (closeDropdown(_dd_target)) return true;
  if (closeDropdown(_dd_recipient)) return true;
  if (closeDropdown(_dd_message)) return true;
  if (keyboardVisible()) {
    setKeyboardVisible(false);
    rebuildFocusGroup(_ta_message);
    return true;
  }
  // Keep the pane open after ESC/back once transient controls are gone.
  // QuickPingPane dismissal is intentionally owned by the upward swipe path.
  return true;
}

_lv_obj_t* QuickPingOverlay::focusedObject() const {
  if (_focus_group) {
    if (_lv_obj_t* focused = lv_group_get_focused(_focus_group)) {
      if (focusable(focused)) return focused;
    }
  }
  if (focusable(_dd_target)) return _dd_target;
  if (focusable(_dd_recipient)) return _dd_recipient;
  if (focusable(_ta_message)) return _ta_message;
  if (focusable(_dd_message)) return _dd_message;
  if (focusable(_keyboard)) return _keyboard;
  return _root;
}

_lv_obj_t* QuickPingOverlay::createDropdownRow(_lv_obj_t* parent, const char* label_text,
                                            _lv_obj_t** out_dropdown) {
  if (out_dropdown) *out_dropdown = nullptr;
  _lv_obj_t* row = ht_obj_create(parent, meta_id::QuickPingRow);
  if (!row) return nullptr;
  lv_obj_set_size(row, lv_pct(100), kDropdownRowHeight);
  lv_obj_set_layout(row, 0);
  lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN);
  lv_obj_set_style_radius(row, kDropdownRowRadius, LV_PART_MAIN);
  lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(row, LV_OBJ_FLAG_CLICK_FOCUSABLE);
  lv_obj_add_event_cb(row, onOutsideEvent, LV_EVENT_PRESSED, this);
  lv_obj_add_event_cb(row, onOutsideEvent, LV_EVENT_CLICKED, this);

  _lv_obj_t* label = ht_label_create(row, meta_id::QuickPingLabel, label_text ? label_text : "");
  if (label) {
    lv_obj_set_width(label, kRowLabelWidth);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, kRowLabelX, 0);
    lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
  }

  _lv_obj_t* dropdown = ht_dropdown_create(row, meta_id::QuickPingDropdown);
  if (!dropdown) return row;
  lv_obj_set_size(dropdown, kDropdownControlWidth, kDropdownControlHeight);
  lv_obj_align(dropdown, LV_ALIGN_TOP_LEFT, kDropdownControlX, kDropdownControlY);
  lv_obj_set_style_pad_right(dropdown, 8, LV_PART_MAIN);
  ui_theme_center_dropdown_value(dropdown);
  lv_dropdown_set_dir(dropdown, LV_DIR_BOTTOM);
  lv_dropdown_set_selected_highlight(dropdown, true);
  lv_obj_add_event_cb(dropdown, onDropdownConfirmPreprocess,
                      static_cast<lv_event_code_t>(LV_EVENT_KEY | LV_EVENT_PREPROCESS), this);
  lv_obj_add_event_cb(dropdown, onDropdownConfirmPreprocess,
                      static_cast<lv_event_code_t>(LV_EVENT_RELEASED | LV_EVENT_PREPROCESS), this);
  lv_obj_add_event_cb(dropdown, onDropdownEvent, LV_EVENT_ALL, this);
  if (out_dropdown) *out_dropdown = dropdown;
  return row;
}

_lv_obj_t* QuickPingOverlay::createMessageRow(_lv_obj_t* parent) {
  _lv_obj_t* row = ht_obj_create(parent, meta_id::QuickPingRow);
  if (!row) return nullptr;
  lv_obj_set_size(row, lv_pct(100), kDropdownRowHeight);
  lv_obj_set_layout(row, 0);
  lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN);
  lv_obj_set_style_radius(row, kDropdownRowRadius, LV_PART_MAIN);
  lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(row, LV_OBJ_FLAG_CLICK_FOCUSABLE);
  lv_obj_add_event_cb(row, onOutsideEvent, LV_EVENT_PRESSED, this);
  lv_obj_add_event_cb(row, onOutsideEvent, LV_EVENT_CLICKED, this);

  _lv_obj_t* label = ht_label_create(row, meta_id::QuickPingLabel, "message:");
  if (label) {
    lv_obj_set_width(label, kRowLabelWidth);
    lv_obj_align(label, LV_ALIGN_LEFT_MID, kRowLabelX, 0);
    lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
    lv_obj_add_flag(label, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(label, LV_OBJ_FLAG_CLICK_FOCUSABLE);
    lv_obj_add_event_cb(label, onOutsideEvent, LV_EVENT_PRESSED, this);
    lv_obj_add_event_cb(label, onOutsideEvent, LV_EVENT_CLICKED, this);
  }

  _ta_message = ht_textarea_create(row, meta_id::QuickPingMessageInput);
  if (_ta_message) {
    lv_obj_set_size(_ta_message, kDropdownControlWidth - kMessageDropdownWidth,
                    kDropdownControlHeight);
    lv_obj_align(_ta_message, LV_ALIGN_TOP_LEFT, kDropdownControlX,
                 kDropdownControlY);
    lv_textarea_set_one_line(_ta_message, true);
    lv_textarea_set_max_length(_ta_message, kMaxMessageLength);
    lv_textarea_set_text_buffer(_ta_message, _message_text, sizeof(_message_text));
    lv_textarea_set_cursor_click_pos(_ta_message, true);
    lv_textarea_set_placeholder_text(_ta_message, "");
    if (_lv_obj_t* text_label = lv_textarea_get_label(_ta_message)) {
      ht_set_meta_id(text_label, meta_id::QuickPingMessageInputLabel);
      ui_widget_theme_apply(text_label);
      lv_obj_set_width(text_label, lv_pct(100));
      lv_label_set_long_mode(text_label, LV_LABEL_LONG_CLIP);
      lv_obj_clear_flag(text_label, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CLICK_FOCUSABLE);
    }
    lv_obj_add_event_cb(_ta_message, onMessageInputEvent,
                        static_cast<lv_event_code_t>(LV_EVENT_KEY | LV_EVENT_PREPROCESS), this);
    lv_obj_add_event_cb(_ta_message, onMessageInputEvent, LV_EVENT_CLICKED, this);
  }

  _dd_message = ht_dropdown_create(row, meta_id::QuickPingMessageDropdown);
  if (_dd_message) {
    lv_obj_set_size(_dd_message, kMessageDropdownWidth, kDropdownControlHeight);
    lv_obj_align(_dd_message, LV_ALIGN_TOP_LEFT,
                 kDropdownControlX + kDropdownControlWidth - kMessageDropdownWidth,
                 kDropdownControlY);
    ui_theme_center_dropdown_value(_dd_message);
    lv_dropdown_set_options_static(_dd_message, kMessageOptions);
    lv_dropdown_set_text(_dd_message, LV_SYMBOL_DOWN);
    lv_dropdown_set_symbol(_dd_message, nullptr);
    lv_dropdown_set_dir(_dd_message, LV_DIR_BOTTOM);
    lv_dropdown_set_selected_highlight(_dd_message, true);
    lv_obj_add_event_cb(_dd_message, onDropdownConfirmPreprocess,
                        static_cast<lv_event_code_t>(LV_EVENT_KEY | LV_EVENT_PREPROCESS), this);
    lv_obj_add_event_cb(_dd_message, onDropdownConfirmPreprocess,
                        static_cast<lv_event_code_t>(LV_EVENT_RELEASED | LV_EVENT_PREPROCESS), this);
    lv_obj_add_event_cb(_dd_message, onDropdownEvent, LV_EVENT_ALL, this);
  }
  return row;
}

_lv_obj_t* QuickPingOverlay::createKeyboard(_lv_obj_t* parent) {
  _lv_obj_t* keyboard = ht_keyboard_create(parent, meta_id::QuickPingKeyboard);
  if (!keyboard) return nullptr;
  lv_obj_set_size(keyboard, lv_pct(100), 96);
  // Keep 96px as the compact baseline, then fill any space left below the message row.
  lv_obj_set_flex_grow(keyboard, 1);
  lv_keyboard_set_map(keyboard, LV_KEYBOARD_MODE_USER_2, kMessageCompactKbMap,
                      kMessageCompactKbCtrl);
  lv_keyboard_set_map(keyboard, LV_KEYBOARD_MODE_USER_3, kMessageCompactUpperKbMap,
                      kMessageCompactUpperKbCtrl);
  lv_keyboard_set_textarea(keyboard, _ta_message);
  lv_keyboard_set_popovers(keyboard, false);
  lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_USER_2);
  lv_obj_add_event_cb(keyboard, onKeyboardEvent, LV_EVENT_READY, this);
  lv_obj_add_event_cb(keyboard, onKeyboardEvent, LV_EVENT_CANCEL, this);
  lv_obj_add_event_cb(keyboard, onKeyboardValuePre,
                      static_cast<lv_event_code_t>(LV_EVENT_VALUE_CHANGED | LV_EVENT_PREPROCESS),
                      this);
  lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
  return keyboard;
}

void QuickPingOverlay::applyState(bool keep_focus) {
  syncRecipientDropdown();

  const TargetKind kind = targetKind();
  setRowVisible(_row_recipient, kind == TargetKind::Group || kind == TargetKind::Personal);
  const bool message_ready = currentTargetReady();
  if (!message_ready) setKeyboardVisible(false);
  setDropdownEnabled(_ta_message, message_ready);
  setDropdownEnabled(_dd_message, message_ready);

  _lv_obj_t* preferred = keep_focus && _focus_group
                             ? lv_group_get_focused(_focus_group)
                             : _dd_target;
  rebuildFocusGroup(preferred);
}

void QuickPingOverlay::rebuildFocusGroup(_lv_obj_t* preferred) {
  if (!_focus_group) return;
  lv_group_t* group = _focus_group;
  if (!preferred) preferred = lv_group_get_focused(group);

  _rebuilding_focus_group = true;
  clearFocusObjects();
  if (focusable(_dd_target)) addFocusObject(_dd_target);
  if (focusable(_dd_recipient)) addFocusObject(_dd_recipient);
  if (focusable(_ta_message)) addFocusObject(_ta_message);
  if (focusable(_dd_message)) addFocusObject(_dd_message);
  if (focusable(_keyboard)) addFocusObject(_keyboard);
  lv_group_set_editing(group, false);

  if (preferred && lv_obj_is_valid(preferred) && lv_obj_get_group(preferred) == group &&
      focusable(preferred)) {
    lv_group_focus_obj(preferred);
  } else if (focusable(_dd_target)) {
    lv_group_focus_obj(_dd_target);
  } else if (focusable(_dd_recipient)) {
    lv_group_focus_obj(_dd_recipient);
  } else if (focusable(_ta_message)) {
    lv_group_focus_obj(_ta_message);
  } else if (focusable(_dd_message)) {
    lv_group_focus_obj(_dd_message);
  } else if (focusable(_keyboard)) {
    lv_group_focus_obj(_keyboard);
  }
  _rebuilding_focus_group = false;
}

void QuickPingOverlay::syncLists() {
  const int old_channel = currentGroupChannel();
  uint8_t old_key[6] = {};
  const uint8_t* key = currentContactKey();
  const bool had_key = key != nullptr;
  if (had_key) memcpy(old_key, key, sizeof(old_key));

  _group_count = 0;
  const int group_total = _biz.sendMessageGroupCount();
  for (int i = 0; i < group_total && _group_count < kMaxGroups; ++i) {
    CachedGroup& g = _groups[_group_count];
    int channel_idx = -1;
    if (!_biz.sendMessageGroupAt(i, &channel_idx, g.label, sizeof(g.label))) continue;
    sanitizeOptionLabel(g.label, sizeof(g.label));
    g.channel_idx = channel_idx;
    ++_group_count;
  }

  _contact_count = 0;
  const int contact_total = _biz.sendMessagePersonalCount();
  for (int i = 0; i < contact_total && _contact_count < kMaxContacts; ++i) {
    CachedContact& c = _contacts[_contact_count];
    if (!_biz.sendMessagePersonalAt(i, c.pub_key_prefix, c.label, sizeof(c.label))) continue;
    sanitizeOptionLabel(c.label, sizeof(c.label));
    ++_contact_count;
  }

  int group_match = -1;
  if (old_channel >= 0) {
    for (int i = 0; i < _group_count; ++i) {
      if (_groups[i].channel_idx == old_channel) {
        group_match = i;
        break;
      }
    }
  }
  if (group_match >= 0) {
    _group_index = static_cast<uint8_t>(group_match);
  } else if (_group_count <= 0) {
    _group_index = 0;
  } else if (_group_index >= _group_count) {
    _group_index = static_cast<uint8_t>(_group_count - 1);
  }

  int contact_match = -1;
  if (had_key) {
    for (int i = 0; i < _contact_count; ++i) {
      if (sameKey(_contacts[i].pub_key_prefix, old_key)) {
        contact_match = i;
        break;
      }
    }
  }
  if (contact_match >= 0) {
    _contact_index = static_cast<uint8_t>(contact_match);
  } else if (_contact_count <= 0) {
    _contact_index = 0;
  } else if (_contact_index >= _contact_count) {
    _contact_index = static_cast<uint8_t>(_contact_count - 1);
  }
}

void QuickPingOverlay::syncDropdownOptions() {
  const bool was_syncing = _syncing_dropdowns;
  _syncing_dropdowns = true;
  if (_dd_target) {
    lv_dropdown_set_options_static(_dd_target, kTargetOptions);
    lv_dropdown_set_selected(_dd_target, dropdownIndexForTargetKind(targetKind()));
  }
  if (_dd_message) {
    lv_dropdown_set_options_static(_dd_message, kMessageOptions);
    lv_dropdown_set_selected(_dd_message, _message_index);
  }
  _syncing_dropdowns = was_syncing;
}

void QuickPingOverlay::syncRecipientDropdown() {
  if (!_dd_recipient) return;

  _recipient_options[0] = '\0';
  const TargetKind kind = targetKind();
  bool has_options = false;

  if (kind == TargetKind::Group) {
    if (_group_count <= 0) {
      appendOption(_recipient_options, sizeof(_recipient_options), "no groups");
    } else {
      for (int i = 0; i < _group_count; ++i) {
        appendOption(_recipient_options, sizeof(_recipient_options), _groups[i].label);
      }
      has_options = true;
    }
  } else if (kind == TargetKind::Personal) {
    if (_contact_count <= 0) {
      appendOption(_recipient_options, sizeof(_recipient_options), "no contacts");
    } else {
      for (int i = 0; i < _contact_count; ++i) {
        appendOption(_recipient_options, sizeof(_recipient_options), _contacts[i].label);
      }
      has_options = true;
    }
  } else {
    appendOption(_recipient_options, sizeof(_recipient_options), "-");
  }

  const bool was_syncing = _syncing_dropdowns;
  _syncing_dropdowns = true;
  lv_dropdown_set_options_static(_dd_recipient, _recipient_options);
  if (kind == TargetKind::Group) {
    lv_dropdown_set_selected(_dd_recipient, _group_count > 0 ? _group_index : 0);
  } else if (kind == TargetKind::Personal) {
    lv_dropdown_set_selected(_dd_recipient, _contact_count > 0 ? _contact_index : 0);
  } else {
    lv_dropdown_set_selected(_dd_recipient, 0);
  }
  setDropdownEnabled(_dd_recipient, has_options);
  _syncing_dropdowns = was_syncing;
}

void QuickPingOverlay::setRowVisible(_lv_obj_t* row, bool visible) {
  if (!row) return;
  if (visible) {
    lv_obj_clear_flag(row, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(row, LV_OBJ_FLAG_HIDDEN);
  }
}

void QuickPingOverlay::setDropdownEnabled(_lv_obj_t* dropdown, bool enabled) {
  if (!dropdown) return;
  if (enabled) {
    lv_obj_clear_state(dropdown, LV_STATE_DISABLED);
  } else {
    lv_obj_add_state(dropdown, LV_STATE_DISABLED);
  }
}

void QuickPingOverlay::closeDropdowns() {
  closeDropdown(_dd_target);
  closeDropdown(_dd_recipient);
  closeDropdown(_dd_message);
}

bool QuickPingOverlay::closeDropdown(_lv_obj_t* dropdown) {
  if (!dropdown || !lv_obj_is_valid(dropdown)) return false;
  if (!lv_dropdown_is_open(dropdown)) return false;
  if (_repeat_dropdown == dropdown) clearRepeatSelection();
  lv_dropdown_close(dropdown);
  return true;
}

void QuickPingOverlay::syncDropdownListLayout(_lv_obj_t* dropdown) {
  if (!dropdown) return;
  _lv_obj_t* const list = lv_dropdown_get_list(dropdown);
  if (!list) return;
  lv_coord_t list_w = lv_obj_get_width(dropdown);
  const lv_coord_t min_list_w = ui_quick_ping_metrics(_root).message_list_min_width;
  if (dropdown == _dd_message && list_w < min_list_w) {
    list_w = min_list_w;
  }
  lv_obj_set_width(list, list_w);
  lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_OFF);
  ui_theme_apply_dropdown_list(list);
  ui_theme_match_dropdown_list_padding(dropdown, list);
}

void QuickPingOverlay::setKeyboardVisible(bool visible) {
  if (!_keyboard || !lv_obj_is_valid(_keyboard)) return;
  const bool was_visible = keyboardVisible();
  if (visible) {
    lv_keyboard_set_textarea(_keyboard, _ta_message);
    lv_obj_clear_flag(_keyboard, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(_keyboard, LV_OBJ_FLAG_HIDDEN);
  }
  if (was_visible != visible && _root) lv_obj_update_layout(_root);
}

bool QuickPingOverlay::keyboardVisible() const {
  return _keyboard && lv_obj_is_valid(_keyboard) &&
         !lv_obj_has_flag(_keyboard, LV_OBJ_FLAG_HIDDEN);
}

bool QuickPingOverlay::hitVisibleKeyboard(int16_t x, int16_t y) const {
  if (!keyboardVisible()) return false;
  lv_point_t point;
  point.x = static_cast<lv_coord_t>(x);
  point.y = static_cast<lv_coord_t>(y);
  return lv_obj_hit_test(_keyboard, &point);
}

bool QuickPingOverlay::hitVerticalSwipeControl(int16_t x, int16_t y) const {
  lv_point_t point;
  point.x = static_cast<lv_coord_t>(x);
  point.y = static_cast<lv_coord_t>(y);
  if (hitVisibleKeyboard(x, y)) return true;

  _lv_obj_t* const protected_controls[] = {
      _dd_target, _dd_recipient, _dd_message, _ta_message,
  };
  for (_lv_obj_t* control : protected_controls) {
    if (control && lv_obj_is_valid(control) &&
        !lv_obj_has_flag(control, LV_OBJ_FLAG_HIDDEN) && lv_obj_hit_test(control, &point)) {
      return true;
    }
  }

  _lv_obj_t* const dropdowns[] = {_dd_target, _dd_recipient, _dd_message};
  for (_lv_obj_t* dropdown : dropdowns) {
    _lv_obj_t* list = dropdown ? lv_dropdown_get_list(dropdown) : nullptr;
    if (list && lv_obj_is_valid(list) && !lv_obj_has_flag(list, LV_OBJ_FLAG_HIDDEN) &&
        lv_obj_hit_test(list, &point)) {
      return true;
    }
  }
  return false;
}

bool QuickPingOverlay::hitSwipeDismissRegion(int16_t x, int16_t y) const {
  if (!_root || !lv_obj_is_valid(_root) || lv_obj_has_flag(_root, LV_OBJ_FLAG_HIDDEN)) {
    return false;
  }

  lv_point_t point;
  point.x = static_cast<lv_coord_t>(x);
  point.y = static_cast<lv_coord_t>(y);
  if (!lv_obj_hit_test(_root, &point) || hitVerticalSwipeControl(x, y)) return false;
  return true;
}

bool QuickPingOverlay::dismissTransientControls() {
  if (closeDropdown(_dd_target) || closeDropdown(_dd_recipient) || closeDropdown(_dd_message)) {
    return true;
  }
  if (!keyboardVisible()) return false;
  setKeyboardVisible(false);
  rebuildFocusGroup(_ta_message);
  return true;
}

bool QuickPingOverlay::targetInside(_lv_obj_t* target, _lv_obj_t* ancestor) const {
  if (!target || !ancestor || !lv_obj_is_valid(target) || !lv_obj_is_valid(ancestor)) return false;
  for (_lv_obj_t* obj = target; obj; obj = lv_obj_get_parent(obj)) {
    if (obj == ancestor) return true;
  }
  return false;
}

void QuickPingOverlay::closeKeyboardForOutsideTarget(_lv_obj_t* target) {
  if (!keyboardVisible()) return;
  if (targetInside(target, _keyboard) || targetInside(target, _ta_message)) return;
  setKeyboardVisible(false);
  rebuildFocusGroup(focusable(target) ? target : nullptr);
}

void QuickPingOverlay::handleDropdownRow(_lv_obj_t* target) {
  _lv_obj_t* dropdown = nullptr;
  if (targetInside(target, _row_target)) {
    dropdown = _dd_target;
  } else if (targetInside(target, _row_recipient)) {
    dropdown = _dd_recipient;
  }
  if (!focusable(dropdown)) return;

  closeKeyboardForOutsideTarget(dropdown);
  rebuildFocusGroup(dropdown);
  if (!lv_dropdown_is_open(dropdown)) lv_dropdown_open(dropdown);
}

void QuickPingOverlay::handleTargetChanged() {
  if (!_dd_target) return;
  _target_index = static_cast<uint8_t>(lv_dropdown_get_selected(_dd_target));
  const TargetKind kind = targetKind();

  if (kind == TargetKind::Advert) {
    _biz.sendAdvertWithFeedback();
  } else if (kind == TargetKind::Group && _group_count <= 0) {
    _biz.showAlert("No group channels", 900);
  } else if (kind == TargetKind::Personal && _contact_count <= 0) {
    _biz.showAlert("No contacts", 900);
  }

  applyState(true);
}

void QuickPingOverlay::handleRecipientChanged() {
  if (!_dd_recipient) return;
  const uint16_t selected = lv_dropdown_get_selected(_dd_recipient);
  if (targetKind() == TargetKind::Group) {
    _group_index = static_cast<uint8_t>(
        _group_count > 0 && selected < _group_count ? selected : 0);
  } else if (targetKind() == TargetKind::Personal) {
    _contact_index = static_cast<uint8_t>(
        _contact_count > 0 && selected < _contact_count ? selected : 0);
  }
  applyState(true);
}

void QuickPingOverlay::handleMessageSelectionChanged() {
  if (!_dd_message) return;
  closeDropdown(_dd_message);
  _message_index = static_cast<uint8_t>(lv_dropdown_get_selected(_dd_message));

  if (_message_index < kMessagePresetCount) {
    if (!currentTargetReady()) {
      showMissingTargetAlert();
      applyState(true);
      return;
    }
    if (_ta_message) lv_textarea_set_text(_ta_message, kMessagePresets[_message_index]);
    sendMessageText(kMessagePresets[_message_index]);
  }
  applyState(true);
}

void QuickPingOverlay::handleMessageInput() {
  if (!currentTargetReady()) {
    showMissingTargetAlert();
    return;
  }
  if (keyboardVisible() || _keyboard_open_pending) return;
  _keyboard_open_pending = true;
  if (!ui_defer(openPendingKeyboardAsync, this)) _keyboard_open_pending = false;
}

void QuickPingOverlay::openPendingKeyboard() {
  if (!_keyboard_open_pending) return;
  _keyboard_open_pending = false;
  if (!focusable(_ta_message) || !currentTargetReady()) return;
  // A newly opened keyboard starts a new submission cycle.
  _message_submit_handled = false;
  setKeyboardVisible(true);
  rebuildFocusGroup(_keyboard);
}

void QuickPingOverlay::submitMessageFromTextarea() {
  if (_message_submit_handled) return;
  if (!_ta_message) {
    setKeyboardVisible(false);
    rebuildFocusGroup(nullptr);
    return;
  }
  if (!currentTargetReady()) {
    _message_submit_handled = true;
    showMissingTargetAlert();
    setKeyboardVisible(false);
    rebuildFocusGroup(_ta_message);
    return;
  }
  const char* text = lv_textarea_get_text(_ta_message);
  // Latch before calling into the business layer. This protects against a
  // nested READY event generated while the send result is being displayed.
  _message_submit_handled = true;
  if (!sendMessageText(text)) {
    // Keep the keyboard usable after a validation or transport failure.
    _message_submit_handled = false;
    return;
  }
  setKeyboardVisible(false);
  rebuildFocusGroup(_ta_message);
}

bool QuickPingOverlay::sendMessageText(const char* text) {
  char normalized[kMaxMessageLength + 1] = {};
  if (normalizeSingleLineText(text, normalized, sizeof(normalized)) == 0) {
    _biz.showAlert("Enter message", 900);
    return false;
  }

  const TargetKind kind = targetKind();
  if (kind == TargetKind::Personal) {
    const uint8_t* key = currentContactKey();
    if (!key) {
      _biz.showAlert("No contacts", 900);
      return false;
    }
    return _biz.sendDirectMessage(key, normalized);
  } else if (kind == TargetKind::Group) {
    const int channel = currentGroupChannel();
    if (channel < 0) {
      _biz.showAlert("No group channels", 900);
      return false;
    }
    return _biz.sendGroupMessage(channel, normalized);
  } else if (kind == TargetKind::Broadcast) {
    const int len = static_cast<int>(strlen(normalized));
    const bool ok = _biz.sendBroadcast(normalized, len);
    _biz.showAlert(ok ? "Queued" : "Failed", 900);
    return ok;
  }
  showMissingTargetAlert();
  return false;
}

void QuickPingOverlay::activateDropdown(_lv_obj_t* dropdown) {
  if (!dropdown || _syncing_dropdowns) return;
  if (dropdown == _dd_target) {
    handleTargetChanged();
  } else if (dropdown == _dd_recipient) {
    handleRecipientChanged();
  } else if (dropdown == _dd_message) {
    handleMessageSelectionChanged();
  }
}

void QuickPingOverlay::clearRepeatSelection() {
  ui_defer_cancel(finishRepeatSelectionAsync, this);
  _repeat_pending = false;
  _repeat_index = 0;
  _repeat_dropdown = nullptr;
}

void QuickPingOverlay::armRepeatSelection(_lv_obj_t* dropdown) {
  if (!dropdown || !lv_obj_is_valid(dropdown) || !lv_dropdown_is_open(dropdown)) return;
  clearRepeatSelection();
  _repeat_pending = true;
  _repeat_index = lv_dropdown_get_selected(dropdown);
  _repeat_dropdown = dropdown;
  if (!ui_defer(finishRepeatSelectionAsync, this)) clearRepeatSelection();
}

void QuickPingOverlay::finishRepeatSelection() {
  if (!_repeat_pending) return;
  _lv_obj_t* dropdown = _repeat_dropdown;
  const uint16_t index = _repeat_index;
  _repeat_pending = false;
  _repeat_index = 0;
  _repeat_dropdown = nullptr;

  if (!dropdown || !lv_obj_is_valid(dropdown) || lv_dropdown_is_open(dropdown)) return;
  if (lv_dropdown_get_selected(dropdown) != index) return;
  activateDropdown(dropdown);
}

bool QuickPingOverlay::currentTargetReady() const {
  switch (targetKind()) {
    case TargetKind::Broadcast:
      return true;
    case TargetKind::Group:
      return _group_count > 0 && _group_index < _group_count;
    case TargetKind::Personal:
      return _contact_count > 0 && _contact_index < _contact_count;
    case TargetKind::Advert:
      return false;
  }
  return false;
}

QuickPingOverlay::TargetKind QuickPingOverlay::targetKind() const {
  return targetKindFromDropdownIndex(_target_index);
}

QuickPingOverlay::TargetKind QuickPingOverlay::targetKindFromDropdownIndex(uint16_t index) {
  switch (index) {
    case 0:
      return TargetKind::Group;
    case 1:
      return TargetKind::Broadcast;
    case 2:
      return TargetKind::Personal;
    case 3:
      return TargetKind::Advert;
    default:
      return TargetKind::Broadcast;
  }
}

uint16_t QuickPingOverlay::dropdownIndexForTargetKind(TargetKind kind) {
  return static_cast<uint16_t>(kind);
}

int QuickPingOverlay::currentGroupChannel() const {
  if (_group_count <= 0 || _group_index >= _group_count) return -1;
  return _groups[_group_index].channel_idx;
}

const uint8_t* QuickPingOverlay::currentContactKey() const {
  if (_contact_count <= 0 || _contact_index >= _contact_count) return nullptr;
  return _contacts[_contact_index].pub_key_prefix;
}

void QuickPingOverlay::showMissingTargetAlert() const {
  switch (targetKind()) {
    case TargetKind::Group:
      _biz.showAlert("No group channels", 900);
      break;
    case TargetKind::Personal:
      _biz.showAlert("No contacts", 900);
      break;
    case TargetKind::Advert:
      _biz.showAlert("Select message target", 900);
      break;
    case TargetKind::Broadcast:
      _biz.showAlert("Failed", 900);
      break;
  }
}

bool QuickPingOverlay::appendOption(char* buf, size_t buf_len, const char* option) const {
  if (!buf || buf_len == 0 || !option) return false;
  const size_t used = strlen(buf);
  if (used >= buf_len - 1) return false;
  if (option[0] == '\0') option = "?";
  const char* sep = used > 0 ? "\n" : "";
  const int written = lv_snprintf(buf + used, buf_len - used, "%s%s", sep, option);
  return written > 0 && static_cast<size_t>(written) < buf_len - used;
}

void QuickPingOverlay::onDropdownEvent(lv_event_t* e) {
  auto* self = static_cast<QuickPingOverlay*>(lv_event_get_user_data(e));
  if (!self) return;

  const lv_event_code_t code = lv_event_get_code(e);
  _lv_obj_t* dropdown = lv_event_get_target(e);

  if (code == LV_EVENT_PRESSED || code == LV_EVENT_CLICKED ||
      code == LV_EVENT_FOCUSED || code == LV_EVENT_VALUE_CHANGED) {
    if (!self->_rebuilding_focus_group) self->closeKeyboardForOutsideTarget(dropdown);
  }

  if (code == LV_EVENT_READY) {
    self->syncDropdownListLayout(dropdown);
    return;
  }
  if (code != LV_EVENT_VALUE_CHANGED) return;
  if (self->_syncing_dropdowns) return;
  self->clearRepeatSelection();
  self->activateDropdown(dropdown);

  lv_event_stop_bubbling(e);
  lv_event_stop_processing(e);
}

void QuickPingOverlay::onDropdownConfirmPreprocess(lv_event_t* e) {
  const lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_KEY) {
    const uint32_t key = lv_event_get_key(e);
    auto* self = static_cast<QuickPingOverlay*>(lv_event_get_user_data(e));
    if (!self) return;
    _lv_obj_t* dropdown = lv_event_get_target(e);

    // Handle ESC before the LVGL dropdown class closes the list. Otherwise
    // the bubbled ESC reaches the overlay after the list is already closed
    // and incorrectly closes the whole QuickPing pane as well.
    if (key == LV_KEY_ESC) {
      if (!dropdown || !lv_dropdown_is_open(dropdown)) return;
      self->clearRepeatSelection();
      lv_dropdown_close(dropdown);
      lv_event_stop_bubbling(e);
      lv_event_stop_processing(e);
      return;
    }
    if (key != LV_KEY_ENTER) return;
    if (dropdown && lv_dropdown_is_open(dropdown)) self->armRepeatSelection(dropdown);
    return;
  } else if (code == LV_EVENT_RELEASED) {
    lv_indev_t* indev = lv_indev_get_act();
    if (!indev || lv_indev_get_type(indev) == LV_INDEV_TYPE_POINTER) return;
  } else {
    return;
  }
  auto* self = static_cast<QuickPingOverlay*>(lv_event_get_user_data(e));
  if (!self) return;
  _lv_obj_t* dropdown = lv_event_get_target(e);
  if (dropdown && lv_dropdown_is_open(dropdown)) self->armRepeatSelection(dropdown);
}

void QuickPingOverlay::finishRepeatSelectionAsync(void* user_data) {
  auto* self = static_cast<QuickPingOverlay*>(user_data);
  if (self) self->finishRepeatSelection();
}

void QuickPingOverlay::openPendingKeyboardAsync(void* user_data) {
  auto* self = static_cast<QuickPingOverlay*>(user_data);
  if (self) self->openPendingKeyboard();
}

void QuickPingOverlay::onMessageInputEvent(lv_event_t* e) {
  const lv_event_code_t code = lv_event_get_code(e);
  auto* self = static_cast<QuickPingOverlay*>(lv_event_get_user_data(e));
  if (!self) return;

  if (code == LV_EVENT_KEY) {
    const uint32_t key = lv_event_get_key(e);
    if (key == LV_KEY_ENTER) {
      self->handleMessageInput();
    } else if (key == LV_KEY_ESC) {
      (void)self->onKey(key);
    } else {
      return;
    }
  } else if (code == LV_EVENT_CLICKED) {
    self->handleMessageInput();
  } else {
    return;
  }
  lv_event_stop_bubbling(e);
  lv_event_stop_processing(e);
}

void QuickPingOverlay::onKeyboardEvent(lv_event_t* e) {
  auto* self = static_cast<QuickPingOverlay*>(lv_event_get_user_data(e));
  if (!self) return;
  const lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_READY) {
    self->submitMessageFromTextarea();
  } else if (code == LV_EVENT_CANCEL) {
    self->setKeyboardVisible(false);
    self->rebuildFocusGroup(self->_ta_message);
  }
  if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
    lv_event_stop_bubbling(e);
    lv_event_stop_processing(e);
  }
}

void QuickPingOverlay::onKeyboardValuePre(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;
  auto* self = static_cast<QuickPingOverlay*>(lv_event_get_user_data(e));
  if (!self) return;
  _lv_obj_t* keyboard = lv_event_get_target(e);
  if (!keyboard) return;
  const uint16_t btn_id = lv_btnmatrix_get_selected_btn(keyboard);
  if (btn_id == LV_BTNMATRIX_BTN_NONE) return;
  const char* text = lv_btnmatrix_get_btn_text(keyboard, btn_id);
  if (!text) return;

  if (strcmp(text, "ABC") == 0) {
    lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_USER_3);
    lv_event_stop_processing(e);
  } else if (strcmp(text, "abc") == 0) {
    lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_USER_2);
    lv_event_stop_processing(e);
  } else if (strcmp(text, "SP") == 0) {
    if (self->_ta_message) lv_textarea_add_char(self->_ta_message, ' ');
    lv_event_stop_processing(e);
  }
  if (strcmp(text, "ABC") == 0 || strcmp(text, "abc") == 0 || strcmp(text, "SP") == 0) {
    lv_event_stop_bubbling(e);
  }
}

void QuickPingOverlay::onOutsideEvent(lv_event_t* e) {
  const lv_event_code_t code = lv_event_get_code(e);
  if (code != LV_EVENT_CLICKED) return;
  auto* self = static_cast<QuickPingOverlay*>(lv_event_get_user_data(e));
  if (!self) return;
  _lv_obj_t* target = lv_event_get_target(e);

  // A background click may dismiss a transient dropdown/keyboard, but the
  // pane itself is closed only by its upward swipe gesture (or non-touch ESC).
  if (target == self->_root || target == self->_content) {
    (void)self->dismissTransientControls();
  } else {
    self->closeKeyboardForOutsideTarget(target);
    self->handleDropdownRow(target);
  }
  lv_event_stop_bubbling(e);
  lv_event_stop_processing(e);
}

}  // namespace heltec::meshcore::ui

#endif  // HELTEC_V4_R8_TFT && HELTEC_HAS_TOUCH
