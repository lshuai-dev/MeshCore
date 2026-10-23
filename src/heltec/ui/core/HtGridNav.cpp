#include "HtGridNav.hpp"

namespace Heltec::MeshCore::Ui
{
namespace
{

lv_group_t *s_native_control_group = nullptr;

struct FindContext
{
    const lv_obj_t *current = nullptr;
    lv_obj_t *result = nullptr;
    lv_obj_t *last = nullptr;
    bool found_current = false;
};

} // namespace

void HtGridNav::bindRoot(lv_obj_t *root)
{
    enterBrowse();
    _root = root;
    (void)nativeControlGroup();
}

void HtGridNav::add(lv_obj_t *object)
{
    if (object == nullptr)
    {
        return;
    }
    lv_obj_add_flag(object,
                    kNavigableFlag | LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CLICK_FOCUSABLE);
    if (object == _root)
    {
        lv_obj_clear_flag(object, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    }
    else
    {
        lv_obj_add_flag(object, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    }
    lv_group_t *const group = nativeControlGroup();
    if (group != nullptr && lv_obj_get_group(object) != group)
    {
        lv_group_add_obj(group, object);
    }
}

void HtGridNav::remove(lv_obj_t *object)
{
    if (object == nullptr)
    {
        return;
    }
    if (_selected == object)
    {
        select(nullptr);
    }
    lv_obj_clear_flag(object, kNavigableFlag);
}

void HtGridNav::clear()
{
    enterBrowse();
    if (_root != nullptr)
    {
        lv_obj_tree_walk(_root, clearVisitor, nullptr);
    }
}

bool HtGridNav::enterControls()
{
    if (_root == nullptr)
    {
        return false;
    }
    lv_obj_t *first = nullptr;
    lv_obj_tree_walk(_root, firstVisitor, &first);
    if (first == nullptr)
    {
        return false;
    }
    _mode = Mode::Controls;
    return select(first);
}

void HtGridNav::enterBrowse()
{
    releaseCapture();
    (void)select(nullptr);
    _mode = Mode::Browse;
}

void HtGridNav::restoreFocus()
{
    if (_mode == Mode::Controls && _selected != nullptr && available(_selected))
    {
        (void)select(_selected);
    }
}

bool HtGridNav::handleKey(uint32_t key)
{
    if (_mode != Mode::Controls || _selected == nullptr || _frozen)
    {
        return false;
    }

    if (_captured != nullptr)
    {
        if (key == LV_KEY_ENTER)
        {
            const bool persistent = _capture_mode == CaptureMode::Persistent;
            const bool handled = activateSelected();
            if (!persistent)
            {
                releaseCapture();
            }
            return handled;
        }
        const bool handled = sendKey(key);
        if (key == LV_KEY_ESC)
        {
            releaseCapture();
        }
        return handled;
    }

    if (key == LV_KEY_DOWN || key == LV_KEY_RIGHT || key == LV_KEY_NEXT)
    {
        focusNext();
        return true;
    }
    if (key == LV_KEY_UP || key == LV_KEY_LEFT || key == LV_KEY_PREV)
    {
        focusPrevious();
        return true;
    }
    if (key == LV_KEY_ENTER)
    {
        if (lv_obj_is_editable(_selected))
        {
            _captured = _selected;
            lv_group_t *const group = nativeControlGroup();
            if (group != nullptr)
            {
                lv_group_set_editing(group, true);
            }
            lv_obj_add_state(_selected, LV_STATE_EDITED);
        }
        return activateSelected();
    }
    return false;
}

bool HtGridNav::focus(lv_obj_t *object)
{
    if (!contains(object) || !available(object))
    {
        return false;
    }
    _mode = Mode::Controls;
    return select(object);
}

void HtGridNav::focusNext()
{
    if (_root == nullptr)
    {
        return;
    }
    FindContext context;
    context.current = _selected;
    lv_obj_tree_walk(_root, nextVisitor, &context);
    if (context.result == nullptr && _wrap)
    {
        lv_obj_tree_walk(_root, firstVisitor, &context.result);
    }
    if (context.result != nullptr)
    {
        (void)select(context.result);
    }
}

void HtGridNav::focusPrevious()
{
    if (_root == nullptr)
    {
        return;
    }
    FindContext context;
    context.current = _selected;
    lv_obj_tree_walk(_root, previousVisitor, &context);
    if (context.result == nullptr && _wrap)
    {
        context.current = nullptr;
        context.last = nullptr;
        lv_obj_tree_walk(_root, previousVisitor, &context);
        context.result = context.last;
    }
    if (context.result != nullptr)
    {
        (void)select(context.result);
    }
}

void HtGridNav::setWrap(bool enabled)
{
    _wrap = enabled;
}

void HtGridNav::setCaptureMode(CaptureMode mode)
{
    _capture_mode = mode;
}

void HtGridNav::setHorizontalNavigation(bool enabled)
{
    _horizontal_navigation = enabled;
}

void HtGridNav::setEditing(bool enabled)
{
    if (!enabled)
    {
        releaseCapture();
    }
    else if (_selected != nullptr)
    {
        _captured = _selected;
        lv_group_t *const group = nativeControlGroup();
        if (group != nullptr)
        {
            lv_group_set_editing(group, true);
        }
        lv_obj_add_state(_selected, LV_STATE_EDITED);
    }
}

void HtGridNav::freeze(bool enabled)
{
    _frozen = enabled;
}

void HtGridNav::setFocusChangedCallback(FocusChangedFn callback, void *user_data)
{
    _focus_changed = callback;
    _focus_user_data = user_data;
}

bool HtGridNav::contains(const lv_obj_t *object) const
{
    if (_root == nullptr || object == nullptr || !lv_obj_has_flag(object, kNavigableFlag))
    {
        return false;
    }
    for (const lv_obj_t *current = object; current != nullptr;
         current = lv_obj_get_parent(current))
    {
        if (current == _root)
        {
            return true;
        }
    }
    return false;
}

bool HtGridNav::isEditing() const
{
    return _captured != nullptr;
}

bool HtGridNav::isFrozen() const
{
    return _frozen;
}

uint16_t HtGridNav::count() const
{
    uint16_t result = 0;
    if (_root != nullptr)
    {
        lv_obj_tree_walk(_root, countVisitor, &result);
    }
    return result;
}

HtGridNav::Mode HtGridNav::mode() const
{
    return _mode;
}

lv_obj_t *HtGridNav::focused() const
{
    return _selected;
}

bool HtGridNav::available(const lv_obj_t *object)
{
    if (object == nullptr || !lv_obj_is_valid(object) ||
        lv_obj_has_state(object, LV_STATE_DISABLED))
    {
        return false;
    }
    for (const lv_obj_t *parent = object; parent != nullptr; parent = lv_obj_get_parent(parent))
    {
        if (lv_obj_has_flag(parent, LV_OBJ_FLAG_HIDDEN))
        {
            return false;
        }
    }
    return true;
}

lv_group_t *HtGridNav::nativeControlGroup()
{
    if (s_native_control_group == nullptr)
    {
        s_native_control_group = lv_group_create();
        if (s_native_control_group != nullptr)
        {
            lv_group_set_wrap(s_native_control_group, false);
        }
    }
    return s_native_control_group;
}

lv_obj_tree_walk_res_t HtGridNav::clearVisitor(lv_obj_t *object, void *user_data)
{
    (void)user_data;
    if (object != nullptr)
    {
        lv_obj_clear_flag(object, kNavigableFlag);
    }
    return LV_OBJ_TREE_WALK_NEXT;
}

lv_obj_tree_walk_res_t HtGridNav::countVisitor(lv_obj_t *object, void *user_data)
{
    auto *count = static_cast<uint16_t *>(user_data);
    if (count != nullptr && object != nullptr && lv_obj_has_flag(object, kNavigableFlag) &&
        available(object))
    {
        ++(*count);
    }
    return LV_OBJ_TREE_WALK_NEXT;
}

lv_obj_tree_walk_res_t HtGridNav::firstVisitor(lv_obj_t *object, void *user_data)
{
    auto **result = static_cast<lv_obj_t **>(user_data);
    if (result != nullptr && *result == nullptr && object != nullptr &&
        lv_obj_has_flag(object, kNavigableFlag) && available(object))
    {
        *result = object;
        return LV_OBJ_TREE_WALK_END;
    }
    return LV_OBJ_TREE_WALK_NEXT;
}

lv_obj_tree_walk_res_t HtGridNav::nextVisitor(lv_obj_t *object, void *user_data)
{
    auto *context = static_cast<FindContext *>(user_data);
    if (context == nullptr || object == nullptr || !lv_obj_has_flag(object, kNavigableFlag) ||
        !available(object))
    {
        return LV_OBJ_TREE_WALK_NEXT;
    }
    if (context->found_current)
    {
        context->result = object;
        return LV_OBJ_TREE_WALK_END;
    }
    if (object == context->current)
    {
        context->found_current = true;
    }
    return LV_OBJ_TREE_WALK_NEXT;
}

lv_obj_tree_walk_res_t HtGridNav::previousVisitor(lv_obj_t *object, void *user_data)
{
    auto *context = static_cast<FindContext *>(user_data);
    if (context == nullptr || object == nullptr || !lv_obj_has_flag(object, kNavigableFlag) ||
        !available(object))
    {
        return LV_OBJ_TREE_WALK_NEXT;
    }
    if (context->current != nullptr && object == context->current)
    {
        context->result = context->last;
        return LV_OBJ_TREE_WALK_END;
    }
    context->last = object;
    return LV_OBJ_TREE_WALK_NEXT;
}

bool HtGridNav::select(lv_obj_t *object)
{
    lv_group_t *const group = nativeControlGroup();
    if (_selected == object && object != nullptr && group != nullptr &&
        lv_obj_get_group(object) == group && lv_obj_has_state(object, LV_STATE_FOCUSED))
    {
        return true;
    }
    releaseCapture();
    if (object == nullptr && _selected != nullptr && lv_obj_is_valid(_selected))
    {
        (void)lv_event_send(_selected, LV_EVENT_DEFOCUSED, nullptr);
    }
    _selected = object;
    if (_selected != nullptr)
    {
        if (group == nullptr)
        {
            _selected = nullptr;
            return false;
        }
        if (lv_obj_get_group(_selected) != group)
        {
            lv_group_add_obj(group, _selected);
        }
        lv_group_set_editing(group, false);
        if (lv_group_get_focused(group) == _selected)
        {
            (void)lv_event_send(_selected, LV_EVENT_FOCUSED, nullptr);
        }
        else
        {
            lv_group_focus_obj(_selected);
        }
    }
    if (_focus_changed != nullptr)
    {
        _focus_changed(_selected, _focus_user_data);
    }
    return _selected != nullptr;
}

bool HtGridNav::sendKey(uint32_t key)
{
    if (_selected == nullptr || !available(_selected))
    {
        return false;
    }

    uint32_t mapped_key = key;
    if (_horizontal_navigation)
    {
        if (key == LV_KEY_UP || key == LV_KEY_PREV)
        {
            mapped_key = LV_KEY_LEFT;
        }
        else if (key == LV_KEY_DOWN || key == LV_KEY_NEXT)
        {
            mapped_key = LV_KEY_RIGHT;
        }
    }

    return lv_event_send(_selected, LV_EVENT_KEY, &mapped_key) == LV_RES_OK;
}

bool HtGridNav::activateSelected()
{
    if (_selected == nullptr || !available(_selected))
    {
        return false;
    }
    lv_indev_t *const indev = lv_indev_get_act();
    if (lv_event_send(_selected, LV_EVENT_PRESSED, indev) != LV_RES_OK)
    {
        return false;
    }
    if (lv_event_send(_selected, LV_EVENT_RELEASED, indev) != LV_RES_OK)
    {
        return false;
    }
    return lv_event_send(_selected, LV_EVENT_CLICKED, indev) == LV_RES_OK;
}

void HtGridNav::releaseCapture()
{
    if (_captured != nullptr && lv_obj_is_valid(_captured))
    {
        lv_obj_clear_state(_captured, LV_STATE_EDITED);
    }
    lv_group_t *const group = nativeControlGroup();
    if (group != nullptr)
    {
        lv_group_set_editing(group, false);
    }
    _captured = nullptr;
}

} // namespace Heltec::MeshCore::Ui
