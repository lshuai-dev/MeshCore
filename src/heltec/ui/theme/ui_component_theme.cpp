#include "ui/theme/ui_widget_theme.hpp"

#include "ui/app/ui_theme.hpp"
#include "ui/core/ht_meta_data.hpp"
#include "ui/core/license_gate_ids.hpp"
#include "ui/map/map_panel_ids.hpp"
#include "ui/screens/compass_dial_widget.hpp"
#include "ui/widgets/button_roller.hpp"

namespace heltec::meshcore::ui {
namespace {

lv_style_t s_license_gate_root_style;
lv_style_t s_license_gate_label_style;
lv_style_t s_license_gate_chip_style;
bool s_license_gate_styles_ready = false;

void init_license_gate_styles() {
  if (s_license_gate_styles_ready) return;

  lv_style_init(&s_license_gate_root_style);
  lv_style_set_width(&s_license_gate_root_style, lv_pct(100));
  lv_style_set_height(&s_license_gate_root_style, lv_pct(100));
  lv_style_set_radius(&s_license_gate_root_style, 0);
#if defined(HELTEC_V4_R8_TFT)
  lv_style_set_pad_all(&s_license_gate_root_style, 0);
  lv_style_set_border_width(&s_license_gate_root_style, 0);
  lv_style_set_outline_width(&s_license_gate_root_style, 0);
#else
  lv_style_set_pad_all(&s_license_gate_root_style, 8);
#endif
  lv_style_set_bg_color(&s_license_gate_root_style, lv_color_black());
  lv_style_set_bg_opa(&s_license_gate_root_style, LV_OPA_COVER);
  lv_style_set_layout(&s_license_gate_root_style, LV_LAYOUT_FLEX);
  lv_style_set_flex_flow(&s_license_gate_root_style, LV_FLEX_FLOW_COLUMN);
  lv_style_set_flex_main_place(&s_license_gate_root_style,
                               LV_FLEX_ALIGN_CENTER);
  lv_style_set_flex_cross_place(&s_license_gate_root_style,
                                LV_FLEX_ALIGN_CENTER);
  lv_style_set_flex_track_place(&s_license_gate_root_style,
                                LV_FLEX_ALIGN_CENTER);
#if defined(HELTEC_V4_R8_TFT)
  lv_style_set_pad_row(&s_license_gate_root_style, 0);
  lv_style_set_pad_column(&s_license_gate_root_style, 0);
#else
  lv_style_set_pad_row(&s_license_gate_root_style, 12);
#endif

  lv_style_init(&s_license_gate_label_style);
  lv_style_set_text_color(&s_license_gate_label_style, lv_color_white());
  lv_style_set_text_align(&s_license_gate_label_style, LV_TEXT_ALIGN_CENTER);
#if defined(HELTEC_V4_R8_TFT)
  lv_style_set_pad_all(&s_license_gate_label_style, 0);
  lv_style_set_border_width(&s_license_gate_label_style, 0);
  lv_style_set_outline_width(&s_license_gate_label_style, 0);
#endif

  lv_style_init(&s_license_gate_chip_style);
  lv_style_set_width(&s_license_gate_chip_style, lv_pct(100));
#if defined(HELTEC_V4_R8_TFT)
  lv_style_set_pad_all(&s_license_gate_chip_style, 0);
  lv_style_set_border_width(&s_license_gate_chip_style, 0);
  lv_style_set_outline_width(&s_license_gate_chip_style, 0);
#endif

  s_license_gate_styles_ready = true;
}

#if !defined(UI_NAVIGATION_GRID) || !UI_NAVIGATION_GRID
struct RadialNavigationIconStyleSet {
  bool ready = false;
  lv_opa_t recolor_opa = LV_OPA_TRANSP;
  lv_style_t idle;
  lv_style_t focus;
};

RadialNavigationIconStyleSet s_radial_navigation_icon_styles[2];

bool radial_navigation_icon_uses_recolor(const lv_img_dsc_t* image) {
  if (!image) return true;
  switch (image->header.cf) {
    case LV_IMG_CF_ALPHA_1BIT:
    case LV_IMG_CF_ALPHA_2BIT:
    case LV_IMG_CF_ALPHA_4BIT:
    case LV_IMG_CF_ALPHA_8BIT:
    case LV_IMG_CF_INDEXED_1BIT:
    case LV_IMG_CF_INDEXED_2BIT:
    case LV_IMG_CF_INDEXED_4BIT:
    case LV_IMG_CF_INDEXED_8BIT:
      return true;
    default:
      return false;
  }
}

RadialNavigationIconStyleSet* radial_navigation_icon_styles(
    lv_opa_t recolor_opa) {
  for (RadialNavigationIconStyleSet& slot :
       s_radial_navigation_icon_styles) {
    if (slot.ready && slot.recolor_opa == recolor_opa) return &slot;
  }
  for (RadialNavigationIconStyleSet& slot :
       s_radial_navigation_icon_styles) {
    if (slot.ready) continue;
    slot.ready = true;
    slot.recolor_opa = recolor_opa;

    lv_style_init(&slot.idle);
    lv_style_set_bg_opa(&slot.idle, LV_OPA_TRANSP);
    lv_style_set_border_width(&slot.idle, 0);
    lv_style_set_shadow_width(&slot.idle, 0);
    lv_style_set_pad_all(&slot.idle, 0);
    lv_style_set_img_recolor(&slot.idle, ui_color_fg());
    lv_style_set_img_recolor_opa(&slot.idle, recolor_opa);

    lv_style_init(&slot.focus);
    lv_style_set_bg_opa(&slot.focus, LV_OPA_TRANSP);
    lv_style_set_border_width(&slot.focus, 0);
    lv_style_set_img_recolor(&slot.focus, ui_color_accent());
    lv_style_set_img_recolor_opa(&slot.focus, recolor_opa);
    return &slot;
  }
  return nullptr;
}
#endif

lv_style_t s_roller_root_style;
lv_style_t s_roller_button_style;
lv_style_t s_roller_button_selected_style;
lv_style_t s_roller_label_style;
lv_style_t s_roller_label_selected_style;
bool s_roller_styles_ready = false;
UiButtonRollerStyleConfig s_roller_style_config = {0, 2, 12};

static const lv_style_selector_t kRollerItemSelectedSelectors[] = {
    LV_PART_MAIN | LV_STATE_CHECKED,
    LV_PART_MAIN | LV_STATE_FOCUSED,
    LV_PART_MAIN | LV_STATE_FOCUS_KEY,
    LV_PART_MAIN | (LV_STATE_CHECKED | LV_STATE_FOCUSED),
    LV_PART_MAIN | (LV_STATE_CHECKED | LV_STATE_FOCUS_KEY),
    LV_PART_MAIN | LV_STATE_PRESSED,
};

static lv_style_t s_cascading_menu_page_style;
static lv_style_t s_cascading_menu_item_style;
static bool s_cascading_menu_styles_ready = false;

static const lv_style_selector_t kCascadingMenuPageSelectors[] = {
    LV_PART_MAIN,
    LV_PART_MAIN | LV_STATE_FOCUSED,
    LV_PART_MAIN | LV_STATE_FOCUS_KEY,
    LV_PART_MAIN | LV_STATE_PRESSED,
};

static const lv_style_selector_t kCascadingMenuItemSelectors[] = {
    LV_PART_MAIN,
    LV_PART_MAIN | LV_STATE_CHECKED,
    LV_PART_MAIN | LV_STATE_FOCUSED,
    LV_PART_MAIN | LV_STATE_FOCUS_KEY,
    LV_PART_MAIN | LV_STATE_PRESSED,
};

void init_cascading_menu_styles() {
  if (s_cascading_menu_styles_ready) return;

  lv_style_init(&s_cascading_menu_page_style);
  lv_style_set_width(&s_cascading_menu_page_style, lv_pct(100));
  lv_style_set_height(&s_cascading_menu_page_style, lv_pct(100));
  lv_style_set_pad_all(&s_cascading_menu_page_style, 0);
  lv_style_set_pad_row(&s_cascading_menu_page_style, 0);
  lv_style_set_border_width(&s_cascading_menu_page_style, 0);
  lv_style_set_outline_width(&s_cascading_menu_page_style, 0);

  lv_style_init(&s_cascading_menu_item_style);
  lv_style_set_pad_left(&s_cascading_menu_item_style, 4);
  lv_style_set_pad_right(&s_cascading_menu_item_style, 4);

  s_cascading_menu_styles_ready = true;
}

void init_roller_styles() {
  if (s_roller_styles_ready) return;
  const UiButtonRollerStyleConfig& geometry = s_roller_style_config;

  lv_style_init(&s_roller_root_style);
  lv_style_set_border_width(&s_roller_root_style, geometry.border_width);
  lv_style_set_pad_all(&s_roller_root_style, geometry.pad);
  lv_style_set_bg_opa(&s_roller_root_style, LV_OPA_TRANSP);

  lv_style_init(&s_roller_button_style);
  lv_style_set_height(&s_roller_button_style, geometry.button_height);
  lv_style_set_pad_all(&s_roller_button_style, geometry.pad);
  lv_style_set_border_width(&s_roller_button_style, geometry.border_width);
  lv_style_set_bg_opa(&s_roller_button_style, LV_OPA_TRANSP);
  lv_style_set_text_color(&s_roller_button_style, ui_color_fg());
  lv_style_set_shadow_width(&s_roller_button_style, 0);
  lv_style_set_radius(&s_roller_button_style, 0);

  lv_style_init(&s_roller_button_selected_style);
  lv_style_set_border_width(&s_roller_button_selected_style, 0);
  lv_style_set_outline_width(&s_roller_button_selected_style, 0);
  lv_style_set_shadow_width(&s_roller_button_selected_style, 0);
  lv_style_set_radius(&s_roller_button_selected_style, 0);
  lv_style_set_bg_color(&s_roller_button_selected_style,
                        ui_color_highlight_bg());
  lv_style_set_bg_opa(&s_roller_button_selected_style, LV_OPA_COVER);
  lv_style_set_text_color(&s_roller_button_selected_style,
                          ui_color_highlight_fg());

  lv_style_init(&s_roller_label_style);
  lv_style_set_text_color(&s_roller_label_style, ui_color_fg());
  lv_style_set_text_line_space(&s_roller_label_style, 0);
  lv_style_set_text_letter_space(&s_roller_label_style, 0);

  lv_style_init(&s_roller_label_selected_style);
  lv_style_set_text_color(&s_roller_label_selected_style,
                          ui_color_highlight_fg());
  lv_style_set_text_line_space(&s_roller_label_selected_style, 0);
  lv_style_set_text_letter_space(&s_roller_label_selected_style, 0);

  s_roller_styles_ready = true;
}

void style_roller_root(lv_obj_t* obj) {
  init_roller_styles();
  lv_obj_add_style(obj, &s_roller_root_style, LV_PART_MAIN);
}

void style_roller_button(lv_obj_t* obj) {
  init_roller_styles();
  lv_obj_add_style(obj, &s_roller_button_style, LV_PART_MAIN);
  for (lv_style_selector_t selector : kRollerItemSelectedSelectors) {
    lv_obj_add_style(obj, &s_roller_button_selected_style, selector);
  }
}

void style_roller_label(lv_obj_t* obj) {
  init_roller_styles();
  lv_obj_add_style(obj, &s_roller_label_style, LV_PART_MAIN);
  for (lv_style_selector_t selector : kRollerItemSelectedSelectors) {
    lv_obj_add_style(obj, &s_roller_label_selected_style, selector);
  }
}

#if defined(ENV_INCLUDE_MAP) && ENV_INCLUDE_MAP
static lv_style_t s_map_status_label_style;
static lv_style_t s_map_opaque_surface_style;
static lv_style_t s_map_viewport_layout_style;
static lv_style_t s_map_toolbar_style;
static lv_style_t s_map_toolbar_button_style;
static lv_style_t s_map_toolbar_button_checked_style;
static lv_style_t s_map_toolbar_button_pressed_style;
static lv_style_t s_map_toolbar_button_disabled_style;
static lv_style_t s_map_toolbar_label_style;
static lv_style_t s_map_toolbar_label_checked_style;
static lv_style_t s_map_toolbar_label_pressed_style;
static lv_style_t s_map_toolbar_label_disabled_style;
static lv_style_t s_map_transparent_layer_style;
static lv_style_t s_map_marker_style;
static lv_style_t s_map_range_ring_style;
static lv_style_t s_map_tile_placeholder_style;
static lv_style_t s_map_marker_label_style;
static lv_style_t s_map_range_label_style;
static bool s_map_styles_ready = false;

struct MapColorStyleSlot {
  bool ready = false;
  uint32_t color = 0;
  lv_style_t style;
};

struct MapOpaStyleSlot {
  bool ready = false;
  lv_opa_t opa = LV_OPA_TRANSP;
  lv_style_t style;
};

static MapColorStyleSlot s_map_marker_color_styles[8];
static MapOpaStyleSlot s_map_range_opa_styles[8];

void init_map_styles() {
  if (s_map_styles_ready) return;

  lv_style_init(&s_map_status_label_style);
  lv_style_set_text_color(&s_map_status_label_style, ui_color_fg());
  lv_style_set_text_align(&s_map_status_label_style, LV_TEXT_ALIGN_LEFT);

  lv_style_init(&s_map_opaque_surface_style);
  lv_style_set_bg_opa(&s_map_opaque_surface_style, LV_OPA_COVER);
  lv_style_set_border_width(&s_map_opaque_surface_style, 0);
  lv_style_set_pad_all(&s_map_opaque_surface_style, 0);

  lv_style_init(&s_map_viewport_layout_style);
  lv_style_set_min_height(&s_map_viewport_layout_style, 140);

  lv_style_init(&s_map_toolbar_style);
  lv_style_set_radius(&s_map_toolbar_style, 4);
  lv_style_set_bg_opa(&s_map_toolbar_style, LV_OPA_TRANSP);
  lv_style_set_border_width(&s_map_toolbar_style, 0);
  lv_style_set_shadow_width(&s_map_toolbar_style, 0);
  lv_style_set_pad_all(&s_map_toolbar_style, 3);
  lv_style_set_pad_column(&s_map_toolbar_style, 3);

  lv_style_init(&s_map_toolbar_button_style);
  lv_style_set_radius(&s_map_toolbar_button_style, 3);
  lv_style_set_border_width(&s_map_toolbar_button_style, 1);
  lv_style_set_border_color(&s_map_toolbar_button_style, ui_color_accent());
  lv_style_set_border_opa(&s_map_toolbar_button_style, LV_OPA_COVER);
  lv_style_set_shadow_width(&s_map_toolbar_button_style, 0);
  lv_style_set_bg_opa(&s_map_toolbar_button_style, LV_OPA_TRANSP);
  lv_style_set_pad_all(&s_map_toolbar_button_style, 0);

  lv_style_init(&s_map_toolbar_button_checked_style);
  lv_style_set_border_color(&s_map_toolbar_button_checked_style,
                            ui_color_success());
  lv_style_set_bg_opa(&s_map_toolbar_button_checked_style, LV_OPA_TRANSP);

  lv_style_init(&s_map_toolbar_button_pressed_style);
  lv_style_set_border_color(&s_map_toolbar_button_pressed_style,
                            ui_color_error());
  lv_style_set_bg_opa(&s_map_toolbar_button_pressed_style, LV_OPA_TRANSP);

  lv_style_init(&s_map_toolbar_button_disabled_style);
  lv_style_set_border_opa(&s_map_toolbar_button_disabled_style,
                          ui_effective_opa(LV_OPA_50));
  lv_style_set_bg_opa(&s_map_toolbar_button_disabled_style, LV_OPA_TRANSP);

  lv_style_init(&s_map_toolbar_label_style);
  lv_style_set_text_color(&s_map_toolbar_label_style, ui_color_accent());
  lv_style_set_text_font(&s_map_toolbar_label_style, LV_FONT_DEFAULT);

  lv_style_init(&s_map_toolbar_label_checked_style);
  lv_style_set_text_color(&s_map_toolbar_label_checked_style,
                          ui_color_success());

  lv_style_init(&s_map_toolbar_label_pressed_style);
  lv_style_set_text_color(&s_map_toolbar_label_pressed_style, ui_color_error());

  lv_style_init(&s_map_toolbar_label_disabled_style);
  lv_style_set_text_color(&s_map_toolbar_label_disabled_style,
                          lv_color_hex(0x607080));

  lv_style_init(&s_map_transparent_layer_style);
  lv_style_set_bg_opa(&s_map_transparent_layer_style, LV_OPA_TRANSP);
  lv_style_set_border_width(&s_map_transparent_layer_style, 0);
  lv_style_set_pad_all(&s_map_transparent_layer_style, 0);

  lv_style_init(&s_map_marker_style);
  lv_style_set_radius(&s_map_marker_style, LV_RADIUS_CIRCLE);
  lv_style_set_bg_color(&s_map_marker_style, ui_color_success());
  lv_style_set_bg_opa(&s_map_marker_style, LV_OPA_COVER);
  lv_style_set_border_width(&s_map_marker_style, 1);
  lv_style_set_border_color(&s_map_marker_style, ui_color_fg());

  lv_style_init(&s_map_range_ring_style);
  lv_style_set_radius(&s_map_range_ring_style, LV_RADIUS_CIRCLE);
  lv_style_set_bg_opa(&s_map_range_ring_style, LV_OPA_TRANSP);
  lv_style_set_border_width(&s_map_range_ring_style, 1);
  lv_style_set_border_color(&s_map_range_ring_style, ui_color_fg());

  lv_style_init(&s_map_tile_placeholder_style);
  lv_style_set_text_color(&s_map_tile_placeholder_style,
                          lv_color_hex(0x606060));
  lv_style_set_text_align(&s_map_tile_placeholder_style,
                          LV_TEXT_ALIGN_CENTER);

  lv_style_init(&s_map_marker_label_style);
  lv_style_set_text_color(&s_map_marker_label_style, ui_color_bg());

  lv_style_init(&s_map_range_label_style);
  lv_style_set_text_color(&s_map_range_label_style, ui_color_fg());
  lv_style_set_bg_opa(&s_map_range_label_style, LV_OPA_TRANSP);
  lv_style_set_pad_all(&s_map_range_label_style, 0);

  s_map_styles_ready = true;
}

lv_style_t* marker_color_style(lv_color_t color) {
  const uint32_t color32 = lv_color_to32(color);
  for (MapColorStyleSlot& slot : s_map_marker_color_styles) {
    if (slot.ready && slot.color == color32) return &slot.style;
  }
  for (MapColorStyleSlot& slot : s_map_marker_color_styles) {
    if (slot.ready) continue;
    slot.ready = true;
    slot.color = color32;
    lv_style_init(&slot.style);
    lv_style_set_bg_color(&slot.style, color);
    return &slot.style;
  }
  return nullptr;
}

lv_style_t* range_opa_style(lv_opa_t opa) {
  for (MapOpaStyleSlot& slot : s_map_range_opa_styles) {
    if (slot.ready && slot.opa == opa) return &slot.style;
  }
  for (MapOpaStyleSlot& slot : s_map_range_opa_styles) {
    if (slot.ready) continue;
    slot.ready = true;
    slot.opa = opa;
    lv_style_init(&slot.style);
    lv_style_set_border_opa(&slot.style, ui_effective_opa(opa));
    return &slot.style;
  }
  return nullptr;
}
#endif

#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
static lv_style_t s_compass_plain_overlay_style;
static lv_style_t s_compass_hub_style;
static lv_style_t s_compass_needle_style;
static lv_style_t s_compass_label_style;
static lv_style_t s_compass_q_success_style;
static lv_style_t s_compass_q_error_style;
static bool s_compass_styles_ready = false;

void init_compass_styles() {
  if (s_compass_styles_ready) return;

  lv_style_init(&s_compass_plain_overlay_style);
  lv_style_set_bg_opa(&s_compass_plain_overlay_style, LV_OPA_TRANSP);
  lv_style_set_border_width(&s_compass_plain_overlay_style, 0);
  lv_style_set_pad_all(&s_compass_plain_overlay_style, 0);

  lv_style_init(&s_compass_hub_style);
  lv_style_set_bg_color(&s_compass_hub_style, ui_color_panel_dark_bg());
  lv_style_set_bg_opa(&s_compass_hub_style, LV_OPA_COVER);
  lv_style_set_radius(&s_compass_hub_style, LV_RADIUS_CIRCLE);
  lv_style_set_border_width(&s_compass_hub_style, 1);
  lv_style_set_border_color(&s_compass_hub_style, ui_color_fg_on_dark());
  lv_style_set_border_opa(&s_compass_hub_style, LV_OPA_COVER);
  lv_style_set_clip_corner(&s_compass_hub_style, true);
  lv_style_set_pad_all(&s_compass_hub_style, 0);

  lv_style_init(&s_compass_needle_style);
  lv_style_set_radius(&s_compass_needle_style, LV_RADIUS_CIRCLE);
  lv_style_set_clip_corner(&s_compass_needle_style, true);
  lv_style_set_pad_all(&s_compass_needle_style, 0);

  lv_style_init(&s_compass_label_style);
  lv_style_set_text_color(&s_compass_label_style, ui_color_fg());
  lv_style_set_text_align(&s_compass_label_style, LV_TEXT_ALIGN_LEFT);

  lv_style_init(&s_compass_q_success_style);
  lv_style_set_text_color(&s_compass_q_success_style, ui_color_success());

  lv_style_init(&s_compass_q_error_style);
  lv_style_set_text_color(&s_compass_q_error_style, ui_color_error());

  s_compass_styles_ready = true;
}

void style_compass_plain_overlay(_lv_obj_t* obj) {
  init_compass_styles();
  lv_obj_add_style(obj, &s_compass_plain_overlay_style, LV_PART_MAIN);
}
#endif

}  // namespace

bool ui_license_gate_apply_theme(_lv_obj_t* obj) {
  if (!obj) return false;

  switch (ht_id(obj)) {
    case meta_id::LicenseGateRoot:
      init_license_gate_styles();
      lv_obj_add_style(obj, &s_license_gate_root_style, LV_PART_MAIN);
      return true;
    case meta_id::LicenseGateTitle:
      init_license_gate_styles();
      lv_obj_add_style(obj, &s_license_gate_label_style, LV_PART_MAIN);
      return true;
    case meta_id::LicenseGateChip:
      init_license_gate_styles();
      lv_obj_add_style(obj, &s_license_gate_label_style, LV_PART_MAIN);
      lv_obj_add_style(obj, &s_license_gate_chip_style, LV_PART_MAIN);
      return true;
    default:
      return false;
  }
}

void ui_theme_apply_radial_navigation_icon(_lv_obj_t* button,
                                           const lv_img_dsc_t* image) {
#if !defined(UI_NAVIGATION_GRID) || !UI_NAVIGATION_GRID
  if (!button) return;
  lv_obj_remove_style_all(button);
  const lv_opa_t recolor_opa =
      radial_navigation_icon_uses_recolor(image) ? LV_OPA_COVER
                                                 : LV_OPA_TRANSP;
  RadialNavigationIconStyleSet* styles =
      radial_navigation_icon_styles(recolor_opa);
  if (!styles) return;
  static const lv_style_selector_t idle_selectors[] = {
      LV_PART_MAIN,
      LV_PART_MAIN | LV_STATE_PRESSED,
  };
  static const lv_style_selector_t focus_selectors[] = {
      LV_PART_MAIN | LV_STATE_FOCUS_KEY,
      LV_PART_MAIN | LV_STATE_FOCUS_KEY | LV_STATE_PRESSED,
      LV_PART_MAIN | LV_STATE_FOCUSED,
      LV_PART_MAIN | LV_STATE_FOCUSED | LV_STATE_PRESSED,
  };
  for (lv_style_selector_t selector : idle_selectors) {
    lv_obj_add_style(button, &styles->idle, selector);
  }
  for (lv_style_selector_t selector : focus_selectors) {
    lv_obj_add_style(button, &styles->focus, selector);
  }
#else
  (void)button;
  (void)image;
#endif
}

void ui_widget_theme_set_button_roller_style(
    const UiButtonRollerStyleConfig& config) {
  if (s_roller_styles_ready) return;
  s_roller_style_config = config;
}

bool ui_button_roller_apply_theme(_lv_obj_t* obj) {
  if (!obj) return false;

  switch (ht_id(obj)) {
    case meta_id::ButtonRollerRoot:
      style_roller_root(obj);
      return true;
    case meta_id::ButtonRollerItem:
      style_roller_button(obj);
      return true;
    case meta_id::ButtonRollerLabel:
      style_roller_label(obj);
      return true;
    default:
      return false;
  }
}

void ui_theme_apply_cascading_menu_page(_lv_obj_t* obj) {
  if (!obj) return;
  init_cascading_menu_styles();
  for (lv_style_selector_t selector : kCascadingMenuPageSelectors) {
    lv_obj_add_style(obj, &s_cascading_menu_page_style, selector);
  }
}

void ui_theme_apply_cascading_menu_item(_lv_obj_t* obj) {
  if (!obj) return;
  init_cascading_menu_styles();
  for (lv_style_selector_t selector : kCascadingMenuItemSelectors) {
    lv_obj_add_style(obj, &s_cascading_menu_item_style, selector);
  }
}

#if defined(ENV_INCLUDE_MAP) && ENV_INCLUDE_MAP
bool ui_map_widget_apply_theme(_lv_obj_t* obj) {
  if (!obj) return false;
  init_map_styles();

  switch (ht_id(obj)) {
    case meta_id::MapStatusLabel:
      lv_obj_add_style(obj, &s_map_status_label_style, LV_PART_MAIN);
      return true;
    case meta_id::MapViewport:
      lv_obj_add_style(obj, &s_map_opaque_surface_style, LV_PART_MAIN);
      lv_obj_add_style(obj, &s_map_viewport_layout_style, LV_PART_MAIN);
      return true;
    case meta_id::MapToolbar:
      lv_obj_add_style(obj, &s_map_toolbar_style, LV_PART_MAIN);
      return true;
    case meta_id::MapToolbarButton:
      lv_obj_add_style(obj, &s_map_toolbar_button_style, LV_PART_MAIN);
      lv_obj_add_style(obj, &s_map_toolbar_button_checked_style,
                       LV_PART_MAIN | LV_STATE_CHECKED);
      lv_obj_add_style(obj, &s_map_toolbar_button_pressed_style,
                       LV_PART_MAIN | LV_STATE_PRESSED);
      lv_obj_add_style(obj, &s_map_toolbar_button_disabled_style,
                       LV_PART_MAIN | LV_STATE_DISABLED);
      return true;
    case meta_id::MapToolbarButtonLabel:
      lv_obj_add_style(obj, &s_map_toolbar_label_style, LV_PART_MAIN);
      lv_obj_add_style(obj, &s_map_toolbar_label_checked_style,
                       LV_PART_MAIN | LV_STATE_CHECKED);
      lv_obj_add_style(obj, &s_map_toolbar_label_pressed_style,
                       LV_PART_MAIN | LV_STATE_PRESSED);
      lv_obj_add_style(obj, &s_map_toolbar_label_disabled_style,
                       LV_PART_MAIN | LV_STATE_DISABLED);
      return true;
    case meta_id::MapTileLayer:
    case meta_id::MapRangeLayer:
    case meta_id::MapMarkerLayer:
      lv_obj_add_style(obj, &s_map_transparent_layer_style, LV_PART_MAIN);
      return true;
    case meta_id::MapTile:
      lv_obj_add_style(obj, &s_map_opaque_surface_style, LV_PART_MAIN);
      return true;
    case meta_id::MapTileImage:
      return true;
    case meta_id::MapMarker:
      lv_obj_add_style(obj, &s_map_marker_style, LV_PART_MAIN);
      return true;
    case meta_id::MapRangeRing:
      lv_obj_add_style(obj, &s_map_range_ring_style, LV_PART_MAIN);
      return true;
    case meta_id::MapTilePlaceholder:
      lv_obj_add_style(obj, &s_map_tile_placeholder_style, LV_PART_MAIN);
      return true;
    case meta_id::MapMarkerLabel:
      lv_obj_add_style(obj, &s_map_marker_label_style, LV_PART_MAIN);
      return true;
    case meta_id::MapRangeLabel:
      lv_obj_add_style(obj, &s_map_range_label_style, LV_PART_MAIN);
      return true;
    default:
      return false;
  }
}

void ui_map_marker_apply_color(_lv_obj_t* obj, lv_color_t color) {
  if (!obj) return;
  if (lv_style_t* style = marker_color_style(color)) {
    lv_obj_add_style(obj, style, LV_PART_MAIN);
  }
}

void ui_map_range_ring_apply_opa(_lv_obj_t* obj, lv_opa_t opa) {
  if (!obj) return;
  if (lv_style_t* style = range_opa_style(opa)) {
    lv_obj_add_style(obj, style, LV_PART_MAIN);
  }
}
#else
bool ui_map_widget_apply_theme(_lv_obj_t* obj) {
  (void)obj;
  return false;
}

void ui_map_marker_apply_color(_lv_obj_t* obj, lv_color_t color) {
  (void)obj;
  (void)color;
}

void ui_map_range_ring_apply_opa(_lv_obj_t* obj, lv_opa_t opa) {
  (void)obj;
  (void)opa;
}
#endif

#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
void ui_theme_set_compass_info_layout(_lv_obj_t* obj, lv_coord_t pad_left,
                                      lv_coord_t pad_row) {
  if (!obj) return;
  lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_left(obj, pad_left, LV_PART_MAIN);
  lv_obj_set_style_pad_row(obj, pad_row, LV_PART_MAIN);
}

void ui_theme_set_compass_dial_row_layout(_lv_obj_t* obj,
                                          lv_coord_t pad_right) {
  if (!obj) return;
  lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_right(obj, pad_right, LV_PART_MAIN);
  lv_obj_set_style_pad_column(obj, 0, LV_PART_MAIN);
}

bool ui_compass_widget_apply_theme(_lv_obj_t* obj) {
  if (!obj) return false;
  init_compass_styles();

  switch (ht_id(obj)) {
    case meta_id::CompassInfoColumn:
    case meta_id::CompassQRow:
    case meta_id::CompassDialCenter:
    case meta_id::CompassDialRing:
      style_compass_plain_overlay(obj);
      return true;
    case meta_id::CompassDialHub:
      lv_obj_add_style(obj, &s_compass_hub_style, LV_PART_MAIN);
      return true;
    case meta_id::CompassDialNeedle:
      style_compass_plain_overlay(obj);
      lv_obj_add_style(obj, &s_compass_needle_style, LV_PART_MAIN);
      return true;
    case meta_id::CompassInfoLabel:
      lv_obj_add_style(obj, &s_compass_label_style, LV_PART_MAIN);
      return true;
    case meta_id::CompassQLabel:
      lv_obj_add_style(obj, &s_compass_label_style, LV_PART_MAIN);
      lv_obj_add_style(obj, &s_compass_q_success_style,
                       LV_PART_MAIN | LV_STATE_USER_1);
      lv_obj_add_style(obj, &s_compass_q_error_style,
                       LV_PART_MAIN | LV_STATE_USER_2);
      return true;
    default:
      return false;
  }
}
#else
void ui_theme_set_compass_info_layout(_lv_obj_t* obj, lv_coord_t pad_left,
                                      lv_coord_t pad_row) {
  (void)obj;
  (void)pad_left;
  (void)pad_row;
}

void ui_theme_set_compass_dial_row_layout(_lv_obj_t* obj,
                                          lv_coord_t pad_right) {
  (void)obj;
  (void)pad_right;
}

bool ui_compass_widget_apply_theme(_lv_obj_t* obj) {
  (void)obj;
  return false;
}
#endif

}  // namespace heltec::meshcore::ui
