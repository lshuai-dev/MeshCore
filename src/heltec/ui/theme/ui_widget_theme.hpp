#pragma once

#include <lvgl.h>

namespace heltec::meshcore::ui {

bool ui_widget_theme_apply(_lv_obj_t* obj);
void ui_app_active_screen_apply_theme(_lv_obj_t* obj);

void ui_map_marker_apply_color(_lv_obj_t* obj, lv_color_t color);
void ui_map_range_ring_apply_opa(_lv_obj_t* obj, lv_opa_t opa);
bool ui_map_widget_apply_theme(_lv_obj_t* obj);
bool ui_compass_widget_apply_theme(_lv_obj_t* obj);
bool ui_button_roller_apply_theme(_lv_obj_t* obj);

void ui_navigator_apply_footer_cell_theme(_lv_obj_t* cell);

}  // namespace heltec::meshcore::ui
