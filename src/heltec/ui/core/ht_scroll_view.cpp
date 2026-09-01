/**
 * @file    ht_scroll_view.cpp
 * @brief   Native LVGL scroll view with cyclic edge handling.
 */
#include "ht_scroll_view.hpp"

#include "ht_meta_data_internal.hpp"

namespace heltec::meshcore::ui
{
namespace
{

void scroll_view_key(lv_event_t *event)
{
    if (!event || lv_event_get_code(event) != LV_EVENT_KEY)
    {
        return;
    }

    lv_obj_t *const view = lv_event_get_current_target(event);
    if (!view)
    {
        return;
    }

    const uint32_t key = lv_event_get_key(event);
    const lv_coord_t current = lv_obj_get_scroll_y(view);
    const lv_coord_t remaining = lv_obj_get_scroll_bottom(view);
    if (remaining <= 0 && current <= 0)
    {
        return;
    }

    if ((key == LV_KEY_DOWN || key == LV_KEY_RIGHT) && remaining <= 0)
    {
        lv_obj_scroll_to_y(view, 0, LV_ANIM_OFF);
        lv_event_stop_processing(event);
    }
    else if ((key == LV_KEY_UP || key == LV_KEY_LEFT) && current <= 0)
    {
        lv_obj_scroll_to_y(view, remaining, LV_ANIM_OFF);
        lv_event_stop_processing(event);
    }
}

void scroll_view_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj)
{
    (void)class_p;
    if (!obj)
    {
        return;
    }

    lv_obj_add_flag(obj, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_WITH_ARROW);
    lv_obj_set_scroll_dir(obj, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_add_event_cb(
        obj,
        scroll_view_key,
        static_cast<lv_event_code_t>(LV_EVENT_KEY | LV_EVENT_PREPROCESS), nullptr);
}

} // namespace

const lv_obj_class_t ht_scroll_view_class = {
    &lv_obj_class,
    scroll_view_constructor,
    nullptr,
#if LV_USE_USER_DATA
    nullptr,
#endif
    nullptr,
    LV_SIZE_CONTENT,
    LV_SIZE_CONTENT,
    LV_OBJ_CLASS_EDITABLE_INHERIT,
    LV_OBJ_CLASS_GROUP_DEF_INHERIT,
    sizeof(ht_scroll_view_t),
};

lv_obj_t *ht_scroll_view_create(lv_obj_t *parent, void *user_data)
{
    return detail::create_object(&ht_scroll_view_class, parent, user_data);
}

} // namespace heltec::meshcore::ui
