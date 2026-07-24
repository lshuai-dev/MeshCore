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

struct UiNavigationMetrics {
  uint16_t auto_hide_ms;
  uint16_t ring_anim_ms;
  uint16_t open_anim_ms;
  lv_coord_t min_touch_pad;
  lv_coord_t focus_extra;
  lv_coord_t ring_edge_pad;
  uint8_t grid_cols;
  uint8_t grid_rows;
  lv_coord_t grid_gap;
  lv_coord_t grid_pad;
  lv_coord_t grid_label_h;
  lv_coord_t grid_cell_radius;
  lv_coord_t grid_footer_h;
};

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
