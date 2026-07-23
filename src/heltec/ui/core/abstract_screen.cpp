#include "abstract_screen.hpp"

#include "ht_meta_data.hpp"
#include "ui/core/ui_events.h"

namespace heltec::meshcore::ui {
namespace {

bool isScrollForwardKey(uint32_t key) {
  return key == LV_KEY_DOWN || key == LV_KEY_NEXT || key == LV_KEY_RIGHT;
}

bool isScrollBackwardKey(uint32_t key) {
  return key == LV_KEY_UP || key == LV_KEY_PREV || key == LV_KEY_LEFT;
}

}  // namespace

AbstractScreen::AbstractScreen(biz::IBizFacade& biz, const char* title, const lv_img_dsc_t* icon,
                               bool root_scroll_focus)
    : UiSurface(biz), _title(title), _icon(icon), _root_scroll_focus(root_scroll_focus) {}

_lv_obj_t* AbstractScreen::createRoot(_lv_obj_t* parent) {
  return ht_obj_create(parent, meta_id::ScreenRoot);
}

_lv_obj_t* AbstractScreen::create(_lv_obj_t* parent) {
  if (!UiSurface::create(parent)) return nullptr;
  const lv_coord_t gap =
#if defined(HELTEC_V4_R8_TFT)
      LV_DPX(10);
#else
      3;
#endif
  lv_obj_set_size(_root, lv_pct(100), lv_pct(100));
  lv_obj_set_flex_flow(_root, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(_root, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_START);
  lv_obj_set_style_pad_all(_root, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_row(_root, gap, LV_PART_MAIN);
  lv_obj_set_style_pad_column(_root, gap, LV_PART_MAIN);
  if (_root_scroll_focus) {
    lv_obj_add_flag(_root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(_root, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(_root, LV_SCROLLBAR_MODE_AUTO);
  } else {
    lv_obj_clear_flag(_root, LV_OBJ_FLAG_SCROLLABLE);
  }
  if (_focus_group && _root_scroll_focus) {
    lv_obj_add_flag(_root, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLL_WITH_ARROW);
    lv_obj_add_event_cb(_root, [](lv_event_t* e) {
      if (LV_EVENT_KEY != lv_event_get_code(e)) return;
      auto* self = static_cast<AbstractScreen*>(lv_event_get_user_data(e));
      if (!self || !self->handleScrollWrapKey(lv_event_get_key(e))) return;
      lv_event_stop_processing(e);
      lv_event_stop_bubbling(e);
    }, static_cast<lv_event_code_t>(LV_EVENT_KEY | LV_EVENT_PREPROCESS), this);
    lv_group_add_obj(_focus_group, _root);
  }
  return _root;
}

const char* AbstractScreen::title() const { return _title; }

const lv_img_dsc_t* AbstractScreen::icon() const { return _icon; }

_lv_obj_t* AbstractScreen::tile() const {
  return _root ? lv_obj_get_parent(_root) : nullptr;
}

void AbstractScreen::onEnter() {
  UiSurface::onEnter();
}

void AbstractScreen::onExit() {
  // Tile screens stay visible when focus moves to nav pane / overlay; only overlays hide on exit.
}

_lv_obj_t* AbstractScreen::focusedObject() const {
  if (_root_scroll_focus && _root) return _root;
  return defaultFocus();
}

bool AbstractScreen::handleScrollWrapKey(uint32_t key) {
  if (!_root_scroll_focus || !_root) return false;

  const bool forward = isScrollForwardKey(key);
  const bool backward = isScrollBackwardKey(key);
  if (!forward && !backward) return false;

  lv_obj_update_layout(_root);
  const lv_coord_t top = lv_obj_get_scroll_top(_root);
  const lv_coord_t bottom = lv_obj_get_scroll_bottom(_root);
  if (top <= 0 && bottom <= 0) return false;

  if (forward && bottom <= 0 && top > 0) {
    lv_obj_scroll_to_y(_root, 0, LV_ANIM_OFF);
    return true;
  }

  if (backward && top <= 0 && bottom > 0) {
    lv_obj_scroll_to_y(_root, top + bottom, LV_ANIM_OFF);
    return true;
  }

  return false;
}

bool AbstractScreen::onKey(uint32_t key) {
  if (key == LV_KEY_ESC) {
    return emitEvent(UiEventType::NavOpen);
  }
  if (key == LV_KEY_ENTER) {
    bool eligible = screenId() == eScreenId::Home || screenId() == eScreenId::Recent ||
                    screenId() == eScreenId::Radio || screenId() == eScreenId::GPS;
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
    eligible = eligible || screenId() == eScreenId::Compass ||
               screenId() == eScreenId::FindFriend;
#endif
    if (!eligible) return false;
#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
    return emitEvent(UiEventType::QuickPingOpen);
#else
    return emitEvent(UiEventType::ContextOpen);
#endif
  }
  return false;
}
}  // namespace heltec::meshcore::ui
