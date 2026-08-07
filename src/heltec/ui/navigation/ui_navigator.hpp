#pragma once

#include <lvgl.h>
#include <stdint.h>

#include "ui/core/ht_meta_data.hpp"

namespace heltec::meshcore::ui {

namespace meta_id {
constexpr MetaId NavigationRoot = ht_meta_id(MetaIdScope::Navigation, 0x00);
constexpr MetaId NavigationPanel = ht_meta_id(MetaIdScope::Navigation, 0x01);
constexpr MetaId NavigationContent = ht_meta_id(MetaIdScope::Navigation, 0x02);
constexpr MetaId NavigationCell = ht_meta_id(MetaIdScope::Navigation, 0x03);
constexpr MetaId NavigationIconArea = ht_meta_id(MetaIdScope::Navigation, 0x04);
constexpr MetaId NavigationIcon = ht_meta_id(MetaIdScope::Navigation, 0x05);
constexpr MetaId NavigationTitleBar = ht_meta_id(MetaIdScope::Navigation, 0x06);
constexpr MetaId NavigationTitleLabel = ht_meta_id(MetaIdScope::Navigation, 0x07);
constexpr MetaId NavigationRing = ht_meta_id(MetaIdScope::Navigation, 0x08);
constexpr MetaId NavigationRingItem = ht_meta_id(MetaIdScope::Navigation, 0x09);
}

/**
 * Static navigation item description used during UI construction.
 *
 * The array is owned by the caller and is only consumed while the
 * navigation surface is configured; NavigationPane does not retain it.
 */
struct UiNavigationItem {
  uint8_t screen_index;
  const char* label;
  const lv_img_dsc_t* icon;
  bool footer;
};

lv_obj_t* ui_navigator_create_grid(lv_obj_t* parent);
bool ui_navigator_is(lv_obj_t* obj);
bool ui_navigator_is_grid(lv_obj_t* obj);
lv_obj_t* ui_navigator_content(lv_obj_t* obj);

}  // namespace heltec::meshcore::ui
