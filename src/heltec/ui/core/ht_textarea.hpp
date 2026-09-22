/**
 * @file ht_textarea.hpp
 * @brief Shared LVGL textarea behavior.
 */
#ifndef HELTEC_UI_CORE_HT_TEXTAREA_HPP
#define HELTEC_UI_CORE_HT_TEXTAREA_HPP

#include <lvgl.h>

namespace heltec::meshcore::ui {

typedef struct {
    lv_textarea_t textarea;
} ht_textarea_t;

extern const lv_obj_class_t ht_textarea_class;

lv_obj_t *ht_textarea_create(lv_obj_t *parent);

} // namespace heltec::meshcore::ui

#endif // HELTEC_UI_CORE_HT_TEXTAREA_HPP
