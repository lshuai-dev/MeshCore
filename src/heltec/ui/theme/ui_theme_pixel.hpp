#pragma once

#include <lvgl.h>

namespace heltec::meshcore::ui {

/** Heltec-owned pixel theme; no LVGL default parent theme is chained in. */
lv_theme_t* init_ui_pixel_theme(lv_disp_t* disp, const lv_font_t* font);

void ui_pixel_apply_switch_row_focus(lv_obj_t* row, lv_obj_t* sw);

}  // namespace heltec::meshcore::ui
