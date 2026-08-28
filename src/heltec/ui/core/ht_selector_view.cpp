/**
 * @file    ht_selector_view.cpp
 * @brief   LVGL view class for surfaces that handle selection movement.
 */
#include "ht_selector_view.hpp"

#include "ht_meta_data_internal.hpp"

namespace heltec::meshcore::ui
{

const lv_obj_class_t ht_selector_view_class = {
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
    sizeof(ht_selector_view_t),
};

lv_obj_t *ht_selector_view_create(lv_obj_t *parent, uint16_t id, void *user_data)
{
    return detail::create_object(&ht_selector_view_class, parent, id, user_data);
}

} // namespace heltec::meshcore::ui
