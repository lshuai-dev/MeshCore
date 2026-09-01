/**
 * @file    screen_widget_classes.cpp
 * @brief   Semantic LVGL classes for exceptional screen widgets.
 */
#include "screen_widget_classes.hpp"

#include "ui/core/ht_meta_data.hpp"
#include "ui/core/ht_meta_data_internal.hpp"

namespace heltec::meshcore::ui
{

const lv_obj_class_t ht_screen_trailing_label_class = {
    &lv_label_class,
    nullptr,
    nullptr,
#if LV_USE_USER_DATA
    nullptr,
#endif
    nullptr,
    LV_SIZE_CONTENT,
    LV_SIZE_CONTENT,
    LV_OBJ_CLASS_EDITABLE_INHERIT,
    LV_OBJ_CLASS_GROUP_DEF_INHERIT,
    sizeof(lv_label_t),
};

const lv_obj_class_t ht_screen_centered_label_class = {
    &lv_label_class,
    nullptr,
    nullptr,
#if LV_USE_USER_DATA
    nullptr,
#endif
    nullptr,
    LV_SIZE_CONTENT,
    LV_SIZE_CONTENT,
    LV_OBJ_CLASS_EDITABLE_INHERIT,
    LV_OBJ_CLASS_GROUP_DEF_INHERIT,
    sizeof(lv_label_t),
};

const lv_obj_class_t ht_find_friend_dial_host_class = {
    &lv_obj_class,
    nullptr,
    nullptr,
#if LV_USE_USER_DATA
    nullptr,
#endif
    nullptr,
    LV_SIZE_CONTENT,
    LV_SIZE_CONTENT,
    LV_OBJ_CLASS_EDITABLE_INHERIT,
    LV_OBJ_CLASS_GROUP_DEF_INHERIT,
    sizeof(lv_obj_t),
};

const lv_obj_class_t ht_volume_controls_class = {
    &lv_obj_class,
    nullptr,
    nullptr,
#if LV_USE_USER_DATA
    nullptr,
#endif
    nullptr,
    LV_SIZE_CONTENT,
    LV_SIZE_CONTENT,
    LV_OBJ_CLASS_EDITABLE_INHERIT,
    LV_OBJ_CLASS_GROUP_DEF_INHERIT,
    sizeof(lv_obj_t),
};

const lv_obj_class_t ht_volume_button_class = {
    &lv_btn_class,
    nullptr,
    nullptr,
#if LV_USE_USER_DATA
    nullptr,
#endif
    nullptr,
    LV_SIZE_CONTENT,
    LV_SIZE_CONTENT,
    LV_OBJ_CLASS_EDITABLE_INHERIT,
    LV_OBJ_CLASS_GROUP_DEF_INHERIT,
    sizeof(lv_btn_t),
};

lv_obj_t *ht_screen_trailing_label_create(lv_obj_t *parent, const char *text)
{
    lv_obj_t *const label =
        detail::create_object(&ht_screen_trailing_label_class, parent);
    if (label && text)
    {
        lv_label_set_text(label, text);
    }
    return label;
}

lv_obj_t *ht_screen_centered_label_create(lv_obj_t *parent, const char *text)
{
    lv_obj_t *const label = detail::create_object(&ht_screen_centered_label_class, parent);
    if (label && text)
    {
        lv_label_set_text(label, text);
    }
    return label;
}

lv_obj_t *ht_find_friend_dial_host_create(lv_obj_t *parent)
{
    return detail::create_object(&ht_find_friend_dial_host_class, parent);
}

lv_obj_t *ht_volume_controls_create(lv_obj_t *parent)
{
    return detail::create_object(&ht_volume_controls_class, parent);
}

lv_obj_t *ht_volume_button_create(lv_obj_t *parent)
{
    return detail::create_object(&ht_volume_button_class, parent);
}

} // namespace heltec::meshcore::ui
