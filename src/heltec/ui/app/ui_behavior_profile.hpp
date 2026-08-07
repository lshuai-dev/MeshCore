#pragma once

#include <lvgl.h>
#include <stdint.h>

namespace heltec::meshcore::ui {

struct UiContextMenuBehavior {
  uint16_t page_anim_ms;
};

struct UiNavigationBehavior {
  uint16_t auto_hide_ms;
  uint16_t ring_anim_ms;
  uint16_t open_anim_ms;
  lv_coord_t min_touch_pad;
  lv_coord_t focus_extra;
};

struct UiBehaviorProfile {
  UiContextMenuBehavior context_menu;
  UiNavigationBehavior navigation;
};

const UiBehaviorProfile& ui_behavior_profile();
void ui_set_behavior_profile(const UiBehaviorProfile* profile);

}  // namespace heltec::meshcore::ui
