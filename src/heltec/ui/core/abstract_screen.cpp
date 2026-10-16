#include "abstract_screen.hpp"
#include "ui/widgets/ht_container.hpp"
#include "ui/core/ht_event.h"
#include "ui/widgets/ht_scroll_view.hpp"
#if defined(HELTEC_HAS_KEY_FRAME) && HELTEC_HAS_KEY_FRAME
#include "heltec/drivers/display/e213_port.hpp"
#endif

namespace heltec::meshcore::ui {
namespace {

constexpr lv_obj_flag_t kFocusItemFlag = LV_OBJ_FLAG_USER_1;

bool focus_object_available(const lv_obj_t *object) {
    if (!object || !lv_obj_is_valid(object) || lv_obj_has_state(object, LV_STATE_DISABLED)) {
        return false;
    }
    for (const lv_obj_t *parent = object; parent; parent = lv_obj_get_parent(parent)) {
        if (lv_obj_has_flag(parent, LV_OBJ_FLAG_HIDDEN)) {
            return false;
        }
    }
    return true;
}

} // namespace

AbstractScreen::AbstractScreen(biz::IBizFacade &biz, const char *title, const lv_img_dsc_t *icon)
    : AbstractScreen(biz, title, icon, ht_scroll_view_class) {
}

AbstractScreen::AbstractScreen(biz::IBizFacade &biz, const char *title, const lv_img_dsc_t *icon,
                               const lv_obj_class_t &root_class)
    : UiSurface(biz, root_class), _title(title), _icon(icon) {
}

bool AbstractScreen::buildSurface() {
    lv_obj_set_size(_root, lv_pct(100), lv_pct(100));
    lv_obj_set_flex_flow(_root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(_root, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_user_data(_root, this);

    lv_obj_add_flag(_root,
                    LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_event_cb(_root, handleRootEvent, LV_EVENT_CLICKED, this);
    focusRoot();
    return true;
}

const char *AbstractScreen::title() const {
    return _title;
}

const lv_img_dsc_t *AbstractScreen::icon() const {
    return _icon;
}

void AbstractScreen::onEnter() {
    (void)lv_async_call_cancel(deferredActivateControls, this);
    (void)lv_async_call_cancel(deferredActivateRoot, this);
    if (!focusGroup() || !_root) return;
    lv_obj_scroll_to_y(_root, 0, LV_ANIM_OFF);
    focusRoot();
}

void AbstractScreen::onExit() {
    (void)lv_async_call_cancel(deferredActivateControls, this);
    (void)lv_async_call_cancel(deferredActivateRoot, this);
    if (focusGroup()) {
        lv_group_set_editing(focusGroup(), false);
    }
    // Tile screens stay visible when focus moves to nav pane / overlay; only overlays hide on exit.
}

void AbstractScreen::focusRoot() {
    if (!focusGroup() || !_root || !lv_obj_is_valid(_root)) {
        return;
    }
    lv_group_set_editing(focusGroup(), false);
    lv_group_remove_all_objs(focusGroup());
    lv_group_add_obj(focusGroup(), _root);
}

void AbstractScreen::focusControls() {
    if (!focusGroup()) {
        return;
    }
    lv_group_set_editing(focusGroup(), false);
    lv_group_remove_all_objs(focusGroup());
    lv_obj_tree_walk(_root, addFocusObjectFromTree, focusGroup());
    if (!lv_group_get_focused(focusGroup()) && _root && lv_obj_is_valid(_root)) {
        lv_group_add_obj(focusGroup(), _root);
    }
}

lv_obj_tree_walk_res_t AbstractScreen::addFocusObjectFromTree(_lv_obj_t *object, void *user_data) {
    auto *const group = static_cast<lv_group_t *>(user_data);
    if (!object || !group) {
        return LV_OBJ_TREE_WALK_END;
    }
    if (lv_obj_has_flag(object, LV_OBJ_FLAG_HIDDEN)) {
        return LV_OBJ_TREE_WALK_SKIP_CHILDREN;
    }
    if (lv_obj_has_flag(object, kFocusItemFlag) && focus_object_available(object)) {
        lv_group_add_obj(group, object);
    }
    return LV_OBJ_TREE_WALK_NEXT;
}

void AbstractScreen::deferredActivateControls(void *user_data) {
    auto *const screen = static_cast<AbstractScreen *>(user_data);
    if (!screen || !screen->focusGroup() ||
        lv_group_get_focused(screen->focusGroup()) != screen->_root) {
        return;
    }
    screen->focusControls();
}

void AbstractScreen::deferredActivateRoot(void *user_data) {
    auto *const screen = static_cast<AbstractScreen *>(user_data);
    if (!screen || !screen->focusGroup() ||
        lv_group_get_focused(screen->focusGroup()) == screen->_root) {
        return;
    }
    screen->focusRoot();
}

void AbstractScreen::handleRootEvent(lv_event_t *event) {
    auto *const screen = static_cast<AbstractScreen *>(lv_event_get_user_data(event));
    if (!screen || !screen->focusGroup() || lv_event_get_target(event) != screen->_root ||
        lv_group_get_focused(screen->focusGroup()) != screen->_root) {
        return;
    }
    (void)lv_async_call(deferredActivateControls, screen);
}

void AbstractScreen::handleFocusChangedEvent(lv_event_t *event) {
    const lv_event_code_t code = lv_event_get_code(event);
    _lv_obj_t *const frame = static_cast<_lv_obj_t *>(lv_event_get_user_data(event));
    if (!frame) return;
    if (lv_obj_check_type(frame, &ht_container_class)) {
        ht_container_set_focused(frame, code == LV_EVENT_FOCUSED);
    }
    lv_obj_invalidate(frame);
}

void AbstractScreen::handleFramePressedEvent(lv_event_t *event) {
    if (lv_event_get_code(event) != LV_EVENT_PRESSED) return;
    _lv_obj_t *const control = static_cast<_lv_obj_t *>(lv_event_get_user_data(event));
    if (!focus_object_available(control)) return;
    if (lv_group_t *const group = lv_obj_get_group(control)) {
        lv_group_focus_obj(control);
    }
}

#if defined(HELTEC_HAS_KEY_FRAME) && HELTEC_HAS_KEY_FRAME
void AbstractScreen::keyFrameFocusEvent(lv_event_t *event) {
    if (lv_event_get_code(event) == LV_EVENT_FOCUSED) {
        heltec::meshcore::dal::e213::requestSelectionRefresh();
    }
}
#endif

void AbstractScreen::addFocusableObject(_lv_obj_t *object, _lv_obj_t *frame, bool propagate_focus) {
    if (!object || lv_obj_has_flag(object, kFocusItemFlag)) return;

    _lv_obj_t *const focus_frame = frame ? frame : object;
    if (propagate_focus) {
        lv_obj_clear_flag(focus_frame, LV_OBJ_FLAG_CLICK_FOCUSABLE);
        lv_obj_add_event_cb(object, handleFocusChangedEvent, LV_EVENT_FOCUSED, focus_frame);
        lv_obj_add_event_cb(object, handleFocusChangedEvent, LV_EVENT_DEFOCUSED, focus_frame);
    }
    if (focus_frame != object) {
        lv_obj_clear_flag(focus_frame, LV_OBJ_FLAG_CLICK_FOCUSABLE);
        lv_obj_add_event_cb(focus_frame, handleFramePressedEvent,
                            static_cast<lv_event_code_t>(LV_EVENT_PRESSED | LV_EVENT_PREPROCESS),
                            object);
    }
    lv_obj_add_flag(object, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_flag(object, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_add_event_cb(object, lv_event_stop_bubbling,
                        static_cast<lv_event_code_t>(LV_EVENT_FOCUSED | LV_EVENT_PREPROCESS),
                        object);
    lv_obj_add_event_cb(object, lv_event_stop_bubbling,
                        static_cast<lv_event_code_t>(LV_EVENT_DEFOCUSED | LV_EVENT_PREPROCESS),
                        object);
    for (lv_obj_t *parent = lv_obj_get_parent(object); parent && parent != _root;
         parent = lv_obj_get_parent(parent)) {
        lv_obj_add_flag(parent, LV_OBJ_FLAG_EVENT_BUBBLE);
    }
    lv_obj_add_flag(object, LV_OBJ_FLAG_CLICK_FOCUSABLE);
    lv_obj_add_flag(object, kFocusItemFlag);
#if defined(HELTEC_HAS_KEY_FRAME) && HELTEC_HAS_KEY_FRAME
    lv_obj_add_event_cb(object, keyFrameFocusEvent, LV_EVENT_FOCUSED, nullptr);
#endif
}

bool AbstractScreen::onKey(uint32_t key) {
    lv_group_t *const active_group = focusGroup();
    if (active_group && lv_group_get_focused(active_group) != _root) {
        if (key == LV_KEY_DOWN || key == LV_KEY_RIGHT || key == LV_KEY_NEXT) {
            lv_group_set_wrap(active_group, true);
            lv_group_focus_next(active_group);
            return true;
        }
        if (key == LV_KEY_UP || key == LV_KEY_LEFT || key == LV_KEY_PREV) {
            lv_group_set_wrap(active_group, true);
            lv_group_focus_prev(active_group);
            return true;
        }
    }
    if (key == LV_KEY_ESC) {
        if (active_group && lv_group_get_focused(active_group) != _root) {
            (void)lv_async_call(deferredActivateRoot, this);
            return true;
        }
        return emitEvent(EventType::NavOpen);
    }
    return false;
}
} // namespace heltec::meshcore::ui
