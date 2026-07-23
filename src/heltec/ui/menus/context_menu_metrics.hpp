#pragma once

#include <lvgl.h>
#include <stdint.h>

namespace heltec::meshcore::ui {

struct UiContextMenuMetrics {
  lv_coord_t icon_btn;
  lv_coord_t icon_pad;
  lv_coord_t border_width;
  lv_coord_t frame_pad;
  lv_coord_t title_border_width;
  lv_coord_t border_radius;
  uint16_t page_anim_ms;
};

}  // namespace heltec::meshcore::ui
