/**
 * @file    screen_widget_classes.hpp
 * @brief   Semantic LVGL classes for exceptional screen widgets.
 */
#ifndef HELTEC_UI_SCREENS_SCREEN_WIDGET_CLASSES_HPP
#define HELTEC_UI_SCREENS_SCREEN_WIDGET_CLASSES_HPP

#include <lvgl.h>

namespace heltec::meshcore::ui
{

extern const lv_obj_class_t ht_screen_trailing_label_class;
extern const lv_obj_class_t ht_screen_centered_label_class;
extern const lv_obj_class_t ht_find_friend_dial_host_class;
extern const lv_obj_class_t ht_volume_controls_class;
extern const lv_obj_class_t ht_volume_button_class;

lv_obj_t *ht_screen_trailing_label_create(lv_obj_t *parent, const char *text = nullptr);
lv_obj_t *ht_screen_centered_label_create(lv_obj_t *parent, const char *text = nullptr);
lv_obj_t *ht_find_friend_dial_host_create(lv_obj_t *parent);
lv_obj_t *ht_volume_controls_create(lv_obj_t *parent);
lv_obj_t *ht_volume_button_create(lv_obj_t *parent);

} // namespace heltec::meshcore::ui

#endif // HELTEC_UI_SCREENS_SCREEN_WIDGET_CLASSES_HPP
