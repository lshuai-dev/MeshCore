/**
 * @file    ht_container.hpp
 * @brief   Common UI container type and layout factory.
 */
#ifndef HELTEC_UI_CORE_HT_CONTAINER_HPP
#define HELTEC_UI_CORE_HT_CONTAINER_HPP

#include "ht_meta_data.hpp"

#include <lvgl.h>

namespace heltec::meshcore::ui
{

typedef struct
{
    lv_obj_t obj;
} ht_container_t;

extern const lv_obj_class_t ht_container_class;

lv_obj_t *ht_container_create(lv_obj_t *parent, uint16_t id = HT_META_ID_UNSET,
                               void *user_data = nullptr);

} // namespace heltec::meshcore::ui

#endif // HELTEC_UI_CORE_HT_CONTAINER_HPP
