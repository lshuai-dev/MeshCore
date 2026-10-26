#pragma once
#if LV_USE_THEME_MONO
#include <lvgl.h>
namespace Heltec::MeshCore::Ui
{
struct ThemePalette;

/** Heltec-owned mono theme; no LVGL parent theme is chained in. */
lv_theme_t *initMonoTheme(lv_disp_t *disp, const lv_font_t *font);

/** SSD1306 variant of the mono theme; same controls, tighter 128x64 layout. */
lv_theme_t *initSsd1306Theme(lv_disp_t *disp, const lv_font_t *font);

/** Shared mono renderer initialized with a device-owned color palette. */
lv_theme_t *initMonoThemeWithPalette(lv_disp_t *disp, const lv_font_t *font,
                                                const ThemePalette &colors);

} // namespace Heltec::MeshCore::Ui
#endif // LV_USE_THEME_MONO
