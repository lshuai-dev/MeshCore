#include "ui/app/ui_theme.hpp"

#include <lvgl.h>

#if LV_USE_THEME_DEFAULT
#include "extra/themes/default/lv_theme_default.h"
#endif

#if LV_USE_THEME_MONO && defined(UI_THEME_MONO) && UI_THEME_MONO
#include "ui/theme/ui_theme_mono_flat.hpp"
#endif

#if defined(UI_THEME_COLOR) && UI_THEME_COLOR
#include "ui/theme/ui_theme_pixel.hpp"
#endif

namespace heltec::meshcore::ui {

namespace {
static const UiThemeColors* s_active_colors = nullptr;

static const UiThemeColors& fallback_colors() {
  static const UiThemeColors colors = make_ui_theme_colors();
  return colors;
}

}  // namespace

UiThemeColors make_ui_theme_colors() {
  return {
      0xFFFFFF,
      0x000000,
      0x000000,
      0xFFFFFF,
      0x000000,
      0x00E060,
      0xFF3030,
      0xFFB000,
      0x00E060,
      0xFFB000,
      0xFF3030,
      0x00FF00,
      0xFFFF00,
      0x000000,
      0xFFFFFF,
      0xFFFFFF,
      0x000000,
      0x202020,
      0x303030,
      0x000000,
      0xFFFFFF,
      0xFFFFFF,
      0x000000,
      LV_OPA_COVER,
      0x000000,
      LV_OPA_COVER,
      2,
  };
}

const UiThemeColors& ui_theme_colors() {
  return s_active_colors ? *s_active_colors : fallback_colors();
}

void ui_set_active_theme_colors(const UiThemeColors* colors) {
  s_active_colors = colors;
}

bool ui_theme_init(lv_disp_t* disp) {
  if (!disp) return false;

#if defined(UI_THEME_COLOR) && UI_THEME_COLOR
  const lv_font_t* ui_font =
#if defined(LV_FONT_UNSCII_8) && LV_FONT_UNSCII_8
      &lv_font_unscii_8;
#else
      LV_FONT_DEFAULT;
#endif
  return init_ui_pixel_theme(disp, ui_font) != nullptr;

#elif LV_USE_THEME_MONO && defined(UI_THEME_MONO) && UI_THEME_MONO
  const lv_font_t* ui_font =
#if defined(LV_FONT_UNSCII_8) && LV_FONT_UNSCII_8
      &lv_font_unscii_8;
#else
      LV_FONT_DEFAULT;
#endif
#if (defined(UI_THEME_SSD1306) && UI_THEME_SSD1306) || \
    (defined(HELTEC_DISPLAY_SSD1306) && HELTEC_DISPLAY_SSD1306) || LV_COLOR_DEPTH == 1
  return init_ui_ssd1306_theme(disp, ui_font) != nullptr;
#else
  return init_ui_mono_flat_theme(disp, ui_font) != nullptr;
#endif

#elif LV_USE_THEME_DEFAULT
  static UiThemeColors default_colors = make_ui_theme_colors();
  ui_set_active_theme_colors(&default_colors);
  const bool dark_theme = (ui_theme_colors().bg == 0x000000);
  lv_theme_t* th = lv_theme_default_init(
      disp,
      lv_palette_main(LV_PALETTE_BLUE),
      lv_palette_main(LV_PALETTE_GREY),
      dark_theme,
      LV_FONT_DEFAULT);
  if (!th) return false;
  return true;

#else
  (void)disp;
  static UiThemeColors default_colors = make_ui_theme_colors();
  ui_set_active_theme_colors(&default_colors);
  return true;
#endif
}

}  // namespace heltec::meshcore::ui
