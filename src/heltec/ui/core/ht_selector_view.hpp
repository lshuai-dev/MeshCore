/**
 * @file    ht_selector_view.hpp
 * @brief   LVGL view class for surfaces that handle selection movement.
 */
#ifndef HELTEC_UI_CORE_HT_SELECTOR_VIEW_HPP
#define HELTEC_UI_CORE_HT_SELECTOR_VIEW_HPP

#include "ht_meta_data.hpp"

#include <lvgl.h>

namespace heltec::meshcore::ui
{

typedef struct
{
    lv_obj_t obj;
} ht_selector_view_t;

extern const lv_obj_class_t ht_selector_view_class;

lv_obj_t *ht_selector_view_create(lv_obj_t *parent, void *user_data = nullptr);

} // namespace heltec::meshcore::ui

#endif // HELTEC_UI_CORE_HT_SELECTOR_VIEW_HPP
