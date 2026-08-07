#pragma once

#include <lvgl.h>
#include <stdint.h>

namespace heltec::meshcore::ui {

struct UiAppFrameStyleConfig {
  lv_coord_t screen_pad;
  lv_coord_t frame_margin_left;
  lv_coord_t frame_margin_right;
  lv_coord_t frame_margin_top;
  lv_coord_t frame_margin_bottom;
  lv_coord_t content_radius;
};

struct UiContextMenuStyleConfig {
  lv_coord_t icon_size;
  lv_coord_t icon_pad;
  lv_coord_t border_width;
  lv_coord_t title_border_width;
  lv_coord_t border_radius;
  lv_coord_t frame_inset_x;
  lv_coord_t frame_inset_y;
};

struct UiQuickPingStyleConfig {
  lv_coord_t message_list_min_width;
};

struct UiNavigationStyleConfig {
  uint8_t grid_cols;
  uint8_t grid_rows;
  lv_coord_t grid_gap;
  lv_coord_t grid_pad;
  lv_coord_t grid_label_height;
  lv_coord_t grid_cell_radius;
  lv_coord_t grid_footer_height;
  lv_coord_t ring_edge_pad;
};

// Geometry is applied to LVGL styles during theme initialization. Widgets
// should read their resolved LVGL geometry instead of carrying a parallel
// per-object metrics profile.
struct UiTopPaneStyleConfig {
  lv_coord_t height;
  lv_coord_t radius;
  lv_coord_t pad_left;
  lv_coord_t pad_right;
  lv_coord_t pad_top;
  lv_coord_t battery_width;
  lv_coord_t battery_height;
  lv_coord_t battery_pad_right;
};

struct UiButtonRollerStyleConfig {
  lv_coord_t border_width;
  lv_coord_t pad;
  lv_coord_t button_height;
};

void ui_widget_theme_set_app_frame_style(const UiAppFrameStyleConfig& config);
void ui_widget_theme_set_context_menu_style(const UiContextMenuStyleConfig& config);
void ui_widget_theme_set_quick_ping_style(const UiQuickPingStyleConfig& config);
void ui_widget_theme_set_navigation_style(const UiNavigationStyleConfig& config);
void ui_widget_theme_set_top_pane_style(const UiTopPaneStyleConfig& config);
void ui_widget_theme_set_button_roller_style(const UiButtonRollerStyleConfig& config);

bool ui_widget_theme_apply(_lv_obj_t* obj);
void ui_app_active_screen_apply_theme(_lv_obj_t* obj);

void ui_map_marker_apply_color(_lv_obj_t* obj, lv_color_t color);
void ui_map_range_ring_apply_opa(_lv_obj_t* obj, lv_opa_t opa);
bool ui_map_widget_apply_theme(_lv_obj_t* obj);
bool ui_compass_widget_apply_theme(_lv_obj_t* obj);
void ui_theme_set_compass_info_layout(_lv_obj_t* obj, lv_coord_t pad_left,
                                      lv_coord_t pad_row);
void ui_theme_set_compass_dial_row_layout(_lv_obj_t* obj,
                                          lv_coord_t pad_right);
bool ui_button_roller_apply_theme(_lv_obj_t* obj);
bool ui_license_gate_apply_theme(_lv_obj_t* obj);
void ui_theme_apply_cascading_menu_page(_lv_obj_t* obj);
void ui_theme_apply_cascading_menu_item(_lv_obj_t* obj);
void ui_theme_set_radio_sync_list_margin(_lv_obj_t* obj, lv_coord_t margin);
void ui_theme_center_single_line_textarea(_lv_obj_t* textarea);
void ui_theme_apply_radial_navigation_icon(_lv_obj_t* button,
                                           const lv_img_dsc_t* image);
void ui_theme_layout_navigation_grid(_lv_obj_t* host,
                                     _lv_obj_t* footer_cell);
void ui_theme_sync_navigation_radius(_lv_obj_t* pane,
                                     _lv_obj_t* reference);

void ui_navigator_apply_footer_cell_theme(_lv_obj_t* cell);

}  // namespace heltec::meshcore::ui
