#pragma once
#if LV_USE_THEME_MONO
#include <lvgl.h>
namespace heltec::meshcore::ui
{
/** Heltec-owned mono theme; no LVGL parent theme is chained in. */
lv_theme_t *init_ui_mono_flat_theme(lv_disp_t *disp, const lv_font_t *font);

/** SSD1306 variant of the mono theme; same controls, tighter 128x64 layout. */
lv_theme_t *init_ui_ssd1306_theme(lv_disp_t *disp, const lv_font_t *font);

} // namespace heltec::meshcore::ui
#endif // LV_USE_THEME_MONO
