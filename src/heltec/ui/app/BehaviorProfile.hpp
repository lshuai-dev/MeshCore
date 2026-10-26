#pragma once

#include <lvgl.h>
#include <stdint.h>

namespace Heltec::MeshCore::Ui
{

struct UiContextMenuBehavior
{
    uint16_t page_anim_ms;
};

struct UiNavigationBehavior
{
    uint16_t auto_hide_ms;
    uint16_t ring_anim_ms;
    uint16_t open_anim_ms;
    lv_coord_t min_touch_pad;
    lv_coord_t focus_extra;
};

struct UiBehaviorProfile
{
    UiContextMenuBehavior context_menu;
    UiNavigationBehavior navigation;
};

const UiBehaviorProfile &behaviorProfile();
void setBehaviorProfile(const UiBehaviorProfile *profile);

} // namespace Heltec::MeshCore::Ui
