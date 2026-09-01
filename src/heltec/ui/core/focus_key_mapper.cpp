#include "focus_key_mapper.hpp"

#include "ht_selector_view.hpp"
#include "ht_scroll_view.hpp"
#include "ui/navigation/ui_navigator.hpp"

#include <lvgl.h>

namespace heltec::meshcore::ui
{
uint32_t FocusKeyMapper::translate(uint32_t lv_key)
{
    return translateForGroup(lv_group_get_default(), lv_key);
}

uint32_t FocusKeyMapper::translateForGroup(lv_group_t *group, uint32_t lv_key)
{
    if (!group)
        return lv_key;
    return translateForObject(lv_group_get_focused(group), lv_key);
}

uint32_t FocusKeyMapper::translateForObject(lv_obj_t *obj, uint32_t lv_key)
{
    if (!obj)
        return lv_key;

    /* Navigator surfaces consume NEXT/PREV in their LV_EVENT_KEY handler.
       Convert them to directional keys so LVGL does not consume them as
       group-focus traversal before the surface receives the event. */
    if (lv_obj_has_class(obj, &ht_navigator_class))
    {
        if (lv_key == LV_KEY_PREV)
            return LV_KEY_LEFT;
        if (lv_key == LV_KEY_NEXT)
            return LV_KEY_RIGHT;
        return lv_key;
    }

#if LV_USE_ROLLER
    if (lv_obj_check_type(obj, &lv_roller_class))
    {
        if (lv_key == LV_KEY_NEXT || lv_key == LV_KEY_RIGHT)
            return LV_KEY_DOWN;
        if (lv_key == LV_KEY_PREV || lv_key == LV_KEY_LEFT)
            return LV_KEY_UP;
        return lv_key;
    }
#endif

#if LV_USE_KEYBOARD
    if (lv_obj_has_class(obj, &lv_keyboard_class))
    {
        if (lv_keyboard_get_mode(obj) == LV_KEYBOARD_MODE_USER_1)
        {
            if (lv_key == LV_KEY_NEXT)
                return LV_KEY_RIGHT;
            if (lv_key == LV_KEY_PREV)
                return LV_KEY_LEFT;
            if (lv_key == LV_KEY_RIGHT)
                return LV_KEY_DOWN;
            if (lv_key == LV_KEY_LEFT)
                return LV_KEY_UP;
            return lv_key;
        }
        if (lv_key == LV_KEY_NEXT || lv_key == LV_KEY_RIGHT)
            return LV_KEY_RIGHT;
        if (lv_key == LV_KEY_PREV || lv_key == LV_KEY_LEFT)
            return LV_KEY_LEFT;
        return lv_key;
    }
#endif

#if LV_USE_DROPDOWN
    if (lv_obj_check_type(obj, &lv_dropdown_class) && lv_dropdown_is_open(obj))
    {
        if (lv_key == LV_KEY_NEXT || lv_key == LV_KEY_RIGHT)
            return LV_KEY_DOWN;
        if (lv_key == LV_KEY_PREV || lv_key == LV_KEY_LEFT)
            return LV_KEY_UP;
        return lv_key;
    }
#endif

    if (lv_obj_check_type(obj, &ht_scroll_view_class))
    {
        if (lv_key == LV_KEY_NEXT)
            return LV_KEY_DOWN;
        if (lv_key == LV_KEY_PREV)
            return LV_KEY_UP;
    }

    if (lv_obj_has_class(obj, &ht_selector_view_class))
    {
        if (lv_key == LV_KEY_NEXT)
            return LV_KEY_DOWN;
        if (lv_key == LV_KEY_PREV)
            return LV_KEY_UP;
    }

    return lv_key;
}

} // namespace heltec::meshcore::ui
