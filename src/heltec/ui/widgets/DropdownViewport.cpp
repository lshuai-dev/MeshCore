/**
 * @file DropdownViewport.cpp
 * @brief Viewport placement helpers for native LVGL dropdowns.
 */
#include "DropdownViewport.hpp"

namespace Heltec::MeshCore::Ui
{
namespace
{

lv_obj_t *s_pending_realign = nullptr;
lv_obj_t *s_pending_viewport = nullptr;

lv_obj_t *findScrollParent(lv_obj_t *object)
{
    for (lv_obj_t *parent = lv_obj_get_parent(object); parent != nullptr;
         parent = lv_obj_get_parent(parent))
    {
        if (lv_obj_has_flag(parent, LV_OBJ_FLAG_SCROLLABLE))
        {
            return parent;
        }
    }
    return nullptr;
}

void fitListToViewport(lv_obj_t *dropdown, lv_obj_t *viewport)
{
    if (dropdown == nullptr || !lv_obj_is_valid(dropdown) || !lv_dropdown_is_open(dropdown))
    {
        return;
    }

    lv_obj_t *const list = lv_dropdown_get_list(dropdown);
    if (list == nullptr || viewport == nullptr || !lv_obj_is_valid(list) ||
        !lv_obj_is_valid(viewport))
    {
        return;
    }

    lv_obj_update_layout(viewport);
    lv_obj_update_layout(dropdown);
    lv_obj_update_layout(list);
    const lv_coord_t dropdown_width = lv_obj_get_width(dropdown);
    if (dropdown_width > 0)
    {
        lv_obj_set_width(list, dropdown_width);
    }
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_OFF);
    lv_obj_update_layout(list);

    lv_obj_t *const label = lv_obj_get_child(list, 0);
    const lv_coord_t border = lv_obj_get_style_border_width(list, LV_PART_MAIN);
    lv_coord_t content_height = label != nullptr ? lv_obj_get_height(label)
                                                  : lv_obj_get_height(list);
    content_height += lv_obj_get_style_pad_top(list, LV_PART_MAIN) +
                      lv_obj_get_style_pad_bottom(list, LV_PART_MAIN) + border * 2;

    lv_area_t viewport_area = {};
    lv_area_t dropdown_area = {};
    lv_obj_get_coords(viewport, &viewport_area);
    lv_obj_get_coords(dropdown, &dropdown_area);
    const lv_coord_t viewport_height = lv_area_get_height(&viewport_area);
    const lv_coord_t dropdown_height = lv_area_get_height(&dropdown_area);
    const lv_coord_t max_popup_height =
        viewport_height > dropdown_height ? viewport_height - dropdown_height : 0;
    if (content_height <= 0 || max_popup_height <= 0)
    {
        return;
    }

    const lv_coord_t desired_height = LV_MIN(content_height, max_popup_height);
    lv_coord_t available_above = LV_MAX(dropdown_area.y1 - viewport_area.y1, 0);
    lv_coord_t available_below = LV_MAX(viewport_area.y2 - dropdown_area.y2, 0);
    const bool prefer_above = lv_dropdown_get_dir(dropdown) == LV_DIR_TOP;
    lv_obj_t *const scroll_parent = findScrollParent(dropdown);
    if (scroll_parent != nullptr)
    {
        const lv_coord_t available = prefer_above ? available_above : available_below;
        lv_coord_t delta = desired_height - available;
        if (delta > 0)
        {
            const lv_coord_t capacity = prefer_above ? lv_obj_get_scroll_top(scroll_parent)
                                                     : lv_obj_get_scroll_bottom(scroll_parent);
            delta = LV_MIN(delta, capacity);
            if (delta > 0)
            {
                const lv_coord_t current = lv_obj_get_scroll_y(scroll_parent);
                lv_obj_scroll_to_y(scroll_parent,
                                   prefer_above ? current - delta : current + delta,
                                   LV_ANIM_OFF);
                lv_obj_update_layout(scroll_parent);
                lv_obj_update_layout(viewport);
                lv_obj_update_layout(dropdown);
                lv_obj_get_coords(viewport, &viewport_area);
                lv_obj_get_coords(dropdown, &dropdown_area);
                available_above = LV_MAX(dropdown_area.y1 - viewport_area.y1, 0);
                available_below = LV_MAX(viewport_area.y2 - dropdown_area.y2, 0);
            }
        }
    }

    bool place_above = prefer_above;
    if (prefer_above && available_above < desired_height && available_below > available_above)
    {
        place_above = false;
    }
    else if (!prefer_above && available_below < desired_height &&
             available_above > available_below)
    {
        place_above = true;
    }
    const lv_coord_t available_height = place_above ? available_above : available_below;
    if (available_height <= 0)
    {
        return;
    }

    lv_obj_set_height(list, LV_MIN(desired_height, available_height));
    lv_obj_align_to(list, dropdown,
                    place_above ? LV_ALIGN_OUT_TOP_LEFT : LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    lv_obj_update_layout(list);
}

void realignAsync(void *user_data)
{
    fitListToViewport(static_cast<lv_obj_t *>(user_data), s_pending_viewport);
    if (s_pending_realign == user_data)
    {
        s_pending_realign = nullptr;
        s_pending_viewport = nullptr;
    }
}

} // namespace

void dropdown_viewport_realign(lv_obj_t *dropdown, lv_obj_t *viewport)
{
    if (dropdown == nullptr || viewport == nullptr || !lv_obj_is_valid(dropdown) ||
        !lv_obj_is_valid(viewport) || s_pending_realign != nullptr)
    {
        return;
    }
    s_pending_realign = dropdown;
    s_pending_viewport = viewport;
    if (lv_async_call(realignAsync, dropdown) != LV_RES_OK)
    {
        fitListToViewport(dropdown, viewport);
        s_pending_realign = nullptr;
        s_pending_viewport = nullptr;
    }
}

} // namespace Heltec::MeshCore::Ui
