/**
 * @file    ht_container.cpp
 * @brief   Common UI container type and layout factory.
 */
#include "ht_container.hpp"

#include "ht_meta_data_internal.hpp"

namespace heltec::meshcore::ui
{
namespace
{

void container_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj)
{
    (void)class_p;
    if (!obj)
    {
        return;
    }

    lv_obj_set_width(obj, lv_pct(100));
    lv_obj_set_height(obj, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(obj, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
}

} // namespace

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
