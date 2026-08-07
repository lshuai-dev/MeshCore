#include "ui/navigation/ui_navigator.hpp"

#include "ui/app/ui_app_ids.hpp"
#include "ui/app/ui_theme.hpp"
#include "ui/core/ht_meta_data.hpp"
#include "ui/core/abstract_overlay.hpp"
#include "ui/core/screen_id.hpp"
#include "ui/menus/context_menu.hpp"
#include "ui/overlays/quick_ping_overlay.hpp"
#include "ui/overlays/alert_overlay.hpp"
#include "ui/overlays/calibration_overlay.hpp"
#include "ui/overlays/confirm_overlay.hpp"
#include "ui/overlays/keyboard_overlay.hpp"
#include "ui/overlays/preview_overlay.hpp"
#include "ui/overlays/radio_pram_sync_overlay.hpp"
#include "ui/overlays/repeat_mode_overlay.hpp"
#include "ui/overlays/send_message_overlay_ids.hpp"
#include "ui/overlays/splash_overlay.hpp"
#include "ui/screens/compass_dial_widget.hpp"
#include "ui/screens/find_friend_screen_ids.hpp"
#include "ui/screens/gps_screen.hpp"
#include "ui/screens/home_screen.hpp"
#include "ui/screens/radio_screen.hpp"
#include "ui/screens/recent_screen.hpp"
#include "ui/screens/system_screen.hpp"
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
static lv_style_t s_ctx_header_row_style;
static lv_style_t s_ctx_icon_row_layout_style;
static lv_style_t s_ctx_title_style;
static lv_style_t s_ctx_icon_btn_main_style;
static lv_style_t s_ctx_icon_btn_checked_style;
static lv_style_t s_ctx_icon_style;
static bool s_ctx_styles_ready = false;
#endif

static lv_style_t s_overlay_center_label_style;
static bool s_overlay_center_label_style_ready = false;
static lv_style_t s_send_list_style;
static lv_style_t s_send_touch_list_style;
static lv_style_t s_send_root_layout_style;
static lv_style_t s_send_row_main_style;
static lv_style_t s_send_row_active_style;
static lv_style_t s_send_row_label_active_style;
static bool s_send_styles_ready = false;

static lv_style_t s_radio_sync_list_style;
static lv_style_t s_radio_sync_root_layout_style;
static lv_style_t s_radio_sync_row_main_style;
static lv_style_t s_radio_sync_row_checked_style;
static lv_style_t s_overlay_roller_style;
static lv_style_t s_overlay_roller_selected_style;
static bool s_radio_sync_styles_ready = false;

static lv_style_t s_repeat_root_layout_style;
static lv_style_t s_repeat_list_style;
static lv_style_t s_repeat_item_style;
static lv_style_t s_repeat_item_selected_style;
static lv_style_t s_repeat_item_label_style;
static lv_style_t s_repeat_item_label_selected_style;
static bool s_repeat_styles_ready = false;

static lv_style_t s_keyboard_textarea_style;
static lv_style_t s_keyboard_root_layout_style;
static lv_style_t s_keyboard_root_waypoint_layout_style;
static lv_style_t s_keyboard_main_style;
static lv_style_t s_keyboard_waypoint_main_style;
static lv_style_t s_keyboard_items_base_style;
static lv_style_t s_keyboard_waypoint_items_style;
static lv_style_t s_keyboard_items_selected_style;
static bool s_keyboard_child_styles_ready = false;

static lv_style_t s_overlay_label_all_states_style;
static lv_style_t s_alert_box_style;
static lv_style_t s_calibration_panel_style;
static bool s_overlay_feedback_styles_ready = false;

static lv_style_t s_surface_root_common_style;
static lv_style_t s_screen_root_layout_style;
static lv_style_t s_home_root_layout_style;
static lv_style_t s_tracker_root_layout_style;
static lv_style_t s_preview_root_layout_style;
static lv_style_t s_splash_root_layout_style;
static lv_style_t s_plain_container_style;
static lv_style_t s_no_chrome_style;
static lv_style_t s_hidden_scrollbar_style;
static lv_style_t s_top_pane_slot_style;
static lv_style_t s_top_pane_right_slot_style;
static lv_style_t s_touch_reset_style;
static bool s_common_widget_styles_ready = false;

static lv_style_t s_top_pane_root_style;
static lv_style_t s_top_pane_battery_style;
static lv_style_t s_top_pane_title_style;
static lv_style_t s_top_pane_battery_outline_style;
static lv_style_t s_top_pane_battery_cap_style;
static lv_style_t s_top_pane_battery_fill_style;
static lv_style_t s_top_pane_battery_fill_low_style;
static lv_style_t s_top_pane_battery_fill_mid_style;
static lv_style_t s_top_pane_battery_fill_high_style;
static bool s_top_pane_styles_ready = false;
static UiTopPaneStyleConfig s_top_pane_style_config = {
    12, 4, 4, 4, 0, 14, 8, 4,
};

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
static UiAppFrameStyleConfig s_app_frame_style_config = {
    2, 0, 0, 0, 0, 2,
};
static UiContextMenuStyleConfig s_context_menu_style_config = {
    12, 1, 1, 1, 3, 1, 1,
};

#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
static lv_style_t s_quick_ping_root_style;
static lv_style_t s_quick_ping_backdrop_style;
static lv_style_t s_quick_ping_title_bar_style;
static lv_style_t s_quick_ping_content_style;
static lv_style_t s_quick_ping_title_style;
static lv_style_t s_quick_ping_row_style;
static lv_style_t s_quick_ping_row_focus_style;
static lv_style_t s_quick_ping_label_style;
static lv_style_t s_quick_ping_dropdown_style;
static lv_style_t s_quick_ping_message_dropdown_style;
static lv_style_t s_quick_ping_control_focus_style;
static lv_style_t s_quick_ping_dropdown_disabled_style;
static lv_style_t s_quick_ping_dropdown_indicator_style;
static lv_style_t s_quick_ping_message_input_style;
static lv_style_t s_quick_ping_message_input_label_style;
static lv_style_t s_quick_ping_message_input_label_editing_style;
static lv_style_t s_quick_ping_keyboard_style;
static lv_style_t s_quick_ping_keyboard_items_style;
static lv_style_t s_quick_ping_keyboard_items_selected_style;
static lv_style_t s_quick_ping_keyboard_editor_style;
static lv_style_t s_quick_ping_keyboard_placeholder_style;
static lv_style_t s_quick_ping_keyboard_counter_style;
static lv_style_t s_quick_ping_keyboard_cursor_style;
static lv_style_t s_quick_ping_message_list_style;
static lv_style_t s_quick_ping_icon_badge_style;
static lv_style_t s_quick_ping_plain_container_style;
static bool s_quick_ping_styles_ready = false;
#endif
static UiQuickPingStyleConfig s_quick_ping_style_config = {104};

static lv_style_t s_warning_text_style;
static lv_style_t s_accent_text_style;
static lv_style_t s_system_row_style;
static lv_style_t s_settings_row_layout_style;
static lv_style_t s_find_friend_row_layout_style;
static lv_style_t s_find_friend_dial_layout_style;
static lv_style_t s_recent_row_layout_style;
static lv_style_t s_screen_dropdown_list_layout_style;
static lv_style_t s_system_label_style;
static lv_style_t s_system_dropdown_style;
static lv_style_t s_system_dropdown_focus_style;
static lv_style_t s_system_dropdown_list_style;
static lv_style_t s_system_volume_controls_style;
static lv_style_t s_system_volume_button_style;
static lv_style_t s_system_volume_button_pressed_style;
static lv_style_t s_system_volume_button_focus_style;
static lv_style_t s_system_volume_slider_style;
static lv_style_t s_system_volume_slider_indicator_style;
static lv_style_t s_system_volume_slider_knob_style;
static lv_style_t s_confirm_overlay_backdrop_style;
static lv_style_t s_confirm_overlay_box_style;
static lv_style_t s_confirm_overlay_text_style;
static lv_style_t s_confirm_overlay_button_row_style;
static lv_style_t s_confirm_overlay_button_style;
static lv_style_t s_confirm_overlay_button_pressed_style;
static bool s_screen_system_styles_ready = false;

static lv_style_t s_nav_content_host_style;
static lv_style_t s_nav_panel_base_style;
static lv_style_t s_nav_root_base_style;
static lv_style_t s_visibility_visible_style;
static lv_style_t s_visibility_hidden_style;
static UiNavigationStyleConfig s_navigation_style_config = {
    2, 3, 6, 0, 18, 8, 36, 2,
};
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

static void init_overlay_center_label_style() {
  if (s_overlay_center_label_style_ready) return;
  lv_style_init(&s_overlay_center_label_style);
  lv_style_set_text_color(&s_overlay_center_label_style, ui_color_overlay_fg());
  lv_style_set_text_align(&s_overlay_center_label_style, LV_TEXT_ALIGN_CENTER);
  set_overlay_text_spacing(&s_overlay_center_label_style);
  s_overlay_center_label_style_ready = true;
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
  (void)obj;
  const UiContextMenuStyleConfig& config = s_context_menu_style_config;
  const lv_coord_t icon_pad_px = config.icon_pad;
  const lv_coord_t border_width_px = config.border_width;
  const lv_coord_t title_border_width_px = config.title_border_width;
  const lv_coord_t button_side = config.icon_size + 2 * icon_pad_px;

  lv_style_init(&s_ctx_menu_style);
  lv_style_set_border_width(&s_ctx_menu_style, 0);
  lv_style_set_outline_width(&s_ctx_menu_style, 0);
  lv_style_set_bg_opa(&s_ctx_menu_style, LV_OPA_COVER);
  lv_style_set_bg_color(&s_ctx_menu_style, ui_color_bg());
  lv_style_set_text_color(&s_ctx_menu_style, ui_color_fg());
  lv_style_set_pad_all(&s_ctx_menu_style, 0);
  lv_style_set_pad_row(&s_ctx_menu_style, reference_card_gap());

  lv_style_init(&s_ctx_header_style);
  lv_style_set_bg_opa(&s_ctx_header_style, LV_OPA_COVER);
  lv_style_set_bg_color(&s_ctx_header_style, ui_color_fg());
  lv_style_set_text_color(&s_ctx_header_style, ui_color_bg());
  lv_style_set_border_width(&s_ctx_header_style, 0);
  lv_style_set_outline_width(&s_ctx_header_style, 0);
  lv_style_set_pad_all(&s_ctx_header_style, 0);
  // lv_style_set_shadow_width(&s_ctx_header_style, 0);

  lv_style_init(&s_ctx_header_row_style);
  lv_style_set_bg_opa(&s_ctx_header_row_style, LV_OPA_TRANSP);
  lv_style_set_text_color(&s_ctx_header_row_style, ui_color_bg());
  lv_style_set_border_width(&s_ctx_header_row_style, 0);
  lv_style_set_outline_width(&s_ctx_header_row_style, 0);
  lv_style_set_shadow_width(&s_ctx_header_row_style, 0);
  lv_style_set_radius(&s_ctx_header_row_style, 0);
  lv_style_set_pad_all(&s_ctx_header_row_style, 0);

  lv_style_init(&s_ctx_icon_row_layout_style);
  lv_style_set_width(&s_ctx_icon_row_layout_style, lv_pct(100));
  lv_style_set_height(&s_ctx_icon_row_layout_style, button_side);
  lv_style_set_pad_all(&s_ctx_icon_row_layout_style, 0);
  lv_style_set_pad_column(&s_ctx_icon_row_layout_style, icon_pad_px);

  lv_style_init(&s_ctx_title_style);
  lv_style_set_text_align(&s_ctx_title_style, LV_TEXT_ALIGN_CENTER);
  set_overlay_text_spacing(&s_ctx_title_style);
  lv_style_set_bg_opa(&s_ctx_title_style, LV_OPA_COVER);
  lv_style_set_bg_color(&s_ctx_title_style, ui_color_fg());
  lv_style_set_text_color(&s_ctx_title_style, ui_color_bg());
  lv_style_set_outline_width(&s_ctx_title_style, 0);
  lv_style_set_shadow_width(&s_ctx_title_style, 0);
  lv_style_set_radius(&s_ctx_title_style, 0);
  lv_style_set_pad_all(&s_ctx_title_style, 0);
  if (title_border_width_px > 0) {
    lv_style_set_border_width(&s_ctx_title_style, title_border_width_px);
    lv_style_set_border_side(&s_ctx_title_style, LV_BORDER_SIDE_FULL);
    lv_style_set_border_color(&s_ctx_title_style, ui_color_bg());
    lv_style_set_border_opa(&s_ctx_title_style, LV_OPA_COVER);
  }

  lv_style_init(&s_ctx_icon_btn_main_style);
  lv_style_set_width(&s_ctx_icon_btn_main_style, button_side);
  lv_style_set_height(&s_ctx_icon_btn_main_style, button_side);
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

  lv_style_init(&s_ctx_icon_style);
  lv_style_set_width(&s_ctx_icon_style, config.icon_size);
  lv_style_set_height(&s_ctx_icon_style, config.icon_size);
  lv_style_set_img_recolor_opa(&s_ctx_icon_style, LV_OPA_TRANSP);

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

  init_overlay_center_label_style();

  lv_style_init(&s_send_root_layout_style);
#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
  lv_style_set_pad_hor(&s_send_root_layout_style, 2);
  lv_style_set_pad_ver(&s_send_root_layout_style, 1);
  lv_style_set_pad_row(&s_send_root_layout_style, 0);
#elif (defined(HELTEC_DISPLAY_ST7789) && HELTEC_DISPLAY_ST7789) || \
    (defined(HELTEC_DISPLAY_ST7735) && HELTEC_DISPLAY_ST7735) || \
    (defined(HELTEC_DISPLAY_SSD1306) && HELTEC_DISPLAY_SSD1306) || \
    LV_COLOR_DEPTH == 1
  lv_style_set_pad_hor(&s_send_root_layout_style, 2);
  lv_style_set_pad_ver(&s_send_root_layout_style, 1);
  lv_style_set_pad_row(&s_send_root_layout_style, 0);
#else
  lv_style_set_pad_hor(&s_send_root_layout_style, 6);
  lv_style_set_pad_ver(&s_send_root_layout_style, 4);
  lv_style_set_pad_row(&s_send_root_layout_style,
#if defined(HELTEC_V4_R8_TFT)
                       LV_DPX(10)
#else
                       3
#endif
  );
#endif

  lv_style_init(&s_send_list_style);
  lv_style_set_bg_opa(&s_send_list_style, LV_OPA_COVER);
  lv_style_set_bg_color(&s_send_list_style, ui_color_overlay_bg());
  lv_style_set_border_width(&s_send_list_style, 0);
  lv_style_set_pad_all(&s_send_list_style, 0);
  lv_style_set_pad_row(&s_send_list_style,
#if defined(HELTEC_V4_R8_TFT)
                       LV_DPX(10)
#else
                       3
#endif
  );

  lv_style_init(&s_send_touch_list_style);
  lv_style_set_bg_opa(&s_send_touch_list_style, LV_OPA_TRANSP);
  lv_style_set_border_width(&s_send_touch_list_style, 0);
  lv_style_set_pad_all(&s_send_touch_list_style, 0);
  lv_style_set_pad_row(&s_send_touch_list_style,
#if defined(HELTEC_V4_R8_TFT)
                       LV_DPX(10)
#else
                       3
#endif
  );

  lv_style_init(&s_send_row_main_style);
  lv_style_set_bg_opa(&s_send_row_main_style, LV_OPA_TRANSP);
  lv_style_set_radius(&s_send_row_main_style, row_radius);
  lv_style_set_shadow_width(&s_send_row_main_style, 0);
  lv_style_set_border_width(&s_send_row_main_style, 0);
  lv_style_set_outline_width(&s_send_row_main_style, 0);
  lv_style_set_text_color(&s_send_row_main_style, ui_color_overlay_fg());
  lv_style_set_text_align(&s_send_row_main_style, LV_TEXT_ALIGN_CENTER);
  lv_style_set_pad_all(&s_send_row_main_style, 0);
#if !defined(HELTEC_V4_R8_TFT) || !defined(HELTEC_HAS_TOUCH) || !HELTEC_HAS_TOUCH
#if defined(HELTEC_DISPLAY_ST7789) && HELTEC_DISPLAY_ST7789
  constexpr lv_coord_t send_row_height = 16;
#elif (defined(HELTEC_DISPLAY_ST7735) && HELTEC_DISPLAY_ST7735) || \
    (defined(HELTEC_DISPLAY_SSD1306) && HELTEC_DISPLAY_SSD1306) || \
    LV_COLOR_DEPTH == 1
  constexpr lv_coord_t send_row_height = 12;
#else
  constexpr lv_coord_t send_row_height = 28;
#endif
  const lv_coord_t send_font_height = lv_font_get_line_height(LV_FONT_DEFAULT);
  lv_style_set_pad_top(&s_send_row_main_style,
                       send_row_height > send_font_height
                           ? (send_row_height - send_font_height) / 2
                           : 0);
#endif

  lv_style_init(&s_send_row_active_style);
  lv_style_set_bg_opa(&s_send_row_active_style, LV_OPA_COVER);
  lv_style_set_bg_color(&s_send_row_active_style, ui_color_highlight_bg());
  lv_style_set_text_color(&s_send_row_active_style, ui_color_highlight_fg());
  lv_style_set_shadow_width(&s_send_row_active_style, 0);
  lv_style_set_border_width(&s_send_row_active_style, 0);
  lv_style_set_outline_width(&s_send_row_active_style, 0);

  lv_style_init(&s_send_row_label_active_style);
  lv_style_set_text_color(&s_send_row_label_active_style, ui_color_highlight_fg());

  s_send_styles_ready = true;
}

static void init_radio_sync_styles() {
  if (s_radio_sync_styles_ready) return;

  init_overlay_center_label_style();
  const lv_coord_t gap =
#if defined(HELTEC_V4_R8_TFT)
      LV_DPX(10);
#else
      3;
#endif

  lv_style_init(&s_radio_sync_root_layout_style);
  lv_style_set_pad_hor(&s_radio_sync_root_layout_style, 6);
  lv_style_set_pad_ver(&s_radio_sync_root_layout_style, 4);
  lv_style_set_pad_row(&s_radio_sync_root_layout_style, gap);

  lv_style_init(&s_radio_sync_list_style);
  lv_style_set_bg_opa(&s_radio_sync_list_style, LV_OPA_TRANSP);
  lv_style_set_border_width(&s_radio_sync_list_style, 0);
  lv_style_set_pad_all(&s_radio_sync_list_style, 0);
  lv_style_set_pad_row(&s_radio_sync_list_style, gap);

  lv_style_init(&s_radio_sync_row_main_style);
  lv_style_set_text_color(&s_radio_sync_row_main_style, ui_color_overlay_fg());
  lv_style_set_text_align(&s_radio_sync_row_main_style, LV_TEXT_ALIGN_CENTER);
  set_overlay_text_spacing(&s_radio_sync_row_main_style);
  lv_style_set_bg_opa(&s_radio_sync_row_main_style, LV_OPA_TRANSP);

  lv_style_init(&s_radio_sync_row_checked_style);
  lv_style_set_text_color(&s_radio_sync_row_checked_style, ui_color_highlight_fg());
  lv_style_set_bg_opa(&s_radio_sync_row_checked_style, LV_OPA_COVER);
  lv_style_set_bg_color(&s_radio_sync_row_checked_style, ui_color_highlight_bg());

  lv_style_init(&s_overlay_roller_style);
  lv_style_set_pad_all(&s_overlay_roller_style, 0);
  lv_style_set_bg_opa(&s_overlay_roller_style, LV_OPA_TRANSP);
  lv_style_set_border_width(&s_overlay_roller_style, 0);
  lv_style_set_outline_width(&s_overlay_roller_style, 0);
  lv_style_set_shadow_width(&s_overlay_roller_style, 0);
  lv_style_set_text_color(&s_overlay_roller_style, ui_color_overlay_fg());
  lv_style_set_text_align(&s_overlay_roller_style, LV_TEXT_ALIGN_CENTER);
  lv_style_set_text_line_space(&s_overlay_roller_style, LV_DPX(18));
  lv_style_set_anim_time(&s_overlay_roller_style, 220);

  lv_style_init(&s_overlay_roller_selected_style);
  lv_style_set_bg_color(&s_overlay_roller_selected_style,
                        ui_color_highlight_bg());
  lv_style_set_bg_opa(&s_overlay_roller_selected_style, LV_OPA_COVER);
  lv_style_set_text_color(&s_overlay_roller_selected_style,
                          ui_color_highlight_fg());
  lv_style_set_radius(&s_overlay_roller_selected_style, LV_DPX(4));

  s_radio_sync_styles_ready = true;
}

static void init_repeat_mode_styles() {
  if (s_repeat_styles_ready) return;
  init_radio_sync_styles();

  lv_style_init(&s_repeat_root_layout_style);
  lv_style_set_pad_hor(&s_repeat_root_layout_style, 6);
  lv_style_set_pad_ver(&s_repeat_root_layout_style, 4);
  lv_style_set_pad_row(&s_repeat_root_layout_style, LV_DPX(4));

  lv_style_init(&s_repeat_list_style);
  lv_style_set_pad_all(&s_repeat_list_style, 0);
  lv_style_set_bg_opa(&s_repeat_list_style, LV_OPA_TRANSP);
  lv_style_set_border_width(&s_repeat_list_style, 0);

  lv_style_init(&s_repeat_item_style);
  lv_style_set_bg_opa(&s_repeat_item_style, LV_OPA_TRANSP);
  lv_style_set_border_width(&s_repeat_item_style, 0);
  lv_style_set_text_color(&s_repeat_item_style, ui_color_overlay_fg());

  lv_style_init(&s_repeat_item_selected_style);
  lv_style_set_bg_color(&s_repeat_item_selected_style,
                        ui_color_highlight_bg());
  lv_style_set_bg_opa(&s_repeat_item_selected_style, LV_OPA_COVER);
  lv_style_set_text_color(&s_repeat_item_selected_style,
                          ui_color_highlight_fg());

  lv_style_init(&s_repeat_item_label_style);
  lv_style_set_text_color(&s_repeat_item_label_style, ui_color_overlay_fg());

  lv_style_init(&s_repeat_item_label_selected_style);
  lv_style_set_text_color(&s_repeat_item_label_selected_style,
                          ui_color_highlight_fg());

  s_repeat_styles_ready = true;
}

static void init_keyboard_child_styles() {
  if (s_keyboard_child_styles_ready) return;

  init_overlay_center_label_style();
  lv_disp_t* const disp = lv_disp_get_default();
  const bool compact = disp && lv_disp_get_ver_res(disp) <= 80;
  const lv_font_t* waypoint_font =
#if defined(LV_FONT_MONTSERRAT_12) && LV_FONT_MONTSERRAT_12
      &lv_font_montserrat_12;
#elif defined(LV_FONT_UNSCII_16) && LV_FONT_UNSCII_16
      &lv_font_unscii_16;
#else
      LV_FONT_DEFAULT;
#endif

  lv_style_init(&s_keyboard_root_layout_style);
  lv_style_set_flex_main_place(&s_keyboard_root_layout_style,
                               LV_FLEX_ALIGN_START);
  lv_style_set_flex_cross_place(&s_keyboard_root_layout_style,
                                LV_FLEX_ALIGN_CENTER);
  lv_style_set_flex_track_place(&s_keyboard_root_layout_style,
                                LV_FLEX_ALIGN_START);
  if (compact) {
    lv_style_set_pad_ver(&s_keyboard_root_layout_style, 1);
    lv_style_set_pad_row(&s_keyboard_root_layout_style, 1);
  }

  lv_style_init(&s_keyboard_root_waypoint_layout_style);
  lv_style_set_flex_main_place(&s_keyboard_root_waypoint_layout_style,
                               LV_FLEX_ALIGN_START);
  lv_style_set_flex_cross_place(&s_keyboard_root_waypoint_layout_style,
                                LV_FLEX_ALIGN_CENTER);
  lv_style_set_flex_track_place(&s_keyboard_root_waypoint_layout_style,
                                LV_FLEX_ALIGN_START);
  lv_style_set_pad_ver(&s_keyboard_root_waypoint_layout_style, compact ? 1 : 2);
  lv_style_set_pad_row(&s_keyboard_root_waypoint_layout_style, compact ? 1 : 2);

  lv_style_init(&s_keyboard_textarea_style);
  lv_style_set_text_color(&s_keyboard_textarea_style, ui_color_overlay_fg());
  lv_style_set_bg_opa(&s_keyboard_textarea_style, LV_OPA_TRANSP);
  lv_style_set_border_color(&s_keyboard_textarea_style, ui_color_overlay_fg());
  lv_style_set_border_width(&s_keyboard_textarea_style, 1);
  lv_style_set_text_align(&s_keyboard_textarea_style, LV_TEXT_ALIGN_LEFT);
  set_overlay_text_spacing(&s_keyboard_textarea_style);
  lv_style_set_pad_all(&s_keyboard_textarea_style, 1);
  lv_style_set_text_font(&s_keyboard_textarea_style, LV_FONT_DEFAULT);
  lv_style_set_height(&s_keyboard_textarea_style, compact ? 16 : 18);
  if (compact) lv_style_set_min_height(&s_keyboard_textarea_style, 16);

  lv_style_init(&s_keyboard_main_style);
  lv_style_set_pad_all(&s_keyboard_main_style, 0);
  lv_style_set_pad_row(&s_keyboard_main_style, 0);
  lv_style_set_pad_column(&s_keyboard_main_style, 1);
  lv_style_set_border_width(&s_keyboard_main_style, 0);
  lv_style_set_outline_width(&s_keyboard_main_style, 0);
  lv_style_set_shadow_width(&s_keyboard_main_style, 0);
  lv_style_set_flex_grow(&s_keyboard_main_style, compact ? 0 : 1);
  lv_style_set_width(&s_keyboard_main_style, lv_pct(100));
  lv_style_set_height(&s_keyboard_main_style,
                      compact ? 36 : LV_SIZE_CONTENT);
  lv_style_set_min_height(&s_keyboard_main_style, compact ? 36 : 40);

  lv_style_init(&s_keyboard_waypoint_main_style);
  lv_style_set_flex_grow(&s_keyboard_waypoint_main_style, 0);
  lv_style_set_width(&s_keyboard_waypoint_main_style, lv_pct(100));
  lv_style_set_height(&s_keyboard_waypoint_main_style, compact ? 36 : 40);
  lv_style_set_min_height(&s_keyboard_waypoint_main_style, compact ? 36 : 40);

  lv_style_init(&s_keyboard_items_base_style);
  lv_style_set_pad_all(&s_keyboard_items_base_style, compact ? 0 : 1);
  lv_style_set_border_width(&s_keyboard_items_base_style, 0);
  lv_style_set_outline_width(&s_keyboard_items_base_style, 0);
  lv_style_set_shadow_width(&s_keyboard_items_base_style, 0);
  lv_style_set_bg_opa(&s_keyboard_items_base_style, LV_OPA_TRANSP);
  lv_style_set_text_color(&s_keyboard_items_base_style, ui_color_overlay_fg());
  lv_style_set_text_font(&s_keyboard_items_base_style, LV_FONT_DEFAULT);

  lv_style_init(&s_keyboard_waypoint_items_style);
  lv_style_set_text_font(&s_keyboard_waypoint_items_style, waypoint_font);
  lv_style_set_pad_all(&s_keyboard_waypoint_items_style, compact ? 1 : 2);

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
  lv_style_set_pad_all(&s_calibration_panel_style, 8);
  lv_style_set_pad_row(&s_calibration_panel_style, reference_card_gap());

  s_overlay_feedback_styles_ready = true;
}

static void init_common_widget_styles() {
  if (s_common_widget_styles_ready) return;

  lv_style_init(&s_surface_root_common_style);
  lv_style_set_bg_opa(&s_surface_root_common_style, LV_OPA_TRANSP);
  lv_style_set_pad_all(&s_surface_root_common_style, 0);

  lv_style_init(&s_screen_root_layout_style);
  lv_style_set_pad_row(&s_screen_root_layout_style, reference_card_gap());

  lv_style_init(&s_home_root_layout_style);
  lv_style_set_pad_all(&s_home_root_layout_style, 4);

  lv_style_init(&s_tracker_root_layout_style);
  lv_style_set_pad_top(&s_tracker_root_layout_style, 1);
  lv_style_set_pad_row(&s_tracker_root_layout_style, 2);

  lv_style_init(&s_preview_root_layout_style);
  lv_style_set_pad_all(&s_preview_root_layout_style, 4);
  lv_style_set_pad_row(&s_preview_root_layout_style, reference_card_gap());

  lv_style_init(&s_splash_root_layout_style);
  lv_style_set_pad_all(&s_splash_root_layout_style, 8);
  lv_style_set_pad_row(&s_splash_root_layout_style, reference_card_gap());

  lv_style_init(&s_plain_container_style);
  lv_style_set_bg_opa(&s_plain_container_style, LV_OPA_TRANSP);
  lv_style_set_border_width(&s_plain_container_style, 0);
  lv_style_set_pad_all(&s_plain_container_style, 0);

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

  (void)obj;
  const UiTopPaneStyleConfig& geometry = s_top_pane_style_config;
  const lv_coord_t top_radius = geometry.radius;
  const lv_coord_t bat_w = geometry.battery_width;
  const lv_coord_t bat_h = geometry.battery_height;
  const lv_color_t bar_fg = ui_color_top_pane_fg();
#if defined(HELTEC_V4_R8_TFT)
  const lv_color_t battery_chrome = lv_color_white();
#else
  const lv_color_t battery_chrome = bar_fg;
#endif
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
  lv_style_set_height(&s_top_pane_root_style, geometry.height);
  lv_style_set_pad_left(&s_top_pane_root_style, geometry.pad_left);
  lv_style_set_pad_right(&s_top_pane_root_style, geometry.pad_right);
  lv_style_set_pad_top(&s_top_pane_root_style, geometry.pad_top);
  lv_style_set_pad_bottom(&s_top_pane_root_style, 0);
  lv_style_set_radius(&s_top_pane_root_style, top_radius);
  lv_style_set_clip_corner(&s_top_pane_root_style, top_radius > 0);

  lv_style_init(&s_top_pane_right_slot_style);
  lv_style_set_pad_right(&s_top_pane_right_slot_style,
                         geometry.battery_pad_right);

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
  lv_style_set_height(&s_top_pane_battery_fill_style, LV_MAX(0, bat_h - 2));
  lv_style_set_bg_opa(&s_top_pane_battery_fill_style, LV_OPA_TRANSP);

  lv_style_init(&s_top_pane_battery_fill_low_style);
  lv_style_set_bg_opa(&s_top_pane_battery_fill_low_style, LV_OPA_COVER);
  lv_style_set_bg_color(&s_top_pane_battery_fill_low_style,
#if LV_COLOR_DEPTH == 1
                         bar_fg
#else
                         ui_color_battery_low()
#endif
  );

  lv_style_init(&s_top_pane_battery_fill_mid_style);
  lv_style_set_bg_opa(&s_top_pane_battery_fill_mid_style, LV_OPA_COVER);
  lv_style_set_bg_color(&s_top_pane_battery_fill_mid_style,
#if LV_COLOR_DEPTH == 1
                         bar_fg
#else
                         ui_color_battery_mid()
#endif
  );

  lv_style_init(&s_top_pane_battery_fill_high_style);
  lv_style_set_bg_opa(&s_top_pane_battery_fill_high_style, LV_OPA_COVER);
  lv_style_set_bg_color(&s_top_pane_battery_fill_high_style,
#if LV_COLOR_DEPTH == 1
                         bar_fg
#else
                         ui_color_battery_high()
#endif
  );

  s_top_pane_styles_ready = true;
}

static void init_active_screen_style() {
  if (s_active_screen_style_ready) return;

  lv_style_init(&s_active_screen_style);
  lv_style_set_border_width(&s_active_screen_style, 0);
  lv_style_set_bg_color(&s_active_screen_style, ui_color_frame_bg());
  lv_style_set_bg_opa(&s_active_screen_style, ui_frame_bg_opa());
  lv_style_set_pad_all(&s_active_screen_style, 0);

  s_active_screen_style_ready = true;
}

static void init_surface_app_styles(lv_obj_t* obj = nullptr) {
  if (s_surface_app_styles_ready) return;

  (void)obj;
  const UiAppFrameStyleConfig& frame_config = s_app_frame_style_config;
  const lv_coord_t screen_pad = frame_config.screen_pad;
  const lv_coord_t content_radius = frame_config.content_radius;
#if !defined(HELTEC_V4_R8_TFT) || !defined(HELTEC_HAS_TOUCH) || !HELTEC_HAS_TOUCH
  const UiContextMenuStyleConfig& context_config = s_context_menu_style_config;
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
      context_config.border_width > 0 ? context_config.border_width : 1;

  lv_style_init(&s_context_menu_root_style);
  lv_style_set_bg_opa(&s_context_menu_root_style, LV_OPA_COVER);
  lv_style_set_bg_color(&s_context_menu_root_style, ui_color_bg());
  lv_style_set_text_color(&s_context_menu_root_style, ui_color_fg());
  lv_style_set_border_width(&s_context_menu_root_style, context_config.border_width);
  lv_style_set_border_color(&s_context_menu_root_style, ui_color_fg());
  lv_style_set_border_opa(&s_context_menu_root_style, LV_OPA_COVER);
  // lv_style_set_border_side(&s_context_menu_root_style, LV_BORDER_SIDE_FULL);
  lv_style_set_outline_width(&s_context_menu_root_style, context_border_w);
  lv_style_set_outline_color(&s_context_menu_root_style, ui_color_bg());
  lv_style_set_outline_opa(&s_context_menu_root_style, LV_OPA_COVER);
  lv_style_set_outline_pad(&s_context_menu_root_style, 1);
  lv_style_set_radius(&s_context_menu_root_style, context_config.border_radius);
  lv_style_set_pad_all(&s_context_menu_root_style, 0);
  lv_style_set_pad_row(&s_context_menu_root_style, reference_card_gap());
  const lv_coord_t display_w = lv_disp_get_hor_res(nullptr);
  const lv_coord_t display_h = lv_disp_get_ver_res(nullptr);
  const lv_coord_t frame_w = LV_MAX(1, display_w - 2 * context_config.frame_inset_x);
  const lv_coord_t frame_h = LV_MAX(1, display_h - 2 * context_config.frame_inset_y);
  lv_style_set_width(&s_context_menu_root_style, frame_w);
  lv_style_set_height(&s_context_menu_root_style, frame_h);
  lv_style_set_align(&s_context_menu_root_style, LV_ALIGN_TOP_LEFT);
  lv_style_set_x(&s_context_menu_root_style, context_config.frame_inset_x);
  lv_style_set_y(&s_context_menu_root_style, context_config.frame_inset_y);
#endif

  lv_style_init(&s_app_overlay_layer_style);
  lv_style_set_bg_opa(&s_app_overlay_layer_style, LV_OPA_TRANSP);
  lv_style_set_border_width(&s_app_overlay_layer_style, 0);
  lv_style_set_radius(&s_app_overlay_layer_style, 0);
  lv_style_set_pad_all(&s_app_overlay_layer_style, 0);

  lv_style_init(&s_app_frame_layout_style);
  lv_style_set_border_width(&s_app_frame_layout_style, 0);
  lv_style_set_bg_color(&s_app_frame_layout_style, ui_color_frame_bg());
  lv_style_set_bg_opa(&s_app_frame_layout_style, ui_frame_bg_opa());
  lv_style_set_pad_all(&s_app_frame_layout_style, 0);
  lv_style_set_pad_row(&s_app_frame_layout_style, 0);
  lv_style_set_pad_top(&s_app_frame_layout_style, frame_config.frame_margin_top);

  lv_style_init(&s_app_content_style);
  lv_style_set_bg_opa(&s_app_content_style, LV_OPA_TRANSP);
  lv_style_set_border_width(&s_app_content_style, 0);
  lv_style_set_pad_all(&s_app_content_style, 0);
  lv_style_set_pad_left(&s_app_content_style, frame_config.frame_margin_left);
  lv_style_set_pad_right(&s_app_content_style, frame_config.frame_margin_right);
  lv_style_set_pad_bottom(&s_app_content_style, frame_config.frame_margin_bottom);

  lv_style_init(&s_app_screen_root_style);
  lv_style_set_bg_opa(&s_app_screen_root_style, LV_OPA_TRANSP);
  lv_style_set_border_width(&s_app_screen_root_style, 0);
  lv_style_set_radius(&s_app_screen_root_style, 0);
  lv_style_set_clip_corner(&s_app_screen_root_style, false);
  lv_style_set_pad_all(&s_app_screen_root_style, 0);
#if !defined(HELTEC_V4_R8_TFT)
  if (screen_pad > 0 || content_radius > 0) {
    lv_style_set_bg_color(&s_app_screen_root_style, ui_color_panel_bg());
    lv_style_set_bg_opa(&s_app_screen_root_style, LV_OPA_COVER);
    lv_style_set_radius(&s_app_screen_root_style, content_radius > 0 ? content_radius : 0);
    lv_style_set_clip_corner(&s_app_screen_root_style, content_radius > 0);
  }
#endif
  lv_style_init(&s_app_tileview_style);
  lv_style_set_pad_all(&s_app_tileview_style, 0);
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
  lv_style_set_pad_all(&s_app_tile_style, screen_pad);

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
  lv_style_set_pad_all(&s_quick_ping_root_style, 0);

  lv_style_init(&s_quick_ping_backdrop_style);
  lv_style_set_bg_color(&s_quick_ping_backdrop_style,
                        lv_color_hex(0x001765));
  lv_style_set_bg_opa(&s_quick_ping_backdrop_style, LV_OPA_COVER);
  lv_style_set_border_width(&s_quick_ping_backdrop_style, 0);
  lv_style_set_pad_all(&s_quick_ping_backdrop_style, 0);

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
  lv_style_set_pad_all(&s_quick_ping_content_style, 5);
  lv_style_set_pad_row(&s_quick_ping_content_style, 6);

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
  lv_style_set_pad_all(&s_quick_ping_row_style, 0);

  lv_style_init(&s_quick_ping_row_focus_style);
  lv_style_set_bg_color(&s_quick_ping_row_focus_style, lv_color_hex(0xE2F2FF));
  lv_style_set_bg_opa(&s_quick_ping_row_focus_style, LV_OPA_COVER);
  lv_style_set_border_width(&s_quick_ping_row_focus_style, 1);
  lv_style_set_border_color(&s_quick_ping_row_focus_style, ui_color_accent());
  lv_style_set_border_opa(&s_quick_ping_row_focus_style, LV_OPA_COVER);
  lv_style_set_outline_width(&s_quick_ping_row_focus_style, 0);
  lv_style_set_shadow_width(&s_quick_ping_row_focus_style, 0);
  lv_style_set_radius(&s_quick_ping_row_focus_style, 6);

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
  lv_style_set_pad_right(&s_quick_ping_dropdown_style, 8);

  lv_style_init(&s_quick_ping_message_dropdown_style);
  lv_style_set_text_align(&s_quick_ping_message_dropdown_style, LV_TEXT_ALIGN_CENTER);
  lv_style_set_bg_color(&s_quick_ping_message_dropdown_style, lv_color_white());
  lv_style_set_bg_opa(&s_quick_ping_message_dropdown_style, LV_OPA_COVER);

  lv_style_init(&s_quick_ping_message_list_style);
  lv_style_set_min_width(&s_quick_ping_message_list_style,
                         s_quick_ping_style_config.message_list_min_width);

  lv_style_init(&s_quick_ping_control_focus_style);
  lv_style_set_bg_color(&s_quick_ping_control_focus_style, lv_color_hex(0xBDE1FF));
  lv_style_set_bg_opa(&s_quick_ping_control_focus_style, LV_OPA_COVER);
  lv_style_set_border_width(&s_quick_ping_control_focus_style, 0);
  lv_style_set_border_opa(&s_quick_ping_control_focus_style, LV_OPA_TRANSP);
  lv_style_set_outline_width(&s_quick_ping_control_focus_style, 0);
  lv_style_set_shadow_width(&s_quick_ping_control_focus_style, 0);
  lv_style_set_text_color(&s_quick_ping_control_focus_style, ui_color_fg());

  lv_style_init(&s_quick_ping_dropdown_disabled_style);
  lv_style_set_text_color(&s_quick_ping_dropdown_disabled_style, ui_color_fg());
  lv_style_set_opa(&s_quick_ping_dropdown_disabled_style, ui_effective_opa(LV_OPA_50));

  lv_style_init(&s_quick_ping_dropdown_indicator_style);
  lv_style_set_text_color(&s_quick_ping_dropdown_indicator_style, ui_color_fg());

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

  lv_style_init(&s_quick_ping_message_input_label_style);
  lv_style_set_min_width(&s_quick_ping_message_input_label_style, 0);

  lv_style_init(&s_quick_ping_message_input_label_editing_style);
  lv_style_set_min_width(&s_quick_ping_message_input_label_editing_style,
                         lv_pct(100));

  lv_style_init(&s_quick_ping_keyboard_style);
  lv_style_set_pad_left(&s_quick_ping_keyboard_style, 4);
  lv_style_set_pad_right(&s_quick_ping_keyboard_style, 4);
  lv_style_set_pad_top(&s_quick_ping_keyboard_style, 48);
  lv_style_set_pad_bottom(&s_quick_ping_keyboard_style, 4);
  lv_style_set_pad_row(&s_quick_ping_keyboard_style, 3);
  lv_style_set_pad_column(&s_quick_ping_keyboard_style, 2);
  lv_style_set_bg_color(&s_quick_ping_keyboard_style, lv_color_white());
  lv_style_set_bg_opa(&s_quick_ping_keyboard_style, LV_OPA_COVER);
  lv_style_set_border_width(&s_quick_ping_keyboard_style, 0);
  lv_style_set_border_opa(&s_quick_ping_keyboard_style, LV_OPA_TRANSP);
  lv_style_set_outline_width(&s_quick_ping_keyboard_style, 0);
  lv_style_set_shadow_width(&s_quick_ping_keyboard_style, 0);

  lv_style_init(&s_quick_ping_keyboard_items_style);
  lv_style_set_pad_all(&s_quick_ping_keyboard_items_style, 0);
  lv_style_set_bg_color(&s_quick_ping_keyboard_items_style,
                        lv_color_hex(0xF6F8FA));
  lv_style_set_bg_opa(&s_quick_ping_keyboard_items_style, LV_OPA_COVER);
  lv_style_set_border_width(&s_quick_ping_keyboard_items_style, 1);
  lv_style_set_border_color(&s_quick_ping_keyboard_items_style,
                            lv_color_hex(0xCBD5DF));
  lv_style_set_border_opa(&s_quick_ping_keyboard_items_style, LV_OPA_COVER);
  lv_style_set_radius(&s_quick_ping_keyboard_items_style, 4);
  lv_style_set_outline_width(&s_quick_ping_keyboard_items_style, 0);
  lv_style_set_shadow_width(&s_quick_ping_keyboard_items_style, 0);
  lv_style_set_text_color(&s_quick_ping_keyboard_items_style, ui_color_fg());
#if defined(LV_FONT_MONTSERRAT_14) && LV_FONT_MONTSERRAT_14
  lv_style_set_text_font(&s_quick_ping_keyboard_items_style,
                         &lv_font_montserrat_14);
#endif

  lv_style_init(&s_quick_ping_keyboard_items_selected_style);
  lv_style_set_pad_all(&s_quick_ping_keyboard_items_selected_style, 0);
  lv_style_set_bg_color(&s_quick_ping_keyboard_items_selected_style,
                        lv_color_hex(0xBDE1FF));
  lv_style_set_bg_opa(&s_quick_ping_keyboard_items_selected_style, LV_OPA_COVER);
  lv_style_set_border_width(&s_quick_ping_keyboard_items_selected_style, 1);
  lv_style_set_border_color(&s_quick_ping_keyboard_items_selected_style,
                            lv_color_hex(0x9BBFD8));
  lv_style_set_border_opa(&s_quick_ping_keyboard_items_selected_style,
                          LV_OPA_COVER);
  lv_style_set_radius(&s_quick_ping_keyboard_items_selected_style, 4);
  lv_style_set_outline_width(&s_quick_ping_keyboard_items_selected_style, 0);
  lv_style_set_shadow_width(&s_quick_ping_keyboard_items_selected_style, 0);
  lv_style_set_text_color(&s_quick_ping_keyboard_items_selected_style,
                          ui_color_fg());

  lv_style_init(&s_quick_ping_keyboard_editor_style);
  lv_style_set_bg_color(&s_quick_ping_keyboard_editor_style,
                        lv_color_white());
  lv_style_set_bg_opa(&s_quick_ping_keyboard_editor_style, LV_OPA_COVER);
  lv_style_set_border_width(&s_quick_ping_keyboard_editor_style, 1);
  lv_style_set_border_color(&s_quick_ping_keyboard_editor_style,
                            lv_color_hex(0x9BBFD8));
  lv_style_set_border_opa(&s_quick_ping_keyboard_editor_style, LV_OPA_COVER);
  lv_style_set_radius(&s_quick_ping_keyboard_editor_style, 5);
  lv_style_set_outline_width(&s_quick_ping_keyboard_editor_style, 0);
  lv_style_set_shadow_width(&s_quick_ping_keyboard_editor_style, 0);
  lv_style_set_pad_left(&s_quick_ping_keyboard_editor_style, 7);
  lv_style_set_pad_right(&s_quick_ping_keyboard_editor_style, 48);
  lv_style_set_text_color(&s_quick_ping_keyboard_editor_style, ui_color_fg());
#if defined(LV_FONT_MONTSERRAT_14) && LV_FONT_MONTSERRAT_14
  lv_style_set_text_font(&s_quick_ping_keyboard_editor_style,
                         &lv_font_montserrat_14);
#endif
  set_overlay_text_spacing(&s_quick_ping_keyboard_editor_style);

  lv_style_init(&s_quick_ping_keyboard_placeholder_style);
  lv_style_set_text_color(&s_quick_ping_keyboard_placeholder_style,
                          lv_color_hex(0xA8B0B8));
  lv_style_set_text_opa(&s_quick_ping_keyboard_placeholder_style,
                        LV_OPA_COVER);

  lv_style_init(&s_quick_ping_keyboard_counter_style);
  lv_style_set_bg_opa(&s_quick_ping_keyboard_counter_style, LV_OPA_TRANSP);
  lv_style_set_border_width(&s_quick_ping_keyboard_counter_style, 0);
  lv_style_set_text_color(&s_quick_ping_keyboard_counter_style,
                          lv_color_hex(0x7F8992));
  lv_style_set_text_align(&s_quick_ping_keyboard_counter_style,
                          LV_TEXT_ALIGN_RIGHT);
#if defined(LV_FONT_MONTSERRAT_12) && LV_FONT_MONTSERRAT_12
  lv_style_set_text_font(&s_quick_ping_keyboard_counter_style,
                         &lv_font_montserrat_12);
#endif
  set_overlay_text_spacing(&s_quick_ping_keyboard_counter_style);

  lv_style_init(&s_quick_ping_keyboard_cursor_style);
  lv_style_set_border_color(&s_quick_ping_keyboard_cursor_style, ui_color_fg());
  lv_style_set_border_width(&s_quick_ping_keyboard_cursor_style, 2);
  lv_style_set_border_side(&s_quick_ping_keyboard_cursor_style,
                           LV_BORDER_SIDE_LEFT);
  lv_style_set_pad_left(&s_quick_ping_keyboard_cursor_style, -1);
  lv_style_set_anim_time(&s_quick_ping_keyboard_cursor_style, 400);

  lv_style_init(&s_quick_ping_icon_badge_style);
  lv_style_set_bg_color(&s_quick_ping_icon_badge_style,
                        lv_color_hex(0xBDE1FF));
  lv_style_set_bg_opa(&s_quick_ping_icon_badge_style, LV_OPA_COVER);
  lv_style_set_radius(&s_quick_ping_icon_badge_style, 8);
  lv_style_set_border_width(&s_quick_ping_icon_badge_style, 0);
  lv_style_set_shadow_width(&s_quick_ping_icon_badge_style, 0);
  lv_style_set_pad_all(&s_quick_ping_icon_badge_style, 0);

  lv_style_init(&s_quick_ping_plain_container_style);
  lv_style_set_bg_opa(&s_quick_ping_plain_container_style, LV_OPA_TRANSP);
  lv_style_set_border_width(&s_quick_ping_plain_container_style, 0);
  lv_style_set_outline_width(&s_quick_ping_plain_container_style, 0);
  lv_style_set_shadow_width(&s_quick_ping_plain_container_style, 0);
  lv_style_set_pad_all(&s_quick_ping_plain_container_style, 0);

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

  lv_style_init(&s_system_row_style);
  lv_style_set_bg_opa(&s_system_row_style, LV_OPA_TRANSP);
  lv_style_set_border_width(&s_system_row_style, 0);
  lv_style_set_border_opa(&s_system_row_style, LV_OPA_TRANSP);
  lv_style_set_border_side(&s_system_row_style, LV_BORDER_SIDE_NONE);
  lv_style_set_outline_width(&s_system_row_style, 0);
  lv_style_set_outline_opa(&s_system_row_style, LV_OPA_TRANSP);
  lv_style_set_shadow_width(&s_system_row_style, 0);
  lv_style_set_shadow_opa(&s_system_row_style, LV_OPA_TRANSP);
#if LV_COLOR_DEPTH == 1
  lv_style_set_pad_all(&s_system_row_style, 0);
  lv_style_set_pad_row(&s_system_row_style, 0);
  lv_style_set_pad_column(&s_system_row_style, 2);
#elif defined(HELTEC_V4_R8_TFT)
  lv_style_set_pad_hor(&s_system_row_style, ui_settings_row_pad_hor());
  lv_style_set_pad_ver(&s_system_row_style, ui_settings_row_pad_ver());
  lv_style_set_pad_row(&s_system_row_style, LV_DPX(10));
  lv_style_set_pad_column(&s_system_row_style, LV_DPX(10));
#else
  lv_style_set_pad_all(&s_system_row_style, 2);
#endif

  lv_style_init(&s_settings_row_layout_style);
  lv_style_set_pad_all(&s_settings_row_layout_style, 0);
  lv_style_set_pad_hor(&s_settings_row_layout_style,
                       ui_settings_row_pad_hor());
  lv_style_set_pad_ver(&s_settings_row_layout_style,
                       ui_settings_row_pad_ver());

  lv_style_init(&s_find_friend_row_layout_style);
#if defined(HELTEC_V4_R8_TFT)
  lv_style_set_pad_hor(&s_find_friend_row_layout_style,
                       ui_settings_row_pad_hor());
  lv_style_set_pad_ver(&s_find_friend_row_layout_style, 1);
#elif defined(HELTEC_T1)
  lv_style_set_pad_all(&s_find_friend_row_layout_style, 0);
#else
  lv_style_set_pad_all(&s_find_friend_row_layout_style,
                       LV_COLOR_DEPTH == 1 ? 0 : 2);
#endif
  lv_style_set_pad_column(&s_find_friend_row_layout_style,
#if defined(HELTEC_T1)
                          2
#else
                          LV_DPX(4)
#endif
  );

  lv_style_init(&s_find_friend_dial_layout_style);
  lv_style_set_min_height(&s_find_friend_dial_layout_style,
#if defined(HELTEC_T1)
                          0
#else
                          LV_COLOR_DEPTH == 1 ? LV_DPX(32) : LV_DPX(96)
#endif
  );

  lv_style_init(&s_recent_row_layout_style);
  lv_style_set_pad_all(&s_recent_row_layout_style, 0);
  lv_style_set_pad_column(&s_recent_row_layout_style, 4);

  lv_style_init(&s_screen_dropdown_list_layout_style);
#if defined(HELTEC_V4_R8_TFT)
  lv_style_set_pad_top(&s_screen_dropdown_list_layout_style, 2);
  lv_style_set_pad_bottom(&s_screen_dropdown_list_layout_style, 2);
#endif

  lv_style_init(&s_system_label_style);
  lv_style_set_bg_opa(&s_system_label_style, LV_OPA_TRANSP);
  lv_style_set_text_color(&s_system_label_style, ui_color_fg());
  lv_style_set_text_align(&s_system_label_style, LV_TEXT_ALIGN_LEFT);
  lv_style_set_border_width(&s_system_label_style, 0);
  lv_style_set_border_opa(&s_system_label_style, LV_OPA_TRANSP);
  lv_style_set_border_side(&s_system_label_style, LV_BORDER_SIDE_NONE);
  lv_style_set_outline_width(&s_system_label_style, 0);
  lv_style_set_outline_opa(&s_system_label_style, LV_OPA_TRANSP);
  lv_style_set_shadow_width(&s_system_label_style, 0);
  lv_style_set_shadow_opa(&s_system_label_style, LV_OPA_TRANSP);

  lv_style_init(&s_system_dropdown_style);
#if LV_COLOR_DEPTH == 1
#if defined(HELTEC_LORA_V4_OLED)
  // LVGL 8 anchors the closed dropdown value at border + pad_top. The V4
  // OLED's 12 px control, 8 px font and 1 px border leave 2 px to split
  // vertically, matching the flex-centered label beside it.
  lv_style_set_pad_top(&s_system_dropdown_style, 1);
  lv_style_set_pad_bottom(&s_system_dropdown_style, 1);
#else
  lv_style_set_pad_top(&s_system_dropdown_style, 0);
  lv_style_set_pad_bottom(&s_system_dropdown_style, 0);
#endif
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

  lv_style_init(&s_system_dropdown_list_style);
#if defined(HELTEC_V4_R8_TFT)
  lv_style_set_pad_top(&s_system_dropdown_list_style, 2);
  lv_style_set_pad_bottom(&s_system_dropdown_list_style, 2);
#endif

  lv_style_init(&s_system_volume_controls_style);
  lv_style_set_pad_all(&s_system_volume_controls_style, 0);
  lv_style_set_pad_column(&s_system_volume_controls_style, 4);
  lv_style_set_bg_opa(&s_system_volume_controls_style, LV_OPA_TRANSP);
  lv_style_set_border_width(&s_system_volume_controls_style, 0);

  lv_style_init(&s_system_volume_button_style);
  lv_style_set_pad_all(&s_system_volume_button_style, 0);
  lv_style_set_bg_opa(&s_system_volume_button_style, LV_OPA_TRANSP);
  lv_style_set_border_width(&s_system_volume_button_style, 1);
  lv_style_set_border_color(&s_system_volume_button_style,
                            ui_color_panel_border());
  lv_style_set_border_opa(&s_system_volume_button_style, LV_OPA_TRANSP);
  lv_style_set_text_color(&s_system_volume_button_style, ui_color_fg());

  lv_style_init(&s_system_volume_button_pressed_style);
  lv_style_set_bg_color(&s_system_volume_button_pressed_style,
                        ui_color_highlight_bg());
  lv_style_set_bg_opa(&s_system_volume_button_pressed_style, LV_OPA_COVER);
  lv_style_set_text_color(&s_system_volume_button_pressed_style,
                          ui_color_highlight_fg());

  lv_style_init(&s_system_volume_button_focus_style);
  lv_style_set_bg_opa(&s_system_volume_button_focus_style, LV_OPA_TRANSP);
  lv_style_set_border_width(&s_system_volume_button_focus_style, 1);
  lv_style_set_border_color(&s_system_volume_button_focus_style,
                            lv_color_white());
  lv_style_set_border_opa(&s_system_volume_button_focus_style, LV_OPA_COVER);
  lv_style_set_text_color(&s_system_volume_button_focus_style, ui_color_fg());

  lv_style_init(&s_system_volume_slider_style);
  lv_style_set_bg_color(&s_system_volume_slider_style, ui_color_switch_bg());
  lv_style_set_bg_opa(&s_system_volume_slider_style, LV_OPA_COVER);
  lv_style_set_border_width(&s_system_volume_slider_style, 1);
  lv_style_set_border_color(&s_system_volume_slider_style,
                            ui_color_panel_border());
  lv_style_set_radius(&s_system_volume_slider_style, LV_RADIUS_CIRCLE);

  lv_style_init(&s_system_volume_slider_indicator_style);
  lv_style_set_bg_color(&s_system_volume_slider_indicator_style,
                        ui_color_accent());
  lv_style_set_bg_opa(&s_system_volume_slider_indicator_style, LV_OPA_COVER);
  lv_style_set_radius(&s_system_volume_slider_indicator_style,
                      LV_RADIUS_CIRCLE);

  lv_style_init(&s_system_volume_slider_knob_style);
  lv_style_set_bg_color(&s_system_volume_slider_knob_style, ui_color_fg());
  lv_style_set_bg_opa(&s_system_volume_slider_knob_style, LV_OPA_COVER);
  lv_style_set_border_width(&s_system_volume_slider_knob_style, 1);
  lv_style_set_border_color(&s_system_volume_slider_knob_style,
                            ui_color_panel_border());
  lv_style_set_pad_hor(&s_system_volume_slider_knob_style, 0);
  lv_style_set_pad_ver(&s_system_volume_slider_knob_style, 3);
  lv_style_set_transform_width(&s_system_volume_slider_knob_style, -1);

  lv_disp_t* const system_disp = lv_disp_get_default();
  const bool compact_dialog =
      system_disp && (lv_disp_get_hor_res(system_disp) <= 160 ||
                      lv_disp_get_ver_res(system_disp) <= 128);

  lv_style_init(&s_confirm_overlay_backdrop_style);
  lv_style_set_bg_color(&s_confirm_overlay_backdrop_style, lv_color_black());
  lv_style_set_bg_opa(&s_confirm_overlay_backdrop_style, LV_OPA_50);
  lv_style_set_border_width(&s_confirm_overlay_backdrop_style, 0);
  lv_style_set_pad_all(&s_confirm_overlay_backdrop_style, 0);

  lv_style_init(&s_confirm_overlay_box_style);
  lv_style_set_bg_color(&s_confirm_overlay_box_style, ui_color_overlay_bg());
  lv_style_set_bg_opa(&s_confirm_overlay_box_style, LV_OPA_COVER);
  lv_style_set_border_color(&s_confirm_overlay_box_style,
                             ui_color_overlay_fg());
  lv_style_set_border_width(&s_confirm_overlay_box_style, 1);
  lv_style_set_radius(&s_confirm_overlay_box_style,
                       compact_dialog ? 2 : LV_DPX(8));
  lv_style_set_pad_all(&s_confirm_overlay_box_style,
                        compact_dialog ? 4 : LV_DPX(12));
  lv_style_set_pad_row(&s_confirm_overlay_box_style,
                        compact_dialog ? 4 : LV_DPX(12));
  lv_style_set_text_color(&s_confirm_overlay_box_style,
                           ui_color_overlay_fg());

  lv_style_init(&s_confirm_overlay_text_style);
  lv_style_set_text_align(&s_confirm_overlay_text_style, LV_TEXT_ALIGN_CENTER);
  lv_style_set_text_color(&s_confirm_overlay_text_style,
                           ui_color_overlay_fg());

  lv_style_init(&s_confirm_overlay_button_row_style);
  lv_style_set_bg_opa(&s_confirm_overlay_button_row_style, LV_OPA_TRANSP);
  lv_style_set_border_width(&s_confirm_overlay_button_row_style, 0);
  lv_style_set_pad_all(&s_confirm_overlay_button_row_style, 0);
  lv_style_set_pad_column(&s_confirm_overlay_button_row_style,
                           compact_dialog ? 4 : LV_DPX(10));

  lv_style_init(&s_confirm_overlay_button_style);
  lv_style_set_bg_color(&s_confirm_overlay_button_style, ui_color_panel_bg());
  lv_style_set_bg_opa(&s_confirm_overlay_button_style, LV_OPA_COVER);
  lv_style_set_border_color(&s_confirm_overlay_button_style,
                             ui_color_panel_border());
  lv_style_set_border_width(&s_confirm_overlay_button_style, 1);
  lv_style_set_radius(&s_confirm_overlay_button_style,
                       compact_dialog ? 1 : LV_DPX(5));
  lv_style_set_pad_all(&s_confirm_overlay_button_style, 0);
  lv_style_set_text_color(&s_confirm_overlay_button_style,
                           ui_color_overlay_fg());

  lv_style_init(&s_confirm_overlay_button_pressed_style);
  lv_style_set_bg_color(&s_confirm_overlay_button_pressed_style,
                         ui_color_highlight_bg());
  lv_style_set_text_color(&s_confirm_overlay_button_pressed_style,
                           ui_color_highlight_fg());

  s_screen_system_styles_ready = true;
}

static void init_navigation_styles(lv_obj_t* obj = nullptr) {
  if (s_navigation_styles_ready) return;
  (void)obj;
  const UiNavigationStyleConfig& config = s_navigation_style_config;
  const lv_coord_t panel_radius = s_app_frame_style_config.content_radius;

  lv_style_init(&s_nav_content_host_style);
  lv_style_set_bg_opa(&s_nav_content_host_style, LV_OPA_TRANSP);
  lv_style_set_border_width(&s_nav_content_host_style, 0);

#if defined(UI_NAVIGATION_GRID) && UI_NAVIGATION_GRID
  lv_style_set_pad_all(&s_nav_content_host_style, config.grid_pad);
  lv_style_set_pad_column(&s_nav_content_host_style, config.grid_gap);
  lv_style_set_pad_row(&s_nav_content_host_style, config.grid_gap);

  const lv_coord_t nav_radius = config.grid_cell_radius;
  const lv_coord_t icon_radius = panel_radius;
  const lv_coord_t title_radius = panel_radius;
  lv_style_init(&s_nav_grid_cell_style);
  lv_style_set_pad_all(&s_nav_grid_cell_style, 0);
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
  lv_style_set_pad_all(&s_nav_grid_icon_area_style,
#if defined(HELTEC_V4_R8_TFT)
                       0
#else
                       4
#endif
  );

  lv_style_init(&s_nav_grid_icon_style);
  lv_style_set_radius(&s_nav_grid_icon_style, icon_radius);
  lv_style_set_clip_corner(&s_nav_grid_icon_style, icon_radius > 0);

  lv_style_init(&s_nav_grid_title_bar_style);
  lv_style_set_width(&s_nav_grid_title_bar_style, lv_pct(100));
  lv_style_set_height(&s_nav_grid_title_bar_style, config.grid_label_height);
  lv_style_set_pad_hor(&s_nav_grid_title_bar_style, 2);
  lv_style_set_pad_ver(&s_nav_grid_title_bar_style, 0);
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
  lv_style_set_pad_all(&s_nav_root_base_style, 0);

#if defined(UI_NAVIGATION_GRID) && UI_NAVIGATION_GRID
  lv_style_init(&s_nav_root_grid_style);
  lv_style_set_bg_opa(&s_nav_root_grid_style, LV_OPA_TRANSP);

  lv_style_init(&s_nav_footer_bar_style);
  lv_style_set_height(&s_nav_footer_bar_style, lv_pct(100));
  lv_style_set_flex_grow(&s_nav_footer_bar_style, 1);
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
  lv_style_set_pad_all(&s_nav_ring_style, config.ring_edge_pad);
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
    case meta_id::ConfirmOverlayRoot:
    case meta_id::KeyboardOverlayRoot:
    case meta_id::RadioParamSyncOverlayRoot:
    case meta_id::RepeatModeOverlayRoot:
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
      style_top_pane_slot(obj);
      return true;

    case meta_id::TopPaneRightSlot:
      style_top_pane_slot(obj);
      init_top_pane_styles(parent);
      lv_obj_add_style(obj, &s_top_pane_right_slot_style, LV_PART_MAIN);
      return true;

    case meta_id::TopPaneCenterSlot:
      style_top_pane_slot(obj);
      return true;

    default:
      return false;
  }
}

static bool apply_top_pane_nested_child(_lv_obj_t* obj) {
  switch (ht_id(obj)) {
    case meta_id::TopPaneBattery:
      init_top_pane_styles(obj);
      lv_obj_add_style(obj, &s_top_pane_battery_style, LV_PART_MAIN);
      return true;

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
      lv_obj_add_style(obj, &s_top_pane_battery_fill_low_style,
                       LV_PART_MAIN | LV_STATE_USER_1);
      lv_obj_add_style(obj, &s_top_pane_battery_fill_mid_style,
                       LV_PART_MAIN | LV_STATE_USER_2);
      lv_obj_add_style(obj, &s_top_pane_battery_fill_high_style,
                       LV_PART_MAIN | LV_STATE_USER_3);
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
    init_common_widget_styles();
    lv_obj_add_style(obj, &s_screen_root_layout_style, LV_PART_MAIN);
    if (id == meta_id::HomeScreenRoot) {
      lv_obj_add_style(obj, &s_home_root_layout_style, LV_PART_MAIN);
    } else if (id == meta_id::TrackerScreenRoot) {
      lv_obj_add_style(obj, &s_tracker_root_layout_style, LV_PART_MAIN);
    }
    apply_no_chrome(obj);
    hide_scrollbar_chrome(obj);
    return true;
  }
  if (is_overlay_root_id(id)) {
    apply_surface_root_common(obj);
    switch (id) {
      case meta_id::PreviewOverlayRoot:
        apply_overlay_root_chrome(obj);
        init_common_widget_styles();
        lv_obj_add_style(obj, &s_preview_root_layout_style, LV_PART_MAIN);
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
      case meta_id::ConfirmOverlayRoot:
        init_screen_system_styles();
        lv_obj_add_style(obj, &s_confirm_overlay_backdrop_style, LV_PART_MAIN);
        break;
      case meta_id::KeyboardOverlayRoot:
        apply_overlay_root_chrome(obj);
        init_keyboard_child_styles();
        lv_obj_add_style(obj, &s_keyboard_root_layout_style, LV_PART_MAIN);
        lv_obj_add_style(obj, &s_keyboard_root_waypoint_layout_style,
                         LV_PART_MAIN | LV_STATE_USER_1);
        break;
      case meta_id::RadioParamSyncOverlayRoot:
        apply_overlay_root_chrome(obj);
        init_radio_sync_styles();
        lv_obj_add_style(obj, &s_radio_sync_root_layout_style, LV_PART_MAIN);
        break;
      case meta_id::RepeatModeOverlayRoot:
        apply_overlay_root_chrome(obj);
        init_repeat_mode_styles();
        lv_obj_add_style(obj, &s_repeat_root_layout_style, LV_PART_MAIN);
        break;
      case meta_id::SendMessageOverlayRoot:
        apply_overlay_root_chrome(obj);
        init_send_message_styles();
        lv_obj_add_style(obj, &s_send_root_layout_style, LV_PART_MAIN);
        break;
      case meta_id::SplashOverlayRoot: {
        apply_overlay_root_chrome(obj);
        init_common_widget_styles();
        lv_obj_add_style(obj, &s_splash_root_layout_style, LV_PART_MAIN);
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
        lv_obj_add_style(obj, &s_ctx_header_row_style, selector);
      }
      lv_obj_add_style(obj, &s_ctx_icon_row_layout_style, LV_PART_MAIN);
      return true;
    }

    case meta_id::ContextMenuHeaderNavRow:
      init_classic_context_styles(obj);
      for (lv_style_selector_t selector : kContextMenuRootSelectors) {
        lv_obj_add_style(obj, &s_ctx_header_row_style, selector);
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
      lv_obj_add_style(obj, &s_ctx_icon_style, LV_PART_MAIN);
      lv_obj_add_style(obj, &s_ctx_icon_style, LV_PART_MAIN | LV_STATE_CHECKED);
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

    case meta_id::QuickPingBackdrop:
      init_quick_ping_styles();
      reset_touch_object(obj);
      lv_obj_add_style(obj, &s_quick_ping_backdrop_style, LV_PART_MAIN);
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
    case meta_id::QuickPingMessageControls:
      init_quick_ping_styles();
      reset_touch_object(obj);
      lv_obj_add_style(obj, &s_quick_ping_row_style, LV_PART_MAIN);
      lv_obj_add_style(obj, &s_quick_ping_row_focus_style,
                       LV_PART_MAIN | LV_STATE_FOCUSED);
      lv_obj_add_style(obj, &s_quick_ping_row_focus_style,
                       LV_PART_MAIN | LV_STATE_FOCUS_KEY);
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
      lv_obj_add_style(obj, &s_quick_ping_control_focus_style,
                       LV_PART_MAIN | LV_STATE_FOCUSED);
      lv_obj_add_style(obj, &s_quick_ping_dropdown_disabled_style,
                       LV_PART_MAIN | LV_STATE_DISABLED);
      lv_obj_add_style(obj, &s_quick_ping_dropdown_indicator_style, LV_PART_INDICATOR);
      lv_obj_add_style(obj, &s_quick_ping_dropdown_indicator_style,
                       LV_PART_INDICATOR | LV_STATE_FOCUSED);
      return true;

    case meta_id::QuickPingMessageInput:
      init_quick_ping_styles();
      reset_touch_object(obj);
      lv_obj_add_style(obj, &s_quick_ping_message_input_style, LV_PART_MAIN);
      lv_obj_add_style(obj, &s_quick_ping_control_focus_style,
                       LV_PART_MAIN | LV_STATE_FOCUSED);
      lv_obj_add_style(obj, &s_quick_ping_control_focus_style,
                       LV_PART_MAIN | LV_STATE_PRESSED);
      lv_obj_add_style(obj, &s_quick_ping_dropdown_disabled_style,
                       LV_PART_MAIN | LV_STATE_DISABLED);
      return true;

    case meta_id::QuickPingMessageInputLabel:
      init_quick_ping_styles();
      reset_touch_object(obj);
      lv_obj_add_style(obj, &s_quick_ping_label_style, LV_PART_MAIN);
      lv_obj_add_style(obj, &s_quick_ping_label_style,
                       LV_PART_MAIN | LV_STATE_FOCUSED);
      lv_obj_add_style(obj, &s_quick_ping_message_input_label_style,
                       LV_PART_MAIN);
      lv_obj_add_style(obj, &s_quick_ping_message_input_label_editing_style,
                       LV_PART_MAIN | LV_STATE_USER_1);
      return true;

    case meta_id::QuickPingIconBadge:
      init_quick_ping_styles();
      reset_touch_object(obj);
      lv_obj_add_style(obj, &s_quick_ping_icon_badge_style, LV_PART_MAIN);
      return true;

    case meta_id::QuickPingMessageRow:
    case meta_id::QuickPingMessageHeader:
      init_quick_ping_styles();
      reset_touch_object(obj);
      lv_obj_add_style(obj, &s_quick_ping_plain_container_style,
                       LV_PART_MAIN);
      return true;

    case meta_id::QuickPingMessageDropdownList:
      init_quick_ping_styles();
      lv_obj_add_style(obj, &s_quick_ping_message_list_style, LV_PART_MAIN);
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
          LV_PART_ITEMS | LV_STATE_PRESSED,
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

    case meta_id::QuickPingKeyboardEditor:
      init_quick_ping_styles();
      reset_touch_object(obj);
      lv_obj_add_style(obj, &s_quick_ping_keyboard_editor_style,
                       LV_PART_MAIN);
      lv_obj_add_style(obj, &s_quick_ping_keyboard_placeholder_style,
                       LV_PART_TEXTAREA_PLACEHOLDER);
      lv_obj_add_style(obj, &s_quick_ping_keyboard_cursor_style,
                       LV_PART_CURSOR);
      return true;

    case meta_id::QuickPingKeyboardCounter:
      init_quick_ping_styles();
      reset_touch_object(obj);
      lv_obj_add_style(obj, &s_quick_ping_keyboard_counter_style,
                       LV_PART_MAIN);
      return true;

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
    case meta_id::GpsLocationShareRow:
    case meta_id::GpsAdvIntervalRow:
    case meta_id::GpsTrackRow:
      style_plain_container(obj);
      init_screen_system_styles();
      lv_obj_add_style(obj, &s_settings_row_layout_style, LV_PART_MAIN);
      return true;

    case meta_id::GpsFixLabel:
    case meta_id::GpsSatLabel:
    case meta_id::GpsLatLonLabel:
    case meta_id::GpsAltLabel:
    case meta_id::GpsSpeedLabel:
    case meta_id::GpsRawLabel:
      style_screen_label(obj);
      if (ht_id(obj) == meta_id::GpsRawLabel) {
        init_screen_system_styles();
        lv_obj_add_style(obj, &s_warning_text_style, LV_PART_MAIN);
      }
      return true;

    case meta_id::GpsPowerPrefix:
    case meta_id::GpsLocationShareLabel:
    case meta_id::GpsAdvIntervalLabel:
    case meta_id::GpsTrackLabel:
      style_screen_label(obj);
      return true;

    case meta_id::GpsPowerSwitch:
    case meta_id::GpsLocationShareSwitch:
    case meta_id::GpsTrackSwitch:
      // The active UI theme owns all switch-part styling.
      return true;

    case meta_id::GpsAdvIntervalDropdown:
      init_screen_system_styles();
      apply_system_control_no_chrome(obj);
      lv_obj_add_style(obj, &s_system_dropdown_style, LV_PART_MAIN);
      lv_obj_add_style(obj, &s_system_dropdown_focus_style,
                       LV_PART_MAIN | LV_STATE_FOCUS_KEY);
      lv_obj_add_style(obj, &s_system_dropdown_focus_style,
                       LV_PART_MAIN | LV_STATE_FOCUSED);
      hide_scrollbar_chrome(obj);
      return true;

    default:
      return false;
  }
}

static bool apply_recent_screen_child_theme(_lv_obj_t* obj) {
  switch (ht_id(obj)) {
    case meta_id::RecentRow:
      style_plain_container(obj);
      init_screen_system_styles();
      lv_obj_add_style(obj, &s_recent_row_layout_style, LV_PART_MAIN);
      return true;
    case meta_id::RecentName:
      style_screen_label(obj);
      return true;
    case meta_id::RecentAge:
      style_screen_label(obj, LV_TEXT_ALIGN_RIGHT);
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
      init_screen_system_styles();
      lv_obj_add_style(obj, &s_settings_row_layout_style, LV_PART_MAIN);
      return true;
    case meta_id::RadioLnaSwitch:
      return true;
    default:
      return false;
  }
}

static bool apply_find_friend_screen_child_theme(_lv_obj_t* obj) {
  switch (ht_id(obj)) {
    case meta_id::FindFriendDialRow:
      style_plain_container(obj);
      init_screen_system_styles();
      lv_obj_add_style(obj, &s_find_friend_dial_layout_style, LV_PART_MAIN);
      return true;

    case meta_id::FindFriendSettingRow:
    case meta_id::FindFriendActionRow:
      style_plain_container(obj);
      init_screen_system_styles();
      lv_obj_add_style(obj, &s_find_friend_row_layout_style, LV_PART_MAIN);
      return true;

    case meta_id::FindFriendSettingLabel:
    case meta_id::FindFriendActionLabel:
      style_screen_label(obj);
      return true;

    case meta_id::FindFriendSwitch:
      return true;

    case meta_id::FindFriendDropdown:
      init_screen_system_styles();
      apply_system_control_no_chrome(obj);
      lv_obj_add_style(obj, &s_system_dropdown_style, LV_PART_MAIN);
      lv_obj_add_style(obj, &s_system_dropdown_focus_style,
                       LV_PART_MAIN | LV_STATE_FOCUS_KEY);
      lv_obj_add_style(obj, &s_system_dropdown_focus_style,
                       LV_PART_MAIN | LV_STATE_FOCUSED);
      hide_scrollbar_chrome(obj);
      return true;

    default:
      return false;
  }
}

static void style_system_row_common(_lv_obj_t* row) {
  if (!row) return;
  init_screen_system_styles();
  lv_obj_add_style(row, &s_system_row_style, LV_PART_MAIN);
}

static bool apply_system_screen_child_theme(_lv_obj_t* obj) {
  if (!obj) return false;

  switch (ht_id(obj)) {
    case meta_id::SystemActionRow:
      style_system_row_common(obj);
      return true;

    case meta_id::SystemSwitchRow:
      style_system_row_common(obj);
      return true;

    case meta_id::SystemDropdownRow:
      style_system_row_common(obj);
      return true;

    case meta_id::SystemActionLabel:
      init_screen_system_styles();
      lv_obj_add_style(obj, &s_system_label_style, LV_PART_MAIN);
      return true;

    case meta_id::SystemSwitchLabel:
      init_screen_system_styles();
      lv_obj_add_style(obj, &s_system_label_style, LV_PART_MAIN);
      return true;

    case meta_id::SystemDropdownLabel:
      init_screen_system_styles();
      lv_obj_add_style(obj, &s_system_label_style, LV_PART_MAIN);
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

    case meta_id::FindFriendDropdownList:
      init_screen_system_styles();
      ui_theme_apply_dropdown_list(obj);
      lv_obj_add_style(obj, &s_screen_dropdown_list_layout_style,
                       LV_PART_MAIN);
      lv_obj_add_style(obj, &s_screen_dropdown_list_layout_style,
                       LV_PART_SELECTED);
      return true;

    case meta_id::GpsDropdownList:
      init_screen_system_styles();
      ui_theme_apply_dropdown_list(obj);
      lv_obj_add_style(obj, &s_screen_dropdown_list_layout_style,
                       LV_PART_MAIN);
      lv_obj_add_style(obj, &s_screen_dropdown_list_layout_style,
                       LV_PART_SELECTED);
      return true;

    case meta_id::SystemDropdownList:
      init_screen_system_styles();
      ui_theme_apply_dropdown_list(obj);
      lv_obj_add_style(obj, &s_system_dropdown_list_style, LV_PART_MAIN);
      lv_obj_add_style(obj, &s_system_dropdown_list_style, LV_PART_SELECTED);
      return true;

    case meta_id::SystemSwitch:
      init_screen_system_styles();
      // The active UI theme fully styles each switch part. Applying the shared
      // no-chrome style here also adds transparent FOCUSED/CHECKED selectors,
      // which are more specific than the switch's base part styles and can
      // make System switches render black/invisible across board themes.
      return true;

#if defined(HAS_BUZZER_VOLUME_CONTROL) && HAS_BUZZER_VOLUME_CONTROL
    case meta_id::SystemVolumeRow:
      style_system_row_common(obj);
      return true;

    case meta_id::SystemVolumeLabel:
    case meta_id::SystemVolumeButtonLabel:
      init_screen_system_styles();
      lv_obj_add_style(obj, &s_system_label_style, LV_PART_MAIN);
      return true;

    case meta_id::SystemVolumeControls:
      init_screen_system_styles();
      lv_obj_add_style(obj, &s_system_volume_controls_style, LV_PART_MAIN);
      return true;

    case meta_id::SystemVolumeButton:
      init_screen_system_styles();
      lv_obj_add_style(obj, &s_system_volume_button_style, LV_PART_MAIN);
      lv_obj_add_style(obj, &s_system_volume_button_pressed_style,
                       LV_PART_MAIN | LV_STATE_PRESSED);
      lv_obj_add_style(obj, &s_system_volume_button_focus_style,
                       LV_PART_MAIN | LV_STATE_FOCUSED);
      lv_obj_add_style(obj, &s_system_volume_button_focus_style,
                       LV_PART_MAIN | LV_STATE_FOCUS_KEY);
      return true;

    case meta_id::SystemVolumeSlider:
      init_screen_system_styles();
      lv_obj_add_style(obj, &s_system_volume_slider_style, LV_PART_MAIN);
      lv_obj_add_style(obj, &s_system_volume_slider_indicator_style,
                       LV_PART_INDICATOR);
      lv_obj_add_style(obj, &s_system_volume_slider_knob_style, LV_PART_KNOB);
      return true;
#endif

    default:
      break;
  }
  return false;
}

static bool apply_confirm_overlay_child_theme(_lv_obj_t* obj) {
  if (!obj) return false;
  switch (ht_id(obj)) {
    case meta_id::ConfirmBox:
      init_screen_system_styles();
      lv_obj_add_style(obj, &s_confirm_overlay_box_style, LV_PART_MAIN);
      return true;

    case meta_id::ConfirmTitle:
    case meta_id::ConfirmBody:
    case meta_id::ConfirmButtonLabel:
    case meta_id::ConfirmKeyHint:
      init_screen_system_styles();
      lv_obj_add_style(obj, &s_confirm_overlay_text_style, LV_PART_MAIN);
      return true;

    case meta_id::ConfirmButtonRow:
      init_screen_system_styles();
      lv_obj_add_style(obj, &s_confirm_overlay_button_row_style, LV_PART_MAIN);
      return true;

    case meta_id::ConfirmButton:
      init_screen_system_styles();
      lv_obj_add_style(obj, &s_confirm_overlay_button_style, LV_PART_MAIN);
      lv_obj_add_style(obj, &s_confirm_overlay_button_pressed_style,
                       LV_PART_MAIN | LV_STATE_PRESSED);
      return true;

    default:
      return false;
  }
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
         apply_find_friend_screen_child_theme(obj) ||
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
      lv_obj_add_style(obj, &s_overlay_center_label_style, LV_PART_MAIN);
      return true;

    case meta_id::KeyboardTextarea:
      init_keyboard_child_styles();
      lv_obj_add_style(obj, &s_keyboard_textarea_style, LV_PART_MAIN);
      return true;

    case meta_id::KeyboardKeyboard: {
      init_keyboard_child_styles();
      lv_obj_add_style(obj, &s_keyboard_main_style, LV_PART_MAIN);
      lv_obj_add_style(obj, &s_keyboard_waypoint_main_style,
                       LV_PART_MAIN | LV_STATE_USER_1);
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
      lv_obj_add_style(obj, &s_keyboard_waypoint_items_style,
                       LV_PART_ITEMS | LV_STATE_USER_1);
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
      init_radio_sync_styles();
      lv_obj_add_style(obj, &s_overlay_center_label_style, LV_PART_MAIN);
      return true;

    case meta_id::RadioParamSyncList:
      init_radio_sync_styles();
      lv_obj_add_style(obj, &s_radio_sync_list_style, LV_PART_MAIN);
      return true;

    case meta_id::RadioParamSyncRow:
      init_radio_sync_styles();
      lv_obj_add_style(obj, &s_radio_sync_row_main_style, LV_PART_MAIN);
      lv_obj_add_style(obj, &s_radio_sync_row_checked_style,
                       LV_PART_MAIN | LV_STATE_CHECKED);
      return true;

    case meta_id::KeyboardSpacer:
      style_plain_container(obj);
      return true;

    case meta_id::RadioParamSyncRoller:
      init_radio_sync_styles();
      for (lv_style_selector_t selector : kInteractiveSelectors) {
        lv_obj_add_style(obj, &s_overlay_roller_style, selector);
      }
      lv_obj_add_style(obj, &s_overlay_roller_selected_style,
                       LV_PART_SELECTED);
      return true;

    default:
      return false;
  }
}

static bool apply_repeat_mode_overlay_child_theme(_lv_obj_t* obj) {
  static const lv_state_t selected_states[] = {
      LV_STATE_CHECKED,
      LV_STATE_FOCUSED,
      LV_STATE_FOCUS_KEY,
      LV_STATE_PRESSED,
      static_cast<lv_state_t>(LV_STATE_CHECKED | LV_STATE_FOCUSED),
      static_cast<lv_state_t>(LV_STATE_CHECKED | LV_STATE_FOCUS_KEY),
  };

  switch (ht_id(obj)) {
    case meta_id::RepeatModeTitle:
      init_repeat_mode_styles();
      lv_obj_add_style(obj, &s_overlay_center_label_style, LV_PART_MAIN);
      return true;

    case meta_id::RepeatModeList:
      init_repeat_mode_styles();
      lv_obj_add_style(obj, &s_repeat_list_style, LV_PART_MAIN);
      return true;

    case meta_id::RepeatModeRoller:
      init_repeat_mode_styles();
      for (lv_style_selector_t selector : kInteractiveSelectors) {
        lv_obj_add_style(obj, &s_overlay_roller_style, selector);
      }
      lv_obj_add_style(obj, &s_overlay_roller_selected_style,
                       LV_PART_SELECTED);
      return true;

    case meta_id::RepeatModeItem:
      init_repeat_mode_styles();
      lv_obj_add_style(obj, &s_repeat_item_style, LV_PART_MAIN);
      for (lv_state_t state : selected_states) {
        lv_obj_add_style(obj, &s_repeat_item_selected_style,
                         LV_PART_MAIN | state);
      }
      return true;

    case meta_id::RepeatModeItemLabel:
      init_repeat_mode_styles();
      lv_obj_add_style(obj, &s_repeat_item_label_style, LV_PART_MAIN);
      for (lv_state_t state : selected_states) {
        lv_obj_add_style(obj, &s_repeat_item_label_selected_style,
                         LV_PART_MAIN | state);
      }
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
      lv_obj_add_style(obj, &s_overlay_center_label_style, LV_PART_MAIN);
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
      lv_obj_add_style(obj, &s_overlay_center_label_style, LV_PART_MAIN);
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
         apply_confirm_overlay_child_theme(obj) ||
         apply_keyboard_overlay_child_theme(obj) ||
         apply_preview_overlay_child_theme(obj) ||
         apply_radio_param_sync_overlay_child_theme(obj) ||
         apply_repeat_mode_overlay_child_theme(obj) ||
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

void ui_theme_set_radio_sync_list_margin(_lv_obj_t* obj, lv_coord_t margin) {
  if (!obj) return;
  lv_obj_set_style_pad_top(obj, margin, LV_PART_MAIN);
  lv_obj_set_style_pad_bottom(obj, margin, LV_PART_MAIN);
}

void ui_theme_center_single_line_textarea(_lv_obj_t* textarea) {
  if (!textarea) return;
  lv_obj_update_layout(textarea);
  const lv_coord_t height = lv_obj_get_height(textarea);
  const lv_coord_t border =
      lv_obj_get_style_border_width(textarea, LV_PART_MAIN);
  const lv_font_t* const font =
      lv_obj_get_style_text_font(textarea, LV_PART_MAIN);
  const lv_coord_t font_height = font ? lv_font_get_line_height(font) : 0;
  const lv_coord_t free_height =
      height > font_height + border * 2
          ? height - font_height - border * 2
          : 0;
  const lv_coord_t pad_top = free_height / 2;
  lv_obj_set_style_pad_top(textarea, pad_top, LV_PART_MAIN);
  lv_obj_set_style_pad_bottom(textarea, free_height - pad_top, LV_PART_MAIN);
}

#if defined(UI_NAVIGATION_GRID) && UI_NAVIGATION_GRID
static _lv_obj_t* find_direct_child_by_meta_id(_lv_obj_t* parent, MetaId id);
#endif

void ui_theme_layout_navigation_grid(_lv_obj_t* host,
                                     _lv_obj_t* footer_cell) {
#if defined(UI_NAVIGATION_GRID) && UI_NAVIGATION_GRID
  if (!host) return;
  init_navigation_styles(host);
  const UiNavigationStyleConfig& config = s_navigation_style_config;
  const uint8_t cols = LV_MAX(1, config.grid_cols);
  const uint8_t rows = LV_MAX(1, config.grid_rows);
  const lv_coord_t gap = config.grid_gap;
  const lv_coord_t pad = config.grid_pad;
  const lv_coord_t width = lv_obj_get_width(host);
  const lv_coord_t height = lv_obj_get_height(host);
  if (width < 8 || height < 8) return;

  const bool has_footer = footer_cell != nullptr;
  const lv_coord_t footer_height =
      has_footer ? config.grid_footer_height : 0;
  const lv_coord_t footer_gap = has_footer ? gap : 0;
  const lv_coord_t available_width = width - pad * 2;
  const lv_coord_t available_height = height - pad * 2;
  const lv_coord_t footer_block = footer_height + footer_gap;
  const lv_coord_t grid_height_limit = available_height - footer_block;
  const lv_coord_t cell_width = LV_MAX(
      2, (available_width - gap * (static_cast<lv_coord_t>(cols) - 1)) /
             cols);
  const lv_coord_t cell_height = LV_MAX(
      2, (grid_height_limit -
          gap * (static_cast<lv_coord_t>(rows) - 1)) /
             rows);
  const lv_coord_t grid_width =
      cols * cell_width + gap * (static_cast<lv_coord_t>(cols) - 1);
  const lv_coord_t grid_height =
      rows * cell_height + gap * (static_cast<lv_coord_t>(rows) - 1);
  const lv_coord_t block_height = grid_height + footer_block;
  const lv_coord_t origin_x =
      pad + (available_width - grid_width) / 2;
  const lv_coord_t origin_y =
      pad + (available_height - block_height) / 2;

  uint8_t grid_index = 0;
  const uint32_t count = lv_obj_get_child_cnt(host);
  for (uint32_t i = 0; i < count; ++i) {
    _lv_obj_t* cell = lv_obj_get_child(host, i);
    if (!cell) continue;
    if (cell == footer_cell) {
      lv_obj_set_size(cell, available_width, footer_height);
      lv_obj_set_pos(cell, pad,
                     origin_y + grid_height + footer_gap);
      continue;
    }
    const uint8_t column = grid_index % cols;
    const uint8_t row = grid_index / cols;
    ++grid_index;
    lv_obj_set_size(cell, cell_width, cell_height);
    lv_obj_set_pos(cell, origin_x + column * (cell_width + gap),
                   origin_y + row * (cell_height + gap));
  }
#else
  (void)host;
  (void)footer_cell;
#endif
}

void ui_theme_sync_navigation_radius(_lv_obj_t* pane,
                                     _lv_obj_t* reference) {
#if defined(UI_NAVIGATION_GRID) && UI_NAVIGATION_GRID
  if (!pane || !reference || !lv_obj_is_valid(reference)) return;
  const lv_coord_t radius =
      lv_obj_get_style_radius(reference, LV_PART_MAIN);
  lv_obj_set_style_radius(pane, radius, LV_PART_MAIN);
  lv_obj_set_style_clip_corner(pane, radius > 0, LV_PART_MAIN);

  _lv_obj_t* host = ui_navigator_content(pane);
  if (!host) return;
  const uint32_t count = lv_obj_get_child_cnt(host);
  for (uint32_t i = 0; i < count; ++i) {
    _lv_obj_t* cell = lv_obj_get_child(host, i);
    if (!cell) continue;
    _lv_obj_t* icon_area =
        find_direct_child_by_meta_id(cell, meta_id::NavigationIconArea);
    _lv_obj_t* icon = find_direct_child_by_meta_id(
        icon_area, meta_id::NavigationIcon);
    if (icon) {
      lv_obj_set_style_radius(icon, radius, LV_PART_MAIN);
      lv_obj_set_style_clip_corner(icon, radius > 0, LV_PART_MAIN);
    }
    _lv_obj_t* bar =
        find_direct_child_by_meta_id(cell, meta_id::NavigationTitleBar);
    if (bar) {
      lv_obj_set_style_radius(bar, radius, LV_PART_MAIN);
      lv_obj_set_style_clip_corner(bar, radius > 0, LV_PART_MAIN);
      lv_obj_set_style_radius(bar, radius,
                              LV_PART_MAIN | LV_STATE_PRESSED);
      lv_obj_set_style_clip_corner(bar, radius > 0,
                                   LV_PART_MAIN | LV_STATE_PRESSED);
    }
  }
#else
  (void)pane;
  (void)reference;
#endif
}

void ui_widget_theme_set_app_frame_style(
    const UiAppFrameStyleConfig& config) {
  if (s_surface_app_styles_ready) return;
  s_app_frame_style_config = config;
}

void ui_widget_theme_set_context_menu_style(
    const UiContextMenuStyleConfig& config) {
  if (s_surface_app_styles_ready) return;
#if !defined(HELTEC_V4_R8_TFT) || !defined(HELTEC_HAS_TOUCH) || !HELTEC_HAS_TOUCH
  if (s_ctx_styles_ready) return;
#endif
  s_context_menu_style_config = config;
}

void ui_widget_theme_set_quick_ping_style(
    const UiQuickPingStyleConfig& config) {
#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
  if (s_quick_ping_styles_ready) return;
#endif
  s_quick_ping_style_config = config;
}

void ui_widget_theme_set_navigation_style(
    const UiNavigationStyleConfig& config) {
  if (s_navigation_styles_ready) return;
  s_navigation_style_config = config;
}

void ui_widget_theme_set_top_pane_style(
    const UiTopPaneStyleConfig& config) {
  if (s_top_pane_styles_ready) return;
  s_top_pane_style_config = config;
}

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
           ui_license_gate_apply_theme(target) ||
           ui_button_roller_apply_theme(target) ||
           ui_compass_widget_apply_theme(target) ||
           ui_map_widget_apply_theme(target);
  };

  return apply_custom(obj);
}

}  // namespace heltec::meshcore::ui
