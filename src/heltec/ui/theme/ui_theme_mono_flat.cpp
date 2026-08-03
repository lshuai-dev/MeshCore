#include <lvgl.h>

#if LV_USE_THEME_MONO
#include "ui_theme_mono_flat.hpp"
#include "ui/app/ui_app_ids.hpp"
#include "ui/app/ui_app_frame_metrics.hpp"
#include "ui/app/ui_theme.hpp"
#include "ui/core/ht_meta_data.hpp"
#include "ui/menus/context_menu.hpp"
#include "ui/menus/context_menu_metrics.hpp"
#include "ui/navigation/ui_navigator.hpp"
#include "ui/screens/find_friend_screen_ids.hpp"
#include "ui/screens/gps_screen.hpp"
#include "ui/screens/radio_screen.hpp"
#include "ui/screens/system_screen.hpp"
#include "ui/theme/ui_theme_metrics.hpp"
#include "ui/theme/ui_widget_theme.hpp"
#include "ui/widgets/button_roller.hpp"
#include "ui/widgets/top_pane.hpp"

static heltec::meshcore::ui::UiTheme s_ui_mono_flat;
static heltec::meshcore::ui::UiTheme s_ui_ssd1306;

static lv_style_t s_mono_reset_style;
static bool s_widget_styles_inited = false;

static lv_color_t s_color_fg;
static lv_color_t s_color_bg;
static lv_color_t s_color_border;
static lv_color_t s_color_on;
static lv_color_t s_color_switch_bg;
static lv_color_t s_color_switch_indicator;

static lv_style_t s_switch_main;
static lv_style_t s_switch_main_focus;
static lv_style_t s_switch_indicator;
static lv_style_t s_switch_indicator_checked;
static lv_style_t s_switch_knob;
static lv_style_t s_switch_knob_checked;

static lv_style_t s_row_focus_key;
static bool s_row_focus_key_ready = false;
static lv_style_t s_metrics_style;
static bool s_device_styles_inited = false;

static const heltec::meshcore::ui::UiThemeColors kMonoFlatColors = {
    0xFFFFFF,  // fg
    0x000000,  // bg
    0x000000,  // fg_inv
    0xFFFFFF,  // fg_on_dark
    0x000000,  // panel_dark_bg
    0x00E060,  // success
    0xFF3030,  // error
    0xFFB000,  // warning
    0x00E060,  // bat_high
    0xFFB000,  // bat_mid
    0xFF3030,  // bat_low
    0x00FF00,  // accent
    0xFFFF00,  // accent_alt
    0x000000,  // overlay_bg
    0xFFFFFF,  // overlay_fg
    0xFFFFFF,  // highlight_bg
    0x000000,  // highlight_fg
    0x202020,  // switch_bg
    0x303030,  // switch_indicator
    0x000000,  // panel_bg
    0xFFFFFF,  // panel_border
    0xFFFFFF,  // top_pane_bg
    0x000000,  // top_pane_fg
    LV_OPA_COVER,
    0x000000,  // frame_bg
    LV_OPA_COVER,
    2,         // widget_radius_px
};

static constexpr uint16_t kContextMenuPageAnimMs = 0;

static void init_device_styles(bool ssd1306) {
  using namespace heltec::meshcore::ui;
  ui_theme_metrics_init();

  static const UiThemeMetrics kMonoThemeMetrics = {
      {2, 0, 0, 0, 0, 0},
#if defined(HELTEC_T114_WITH_DISPLAY)
      {18, 4, 24, 0, -1, 0},
#else
      {12, 4, 18, 0, -1, 0},
#endif
      {12, 1, 1, 1, 1, 3, kContextMenuPageAnimMs},
      {100, 100, 8, 104},
      {0, 2, 12},
      {5000, 560, 380, 4, 2, 2, 2, 3, 6, 0, 16, 8, 36},
  };
  static const UiThemeMetrics kSsd1306ThemeMetrics = {
      {2, 0, 0, 0, 0, 0},
      {10, 2, 15, 0, -1, 0},
      {12, 0, 1, 1, 1, 2, kContextMenuPageAnimMs},
      {100, 100, 8, 104},
      {0, 0, 10},
      {5000, 560, 380, 4, 2, 2, 2, 3, 6, 0, 16, 8, 36},
  };

  if (!s_device_styles_inited) {
    lv_style_init(&s_metrics_style);
    s_device_styles_inited = true;
  }

  lv_style_value_t value{};
  value.ptr = ssd1306 ? static_cast<const void*>(&kSsd1306ThemeMetrics)
                      : static_cast<const void*>(&kMonoThemeMetrics);
  lv_style_set_prop(&s_metrics_style, UI_PROP_THEME_METRICS, value);
}

static void apply_device_styles(lv_obj_t* obj) {
  using namespace heltec::meshcore::ui;
  if (!obj) return;
  switch (ht_id(obj)) {
    case meta_id::AppOverlayLayer:
    case meta_id::AppFrameLayout:
      lv_obj_add_style(obj, &s_metrics_style, LV_PART_MAIN);
      break;
    default:
      break;
  }
}

static void style_set_border_frame(lv_style_t* st) {
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
}

static void init_widget_styles() {
  if (s_widget_styles_inited) return;

  lv_style_init(&s_mono_reset_style);
  lv_style_set_border_width(&s_mono_reset_style, 0);
  lv_style_set_outline_width(&s_mono_reset_style, 0);

  s_color_fg = heltec::meshcore::ui::ui_color_fg();
  s_color_bg = heltec::meshcore::ui::ui_color_bg();
  s_color_border = s_color_fg;
  s_color_on = heltec::meshcore::ui::ui_color_success();
  s_color_switch_bg = heltec::meshcore::ui::ui_color_switch_bg();
  s_color_switch_indicator = heltec::meshcore::ui::ui_color_switch_indicator();
  lv_style_set_bg_opa(&s_mono_reset_style, LV_OPA_TRANSP);
  lv_style_set_border_opa(&s_mono_reset_style, LV_OPA_TRANSP);
  lv_style_set_shadow_width(&s_mono_reset_style, 0);
  lv_style_set_radius(&s_mono_reset_style, 0);
  lv_style_set_pad_all(&s_mono_reset_style, 0);
  lv_style_set_text_color(&s_mono_reset_style, s_color_fg);

  lv_style_init(&s_switch_main);
  style_set_border_frame(&s_switch_main);
#if LV_COLOR_DEPTH == 1
  // One physical pixel is the thinnest useful frame on a 1-bit panel. Give
  // it a little more area so it does not dominate the control visually.
  lv_style_set_width(&s_switch_main, 24);
  lv_style_set_height(&s_switch_main, 12);
  lv_style_set_bg_color(&s_switch_main, s_color_bg);
#else
  lv_style_set_width(&s_switch_main, 30);
  lv_style_set_height(&s_switch_main, 16);
  lv_style_set_bg_color(&s_switch_main, s_color_switch_bg);
#endif
  lv_style_set_bg_opa(&s_switch_main, LV_OPA_COVER);
#if LV_COLOR_DEPTH == 1
  lv_style_set_pad_all(&s_switch_main, 1);
  lv_style_set_radius(&s_switch_main, LV_RADIUS_CIRCLE);
#else
  lv_style_set_pad_all(&s_switch_main, 1);
  lv_style_set_radius(&s_switch_main, 8);
#endif

  lv_style_init(&s_switch_main_focus);
  style_set_border_frame(&s_switch_main_focus);

  lv_style_init(&s_switch_indicator);
#if LV_COLOR_DEPTH == 1
  // Avoid a second border inside the main frame. OFF is a black track.
  lv_style_set_border_width(&s_switch_indicator, 0);
  lv_style_set_border_opa(&s_switch_indicator, LV_OPA_TRANSP);
#else
  style_set_border_frame(&s_switch_indicator);
  lv_style_set_bg_color(&s_switch_indicator, s_color_switch_indicator);
#endif
  lv_style_set_bg_opa(&s_switch_indicator, LV_OPA_COVER);
#if LV_COLOR_DEPTH == 1
  lv_style_set_bg_color(&s_switch_indicator, s_color_bg);
  lv_style_set_radius(&s_switch_indicator, LV_RADIUS_CIRCLE);
#else
  lv_style_set_radius(&s_switch_indicator, 8);
#endif

  lv_style_init(&s_switch_indicator_checked);
#if LV_COLOR_DEPTH == 1
  // ON reverses the track to white; the checked knob below reverses to black.
  lv_style_set_bg_color(&s_switch_indicator_checked, s_color_fg);
#else
  lv_style_set_bg_color(&s_switch_indicator_checked, s_color_on);
#endif
  lv_style_set_bg_opa(&s_switch_indicator_checked, LV_OPA_COVER);
#if LV_COLOR_DEPTH == 1
  lv_style_set_border_width(&s_switch_indicator_checked, 0);
  lv_style_set_border_opa(&s_switch_indicator_checked, LV_OPA_TRANSP);
  lv_style_set_radius(&s_switch_indicator_checked, LV_RADIUS_CIRCLE);
#else
  lv_style_set_radius(&s_switch_indicator_checked, 8);
#endif

  lv_style_init(&s_switch_knob);
  lv_style_set_bg_color(&s_switch_knob, s_color_fg);
  lv_style_set_bg_opa(&s_switch_knob, LV_OPA_COVER);
#if LV_COLOR_DEPTH == 1
  lv_style_set_border_width(&s_switch_knob, 0);
  lv_style_set_border_opa(&s_switch_knob, LV_OPA_TRANSP);
#elif defined(HELTEC_V4_R8_TFT)
  lv_style_set_border_width(&s_switch_knob, 0);
  lv_style_set_border_opa(&s_switch_knob, LV_OPA_TRANSP);
  lv_style_set_border_side(&s_switch_knob, LV_BORDER_SIDE_NONE);
#else
  lv_style_set_border_width(&s_switch_knob, 1);
  lv_style_set_border_color(&s_switch_knob, s_color_border);
#endif
  lv_style_set_pad_all(&s_switch_knob, -2);
  lv_style_set_radius(&s_switch_knob, LV_RADIUS_CIRCLE);

  lv_style_init(&s_switch_knob_checked);
#if LV_COLOR_DEPTH == 1
  lv_style_set_bg_color(&s_switch_knob_checked, s_color_bg);
  lv_style_set_bg_opa(&s_switch_knob_checked, LV_OPA_COVER);
  lv_style_set_border_width(&s_switch_knob_checked, 0);
  lv_style_set_border_opa(&s_switch_knob_checked, LV_OPA_TRANSP);
  lv_style_set_pad_all(&s_switch_knob_checked, -2);
  lv_style_set_radius(&s_switch_knob_checked, LV_RADIUS_CIRCLE);
#endif

  s_widget_styles_inited = true;
}

static void apply_mono_fallback(lv_obj_t* obj) {
  lv_obj_add_style(obj, &s_mono_reset_style, LV_PART_MAIN);
}

#if LV_USE_SWITCH
static void mono_flat_apply_switch(lv_obj_t* obj) {
  lv_obj_add_style(obj, &s_switch_main, LV_PART_MAIN);
  lv_obj_add_style(obj, &s_switch_main_focus, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
  lv_obj_add_style(obj, &s_switch_indicator, LV_PART_INDICATOR);
  lv_obj_add_style(obj, &s_switch_indicator_checked, LV_PART_INDICATOR | LV_STATE_CHECKED);
  lv_obj_add_style(obj, &s_switch_knob, LV_PART_KNOB);
#if LV_COLOR_DEPTH == 1
  lv_obj_add_style(obj, &s_switch_knob_checked, LV_PART_KNOB | LV_STATE_CHECKED);
#endif
}
#endif

extern "C" void ui_mono_flat_apply(lv_theme_t* th, lv_obj_t* obj) {
  if (!obj) return;
  (void)th;

  apply_device_styles(obj);
  const bool custom_applied =
      heltec::meshcore::ui::ui_widget_theme_apply(obj);

#if LV_USE_SWITCH
  const heltec::meshcore::ui::MetaId id = heltec::meshcore::ui::ht_id(obj);
  if (id == heltec::meshcore::ui::meta_id::SystemSwitch ||
      id == heltec::meshcore::ui::meta_id::GpsPowerSwitch ||
      id == heltec::meshcore::ui::meta_id::GpsLocationShareSwitch ||
      id == heltec::meshcore::ui::meta_id::GpsTrackSwitch ||
      id == heltec::meshcore::ui::meta_id::RadioLnaSwitch ||
      id == heltec::meshcore::ui::meta_id::FindFriendSwitch) {
    mono_flat_apply_switch(obj);
    return;
  }
#endif

  if (!custom_applied) apply_mono_fallback(obj);
}

namespace heltec::meshcore::ui {

void ui_mono_flat_apply_switch_row_focus(lv_obj_t* row, lv_obj_t* sw) {
  if (!row || !sw || !s_widget_styles_inited) return;

  if (!s_row_focus_key_ready) {
    lv_style_init(&s_row_focus_key);
    s_row_focus_key_ready = true;
  }

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
  lv_style_set_bg_color(&s_row_focus_key, heltec::meshcore::ui::ui_color_highlight_bg());
  lv_style_set_bg_opa(&s_row_focus_key, LV_OPA_20);
#else
  if (bw <= 0) {
    bw = 1;
    bc = s_color_border;
    bo = LV_OPA_COVER;
  }

  lv_style_set_border_width(&s_row_focus_key, bw);
  lv_style_set_border_color(&s_row_focus_key, bc);
  lv_style_set_border_opa(&s_row_focus_key, bo);
  lv_style_set_border_side(&s_row_focus_key, LV_BORDER_SIDE_FULL);
  lv_style_set_outline_width(&s_row_focus_key, 0);
#endif
  lv_obj_add_style(row, &s_row_focus_key, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
}

static lv_theme_t* init_mono_flat_theme_variant(lv_disp_t* disp, const lv_font_t* font,
                                                bool ssd1306,
                                                UiTheme& theme_storage) {
  ui_set_active_theme_colors(&kMonoFlatColors);
  init_device_styles(ssd1306);
  init_widget_styles();

  if (!disp) return nullptr;
  if (!font) font = LV_FONT_DEFAULT;

  lv_memset_00(&theme_storage, sizeof(theme_storage));
  theme_storage.theme.disp = disp;
  theme_storage.theme.color_primary = ui_color_accent();
  theme_storage.theme.color_secondary = ui_color_bg();
  theme_storage.theme.font_small = font;
  theme_storage.theme.font_normal = font;
  theme_storage.theme.font_large = font;
  theme_storage.theme.apply_cb = ui_mono_flat_apply;
  ui_set_active_theme_colors(&kMonoFlatColors);
  lv_disp_set_theme(disp, &theme_storage.theme);
  return &theme_storage.theme;
}

lv_theme_t* init_ui_mono_flat_theme(lv_disp_t* disp, const lv_font_t* font) {
  return init_mono_flat_theme_variant(disp, font, false, s_ui_mono_flat);
}

lv_theme_t* init_ui_ssd1306_theme(lv_disp_t* disp, const lv_font_t* font) {
  return init_mono_flat_theme_variant(disp, font, true, s_ui_ssd1306);
}

}  // namespace heltec::meshcore::ui
#endif  // LV_USE_THEME_MONO
