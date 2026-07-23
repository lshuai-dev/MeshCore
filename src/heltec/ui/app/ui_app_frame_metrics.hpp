#pragma once

#include <lvgl.h>

namespace heltec::meshcore::ui {

struct UiAppFrameMetrics {
  lv_coord_t screen_pad;
  lv_coord_t frame_margin_left;
  lv_coord_t frame_margin_right;
  lv_coord_t frame_margin_top;
  lv_coord_t frame_margin_bottom;
  lv_coord_t content_radius;
};

}  // namespace heltec::meshcore::ui
