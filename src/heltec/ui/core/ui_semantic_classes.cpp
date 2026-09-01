#include "ui_semantic_classes.hpp"

#include "ht_meta_data.hpp"
#include "ht_meta_data_internal.hpp"
#include "ht_container.hpp"
#include "ht_selector_view.hpp"
#include "ui/navigation/ui_navigator.hpp"

namespace heltec::meshcore::ui
{

#define HT_DEFINE_SEMANTIC_CLASS(name, base, instance_type)                                      \
    const lv_obj_class_t name = {                                                               \
        &base,                                                                                  \
        nullptr,                                                                                \
        nullptr,                                                                                \
        nullptr,                                                                                \
        nullptr,                                                                                \
        LV_SIZE_CONTENT,                                                                        \
        LV_SIZE_CONTENT,                                                                        \
        LV_OBJ_CLASS_EDITABLE_INHERIT,                                                          \
        LV_OBJ_CLASS_GROUP_DEF_INHERIT,                                                         \
        sizeof(instance_type),                                                                  \
    }

HT_DEFINE_SEMANTIC_CLASS(ht_top_pane_class, lv_obj_class, lv_obj_t);
HT_DEFINE_SEMANTIC_CLASS(ht_top_pane_right_slot_class, lv_obj_class, lv_obj_t);
HT_DEFINE_SEMANTIC_CLASS(ht_top_pane_battery_outline_class, lv_obj_class, lv_obj_t);
HT_DEFINE_SEMANTIC_CLASS(ht_top_pane_battery_cap_class, lv_obj_class, lv_obj_t);

HT_DEFINE_SEMANTIC_CLASS(ht_button_roller_class, lv_obj_class, lv_obj_t);

HT_DEFINE_SEMANTIC_CLASS(ht_compass_info_column_class, lv_obj_class, lv_obj_t);
HT_DEFINE_SEMANTIC_CLASS(ht_compass_q_label_class, lv_label_class, lv_label_t);
HT_DEFINE_SEMANTIC_CLASS(ht_compass_dial_center_class, lv_obj_class, lv_obj_t);
HT_DEFINE_SEMANTIC_CLASS(ht_compass_dial_hub_class, lv_obj_class, lv_obj_t);
HT_DEFINE_SEMANTIC_CLASS(ht_compass_dial_needle_class, lv_obj_class, lv_obj_t);

HT_DEFINE_SEMANTIC_CLASS(ht_license_gate_class, lv_obj_class, lv_obj_t);
HT_DEFINE_SEMANTIC_CLASS(ht_license_gate_chip_class, lv_label_class, lv_label_t);

HT_DEFINE_SEMANTIC_CLASS(ht_context_menu_class, lv_obj_class, lv_obj_t);
HT_DEFINE_SEMANTIC_CLASS(ht_context_menu_icon_row_class, lv_obj_class, lv_obj_t);
HT_DEFINE_SEMANTIC_CLASS(ht_context_menu_icon_button_class, lv_btn_class, lv_btn_t);

HT_DEFINE_SEMANTIC_CLASS(ht_app_background_class, lv_img_class, lv_img_t);
HT_DEFINE_SEMANTIC_CLASS(ht_app_overlay_layer_class, lv_obj_class, lv_obj_t);
HT_DEFINE_SEMANTIC_CLASS(ht_app_frame_class, lv_obj_class, lv_obj_t);
HT_DEFINE_SEMANTIC_CLASS(ht_app_content_class, lv_obj_class, lv_obj_t);
HT_DEFINE_SEMANTIC_CLASS(ht_app_screen_host_class, lv_obj_class, lv_obj_t);

HT_DEFINE_SEMANTIC_CLASS(ht_navigation_root_class, lv_obj_class, lv_obj_t);
HT_DEFINE_SEMANTIC_CLASS(ht_navigation_content_class, lv_obj_class, lv_obj_t);
HT_DEFINE_SEMANTIC_CLASS(ht_navigation_cell_class, lv_obj_class, lv_obj_t);
HT_DEFINE_SEMANTIC_CLASS(ht_navigation_icon_area_class, lv_obj_class, lv_obj_t);
HT_DEFINE_SEMANTIC_CLASS(ht_navigation_title_bar_class, lv_obj_class, lv_obj_t);
HT_DEFINE_SEMANTIC_CLASS(ht_navigation_ring_class, ht_navigator_class, ht_navigator_t);

HT_DEFINE_SEMANTIC_CLASS(ht_map_status_label_class, lv_label_class, lv_label_t);
HT_DEFINE_SEMANTIC_CLASS(ht_map_viewport_class, lv_obj_class, lv_obj_t);
HT_DEFINE_SEMANTIC_CLASS(ht_map_toolbar_class, lv_obj_class, lv_obj_t);
HT_DEFINE_SEMANTIC_CLASS(ht_map_layer_class, lv_obj_class, lv_obj_t);
HT_DEFINE_SEMANTIC_CLASS(ht_map_tile_class, lv_obj_class, lv_obj_t);
HT_DEFINE_SEMANTIC_CLASS(ht_map_marker_class, lv_obj_class, lv_obj_t);

HT_DEFINE_SEMANTIC_CLASS(ht_overlay_root_class, lv_obj_class, lv_obj_t);
HT_DEFINE_SEMANTIC_CLASS(ht_overlay_bar_class, lv_label_class, lv_label_t);
HT_DEFINE_SEMANTIC_CLASS(ht_overlay_feedback_label_class, lv_label_class, lv_label_t);
HT_DEFINE_SEMANTIC_CLASS(ht_confirm_text_class, lv_label_class, lv_label_t);
HT_DEFINE_SEMANTIC_CLASS(ht_confirm_key_hint_class, ht_overlay_bar_class, lv_label_t);
HT_DEFINE_SEMANTIC_CLASS(ht_confirm_button_class, lv_btn_class, lv_btn_t);

HT_DEFINE_SEMANTIC_CLASS(ht_preview_overlay_class, ht_selector_view_class, ht_selector_view_t);
HT_DEFINE_SEMANTIC_CLASS(ht_preview_metadata_row_class, lv_obj_class, lv_obj_t);
HT_DEFINE_SEMANTIC_CLASS(ht_preview_origin_class, lv_label_class, lv_label_t);
HT_DEFINE_SEMANTIC_CLASS(ht_preview_text_class, lv_label_class, lv_label_t);

HT_DEFINE_SEMANTIC_CLASS(ht_quick_ping_row_class, ht_container_class, ht_container_t);
HT_DEFINE_SEMANTIC_CLASS(ht_quick_ping_badge_class, lv_obj_class, lv_obj_t);
HT_DEFINE_SEMANTIC_CLASS(ht_quick_ping_message_row_class, lv_obj_class, lv_obj_t);

HT_DEFINE_SEMANTIC_CLASS(ht_radio_sync_overlay_class, ht_selector_view_class,
                         ht_selector_view_t);
HT_DEFINE_SEMANTIC_CLASS(ht_radio_sync_list_class, lv_obj_class, lv_obj_t);
HT_DEFINE_SEMANTIC_CLASS(ht_repeat_mode_overlay_class, lv_obj_class, lv_obj_t);
HT_DEFINE_SEMANTIC_CLASS(ht_repeat_mode_list_class, lv_obj_class, lv_obj_t);
HT_DEFINE_SEMANTIC_CLASS(ht_repeat_mode_roller_class, ht_button_roller_class, lv_obj_t);
HT_DEFINE_SEMANTIC_CLASS(ht_send_message_selector_overlay_class, ht_selector_view_class,
                         ht_selector_view_t);
HT_DEFINE_SEMANTIC_CLASS(ht_send_message_touch_overlay_class, lv_obj_class, lv_obj_t);
HT_DEFINE_SEMANTIC_CLASS(ht_send_message_list_class, lv_obj_class, lv_obj_t);
HT_DEFINE_SEMANTIC_CLASS(ht_send_message_touch_list_class, lv_obj_class, lv_obj_t);
HT_DEFINE_SEMANTIC_CLASS(ht_splash_overlay_class, lv_obj_class, lv_obj_t);

#undef HT_DEFINE_SEMANTIC_CLASS

lv_obj_t *ht_semantic_create(const lv_obj_class_t *class_p, lv_obj_t *parent, void *user_data)
{
    return detail::create_object(class_p, parent, user_data);
}

lv_obj_t *ht_semantic_label_create(const lv_obj_class_t *class_p, lv_obj_t *parent,
                                   const char *text, void *user_data)
{
    lv_obj_t *const label = ht_semantic_create(class_p, parent, user_data);
    if (label && text)
    {
        lv_label_set_text(label, text);
    }
    return label;
}

lv_obj_t *ht_semantic_button_create(const lv_obj_class_t *class_p, lv_obj_t *parent,
                                    void *user_data)
{
    return ht_semantic_create(class_p, parent, user_data);
}

} // namespace heltec::meshcore::ui
