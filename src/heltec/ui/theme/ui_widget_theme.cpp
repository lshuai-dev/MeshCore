#include "ui/navigation/ui_navigator.hpp"

#include "ui/app/ui_app_ids.hpp"
#include "ui/app/ui_app_frame_metrics.hpp"
#include "ui/app/ui_theme.hpp"
#include "ui/core/ht_meta_data.hpp"
#include "ui/core/abstract_overlay.hpp"
#include "ui/core/screen_id.hpp"
#include "ui/menus/context_menu.hpp"
#include "ui/menus/context_menu_metrics.hpp"
#include "ui/overlays/quick_ping_overlay.hpp"
#include "ui/overlays/alert_overlay.hpp"
#include "ui/overlays/calibration_overlay.hpp"
#include "ui/overlays/choice_picker_overlay.hpp"
#include "ui/overlays/keyboard_overlay.hpp"
#include "ui/overlays/preview_overlay.hpp"
#include "ui/overlays/radio_pram_sync_overlay.hpp"
#include "ui/overlays/send_message_overlay_ids.hpp"
#include "ui/overlays/splash_overlay.hpp"
#include "ui/screens/compass_dial_widget.hpp"
#include "ui/screens/gps_screen.hpp"
#include "ui/screens/home_screen.hpp"
#include "ui/screens/radio_screen.hpp"
#include "ui/screens/recent_screen.hpp"
#include "ui/screens/system_screen.hpp"
#include "ui/theme/ui_theme_metrics.hpp"
#include "ui/theme/ui_widget_theme.hpp"
#include "ui/widgets/button_roller.hpp"
#include "ui/widgets/top_pane.hpp"

#include <lvgl.h>

namespace heltec::meshcore::ui {
namespace {

static const lv_style_selector_t kInteractiveSelectors[] = {
    LV_PART_MAIN,
    LV_PART_MAIN | LV_STATE_FOCUSED,
    LV_PART_MAIN | LV_STATE_FOCUS_KEY,
    LV_PART_MAIN | LV_STATE_PRESSED,
};

#if !defined(HELTEC_V4_R8_TFT) || !defined(HELTEC_HAS_TOUCH) || !HELTEC_HAS_TOUCH
static const lv_style_selector_t kContextMenuRootSelectors[] = {
    LV_PART_MAIN,
    LV_PART_MAIN | LV_STATE_CHECKED,
    LV_PART_MAIN | LV_STATE_FOCUSED,
    LV_PART_MAIN | LV_STATE_FOCUS_KEY,
    LV_PART_MAIN | LV_STATE_EDITED,
    LV_PART_MAIN | LV_STATE_PRESSED,
};
#endif

static const lv_style_selector_t kMainNoChromeSelectors[] = {
    LV_PART_MAIN,
    LV_PART_MAIN | LV_STATE_FOCUSED,
    LV_PART_MAIN | LV_STATE_FOCUS_KEY,
    LV_PART_MAIN | LV_STATE_PRESSED,
    LV_PART_MAIN | LV_STATE_SCROLLED,
};

static const lv_style_selector_t kScrollbarHiddenSelectors[] = {
    LV_PART_SCROLLBAR,
    LV_PART_SCROLLBAR | LV_STATE_SCROLLED,
};

static const lv_style_selector_t kSystemControlNoChromeSelectors[] = {
    LV_PART_MAIN,
    LV_PART_MAIN | LV_STATE_CHECKED,
    LV_PART_MAIN | LV_STATE_FOCUSED,
    LV_PART_MAIN | LV_STATE_FOCUS_KEY,
    LV_PART_MAIN | LV_STATE_EDITED,
    LV_PART_MAIN | LV_STATE_PRESSED,
    LV_PART_INDICATOR,
    LV_PART_INDICATOR | LV_STATE_CHECKED,
    LV_PART_INDICATOR | LV_STATE_FOCUSED,
    LV_PART_INDICATOR | LV_STATE_FOCUS_KEY,
    LV_PART_KNOB,
    LV_PART_KNOB | LV_STATE_CHECKED,
    LV_PART_ITEMS,
    LV_PART_SELECTED,
    LV_PART_SCROLLBAR,
};

#if !defined(HELTEC_V4_R8_TFT) || !defined(HELTEC_HAS_TOUCH) || !HELTEC_HAS_TOUCH
static lv_style_t s_ctx_menu_style;
static lv_style_t s_ctx_header_style;
static lv_style_t s_ctx_icon_row_style;
static lv_style_t s_ctx_nav_row_style;
static lv_style_t s_ctx_title_style;
static lv_style_t s_ctx_icon_btn_main_style;
static lv_style_t s_ctx_icon_btn_checked_style;
static lv_style_t s_ctx_icon_main_style;
static lv_style_t s_ctx_icon_checked_style;
static bool s_ctx_styles_ready = false;
#endif

static lv_style_t s_send_overlay_label_style;
static lv_style_t s_send_list_style;
static lv_style_t s_send_touch_list_style;
static lv_style_t s_send_row_main_style;
static lv_style_t s_send_row_active_style;
static lv_style_t s_send_row_label_main_style;
static lv_style_t s_send_row_label_active_style;
static bool s_send_styles_ready = false;

static lv_style_t s_radio_sync_overlay_label_style;
static lv_style_t s_radio_sync_list_style;
static lv_style_t s_radio_sync_row_main_style;
static lv_style_t s_radio_sync_row_checked_style;
static bool s_radio_sync_styles_ready = false;

static lv_style_t s_keyboard_title_style;
static lv_style_t s_keyboard_textarea_style;
static lv_style_t s_keyboard_main_style;
static lv_style_t s_keyboard_items_base_style;
static lv_style_t s_keyboard_items_selected_style;
static bool s_keyboard_child_styles_ready = false;

static lv_style_t s_overlay_label_all_states_style;
static lv_style_t s_alert_box_style;
static lv_style_t s_calibration_panel_style;
static bool s_overlay_feedback_styles_ready = false;

static lv_style_t s_surface_root_common_style;
static lv_style_t s_plain_container_style;
static lv_style_t s_no_chrome_style;
static lv_style_t s_hidden_scrollbar_style;
static lv_style_t s_top_pane_slot_style;
static lv_style_t s_touch_reset_style;
static bool s_common_widget_styles_ready = false;

static lv_style_t s_top_pane_root_style;
static lv_style_t s_top_pane_battery_style;
static lv_style_t s_top_pane_title_style;
static lv_style_t s_top_pane_battery_outline_style;
static lv_style_t s_top_pane_battery_cap_style;
static lv_style_t s_top_pane_battery_fill_style;
static bool s_top_pane_styles_ready = false;

static lv_style_t s_alert_overlay_root_style;
static lv_style_t s_calibration_overlay_root_style;
static lv_style_t s_splash_overlay_text_style;
#if !defined(HELTEC_V4_R8_TFT) || !defined(HELTEC_HAS_TOUCH) || !HELTEC_HAS_TOUCH
static lv_style_t s_context_menu_root_style;
#endif
static lv_style_t s_app_overlay_layer_style;
static lv_style_t s_app_frame_layout_style;
static lv_style_t s_app_content_style;
static lv_style_t s_app_screen_root_style;
static lv_style_t s_app_tileview_style;
static lv_style_t s_app_tile_style;
static lv_style_t s_active_screen_style;
static bool s_active_screen_style_ready = false;
static bool s_surface_app_styles_ready = false;
static bool s_splash_overlay_text_style_ready = false;
static const lv_font_t* s_splash_overlay_text_font = nullptr;

#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
static lv_style_t s_quick_ping_root_style;
static lv_style_t s_quick_ping_title_bar_style;
static lv_style_t s_quick_ping_content_style;
static lv_style_t s_quick_ping_title_style;
static lv_style_t s_quick_ping_row_style;
static lv_style_t s_quick_ping_label_style;
static lv_style_t s_quick_ping_dropdown_style;
static lv_style_t s_quick_ping_message_dropdown_style;
static lv_style_t s_quick_ping_dropdown_focus_style;
static lv_style_t s_quick_ping_dropdown_disabled_style;
static lv_style_t s_quick_ping_dropdown_indicator_style;
static lv_style_t s_quick_ping_dropdown_indicator_focus_style;
static lv_style_t s_quick_ping_message_input_style;
static lv_style_t s_quick_ping_message_input_focus_style;
static lv_style_t s_quick_ping_message_input_label_focus_style;
static lv_style_t s_quick_ping_message_input_label_style;
static lv_style_t s_quick_ping_keyboard_style;
static lv_style_t s_quick_ping_keyboard_items_style;
static lv_style_t s_quick_ping_keyboard_items_selected_style;
static bool s_quick_ping_styles_ready = false;
#endif

static lv_style_t s_warning_text_style;
static lv_style_t s_accent_text_style;
static lv_style_t s_system_action_row_style;
static lv_style_t s_system_switch_row_style;
static lv_style_t s_system_dropdown_row_style;
static lv_style_t s_system_action_label_style;
static lv_style_t s_system_switch_label_style;
static lv_style_t s_system_dropdown_label_style;
static lv_style_t s_system_dropdown_style;
static lv_style_t s_system_dropdown_focus_style;
static bool s_screen_system_styles_ready = false;

static lv_style_t s_nav_content_host_style;
static lv_style_t s_nav_panel_base_style;
static lv_style_t s_nav_root_base_style;
static lv_style_t s_visibility_visible_style;
static lv_style_t s_visibility_hidden_style;
#if defined(UI_NAVIGATION_GRID) && UI_NAVIGATION_GRID
static lv_style_t s_nav_grid_cell_style;
static lv_style_t s_nav_grid_cell_focus_style;
static lv_style_t s_nav_grid_icon_area_style;
static lv_style_t s_nav_grid_icon_style;
static lv_style_t s_nav_grid_title_bar_style;
static lv_style_t s_nav_grid_title_label_style;
static lv_style_t s_nav_panel_grid_style;
static lv_style_t s_nav_root_grid_style;
static lv_style_t s_nav_footer_bar_style;
static lv_style_t s_nav_footer_label_style;
#else
static lv_style_t s_nav_panel_ring_style;
static lv_style_t s_nav_root_ring_style;
static lv_style_t s_nav_ring_style;
#endif
static bool s_navigation_styles_ready = false;

static lv_style_t s_overlay_root_chrome_style;
static bool s_overlay_root_chrome_style_ready = false;

struct LabelStyleSlot {
  bool ready = false;
  uint32_t color = 0;
  lv_text_align_t align = LV_TEXT_ALIGN_LEFT;
  lv_style_t style;
};

static LabelStyleSlot s_label_styles[6];

static lv_style_t* cached_label_style(lv_color_t color, lv_text_align_t align) {
  const uint32_t color32 = lv_color_to32(color);
  for (LabelStyleSlot& slot : s_label_styles) {
    if (slot.ready && slot.color == color32 && slot.align == align) {
      return &slot.style;
    }
  }
  for (LabelStyleSlot& slot : s_label_styles) {
    if (slot.ready) continue;
    slot.ready = true;
    slot.color = color32;
    slot.align = align;
    lv_style_init(&slot.style);
    lv_style_set_text_color(&slot.style, color);
    lv_style_set_text_align(&slot.style, align);
    lv_style_set_bg_opa(&slot.style, LV_OPA_TRANSP);
    lv_style_set_border_width(&slot.style, 0);
    lv_style_set_border_opa(&slot.style, LV_OPA_TRANSP);
    lv_style_set_border_side(&slot.style, LV_BORDER_SIDE_NONE);
    lv_style_set_outline_width(&slot.style, 0);
    lv_style_set_outline_opa(&slot.style, LV_OPA_TRANSP);
    lv_style_set_shadow_width(&slot.style, 0);
    lv_style_set_shadow_opa(&slot.style, LV_OPA_TRANSP);
    return &slot.style;
  }
  return nullptr;
}

static void set_overlay_text_spacing(lv_style_t* style) {
  if (!style) return;
  lv_style_set_text_line_space(style, 0);
  lv_style_set_text_letter_space(style, 0);
}

static lv_coord_t reference_card_gap() {
#if defined(HELTEC_V4_R8_TFT)
  // The reference theme inherits PAD_SMALL from LVGL's small-display card style.
  return LV_DPX(10);
#else
  return 3;
#endif
}

#if !defined(HELTEC_V4_R8_TFT) || !defined(HELTEC_HAS_TOUCH) || !HELTEC_HAS_TOUCH
static void init_classic_context_styles(lv_obj_t* obj) {
  if (s_ctx_styles_ready) return;
  const UiContextMenuMetrics& metrics = ui_context_menu_metrics(obj);
  const lv_coord_t icon_pad_px = metrics.icon_pad;
  const lv_coord_t border_width_px = metrics.border_width;
  const lv_coord_t title_border_width_px = metrics.title_border_width;

  lv_style_init(&s_ctx_menu_style);
  lv_style_set_border_width(&s_ctx_menu_style, 0);
  lv_style_set_outline_width(&s_ctx_menu_style, 0);
  lv_style_set_bg_opa(&s_ctx_menu_style, LV_OPA_COVER);
  lv_style_set_bg_color(&s_ctx_menu_style, ui_color_bg());
  lv_style_set_text_color(&s_ctx_menu_style, ui_color_fg());

  lv_style_init(&s_ctx_header_style);
  lv_style_set_bg_opa(&s_ctx_header_style, LV_OPA_COVER);
  lv_style_set_bg_color(&s_ctx_header_style, ui_color_fg());
  lv_style_set_text_color(&s_ctx_header_style, ui_color_bg());
  lv_style_set_border_width(&s_ctx_header_style, 0);
  lv_style_set_outline_width(&s_ctx_header_style, 0);
  // lv_style_set_shadow_width(&s_ctx_header_style, 0);

  lv_style_init(&s_ctx_icon_row_style);
  lv_style_set_bg_opa(&s_ctx_icon_row_style, LV_OPA_TRANSP);
  lv_style_set_text_color(&s_ctx_icon_row_style, ui_color_bg());
  lv_style_set_border_width(&s_ctx_icon_row_style, 0);
  lv_style_set_outline_width(&s_ctx_icon_row_style, 0);
  lv_style_set_shadow_width(&s_ctx_icon_row_style, 0);
  lv_style_set_radius(&s_ctx_icon_row_style, 0);

  lv_style_init(&s_ctx_nav_row_style);
  lv_style_set_bg_opa(&s_ctx_nav_row_style, LV_OPA_TRANSP);
  lv_style_set_text_color(&s_ctx_nav_row_style, ui_color_bg());
  lv_style_set_border_width(&s_ctx_nav_row_style, 0);
  lv_style_set_outline_width(&s_ctx_nav_row_style, 0);
  lv_style_set_shadow_width(&s_ctx_nav_row_style, 0);
  lv_style_set_radius(&s_ctx_nav_row_style, 0);

  lv_style_init(&s_ctx_title_style);
  lv_style_set_text_align(&s_ctx_title_style, LV_TEXT_ALIGN_CENTER);
  set_overlay_text_spacing(&s_ctx_title_style);
  lv_style_set_bg_opa(&s_ctx_title_style, LV_OPA_COVER);
  lv_style_set_bg_color(&s_ctx_title_style, ui_color_fg());
  lv_style_set_text_color(&s_ctx_title_style, ui_color_bg());
  lv_style_set_outline_width(&s_ctx_title_style, 0);
  lv_style_set_shadow_width(&s_ctx_title_style, 0);
  lv_style_set_radius(&s_ctx_title_style, 0);
  if (title_border_width_px > 0) {
    lv_style_set_border_width(&s_ctx_title_style, title_border_width_px);
    lv_style_set_border_side(&s_ctx_title_style, LV_BORDER_SIDE_FULL);
    lv_style_set_border_color(&s_ctx_title_style, ui_color_bg());
    lv_style_set_border_opa(&s_ctx_title_style, LV_OPA_COVER);
  }

  lv_style_init(&s_ctx_icon_btn_main_style);
  lv_style_set_pad_all(&s_ctx_icon_btn_main_style, icon_pad_px);
  lv_style_set_bg_opa(&s_ctx_icon_btn_main_style, LV_OPA_TRANSP);
  lv_style_set_border_width(&s_ctx_icon_btn_main_style, 0);
  lv_style_set_outline_width(&s_ctx_icon_btn_main_style, 0);
  lv_style_set_radius(&s_ctx_icon_btn_main_style, 0);
  lv_style_set_shadow_width(&s_ctx_icon_btn_main_style, 0);
  lv_style_set_img_recolor_opa(&s_ctx_icon_btn_main_style, LV_OPA_TRANSP);

  lv_style_init(&s_ctx_icon_btn_checked_style);
  lv_style_set_pad_all(&s_ctx_icon_btn_checked_style, 0);
  lv_style_set_bg_opa(&s_ctx_icon_btn_checked_style, LV_OPA_TRANSP);
  lv_style_set_border_width(&s_ctx_icon_btn_checked_style, border_width_px);
  lv_style_set_border_color(&s_ctx_icon_btn_checked_style, ui_color_bg());
  lv_style_set_border_opa(&s_ctx_icon_btn_checked_style, LV_OPA_COVER);
  lv_style_set_border_side(&s_ctx_icon_btn_checked_style, LV_BORDER_SIDE_FULL);
  lv_style_set_outline_width(&s_ctx_icon_btn_checked_style, 0);
  lv_style_set_radius(&s_ctx_icon_btn_checked_style, 0);
  lv_style_set_img_recolor_opa(&s_ctx_icon_btn_checked_style, LV_OPA_TRANSP);

  lv_style_init(&s_ctx_icon_main_style);
  lv_style_set_img_recolor_opa(&s_ctx_icon_main_style, LV_OPA_TRANSP);

  lv_style_init(&s_ctx_icon_checked_style);
  lv_style_set_img_recolor_opa(&s_ctx_icon_checked_style, LV_OPA_TRANSP);

  s_ctx_styles_ready = true;
}
#endif

static void init_send_message_styles() {
  if (s_send_styles_ready) return;

#if (defined(HELTEC_DISPLAY_ST7789) && HELTEC_DISPLAY_ST7789)
  const lv_coord_t row_radius = 0;
#elif (defined(HELTEC_DISPLAY_ST7735) && HELTEC_DISPLAY_ST7735) || \
    (defined(HELTEC_DISPLAY_SSD1306) && HELTEC_DISPLAY_SSD1306) || \
    LV_COLOR_DEPTH == 1
  const lv_coord_t row_radius = 0;
#else
  const lv_coord_t row_radius = 3;
#endif

  lv_style_init(&s_send_overlay_label_style);
  lv_style_set_text_color(&s_send_overlay_label_style, ui_color_overlay_fg());
  lv_style_set_text_align(&s_send_overlay_label_style, LV_TEXT_ALIGN_CENTER);
  set_overlay_text_spacing(&s_send_overlay_label_style);

  lv_style_init(&s_send_list_style);
  lv_style_set_bg_opa(&s_send_list_style, LV_OPA_COVER);
  lv_style_set_bg_color(&s_send_list_style, ui_color_overlay_bg());
  lv_style_set_border_width(&s_send_list_style, 0);

  lv_style_init(&s_send_touch_list_style);
  lv_style_set_bg_opa(&s_send_touch_list_style, LV_OPA_TRANSP);
  lv_style_set_border_width(&s_send_touch_list_style, 0);

  lv_style_init(&s_send_row_main_style);
  lv_style_set_bg_opa(&s_send_row_main_style, LV_OPA_TRANSP);
  lv_style_set_radius(&s_send_row_main_style, row_radius);
  lv_style_set_shadow_width(&s_send_row_main_style, 0);
  lv_style_set_border_width(&s_send_row_main_style, 0);
  lv_style_set_outline_width(&s_send_row_main_style, 0);
  lv_style_set_text_color(&s_send_row_main_style, ui_color_overlay_fg());
  lv_style_set_text_align(&s_send_row_main_style, LV_TEXT_ALIGN_CENTER);

  lv_style_init(&s_send_row_active_style);
  lv_style_set_bg_opa(&s_send_row_active_style, LV_OPA_COVER);
  lv_style_set_bg_color(&s_send_row_active_style, ui_color_highlight_bg());
  lv_style_set_text_color(&s_send_row_active_style, ui_color_highlight_fg());
  lv_style_set_shadow_width(&s_send_row_active_style, 0);
  lv_style_set_border_width(&s_send_row_active_style, 0);
  lv_style_set_outline_width(&s_send_row_active_style, 0);

  lv_style_init(&s_send_row_label_main_style);
  lv_style_set_text_color(&s_send_row_label_main_style, ui_color_overlay_fg());
  lv_style_set_text_align(&s_send_row_label_main_style, LV_TEXT_ALIGN_CENTER);
  set_overlay_text_spacing(&s_send_row_label_main_style);

  lv_style_init(&s_send_row_label_active_style);
  lv_style_set_text_color(&s_send_row_label_active_style, ui_color_highlight_fg());

  s_send_styles_ready = true;
}

static void init_radio_sync_styles() {
  if (s_radio_sync_styles_ready) return;

  lv_style_init(&s_radio_sync_overlay_label_style);
  lv_style_set_text_color(&s_radio_sync_overlay_label_style, ui_color_overlay_fg());
  lv_style_set_text_align(&s_radio_sync_overlay_label_style, LV_TEXT_ALIGN_CENTER);
  set_overlay_text_spacing(&s_radio_sync_overlay_label_style);

  lv_style_init(&s_radio_sync_list_style);
  lv_style_set_bg_opa(&s_radio_sync_list_style, LV_OPA_TRANSP);
  lv_style_set_border_width(&s_radio_sync_list_style, 0);

  lv_style_init(&s_radio_sync_row_main_style);
  lv_style_set_text_color(&s_radio_sync_row_main_style, ui_color_overlay_fg());
  lv_style_set_text_align(&s_radio_sync_row_main_style, LV_TEXT_ALIGN_CENTER);
  set_overlay_text_spacing(&s_radio_sync_row_main_style);
  lv_style_set_bg_opa(&s_radio_sync_row_main_style, LV_OPA_TRANSP);

  lv_style_init(&s_radio_sync_row_checked_style);
  lv_style_set_text_color(&s_radio_sync_row_checked_style, ui_color_highlight_fg());
  lv_style_set_bg_opa(&s_radio_sync_row_checked_style, LV_OPA_COVER);
  lv_style_set_bg_color(&s_radio_sync_row_checked_style, ui_color_highlight_bg());

  s_radio_sync_styles_ready = true;
}

static void init_keyboard_child_styles() {
  if (s_keyboard_child_styles_ready) return;

  lv_style_init(&s_keyboard_title_style);
  lv_style_set_text_color(&s_keyboard_title_style, ui_color_overlay_fg());
  lv_style_set_text_align(&s_keyboard_title_style, LV_TEXT_ALIGN_CENTER);
  set_overlay_text_spacing(&s_keyboard_title_style);

  lv_style_init(&s_keyboard_textarea_style);
  lv_style_set_text_color(&s_keyboard_textarea_style, ui_color_overlay_fg());
  lv_style_set_bg_opa(&s_keyboard_textarea_style, LV_OPA_TRANSP);
  lv_style_set_border_color(&s_keyboard_textarea_style, ui_color_overlay_fg());
  lv_style_set_border_width(&s_keyboard_textarea_style, 1);
  lv_style_set_text_align(&s_keyboard_textarea_style, LV_TEXT_ALIGN_LEFT);
  set_overlay_text_spacing(&s_keyboard_textarea_style);
  lv_style_set_pad_all(&s_keyboard_textarea_style, 1);

  lv_style_init(&s_keyboard_main_style);
  lv_style_set_pad_all(&s_keyboard_main_style, 0);
  lv_style_set_pad_row(&s_keyboard_main_style, 0);
  lv_style_set_pad_column(&s_keyboard_main_style, 1);
  lv_style_set_border_width(&s_keyboard_main_style, 0);
  lv_style_set_outline_width(&s_keyboard_main_style, 0);
  lv_style_set_shadow_width(&s_keyboard_main_style, 0);

  lv_style_init(&s_keyboard_items_base_style);
  lv_style_set_pad_all(&s_keyboard_items_base_style, 1);
  lv_style_set_border_width(&s_keyboard_items_base_style, 0);
  lv_style_set_outline_width(&s_keyboard_items_base_style, 0);
  lv_style_set_shadow_width(&s_keyboard_items_base_style, 0);
  lv_style_set_bg_opa(&s_keyboard_items_base_style, LV_OPA_TRANSP);
  lv_style_set_text_color(&s_keyboard_items_base_style, ui_color_overlay_fg());

  lv_style_init(&s_keyboard_items_selected_style);
  lv_style_set_bg_color(&s_keyboard_items_selected_style, ui_color_highlight_bg());
  lv_style_set_bg_opa(&s_keyboard_items_selected_style, LV_OPA_COVER);
  lv_style_set_text_color(&s_keyboard_items_selected_style, ui_color_highlight_fg());
  lv_style_set_border_width(&s_keyboard_items_selected_style, 0);
  lv_style_set_outline_width(&s_keyboard_items_selected_style, 0);
  lv_style_set_shadow_width(&s_keyboard_items_selected_style, 0);

  s_keyboard_child_styles_ready = true;
}

static void init_overlay_feedback_styles() {
  if (s_overlay_feedback_styles_ready) return;

  lv_style_init(&s_overlay_label_all_states_style);
  lv_style_set_text_color(&s_overlay_label_all_states_style, ui_color_overlay_fg());
  lv_style_set_text_opa(&s_overlay_label_all_states_style, LV_OPA_COVER);
  lv_style_set_bg_opa(&s_overlay_label_all_states_style, LV_OPA_TRANSP);
  lv_style_set_text_align(&s_overlay_label_all_states_style, LV_TEXT_ALIGN_CENTER);
  set_overlay_text_spacing(&s_overlay_label_all_states_style);

  lv_style_init(&s_alert_box_style);
  lv_style_set_bg_color(&s_alert_box_style, ui_color_overlay_bg());
  lv_style_set_bg_opa(&s_alert_box_style, LV_OPA_COVER);
  lv_style_set_border_color(&s_alert_box_style, ui_color_overlay_fg());
  lv_style_set_border_width(&s_alert_box_style, 1);
  lv_style_set_border_opa(&s_alert_box_style, LV_OPA_COVER);
  lv_style_set_outline_width(&s_alert_box_style, 0);
  lv_style_set_radius(&s_alert_box_style, 3);
  lv_style_set_pad_all(&s_alert_box_style, 8);

  lv_style_init(&s_calibration_panel_style);
  lv_style_set_radius(&s_calibration_panel_style, 0);
  lv_style_set_bg_color(&s_calibration_panel_style, ui_color_overlay_bg());
  lv_style_set_bg_opa(&s_calibration_panel_style, LV_OPA_COVER);
  lv_style_set_border_width(&s_calibration_panel_style, 0);
  lv_style_set_outline_width(&s_calibration_panel_style, 0);

  s_overlay_feedback_styles_ready = true;
}

static void init_common_widget_styles() {
  if (s_common_widget_styles_ready) return;

  lv_style_init(&s_surface_root_common_style);
  lv_style_set_bg_opa(&s_surface_root_common_style, LV_OPA_TRANSP);

  lv_style_init(&s_plain_container_style);
  lv_style_set_bg_opa(&s_plain_container_style, LV_OPA_TRANSP);
  lv_style_set_border_width(&s_plain_container_style, 0);

  lv_style_init(&s_no_chrome_style);
  lv_style_set_border_width(&s_no_chrome_style, 0);
  lv_style_set_border_opa(&s_no_chrome_style, LV_OPA_TRANSP);
  lv_style_set_border_side(&s_no_chrome_style, LV_BORDER_SIDE_NONE);
  lv_style_set_outline_width(&s_no_chrome_style, 0);
  lv_style_set_outline_opa(&s_no_chrome_style, LV_OPA_TRANSP);
  lv_style_set_shadow_width(&s_no_chrome_style, 0);
  lv_style_set_shadow_opa(&s_no_chrome_style, LV_OPA_TRANSP);

  lv_style_init(&s_hidden_scrollbar_style);
  lv_style_set_width(&s_hidden_scrollbar_style, 0);
  lv_style_set_pad_all(&s_hidden_scrollbar_style, 0);
  lv_style_set_bg_opa(&s_hidden_scrollbar_style, LV_OPA_TRANSP);
  lv_style_set_border_width(&s_hidden_scrollbar_style, 0);
  lv_style_set_border_opa(&s_hidden_scrollbar_style, LV_OPA_TRANSP);
  lv_style_set_border_side(&s_hidden_scrollbar_style, LV_BORDER_SIDE_NONE);
  lv_style_set_outline_width(&s_hidden_scrollbar_style, 0);
  lv_style_set_outline_opa(&s_hidden_scrollbar_style, LV_OPA_TRANSP);
  lv_style_set_shadow_width(&s_hidden_scrollbar_style, 0);
  lv_style_set_shadow_opa(&s_hidden_scrollbar_style, LV_OPA_TRANSP);
  lv_style_set_opa(&s_hidden_scrollbar_style, LV_OPA_TRANSP);

  lv_style_init(&s_top_pane_slot_style);
  lv_style_set_bg_opa(&s_top_pane_slot_style, LV_OPA_TRANSP);
  lv_style_set_border_width(&s_top_pane_slot_style, 0);
  lv_style_set_border_opa(&s_top_pane_slot_style, LV_OPA_TRANSP);
  lv_style_set_border_side(&s_top_pane_slot_style, LV_BORDER_SIDE_NONE);
  lv_style_set_outline_width(&s_top_pane_slot_style, 0);
  lv_style_set_outline_opa(&s_top_pane_slot_style, LV_OPA_TRANSP);
  lv_style_set_shadow_width(&s_top_pane_slot_style, 0);
  lv_style_set_shadow_opa(&s_top_pane_slot_style, LV_OPA_TRANSP);
  lv_style_set_radius(&s_top_pane_slot_style, 0);

  lv_style_init(&s_touch_reset_style);
  lv_style_set_shadow_width(&s_touch_reset_style, 0);

  s_common_widget_styles_ready = true;
}

static void init_top_pane_styles(lv_obj_t* obj) {
  if (s_top_pane_styles_ready) return;

  const UiTopPaneMetrics& top_metrics = ui_top_pane_metrics(obj);
  const lv_coord_t top_h = top_metrics.height;
  const lv_coord_t top_radius = top_metrics.radius;
  const lv_coord_t bat_w = top_metrics.battery_w;
  const lv_coord_t bat_h_cfg = top_metrics.battery_h;
  const lv_color_t bar_fg = ui_color_top_pane_fg();
#if defined(HELTEC_V4_R8_TFT)
  const lv_color_t battery_chrome = lv_color_white();
#else
  const lv_color_t battery_chrome = bar_fg;
#endif
  const lv_coord_t bat_h =
      bat_h_cfg > 0 ? bat_h_cfg : static_cast<lv_coord_t>((top_h * 7 + 5) / 10);
  const int16_t cap_i = (bat_h + 1) / 3;
  const int16_t cap_px = (cap_i < 2 ? 2 : (cap_i > 6 ? 6 : cap_i));
  const lv_coord_t body_w = bat_w - cap_px + 1;
#if defined(HELTEC_V4_R8_TFT)
  const lv_coord_t cap_h = LV_MAX(4, static_cast<lv_coord_t>((bat_h * 2 + 2) / 3));
#else
  const lv_coord_t cap_h = LV_MAX(2, bat_h / 2);
#endif
  lv_style_init(&s_top_pane_root_style);
  lv_style_set_bg_opa(&s_top_pane_root_style, ui_top_pane_bg_opa());
  lv_style_set_bg_color(&s_top_pane_root_style, ui_color_top_pane_bg());
  lv_style_set_text_color(&s_top_pane_root_style, bar_fg);
  lv_style_set_border_width(&s_top_pane_root_style, 0);
  lv_style_set_border_opa(&s_top_pane_root_style, LV_OPA_TRANSP);
  lv_style_set_border_side(&s_top_pane_root_style, LV_BORDER_SIDE_NONE);
  lv_style_set_outline_width(&s_top_pane_root_style, 0);
  lv_style_set_outline_opa(&s_top_pane_root_style, LV_OPA_TRANSP);
  lv_style_set_shadow_width(&s_top_pane_root_style, 0);
  lv_style_set_shadow_opa(&s_top_pane_root_style, LV_OPA_TRANSP);
  lv_style_set_radius(&s_top_pane_root_style, top_radius);
  lv_style_set_clip_corner(&s_top_pane_root_style, top_radius > 0);

  lv_style_init(&s_top_pane_battery_style);
  lv_style_set_bg_opa(&s_top_pane_battery_style, LV_OPA_TRANSP);
  lv_style_set_pad_all(&s_top_pane_battery_style, 0);
  lv_style_set_width(&s_top_pane_battery_style, bat_w);
  lv_style_set_height(&s_top_pane_battery_style, bat_h);
  lv_style_set_border_width(&s_top_pane_battery_style, 0);
  lv_style_set_border_opa(&s_top_pane_battery_style, LV_OPA_TRANSP);
  lv_style_set_border_color(&s_top_pane_battery_style, battery_chrome);
  lv_style_set_pad_all(&s_top_pane_battery_style, 0);
  lv_style_set_outline_width(&s_top_pane_battery_style, 0);
  lv_style_set_shadow_width(&s_top_pane_battery_style, 0);
  lv_style_set_radius(&s_top_pane_battery_style, 0);
  lv_style_set_clip_corner(&s_top_pane_battery_style, false);

  lv_style_init(&s_top_pane_title_style);
  lv_style_set_bg_opa(&s_top_pane_title_style, LV_OPA_TRANSP);
  lv_style_set_text_color(&s_top_pane_title_style, bar_fg);
  lv_style_set_text_align(&s_top_pane_title_style, LV_TEXT_ALIGN_CENTER);
  lv_style_set_border_width(&s_top_pane_title_style, 0);
  lv_style_set_border_opa(&s_top_pane_title_style, LV_OPA_TRANSP);
  lv_style_set_border_side(&s_top_pane_title_style, LV_BORDER_SIDE_NONE);
  lv_style_set_outline_width(&s_top_pane_title_style, 0);
  lv_style_set_outline_opa(&s_top_pane_title_style, LV_OPA_TRANSP);
  lv_style_set_shadow_width(&s_top_pane_title_style, 0);
  lv_style_set_shadow_opa(&s_top_pane_title_style, LV_OPA_TRANSP);

  lv_style_init(&s_top_pane_battery_outline_style);
#if defined(HELTEC_V4_R8_TFT)
  lv_style_set_pad_all(&s_top_pane_battery_outline_style, 0);
#else
  lv_style_set_pad_all(&s_top_pane_battery_outline_style, 1);
#endif
  lv_style_set_width(&s_top_pane_battery_outline_style, body_w);
  lv_style_set_height(&s_top_pane_battery_outline_style, bat_h);
  lv_style_set_bg_opa(&s_top_pane_battery_outline_style, LV_OPA_TRANSP);
  lv_style_set_border_width(&s_top_pane_battery_outline_style, 1);
  lv_style_set_border_color(&s_top_pane_battery_outline_style, battery_chrome);
  lv_style_set_border_opa(&s_top_pane_battery_outline_style, LV_OPA_COVER);
  lv_style_set_radius(&s_top_pane_battery_outline_style, 0);
  lv_style_set_outline_width(&s_top_pane_battery_outline_style, 0);
  lv_style_set_outline_opa(&s_top_pane_battery_outline_style, LV_OPA_TRANSP);
  lv_style_set_shadow_width(&s_top_pane_battery_outline_style, 0);
  lv_style_set_shadow_opa(&s_top_pane_battery_outline_style, LV_OPA_TRANSP);

  lv_style_init(&s_top_pane_battery_cap_style);
  lv_style_set_pad_all(&s_top_pane_battery_cap_style, 0);
  lv_style_set_width(&s_top_pane_battery_cap_style, cap_px);
  lv_style_set_height(&s_top_pane_battery_cap_style, cap_h);
  lv_style_set_bg_opa(&s_top_pane_battery_cap_style, LV_OPA_COVER);
  lv_style_set_bg_color(&s_top_pane_battery_cap_style, battery_chrome);
  lv_style_set_border_width(&s_top_pane_battery_cap_style, 0);
  lv_style_set_border_opa(&s_top_pane_battery_cap_style, LV_OPA_TRANSP);
  lv_style_set_outline_width(&s_top_pane_battery_cap_style, 0);
  lv_style_set_outline_opa(&s_top_pane_battery_cap_style, LV_OPA_TRANSP);
  lv_style_set_shadow_width(&s_top_pane_battery_cap_style, 0);
  lv_style_set_shadow_opa(&s_top_pane_battery_cap_style, LV_OPA_TRANSP);
  lv_style_set_radius(&s_top_pane_battery_cap_style, 0);

  lv_style_init(&s_top_pane_battery_fill_style);
  lv_style_set_pad_all(&s_top_pane_battery_fill_style, 0);
  lv_style_set_border_width(&s_top_pane_battery_fill_style, 0);
  lv_style_set_border_opa(&s_top_pane_battery_fill_style, LV_OPA_TRANSP);
  lv_style_set_outline_width(&s_top_pane_battery_fill_style, 0);
  lv_style_set_outline_opa(&s_top_pane_battery_fill_style, LV_OPA_TRANSP);
  lv_style_set_shadow_width(&s_top_pane_battery_fill_style, 0);
  lv_style_set_shadow_opa(&s_top_pane_battery_fill_style, LV_OPA_TRANSP);
  lv_style_set_radius(&s_top_pane_battery_fill_style, 0);
  lv_style_set_width(&s_top_pane_battery_fill_style, 0);
  lv_style_set_height(&s_top_pane_battery_fill_style, bat_h - 2);
  lv_style_set_bg_opa(&s_top_pane_battery_fill_style, LV_OPA_COVER);
  lv_style_set_bg_color(&s_top_pane_battery_fill_style, bar_fg);

  s_top_pane_styles_ready = true;
}

static void init_active_screen_style() {
  if (s_active_screen_style_ready) return;

  lv_style_init(&s_active_screen_style);
  lv_style_set_border_width(&s_active_screen_style, 0);
  lv_style_set_bg_color(&s_active_screen_style, ui_color_frame_bg());
  lv_style_set_bg_opa(&s_active_screen_style, ui_frame_bg_opa());

  s_active_screen_style_ready = true;
}

static void init_surface_app_styles(lv_obj_t* obj = nullptr) {
  if (s_surface_app_styles_ready) return;

  const UiAppFrameMetrics& frame_metrics = ui_app_frame_metrics(obj);
  const lv_coord_t screen_pad = frame_metrics.screen_pad;
  const lv_coord_t content_radius = frame_metrics.content_radius;
#if !defined(HELTEC_V4_R8_TFT) || !defined(HELTEC_HAS_TOUCH) || !HELTEC_HAS_TOUCH
  const UiContextMenuMetrics& context_metrics = ui_context_menu_metrics(obj);
  const lv_coord_t context_border_w_cfg = context_metrics.border_width;
  const lv_coord_t context_border_radius = context_metrics.border_radius;
#endif

  lv_style_init(&s_alert_overlay_root_style);
  lv_style_set_bg_opa(&s_alert_overlay_root_style, LV_OPA_TRANSP);
  lv_style_set_border_width(&s_alert_overlay_root_style, 0);
  lv_style_set_outline_width(&s_alert_overlay_root_style, 0);

  lv_style_init(&s_calibration_overlay_root_style);
  lv_style_set_bg_color(&s_calibration_overlay_root_style, ui_color_overlay_bg());
  lv_style_set_bg_opa(&s_calibration_overlay_root_style, LV_OPA_COVER);
  lv_style_set_border_width(&s_calibration_overlay_root_style, 0);

#if !defined(HELTEC_V4_R8_TFT) || !defined(HELTEC_HAS_TOUCH) || !HELTEC_HAS_TOUCH
  const lv_coord_t context_border_w =
      context_border_w_cfg > 0 ? static_cast<lv_coord_t>(context_border_w_cfg) : 1;

  lv_style_init(&s_context_menu_root_style);
  lv_style_set_bg_opa(&s_context_menu_root_style, LV_OPA_COVER);
  lv_style_set_bg_color(&s_context_menu_root_style, ui_color_bg());
  lv_style_set_text_color(&s_context_menu_root_style, ui_color_fg());
  lv_style_set_border_width(&s_context_menu_root_style, context_border_w);
  lv_style_set_border_color(&s_context_menu_root_style, ui_color_fg());
  lv_style_set_border_opa(&s_context_menu_root_style, LV_OPA_COVER);
  // lv_style_set_border_side(&s_context_menu_root_style, LV_BORDER_SIDE_FULL);
  lv_style_set_outline_width(&s_context_menu_root_style, context_border_w);
  lv_style_set_outline_color(&s_context_menu_root_style, ui_color_bg());
  lv_style_set_outline_opa(&s_context_menu_root_style, LV_OPA_COVER);
  lv_style_set_outline_pad(&s_context_menu_root_style, 1);
  lv_style_set_radius(&s_context_menu_root_style, context_border_radius);
#endif

  lv_style_init(&s_app_overlay_layer_style);
  lv_style_set_bg_opa(&s_app_overlay_layer_style, LV_OPA_TRANSP);
  lv_style_set_border_width(&s_app_overlay_layer_style, 0);
  lv_style_set_radius(&s_app_overlay_layer_style, 0);

  lv_style_init(&s_app_frame_layout_style);
  lv_style_set_border_width(&s_app_frame_layout_style, 0);
  lv_style_set_bg_color(&s_app_frame_layout_style, ui_color_frame_bg());
  lv_style_set_bg_opa(&s_app_frame_layout_style, ui_frame_bg_opa());

  lv_style_init(&s_app_content_style);
  lv_style_set_bg_opa(&s_app_content_style, LV_OPA_TRANSP);
  lv_style_set_border_width(&s_app_content_style, 0);

  lv_style_init(&s_app_screen_root_style);
  lv_style_set_bg_opa(&s_app_screen_root_style, LV_OPA_TRANSP);
  lv_style_set_border_width(&s_app_screen_root_style, 0);
  lv_style_set_radius(&s_app_screen_root_style, 0);
  lv_style_set_clip_corner(&s_app_screen_root_style, false);
#if !defined(HELTEC_V4_R8_TFT)
  if (screen_pad > 0 || content_radius > 0) {
    lv_style_set_bg_color(&s_app_screen_root_style, ui_color_panel_bg());
    lv_style_set_bg_opa(&s_app_screen_root_style, LV_OPA_COVER);
    lv_style_set_radius(&s_app_screen_root_style, content_radius > 0 ? content_radius : 0);
    lv_style_set_clip_corner(&s_app_screen_root_style, content_radius > 0);
  }
#endif
  lv_style_init(&s_app_tileview_style);
#if defined(HELTEC_V4_R8_TFT)
  lv_style_set_bg_color(&s_app_tileview_style, lv_color_white());
  lv_style_set_bg_opa(&s_app_tileview_style, LV_OPA_COVER);
  lv_style_set_border_width(&s_app_tileview_style, 0);
  lv_style_set_radius(&s_app_tileview_style, content_radius);
  lv_style_set_clip_corner(&s_app_tileview_style, content_radius > 0);
#endif

  lv_style_init(&s_app_tile_style);
  lv_style_set_bg_opa(&s_app_tile_style, LV_OPA_TRANSP);
  lv_style_set_border_width(&s_app_tile_style, 0);

  init_active_screen_style();

  s_surface_app_styles_ready = true;
}

#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
static void init_quick_ping_styles() {
  if (s_quick_ping_styles_ready) return;

  const lv_color_t pane_bg = lv_color_hex(0xEEF6FF);
  const lv_color_t control_bg = lv_color_hex(0xE8E8E8);

  lv_style_init(&s_quick_ping_root_style);
  lv_style_set_bg_opa(&s_quick_ping_root_style, LV_OPA_COVER);
  lv_style_set_bg_color(&s_quick_ping_root_style, pane_bg);
  lv_style_set_text_color(&s_quick_ping_root_style, ui_color_fg());
  lv_style_set_border_width(&s_quick_ping_root_style, 1);
  lv_style_set_border_color(&s_quick_ping_root_style, ui_color_fg());
  lv_style_set_border_opa(&s_quick_ping_root_style, LV_OPA_COVER);
  lv_style_set_radius(&s_quick_ping_root_style, 10);
  lv_style_set_clip_corner(&s_quick_ping_root_style, true);

  lv_style_init(&s_quick_ping_title_bar_style);
  lv_style_set_pad_all(&s_quick_ping_title_bar_style, 0);
  lv_style_set_bg_color(&s_quick_ping_title_bar_style, pane_bg);
  lv_style_set_bg_opa(&s_quick_ping_title_bar_style, LV_OPA_COVER);
  lv_style_set_border_width(&s_quick_ping_title_bar_style, 0);
  lv_style_set_radius(&s_quick_ping_title_bar_style, 0);
  lv_style_set_outline_width(&s_quick_ping_title_bar_style, 0);
  lv_style_set_shadow_width(&s_quick_ping_title_bar_style, 0);

  lv_style_init(&s_quick_ping_content_style);
  lv_style_set_bg_color(&s_quick_ping_content_style, pane_bg);
  lv_style_set_bg_opa(&s_quick_ping_content_style, LV_OPA_COVER);
  lv_style_set_border_width(&s_quick_ping_content_style, 0);
  lv_style_set_radius(&s_quick_ping_content_style, 0);
  lv_style_set_outline_width(&s_quick_ping_content_style, 0);
  lv_style_set_shadow_width(&s_quick_ping_content_style, 0);

  lv_style_init(&s_quick_ping_title_style);
  lv_style_set_bg_opa(&s_quick_ping_title_style, LV_OPA_TRANSP);
  lv_style_set_text_color(&s_quick_ping_title_style, ui_color_fg());
  lv_style_set_text_align(&s_quick_ping_title_style, LV_TEXT_ALIGN_CENTER);
#if defined(LV_FONT_MONTSERRAT_14) && LV_FONT_MONTSERRAT_14
  lv_style_set_text_font(&s_quick_ping_title_style, &lv_font_montserrat_14);
#endif
  set_overlay_text_spacing(&s_quick_ping_title_style);

  lv_style_init(&s_quick_ping_row_style);
  lv_style_set_bg_color(&s_quick_ping_row_style, lv_color_white());
  lv_style_set_bg_opa(&s_quick_ping_row_style, LV_OPA_COVER);
  lv_style_set_border_width(&s_quick_ping_row_style, 1);
  lv_style_set_border_color(&s_quick_ping_row_style, ui_color_fg());
  lv_style_set_border_opa(&s_quick_ping_row_style, LV_OPA_COVER);
  lv_style_set_radius(&s_quick_ping_row_style, 6);
  lv_style_set_clip_corner(&s_quick_ping_row_style, true);

  lv_style_init(&s_quick_ping_label_style);
  lv_style_set_text_color(&s_quick_ping_label_style, ui_color_fg());
  lv_style_set_text_align(&s_quick_ping_label_style, LV_TEXT_ALIGN_LEFT);
  set_overlay_text_spacing(&s_quick_ping_label_style);

  lv_style_init(&s_quick_ping_dropdown_style);
  lv_style_set_radius(&s_quick_ping_dropdown_style, 6);
  lv_style_set_pad_hor(&s_quick_ping_dropdown_style, 6);
  lv_style_set_pad_ver(&s_quick_ping_dropdown_style, 0);
  lv_style_set_bg_color(&s_quick_ping_dropdown_style, control_bg);
  lv_style_set_bg_opa(&s_quick_ping_dropdown_style, LV_OPA_COVER);
  lv_style_set_border_width(&s_quick_ping_dropdown_style, 0);
  lv_style_set_border_opa(&s_quick_ping_dropdown_style, LV_OPA_TRANSP);
  lv_style_set_outline_width(&s_quick_ping_dropdown_style, 0);
  lv_style_set_shadow_width(&s_quick_ping_dropdown_style, 0);
  lv_style_set_text_color(&s_quick_ping_dropdown_style, ui_color_fg());

  lv_style_init(&s_quick_ping_message_dropdown_style);
  lv_style_set_text_align(&s_quick_ping_message_dropdown_style, LV_TEXT_ALIGN_CENTER);
  lv_style_set_bg_color(&s_quick_ping_message_dropdown_style, lv_color_white());
  lv_style_set_bg_opa(&s_quick_ping_message_dropdown_style, LV_OPA_COVER);

  lv_style_init(&s_quick_ping_dropdown_focus_style);
  lv_style_set_bg_color(&s_quick_ping_dropdown_focus_style, control_bg);
  lv_style_set_bg_opa(&s_quick_ping_dropdown_focus_style, LV_OPA_COVER);
  lv_style_set_border_width(&s_quick_ping_dropdown_focus_style, 0);
  lv_style_set_border_opa(&s_quick_ping_dropdown_focus_style, LV_OPA_TRANSP);
  lv_style_set_outline_width(&s_quick_ping_dropdown_focus_style, 0);
  lv_style_set_shadow_width(&s_quick_ping_dropdown_focus_style, 0);
  lv_style_set_text_color(&s_quick_ping_dropdown_focus_style, ui_color_highlight_fg());

  lv_style_init(&s_quick_ping_dropdown_disabled_style);
  lv_style_set_text_color(&s_quick_ping_dropdown_disabled_style, ui_color_fg());
  lv_style_set_opa(&s_quick_ping_dropdown_disabled_style, ui_effective_opa(LV_OPA_50));

  lv_style_init(&s_quick_ping_dropdown_indicator_style);
  lv_style_set_text_color(&s_quick_ping_dropdown_indicator_style, ui_color_fg());

  lv_style_init(&s_quick_ping_dropdown_indicator_focus_style);
  lv_style_set_text_color(&s_quick_ping_dropdown_indicator_focus_style, ui_color_highlight_fg());

  lv_style_init(&s_quick_ping_message_input_style);
  lv_style_set_radius(&s_quick_ping_message_input_style, 6);
  lv_style_set_pad_hor(&s_quick_ping_message_input_style, 6);
  lv_style_set_pad_ver(&s_quick_ping_message_input_style, 0);
  lv_style_set_bg_color(&s_quick_ping_message_input_style, lv_color_white());
  lv_style_set_bg_opa(&s_quick_ping_message_input_style, LV_OPA_TRANSP);
  lv_style_set_border_width(&s_quick_ping_message_input_style, 0);
  lv_style_set_border_opa(&s_quick_ping_message_input_style, LV_OPA_TRANSP);
  lv_style_set_outline_width(&s_quick_ping_message_input_style, 0);
  lv_style_set_shadow_width(&s_quick_ping_message_input_style, 0);
  lv_style_set_text_color(&s_quick_ping_message_input_style, ui_color_fg());

  lv_style_init(&s_quick_ping_message_input_focus_style);
  lv_style_set_bg_color(&s_quick_ping_message_input_focus_style, ui_color_highlight_bg());
  lv_style_set_bg_opa(&s_quick_ping_message_input_focus_style, LV_OPA_COVER);
  lv_style_set_border_width(&s_quick_ping_message_input_focus_style, 0);
  lv_style_set_border_opa(&s_quick_ping_message_input_focus_style, LV_OPA_TRANSP);
  lv_style_set_outline_width(&s_quick_ping_message_input_focus_style, 0);
  lv_style_set_shadow_width(&s_quick_ping_message_input_focus_style, 0);
  lv_style_set_text_color(&s_quick_ping_message_input_focus_style, ui_color_highlight_fg());

  lv_style_init(&s_quick_ping_message_input_label_focus_style);
  lv_style_set_text_color(&s_quick_ping_message_input_label_focus_style,
                          ui_color_highlight_fg());
  set_overlay_text_spacing(&s_quick_ping_message_input_label_focus_style);

  lv_style_init(&s_quick_ping_message_input_label_style);
  lv_style_set_text_color(&s_quick_ping_message_input_label_style, ui_color_fg());
  lv_style_set_text_align(&s_quick_ping_message_input_label_style, LV_TEXT_ALIGN_LEFT);
  set_overlay_text_spacing(&s_quick_ping_message_input_label_style);

  lv_style_init(&s_quick_ping_keyboard_style);
  lv_style_set_pad_all(&s_quick_ping_keyboard_style, 0);
  lv_style_set_pad_row(&s_quick_ping_keyboard_style, 0);
  lv_style_set_pad_column(&s_quick_ping_keyboard_style, 0);
  lv_style_set_bg_opa(&s_quick_ping_keyboard_style, LV_OPA_TRANSP);
  lv_style_set_border_width(&s_quick_ping_keyboard_style, 0);
  lv_style_set_border_opa(&s_quick_ping_keyboard_style, LV_OPA_TRANSP);
  lv_style_set_outline_width(&s_quick_ping_keyboard_style, 0);
  lv_style_set_shadow_width(&s_quick_ping_keyboard_style, 0);

  lv_style_init(&s_quick_ping_keyboard_items_style);
  lv_style_set_pad_all(&s_quick_ping_keyboard_items_style, 0);
  lv_style_set_bg_opa(&s_quick_ping_keyboard_items_style, LV_OPA_TRANSP);
  lv_style_set_border_width(&s_quick_ping_keyboard_items_style, 0);
  lv_style_set_outline_width(&s_quick_ping_keyboard_items_style, 0);
  lv_style_set_shadow_width(&s_quick_ping_keyboard_items_style, 0);
  lv_style_set_text_color(&s_quick_ping_keyboard_items_style, ui_color_fg());

  lv_style_init(&s_quick_ping_keyboard_items_selected_style);
  lv_style_set_pad_all(&s_quick_ping_keyboard_items_selected_style, 0);
  lv_style_set_bg_color(&s_quick_ping_keyboard_items_selected_style, ui_color_highlight_bg());
  lv_style_set_bg_opa(&s_quick_ping_keyboard_items_selected_style, LV_OPA_COVER);
  lv_style_set_border_width(&s_quick_ping_keyboard_items_selected_style, 0);
  lv_style_set_outline_width(&s_quick_ping_keyboard_items_selected_style, 0);
  lv_style_set_shadow_width(&s_quick_ping_keyboard_items_selected_style, 0);
  lv_style_set_text_color(&s_quick_ping_keyboard_items_selected_style,
                          ui_color_highlight_fg());

  s_quick_ping_styles_ready = true;
}
#endif

static void init_screen_system_styles() {
  if (s_screen_system_styles_ready) return;

  lv_style_init(&s_warning_text_style);
  lv_style_set_text_color(&s_warning_text_style, ui_color_warning());
  set_overlay_text_spacing(&s_warning_text_style);

  lv_style_init(&s_accent_text_style);
  lv_style_set_text_color(&s_accent_text_style, ui_color_accent());
  set_overlay_text_spacing(&s_accent_text_style);

  auto init_system_row = [](lv_style_t* st) {
    lv_style_init(st);
    lv_style_set_bg_opa(st, LV_OPA_TRANSP);
    lv_style_set_border_width(st, 0);
    lv_style_set_border_opa(st, LV_OPA_TRANSP);
    lv_style_set_border_side(st, LV_BORDER_SIDE_NONE);
    lv_style_set_outline_width(st, 0);
    lv_style_set_outline_opa(st, LV_OPA_TRANSP);
    lv_style_set_shadow_width(st, 0);
    lv_style_set_shadow_opa(st, LV_OPA_TRANSP);
  };

  init_system_row(&s_system_action_row_style);
  init_system_row(&s_system_switch_row_style);
  init_system_row(&s_system_dropdown_row_style);

  lv_style_init(&s_system_action_label_style);
  lv_style_set_bg_opa(&s_system_action_label_style, LV_OPA_TRANSP);
  lv_style_set_text_color(&s_system_action_label_style, ui_color_fg());
  lv_style_set_text_align(&s_system_action_label_style, LV_TEXT_ALIGN_LEFT);
  lv_style_set_border_width(&s_system_action_label_style, 0);
  lv_style_set_border_opa(&s_system_action_label_style, LV_OPA_TRANSP);
  lv_style_set_border_side(&s_system_action_label_style, LV_BORDER_SIDE_NONE);
  lv_style_set_outline_width(&s_system_action_label_style, 0);
  lv_style_set_outline_opa(&s_system_action_label_style, LV_OPA_TRANSP);
  lv_style_set_shadow_width(&s_system_action_label_style, 0);
  lv_style_set_shadow_opa(&s_system_action_label_style, LV_OPA_TRANSP);

  lv_style_init(&s_system_switch_label_style);
  lv_style_set_bg_opa(&s_system_switch_label_style, LV_OPA_TRANSP);
  lv_style_set_text_color(&s_system_switch_label_style, ui_color_fg());
  lv_style_set_text_align(&s_system_switch_label_style, LV_TEXT_ALIGN_LEFT);
  lv_style_set_border_width(&s_system_switch_label_style, 0);
  lv_style_set_border_opa(&s_system_switch_label_style, LV_OPA_TRANSP);
  lv_style_set_border_side(&s_system_switch_label_style, LV_BORDER_SIDE_NONE);
  lv_style_set_outline_width(&s_system_switch_label_style, 0);
  lv_style_set_outline_opa(&s_system_switch_label_style, LV_OPA_TRANSP);
  lv_style_set_shadow_width(&s_system_switch_label_style, 0);
  lv_style_set_shadow_opa(&s_system_switch_label_style, LV_OPA_TRANSP);

  lv_style_init(&s_system_dropdown_label_style);
  lv_style_set_bg_opa(&s_system_dropdown_label_style, LV_OPA_TRANSP);
  lv_style_set_text_color(&s_system_dropdown_label_style, ui_color_fg());
  lv_style_set_text_align(&s_system_dropdown_label_style, LV_TEXT_ALIGN_LEFT);
  lv_style_set_border_width(&s_system_dropdown_label_style, 0);
  lv_style_set_border_opa(&s_system_dropdown_label_style, LV_OPA_TRANSP);
  lv_style_set_border_side(&s_system_dropdown_label_style, LV_BORDER_SIDE_NONE);
  lv_style_set_outline_width(&s_system_dropdown_label_style, 0);
  lv_style_set_outline_opa(&s_system_dropdown_label_style, LV_OPA_TRANSP);
  lv_style_set_shadow_width(&s_system_dropdown_label_style, 0);
  lv_style_set_shadow_opa(&s_system_dropdown_label_style, LV_OPA_TRANSP);

  lv_style_init(&s_system_dropdown_style);
#if LV_COLOR_DEPTH == 1
  lv_style_set_pad_top(&s_system_dropdown_style, 0);
  lv_style_set_pad_bottom(&s_system_dropdown_style, 0);
  lv_style_set_pad_left(&s_system_dropdown_style, 1);
  lv_style_set_pad_right(&s_system_dropdown_style, 1);
  lv_style_set_radius(&s_system_dropdown_style, 0);
  lv_style_set_border_width(&s_system_dropdown_style, 1);
  lv_style_set_border_color(&s_system_dropdown_style, ui_color_fg());
  lv_style_set_border_opa(&s_system_dropdown_style, LV_OPA_COVER);
#else
#if defined(HELTEC_V4_R8_TFT)
  lv_style_set_pad_all(&s_system_dropdown_style, reference_card_gap());
#else
  lv_style_set_pad_hor(&s_system_dropdown_style, 4);
  lv_style_set_pad_ver(&s_system_dropdown_style, 2);
#endif
  lv_style_set_radius(&s_system_dropdown_style, ui_widget_radius_px());
  lv_style_set_border_width(&s_system_dropdown_style, 0);
  lv_style_set_border_opa(&s_system_dropdown_style, LV_OPA_TRANSP);
  lv_style_set_border_side(&s_system_dropdown_style, LV_BORDER_SIDE_NONE);
#endif
#if defined(HELTEC_V4_R8_TFT)
  // Screen focus is shown on the complete logical row. Keeping a permanent
  // blue dropdown fill makes an unfocused row look selected as well.
  lv_style_set_bg_opa(&s_system_dropdown_style, LV_OPA_TRANSP);
#else
  lv_style_set_bg_color(&s_system_dropdown_style, ui_color_panel_bg());
  lv_style_set_bg_opa(&s_system_dropdown_style, LV_OPA_COVER);
#endif
  lv_style_set_text_color(&s_system_dropdown_style, ui_color_fg());
  lv_style_set_text_align(&s_system_dropdown_style, LV_TEXT_ALIGN_CENTER);
  lv_style_set_outline_width(&s_system_dropdown_style, 0);
  lv_style_set_outline_opa(&s_system_dropdown_style, LV_OPA_TRANSP);
  lv_style_set_shadow_width(&s_system_dropdown_style, 0);
  lv_style_set_shadow_opa(&s_system_dropdown_style, LV_OPA_TRANSP);

  lv_style_init(&s_system_dropdown_focus_style);
#if LV_COLOR_DEPTH == 1
  lv_style_set_border_width(&s_system_dropdown_focus_style, 1);
  lv_style_set_border_color(&s_system_dropdown_focus_style, ui_color_fg());
  lv_style_set_border_opa(&s_system_dropdown_focus_style, LV_OPA_COVER);
#else
  lv_style_set_border_width(&s_system_dropdown_focus_style, 0);
  lv_style_set_border_opa(&s_system_dropdown_focus_style, LV_OPA_TRANSP);
  lv_style_set_border_side(&s_system_dropdown_focus_style, LV_BORDER_SIDE_NONE);
#endif
  lv_style_set_outline_width(&s_system_dropdown_focus_style, 0);
  lv_style_set_outline_opa(&s_system_dropdown_focus_style, LV_OPA_TRANSP);
  lv_style_set_shadow_width(&s_system_dropdown_focus_style, 0);
  lv_style_set_shadow_opa(&s_system_dropdown_focus_style, LV_OPA_TRANSP);

  s_screen_system_styles_ready = true;
}

static void init_navigation_styles(lv_obj_t* obj = nullptr) {
  if (s_navigation_styles_ready) return;

  lv_style_init(&s_nav_content_host_style);
  lv_style_set_bg_opa(&s_nav_content_host_style, LV_OPA_TRANSP);
  lv_style_set_border_width(&s_nav_content_host_style, 0);

  const lv_coord_t panel_radius = ui_app_frame_metrics(obj).content_radius;
#if defined(UI_NAVIGATION_GRID) && UI_NAVIGATION_GRID
  const lv_coord_t nav_radius = ui_navigation_metrics(obj).grid_cell_radius;
  const lv_coord_t icon_radius = panel_radius;
  const lv_coord_t title_radius = panel_radius;
  lv_style_init(&s_nav_grid_cell_style);
  lv_style_set_bg_color(&s_nav_grid_cell_style, lv_color_white());
  lv_style_set_bg_opa(&s_nav_grid_cell_style, LV_OPA_COVER);
  lv_style_set_border_color(&s_nav_grid_cell_style, ui_color_panel_border());
  lv_style_set_border_width(&s_nav_grid_cell_style, 1);
  lv_style_set_border_opa(&s_nav_grid_cell_style, LV_OPA_COVER);
  lv_style_set_radius(&s_nav_grid_cell_style, nav_radius);
  lv_style_set_shadow_width(&s_nav_grid_cell_style, 0);

  lv_style_init(&s_nav_grid_cell_focus_style);
  lv_style_set_border_width(&s_nav_grid_cell_focus_style, 2);

  lv_style_init(&s_nav_grid_icon_area_style);
  lv_style_set_bg_opa(&s_nav_grid_icon_area_style, LV_OPA_TRANSP);
  lv_style_set_border_width(&s_nav_grid_icon_area_style, 0);

  lv_style_init(&s_nav_grid_icon_style);
  lv_style_set_radius(&s_nav_grid_icon_style, icon_radius);
  lv_style_set_clip_corner(&s_nav_grid_icon_style, icon_radius > 0);

  lv_style_init(&s_nav_grid_title_bar_style);
  lv_style_set_bg_color(&s_nav_grid_title_bar_style, ui_color_accent());
  lv_style_set_bg_opa(&s_nav_grid_title_bar_style, LV_OPA_COVER);
  lv_style_set_border_width(&s_nav_grid_title_bar_style, 1);
  lv_style_set_border_color(&s_nav_grid_title_bar_style, lv_color_white());
  lv_style_set_border_opa(&s_nav_grid_title_bar_style, LV_OPA_COVER);
  lv_style_set_radius(&s_nav_grid_title_bar_style, title_radius);
  lv_style_set_clip_corner(&s_nav_grid_title_bar_style, title_radius > 0);

  lv_style_init(&s_nav_grid_title_label_style);
  lv_style_set_text_align(&s_nav_grid_title_label_style, LV_TEXT_ALIGN_CENTER);
  lv_style_set_text_color(&s_nav_grid_title_label_style, ui_color_fg_inv());

  lv_style_init(&s_nav_panel_base_style);
  lv_style_set_border_width(&s_nav_panel_base_style, 0);
  lv_style_set_outline_width(&s_nav_panel_base_style, 0);

  lv_style_init(&s_nav_panel_grid_style);
  lv_style_set_opa(&s_nav_panel_grid_style, LV_OPA_TRANSP);
  lv_style_set_bg_color(&s_nav_panel_grid_style, lv_color_hex(0x001765));
  lv_style_set_bg_opa(&s_nav_panel_grid_style, LV_OPA_COVER);
  lv_style_set_border_width(&s_nav_panel_grid_style, 0);
  lv_style_set_border_color(&s_nav_panel_grid_style, ui_color_panel_border());
  lv_style_set_border_opa(&s_nav_panel_grid_style, ui_effective_opa(LV_OPA_50));
  lv_style_set_radius(&s_nav_panel_grid_style, panel_radius);
  lv_style_set_clip_corner(&s_nav_panel_grid_style, panel_radius > 0);
#endif

  lv_style_init(&s_nav_root_base_style);
  lv_style_set_border_width(&s_nav_root_base_style, 0);

#if defined(UI_NAVIGATION_GRID) && UI_NAVIGATION_GRID
  lv_style_init(&s_nav_root_grid_style);
  lv_style_set_bg_opa(&s_nav_root_grid_style, LV_OPA_TRANSP);

  lv_style_init(&s_nav_footer_bar_style);
  lv_style_set_bg_color(&s_nav_footer_bar_style, ui_color_panel_bg());
  lv_style_set_bg_opa(&s_nav_footer_bar_style, LV_OPA_COVER);

  lv_style_init(&s_nav_footer_label_style);
  lv_style_set_text_color(&s_nav_footer_label_style, ui_color_fg());
#else
  lv_style_init(&s_nav_panel_ring_style);
  lv_style_set_opa(&s_nav_panel_ring_style, LV_OPA_COVER);
  lv_style_set_bg_opa(&s_nav_panel_ring_style, LV_OPA_TRANSP);

  lv_style_init(&s_nav_root_ring_style);
  lv_style_set_bg_color(&s_nav_root_ring_style, ui_color_bg());
  lv_style_set_bg_opa(&s_nav_root_ring_style, ui_effective_opa(LV_OPA_50));

  lv_style_init(&s_nav_ring_style);
  lv_style_set_border_width(&s_nav_ring_style, 0);
  lv_style_set_outline_width(&s_nav_ring_style, 0);
  lv_style_set_opa(&s_nav_ring_style, LV_OPA_COVER);
  lv_style_set_bg_opa(&s_nav_ring_style, LV_OPA_TRANSP);
#endif

  lv_style_init(&s_visibility_visible_style);
  lv_style_set_opa(&s_visibility_visible_style, LV_OPA_COVER);

  lv_style_init(&s_visibility_hidden_style);
  lv_style_set_opa(&s_visibility_hidden_style, LV_OPA_TRANSP);

  s_navigation_styles_ready = true;
}

static lv_style_t* overlay_root_chrome_style() {
  if (!s_overlay_root_chrome_style_ready) {
    lv_style_init(&s_overlay_root_chrome_style);
    lv_style_set_bg_opa(&s_overlay_root_chrome_style, LV_OPA_COVER);
    lv_style_set_bg_color(&s_overlay_root_chrome_style, ui_color_overlay_bg());
    lv_style_set_border_width(&s_overlay_root_chrome_style, 0);
    lv_style_set_border_opa(&s_overlay_root_chrome_style, LV_OPA_TRANSP);
    lv_style_set_border_side(&s_overlay_root_chrome_style, LV_BORDER_SIDE_NONE);
    lv_style_set_outline_width(&s_overlay_root_chrome_style, 0);
    lv_style_set_outline_opa(&s_overlay_root_chrome_style, LV_OPA_TRANSP);
    lv_style_set_shadow_width(&s_overlay_root_chrome_style, 0);
    lv_style_set_shadow_opa(&s_overlay_root_chrome_style, LV_OPA_TRANSP);
    lv_style_set_radius(&s_overlay_root_chrome_style, 0);
    s_overlay_root_chrome_style_ready = true;
  }
  return &s_overlay_root_chrome_style;
}

static bool is_screen_root_id(MetaId id) {
  switch (id) {
    case meta_id::ScreenRoot:
    case meta_id::HomeScreenRoot:
    case meta_id::GpsScreenRoot:
    case meta_id::RadioScreenRoot:
    case meta_id::RecentScreenRoot:
    case meta_id::CompassScreenRoot:
    case meta_id::FindFriendScreenRoot:
    case meta_id::TrackerScreenRoot:
    case meta_id::SystemRoot:
      return true;
    default:
      return false;
  }
}

static bool is_overlay_root_id(MetaId id) {
  switch (id) {
    case meta_id::OverlayRoot:
    case meta_id::PreviewOverlayRoot:
    case meta_id::AlertOverlayRoot:
    case meta_id::CalibrationOverlayRoot:
    case meta_id::KeyboardOverlayRoot:
    case meta_id::RadioParamSyncOverlayRoot:
    case meta_id::ChoicePickerOverlayRoot:
    case meta_id::SendMessageOverlayRoot:
    case meta_id::SplashOverlayRoot:
      return true;
    default:
      return false;
  }
}

static void apply_surface_root_common(_lv_obj_t* obj) {
  if (!obj) return;
  init_common_widget_styles();
  lv_obj_add_style(obj, &s_surface_root_common_style, LV_PART_MAIN);
}

static void style_plain_container(_lv_obj_t* obj) {
  if (!obj) return;
  init_common_widget_styles();
  lv_obj_add_style(obj, &s_plain_container_style, LV_PART_MAIN);
}

static void style_top_pane_slot(_lv_obj_t* obj) {
  if (!obj) return;
  init_common_widget_styles();
  lv_obj_add_style(obj, &s_top_pane_slot_style, LV_PART_MAIN);
  lv_obj_add_style(obj, &s_top_pane_slot_style, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
}

static void reset_touch_object(lv_obj_t* obj) {
  if (!obj) return;
  lv_obj_remove_style_all(obj);
  init_common_widget_styles();
  lv_obj_add_style(obj, &s_touch_reset_style, LV_PART_MAIN);
}

static void apply_no_chrome(_lv_obj_t* obj) {
  if (!obj) return;
  init_common_widget_styles();
  for (lv_style_selector_t selector : kMainNoChromeSelectors) {
    lv_obj_add_style(obj, &s_no_chrome_style, selector);
  }
}

static void apply_system_control_no_chrome(_lv_obj_t* obj) {
  if (!obj) return;
  init_common_widget_styles();
  for (lv_style_selector_t selector : kSystemControlNoChromeSelectors) {
    lv_obj_add_style(obj, &s_no_chrome_style, selector);
  }
}

static void hide_scrollbar_chrome(_lv_obj_t* obj) {
  if (!obj) return;
  init_common_widget_styles();
  for (lv_style_selector_t selector : kScrollbarHiddenSelectors) {
    lv_obj_add_style(obj, &s_hidden_scrollbar_style, selector);
  }
}

static void style_screen_label(_lv_obj_t* obj, lv_text_align_t align = LV_TEXT_ALIGN_LEFT) {
  if (!obj) return;
  lv_style_t* style = cached_label_style(ui_color_fg(), align);
  if (style) lv_obj_add_style(obj, style, LV_PART_MAIN);
  apply_no_chrome(obj);
}

static void style_overlay_label(_lv_obj_t* obj, lv_color_t color,
                                lv_text_align_t align = LV_TEXT_ALIGN_CENTER) {
  if (!obj) return;
  lv_style_t* style = cached_label_style(color, align);
  if (style) lv_obj_add_style(obj, style, LV_PART_MAIN);
  lv_obj_set_style_text_line_space(obj, 0, LV_PART_MAIN);
  lv_obj_set_style_text_letter_space(obj, 0, LV_PART_MAIN);
}

static void apply_overlay_root_chrome(_lv_obj_t* obj) {
  if (!obj) return;
  lv_obj_add_style(obj, overlay_root_chrome_style(), LV_PART_MAIN);
}

static bool apply_top_pane_root(_lv_obj_t* obj) {
  if (ht_id(obj) != meta_id::TopPaneRoot) return false;
  init_top_pane_styles(obj);
  lv_obj_add_style(obj, &s_top_pane_root_style, LV_PART_MAIN);

  return true;
}

static bool apply_top_pane_direct_child(_lv_obj_t* obj, _lv_obj_t* parent) {
  if (ht_id(parent) != meta_id::TopPaneRoot) return false;

  switch (ht_id(obj)) {
    case meta_id::TopPaneLeftSlot:
    case meta_id::TopPaneRightSlot:
      style_top_pane_slot(obj);
      return true;

    case meta_id::TopPaneCenterSlot:
      style_top_pane_slot(obj);
      return true;

    case meta_id::TopPaneBattery:
      init_top_pane_styles(parent);
      lv_obj_add_style(obj, &s_top_pane_battery_style, LV_PART_MAIN);
      return true;

    default:
      return false;
  }
}

static bool apply_top_pane_nested_child(_lv_obj_t* obj) {
  switch (ht_id(obj)) {
    case meta_id::TopPaneTitle:
      init_top_pane_styles(obj);
      lv_obj_add_style(obj, &s_top_pane_title_style, LV_PART_MAIN);
      return true;

    case meta_id::TopPaneBatteryOutline: {
      init_top_pane_styles(obj);
      lv_obj_add_style(obj, &s_top_pane_battery_outline_style, LV_PART_MAIN);
#if defined(HELTEC_V4_R8_TFT)
      lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN);
      lv_obj_set_style_border_color(obj, lv_color_white(), LV_PART_MAIN);
      lv_obj_set_style_border_opa(obj, LV_OPA_COVER, LV_PART_MAIN);
#endif
      return true;
    }

    case meta_id::TopPaneBatteryCap: {
      init_top_pane_styles(obj);
      lv_obj_add_style(obj, &s_top_pane_battery_cap_style, LV_PART_MAIN);
      return true;
    }

    case meta_id::TopPaneBatteryFill: {
      init_top_pane_styles(obj);
      lv_obj_add_style(obj, &s_top_pane_battery_fill_style, LV_PART_MAIN);
      return true;
    }

    default:
      return false;
  }
}

static bool apply_surface_root_theme(_lv_obj_t* obj) {
  const MetaId id = ht_id(obj);
  if (is_screen_root_id(id)) {
    apply_surface_root_common(obj);
    apply_no_chrome(obj);
    hide_scrollbar_chrome(obj);
    return true;
  }
  if (is_overlay_root_id(id)) {
    apply_surface_root_common(obj);
    switch (id) {
      case meta_id::PreviewOverlayRoot:
        apply_overlay_root_chrome(obj);
        break;
      case meta_id::AlertOverlayRoot:
        init_surface_app_styles(obj);
        for (lv_style_selector_t sel : kInteractiveSelectors) {
          lv_obj_add_style(obj, &s_alert_overlay_root_style, sel);
        }
        break;
      case meta_id::CalibrationOverlayRoot:
        init_surface_app_styles(obj);
        lv_obj_add_style(obj, &s_calibration_overlay_root_style, LV_PART_MAIN);
        break;
      case meta_id::KeyboardOverlayRoot:
        apply_overlay_root_chrome(obj);
        break;
      case meta_id::RadioParamSyncOverlayRoot:
      case meta_id::ChoicePickerOverlayRoot:
        apply_overlay_root_chrome(obj);
        break;
      case meta_id::SendMessageOverlayRoot:
        apply_overlay_root_chrome(obj);
        break;
      case meta_id::SplashOverlayRoot: {
        apply_overlay_root_chrome(obj);
        const lv_font_t* font = lv_theme_get_font_normal(obj);
        if (!font) font = LV_FONT_DEFAULT;
        if (!s_splash_overlay_text_style_ready) {
          lv_style_init(&s_splash_overlay_text_style);
          s_splash_overlay_text_style_ready = true;
        } else if (s_splash_overlay_text_font != font) {
          lv_style_reset(&s_splash_overlay_text_style);
        }
        if (s_splash_overlay_text_font != font) {
          s_splash_overlay_text_font = font;
          lv_style_set_text_font(&s_splash_overlay_text_style, font);
          set_overlay_text_spacing(&s_splash_overlay_text_style);
        }
        lv_obj_add_style(obj, &s_splash_overlay_text_style, LV_PART_MAIN);
#if defined(HELTEC_V4_R8_TFT)
        lv_obj_set_style_bg_color(obj, lv_color_hex(0x001765), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN);
#endif
        break;
      }
      default:
        break;
    }
    return true;
  }
#if !defined(HELTEC_V4_R8_TFT) || !defined(HELTEC_HAS_TOUCH) || !HELTEC_HAS_TOUCH
  if (id == meta_id::ContextMenuRoot) {
    apply_surface_root_common(obj);
    init_surface_app_styles(obj);
    for (lv_style_selector_t selector : kContextMenuRootSelectors) {
      lv_obj_add_style(obj, &s_context_menu_root_style, selector);
    }
    return true;
  }
#endif
  return false;
}

static bool apply_app_frame_theme(_lv_obj_t* obj) {
  if (!obj) return false;

  switch (ht_id(obj)) {
    case meta_id::AppBackgroundImage:
      // Created before metric-bearing frame/overlay nodes; keep it from seeding surface styles.
      return true;

    case meta_id::AppOverlayLayer:
      init_surface_app_styles(obj);
      lv_obj_add_style(obj, &s_app_overlay_layer_style, LV_PART_MAIN);
      return true;

    case meta_id::AppFrameLayout:
      init_surface_app_styles(obj);
      lv_obj_add_style(obj, &s_app_frame_layout_style, LV_PART_MAIN);
      return true;

    case meta_id::AppContent: {
      init_surface_app_styles(obj);
      lv_obj_add_style(obj, &s_app_content_style, LV_PART_MAIN);
      return true;
    }

    case meta_id::AppScreenRoot: {
      init_surface_app_styles(obj);
      lv_obj_add_style(obj, &s_app_screen_root_style, LV_PART_MAIN);
      apply_no_chrome(obj);
      hide_scrollbar_chrome(obj);
      return true;
    }

    case meta_id::AppTileView:
      init_surface_app_styles(obj);
      lv_obj_add_style(obj, &s_app_tileview_style, LV_PART_MAIN);
      apply_no_chrome(obj);
      hide_scrollbar_chrome(obj);
      return true;

    case meta_id::AppTile:
      init_surface_app_styles(obj);
      hide_scrollbar_chrome(obj);
      lv_obj_add_style(obj, &s_app_tile_style, LV_PART_MAIN);
      apply_no_chrome(obj);
      return true;

    default:
      return false;
  }
}

static bool apply_classic_context_menu_child_theme(_lv_obj_t* obj) {
#if !defined(HELTEC_V4_R8_TFT) || !defined(HELTEC_HAS_TOUCH) || !HELTEC_HAS_TOUCH
  if (!obj) return false;
  switch (ht_id(obj)) {
    case meta_id::ContextMenuMenu:
      init_classic_context_styles(obj);
      lv_obj_add_style(obj, &s_ctx_menu_style, LV_PART_MAIN);
      return true;

    case meta_id::ContextMenuHeader:
      init_classic_context_styles(obj);
      for (lv_style_selector_t selector : kContextMenuRootSelectors) {
        lv_obj_add_style(obj, &s_ctx_header_style, selector);
      }
      return true;

    case meta_id::ContextMenuHeaderIconRow: {
      init_classic_context_styles(obj);
      for (lv_style_selector_t selector : kContextMenuRootSelectors) {
        lv_obj_add_style(obj, &s_ctx_icon_row_style, selector);
      }
      return true;
    }

    case meta_id::ContextMenuHeaderNavRow:
      init_classic_context_styles(obj);
      for (lv_style_selector_t selector : kContextMenuRootSelectors) {
        lv_obj_add_style(obj, &s_ctx_nav_row_style, selector);
      }
      return true;

    case meta_id::ContextMenuBackButton:
      return true;

    case meta_id::ContextMenuTitle:
      init_classic_context_styles(obj);
      for (lv_style_selector_t selector : kContextMenuRootSelectors) {
        lv_obj_add_style(obj, &s_ctx_title_style, selector);
      }
      return true;

    case meta_id::ContextMenuIconButton: {
      init_classic_context_styles(obj);
      lv_obj_add_style(obj, &s_ctx_icon_btn_main_style, LV_PART_MAIN);
      lv_obj_add_style(obj, &s_ctx_icon_btn_checked_style, LV_PART_MAIN | LV_STATE_CHECKED);
      return true;
    }

    case meta_id::ContextMenuIcon: {
      init_classic_context_styles(obj);
      lv_obj_add_style(obj, &s_ctx_icon_main_style, LV_PART_MAIN);
      lv_obj_add_style(obj, &s_ctx_icon_checked_style, LV_PART_MAIN | LV_STATE_CHECKED);
      return true;
    }

    default:
      break;
  }
#else
  (void)obj;
#endif
  return false;
}

#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
static bool apply_quick_ping_overlay_theme(_lv_obj_t* obj) {
  if (!obj) return false;
  switch (ht_id(obj)) {
    case meta_id::QuickPingOverlayRoot:
      init_quick_ping_styles();
      reset_touch_object(obj);
      lv_obj_add_style(obj, &s_quick_ping_root_style, LV_PART_MAIN);
      return true;

    case meta_id::QuickPingTitleBar:
      init_quick_ping_styles();
      reset_touch_object(obj);
      lv_obj_add_style(obj, &s_quick_ping_title_bar_style, LV_PART_MAIN);
      return true;

    case meta_id::QuickPingContent:
      init_quick_ping_styles();
      reset_touch_object(obj);
      lv_obj_add_style(obj, &s_quick_ping_content_style, LV_PART_MAIN);
      return true;

    case meta_id::QuickPingTitle:
      init_quick_ping_styles();
      reset_touch_object(obj);
      lv_obj_add_style(obj, &s_quick_ping_title_style, LV_PART_MAIN);
      return true;

    case meta_id::QuickPingRow:
      init_quick_ping_styles();
      reset_touch_object(obj);
      lv_obj_add_style(obj, &s_quick_ping_row_style, LV_PART_MAIN);
      return true;

    case meta_id::QuickPingLabel:
      init_quick_ping_styles();
      reset_touch_object(obj);
      lv_obj_add_style(obj, &s_quick_ping_label_style, LV_PART_MAIN);
      return true;

    case meta_id::QuickPingDropdown:
    case meta_id::QuickPingMessageDropdown:
      init_quick_ping_styles();
      reset_touch_object(obj);
      lv_obj_add_style(obj, &s_quick_ping_dropdown_style, LV_PART_MAIN);
      if (ht_id(obj) == meta_id::QuickPingMessageDropdown) {
        lv_obj_add_style(obj, &s_quick_ping_message_dropdown_style, LV_PART_MAIN);
      }
      lv_obj_add_style(obj, &s_quick_ping_dropdown_focus_style,
                       LV_PART_MAIN | LV_STATE_FOCUSED);
      lv_obj_add_style(obj, &s_quick_ping_dropdown_disabled_style,
                       LV_PART_MAIN | LV_STATE_DISABLED);
      lv_obj_add_style(obj, &s_quick_ping_dropdown_indicator_style, LV_PART_INDICATOR);
      lv_obj_add_style(obj, &s_quick_ping_dropdown_indicator_focus_style,
                       LV_PART_INDICATOR | LV_STATE_FOCUSED);
      return true;

    case meta_id::QuickPingMessageInput:
      init_quick_ping_styles();
      reset_touch_object(obj);
      lv_obj_add_style(obj, &s_quick_ping_message_input_style, LV_PART_MAIN);
      lv_obj_add_style(obj, &s_quick_ping_message_input_focus_style,
                       LV_PART_MAIN | LV_STATE_FOCUSED);
      lv_obj_add_style(obj, &s_quick_ping_message_input_focus_style,
                       LV_PART_MAIN | LV_STATE_PRESSED);
      lv_obj_add_style(obj, &s_quick_ping_dropdown_disabled_style,
                       LV_PART_MAIN | LV_STATE_DISABLED);
      return true;

    case meta_id::QuickPingMessageInputLabel:
      init_quick_ping_styles();
      reset_touch_object(obj);
      lv_obj_add_style(obj, &s_quick_ping_message_input_label_style, LV_PART_MAIN);
      lv_obj_add_style(obj, &s_quick_ping_message_input_label_focus_style,
                       LV_PART_MAIN | LV_STATE_FOCUSED);
      return true;

    case meta_id::QuickPingKeyboard: {
      init_quick_ping_styles();
      reset_touch_object(obj);
      lv_obj_add_style(obj, &s_quick_ping_keyboard_style, LV_PART_MAIN);
      lv_obj_add_style(obj, &s_quick_ping_keyboard_style,
                       LV_PART_MAIN | LV_STATE_FOCUSED);
      lv_obj_add_style(obj, &s_quick_ping_keyboard_style,
                       LV_PART_MAIN | LV_STATE_PRESSED);
      static const lv_style_selector_t base_item_sels[] = {
          LV_PART_ITEMS,
          LV_PART_ITEMS | LV_STATE_CHECKED,
          LV_PART_ITEMS | LV_STATE_PRESSED,
      };
      for (lv_style_selector_t sel : base_item_sels) {
        lv_obj_add_style(obj, &s_quick_ping_keyboard_items_style, sel);
      }
      static const lv_style_selector_t selected_item_sels[] = {
          LV_PART_ITEMS | LV_STATE_FOCUSED,
          LV_PART_ITEMS | LV_STATE_FOCUS_KEY,
          LV_PART_ITEMS | LV_STATE_EDITED,
          LV_PART_ITEMS | (LV_STATE_FOCUSED | LV_STATE_CHECKED),
          LV_PART_ITEMS | (LV_STATE_FOCUS_KEY | LV_STATE_CHECKED),
          LV_PART_ITEMS | (LV_STATE_EDITED | LV_STATE_CHECKED),
      };
      for (lv_style_selector_t sel : selected_item_sels) {
        lv_obj_add_style(obj, &s_quick_ping_keyboard_items_selected_style, sel);
      }
      return true;
    }

    default:
      return false;
  }
}
#else
static bool apply_quick_ping_overlay_theme(_lv_obj_t*) { return false; }
#endif

static bool apply_context_menu_child_theme(_lv_obj_t* obj) {
  return apply_classic_context_menu_child_theme(obj) || apply_quick_ping_overlay_theme(obj);
}

static bool apply_home_screen_child_theme(_lv_obj_t* obj) {
  switch (ht_id(obj)) {
    case meta_id::HomeIdLabel:
    case meta_id::HomeMessageLabel:
    case meta_id::HomeStatusLabel:
      style_screen_label(obj, LV_TEXT_ALIGN_CENTER);
      apply_no_chrome(obj);
      hide_scrollbar_chrome(obj);
      return true;
    default:
      return false;
  }
}

static bool apply_gps_screen_child_theme(_lv_obj_t* obj) {
  switch (ht_id(obj)) {
    case meta_id::GpsPowerRow:
    case meta_id::GpsTrackRow:
      style_plain_container(obj);
      return true;

    case meta_id::GpsFixLabel:
    case meta_id::GpsSatLabel:
    case meta_id::GpsLatLonLabel:
    case meta_id::GpsAltLabel:
    case meta_id::GpsRawLabel:
      style_screen_label(obj);
      if (ht_id(obj) == meta_id::GpsRawLabel) {
        init_screen_system_styles();
        lv_obj_add_style(obj, &s_warning_text_style, LV_PART_MAIN);
      }
      return true;

    case meta_id::GpsPowerPrefix:
    case meta_id::GpsTrackLabel:
      style_screen_label(obj);
      return true;

    case meta_id::GpsPowerSwitch:
    case meta_id::GpsTrackSwitch:
      // The active UI theme owns all switch-part styling.
      return true;

    default:
      return false;
  }
}

static bool apply_recent_screen_child_theme(_lv_obj_t* obj) {
  switch (ht_id(obj)) {
    case meta_id::RecentRowLabel:
      style_screen_label(obj);
      return true;
    case meta_id::RecentSendButton:
      return true;
    case meta_id::RecentSendButtonLabel:
      style_screen_label(obj, LV_TEXT_ALIGN_CENTER);
      apply_no_chrome(obj);
      return true;
    case meta_id::RecentDetailContact:
      style_screen_label(obj, LV_TEXT_ALIGN_CENTER);
      return true;
    case meta_id::RecentDetailMessage:
      style_screen_label(obj);
      return true;
    default:
      return false;
  }
}

static bool apply_radio_screen_child_theme(_lv_obj_t* obj) {
  switch (ht_id(obj)) {
    case meta_id::RadioLineLabel:
    case meta_id::RadioLnaLabel:
      style_screen_label(obj);
      return true;
    case meta_id::RadioLnaRow:
      style_plain_container(obj);
      return true;
    case meta_id::RadioLnaSwitch:
      return true;
    default:
      return false;
  }
}

static void style_system_row_common(_lv_obj_t* row, lv_style_t* style) {
  if (!row) return;
  init_screen_system_styles();
  if (style) lv_obj_add_style(row, style, LV_PART_MAIN);
}

static bool apply_system_screen_child_theme(_lv_obj_t* obj) {
  if (!obj) return false;

  switch (ht_id(obj)) {
    case meta_id::SystemActionRow:
      style_system_row_common(obj, &s_system_action_row_style);
      return true;

    case meta_id::SystemSwitchRow:
      style_system_row_common(obj, &s_system_switch_row_style);
      return true;

    case meta_id::SystemDropdownRow:
      style_system_row_common(obj, &s_system_dropdown_row_style);
      return true;

    case meta_id::SystemActionLabel:
      init_screen_system_styles();
      lv_obj_add_style(obj, &s_system_action_label_style, LV_PART_MAIN);
      return true;

    case meta_id::SystemSwitchLabel:
      init_screen_system_styles();
      lv_obj_add_style(obj, &s_system_switch_label_style, LV_PART_MAIN);
      return true;

    case meta_id::SystemDropdownLabel:
      init_screen_system_styles();
      lv_obj_add_style(obj, &s_system_dropdown_label_style, LV_PART_MAIN);
      return true;

    case meta_id::SystemDropdown:
      init_screen_system_styles();
      apply_system_control_no_chrome(obj);
      lv_obj_add_style(obj, &s_system_dropdown_style, LV_PART_MAIN);
      lv_obj_add_style(obj, &s_system_dropdown_focus_style,
                       LV_PART_MAIN | LV_STATE_FOCUS_KEY);
      lv_obj_add_style(obj, &s_system_dropdown_focus_style,
                       LV_PART_MAIN | LV_STATE_FOCUSED);
      hide_scrollbar_chrome(obj);
      return true;

    case meta_id::SystemSwitch:
      init_screen_system_styles();
      // The active UI theme fully styles each switch part. Applying the shared
      // no-chrome style here also adds transparent FOCUSED/CHECKED selectors,
      // which are more specific than the switch's base part styles and can
      // make System switches render black/invisible across board themes.
      return true;

    default:
      break;
  }
  return false;
}

static bool apply_screen_child_theme(_lv_obj_t* obj) {
  if (ht_id(obj) == meta_id::ScreenClipLabel) {
    style_screen_label(obj);
    return true;
  }
  return apply_home_screen_child_theme(obj) ||
         apply_gps_screen_child_theme(obj) ||
         apply_recent_screen_child_theme(obj) ||
         apply_radio_screen_child_theme(obj) ||
         apply_system_screen_child_theme(obj);
}

static bool apply_preview_overlay_child_theme(_lv_obj_t* obj) {
  switch (ht_id(obj)) {
    case meta_id::PreviewHeader:
      style_plain_container(obj);
      return true;

    case meta_id::PreviewOrigin:
      style_overlay_label(obj, ui_color_accent_alt(), LV_TEXT_ALIGN_LEFT);
      return true;

    case meta_id::PreviewText:
      style_overlay_label(obj, ui_color_overlay_fg(), LV_TEXT_ALIGN_LEFT);
      return true;

    case meta_id::PreviewFooter:
      style_overlay_label(obj, ui_color_overlay_fg());
      return true;

    case meta_id::PreviewTitle:
    case meta_id::PreviewAge:
      init_screen_system_styles();
      lv_obj_add_style(obj, &s_accent_text_style, LV_PART_MAIN);
      return true;

    default:
      return false;
  }
}

static void style_overlay_label_all_states(_lv_obj_t* label) {
  if (!label) return;
  init_overlay_feedback_styles();
  for (lv_style_selector_t sel : kInteractiveSelectors) {
    lv_obj_add_style(label, &s_overlay_label_all_states_style, sel);
  }
}

static bool apply_alert_overlay_child_theme(_lv_obj_t* obj) {
  if (ht_id(obj) == meta_id::AlertBox) {
    init_overlay_feedback_styles();
    for (lv_style_selector_t sel : kInteractiveSelectors) {
      lv_obj_add_style(obj, &s_alert_box_style, sel);
    }
    return true;
  }

  if (ht_id(obj) == meta_id::AlertLabel) {
    style_overlay_label_all_states(obj);
    return true;
  }
  return false;
}

static bool apply_calibration_overlay_child_theme(_lv_obj_t* obj) {
  if (ht_id(obj) == meta_id::CalibrationPanel) {
    init_overlay_feedback_styles();
    for (lv_style_selector_t sel : kInteractiveSelectors) {
      lv_obj_add_style(obj, &s_calibration_panel_style, sel);
    }
    return true;
  }

  if (ht_id(obj) == meta_id::CalibrationBody || ht_id(obj) == meta_id::CalibrationFooter) {
    style_overlay_label_all_states(obj);
    return true;
  }
  return false;
}

static bool apply_keyboard_overlay_child_theme(_lv_obj_t* obj) {
  switch (ht_id(obj)) {
    case meta_id::KeyboardTitle:
      init_keyboard_child_styles();
      lv_obj_add_style(obj, &s_keyboard_title_style, LV_PART_MAIN);
      return true;

    case meta_id::KeyboardTextarea:
      init_keyboard_child_styles();
      lv_obj_add_style(obj, &s_keyboard_textarea_style, LV_PART_MAIN);
      return true;

    case meta_id::KeyboardKeyboard: {
      init_keyboard_child_styles();
      lv_obj_add_style(obj, &s_keyboard_main_style, LV_PART_MAIN);
      lv_obj_add_style(obj, &s_keyboard_main_style, LV_PART_MAIN | LV_STATE_FOCUSED);
      lv_obj_add_style(obj, &s_keyboard_main_style, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
      lv_obj_add_style(obj, &s_keyboard_main_style, LV_PART_MAIN | LV_STATE_EDITED);
      lv_obj_add_style(obj, &s_keyboard_main_style, LV_PART_MAIN | LV_STATE_PRESSED);
      lv_obj_add_style(obj, &s_keyboard_main_style, LV_PART_MAIN | LV_STATE_CHECKED);
      static const lv_style_selector_t base_item_sels[] = {
          LV_PART_ITEMS,
          LV_PART_ITEMS | LV_STATE_CHECKED,
          LV_PART_ITEMS | LV_STATE_PRESSED,
      };
      for (lv_style_selector_t sel : base_item_sels) {
        lv_obj_add_style(obj, &s_keyboard_items_base_style, sel);
      }
      static const lv_style_selector_t selected_item_sels[] = {
          LV_PART_ITEMS | LV_STATE_FOCUSED,
          LV_PART_ITEMS | LV_STATE_FOCUS_KEY,
          LV_PART_ITEMS | LV_STATE_EDITED,
          LV_PART_ITEMS | (LV_STATE_FOCUSED | LV_STATE_CHECKED),
          LV_PART_ITEMS | (LV_STATE_FOCUS_KEY | LV_STATE_CHECKED),
          LV_PART_ITEMS | (LV_STATE_EDITED | LV_STATE_CHECKED),
      };
      for (lv_style_selector_t sel : selected_item_sels) {
        lv_obj_add_style(obj, &s_keyboard_items_selected_style, sel);
      }
      return true;
    }

    default:
      return false;
  }
}

static bool apply_radio_param_sync_overlay_child_theme(_lv_obj_t* obj) {
  switch (ht_id(obj)) {
    case meta_id::RadioParamSyncTitle:
    case meta_id::RadioParamSyncFooter:
    case meta_id::ChoicePickerTitle:
    case meta_id::ChoicePickerFooter:
      init_radio_sync_styles();
      lv_obj_add_style(obj, &s_radio_sync_overlay_label_style, LV_PART_MAIN);
      return true;

    case meta_id::RadioParamSyncList:
    case meta_id::ChoicePickerList:
      init_radio_sync_styles();
      lv_obj_add_style(obj, &s_radio_sync_list_style, LV_PART_MAIN);
      return true;

    case meta_id::RadioParamSyncRow:
    case meta_id::ChoicePickerRow:
      init_radio_sync_styles();
      lv_obj_add_style(obj, &s_radio_sync_row_main_style, LV_PART_MAIN);
      lv_obj_add_style(obj, &s_radio_sync_row_checked_style,
                       LV_PART_MAIN | LV_STATE_CHECKED);
      return true;

    default:
      return false;
  }
}

static bool apply_send_message_overlay_child_theme(_lv_obj_t* obj) {
  switch (ht_id(obj)) {
    case meta_id::SendMessageTitle:
    case meta_id::SendMessageFooter:
      init_send_message_styles();
      lv_obj_add_style(obj, &s_send_overlay_label_style, LV_PART_MAIN);
      return true;

    case meta_id::SendMessageList:
      init_send_message_styles();
      lv_obj_add_style(obj, &s_send_list_style, LV_PART_MAIN);
      return true;

    case meta_id::SendMessageTouchList:
      init_send_message_styles();
      lv_obj_add_style(obj, &s_send_touch_list_style, LV_PART_MAIN);
      return true;

    case meta_id::SendMessageRow:
      init_send_message_styles();
      lv_obj_add_style(obj, &s_send_row_main_style, LV_PART_MAIN);
      lv_obj_add_style(obj, &s_send_row_active_style,
                       LV_PART_MAIN | LV_STATE_FOCUSED);
      lv_obj_add_style(obj, &s_send_row_active_style,
                       LV_PART_MAIN | LV_STATE_CHECKED);
      lv_obj_add_style(obj, &s_send_row_active_style,
                       LV_PART_MAIN | LV_STATE_PRESSED);
      return true;

    case meta_id::SendMessageRowLabel:
      init_send_message_styles();
      lv_obj_add_style(obj, &s_send_row_label_main_style, LV_PART_MAIN);
      lv_obj_add_style(obj, &s_send_row_label_active_style,
                       LV_PART_MAIN | LV_STATE_FOCUSED);
      lv_obj_add_style(obj, &s_send_row_label_active_style,
                       LV_PART_MAIN | LV_STATE_CHECKED);
      lv_obj_add_style(obj, &s_send_row_label_active_style,
                       LV_PART_MAIN | LV_STATE_PRESSED);
      return true;

    default:
      return false;
  }
}

static bool apply_splash_overlay_child_theme(_lv_obj_t* obj) {
  switch (ht_id(obj)) {
    case meta_id::SplashVersion:
    case meta_id::SplashDate:
    case meta_id::SplashAttribution:
      style_overlay_label(obj, ui_color_overlay_fg());
      return true;
    case meta_id::SplashLogo:
#if defined(HELTEC_V4_R8_TFT)
      lv_obj_set_style_img_recolor(obj, ui_color_overlay_fg(), LV_PART_MAIN);
      lv_obj_set_style_img_recolor_opa(obj, LV_OPA_COVER, LV_PART_MAIN);
#endif
      return true;
    default:
      return false;
  }
}

static bool apply_overlay_child_theme(_lv_obj_t* obj) {
  return apply_alert_overlay_child_theme(obj) ||
         apply_calibration_overlay_child_theme(obj) ||
         apply_keyboard_overlay_child_theme(obj) ||
         apply_preview_overlay_child_theme(obj) ||
         apply_radio_param_sync_overlay_child_theme(obj) ||
         apply_send_message_overlay_child_theme(obj) ||
         apply_splash_overlay_child_theme(obj);
}

static void style_navigator_content_host(lv_obj_t* content) {
  if (!content) return;
  init_navigation_styles(content);
  lv_obj_remove_style_all(content);
  lv_obj_add_style(content, &s_nav_content_host_style, LV_PART_MAIN);
}

#if defined(UI_NAVIGATION_GRID) && UI_NAVIGATION_GRID
static void apply_navigator_grid_cell_theme(lv_obj_t* cell) {
  if (!cell) return;
  init_navigation_styles(cell);

  static const lv_style_selector_t sels[] = {
      LV_PART_MAIN,
      LV_PART_MAIN | LV_STATE_PRESSED,
      LV_PART_MAIN | LV_STATE_FOCUS_KEY,
      LV_PART_MAIN | LV_STATE_FOCUS_KEY | LV_STATE_PRESSED,
      LV_PART_MAIN | LV_STATE_FOCUSED,
      LV_PART_MAIN | LV_STATE_FOCUSED | LV_STATE_PRESSED,
  };
  for (lv_style_selector_t sel : sels) {
    const bool focus_sel = (sel & (LV_STATE_FOCUS_KEY | LV_STATE_FOCUSED)) != 0;
    lv_obj_add_style(cell, &s_nav_grid_cell_style, sel);
    if (focus_sel) lv_obj_add_style(cell, &s_nav_grid_cell_focus_style, sel);
  }
}

static void apply_navigator_grid_icon_area_theme(lv_obj_t* area) {
  if (!area) return;
  init_navigation_styles(area);
  lv_obj_add_style(area, &s_nav_grid_icon_area_style, LV_PART_MAIN);
}

static void apply_navigator_grid_icon_theme(lv_obj_t* icon) {
  if (!icon) return;
  init_navigation_styles(icon);
  lv_obj_add_style(icon, &s_nav_grid_icon_style, LV_PART_MAIN);
}

static void apply_navigator_grid_title_bar_theme(lv_obj_t* bar) {
  if (!bar) return;
  init_navigation_styles(bar);
  static const lv_style_selector_t sels[] = {
      LV_PART_MAIN,
      LV_PART_MAIN | LV_STATE_PRESSED,
  };
  for (lv_style_selector_t sel : sels) {
    lv_obj_add_style(bar, &s_nav_grid_title_bar_style, sel);
  }
}

static void apply_navigator_grid_title_label_theme(lv_obj_t* label) {
  if (!label) return;
  init_navigation_styles(label);
  lv_obj_add_style(label, &s_nav_grid_title_label_style, LV_PART_MAIN);
}
#endif

static void apply_navigator_panel_chrome(lv_obj_t* obj) {
  if (!obj) return;
  init_navigation_styles(obj);

  lv_obj_add_style(obj, &s_nav_panel_base_style, LV_PART_MAIN);
  lv_obj_add_style(obj, &s_nav_panel_base_style, LV_PART_MAIN | LV_STATE_FOCUS_KEY);

#if defined(UI_NAVIGATION_GRID) && UI_NAVIGATION_GRID
  lv_obj_add_style(obj, &s_nav_panel_grid_style, LV_PART_MAIN);
#else
  lv_obj_add_style(obj, &s_nav_panel_ring_style, LV_PART_MAIN);
#endif
  lv_obj_add_style(obj, &s_visibility_visible_style, LV_PART_MAIN);
  lv_obj_add_style(obj, &s_visibility_hidden_style, LV_PART_MAIN | LV_STATE_USER_4);
}

}  // namespace

static bool apply_top_pane_theme(_lv_obj_t* obj) {
  if (!obj) return false;
  _lv_obj_t* parent = lv_obj_get_parent(obj);
  if (apply_top_pane_root(obj)) return true;
  if (apply_top_pane_direct_child(obj, parent)) return true;
  if (apply_top_pane_nested_child(obj)) return true;
  return false;
}

void ui_app_active_screen_apply_theme(_lv_obj_t* obj) {
  if (!obj) return;
  init_active_screen_style();
  lv_obj_add_style(obj, &s_active_screen_style, LV_PART_MAIN);
}

#if !defined(ENV_INCLUDE_MAP) || !ENV_INCLUDE_MAP
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

#if !defined(ENV_INCLUDE_COMPASS) || !ENV_INCLUDE_COMPASS
bool ui_compass_widget_apply_theme(_lv_obj_t* obj) {
  (void)obj;
  return false;
}
#endif

static bool apply_navigation_theme(_lv_obj_t* obj) {
  if (!obj) return false;
  switch (ht_id(obj)) {
    case meta_id::NavigationRoot:
      init_navigation_styles(obj);
      lv_obj_add_style(obj, &s_nav_root_base_style, LV_PART_MAIN);
#if defined(UI_NAVIGATION_GRID) && UI_NAVIGATION_GRID
      lv_obj_add_style(obj, &s_nav_root_grid_style, LV_PART_MAIN);
#else
      lv_obj_add_style(obj, &s_nav_root_ring_style, LV_PART_MAIN);
#endif
      return true;

    case meta_id::NavigationPanel:
      apply_navigator_panel_chrome(obj);
      if (_lv_obj_t* content = ui_navigator_content(obj)) {
        if (content != obj) style_navigator_content_host(content);
      }
      return true;
    case meta_id::NavigationContent:
      style_navigator_content_host(obj);
      return true;
#if defined(UI_NAVIGATION_GRID) && UI_NAVIGATION_GRID
    case meta_id::NavigationCell:
      apply_navigator_grid_cell_theme(obj);
      return true;
    case meta_id::NavigationIconArea:
      apply_navigator_grid_icon_area_theme(obj);
      return true;
    case meta_id::NavigationIcon:
      apply_navigator_grid_icon_theme(obj);
      return true;
    case meta_id::NavigationTitleBar:
      apply_navigator_grid_title_bar_theme(obj);
      return true;
    case meta_id::NavigationTitleLabel:
      apply_navigator_grid_title_label_theme(obj);
      return true;
#endif
    default:
      break;
  }
  return false;
}

static bool apply_navigation_ring_theme(_lv_obj_t* obj) {
#if !defined(UI_NAVIGATION_GRID) || !UI_NAVIGATION_GRID
  if (ht_id(obj) != meta_id::NavigationRing) return false;
  init_navigation_styles(obj);
  lv_obj_add_style(obj, &s_nav_ring_style, LV_PART_MAIN);
  lv_obj_add_style(obj, &s_nav_ring_style, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
  lv_obj_add_style(obj, &s_visibility_visible_style, LV_PART_MAIN);
  lv_obj_add_style(obj, &s_visibility_hidden_style, LV_PART_MAIN | LV_STATE_USER_4);
  return true;
#else
  (void)obj;
  return false;
#endif
}

#if defined(UI_NAVIGATION_GRID) && UI_NAVIGATION_GRID
static _lv_obj_t* find_direct_child_by_meta_id(_lv_obj_t* parent, MetaId id) {
  if (!parent) return nullptr;
  const uint32_t n = lv_obj_get_child_cnt(parent);
  for (uint32_t i = 0; i < n; ++i) {
    _lv_obj_t* child = lv_obj_get_child(parent, i);
    if (child && ht_id(child) == id) return child;
  }
  return nullptr;
}
#endif

void ui_navigator_apply_footer_cell_theme(_lv_obj_t* cell) {
#if defined(UI_NAVIGATION_GRID) && UI_NAVIGATION_GRID
  if (!cell) return;

  _lv_obj_t* bar = find_direct_child_by_meta_id(cell, meta_id::NavigationTitleBar);
  if (!bar) return;
  init_navigation_styles(cell);
  static const lv_style_selector_t sels[] = {
      LV_PART_MAIN,
      LV_PART_MAIN | LV_STATE_PRESSED,
  };
  for (lv_style_selector_t sel : sels) {
    lv_obj_add_style(bar, &s_nav_footer_bar_style, sel);
  }

  _lv_obj_t* lbl = find_direct_child_by_meta_id(bar, meta_id::NavigationTitleLabel);
  if (lbl) lv_obj_add_style(lbl, &s_nav_footer_label_style, LV_PART_MAIN);
#else
  (void)cell;
#endif
}

bool ui_widget_theme_apply(_lv_obj_t* obj) {
  if (!obj) return false;

  const auto apply_custom = [](lv_obj_t* target) {
    return apply_app_frame_theme(target) ||
           apply_top_pane_theme(target) ||
           apply_surface_root_theme(target) ||
           apply_screen_child_theme(target) ||
           apply_overlay_child_theme(target) ||
           apply_context_menu_child_theme(target) ||
           apply_navigation_theme(target) ||
           apply_navigation_ring_theme(target) ||
           ui_button_roller_apply_theme(target) ||
           ui_compass_widget_apply_theme(target) ||
           ui_map_widget_apply_theme(target);
  };

  return apply_custom(obj);
}

}  // namespace heltec::meshcore::ui
