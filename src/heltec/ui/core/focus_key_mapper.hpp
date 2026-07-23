#pragma once

#include <lvgl.h>
#include <stdint.h>

namespace heltec::meshcore::ui {

class FocusKeyMapper {
 public:
  static uint32_t translate(uint32_t lv_key);
  static uint32_t translateForGroup(lv_group_t* group, uint32_t lv_key);
  static uint32_t translateForObject(lv_obj_t* obj, uint32_t lv_key);
};

}  // namespace heltec::meshcore::ui
