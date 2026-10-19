/**
 * @file HtDropdown.cpp
 * @brief Dropdown class with viewport-aware popup placement.
 */
#include "HtDropdown.hpp"

namespace heltec::meshcore::ui {
namespace {

lv_obj_t *s_pending_realign = nullptr;

lv_obj_t *find_scroll_parent(lv_obj_t *obj) {
    for (lv_obj_t *parent = lv_obj_get_parent(obj); parent; parent = lv_obj_get_parent(parent)) {
        if (lv_obj_has_flag(parent, LV_OBJ_FLAG_SCROLLABLE)) {
            return parent;
        }
    }
    return nullptr;
}

void fit_list_to_viewport(void *user_data) {
    lv_obj_t *const dropdown = static_cast<lv_obj_t *>(user_data);
    if (!dropdown || !lv_obj_is_valid(dropdown) || !lv_dropdown_is_open(dropdown)) {
        return;
    }

    lv_obj_t *const list = lv_dropdown_get_list(dropdown);
    if (!list || !lv_obj_is_valid(list)) {
        return;
    }

    ht_dropdown_t *const instance = reinterpret_cast<ht_dropdown_t *>(dropdown);
    lv_obj_t *const viewport = instance->viewport ? instance->viewport
                                                  : lv_obj_get_screen(dropdown);
    if (!viewport) {
        return;
    }

    lv_obj_update_layout(viewport);
    lv_obj_update_layout(dropdown);
    lv_obj_update_layout(list);

    const lv_coord_t dropdown_width = lv_obj_get_width(dropdown);
    if (dropdown_width > 0) {
        lv_obj_set_width(list, dropdown_width);
    }
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_OFF);
    lv_obj_update_layout(list);

    lv_obj_t *const label = lv_obj_get_child(list, 0);
    const lv_coord_t border = lv_obj_get_style_border_width(list, LV_PART_MAIN);
    lv_coord_t content_height = label ? lv_obj_get_height(label) : lv_obj_get_height(list);
    content_height += lv_obj_get_style_pad_top(list, LV_PART_MAIN) +
                      lv_obj_get_style_pad_bottom(list, LV_PART_MAIN) + border * 2;

    lv_area_t viewport_area{};
    lv_area_t dropdown_area{};
    lv_obj_get_coords(viewport, &viewport_area);
    lv_obj_get_coords(dropdown, &dropdown_area);
    const lv_coord_t viewport_height = lv_area_get_height(&viewport_area);
    const lv_coord_t dropdown_height = lv_area_get_height(&dropdown_area);
    const lv_coord_t max_popup_height =
        viewport_height > dropdown_height ? viewport_height - dropdown_height : 0;
    if (content_height <= 0 || max_popup_height <= 0) {
        return;
    }

    const lv_coord_t desired_height = LV_MIN(content_height, max_popup_height);
    lv_coord_t available_above = LV_MAX(dropdown_area.y1 - viewport_area.y1, 0);
    lv_coord_t available_below = LV_MAX(viewport_area.y2 - dropdown_area.y2, 0);
    const bool prefer_above = lv_dropdown_get_dir(dropdown) == LV_DIR_TOP;
    lv_obj_t *const scroll_parent = instance->scroll_parent
                                        ? instance->scroll_parent
                                        : find_scroll_parent(dropdown);
    if (scroll_parent) {
        const lv_coord_t available = prefer_above ? available_above : available_below;
        lv_coord_t delta = desired_height - available;
        if (delta > 0) {
            const lv_coord_t capacity = prefer_above ? lv_obj_get_scroll_top(scroll_parent)
                                                     : lv_obj_get_scroll_bottom(scroll_parent);
            delta = LV_MIN(delta, capacity);
            if (delta > 0) {
                const lv_coord_t current = lv_obj_get_scroll_y(scroll_parent);
                lv_obj_scroll_to_y(scroll_parent,
                                   prefer_above ? current - delta : current + delta, LV_ANIM_OFF);
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
    if (!instance->scroll_parent) {
        if (prefer_above && available_above < desired_height && available_below > available_above) {
            place_above = false;
        } else if (!prefer_above && available_below < desired_height &&
                   available_above > available_below) {
            place_above = true;
        }
    }

    const lv_coord_t available_height = place_above ? available_above : available_below;
    if (available_height <= 0) {
        return;
    }

    lv_obj_set_height(list, LV_MIN(desired_height, available_height));
    lv_obj_align_to(list, dropdown, place_above ? LV_ALIGN_OUT_TOP_LEFT : LV_ALIGN_OUT_BOTTOM_LEFT,
                    0, 0);
    lv_obj_update_layout(list);
}

void realign_async(void *user_data) {
    fit_list_to_viewport(user_data);
    if (s_pending_realign == user_data) {
        s_pending_realign = nullptr;
    }
}

void dropdown_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj) {
    (void)class_p;
    ht_dropdown_t *const dropdown = reinterpret_cast<ht_dropdown_t *>(obj);
    dropdown->viewport = nullptr;
    dropdown->scroll_parent = nullptr;
}

void center_dropdown_text(lv_obj_t *dropdown) {
    if (!dropdown || lv_obj_get_style_height(dropdown, LV_PART_MAIN) == LV_SIZE_CONTENT) {
        return;
    }

    const lv_coord_t height = lv_obj_get_height(dropdown);
    const lv_coord_t border = lv_obj_get_style_border_width(dropdown, LV_PART_MAIN);
    const lv_font_t *const font = lv_obj_get_style_text_font(dropdown, LV_PART_MAIN);
    const lv_coord_t font_height = font ? lv_font_get_line_height(font) : 0;
    const lv_coord_t occupied_height = font_height + border * 2;
    const lv_coord_t free_height = height > occupied_height ? height - occupied_height : 0;
    const lv_coord_t pad_top = free_height / 2;

    lv_obj_set_style_pad_top(dropdown, pad_top, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(dropdown, free_height - pad_top, LV_PART_MAIN);
}

void dropdown_event(const lv_obj_class_t *class_p, lv_event_t *event) {
    lv_obj_t *const dropdown = lv_event_get_target(event);
    const lv_event_code_t code = lv_event_get_code(event);
    const bool handle_key = code == LV_EVENT_KEY && dropdown && lv_dropdown_is_open(dropdown);
    const uint32_t key = handle_key ? lv_event_get_key(event) : 0;

    if (handle_key) {
        const bool move_next = key == LV_KEY_DOWN || key == LV_KEY_RIGHT || key == LV_KEY_NEXT;
        const bool move_previous = key == LV_KEY_UP || key == LV_KEY_LEFT || key == LV_KEY_PREV;
        if (move_next || move_previous) {
            if (lv_obj_event_base(&lv_dropdown_class, event) != LV_RES_OK) {
                return;
            }
            ht_dropdown_cycle(dropdown, key);
            return;
        }
    }

    if (lv_obj_event_base(class_p, event) != LV_RES_OK) {
        return;
    }
    if (code == LV_EVENT_SIZE_CHANGED) {
        center_dropdown_text(dropdown);
    } else if (code == LV_EVENT_READY) {
        ht_dropdown_realign(dropdown);
    }
}

} // namespace

const lv_obj_class_t ht_dropdown_class = {
    &lv_dropdown_class,
    dropdown_constructor,
    nullptr,
#if LV_USE_USER_DATA
    nullptr,
#endif
    dropdown_event,
    LV_DPI_DEF,
    LV_SIZE_CONTENT,
    LV_OBJ_CLASS_EDITABLE_TRUE,
    LV_OBJ_CLASS_GROUP_DEF_TRUE,
    sizeof(ht_dropdown_t),
};

lv_obj_t *ht_dropdown_create(lv_obj_t *parent) {
    lv_obj_t *const obj = lv_obj_class_create_obj(&ht_dropdown_class, parent);
    if (!obj) {
        return nullptr;
    }
    lv_obj_class_init_obj(obj);
    return obj;
}

void ht_dropdown_set_viewport(lv_obj_t *dropdown, lv_obj_t *viewport) {
    if (!dropdown || !lv_obj_has_class(dropdown, &ht_dropdown_class)) {
        return;
    }
    ht_dropdown_t *const instance = reinterpret_cast<ht_dropdown_t *>(dropdown);
    instance->viewport = viewport;
}

void ht_dropdown_set_scroll_parent(lv_obj_t *dropdown, lv_obj_t *scroll_parent) {
    if (!dropdown || !lv_obj_has_class(dropdown, &ht_dropdown_class)) {
        return;
    }
    ht_dropdown_t *const instance = reinterpret_cast<ht_dropdown_t *>(dropdown);
    instance->scroll_parent = scroll_parent;
}

bool ht_dropdown_cycle(lv_obj_t *dropdown, uint32_t key) {
    if (!dropdown || !lv_obj_is_valid(dropdown) || !lv_dropdown_is_open(dropdown)) {
        return false;
    }

    const uint16_t count = lv_dropdown_get_option_cnt(dropdown);
    if (count <= 1) {
        return false;
    }

    const uint16_t selected = lv_dropdown_get_selected(dropdown);
    uint16_t next = selected;
    if (key == LV_KEY_DOWN || key == LV_KEY_RIGHT || key == LV_KEY_NEXT) {
        next = static_cast<uint16_t>((selected + 1U) % count);
    } else if (key == LV_KEY_UP || key == LV_KEY_LEFT || key == LV_KEY_PREV) {
        next = selected == 0 ? static_cast<uint16_t>(count - 1U)
                             : static_cast<uint16_t>(selected - 1U);
    } else {
        return false;
    }

    lv_dropdown_set_selected(dropdown, next);
    return true;
}

void ht_dropdown_realign(lv_obj_t *dropdown) {
    if (!dropdown || !lv_obj_is_valid(dropdown)) {
        return;
    }
    if (s_pending_realign) {
        return;
    }
    s_pending_realign = dropdown;
    if (lv_async_call(realign_async, dropdown) != LV_RES_OK) {
        fit_list_to_viewport(dropdown);
        s_pending_realign = nullptr;
    }
}

} // namespace heltec::meshcore::ui
