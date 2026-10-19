/**
 * @file    HtSelectorView.cpp
 * @brief   LVGL view class for surfaces that handle selection movement.
 */
#include "HtSelectorView.hpp"

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

lv_obj_t *ht_selector_view_create(lv_obj_t *parent, void *user_data)
{
    lv_obj_t *const obj = lv_obj_class_create_obj(&ht_selector_view_class, parent);
    if (!obj) {
        return nullptr;
    }

    lv_obj_class_init_obj(obj);
    lv_obj_set_user_data(obj, user_data);

    return obj;
}

} // namespace heltec::meshcore::ui
