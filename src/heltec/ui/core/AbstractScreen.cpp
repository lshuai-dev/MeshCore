#include "AbstractScreen.hpp"
#include "ui/core/UiMessage.hpp"
#if defined(HELTEC_HAS_KEY_FRAME) && HELTEC_HAS_KEY_FRAME
#include "heltec/drivers/display/DisplayPort.hpp"
#endif

namespace Heltec::MeshCore::Ui {
#if defined(HELTEC_UI_HAS_TOUCH) && HELTEC_UI_HAS_TOUCH
namespace {

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
#endif

AbstractScreen::AbstractScreen(Biz::IBizFacade &biz, const char *title, const lv_img_dsc_t *icon)
    : UiSurface(biz), _title(title), _icon(icon) {
}

bool AbstractScreen::build() {
    lv_obj_set_size(_root, lv_pct(100), lv_pct(100));
    lv_obj_set_flex_flow(_root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(_root, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_add_flag(_root, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_WITH_ARROW);
    lv_obj_set_scroll_dir(_root, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(_root, LV_SCROLLBAR_MODE_AUTO);
#if defined(HELTEC_UI_HAS_TOUCH) && HELTEC_UI_HAS_TOUCH
    lv_obj_add_flag(_root, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CLICK_FOCUSABLE);
    lv_obj_add_event_cb(_root, handleRootEvent, LV_EVENT_CLICKED, this);
#endif
    return true;
}

void AbstractScreen::onEnter() {
    (void)lv_async_call_cancel(deferredActivateControls, this);
    (void)lv_async_call_cancel(deferredActivateRoot, this);
    if (!_root) return;
    lv_obj_scroll_to_y(_root, 0, LV_ANIM_OFF);
    _gridNav.enterBrowse();
}

void AbstractScreen::onExit() {
    (void)lv_async_call_cancel(deferredActivateControls, this);
    (void)lv_async_call_cancel(deferredActivateRoot, this);
    _gridNav.setEditing(false);
    // Tile screens stay visible when focus moves to nav pane / overlay; only overlays hide on exit.
}

void AbstractScreen::deferredActivateControls(void *user_data) {
    auto *const screen = static_cast<AbstractScreen *>(user_data);
    if (!screen || screen->_gridNav.mode() != HtGridNav::Mode::Browse) {
        return;
    }
    (void)screen->_gridNav.enterControls();
}

void AbstractScreen::deferredActivateRoot(void *user_data) {
    auto *const screen = static_cast<AbstractScreen *>(user_data);
    if (!screen || screen->_gridNav.mode() == HtGridNav::Mode::Browse) {
        return;
    }
    screen->_gridNav.enterBrowse();
}

#if defined(HELTEC_UI_HAS_TOUCH) && HELTEC_UI_HAS_TOUCH
void AbstractScreen::handleRootEvent(lv_event_t *event) {
    auto *const screen = static_cast<AbstractScreen *>(lv_event_get_user_data(event));
    if (!screen || lv_event_get_target(event) != screen->_root ||
        screen->_gridNav.mode() != HtGridNav::Mode::Browse) {
        return;
    }
    (void)lv_async_call(deferredActivateControls, screen);
}

void AbstractScreen::handleFramePressedEvent(lv_event_t *event) {
    if (lv_event_get_code(event) != LV_EVENT_PRESSED) return;
    auto *const screen = static_cast<AbstractScreen *>(lv_event_get_user_data(event));
    lv_obj_t *const frame = lv_event_get_current_target(event);
    if (!screen || !frame) return;

    struct FindContext {
        AbstractScreen *screen = nullptr;
        lv_obj_t *control = nullptr;
    } context;
    context.screen = screen;
    lv_obj_tree_walk(
        frame,
        [](lv_obj_t *object, void *user_data) {
            auto *const find = static_cast<FindContext *>(user_data);
            if (find && find->screen->_gridNav.contains(object) &&
                focus_object_available(object)) {
                find->control = object;
                return LV_OBJ_TREE_WALK_END;
            }
            return LV_OBJ_TREE_WALK_NEXT;
        },
        &context);
    if (context.control) {
        (void)screen->_gridNav.focus(context.control);
    }
}
#endif

#if defined(HELTEC_HAS_KEY_FRAME) && HELTEC_HAS_KEY_FRAME
void AbstractScreen::keyFrameFocusEvent(lv_event_t *event) {
    if (lv_event_get_code(event) == LV_EVENT_FOCUSED) {
        Heltec::MeshCore::Dal::DisplayPort::requestSelectionRefresh();
    }
}
#endif

void AbstractScreen::addControl(_lv_obj_t *control, _lv_obj_t *row) {
    if (!control || _gridNav.contains(control)) return;

#if defined(HELTEC_UI_HAS_TOUCH) && HELTEC_UI_HAS_TOUCH
    _lv_obj_t *const touch_target = row ? row : control;
    if (touch_target != control) {
        lv_obj_clear_flag(touch_target, LV_OBJ_FLAG_CLICK_FOCUSABLE);
        lv_obj_add_event_cb(touch_target, handleFramePressedEvent,
                            static_cast<lv_event_code_t>(LV_EVENT_PRESSED | LV_EVENT_PREPROCESS),
                            this);
    }
#else
    (void)row;
#endif
    _gridNav.add(control);
#if defined(HELTEC_HAS_KEY_FRAME) && HELTEC_HAS_KEY_FRAME
    lv_obj_add_event_cb(control, keyFrameFocusEvent, LV_EVENT_FOCUSED, nullptr);
#endif
}

bool AbstractScreen::onKey(uint32_t key) {
    if (_gridNav.mode() == HtGridNav::Mode::Controls) {
        if (key == LV_KEY_ESC && !_gridNav.isEditing()) {
            (void)lv_async_call(deferredActivateRoot, this);
            return true;
        }
        return _gridNav.handleKey(key);
    }
    if (key == LV_KEY_ESC) {
        return notifyApp(UiMessageType::NavOpen);
    }
    if (key == LV_KEY_ENTER) {
        (void)lv_async_call(deferredActivateControls, this);
        return true;
    }
    return false;
}
} // namespace Heltec::MeshCore::Ui
