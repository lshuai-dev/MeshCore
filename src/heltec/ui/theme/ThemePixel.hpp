#pragma once

#include <lvgl.h>

namespace Heltec::MeshCore::Ui
{

/** Heltec-owned pixel theme; no LVGL default parent theme is chained in. */
lv_theme_t *initPixelTheme(lv_disp_t *disp, const lv_font_t *font);


} // namespace Heltec::MeshCore::Ui
