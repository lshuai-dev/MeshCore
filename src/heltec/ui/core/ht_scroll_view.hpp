/**
 * @file    ht_scroll_view.hpp
 * @brief   Native LVGL scroll view with cyclic edge handling.
 */
#ifndef HELTEC_UI_CORE_HT_SCROLL_VIEW_HPP
#define HELTEC_UI_CORE_HT_SCROLL_VIEW_HPP

#include "ht_meta_data.hpp"

#include <lvgl.h>

namespace heltec::meshcore::ui
{

typedef struct
{
    lv_obj_t obj;
} ht_scroll_view_t;

extern const lv_obj_class_t ht_scroll_view_class;

lv_obj_t *ht_scroll_view_create(lv_obj_t *parent, uint16_t id = HT_META_ID_UNSET,
                                void *user_data = nullptr);

} // namespace heltec::meshcore::ui

#endif // HELTEC_UI_CORE_HT_SCROLL_VIEW_HPP
