#include <lvgl.h>

#include "ui_theme_pixel.hpp"
#include "ui/app/ui_behavior_profile.hpp"
#include "ui/app/ui_app_ids.hpp"
#include "ui/app/ui_theme.hpp"
#include "ui/core/ht_meta_data.hpp"
#include "ui/menus/context_menu.hpp"
#include "ui/navigation/ui_navigator.hpp"
#include "ui/screens/find_friend_screen_ids.hpp"
#include "ui/screens/gps_screen.hpp"
#include "ui/screens/radio_screen.hpp"
#include "ui/screens/system_screen.hpp"
#include "ui/theme/ui_widget_theme.hpp"
#include "ui/widgets/button_roller.hpp"
#include "ui/widgets/top_pane.hpp"

namespace heltec::meshcore::ui {
namespace {

static lv_style_t s_switch_main;
static lv_style_t s_switch_main_focus;
static lv_style_t s_switch_indicator;
static lv_style_t s_switch_indicator_checked;
static lv_style_t s_switch_knob;
static lv_style_t s_row_focus_key;
static lv_style_t s_widget_radius;
static bool s_styles_ready = false;

static lv_color_t s_color_fg;
static lv_color_t s_color_border;
static lv_color_t s_color_on;

static const UiThemeColors kPixelColors = {
    0x142850,  // fg
    0x142850,  // bg
    0xFFFFFF,  // fg_inv
    0xFFFFFF,  // fg_on_dark
    0x102040,  // panel_dark_bg
    0x3A7BC8,  // success
    0xD04040,  // error
    0xE0A030,  // warning
    0x00E060,  // bat_high
    0xFFB000,  // bat_mid
    0xFF3030,  // bat_low
    0x4A90D9,  // accent
    0xFFFF00,  // accent_alt
    0x142850,  // overlay_bg
    0xFFFFFF,  // overlay_fg
    0xFFFFFF,  // highlight_bg
    0x142850,  // highlight_fg
    0x9AB8D8,  // switch_bg
    0x7AA0C8,  // switch_indicator
    0xB8D4F0,  // panel_bg
    0xFFFFFF,  // panel_border
    0x142850,  // top_pane_bg
    0xFFFFFF,  // top_pane_fg
    LV_OPA_TRANSP,
    0x142850,  // frame_bg
    LV_OPA_TRANSP,
    2,         // widget_radius_px
};

static const UiBehaviorProfile kPixelBehaviorProfile = {
    {0},
    {5000, 560, 380, 4, 2},
};

static void initDeviceStyles() {
  ui_set_behavior_profile(&kPixelBehaviorProfile);

#if defined(HELTEC_V4_R8_TFT)
  ui_widget_theme_set_app_frame_style({8, 10, 10, 0, 22, 8});
  ui_widget_theme_set_top_pane_style({32, 12, 10, 10, 5, 24, 14, 10});
#else
  ui_widget_theme_set_app_frame_style({6, 8, 8, 0, 22, 0});
  ui_widget_theme_set_top_pane_style({25, 0, 8, 8, 5, 22, 12, 4});
#endif

#if defined(HELTEC_DISPLAY_ST7789) && HELTEC_DISPLAY_ST7789
  ui_widget_theme_set_context_menu_style({12, 1, 1, 1, 3, 8, 18});
#elif (defined(HELTEC_DISPLAY_ST7735) && HELTEC_DISPLAY_ST7735) || \
    (defined(HELTEC_DISPLAY_SSD1306) && HELTEC_DISPLAY_SSD1306) || \
    LV_COLOR_DEPTH == 1
  ui_widget_theme_set_context_menu_style({12, 1, 1, 1, 3, 8, 6});
#else
  ui_widget_theme_set_context_menu_style({12, 1, 1, 1, 3, 1, 1});
#endif
  ui_widget_theme_set_quick_ping_style({104});
  ui_widget_theme_set_navigation_style({2, 3, 6, 0, 18, 8, 36, 2});
  ui_widget_theme_set_button_roller_style({0, 2, 12});
}

static void styleSetBorderFrame(lv_style_t* st) {
#if defined(HELTEC_V4_R8_TFT)
  lv_style_set_border_width(st, 0);
  lv_style_set_border_opa(st, LV_OPA_TRANSP);
  lv_style_set_border_side(st, LV_BORDER_SIDE_NONE);
#else
  lv_style_set_border_width(st, 1);
  lv_style_set_border_color(st, s_color_border);
  lv_style_set_border_opa(st, LV_OPA_COVER);
#endif
  lv_style_set_outline_width(st, 0);
  lv_style_set_radius(st, ui_widget_radius_px());
}

static void initWidgetStyles() {
  if (s_styles_ready) return;

  s_color_fg = ui_color_fg();
  s_color_border = ui_color_panel_border();
  s_color_on = ui_color_success();

  lv_style_init(&s_switch_main);
  styleSetBorderFrame(&s_switch_main);
  lv_style_set_width(&s_switch_main, 30);
  lv_style_set_height(&s_switch_main, 16);
  lv_style_set_bg_color(&s_switch_main, ui_color_switch_bg());
  lv_style_set_bg_opa(&s_switch_main, LV_OPA_COVER);
  lv_style_set_pad_all(&s_switch_main, 1);

  lv_style_init(&s_switch_main_focus);
  styleSetBorderFrame(&s_switch_main_focus);
  lv_style_set_border_color(&s_switch_main_focus, s_color_fg);

  lv_style_init(&s_switch_indicator);
  styleSetBorderFrame(&s_switch_indicator);
  lv_style_set_bg_color(&s_switch_indicator, ui_color_switch_indicator());
  lv_style_set_bg_opa(&s_switch_indicator, LV_OPA_COVER);

  lv_style_init(&s_switch_indicator_checked);
  lv_style_set_bg_color(&s_switch_indicator_checked, s_color_on);
  lv_style_set_bg_opa(&s_switch_indicator_checked, LV_OPA_COVER);
  lv_style_set_radius(&s_switch_indicator_checked, ui_widget_radius_px());

  lv_style_init(&s_switch_knob);
  lv_style_set_bg_color(&s_switch_knob, s_color_border);
  lv_style_set_bg_opa(&s_switch_knob, LV_OPA_COVER);
#if defined(HELTEC_V4_R8_TFT)
  lv_style_set_border_width(&s_switch_knob, 0);
  lv_style_set_border_opa(&s_switch_knob, LV_OPA_TRANSP);
  lv_style_set_border_side(&s_switch_knob, LV_BORDER_SIDE_NONE);
#else
  lv_style_set_border_width(&s_switch_knob, 1);
  lv_style_set_border_color(&s_switch_knob, s_color_fg);
#endif
  lv_style_set_pad_all(&s_switch_knob, -2);
  lv_style_set_radius(&s_switch_knob, ui_widget_radius_px());

  lv_style_init(&s_row_focus_key);

  lv_style_init(&s_widget_radius);
  lv_style_set_radius(&s_widget_radius, ui_widget_radius_px());
  lv_style_set_bg_opa(&s_widget_radius, LV_OPA_TRANSP);
  lv_style_set_border_width(&s_widget_radius, 0);
  lv_style_set_outline_width(&s_widget_radius, 0);
  lv_style_set_shadow_width(&s_widget_radius, 0);
  lv_style_set_text_color(&s_widget_radius, ui_color_fg());

  s_styles_ready = true;
}

static void applySwitch(lv_obj_t* obj) {
  lv_obj_add_style(obj, &s_switch_main, LV_PART_MAIN);
  lv_obj_add_style(obj, &s_switch_main_focus, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
  lv_obj_add_style(obj, &s_switch_indicator, LV_PART_INDICATOR);
  lv_obj_add_style(obj, &s_switch_indicator_checked, LV_PART_INDICATOR | LV_STATE_CHECKED);
  lv_obj_add_style(obj, &s_switch_knob, LV_PART_KNOB);
}

extern "C" void ui_pixel_theme_apply(lv_theme_t* th, lv_obj_t* obj) {
  if (!obj) return;
  (void)th;

  const bool custom_applied = ui_widget_theme_apply(obj);

#if LV_USE_SWITCH
  const MetaId id = ht_id(obj);
  if (id == meta_id::SystemSwitch || id == meta_id::GpsPowerSwitch ||
      id == meta_id::GpsLocationShareSwitch || id == meta_id::GpsTrackSwitch ||
      id == meta_id::RadioLnaSwitch || id == meta_id::FindFriendSwitch) {
    applySwitch(obj);
    return;
  }
#endif

  if (!custom_applied) lv_obj_add_style(obj, &s_widget_radius, LV_PART_MAIN);
}

}  // namespace

lv_theme_t* init_ui_pixel_theme(lv_disp_t* disp, const lv_font_t* font) {
  if (!disp) return nullptr;
  ui_set_active_theme_colors(&kPixelColors);
  initDeviceStyles();
  initWidgetStyles();

  if (!font) font = LV_FONT_DEFAULT;

  static UiTheme pixel_theme;
  lv_memset_00(&pixel_theme, sizeof(pixel_theme));
  pixel_theme.theme.disp = disp;
  pixel_theme.theme.color_primary = ui_color_accent();
  pixel_theme.theme.color_secondary = ui_color_bg();
  pixel_theme.theme.font_small = font;
  pixel_theme.theme.font_normal = font;
  pixel_theme.theme.font_large = font;
  pixel_theme.theme.apply_cb = ui_pixel_theme_apply;
  ui_set_active_theme_colors(&kPixelColors);
  lv_disp_set_theme(disp, &pixel_theme.theme);
  return &pixel_theme.theme;
}

void ui_pixel_apply_switch_row_focus(lv_obj_t* row, lv_obj_t* sw) {
  if (!row || !sw || !s_styles_ready) return;

  const lv_style_selector_t sel = LV_PART_MAIN | LV_STATE_FOCUS_KEY;
  lv_coord_t bw = lv_obj_get_style_border_width(sw, sel);
  lv_color_t bc = lv_obj_get_style_border_color(sw, sel);
  lv_opa_t bo = lv_obj_get_style_border_opa(sw, sel);
#if defined(HELTEC_V4_R8_TFT)
  (void)bw;
  (void)bc;
  (void)bo;
  lv_style_set_border_width(&s_row_focus_key, 0);
  lv_style_set_border_opa(&s_row_focus_key, LV_OPA_TRANSP);
  lv_style_set_border_side(&s_row_focus_key, LV_BORDER_SIDE_NONE);
  lv_style_set_outline_width(&s_row_focus_key, 0);
  lv_style_set_shadow_width(&s_row_focus_key, 0);
  lv_style_set_bg_color(&s_row_focus_key, ui_color_highlight_bg());
  lv_style_set_bg_opa(&s_row_focus_key, LV_OPA_20);
#else
  if (bw <= 0) {
    bw = 1;
    bc = s_color_fg;
    bo = LV_OPA_COVER;
  }

  lv_style_set_border_width(&s_row_focus_key, bw);
  lv_style_set_border_color(&s_row_focus_key, bc);
  lv_style_set_border_opa(&s_row_focus_key, bo);
  lv_style_set_border_side(&s_row_focus_key, LV_BORDER_SIDE_FULL);
  lv_style_set_outline_width(&s_row_focus_key, 0);
#endif
  lv_style_set_radius(&s_row_focus_key, ui_widget_radius_px());
  lv_obj_add_style(row, &s_row_focus_key, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
}

}  // namespace heltec::meshcore::ui
