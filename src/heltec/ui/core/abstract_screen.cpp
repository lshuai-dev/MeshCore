#include "abstract_screen.hpp"

#include "ht_meta_data.hpp"
#include "ui/core/ui_deferred_queue.hpp"
#include "ui/core/ht_event.h"
#include "ui/core/ht_container.hpp"
#include "ui/app/ui_theme.hpp"

namespace heltec::meshcore::ui
{

AbstractScreen::AbstractScreen(biz::IBizFacade &biz, const char *title, const lv_img_dsc_t *icon,
                               bool root_scroll_focus)
    : UiSurface(biz), _title(title), _icon(icon), _root_scroll_focus(root_scroll_focus)
{
}

_lv_obj_t *AbstractScreen::createRoot(_lv_obj_t *parent)
{
    return ht_obj_create(parent);
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
        lv_group_del(_focus_group);
        _focus_group = nullptr;
        lv_obj_del(_root);
        _root = nullptr;
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
        lv_obj_t *const previous = _active_group ? lv_group_get_focused(_active_group) : nullptr;
        if (previous && previous != focus && lv_obj_is_valid(previous))
        {
            lv_obj_clear_state(previous, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
            lv_obj_invalidate(previous);
        }
        for (uint8_t index = 0; index < _focus_item_count; ++index)
        {
            _lv_obj_t *const object = _focus_items[index].object;
            _lv_obj_t *const frame = _focus_items[index].frame;
            if (object && lv_obj_is_valid(object))
            {
                lv_obj_clear_state(object, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
                lv_obj_invalidate(object);
            }
            if (frame && lv_obj_check_type(frame, &ht_container_class))
            {
                ht_container_set_focused(frame, false);
            }
        }
    }

    _focus_mode = mode;
    _active_group = mode == FocusMode::Idle ? _idle_group : _focus_group;
    lv_group_focus_obj(focus);
    if (mode == FocusMode::Active)
    {
        lv_obj_clear_state(_root, LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
    }
    (void)emit(EventType::REBIND_INPUT);
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
        if (_focus_items[i].object == object)
        {
            return;
        }
    }

    _lv_obj_t *const focus_frame = frame ? frame : object;
    if (propagate_focus)
    {
        lv_obj_clear_flag(focus_frame, LV_OBJ_FLAG_CLICK_FOCUSABLE);
        lv_obj_add_event_cb(object, onFocusItemChanged, LV_EVENT_FOCUSED, focus_frame);
        lv_obj_add_event_cb(object, onFocusItemChanged, LV_EVENT_DEFOCUSED, focus_frame);
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
    _focus_items[_focus_item_count++] = { object, focus_frame };
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
        if (isAvailableFocusItem(_focus_items[i].object))
            return _focus_items[i].object;
    }
    return nullptr;
}

bool AbstractScreen::scrollRootLoop(bool forward)
{
    if (!_root || !_root_scroll_focus ||
        !lv_obj_has_flag(_root, LV_OBJ_FLAG_SCROLLABLE))
    {
        return false;
    }

    const lv_coord_t current = lv_obj_get_scroll_y(_root);
    const lv_coord_t remaining = lv_obj_get_scroll_bottom(_root);
    const lv_coord_t step = ui_settings_row_min_height();
    if (step <= 0 || (current <= 0 && remaining <= 0))
    {
        return false;
    }

    if (forward)
    {
        if (remaining <= 0)
        {
            lv_obj_scroll_to_y(_root, 0, LV_ANIM_OFF);
        }
        else
        {
            lv_obj_scroll_by_bounded(_root, 0, -step, LV_ANIM_OFF);
        }
    }
    else if (current <= 0)
    {
        lv_obj_scroll_to_y(_root, remaining, LV_ANIM_OFF);
    }
    else
    {
        lv_obj_scroll_by_bounded(_root, 0, step, LV_ANIM_OFF);
    }
    return true;
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
        if (lv_obj_check_type(frame, &ht_container_class))
        {
            ht_container_set_focused(frame, true);
        }
    }
    else
    {
        if (lv_obj_check_type(frame, &ht_container_class))
        {
            ht_container_set_focused(frame, false);
        }
    }
    lv_obj_invalidate(frame);
}

bool AbstractScreen::onKey(uint32_t key)
{
    if (_focus_mode == FocusMode::Active && _active_group)
    {
        if (key == LV_KEY_DOWN || key == LV_KEY_RIGHT || key == LV_KEY_NEXT)
        {
            lv_group_set_wrap(_active_group, true);
            lv_group_focus_next(_active_group);
            return true;
        }
        if (key == LV_KEY_UP || key == LV_KEY_LEFT || key == LV_KEY_PREV)
        {
            lv_group_set_wrap(_active_group, true);
            lv_group_focus_prev(_active_group);
            return true;
        }
    }
    if (key == LV_KEY_ESC)
    {
        if (_focus_mode == FocusMode::Active)
        {
            activateFocusMode(FocusMode::Idle, _root);
            return true;
        }
        return emit(EventType::NAV_OPEN);
    }
    if (key == LV_KEY_ENTER && _focus_mode == FocusMode::Idle)
    {
        _lv_obj_t *const first = firstAvailableFocusItem();
        if (first && !_focus_activation_pending)
        {
            _focus_activation_pending = defer(activateFocusModeDeferred, this);
        }
        return true;
    }
    if (_focus_mode == FocusMode::Idle)
    {
        if (key == LV_KEY_NEXT || key == LV_KEY_DOWN)
        {
            (void)scrollRootLoop(true);
            return true;
        }
        if (key == LV_KEY_PREV || key == LV_KEY_UP)
        {
            (void)scrollRootLoop(false);
            return true;
        }
    }
    return false;
}
} // namespace heltec::meshcore::ui
