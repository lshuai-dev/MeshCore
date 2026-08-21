#include "abstract_screen.hpp"

#include "ht_meta_data.hpp"
#include "ui/app/ui_theme.hpp"
#include "ui/core/ui_deferred_queue.hpp"
#include "ui/core/ui_events.h"

namespace heltec::meshcore::ui
{

AbstractScreen::AbstractScreen(biz::IBizFacade &biz, const char *title, const lv_img_dsc_t *icon,
                               bool root_scroll_focus)
    : UiSurface(biz), _title(title), _icon(icon), _root_scroll_focus(root_scroll_focus)
{
}

_lv_obj_t *AbstractScreen::createRoot(_lv_obj_t *parent)
{
    return ht_obj_create(parent, meta_id::ScreenRoot);
}

_lv_obj_t *AbstractScreen::create(_lv_obj_t *parent)
{
    if (!UiSurface::create(parent))
        return nullptr;
    lv_obj_set_size(_root, lv_pct(100), lv_pct(100));
    lv_obj_set_flex_flow(_root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(_root, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    if (_root_scroll_focus)
    {
        lv_obj_add_flag(_root, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scroll_dir(_root, LV_DIR_VER);
        lv_obj_set_scrollbar_mode(_root, LV_SCROLLBAR_MODE_AUTO);
    }
    else
    {
        lv_obj_clear_flag(_root, LV_OBJ_FLAG_SCROLLABLE);
    }
    if (_focus_group)
    {
        lv_group_set_wrap(_focus_group, true);
        if (_root_scroll_focus)
            lv_obj_add_flag(_root, LV_OBJ_FLAG_SCROLL_WITH_ARROW);
        ht_set_user_data(_root, this);
    }

    _idle_group = lv_group_create();
    if (!_idle_group)
    {
        return nullptr;
    }
    lv_group_set_wrap(_idle_group, false);
    lv_group_set_editing(_idle_group, false);
    lv_group_add_obj(_idle_group, _root);
    lv_obj_add_flag(_root, LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_EVENT_BUBBLE);
    _active_group = _idle_group;
    _focus_mode = FocusMode::Idle;
    return _root;
}

const char *AbstractScreen::title() const
{
    return _title;
}

const lv_img_dsc_t *AbstractScreen::icon() const
{
    return _icon;
}

_lv_obj_t *AbstractScreen::tile() const
{
    return _root ? lv_obj_get_parent(_root) : nullptr;
}

void AbstractScreen::onEnter()
{
    UiSurface::onEnter();
    (void)defer_cancel(activateFocusModeDeferred, this);
    _focus_activation_pending = false;
    if (!_focus_group || !_idle_group || !_root)
        return;
    lv_group_set_editing(_idle_group, false);
    lv_group_set_editing(_focus_group, false);
    lv_obj_scroll_to_y(_root, 0, LV_ANIM_OFF);
    activateFocusMode(FocusMode::Idle, _root);
}

void AbstractScreen::onExit()
{
    (void)defer_cancel(activateFocusModeDeferred, this);
    _focus_activation_pending = false;
    if (_focus_group)
    {
        lv_group_set_editing(_focus_group, false);
    }
    if (_idle_group && _root)
    {
        activateFocusMode(FocusMode::Idle, _root);
    }
    // Tile screens stay visible when focus moves to nav pane / overlay; only overlays hide on exit.
}

_lv_obj_t *AbstractScreen::focusedObject() const
{
    if (_active_group)
    {
        _lv_obj_t *const focused = lv_group_get_focused(_active_group);
        if (_focus_mode == FocusMode::Idle && focused == _root)
        {
            return focused;
        }
        if (_focus_mode == FocusMode::Active && isAvailableFocusItem(focused))
        {
            return focused;
        }
    }
    return _focus_mode == FocusMode::Idle ? _root : firstAvailableFocusItem();
}

lv_group_t *AbstractScreen::group() const
{
    return _active_group;
}

void AbstractScreen::activateFocusMode(FocusMode mode, _lv_obj_t *focus)
{
    if (!_focus_group || !_idle_group || !_root || !focus)
    {
        return;
    }

    if (mode == FocusMode::Idle)
    {
        setFocusStateRecursive(_root, nullptr, false);
        ui_theme_set_focus_text_scroll(_root, false);
    }

    _focus_mode = mode;
    _active_group = mode == FocusMode::Idle ? _idle_group : _focus_group;
    lv_group_focus_obj(focus);
    if (mode == FocusMode::Active)
    {
        lv_obj_clear_state(_root, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
    }
    (void)emit(UiEventType::RebindInput);
}

void AbstractScreen::activateFocusModeDeferred(void *user_data)
{
    auto *const screen = static_cast<AbstractScreen *>(user_data);
    if (!screen)
    {
        return;
    }
    screen->_focus_activation_pending = false;
    if (screen->_focus_mode != FocusMode::Idle)
    {
        return;
    }
    _lv_obj_t *const first = screen->firstAvailableFocusItem();
    if (first)
    {
        screen->activateFocusMode(FocusMode::Active, first);
    }
}

void AbstractScreen::addFocusItem(_lv_obj_t *object, _lv_obj_t *frame, bool propagate_focus)
{
    if (!object || !_focus_group || _focus_item_count >= kMaxFocusItems)
    {
        return;
    }
    for (uint8_t i = 0; i < _focus_item_count; ++i)
    {
        if (_focus_items[i] == object)
        {
            return;
        }
    }

    _lv_obj_t *const focus_frame = frame ? frame : object;
    if (lv_obj_get_child_cnt(focus_frame) >= 2)
    {
        lv_obj_set_flex_align(focus_frame, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER,
                              LV_FLEX_ALIGN_CENTER);
    }
    if (propagate_focus)
    {
        ui_theme_apply_focus_row(focus_frame);
    }
    else
    {
        ui_theme_apply_focus_frame(focus_frame);
    }
    if (propagate_focus)
    {
        lv_obj_clear_flag(focus_frame, LV_OBJ_FLAG_CLICK_FOCUSABLE);
        lv_obj_add_event_cb(
            object, onFocusItemChanged,
            static_cast<lv_event_code_t>(LV_EVENT_FOCUSED | LV_EVENT_DEFOCUSED), focus_frame);
    }
    else
    {
        ui_theme_apply_focus_control(object);
    }
    if (focus_frame != object)
    {
        lv_obj_clear_flag(focus_frame, LV_OBJ_FLAG_CLICK_FOCUSABLE);
        lv_obj_add_event_cb(
            focus_frame,
            [](lv_event_t *e)
            {
                if (lv_event_get_code(e) != LV_EVENT_PRESSED)
                {
                    return;
                }
                _lv_obj_t *const control =
                    static_cast<_lv_obj_t *>(lv_event_get_user_data(e));
                if (!control || !lv_obj_is_valid(control) ||
                    lv_obj_has_state(control, LV_STATE_DISABLED) ||
                    lv_obj_has_flag(control, LV_OBJ_FLAG_HIDDEN))
                {
                    return;
                }
                if (lv_group_t *const group = lv_obj_get_group(control))
                {
                    lv_group_focus_obj(control);
                }
            },
            static_cast<lv_event_code_t>(LV_EVENT_PRESSED | LV_EVENT_PREPROCESS), object);
    }
    addFocusObject(object);
    lv_obj_add_flag(object, LV_OBJ_FLAG_CLICK_FOCUSABLE);
    if (propagate_focus && lv_obj_has_state(object, LV_STATE_FOCUSED))
    {
        lv_obj_add_state(focus_frame, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
        lv_obj_invalidate(focus_frame);
    }
    _focus_items[_focus_item_count++] = object;
}

void AbstractScreen::setFocusStateRecursive(_lv_obj_t *object, _lv_obj_t *source, bool focused)
{
    if (!object)
    {
        return;
    }
    if (object != source)
    {
        if (focused)
        {
            lv_obj_add_state(object, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
        }
        else
        {
            lv_obj_clear_state(object, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
        }
    }
    const uint32_t child_count = lv_obj_get_child_cnt(object);
    for (uint32_t index = 0; index < child_count; ++index)
    {
        setFocusStateRecursive(lv_obj_get_child(object, index), source, focused);
    }
}

bool AbstractScreen::isAvailableFocusItem(const _lv_obj_t *obj) const
{
    if (!obj || !lv_obj_is_valid(obj) || lv_obj_get_group(obj) != _focus_group ||
        lv_obj_has_state(obj, LV_STATE_DISABLED))
    {
        return false;
    }
    for (const _lv_obj_t *current = obj; current; current = lv_obj_get_parent(current))
    {
        if (lv_obj_has_flag(current, LV_OBJ_FLAG_HIDDEN))
            return false;
    }
    return true;
}

_lv_obj_t *AbstractScreen::firstAvailableFocusItem() const
{
    for (uint8_t i = 0; i < _focus_item_count; ++i)
    {
        if (isAvailableFocusItem(_focus_items[i]))
            return _focus_items[i];
    }
    return nullptr;
}

void AbstractScreen::onFocusItemChanged(lv_event_t *e)
{
    const lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_FOCUSED && code != LV_EVENT_DEFOCUSED)
        return;
    _lv_obj_t *const frame = static_cast<_lv_obj_t *>(lv_event_get_user_data(e));
    _lv_obj_t *const object = lv_event_get_target(e);
    if (!frame || !object)
        return;

    if (code == LV_EVENT_FOCUSED)
    {
        lv_obj_add_state(frame, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
        setFocusStateRecursive(frame, object, true);
        ui_theme_set_container_focused(frame, true);
    }
    else
    {
        lv_obj_clear_state(frame, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
        setFocusStateRecursive(frame, object, false);
        ui_theme_set_container_focused(frame, false);
    }
    lv_obj_invalidate(frame);
}

bool AbstractScreen::onKey(uint32_t key)
{
    if (key == LV_KEY_ESC)
    {
        return emit(UiEventType::NavOpen);
    }
    if (key == LV_KEY_ENTER && _focus_mode == FocusMode::Idle)
    {
        if (!_focus_activation_pending && firstAvailableFocusItem())
        {
            _focus_activation_pending = defer(activateFocusModeDeferred, this);
        }
        return true;
    }
    return false;
}
} // namespace heltec::meshcore::ui
