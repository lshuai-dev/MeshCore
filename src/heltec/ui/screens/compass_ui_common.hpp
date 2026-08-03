#pragma once

#include "compass_dial_widget.hpp"
#include "compass_needle.hpp"
#include "ui/core/ht_meta_data.hpp"
#include <lvgl.h>

namespace heltec::meshcore::ui {

inline void compass_style_info_label(lv_obj_t* lb, const char* text, lv_label_long_mode_t wrap) {
  if (!lb) return;
  lv_obj_set_width(lb, lv_pct(100));
  lv_label_set_long_mode(lb, wrap);
  // All callers pass either literals or storage owned by the screen. Keep
  // the label pointed at that storage so refreshes do not allocate/free a
  // new LVGL text buffer on a device that runs indefinitely.
  lv_label_set_text_static(lb, text ? text : "");
}

inline lv_obj_t* compass_create_info_column(lv_obj_t* parent, lv_coord_t width_pct, lv_coord_t pad_left,
                                            lv_coord_t pad_row) {
  if (!parent) return nullptr;
  lv_obj_t* info = ht_obj_create(parent, meta_id::CompassInfoColumn);
  if (!info) return nullptr;
  lv_obj_set_size(info, lv_pct(width_pct), lv_pct(100));
  lv_obj_set_flex_grow(info, 0);
  lv_obj_set_flex_flow(info, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(info, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_START);
  lv_obj_set_style_pad_all(info, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_left(info, pad_left, LV_PART_MAIN);
  lv_obj_set_style_pad_row(info, pad_row, LV_PART_MAIN);
  lv_obj_clear_flag(info, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(info, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
  return info;
}

inline void compass_init_dial_row(lv_obj_t* root, lv_coord_t pad_right) {
  if (!root) return;
  lv_obj_set_flex_flow(root, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(root, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_all(root, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_right(root, pad_right, LV_PART_MAIN);
  lv_obj_set_style_pad_column(root, 0, LV_PART_MAIN);
  lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
}

inline int compass_heading_tenths(float deg) {
  int t = (int)(deg * 10.f + 0.5f);
  t %= 3600;
  if (t < 0) t += 3600;
  return t;
}

#ifndef COMPASS_DIAL_REDRAW_STEP_TENTHS
#define COMPASS_DIAL_REDRAW_STEP_TENTHS 50
#endif

inline int16_t compass_dial_redraw_tenths(int16_t tenths) {
  int32_t t = tenths % 3600;
  if (t < 0) t += 3600;
  const int32_t step = (int32_t)COMPASS_DIAL_REDRAW_STEP_TENTHS;
  if (step <= 1) return dial_heading_tenths((int16_t)t);
  return (int16_t)(((t + step / 2) / step) * step % 3600);
}

inline float compass_screen_heading(float sensor_heading_deg, int heading_offset_deg) {
  return compass_wrap_degrees_360(sensor_heading_deg + (float)heading_offset_deg);
}

inline void compass_format_distance_m(char* buf, size_t len, double meters) {
  if (meters < 0.0) {
    lv_snprintf(buf, len, "--");
    return;
  }
  const uint64_t cm = (uint64_t)(meters * 100.0 + 0.5);
  if (meters > 1000.0) {
    const uint64_t km_centi = (cm + 500U) / 1000U;
    lv_snprintf(buf, len, "%lu.%02lukm", (unsigned long)(km_centi / 100U),
                (unsigned long)(km_centi % 100U));
  } else if (meters >= 100.0) {
    lv_snprintf(buf, len, "%lum", (unsigned long)((cm + 50U) / 100U));
  } else if (meters >= 10.0) {
    const uint64_t dm = (cm + 5U) / 10U;
    lv_snprintf(buf, len, "%lu.%lum", (unsigned long)(dm / 10U),
                (unsigned long)(dm % 10U));
  } else {
    lv_snprintf(buf, len, "%lu.%02lum", (unsigned long)(cm / 100U),
                (unsigned long)(cm % 100U));
  }
}

}  // namespace heltec::meshcore::ui
