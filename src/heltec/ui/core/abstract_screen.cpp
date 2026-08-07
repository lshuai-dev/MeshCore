#include "abstract_screen.hpp"

#include "ui/app/ui_theme.hpp"
#include "ht_meta_data.hpp"
#include "ui/core/ui_events.h"

namespace heltec::meshcore::ui {

AbstractScreen::AbstractScreen(biz::IBizFacade& biz, const char* title, const lv_img_dsc_t* icon,
                               bool root_scroll_focus)
    : UiSurface(biz), _title(title), _icon(icon), _root_scroll_focus(root_scroll_focus) {}

_lv_obj_t* AbstractScreen::createRoot(_lv_obj_t* parent) {
  return ht_obj_create(parent, meta_id::ScreenRoot);
}

_lv_obj_t* AbstractScreen::create(_lv_obj_t* parent) {
  if (!UiSurface::create(parent)) return nullptr;
  lv_obj_set_size(_root, lv_pct(100), lv_pct(100));
  lv_obj_set_flex_flow(_root, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(_root, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
  if (_root_scroll_focus) {
    lv_obj_add_flag(_root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(_root, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(_root, LV_SCROLLBAR_MODE_AUTO);
  } else {
    lv_obj_clear_flag(_root, LV_OBJ_FLAG_SCROLLABLE);
  }
  if (_focus_group) {
    lv_group_set_wrap(_focus_group, true);
    lv_obj_add_flag(_root, LV_OBJ_FLAG_CLICK_FOCUSABLE);
    if (_root_scroll_focus) lv_obj_add_flag(_root, LV_OBJ_FLAG_SCROLL_WITH_ARROW);
    ht_set_user_data(_root, this);
    _root_focus_fallback = true;
  }

  addFocusObject(_root);
  return _root;
}

const char* AbstractScreen::title() const { return _title; }

const lv_img_dsc_t* AbstractScreen::icon() const { return _icon; }

_lv_obj_t* AbstractScreen::tile() const {
  return _root ? lv_obj_get_parent(_root) : nullptr;
}

void AbstractScreen::onEnter() {
  UiSurface::onEnter();
  if (!_focus_group) return;
  lv_group_set_editing(_focus_group, false);
  if (_root_focus_fallback && _root) lv_obj_scroll_to_y(_root, 0, LV_ANIM_OFF);
  if (_lv_obj_t* const first = firstAvailableFocusItem()) lv_group_focus_obj(first);
}

void AbstractScreen::onExit() {
  if (_focus_group) lv_group_set_editing(_focus_group, false);
  // Tile screens stay visible when focus moves to nav pane / overlay; only overlays hide on exit.
}

_lv_obj_t* AbstractScreen::focusedObject() const {
  if (_focus_group) {
    _lv_obj_t* const focused = lv_group_get_focused(_focus_group);
    if (isAvailableFocusItem(focused)) return focused;
  }
  return firstAvailableFocusItem();
}

void AbstractScreen::addFocusItem(_lv_obj_t* object, _lv_obj_t* frame,
                                  bool focus_on_pointer_press,
                                  FocusVisual visual) {
  if (!object || !_focus_group || _focus_item_count >= kMaxFocusItems) return;
  for (uint8_t i = 0; i < _focus_item_count; ++i) {
    if (_focus_items[i] == object) return;
  }

  if (_root_focus_fallback && _root && lv_obj_get_group(_root) == _focus_group) {
    lv_group_remove_obj(_root);
    lv_obj_clear_state(_root, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
    _default_focus = nullptr;
    _root_focus_fallback = false;
  }

  _lv_obj_t* const focus_frame = frame ? frame : object;
  if (lv_obj_get_child_cnt(focus_frame) >= 2) {
    lv_obj_set_flex_align(focus_frame, LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  }
  const bool row_visual = visual == FocusVisual::Row || focus_frame != object;
  if (row_visual) {
    ui_theme_apply_focus_row(focus_frame);
  } else {
    ui_theme_apply_focus_frame(focus_frame);
  }
  if (focus_frame != object) {
    // A frame is only the visual focus surface for its child control. Generic
    // LVGL objects are CLICK_FOCUSABLE by default; leaving that flag enabled
    // lets pointer input focus the out-of-group frame while keypad focus stays
    // on another control, producing two highlighted rows.
    lv_obj_clear_flag(focus_frame, LV_OBJ_FLAG_CLICK_FOCUSABLE);
    if (focus_on_pointer_press) {
      lv_obj_add_event_cb(
          focus_frame,
          [](lv_event_t* e) {
            if (lv_event_get_code(e) != LV_EVENT_PRESSED) return;
            _lv_obj_t* const control =
                static_cast<_lv_obj_t*>(lv_event_get_user_data(e));
            if (!control || !lv_obj_is_valid(control) ||
                lv_obj_has_state(control, LV_STATE_DISABLED) ||
                lv_obj_has_flag(control, LV_OBJ_FLAG_HIDDEN)) {
              return;
            }
            lv_group_t* const group = lv_obj_get_group(control);
            if (group) lv_group_focus_obj(control);
          },
          static_cast<lv_event_code_t>(LV_EVENT_PRESSED | LV_EVENT_PREPROCESS), object);
    }
    ui_theme_apply_focus_control(object);
    lv_obj_add_event_cb(object, onFocusItemChanged, LV_EVENT_ALL, focus_frame);
  }
  addFocusObject(object);
  if (focus_on_pointer_press) {
    lv_obj_add_flag(object, LV_OBJ_FLAG_CLICK_FOCUSABLE);
  } else {
    lv_obj_clear_flag(object, LV_OBJ_FLAG_CLICK_FOCUSABLE);
    lv_obj_add_event_cb(
        focus_frame,
        [](lv_event_t* e) {
          if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
          _lv_obj_t* const control =
              static_cast<_lv_obj_t*>(lv_event_get_user_data(e));
          if (!control || !lv_obj_is_valid(control) ||
              lv_obj_has_state(control, LV_STATE_DISABLED) ||
              lv_obj_has_flag(control, LV_OBJ_FLAG_HIDDEN)) {
            return;
          }
          lv_group_t* const group = lv_obj_get_group(control);
          if (group) lv_group_focus_obj(control);
        },
        static_cast<lv_event_code_t>(LV_EVENT_CLICKED | LV_EVENT_PREPROCESS), object);
  }
  if (focus_frame != object && lv_obj_has_state(object, LV_STATE_FOCUSED)) {
    lv_obj_add_state(focus_frame, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
    lv_obj_invalidate(focus_frame);
  }
  _focus_items[_focus_item_count++] = object;
}

bool AbstractScreen::isAvailableFocusItem(const _lv_obj_t* obj) const {
  if (!obj || !lv_obj_is_valid(obj) || lv_obj_get_group(obj) != _focus_group ||
      lv_obj_has_state(obj, LV_STATE_DISABLED)) {
    return false;
  }
  for (const _lv_obj_t* current = obj; current; current = lv_obj_get_parent(current)) {
    if (lv_obj_has_flag(current, LV_OBJ_FLAG_HIDDEN)) return false;
  }
  return true;
}

_lv_obj_t* AbstractScreen::firstAvailableFocusItem() const {
  if (_root_focus_fallback && isAvailableFocusItem(_root)) return _root;
  for (uint8_t i = 0; i < _focus_item_count; ++i) {
    if (isAvailableFocusItem(_focus_items[i])) return _focus_items[i];
  }
  return nullptr;
}

void AbstractScreen::onFocusItemChanged(lv_event_t* e) {
  const lv_event_code_t code = lv_event_get_code(e);
  if (code != LV_EVENT_FOCUSED && code != LV_EVENT_DEFOCUSED) return;
  _lv_obj_t* const frame = static_cast<_lv_obj_t*>(lv_event_get_user_data(e));
  _lv_obj_t* const object = lv_event_get_target(e);
  if (!frame || !object || frame == object) return;

  if (code == LV_EVENT_FOCUSED) {
    lv_obj_add_state(frame, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
  } else {
    lv_obj_clear_state(frame, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
  }
  lv_obj_invalidate(frame);
}

bool AbstractScreen::onKey(uint32_t key) {
  if (key == LV_KEY_ESC) {
    return emitEvent(UiEventType::NavOpen);
  }
  return false;
}
}  // namespace heltec::meshcore::ui
