#pragma once

#include <lvgl.h>

namespace heltec::meshcore::ui
{

extern const lv_obj_class_t ht_top_pane_class;
extern const lv_obj_class_t ht_top_pane_right_slot_class;
extern const lv_obj_class_t ht_top_pane_battery_outline_class;
extern const lv_obj_class_t ht_top_pane_battery_cap_class;

extern const lv_obj_class_t ht_button_roller_class;

extern const lv_obj_class_t ht_compass_info_column_class;
extern const lv_obj_class_t ht_compass_q_label_class;
extern const lv_obj_class_t ht_compass_dial_center_class;
extern const lv_obj_class_t ht_compass_dial_hub_class;
extern const lv_obj_class_t ht_compass_dial_needle_class;

extern const lv_obj_class_t ht_license_gate_class;
extern const lv_obj_class_t ht_license_gate_chip_class;

extern const lv_obj_class_t ht_context_menu_class;
extern const lv_obj_class_t ht_context_menu_icon_row_class;
extern const lv_obj_class_t ht_context_menu_icon_button_class;

extern const lv_obj_class_t ht_app_background_class;
extern const lv_obj_class_t ht_app_overlay_layer_class;
extern const lv_obj_class_t ht_app_frame_class;
extern const lv_obj_class_t ht_app_content_class;
extern const lv_obj_class_t ht_app_screen_host_class;

extern const lv_obj_class_t ht_navigation_root_class;
extern const lv_obj_class_t ht_navigation_content_class;
extern const lv_obj_class_t ht_navigation_cell_class;
extern const lv_obj_class_t ht_navigation_icon_area_class;
extern const lv_obj_class_t ht_navigation_title_bar_class;
extern const lv_obj_class_t ht_navigation_ring_class;

extern const lv_obj_class_t ht_map_status_label_class;
extern const lv_obj_class_t ht_map_viewport_class;
extern const lv_obj_class_t ht_map_toolbar_class;
extern const lv_obj_class_t ht_map_layer_class;
extern const lv_obj_class_t ht_map_tile_class;
extern const lv_obj_class_t ht_map_marker_class;

extern const lv_obj_class_t ht_overlay_root_class;
extern const lv_obj_class_t ht_overlay_bar_class;
extern const lv_obj_class_t ht_overlay_feedback_label_class;
extern const lv_obj_class_t ht_confirm_text_class;
extern const lv_obj_class_t ht_confirm_key_hint_class;
extern const lv_obj_class_t ht_confirm_button_class;

extern const lv_obj_class_t ht_preview_overlay_class;
extern const lv_obj_class_t ht_preview_metadata_row_class;
extern const lv_obj_class_t ht_preview_origin_class;
extern const lv_obj_class_t ht_preview_text_class;

extern const lv_obj_class_t ht_quick_ping_row_class;
extern const lv_obj_class_t ht_quick_ping_badge_class;
extern const lv_obj_class_t ht_quick_ping_message_row_class;

extern const lv_obj_class_t ht_radio_sync_overlay_class;
extern const lv_obj_class_t ht_radio_sync_list_class;
extern const lv_obj_class_t ht_repeat_mode_overlay_class;
extern const lv_obj_class_t ht_repeat_mode_list_class;
extern const lv_obj_class_t ht_repeat_mode_roller_class;
extern const lv_obj_class_t ht_send_message_selector_overlay_class;
extern const lv_obj_class_t ht_send_message_touch_overlay_class;
extern const lv_obj_class_t ht_send_message_list_class;
extern const lv_obj_class_t ht_send_message_touch_list_class;
extern const lv_obj_class_t ht_splash_overlay_class;

lv_obj_t *ht_semantic_create(const lv_obj_class_t *class_p, lv_obj_t *parent,
                             void *user_data = nullptr);
lv_obj_t *ht_semantic_label_create(const lv_obj_class_t *class_p, lv_obj_t *parent,
                                   const char *text = nullptr, void *user_data = nullptr);
lv_obj_t *ht_semantic_button_create(const lv_obj_class_t *class_p, lv_obj_t *parent,
                                    void *user_data = nullptr);

} // namespace heltec::meshcore::ui
