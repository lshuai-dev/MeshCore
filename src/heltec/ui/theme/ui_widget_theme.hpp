#pragma once

#include <lvgl.h>

namespace heltec::meshcore::ui
{

struct UiAppFrameStyleConfig
{
    lv_coord_t screen_pad;
    lv_coord_t frame_margin_horizontal;
    lv_coord_t frame_margin_bottom;
    lv_coord_t content_radius;
    lv_coord_t bar_height;
    lv_coord_t bar_radius;
};

struct UiContextMenuStyleConfig
{
    lv_coord_t icon_size;
    lv_coord_t icon_gap;
    lv_coord_t border_radius;
    lv_coord_t frame_inset_y;
};

// TopPane-specific geometry is applied to LVGL styles during theme
// initialization. Shared application-bar geometry lives in UiAppFrameStyleConfig.
struct UiTopPaneStyleConfig
{
    lv_coord_t pad_horizontal;
    lv_coord_t pad_top;
    lv_coord_t battery_width;
    lv_coord_t battery_height;
    lv_coord_t battery_pad_right;
};

struct UiThemeProfile
{
    UiAppFrameStyleConfig app_frame;
    UiContextMenuStyleConfig context_menu;
    UiTopPaneStyleConfig top_pane;
};

struct UiCascadingMenuStyles
{
    UiCascadingMenuStyles(const lv_style_t *page_style = nullptr,
                          const lv_style_t *item_style = nullptr,
                          const lv_style_t *selected_item_style = nullptr,
                          const lv_style_t *label_style = nullptr,
                          const lv_style_t *selected_label_style = nullptr)
        : page(page_style), item(item_style), item_selected(selected_item_style),
          label(label_style), label_selected(selected_label_style)
    {
    }

    const lv_style_t *page = nullptr;
    const lv_style_t *item = nullptr;
    const lv_style_t *item_selected = nullptr;
    const lv_style_t *label = nullptr;
    const lv_style_t *label_selected = nullptr;
};

/** Configures geometry from a profile with static storage duration. */
void ui_widget_theme_configure(const UiThemeProfile &profile);
void ui_widget_theme_set_cascading_menu_styles(const UiCascadingMenuStyles &styles);
void ui_theme_apply_context_menu_title(_lv_obj_t *obj);

bool ui_widget_theme_apply(_lv_obj_t *obj);
void ui_app_active_screen_apply_theme(_lv_obj_t *obj);
void ui_theme_apply_quick_ping_message_dropdown_list(_lv_obj_t *list);

void ui_map_marker_apply_color(_lv_obj_t *obj, lv_color_t color);
void ui_map_range_ring_apply_opa(_lv_obj_t *obj, lv_opa_t opa);
bool ui_map_widget_apply_theme(_lv_obj_t *obj);
bool ui_compass_widget_apply_theme(_lv_obj_t *obj);
void ui_theme_set_compass_info_layout(_lv_obj_t *obj, lv_coord_t pad_left, lv_coord_t pad_row);
void ui_theme_set_compass_dial_row_layout(_lv_obj_t *obj, lv_coord_t pad_right);
bool ui_button_roller_apply_theme(_lv_obj_t *obj);
bool ui_license_gate_apply_theme(_lv_obj_t *obj);
void ui_theme_apply_cascading_menu_page(_lv_obj_t *obj);
void ui_theme_apply_cascading_menu_item(_lv_obj_t *obj);
void ui_theme_set_radio_sync_list_margin(_lv_obj_t *obj, lv_coord_t margin);
void ui_theme_center_single_line_textarea(_lv_obj_t *textarea);
void ui_theme_apply_radial_navigation_icon(_lv_obj_t *button, const lv_img_dsc_t *image);
void ui_theme_layout_navigation_grid(_lv_obj_t *host, _lv_obj_t *footer_cell);
void ui_theme_sync_navigation_radius(_lv_obj_t *pane, _lv_obj_t *reference);

void ui_navigator_apply_footer_cell_theme(_lv_obj_t *cell);

} // namespace heltec::meshcore::ui
