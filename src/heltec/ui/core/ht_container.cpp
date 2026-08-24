/**
 * @file    ht_container.cpp
 * @brief   Common UI container type and layout factory.
 */
#include "ht_container.hpp"

#include "ht_meta_data_internal.hpp"
#include "ui/app/ui_theme.hpp"

namespace heltec::meshcore::ui
{
namespace
{

void set_container_children_focused(lv_obj_t *object, bool focused)
{
    if (!object)
    {
        return;
    }

    const lv_state_t states = LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY;
    const uint32_t child_count = lv_obj_get_child_cnt(object);
    for (uint32_t index = 0; index < child_count; ++index)
    {
        lv_obj_t *const child = lv_obj_get_child(object, index);
        if (!child)
        {
            continue;
        }
        const bool child_focused = (lv_obj_get_state(child) & states) != 0;
        if (focused && !child_focused)
        {
            lv_obj_add_state(child, states);
        }
        else if (!focused && child_focused)
        {
            lv_obj_clear_state(child, states);
        }
        set_container_children_focused(child, focused);
    }
}

void container_state_changed(lv_event_t *event)
{
    if (!event)
    {
        return;
    }

    const lv_event_code_t code = lv_event_get_code(event);
    if (code != LV_EVENT_FOCUSED && code != LV_EVENT_DEFOCUSED)
    {
        return;
    }

    lv_obj_t *const container = lv_event_get_current_target(event);
    lv_obj_t *const target = lv_event_get_target(event);
    if (!container || target != container)
    {
        return;
    }

    const bool focused = code == LV_EVENT_FOCUSED;
    set_container_children_focused(container, focused);
    ui_theme_set_focus_text_scroll(container, focused);
}

void container_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj)
{
    (void)class_p;
    if (!obj)
    {
        return;
    }

    lv_obj_set_width(obj, lv_pct(100));
    const lv_coord_t row_height = ui_layout_metrics().row_height;
    lv_obj_set_height(obj, row_height > 0 ? row_height : LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(obj, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(obj, 1, LV_PART_MAIN);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_event_cb(obj, container_state_changed, LV_EVENT_FOCUSED, nullptr);
    lv_obj_add_event_cb(obj, container_state_changed, LV_EVENT_DEFOCUSED, nullptr);
}

} // namespace

void ht_container_set_focused(lv_obj_t *container, bool focused)
{
    if (!container || !lv_obj_check_type(container, &ht_container_class))
    {
        return;
    }

    const lv_state_t states = LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY;
    const bool container_focused = (lv_obj_get_state(container) & states) != 0;
    if (focused && !container_focused)
    {
        lv_obj_add_state(container, states);
    }
    else if (!focused && container_focused)
    {
        lv_obj_clear_state(container, states);
    }
    set_container_children_focused(container, focused);
    ui_theme_set_focus_text_scroll(container, focused);
}

const lv_obj_class_t ht_container_class = {
    &lv_obj_class,
    container_constructor,
    nullptr,
#if LV_USE_USER_DATA
    nullptr,
#endif
    nullptr,
    LV_SIZE_CONTENT,
    LV_SIZE_CONTENT,
    LV_OBJ_CLASS_EDITABLE_INHERIT,
    LV_OBJ_CLASS_GROUP_DEF_INHERIT,
    sizeof(ht_container_t),
};

lv_obj_t *ht_container_create(lv_obj_t *parent, uint16_t id, void *user_data)
{
    return detail::create_object(&ht_container_class, parent, id, user_data);
}

} // namespace heltec::meshcore::ui
