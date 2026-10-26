#pragma once

#include <lvgl.h>
#include <stdint.h>

struct _lv_disp_t;
typedef struct _lv_disp_t lv_disp_t;

namespace Heltec::MeshCore::Ui
{

// 普通控件通过 LVGL style 获取颜色；该调色板仅供主题和自定义绘制使用。
struct ThemePalette
{
    uint32_t fg;                // 默认前景色
    uint32_t bg;                // 默认背景色
    uint32_t fg_inv;            // 反色区域的前景色
    uint32_t fg_on_dark;        // 深色绘制区域的前景色
    uint32_t panel_dark_bg;     // 深色面板背景色
    uint32_t success;           // 成功、正常状态颜色
    uint32_t error;             // 错误、告警状态颜色
    uint32_t bat_high;          // 高电量颜色
    uint32_t bat_mid;           // 中等电量颜色
    uint32_t bat_low;           // 低电量颜色
    uint32_t accent;            // 主要强调色
    uint32_t accent_alt;        // 次要强调色
    uint32_t overlay_bg;        // Overlay 背景色
    uint32_t overlay_fg;        // Overlay 前景色
    uint32_t highlight_bg;      // 选中、聚焦状态背景色
    uint32_t highlight_fg;      // 选中、聚焦状态前景色
    uint32_t switch_bg;         // Switch 轨道背景色
    uint32_t switch_indicator;  // Switch 未选中指示色
    uint32_t panel_bg;          // 普通面板背景色
    uint32_t panel_border;      // 普通面板边框色
    uint32_t top_pane_bg;       // 顶栏背景色
    uint32_t top_pane_fg;       // 顶栏前景色
    uint8_t top_pane_bg_opa;    // 顶栏背景透明度
    uint32_t frame_bg;          // 应用框架背景色
    uint8_t frame_bg_opa;       // 应用框架背景透明度
    uint8_t widget_radius_px;   // 控件默认圆角半径（像素）
};

ThemePalette defaultThemePalette();
const ThemePalette &themePalette();
void setThemePalette(const ThemePalette *palette);

inline lv_opa_t effectiveOpacity(uint8_t opacity)
{
#if LV_COLOR_DEPTH == 1
    return opacity == LV_OPA_TRANSP ? LV_OPA_TRANSP : LV_OPA_COVER;
#else
    return static_cast<lv_opa_t>(opacity);
#endif
}

constexpr lv_coord_t settingsRowHeight()
{
#if HELTEC_UI_HAS_TOUCH
    return 32;
#else
    return LV_SIZE_CONTENT;
#endif
}

bool initUiTheme(lv_disp_t *display);

} // namespace Heltec::MeshCore::Ui
