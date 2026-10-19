/**
 * @file HtTextarea.cpp
 * @brief Shared LVGL textarea behavior.
 */
#include "HtTextarea.hpp"

namespace heltec::meshcore::ui {
namespace {

void center_single_line_text(lv_obj_t *textarea) {
    if (!textarea || !lv_textarea_get_one_line(textarea) ||
        lv_obj_get_style_height(textarea, LV_PART_MAIN) == LV_SIZE_CONTENT) {
        return;
    }

    const lv_coord_t height = lv_obj_get_height(textarea);
    const lv_coord_t border = lv_obj_get_style_border_width(textarea, LV_PART_MAIN);
    const lv_font_t *const font = lv_obj_get_style_text_font(textarea, LV_PART_MAIN);
    const lv_coord_t font_height = font ? lv_font_get_line_height(font) : 0;
    const lv_coord_t occupied_height = font_height + border * 2;
    const lv_coord_t free_height = height > occupied_height ? height - occupied_height : 0;
    const lv_coord_t pad_top = free_height / 2;

    lv_obj_set_style_pad_top(textarea, pad_top, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(textarea, free_height - pad_top, LV_PART_MAIN);
}

void textarea_event(const lv_obj_class_t *class_p, lv_event_t *event) {
    if (lv_obj_event_base(class_p, event) != LV_RES_OK ||
        lv_event_get_code(event) != LV_EVENT_SIZE_CHANGED) {
        return;
    }

    center_single_line_text(lv_event_get_target(event));
}

} // namespace

const lv_obj_class_t ht_textarea_class = {
    &lv_textarea_class,
    nullptr,
    nullptr,
#if LV_USE_USER_DATA
    nullptr,
#endif
    textarea_event,
    LV_DPI_DEF,
    LV_SIZE_CONTENT,
    LV_OBJ_CLASS_EDITABLE_TRUE,
    LV_OBJ_CLASS_GROUP_DEF_TRUE,
    sizeof(ht_textarea_t),
};

lv_obj_t *ht_textarea_create(lv_obj_t *parent) {
    lv_obj_t *const obj = lv_obj_class_create_obj(&ht_textarea_class, parent);
    if (!obj) {
        return nullptr;
    }
    lv_obj_class_init_obj(obj);
    return obj;
}

} // namespace heltec::meshcore::ui
