#pragma once

#include <lvgl.h>
#include <stdint.h>

struct _lv_disp_t;
typedef struct _lv_disp_t lv_disp_t;

namespace heltec::meshcore::ui {
// Stable, centralized device-theme boundary. Device variants provide color and
// metric values through this module; screens and widgets do not depend on a
// device-specific theme interface or subclass.
struct UiThemeColors {
  uint32_t fg;
  uint32_t bg;
  uint32_t fg_inv;
  uint32_t fg_on_dark;
  uint32_t panel_dark_bg;
  uint32_t success;
  uint32_t error;
  uint32_t warning;
  uint32_t bat_high;
  uint32_t bat_mid;
  uint32_t bat_low;
  uint32_t accent;
  uint32_t accent_alt;
  uint32_t overlay_bg;
  uint32_t overlay_fg;
  uint32_t highlight_bg;
  uint32_t highlight_fg;
  uint32_t switch_bg;
  uint32_t switch_indicator;
  uint32_t panel_bg;
  uint32_t panel_border;
  uint32_t top_pane_bg;
  uint32_t top_pane_fg;
  uint8_t top_pane_bg_opa;
  uint32_t frame_bg;
  uint8_t frame_bg_opa;
  uint8_t widget_radius_px;
};

struct UiTheme {
  lv_theme_t theme;
};

UiThemeColors make_ui_theme_colors();
const UiThemeColors& ui_theme_colors();
void ui_set_active_theme_colors(const UiThemeColors* colors);

inline lv_color_t ui_color_panel_bg() {
  return lv_color_hex(ui_theme_colors().panel_bg);
}
inline lv_color_t ui_color_panel_border() {
  return lv_color_hex(ui_theme_colors().panel_border);
}
inline lv_color_t ui_color_top_pane_bg() {
  return lv_color_hex(ui_theme_colors().top_pane_bg);
}
inline lv_color_t ui_color_top_pane_fg() {
  return lv_color_hex(ui_theme_colors().top_pane_fg);
}
inline lv_opa_t ui_effective_opa(lv_opa_t opa) {
#if LV_COLOR_DEPTH == 1
  return opa == LV_OPA_TRANSP ? LV_OPA_TRANSP : LV_OPA_COVER;
#else
  return opa;
#endif
}
inline lv_opa_t ui_top_pane_bg_opa() {
  return ui_effective_opa(static_cast<lv_opa_t>(ui_theme_colors().top_pane_bg_opa));
}
inline lv_color_t ui_color_frame_bg() {
  return lv_color_hex(ui_theme_colors().frame_bg);
}
inline lv_opa_t ui_frame_bg_opa() {
  return ui_effective_opa(static_cast<lv_opa_t>(ui_theme_colors().frame_bg_opa));
}
inline lv_color_t ui_color_fg() {
  return lv_color_hex(ui_theme_colors().fg);
}
inline lv_color_t ui_color_bg() {
  return lv_color_hex(ui_theme_colors().bg);
}
inline lv_color_t ui_color_fg_inv() {
  return lv_color_hex(ui_theme_colors().fg_inv);
}
inline lv_color_t ui_color_fg_on_dark() {
  return lv_color_hex(ui_theme_colors().fg_on_dark);
}
inline lv_color_t ui_color_panel_dark_bg() {
  return lv_color_hex(ui_theme_colors().panel_dark_bg);
}
inline lv_color_t ui_color_success() {
  return lv_color_hex(ui_theme_colors().success);
}
inline lv_color_t ui_color_error() {
  return lv_color_hex(ui_theme_colors().error);
}
inline lv_color_t ui_color_warning() {
  return lv_color_hex(ui_theme_colors().warning);
}
inline lv_color_t ui_color_battery_high() {
  return lv_color_hex(ui_theme_colors().bat_high);
}
inline lv_color_t ui_color_battery_mid() {
  return lv_color_hex(ui_theme_colors().bat_mid);
}
inline lv_color_t ui_color_battery_low() {
  return lv_color_hex(ui_theme_colors().bat_low);
}
inline lv_color_t ui_color_accent() {
  return lv_color_hex(ui_theme_colors().accent);
}
inline lv_color_t ui_color_accent_alt() {
  return lv_color_hex(ui_theme_colors().accent_alt);
}
inline lv_color_t ui_color_overlay_bg() {
  return lv_color_hex(ui_theme_colors().overlay_bg);
}
inline lv_color_t ui_color_overlay_fg() {
  return lv_color_hex(ui_theme_colors().overlay_fg);
}
inline lv_color_t ui_color_highlight_bg() {
  return lv_color_hex(ui_theme_colors().highlight_bg);
}
inline lv_color_t ui_color_highlight_fg() {
  return lv_color_hex(ui_theme_colors().highlight_fg);
}
inline lv_color_t ui_color_switch_bg() {
  return lv_color_hex(ui_theme_colors().switch_bg);
}
inline lv_color_t ui_color_switch_indicator() {
  return lv_color_hex(ui_theme_colors().switch_indicator);
}
inline uint8_t ui_widget_radius_px() {
  return ui_theme_colors().widget_radius_px;
}
inline lv_color_t ui_navigation_idle_color() {
  return ui_color_fg();
}
inline lv_color_t ui_navigation_focus_color() {
  return ui_color_accent();
}
bool ui_theme_init(lv_disp_t* disp);
/** Applies the active theme's row focus style for a row associated with a switch. */
void ui_theme_apply_switch_row_focus(_lv_obj_t* row, _lv_obj_t* sw);
/** Applies the active theme's popup dropdown list chrome/background/selection style. */
void ui_theme_apply_dropdown_list(_lv_obj_t* list);
/** Matches popup-list horizontal text padding to its closed dropdown control. */
void ui_theme_match_dropdown_list_padding(_lv_obj_t* dropdown, _lv_obj_t* list);
/** Vertically centers the closed dropdown value inside the current fixed height. */
void ui_theme_center_dropdown_value(_lv_obj_t* dropdown);
}  // namespace heltec::meshcore::ui

#if defined(UI_THEME_MONO) && UI_THEME_MONO && \
    defined(UI_THEME_COLOR) && UI_THEME_COLOR
#error "Define only one of UI_THEME_MONO or UI_THEME_COLOR"
#endif

#if defined(UI_THEME_MONO) && UI_THEME_MONO
#if !LV_USE_THEME_MONO
#error "UI_THEME_MONO requires LV_USE_THEME_MONO=1"
#endif
#elif defined(UI_THEME_COLOR) && UI_THEME_COLOR
#endif
