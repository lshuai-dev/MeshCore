#pragma once

#include <lvgl.h>

namespace heltec::meshcore::ui {
extern const lv_obj_class_t ht_dialog_overlay_class;
extern const lv_obj_class_t ht_dialog_panel_class;
extern const lv_obj_class_t ht_dialog_content_class;
extern const lv_obj_class_t ht_dialog_actions_class;

extern const lv_obj_class_t ht_framed_overlay_content_class;
#if !defined(HELTEC_UI_HAS_TOUCH) || !HELTEC_UI_HAS_TOUCH
extern const lv_obj_class_t ht_send_message_content_class;
#endif
extern const lv_obj_class_t ht_overlay_header_class;
extern const lv_obj_class_t ht_overlay_footer_class;

extern const lv_obj_class_t ht_volume_controls_class;
extern const lv_obj_class_t ht_volume_button_class;

#if LV_USE_KEYBOARD
extern const lv_obj_class_t ht_keyboard_overlay_class;
extern const lv_obj_class_t ht_keyboard_title_class;
extern const lv_obj_class_t ht_keyboard_editor_class;
extern const lv_obj_class_t ht_keyboard_matrix_class;
extern const lv_obj_class_t ht_coordinate_row_class;
extern const lv_obj_class_t ht_coordinate_label_class;
#endif

extern const lv_obj_class_t ht_top_pane_class;
extern const lv_obj_class_t ht_top_pane_right_slot_class;
extern const lv_obj_class_t ht_top_pane_title_class;
extern const lv_obj_class_t ht_top_pane_battery_class;
extern const lv_obj_class_t ht_top_pane_battery_outline_class;
extern const lv_obj_class_t ht_top_pane_battery_cap_class;

extern const lv_obj_class_t ht_button_roller_class;
extern const lv_obj_class_t ht_button_roller_item_class;
extern const lv_obj_class_t ht_button_roller_label_class;

extern const lv_obj_class_t ht_license_gate_class;
extern const lv_obj_class_t ht_license_gate_label_class;
extern const lv_obj_class_t ht_license_gate_chip_class;

#if !defined(HELTEC_UI_HAS_TOUCH) || !HELTEC_UI_HAS_TOUCH
extern const lv_obj_class_t ht_context_menu_class;
extern const lv_obj_class_t ht_context_menu_icon_row_class;
extern const lv_obj_class_t ht_context_menu_icon_button_class;
extern const lv_obj_class_t ht_context_menu_icon_class;
extern const lv_obj_class_t ht_context_menu_title_class;
extern const lv_obj_class_t ht_context_menu_nav_row_class;
#endif

extern const lv_obj_class_t ht_cascading_menu_roller_class;
extern const lv_obj_class_t ht_cascading_menu_item_class;
extern const lv_obj_class_t ht_cascading_menu_label_class;
#if LV_USE_MENU
extern const lv_obj_class_t ht_cascading_menu_page_class;
#endif

extern const lv_obj_class_t ht_app_background_class;
extern const lv_obj_class_t ht_app_overlay_layer_class;
extern const lv_obj_class_t ht_app_frame_class;
extern const lv_obj_class_t ht_app_content_class;
extern const lv_obj_class_t ht_app_screen_host_class;

extern const lv_obj_class_t ht_navigation_root_class;
extern const lv_obj_class_t ht_navigation_content_class;
#if HELTEC_UI_HAS_TOUCH
extern const lv_obj_class_t ht_navigation_cell_class;
extern const lv_obj_class_t ht_navigation_icon_area_class;
extern const lv_obj_class_t ht_navigation_title_bar_class;
extern const lv_obj_class_t ht_navigation_grid_icon_class;
extern const lv_obj_class_t ht_navigation_title_label_class;
#endif
extern const lv_obj_class_t ht_navigation_ring_class;
#if !HELTEC_UI_HAS_TOUCH
extern const lv_obj_class_t ht_radial_navigation_icon_class;
#endif

extern const lv_obj_class_t ht_compass_plain_class;
extern const lv_obj_class_t ht_compass_hub_class;
extern const lv_obj_class_t ht_compass_needle_class;
extern const lv_obj_class_t ht_compass_label_class;
extern const lv_obj_class_t ht_find_friend_dial_host_class;
extern const lv_obj_class_t ht_screen_info_label_class;

#if defined(ENV_INCLUDE_MAP) && ENV_INCLUDE_MAP
extern const lv_obj_class_t ht_map_status_label_class;
extern const lv_obj_class_t ht_map_viewport_class;
extern const lv_obj_class_t ht_map_toolbar_class;
extern const lv_obj_class_t ht_map_toolbar_button_class;
extern const lv_obj_class_t ht_map_toolbar_label_class;
extern const lv_obj_class_t ht_map_toolbar_dropdown_class;
#if defined(HELTEC_UI_HAS_TOUCH) && HELTEC_UI_HAS_TOUCH
extern const lv_obj_class_t ht_tracker_toolbar_class;
extern const lv_obj_class_t ht_tracker_toolbar_button_class;
extern const lv_obj_class_t ht_tracker_toolbar_label_class;
#endif
extern const lv_obj_class_t ht_map_layer_class;
extern const lv_obj_class_t ht_map_tile_class;
extern const lv_obj_class_t ht_map_marker_class;
extern const lv_obj_class_t ht_map_tile_placeholder_class;
extern const lv_obj_class_t ht_map_marker_label_class;
extern const lv_obj_class_t ht_map_range_ring_class;
extern const lv_obj_class_t ht_map_range_label_class;
#endif
extern const lv_obj_class_t ht_overlay_root_class;
extern const lv_obj_class_t ht_overlay_bar_class;
extern const lv_obj_class_t ht_overlay_feedback_label_class;
extern const lv_obj_class_t ht_confirm_text_class;
extern const lv_obj_class_t ht_confirm_button_class;

extern const lv_obj_class_t ht_preview_overlay_class;
extern const lv_obj_class_t ht_preview_metadata_row_class;
extern const lv_obj_class_t ht_preview_metadata_label_class;
extern const lv_obj_class_t ht_preview_origin_class;
extern const lv_obj_class_t ht_preview_text_class;

#if defined(HELTEC_UI_HAS_TOUCH) && HELTEC_UI_HAS_TOUCH
extern const lv_obj_class_t ht_quick_ping_row_class;
extern const lv_obj_class_t ht_quick_ping_badge_class;
extern const lv_obj_class_t ht_quick_ping_message_row_class;
#endif

extern const lv_obj_class_t ht_radio_sync_overlay_class;
extern const lv_obj_class_t ht_radio_sync_list_class;
#if !defined(HELTEC_UI_HAS_TOUCH) || !HELTEC_UI_HAS_TOUCH
extern const lv_obj_class_t ht_radio_sync_row_class;
#endif
extern const lv_obj_class_t ht_repeat_mode_overlay_class;
extern const lv_obj_class_t ht_repeat_mode_list_class;
#if !defined(HELTEC_UI_HAS_TOUCH) || !HELTEC_UI_HAS_TOUCH
extern const lv_obj_class_t ht_repeat_mode_roller_class;
extern const lv_obj_class_t ht_repeat_mode_item_class;
extern const lv_obj_class_t ht_repeat_mode_label_class;
#endif
#if !defined(HELTEC_UI_HAS_TOUCH) || !HELTEC_UI_HAS_TOUCH
extern const lv_obj_class_t ht_send_message_selector_overlay_class;
#else
extern const lv_obj_class_t ht_send_message_touch_overlay_class;
#endif
extern const lv_obj_class_t ht_send_message_list_class;
#if defined(HELTEC_UI_HAS_TOUCH) && HELTEC_UI_HAS_TOUCH
extern const lv_obj_class_t ht_send_message_touch_list_class;
#else
extern const lv_obj_class_t ht_send_message_row_class;
extern const lv_obj_class_t ht_send_message_text_row_class;
#endif
#if defined(HELTEC_UI_HAS_TOUCH) && HELTEC_UI_HAS_TOUCH
extern const lv_obj_class_t ht_send_message_touch_row_class;
#endif
extern const lv_obj_class_t ht_send_message_label_class;
extern const lv_obj_class_t ht_splash_overlay_class;
extern const lv_obj_class_t ht_splash_logo_class;
extern const lv_obj_class_t ht_splash_label_class;

lv_obj_t *ht_class_create(const lv_obj_class_t *class_p, lv_obj_t *parent, void *user_data = nullptr);
lv_obj_t *ht_label_create(const lv_obj_class_t *class_p, lv_obj_t *parent, const char *text = nullptr, void *user_data = nullptr);
lv_obj_t *ht_label_create_static(const lv_obj_class_t *class_p, lv_obj_t *parent,
                                 const char *text, void *user_data = nullptr);
lv_obj_t *ht_btn_create(const lv_obj_class_t *class_p, lv_obj_t *parent, void *user_data = nullptr);

/** Copies text into LVGL-owned storage. Use only while constructing fixed UI objects. */
void ht_label_set_text_copy(lv_obj_t *label, const char *text);

inline lv_obj_t *ht_obj_create(lv_obj_t *parent) {
    return ht_class_create(&lv_obj_class, parent);
}

inline lv_obj_t *ht_label_create(lv_obj_t *parent, const char *text = nullptr) {
    return ht_label_create(&lv_label_class, parent, text);
}

inline lv_obj_t *ht_label_create_static(lv_obj_t *parent, const char *text) {
    return ht_label_create_static(&lv_label_class, parent, text);
}

} // namespace heltec::meshcore::ui
