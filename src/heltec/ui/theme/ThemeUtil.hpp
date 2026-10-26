#pragma once

#include <lvgl.h>

namespace Heltec::MeshCore::Ui
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

struct UiTopPaneStyleConfig
{
    lv_coord_t pad_top;
    lv_coord_t battery_width;
    lv_coord_t battery_height;
};

struct UiThemeProfile
{
    UiAppFrameStyleConfig app_frame;
    UiContextMenuStyleConfig context_menu;
    UiTopPaneStyleConfig top_pane;
};

struct UiContextMenuRollerStyles
{
    UiContextMenuRollerStyles(const lv_style_t *page_style = nullptr,
                              const lv_style_t *item_style = nullptr,
                              const lv_style_t *selected_item_style = nullptr)
        : page(page_style), item(item_style), item_selected(selected_item_style)
    {
    }

    const lv_style_t *page = nullptr;
    const lv_style_t *item = nullptr;
    const lv_style_t *item_selected = nullptr;
};

void configureComponentTheme();
// Profile 必须具有静态生命周期，并在任何主题样式初始化前配置。
void configureWidgetTheme(const UiThemeProfile &profile);
void setContextMenuRollerStyles(const UiContextMenuRollerStyles &styles);

bool applyButtonRollerTheme(_lv_obj_t *obj);
bool applyCompassTheme(_lv_obj_t *obj);
bool applyLicenseGateTheme(_lv_obj_t *obj);
bool applyMapWidgetTheme(_lv_obj_t *obj);
bool applyRadialNavigationIconTheme(_lv_obj_t *icon);

} // namespace Heltec::MeshCore::Ui
