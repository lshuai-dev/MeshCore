/**
 * @file HtTextarea.hpp
 * @brief Shared LVGL textarea behavior.
 */
#ifndef HELTEC_UI_CORE_HT_TEXTAREA_HPP
#define HELTEC_UI_CORE_HT_TEXTAREA_HPP

#include <lvgl.h>

namespace Heltec::MeshCore::Ui {

typedef struct {
    lv_textarea_t textarea;
} ht_textarea_t;

extern const lv_obj_class_t ht_textarea_class;

} // namespace Heltec::MeshCore::Ui

#endif // HELTEC_UI_CORE_HT_TEXTAREA_HPP
