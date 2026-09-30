#include "widget_classes.hpp"

#include "ht_container.hpp"
#include "ht_dropdown.hpp"
#include "ht_selector_view.hpp"
#include "ht_textarea.hpp"
#include "ui/navigation/grid_navigator.hpp"

namespace heltec::meshcore::ui {

#define HT_DEFINE_CLASS(name, base, instance_type) \
    const lv_obj_class_t name = {                         \
        &base,                                            \
        nullptr,                                          \
        nullptr,                                          \
        nullptr,                                          \
        nullptr,                                          \
        LV_SIZE_CONTENT,                                  \
        LV_SIZE_CONTENT,                                  \
        LV_OBJ_CLASS_EDITABLE_INHERIT,                    \
        LV_OBJ_CLASS_GROUP_DEF_INHERIT,                   \
        sizeof(instance_type),                            \
    }

HT_DEFINE_CLASS(ht_dialog_overlay_class, lv_obj_class, lv_obj_t);
HT_DEFINE_CLASS(ht_dialog_panel_class, lv_obj_class, lv_obj_t);
HT_DEFINE_CLASS(ht_dialog_content_class, lv_obj_class, lv_obj_t);
HT_DEFINE_CLASS(ht_dialog_actions_class, lv_obj_class, lv_obj_t);

HT_DEFINE_CLASS(ht_framed_overlay_content_class, lv_obj_class, lv_obj_t);
#if !defined(HELTEC_HAS_TOUCH) || !HELTEC_HAS_TOUCH
HT_DEFINE_CLASS(ht_send_message_content_class, ht_framed_overlay_content_class, lv_obj_t);
#endif

HT_DEFINE_CLASS(ht_volume_controls_class, lv_obj_class, lv_obj_t);
HT_DEFINE_CLASS(ht_volume_button_class, lv_btn_class, lv_btn_t);

#if LV_USE_KEYBOARD
HT_DEFINE_CLASS(ht_keyboard_overlay_class, lv_obj_class, lv_obj_t);
HT_DEFINE_CLASS(ht_keyboard_title_class, lv_label_class, lv_label_t);
HT_DEFINE_CLASS(ht_keyboard_editor_class, ht_textarea_class, ht_textarea_t);
HT_DEFINE_CLASS(ht_keyboard_matrix_class, lv_keyboard_class, lv_keyboard_t);
HT_DEFINE_CLASS(ht_coordinate_row_class, lv_obj_class, lv_obj_t);
HT_DEFINE_CLASS(ht_coordinate_label_class, lv_label_class, lv_label_t);
#endif

HT_DEFINE_CLASS(ht_top_pane_class, lv_obj_class, lv_obj_t);
HT_DEFINE_CLASS(ht_top_pane_right_slot_class, lv_obj_class, lv_obj_t);
HT_DEFINE_CLASS(ht_top_pane_title_class, lv_label_class, lv_label_t);
HT_DEFINE_CLASS(ht_top_pane_battery_class, lv_obj_class, lv_obj_t);
HT_DEFINE_CLASS(ht_top_pane_battery_outline_class, lv_obj_class, lv_obj_t);
HT_DEFINE_CLASS(ht_top_pane_battery_cap_class, lv_obj_class, lv_obj_t);

HT_DEFINE_CLASS(ht_button_roller_class, lv_obj_class, lv_obj_t);
HT_DEFINE_CLASS(ht_button_roller_item_class, lv_btn_class, lv_btn_t);
HT_DEFINE_CLASS(ht_button_roller_label_class, lv_label_class, lv_label_t);

HT_DEFINE_CLASS(ht_license_gate_class, lv_obj_class, lv_obj_t);
HT_DEFINE_CLASS(ht_license_gate_label_class, lv_label_class, lv_label_t);
HT_DEFINE_CLASS(ht_license_gate_chip_class, lv_label_class, lv_label_t);

#if !defined(HELTEC_HAS_TOUCH) || !HELTEC_HAS_TOUCH
HT_DEFINE_CLASS(ht_context_menu_class, lv_obj_class, lv_obj_t);
HT_DEFINE_CLASS(ht_context_menu_icon_row_class, lv_obj_class, lv_obj_t);
HT_DEFINE_CLASS(ht_context_menu_icon_button_class, lv_btn_class, lv_btn_t);
HT_DEFINE_CLASS(ht_context_menu_icon_class, lv_img_class, lv_img_t);
HT_DEFINE_CLASS(ht_context_menu_title_class, lv_label_class, lv_label_t);
HT_DEFINE_CLASS(ht_context_menu_nav_row_class, lv_obj_class, lv_obj_t);
#endif

HT_DEFINE_CLASS(ht_cascading_menu_roller_class, ht_button_roller_class, lv_obj_t);
HT_DEFINE_CLASS(ht_cascading_menu_item_class, ht_button_roller_item_class, lv_btn_t);
HT_DEFINE_CLASS(ht_cascading_menu_label_class, ht_button_roller_label_class, lv_label_t);
#if LV_USE_MENU
HT_DEFINE_CLASS(ht_cascading_menu_page_class, lv_menu_page_class, lv_menu_page_t);
#endif

HT_DEFINE_CLASS(ht_app_background_class, lv_img_class, lv_img_t);
HT_DEFINE_CLASS(ht_app_overlay_layer_class, lv_obj_class, lv_obj_t);
HT_DEFINE_CLASS(ht_app_frame_class, lv_obj_class, lv_obj_t);
HT_DEFINE_CLASS(ht_app_content_class, lv_obj_class, lv_obj_t);
HT_DEFINE_CLASS(ht_app_screen_host_class, lv_obj_class, lv_obj_t);

HT_DEFINE_CLASS(ht_navigation_root_class, lv_obj_class, lv_obj_t);
HT_DEFINE_CLASS(ht_navigation_content_class, lv_obj_class, lv_obj_t);
#if defined(UI_NAVIGATION_GRID) && UI_NAVIGATION_GRID
HT_DEFINE_CLASS(ht_navigation_cell_class, lv_obj_class, lv_obj_t);
HT_DEFINE_CLASS(ht_navigation_icon_area_class, lv_obj_class, lv_obj_t);
HT_DEFINE_CLASS(ht_navigation_title_bar_class, lv_obj_class, lv_obj_t);
HT_DEFINE_CLASS(ht_navigation_grid_icon_class, lv_img_class, lv_img_t);
HT_DEFINE_CLASS(ht_navigation_title_label_class, lv_label_class, lv_label_t);
#endif
HT_DEFINE_CLASS(ht_navigation_ring_class, ht_navigator_class, ht_navigator_t);
#if !defined(UI_NAVIGATION_GRID) || !UI_NAVIGATION_GRID
HT_DEFINE_CLASS(ht_radial_navigation_icon_class, lv_img_class, lv_img_t);
#endif

HT_DEFINE_CLASS(ht_compass_plain_class, lv_obj_class, lv_obj_t);
HT_DEFINE_CLASS(ht_compass_hub_class, lv_obj_class, lv_obj_t);
HT_DEFINE_CLASS(ht_compass_needle_class, lv_obj_class, lv_obj_t);
HT_DEFINE_CLASS(ht_compass_label_class, lv_label_class, lv_label_t);
HT_DEFINE_CLASS(ht_find_friend_dial_host_class, lv_obj_class, lv_obj_t);
HT_DEFINE_CLASS(ht_screen_info_label_class, lv_label_class, lv_label_t);

#if defined(ENV_INCLUDE_MAP) && ENV_INCLUDE_MAP
HT_DEFINE_CLASS(ht_map_status_label_class, lv_label_class, lv_label_t);
HT_DEFINE_CLASS(ht_map_viewport_class, lv_obj_class, lv_obj_t);
HT_DEFINE_CLASS(ht_map_toolbar_class, lv_obj_class, lv_obj_t);
HT_DEFINE_CLASS(ht_map_toolbar_button_class, lv_btn_class, lv_btn_t);
HT_DEFINE_CLASS(ht_map_toolbar_label_class, lv_label_class, lv_label_t);
HT_DEFINE_CLASS(ht_map_toolbar_dropdown_class, ht_dropdown_class, ht_dropdown_t);
#if defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
HT_DEFINE_CLASS(ht_tracker_toolbar_class, ht_map_toolbar_class, lv_obj_t);
HT_DEFINE_CLASS(ht_tracker_toolbar_button_class, ht_map_toolbar_button_class, lv_btn_t);
HT_DEFINE_CLASS(ht_tracker_toolbar_label_class, ht_map_toolbar_label_class, lv_label_t);
#endif
HT_DEFINE_CLASS(ht_map_layer_class, lv_obj_class, lv_obj_t);
HT_DEFINE_CLASS(ht_map_tile_class, lv_obj_class, lv_obj_t);
HT_DEFINE_CLASS(ht_map_marker_class, lv_obj_class, lv_obj_t);
HT_DEFINE_CLASS(ht_map_tile_placeholder_class, lv_label_class, lv_label_t);
HT_DEFINE_CLASS(ht_map_marker_label_class, lv_label_class, lv_label_t);
HT_DEFINE_CLASS(ht_map_range_ring_class, lv_obj_class, lv_obj_t);
HT_DEFINE_CLASS(ht_map_range_label_class, lv_label_class, lv_label_t);
#endif
HT_DEFINE_CLASS(ht_overlay_root_class, lv_obj_class, lv_obj_t);
HT_DEFINE_CLASS(ht_overlay_bar_class, lv_label_class, lv_label_t);
HT_DEFINE_CLASS(ht_overlay_header_class, ht_overlay_bar_class, lv_label_t);
HT_DEFINE_CLASS(ht_overlay_footer_class, ht_overlay_bar_class, lv_label_t);
HT_DEFINE_CLASS(ht_overlay_feedback_label_class, lv_label_class, lv_label_t);
HT_DEFINE_CLASS(ht_confirm_text_class, lv_label_class, lv_label_t);
HT_DEFINE_CLASS(ht_confirm_button_class, lv_btn_class, lv_btn_t);

HT_DEFINE_CLASS(ht_preview_overlay_class, ht_selector_view_class, ht_selector_view_t);
HT_DEFINE_CLASS(ht_preview_metadata_row_class, lv_obj_class, lv_obj_t);
HT_DEFINE_CLASS(ht_preview_metadata_label_class, lv_label_class, lv_label_t);
HT_DEFINE_CLASS(ht_preview_origin_class, lv_label_class, lv_label_t);
HT_DEFINE_CLASS(ht_preview_text_class, lv_label_class, lv_label_t);

#if defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
HT_DEFINE_CLASS(ht_quick_ping_row_class, ht_container_class, ht_container_t);
HT_DEFINE_CLASS(ht_quick_ping_badge_class, lv_obj_class, lv_obj_t);
HT_DEFINE_CLASS(ht_quick_ping_message_row_class, lv_obj_class, lv_obj_t);
#endif

HT_DEFINE_CLASS(ht_radio_sync_overlay_class, ht_selector_view_class, ht_selector_view_t);
HT_DEFINE_CLASS(ht_radio_sync_list_class, lv_obj_class, lv_obj_t);
#if !defined(HELTEC_HAS_TOUCH) || !HELTEC_HAS_TOUCH
HT_DEFINE_CLASS(ht_radio_sync_row_class, lv_label_class, lv_label_t);
#endif
HT_DEFINE_CLASS(ht_repeat_mode_overlay_class, lv_obj_class, lv_obj_t);
HT_DEFINE_CLASS(ht_repeat_mode_list_class, lv_obj_class, lv_obj_t);
#if !defined(HELTEC_HAS_TOUCH) || !HELTEC_HAS_TOUCH
HT_DEFINE_CLASS(ht_repeat_mode_roller_class, ht_button_roller_class, lv_obj_t);
HT_DEFINE_CLASS(ht_repeat_mode_item_class, ht_button_roller_item_class, lv_btn_t);
HT_DEFINE_CLASS(ht_repeat_mode_label_class, ht_button_roller_label_class, lv_label_t);
#endif
#if !defined(HELTEC_HAS_TOUCH) || !HELTEC_HAS_TOUCH
HT_DEFINE_CLASS(ht_send_message_selector_overlay_class, ht_selector_view_class,
                       ht_selector_view_t);
#else
HT_DEFINE_CLASS(ht_send_message_touch_overlay_class, lv_obj_class, lv_obj_t);
#endif
HT_DEFINE_CLASS(ht_send_message_list_class, lv_obj_class, lv_obj_t);
#if defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
HT_DEFINE_CLASS(ht_send_message_touch_list_class, lv_obj_class, lv_obj_t);
#else
HT_DEFINE_CLASS(ht_send_message_row_class, lv_obj_class, lv_obj_t);
HT_DEFINE_CLASS(ht_send_message_text_row_class, lv_label_class, lv_label_t);
#endif
#if defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
HT_DEFINE_CLASS(ht_send_message_touch_row_class, ht_container_class, ht_container_t);
#endif
HT_DEFINE_CLASS(ht_send_message_label_class, lv_label_class, lv_label_t);
HT_DEFINE_CLASS(ht_splash_overlay_class, lv_obj_class, lv_obj_t);
HT_DEFINE_CLASS(ht_splash_logo_class, lv_img_class, lv_img_t);
HT_DEFINE_CLASS(ht_splash_label_class, lv_label_class, lv_label_t);

#undef HT_DEFINE_CLASS

lv_obj_t *ht_class_create(const lv_obj_class_t *class_p, lv_obj_t *parent, void *user_data) {
    lv_obj_t *const obj = lv_obj_class_create_obj(class_p, parent);
    if (!obj) {
        return nullptr;
    }

    lv_obj_class_init_obj(obj);
    lv_obj_set_user_data(obj, user_data);
    return obj;
}

lv_obj_t *ht_label_create(const lv_obj_class_t *class_p, lv_obj_t *parent, const char *text, void *user_data) {
    lv_obj_t *const label = ht_class_create(class_p, parent, user_data);
    if (label && text) {
        ht_label_set_text_copy(label, text);
    }
    return label;
}

lv_obj_t *ht_label_create_static(const lv_obj_class_t *class_p, lv_obj_t *parent,
                                 const char *text, void *user_data) {
    lv_obj_t *const label = ht_class_create(class_p, parent, user_data);
    if (label) {
        lv_label_set_text_static(label, text ? text : "");
    }
    return label;
}

void ht_label_set_text_copy(lv_obj_t *label, const char *text) {
    if (label) {
        lv_label_set_text(label, text ? text : "");
    }
}

lv_obj_t *ht_btn_create(const lv_obj_class_t *class_p, lv_obj_t *parent, void *user_data) {
    return ht_class_create(class_p, parent, user_data);
}

} // namespace heltec::meshcore::ui
