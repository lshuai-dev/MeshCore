#include "ui/theme/WidgetTheme.hpp"
#include "ui/theme/ThemeUtil.hpp"
#include "ui/theme/UiTheme.hpp"
#include "ui/core/AbstractOverlay.hpp"
#include "ui/core/DialogOverlay.hpp"
#include "ui/core/FramedOverlay.hpp"
#include "ui/widgets/HtContainer.hpp"
#include "ui/widgets/SemanticClasses.hpp"
#include "ui/menus/ContextMenu.hpp"
#include "ui/navigation/NavigationWidget.hpp"
#include "ui/overlays/AlertOverlay.hpp"
#include "ui/overlays/CalibrationOverlay.hpp"
#include "ui/overlays/ConfirmOverlay.hpp"
#if HELTEC_UI_ENABLE_KEYBOARD
#include "ui/overlays/KeyboardOverlay.hpp"
#endif
#include "ui/overlays/PreviewOverlay.hpp"
#if HELTEC_UI_HAS_TOUCH
#include "ui/overlays/QuickPingOverlay.hpp"
#endif
#include "ui/overlays/RadioParamSyncOverlay.hpp"
#include "ui/overlays/RepeatModeOverlay.hpp"
#include "ui/overlays/SplashOverlay.hpp"
#include "ui/screens/CompassDialWidget.hpp"
#include "ui/screens/SystemScreen.hpp"
#if defined(ENV_INCLUDE_MAP) && ENV_INCLUDE_MAP
#include "ui/screens/TrackerScreen.hpp"
#endif
#include "ui/widgets/ButtonRoller.hpp"
#include "ui/widgets/TopPane.hpp"

#include <lvgl.h>

namespace Heltec::MeshCore::Ui
{
namespace
{

#if HELTEC_UI_HAS_TOUCH
constexpr lv_coord_t kScreenPadding = 2;
#else
constexpr lv_coord_t kScreenPadding = 0;
#endif

static const lv_style_selector_t kInteractiveSelectors[] = {
    LV_PART_MAIN,
    LV_PART_MAIN | LV_STATE_FOCUSED,
    LV_PART_MAIN | LV_STATE_FOCUS_KEY,
    LV_PART_MAIN | LV_STATE_PRESSED,
};

#if !HELTEC_UI_HAS_TOUCH
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

#if !HELTEC_UI_HAS_TOUCH
static lv_style_t s_ctx_menu_style;
static lv_style_t s_ctx_header_style;
static lv_style_t s_ctx_header_row_style;
static lv_style_t s_ctx_icon_row_layout_style;
static lv_style_t s_ctx_title_style;
static lv_style_t s_ctx_icon_btn_main_style;
static lv_style_t s_ctx_icon_btn_checked_style;
static lv_style_t s_ctx_icon_style;
static lv_style_t s_ctx_icon_checked_style;
static bool s_ctx_styles_ready = false;
#endif

static lv_style_t s_overlay_center_label_style;
static bool s_overlay_center_label_style_ready = false;
static lv_style_t s_overlay_header_footer_style;
static lv_style_t s_framed_overlay_content_style;
static bool s_overlay_header_footer_style_ready = false;
static lv_style_t s_send_list_style;
#if HELTEC_UI_HAS_TOUCH
static lv_style_t s_send_touch_list_style;
#else
static lv_style_t s_send_content_style;
#endif
static lv_style_t s_send_root_layout_style;
static lv_style_t s_send_row_main_style;
static lv_style_t s_send_row_active_style;
static bool s_send_styles_ready = false;

static lv_style_t s_radio_sync_list_style;
static lv_style_t s_radio_sync_root_layout_style;
#if HELTEC_UI_HAS_TOUCH
static lv_style_t s_overlay_roller_style;
static lv_style_t s_overlay_roller_selected_style;
#else
static lv_style_t s_radio_sync_row_main_style;
static lv_style_t s_radio_sync_row_checked_style;
#endif
static bool s_radio_sync_styles_ready = false;

static lv_style_t s_repeat_root_layout_style;
static lv_style_t s_repeat_list_style;
#if !HELTEC_UI_HAS_TOUCH
static lv_style_t s_repeat_item_style;
static lv_style_t s_repeat_item_selected_style;
#endif
static bool s_repeat_styles_ready = false;

#if HELTEC_UI_ENABLE_KEYBOARD
static lv_style_t s_keyboard_textarea_style;
static lv_style_t s_keyboard_waypoint_textarea_style;
static lv_style_t s_keyboard_active_textarea_style;
static lv_style_t s_keyboard_title_style;
static lv_style_t s_keyboard_waypoint_title_style;
#if HELTEC_UI_HAS_TOUCH
static lv_style_t s_coordinate_row_style;
static lv_style_t s_coordinate_label_style;
#endif
static lv_style_t s_keyboard_root_layout_style;
static lv_style_t s_keyboard_root_waypoint_layout_style;
static lv_style_t s_keyboard_surface_color_style;
#if HELTEC_UI_HAS_TOUCH
static lv_style_t s_keyboard_waypoint_surface_style;
#endif
static lv_style_t s_keyboard_main_style;
static lv_style_t s_keyboard_waypoint_main_style;
static lv_style_t s_keyboard_items_base_style;
static lv_style_t s_keyboard_items_selected_style;
static bool s_keyboard_child_styles_ready = false;
#endif

static lv_style_t s_overlay_label_all_states_style;
static lv_style_t s_alert_box_style;
static lv_style_t s_calibration_panel_style;
static bool s_overlay_feedback_styles_ready = false;

static lv_style_t s_surface_root_common_style;
static lv_style_t s_screen_root_layout_style;
static lv_style_t s_system_screen_root_layout_style;
static lv_style_t s_container_layout_style;
#if defined(ENV_INCLUDE_MAP) && ENV_INCLUDE_MAP && defined(HELTEC_UI_HAS_TOUCH) && HELTEC_UI_HAS_TOUCH
static lv_style_t s_tracker_root_layout_style;
#endif
static lv_style_t s_preview_root_layout_style;
static lv_style_t s_splash_root_layout_style;
static lv_style_t s_plain_container_style;
static lv_style_t s_no_chrome_style;
static lv_style_t s_hidden_scrollbar_style;
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

struct NavigationStyleConfig
{
    lv_coord_t grid_label_height;
    lv_coord_t grid_cell_radius;
    lv_coord_t ring_edge_pad;
};

static const UiThemeProfile kDefaultThemeProfile = {
    { 2, 0, 0, 2, 12, 4 },
    { 12, 0, 3, 1 },
    { 0, 14, 8 },
};
static const UiThemeProfile *s_themeProfile = &kDefaultThemeProfile;
static constexpr NavigationStyleConfig kNavigationStyleConfig = {
    18, 8, 2,
};
#if !HELTEC_UI_HAS_TOUCH
static constexpr lv_coord_t kContextMenuBorderWidth = 1;
static constexpr lv_coord_t kContextMenuTitleBorderWidth = 1;
static constexpr lv_coord_t kContextMenuFrameInsetX = 8;
#endif

static const UiThemeProfile &themeProfile()
{
    return *s_themeProfile;
}

static lv_style_t s_alert_overlay_root_style;
static lv_style_t s_calibration_overlay_root_style;
static lv_style_t s_splash_overlay_text_style;
#if !HELTEC_UI_HAS_TOUCH
static lv_style_t s_context_menu_root_style;
#endif
static lv_style_t s_app_frame_layout_style;
static lv_style_t s_app_content_style;
static lv_style_t s_app_screen_root_style;
#if HELTEC_UI_HAS_TOUCH
static lv_style_t s_app_tileview_style;
#endif
static lv_style_t s_app_tile_style;
static bool s_surface_app_styles_ready = false;
static bool s_splash_overlay_text_style_ready = false;
static const lv_font_t *s_splash_overlay_text_font = nullptr;

#if HELTEC_UI_HAS_TOUCH
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
static lv_style_t s_quick_ping_message_input_label_editing_style;
static lv_style_t s_quick_ping_keyboard_style;
static lv_style_t s_quick_ping_keyboard_editor_style;
static lv_style_t s_quick_ping_keyboard_placeholder_style;
static lv_style_t s_quick_ping_keyboard_counter_style;
static lv_style_t s_quick_ping_keyboard_cursor_style;
static lv_style_t s_quick_ping_icon_badge_style;
static bool s_quick_ping_styles_ready = false;
#endif
static lv_style_t s_screen_row_layout_style;
static lv_style_t s_container_focus_style;
static lv_style_t s_find_friend_dial_layout_style;
static lv_style_t s_screen_dropdown_list_layout_style;
static lv_style_t s_system_dropdown_style;
static lv_style_t s_system_dropdown_focus_style;
static lv_style_t s_system_volume_controls_style;
static lv_style_t s_system_volume_button_style;
static lv_style_t s_system_volume_button_pressed_style;
static lv_style_t s_system_volume_button_focus_style;
static lv_style_t s_system_volume_slider_style;
static lv_style_t s_system_volume_slider_indicator_style;
static lv_style_t s_system_volume_slider_knob_style;
static lv_style_t s_confirm_overlay_backdrop_style;
static lv_style_t s_dialog_panel_style;
static lv_style_t s_confirm_overlay_text_style;
static lv_style_t s_confirm_overlay_button_row_style;
static lv_style_t s_confirm_overlay_button_style;
static lv_style_t s_confirm_overlay_button_pressed_style;
static bool s_screen_system_styles_ready = false;

static lv_style_t s_visibility_hidden_style;
#if HELTEC_UI_HAS_TOUCH
static lv_style_t s_nav_content_host_style;
static lv_style_t s_nav_grid_cell_style;
static lv_style_t s_nav_grid_cell_focus_style;
static lv_style_t s_nav_grid_title_bar_style;
static lv_style_t s_nav_grid_title_label_style;
static lv_style_t s_nav_panel_grid_style;
static lv_style_t s_nav_footer_bar_style;
#else
static lv_style_t s_nav_root_ring_style;
static lv_style_t s_nav_ring_style;
#endif
static bool s_navigation_styles_ready = false;

static lv_style_t s_overlayRootChromeStyle;
static bool s_overlayRootChromeStyle_ready = false;

struct LabelStyleSlot
{
    bool ready = false;
    uint32_t color = 0;
    lv_text_align_t align = LV_TEXT_ALIGN_LEFT;
    lv_style_t style;
};

static LabelStyleSlot s_label_styles[6];

static lv_style_t *cachedLabelStyle(lv_color_t color, lv_text_align_t align)
{
    const uint32_t color32 = lv_color_to32(color);
    for (LabelStyleSlot &slot : s_label_styles)
    {
        if (slot.ready && slot.color == color32 && slot.align == align)
        {
            return &slot.style;
        }
    }
    for (LabelStyleSlot &slot : s_label_styles)
    {
        if (slot.ready)
            continue;
        slot.ready = true;
        slot.color = color32;
        slot.align = align;
        lv_style_init(&slot.style);
        lv_style_set_text_color(&slot.style, color);
        lv_style_set_text_align(&slot.style, align);
        return &slot.style;
    }
    return nullptr;
}

static void setOverlayTextSpacing(lv_style_t *style)
{
    if (!style)
        return;
    lv_style_set_text_line_space(style, 0);
    lv_style_set_text_letter_space(style, 0);
}

static void initOverlayCenterLabelStyle()
{
    if (s_overlay_center_label_style_ready)
        return;
    lv_style_init(&s_overlay_center_label_style);
    lv_style_set_text_color(&s_overlay_center_label_style, lv_color_hex(themePalette().overlay_fg));
    lv_style_set_text_align(&s_overlay_center_label_style, LV_TEXT_ALIGN_CENTER);
    setOverlayTextSpacing(&s_overlay_center_label_style);
    s_overlay_center_label_style_ready = true;
}

static lv_coord_t reference_card_gap();

static void initOverlayHeaderFooterStyle()
{
    if (s_overlay_header_footer_style_ready)
        return;
    lv_style_init(&s_overlay_header_footer_style);
    lv_style_set_bg_color(&s_overlay_header_footer_style, lv_color_hex(themePalette().highlight_bg));
    lv_style_set_bg_opa(&s_overlay_header_footer_style, LV_OPA_COVER);
    lv_style_set_text_color(&s_overlay_header_footer_style, lv_color_hex(themePalette().highlight_fg));
    lv_style_set_text_align(&s_overlay_header_footer_style, LV_TEXT_ALIGN_CENTER);
    lv_style_set_height(&s_overlay_header_footer_style, LV_SIZE_CONTENT);
    lv_style_set_border_width(&s_overlay_header_footer_style, 0);
    lv_style_set_outline_width(&s_overlay_header_footer_style, 0);
    lv_style_set_shadow_width(&s_overlay_header_footer_style, 0);
    lv_style_set_pad_all(&s_overlay_header_footer_style, 0);
    const lv_coord_t bar_height = themeProfile().app_frame.bar_height;
    const lv_coord_t line_height = lv_font_get_line_height(LV_FONT_DEFAULT);
    const lv_coord_t free_height = bar_height > line_height ? bar_height - line_height : 0;
    const lv_coord_t pad_top = free_height / 2;
    lv_style_set_pad_top(&s_overlay_header_footer_style, pad_top);
    lv_style_set_pad_bottom(&s_overlay_header_footer_style, free_height - pad_top);
    setOverlayTextSpacing(&s_overlay_header_footer_style);

    lv_style_init(&s_framed_overlay_content_style);
    lv_style_set_width(&s_framed_overlay_content_style, lv_pct(100));
    lv_style_set_bg_opa(&s_framed_overlay_content_style, LV_OPA_TRANSP);
    lv_style_set_border_width(&s_framed_overlay_content_style, 0);
    lv_style_set_outline_width(&s_framed_overlay_content_style, 0);
    lv_style_set_pad_hor(&s_framed_overlay_content_style, 6);
    lv_style_set_pad_ver(&s_framed_overlay_content_style, 4);
    lv_style_set_pad_row(&s_framed_overlay_content_style, reference_card_gap());
    s_overlay_header_footer_style_ready = true;
}

static lv_obj_t *overlay_header_footer_radius_owner(lv_obj_t *obj)
{
    lv_obj_t *owner = obj ? lv_obj_get_parent(obj) : nullptr;
    if (owner && lv_obj_check_type(owner, &ht_dialog_actions_class))
    {
        lv_obj_t *const panel = lv_obj_get_parent(owner);
        if (panel && lv_obj_check_type(panel, &ht_dialog_panel_class))
        {
            owner = panel;
        }
    }
    return owner;
}

static void applyOverlayHeaderFooterStyle(lv_obj_t *obj)
{
    if (!obj)
    {
        return;
    }
    initOverlayHeaderFooterStyle();
    lv_obj_add_style(obj, &s_overlay_header_footer_style, LV_PART_MAIN);

    lv_obj_t *const dialog_parent = lv_obj_get_parent(obj);
    const bool dialog_header =
        dialog_parent && lv_obj_check_type(dialog_parent, &ht_dialog_panel_class);
    if (dialog_header)
    {
        lv_obj_set_style_height(obj, LV_SIZE_CONTENT, LV_PART_MAIN);
        lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN);
    }

#if LV_USE_THEME_MONO
    // E-paper footer hints also size to their content and use the inverse
    // paper-safe palette (white surface with black text).
    const bool dialog_footer_hint =
        dialog_parent && lv_obj_check_type(dialog_parent, &ht_dialog_actions_class);
    if (dialog_footer_hint)
    {
        lv_obj_set_style_height(obj, LV_SIZE_CONTENT, LV_PART_MAIN);
        lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN);
        lv_obj_set_style_bg_color(obj, lv_color_hex(themePalette().highlight_bg), LV_PART_MAIN);
        lv_obj_set_style_text_color(obj, lv_color_hex(themePalette().highlight_fg), LV_PART_MAIN);
    }
#endif

    lv_obj_t *const owner = overlay_header_footer_radius_owner(obj);
    const lv_coord_t radius = owner ? lv_obj_get_style_radius(owner, LV_PART_MAIN) : 0;
    lv_obj_set_style_radius(obj, radius, LV_PART_MAIN);
    lv_obj_set_style_clip_corner(obj, radius > 0, LV_PART_MAIN);
}

static lv_coord_t reference_card_gap()
{
#if HELTEC_UI_HAS_TOUCH
    // The reference theme inherits PAD_SMALL from LVGL's small-display card style.
    return LV_DPX(10);
#else
    return 3;
#endif
}

static bool isCompactDisplay()
{
    lv_disp_t *const display = lv_disp_get_default();
    return display && lv_disp_get_ver_res(display) <= 80;
}

#if !HELTEC_UI_HAS_TOUCH
static void initClassicContextStyles(lv_obj_t *obj)
{
    if (s_ctx_styles_ready)
        return;
    (void)obj;
    const UiContextMenuStyleConfig &config = themeProfile().context_menu;
    const lv_coord_t icon_gap_px = config.icon_gap;
    const lv_coord_t border_width_px = kContextMenuBorderWidth;
    const lv_coord_t title_border_width_px = kContextMenuTitleBorderWidth;
    const lv_coord_t button_side = config.icon_size;

    lv_style_init(&s_ctx_menu_style);
    lv_style_set_border_width(&s_ctx_menu_style, 0);
    lv_style_set_outline_width(&s_ctx_menu_style, 0);
    lv_style_set_bg_opa(&s_ctx_menu_style, LV_OPA_COVER);
    lv_style_set_bg_color(&s_ctx_menu_style, lv_color_hex(themePalette().bg));
    lv_style_set_text_color(&s_ctx_menu_style, lv_color_hex(themePalette().fg));
    lv_style_set_pad_all(&s_ctx_menu_style, 0);
    lv_style_set_pad_row(&s_ctx_menu_style, reference_card_gap());

    lv_style_init(&s_ctx_header_style);
    lv_style_set_bg_opa(&s_ctx_header_style, LV_OPA_COVER);
    lv_style_set_bg_color(&s_ctx_header_style, lv_color_hex(themePalette().fg));
    lv_style_set_text_color(&s_ctx_header_style, lv_color_hex(themePalette().bg));
    lv_style_set_border_width(&s_ctx_header_style, 0);
    lv_style_set_outline_width(&s_ctx_header_style, 0);
    lv_style_set_pad_all(&s_ctx_header_style, 0);

    lv_style_init(&s_ctx_header_row_style);
    lv_style_set_bg_opa(&s_ctx_header_row_style, LV_OPA_TRANSP);
    lv_style_set_border_width(&s_ctx_header_row_style, 0);
    lv_style_set_outline_width(&s_ctx_header_row_style, 0);
    lv_style_set_shadow_width(&s_ctx_header_row_style, 0);
    lv_style_set_radius(&s_ctx_header_row_style, 0);
    lv_style_set_pad_all(&s_ctx_header_row_style, 0);

    lv_style_init(&s_ctx_icon_row_layout_style);
    lv_style_set_width(&s_ctx_icon_row_layout_style, lv_pct(100));
    lv_style_set_height(&s_ctx_icon_row_layout_style, button_side);
    lv_style_set_pad_all(&s_ctx_icon_row_layout_style, 0);
    lv_style_set_pad_column(&s_ctx_icon_row_layout_style, icon_gap_px);

    lv_style_init(&s_ctx_title_style);
    lv_style_set_text_align(&s_ctx_title_style, LV_TEXT_ALIGN_CENTER);
    setOverlayTextSpacing(&s_ctx_title_style);
    lv_style_set_bg_opa(&s_ctx_title_style, LV_OPA_COVER);
    lv_style_set_bg_color(&s_ctx_title_style, lv_color_hex(themePalette().fg));
    lv_style_set_text_color(&s_ctx_title_style, lv_color_hex(themePalette().bg));
    lv_style_set_outline_width(&s_ctx_title_style, 0);
    lv_style_set_shadow_width(&s_ctx_title_style, 0);
    lv_style_set_radius(&s_ctx_title_style, 0);
    lv_style_set_pad_all(&s_ctx_title_style, 0);
    if (title_border_width_px > 0)
    {
        lv_style_set_border_width(&s_ctx_title_style, title_border_width_px);
        lv_style_set_border_side(&s_ctx_title_style, LV_BORDER_SIDE_FULL);
        lv_style_set_border_color(&s_ctx_title_style, lv_color_hex(themePalette().bg));
        lv_style_set_border_opa(&s_ctx_title_style, LV_OPA_COVER);
    }

    lv_style_init(&s_ctx_icon_btn_main_style);
    lv_style_set_width(&s_ctx_icon_btn_main_style, button_side);
    lv_style_set_height(&s_ctx_icon_btn_main_style, button_side);
    lv_style_set_pad_all(&s_ctx_icon_btn_main_style, 0);
    lv_style_set_bg_opa(&s_ctx_icon_btn_main_style, LV_OPA_TRANSP);
    lv_style_set_border_width(&s_ctx_icon_btn_main_style, 0);
    lv_style_set_outline_width(&s_ctx_icon_btn_main_style, 0);
    lv_style_set_radius(&s_ctx_icon_btn_main_style, 0);
    lv_style_set_shadow_width(&s_ctx_icon_btn_main_style, 0);
    lv_style_set_img_recolor_opa(&s_ctx_icon_btn_main_style, LV_OPA_TRANSP);

    lv_style_init(&s_ctx_icon_btn_checked_style);
    lv_style_set_pad_all(&s_ctx_icon_btn_checked_style, 0);
#if defined(HELTEC_HAS_KEY_FRAME) && HELTEC_HAS_KEY_FRAME
    // E213 normally has a single context pane. Keep its selected icon as a
    // white glyph on the black header instead of drawing a solid white tile.
    lv_style_set_bg_opa(&s_ctx_icon_btn_checked_style, LV_OPA_TRANSP);
#else
    lv_style_set_bg_color(&s_ctx_icon_btn_checked_style, lv_color_hex(themePalette().bg));
    lv_style_set_bg_opa(&s_ctx_icon_btn_checked_style, LV_OPA_COVER);
#endif
    lv_style_set_border_width(&s_ctx_icon_btn_checked_style, 0);
    lv_style_set_border_opa(&s_ctx_icon_btn_checked_style, LV_OPA_TRANSP);
    lv_style_set_border_side(&s_ctx_icon_btn_checked_style, LV_BORDER_SIDE_NONE);
    lv_style_set_outline_width(&s_ctx_icon_btn_checked_style, 0);
    lv_style_set_radius(&s_ctx_icon_btn_checked_style, 0);
    lv_style_set_img_recolor_opa(&s_ctx_icon_btn_checked_style, LV_OPA_TRANSP);

    lv_style_init(&s_ctx_icon_style);
    lv_style_set_width(&s_ctx_icon_style, config.icon_size);
    lv_style_set_height(&s_ctx_icon_style, config.icon_size);
    lv_style_set_bg_opa(&s_ctx_icon_style, LV_OPA_TRANSP);
    lv_style_set_img_recolor(&s_ctx_icon_style, lv_color_hex(themePalette().bg));
    lv_style_set_img_recolor_opa(&s_ctx_icon_style, LV_OPA_COVER);

    lv_style_init(&s_ctx_icon_checked_style);
    lv_style_set_width(&s_ctx_icon_checked_style, config.icon_size);
    lv_style_set_height(&s_ctx_icon_checked_style, config.icon_size);
#if defined(HELTEC_HAS_KEY_FRAME) && HELTEC_HAS_KEY_FRAME
    lv_style_set_img_recolor(&s_ctx_icon_checked_style, lv_color_hex(themePalette().bg));
#else
    lv_style_set_img_recolor(&s_ctx_icon_checked_style, lv_color_hex(themePalette().fg));
#endif
    lv_style_set_img_recolor_opa(&s_ctx_icon_checked_style, LV_OPA_COVER);

    s_ctx_styles_ready = true;
}

#endif

static void initSendMessageStyles()
{
    if (s_send_styles_ready)
        return;

    lv_disp_t *const display = lv_disp_get_default();
    const bool wide_display = display && lv_disp_get_hor_res(display) >= 200;
    const lv_coord_t row_radius = LV_COLOR_DEPTH == 1 || wide_display ? 0 : 3;

    initOverlayCenterLabelStyle();

    lv_style_init(&s_send_root_layout_style);
    const lv_coord_t root_radius = themeProfile().app_frame.content_radius > 0
                                       ? themeProfile().app_frame.content_radius
                                       : themePalette().widget_radius_px;
    lv_style_set_radius(&s_send_root_layout_style, root_radius);
    lv_style_set_clip_corner(&s_send_root_layout_style, root_radius > 0);
    lv_style_set_pad_all(&s_send_root_layout_style, 0);
    lv_style_set_pad_row(&s_send_root_layout_style, 0);

#if !HELTEC_UI_HAS_TOUCH
    lv_style_init(&s_send_content_style);
    lv_style_set_bg_opa(&s_send_content_style, LV_OPA_COVER);
    lv_style_set_bg_color(&s_send_content_style, lv_color_hex(themePalette().bg));
#endif

    lv_style_init(&s_send_list_style);
    lv_style_set_bg_opa(&s_send_list_style, LV_OPA_COVER);
#if LV_USE_THEME_MONO
    lv_style_set_bg_color(&s_send_list_style, lv_color_hex(themePalette().bg));
#else
    lv_style_set_bg_color(&s_send_list_style, lv_color_hex(themePalette().overlay_bg));
#endif
    lv_style_set_border_width(&s_send_list_style, 0);
    lv_style_set_pad_all(&s_send_list_style, 0);
    lv_style_set_pad_row(&s_send_list_style, 0);

#if HELTEC_UI_HAS_TOUCH
    lv_style_init(&s_send_touch_list_style);
    lv_style_set_bg_opa(&s_send_touch_list_style, LV_OPA_TRANSP);
    lv_style_set_border_width(&s_send_touch_list_style, 0);
    lv_style_set_pad_all(&s_send_touch_list_style, 0);
    lv_style_set_pad_row(&s_send_touch_list_style, 0);
#endif

    lv_style_init(&s_send_row_main_style);
    lv_style_set_bg_opa(&s_send_row_main_style, LV_OPA_TRANSP);
    lv_style_set_radius(&s_send_row_main_style, row_radius);
    lv_style_set_shadow_width(&s_send_row_main_style, 0);
    lv_style_set_border_width(&s_send_row_main_style, 0);
    lv_style_set_outline_width(&s_send_row_main_style, 0);
#if !LV_USE_THEME_MONO
    lv_style_set_text_color(&s_send_row_main_style, lv_color_hex(themePalette().overlay_fg));
#endif
    lv_style_set_text_align(&s_send_row_main_style, LV_TEXT_ALIGN_CENTER);
    setOverlayTextSpacing(&s_send_row_main_style);
    lv_style_set_pad_all(&s_send_row_main_style, 0);

    lv_style_init(&s_send_row_active_style);
    lv_style_set_bg_opa(&s_send_row_active_style, LV_OPA_COVER);
    lv_style_set_bg_color(&s_send_row_active_style, lv_color_hex(themePalette().highlight_bg));
    lv_style_set_text_color(&s_send_row_active_style, lv_color_hex(themePalette().highlight_fg));
    lv_style_set_shadow_width(&s_send_row_active_style, 0);
    lv_style_set_border_width(&s_send_row_active_style, 0);
    lv_style_set_outline_width(&s_send_row_active_style, 0);

    s_send_styles_ready = true;
}

static void initRadioSyncStyles()
{
    if (s_radio_sync_styles_ready)
        return;

    initOverlayCenterLabelStyle();
    const lv_coord_t gap =
#if HELTEC_UI_HAS_TOUCH
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

#if HELTEC_UI_HAS_TOUCH
    lv_style_init(&s_overlay_roller_style);
    lv_style_set_pad_all(&s_overlay_roller_style, 0);
    lv_style_set_bg_opa(&s_overlay_roller_style, LV_OPA_TRANSP);
    lv_style_set_border_width(&s_overlay_roller_style, 0);
    lv_style_set_outline_width(&s_overlay_roller_style, 0);
    lv_style_set_shadow_width(&s_overlay_roller_style, 0);
    lv_style_set_text_color(&s_overlay_roller_style, lv_color_hex(themePalette().overlay_fg));
    lv_style_set_text_align(&s_overlay_roller_style, LV_TEXT_ALIGN_CENTER);
    lv_style_set_text_line_space(&s_overlay_roller_style, LV_DPX(18));
    lv_style_set_anim_time(&s_overlay_roller_style, 220);

    lv_style_init(&s_overlay_roller_selected_style);
    lv_style_set_bg_color(&s_overlay_roller_selected_style, lv_color_hex(themePalette().highlight_bg));
    lv_style_set_bg_opa(&s_overlay_roller_selected_style, LV_OPA_COVER);
    lv_style_set_text_color(&s_overlay_roller_selected_style, lv_color_hex(themePalette().highlight_fg));
    lv_style_set_radius(&s_overlay_roller_selected_style, LV_DPX(4));
#else
    lv_style_init(&s_radio_sync_row_main_style);
    lv_style_set_text_align(&s_radio_sync_row_main_style, LV_TEXT_ALIGN_CENTER);
    setOverlayTextSpacing(&s_radio_sync_row_main_style);
    lv_style_set_bg_opa(&s_radio_sync_row_main_style, LV_OPA_TRANSP);

    lv_style_init(&s_radio_sync_row_checked_style);
    lv_style_set_text_color(&s_radio_sync_row_checked_style, lv_color_hex(themePalette().highlight_fg));
    lv_style_set_bg_opa(&s_radio_sync_row_checked_style, LV_OPA_COVER);
    lv_style_set_bg_color(&s_radio_sync_row_checked_style, lv_color_hex(themePalette().highlight_bg));
#endif

    s_radio_sync_styles_ready = true;
}

static void initRepeatModeStyles()
{
    if (s_repeat_styles_ready)
        return;
    initRadioSyncStyles();

    lv_style_init(&s_repeat_root_layout_style);
    lv_style_set_pad_hor(&s_repeat_root_layout_style, 6);
    lv_style_set_pad_ver(&s_repeat_root_layout_style, 4);
    lv_style_set_pad_row(&s_repeat_root_layout_style, LV_DPX(4));

    lv_style_init(&s_repeat_list_style);
    lv_style_set_pad_all(&s_repeat_list_style, 0);
    lv_style_set_bg_opa(&s_repeat_list_style, LV_OPA_TRANSP);
    lv_style_set_border_width(&s_repeat_list_style, 0);

#if !HELTEC_UI_HAS_TOUCH
    lv_style_init(&s_repeat_item_style);
    lv_style_set_bg_opa(&s_repeat_item_style, LV_OPA_TRANSP);
    lv_style_set_border_width(&s_repeat_item_style, 0);

    lv_style_init(&s_repeat_item_selected_style);
    lv_style_set_bg_color(&s_repeat_item_selected_style, lv_color_hex(themePalette().highlight_bg));
    lv_style_set_bg_opa(&s_repeat_item_selected_style, LV_OPA_COVER);
    lv_style_set_text_color(&s_repeat_item_selected_style, lv_color_hex(themePalette().highlight_fg));

#endif

    s_repeat_styles_ready = true;
}

static void initKeyboardChildStyles()
{
    if (s_keyboard_child_styles_ready)
        return;

    initOverlayCenterLabelStyle();
    lv_disp_t *const disp = lv_disp_get_default();
    const lv_coord_t vertical_resolution = disp ? lv_disp_get_ver_res(disp) : 0;
    const bool compact = vertical_resolution > 0 && vertical_resolution <= 80;
    const bool short_display = vertical_resolution > 80 && vertical_resolution <= 160;
    lv_style_init(&s_keyboard_root_layout_style);
    lv_style_set_flex_main_place(&s_keyboard_root_layout_style, LV_FLEX_ALIGN_START);
    lv_style_set_flex_cross_place(&s_keyboard_root_layout_style, LV_FLEX_ALIGN_CENTER);
    lv_style_set_flex_track_place(&s_keyboard_root_layout_style, LV_FLEX_ALIGN_START);
    lv_style_set_pad_all(&s_keyboard_root_layout_style, 0);
    lv_style_set_pad_row(&s_keyboard_root_layout_style, 0);

    lv_style_init(&s_keyboard_surface_color_style);
    lv_style_set_bg_opa(&s_keyboard_surface_color_style, LV_OPA_COVER);
    lv_style_set_bg_color(&s_keyboard_surface_color_style, lv_color_hex(themePalette().bg));
    lv_style_set_text_color(&s_keyboard_surface_color_style, lv_color_hex(themePalette().fg));

#if HELTEC_UI_HAS_TOUCH
    lv_style_init(&s_keyboard_waypoint_surface_style);
    lv_style_set_bg_opa(&s_keyboard_waypoint_surface_style, LV_OPA_COVER);
    lv_style_set_bg_color(&s_keyboard_waypoint_surface_style, lv_color_white());
    lv_style_set_text_color(&s_keyboard_waypoint_surface_style, lv_color_black());
#endif

    lv_style_init(&s_keyboard_root_waypoint_layout_style);
    lv_style_set_flex_main_place(&s_keyboard_root_waypoint_layout_style,
                                 LV_FLEX_ALIGN_SPACE_BETWEEN);
    lv_style_set_flex_cross_place(&s_keyboard_root_waypoint_layout_style, LV_FLEX_ALIGN_CENTER);
    lv_style_set_flex_track_place(&s_keyboard_root_waypoint_layout_style, LV_FLEX_ALIGN_START);
    lv_style_set_pad_ver(&s_keyboard_root_waypoint_layout_style, compact ? 1 : 2);
    lv_style_set_pad_row(&s_keyboard_root_waypoint_layout_style, compact ? 1 : 2);

    lv_style_init(&s_keyboard_textarea_style);
#if !LV_USE_THEME_MONO
    lv_style_set_text_color(&s_keyboard_textarea_style, lv_color_hex(themePalette().overlay_fg));
#endif
    lv_style_set_bg_opa(&s_keyboard_textarea_style, LV_OPA_TRANSP);
#if LV_USE_THEME_MONO
    lv_style_set_border_color(&s_keyboard_textarea_style, lv_color_hex(themePalette().fg));
#else
    lv_style_set_border_color(&s_keyboard_textarea_style, lv_color_hex(themePalette().overlay_fg));
#endif
    lv_style_set_border_width(&s_keyboard_textarea_style, 1);
    lv_style_set_text_align(&s_keyboard_textarea_style, LV_TEXT_ALIGN_LEFT);
    setOverlayTextSpacing(&s_keyboard_textarea_style);
    lv_style_set_pad_all(&s_keyboard_textarea_style, 1);
    lv_style_set_text_font(&s_keyboard_textarea_style, LV_FONT_DEFAULT);
    lv_style_set_width(&s_keyboard_textarea_style, lv_pct(100));
    lv_style_set_height(&s_keyboard_textarea_style, 0);
    lv_style_set_flex_grow(&s_keyboard_textarea_style, 1);

    lv_style_init(&s_keyboard_waypoint_textarea_style);
#if defined(HELTEC_UI_HAS_TOUCH) && HELTEC_UI_HAS_TOUCH
    const lv_coord_t waypoint_editor_height = compact ? 16 : (short_display ? 20 : 28);
#else
    const lv_coord_t waypoint_editor_height = compact ? 16 : (short_display ? 20 : 32);
#endif

    lv_style_init(&s_keyboard_active_textarea_style);
#if defined(HELTEC_UI_HAS_TOUCH) && HELTEC_UI_HAS_TOUCH
    lv_style_set_border_width(&s_keyboard_active_textarea_style, 2);
    lv_style_set_border_color(&s_keyboard_active_textarea_style, lv_color_hex(themePalette().accent));
    lv_style_set_border_opa(&s_keyboard_active_textarea_style, LV_OPA_COVER);
    lv_style_set_border_side(&s_keyboard_active_textarea_style, LV_BORDER_SIDE_FULL);
#endif

    lv_style_init(&s_keyboard_title_style);
    lv_style_set_bg_opa(&s_keyboard_title_style, LV_OPA_COVER);
#if defined(HELTEC_UI_HAS_TOUCH) && HELTEC_UI_HAS_TOUCH
    lv_style_set_text_align(&s_keyboard_title_style, LV_TEXT_ALIGN_CENTER);
    lv_style_set_pad_ver(&s_keyboard_title_style, LV_DPX(6));
#endif
#if LV_USE_THEME_MONO
    lv_style_set_bg_color(&s_keyboard_title_style, lv_color_hex(themePalette().bg));
#else
    lv_style_set_bg_color(&s_keyboard_title_style, lv_color_hex(themePalette().highlight_bg));
    lv_style_set_text_color(&s_keyboard_title_style, lv_color_hex(themePalette().highlight_fg));
#endif

    lv_style_init(&s_keyboard_waypoint_title_style);
#if defined(HELTEC_UI_HAS_TOUCH) && HELTEC_UI_HAS_TOUCH
    lv_style_set_bg_color(&s_keyboard_waypoint_title_style, lv_color_hex(themePalette().accent));
    lv_style_set_bg_opa(&s_keyboard_waypoint_title_style, LV_OPA_COVER);
    lv_style_set_text_color(&s_keyboard_waypoint_title_style, lv_color_white());
#else
    lv_style_set_bg_opa(&s_keyboard_waypoint_title_style, LV_OPA_TRANSP);
#if LV_USE_THEME_MONO
    lv_style_set_text_color(&s_keyboard_waypoint_title_style, lv_color_hex(themePalette().fg));
#else
    lv_style_set_text_color(&s_keyboard_waypoint_title_style, lv_color_white());
#endif
#endif

#if HELTEC_UI_HAS_TOUCH
    lv_style_init(&s_coordinate_row_style);
    lv_style_set_bg_opa(&s_coordinate_row_style, LV_OPA_TRANSP);
    lv_style_set_border_width(&s_coordinate_row_style, 0);
    lv_style_set_pad_all(&s_coordinate_row_style, 0);
    lv_style_set_pad_column(&s_coordinate_row_style, LV_DPX(8));

    lv_style_init(&s_coordinate_label_style);
    lv_style_set_text_color(&s_coordinate_label_style, lv_color_black());
    lv_style_set_text_align(&s_coordinate_label_style, LV_TEXT_ALIGN_LEFT);
#endif
    lv_style_set_height(&s_keyboard_waypoint_textarea_style,
                        waypoint_editor_height);
    lv_style_set_min_height(&s_keyboard_waypoint_textarea_style,
                            waypoint_editor_height);
    lv_style_set_max_height(&s_keyboard_waypoint_textarea_style,
                            waypoint_editor_height);
#if !defined(HELTEC_UI_HAS_TOUCH) || !HELTEC_UI_HAS_TOUCH
    lv_style_set_flex_grow(&s_keyboard_waypoint_textarea_style, 0);
#endif
#if defined(HELTEC_UI_HAS_TOUCH) && HELTEC_UI_HAS_TOUCH
    lv_style_set_bg_opa(&s_keyboard_waypoint_textarea_style, LV_OPA_COVER);
    lv_style_set_bg_color(&s_keyboard_waypoint_textarea_style, lv_color_white());
    lv_style_set_text_color(&s_keyboard_waypoint_textarea_style, lv_color_black());
    lv_style_set_border_color(&s_keyboard_waypoint_textarea_style,
                              lv_color_hex(0x7F8C99));
    lv_style_set_border_opa(&s_keyboard_waypoint_textarea_style, LV_OPA_COVER);
    lv_style_set_border_side(&s_keyboard_waypoint_textarea_style, LV_BORDER_SIDE_FULL);
    lv_style_set_border_width(&s_keyboard_waypoint_textarea_style, 1);
    lv_style_set_radius(&s_keyboard_waypoint_textarea_style, 4);
    lv_style_set_pad_hor(&s_keyboard_waypoint_textarea_style, 6);
    lv_style_set_pad_ver(&s_keyboard_waypoint_textarea_style, 2);
#endif

    lv_style_init(&s_keyboard_main_style);
    lv_style_set_pad_all(&s_keyboard_main_style, 0);
    lv_style_set_pad_row(&s_keyboard_main_style, 0);
    lv_style_set_pad_column(&s_keyboard_main_style, 1);
    lv_style_set_border_width(&s_keyboard_main_style, 0);
    lv_style_set_outline_width(&s_keyboard_main_style, 0);
    lv_style_set_shadow_width(&s_keyboard_main_style, 0);
#if LV_USE_THEME_MONO
    lv_style_set_bg_opa(&s_keyboard_main_style, LV_OPA_COVER);
    lv_style_set_bg_color(&s_keyboard_main_style, lv_color_hex(themePalette().bg));
#endif
    lv_style_set_flex_grow(&s_keyboard_main_style, 0);
    lv_style_set_width(&s_keyboard_main_style, lv_pct(100));
    lv_style_set_height(&s_keyboard_main_style, compact ? 36 : lv_pct(50));
    lv_style_set_min_height(&s_keyboard_main_style, compact ? 36 : 40);

    lv_style_init(&s_keyboard_waypoint_main_style);
#if defined(HELTEC_UI_HAS_TOUCH) && HELTEC_UI_HAS_TOUCH
    lv_style_set_flex_grow(&s_keyboard_waypoint_main_style, 1);
    lv_style_set_height(&s_keyboard_waypoint_main_style, 0);
    lv_style_set_min_height(&s_keyboard_waypoint_main_style, 132);
    lv_style_set_pad_top(&s_keyboard_waypoint_main_style, LV_DPX(8));
#else
    lv_style_set_flex_grow(&s_keyboard_waypoint_main_style, 0);
    lv_style_set_width(&s_keyboard_waypoint_main_style, lv_pct(100));
    const lv_coord_t waypoint_keyboard_height = compact ? 40 : (short_display ? 60 : 132);
    lv_style_set_height(&s_keyboard_waypoint_main_style, waypoint_keyboard_height);
    lv_style_set_min_height(&s_keyboard_waypoint_main_style, waypoint_keyboard_height);
#endif
    lv_style_set_width(&s_keyboard_waypoint_main_style, lv_pct(100));
#if defined(HELTEC_UI_HAS_TOUCH) && HELTEC_UI_HAS_TOUCH
    lv_style_set_bg_opa(&s_keyboard_waypoint_main_style, LV_OPA_COVER);
    lv_style_set_bg_color(&s_keyboard_waypoint_main_style, lv_color_white());
#endif

    lv_style_init(&s_keyboard_items_base_style);
#if defined(HELTEC_UI_HAS_TOUCH) && HELTEC_UI_HAS_TOUCH
    lv_style_set_pad_all(&s_keyboard_items_base_style, 0);
    lv_style_set_bg_color(&s_keyboard_items_base_style, lv_color_hex(0xF6F8FA));
    lv_style_set_bg_opa(&s_keyboard_items_base_style, LV_OPA_COVER);
    lv_style_set_text_color(&s_keyboard_items_base_style, lv_color_black());
#if defined(LV_FONT_MONTSERRAT_14) && LV_FONT_MONTSERRAT_14
    lv_style_set_text_font(&s_keyboard_items_base_style, &lv_font_montserrat_14);
#endif
    lv_style_set_border_width(&s_keyboard_items_base_style, 1);
    lv_style_set_border_color(&s_keyboard_items_base_style, lv_color_hex(0xCBD5DF));
    lv_style_set_radius(&s_keyboard_items_base_style, 4);
#else
    lv_style_set_pad_all(&s_keyboard_items_base_style, compact ? 0 : 1);
    lv_style_set_border_width(&s_keyboard_items_base_style, 0);
    lv_style_set_outline_width(&s_keyboard_items_base_style, 0);
    lv_style_set_shadow_width(&s_keyboard_items_base_style, 0);
    lv_style_set_bg_opa(&s_keyboard_items_base_style, LV_OPA_TRANSP);
#if !LV_USE_THEME_MONO
    lv_style_set_text_color(&s_keyboard_items_base_style, lv_color_hex(themePalette().overlay_fg));
#endif
#endif

    lv_style_init(&s_keyboard_items_selected_style);
#if defined(HELTEC_UI_HAS_TOUCH) && HELTEC_UI_HAS_TOUCH
    lv_style_set_bg_color(&s_keyboard_items_selected_style, lv_color_hex(0xBDE1FF));
    lv_style_set_bg_opa(&s_keyboard_items_selected_style, LV_OPA_COVER);
    lv_style_set_text_color(&s_keyboard_items_selected_style, lv_color_black());
    lv_style_set_border_width(&s_keyboard_items_selected_style, 1);
    lv_style_set_border_color(&s_keyboard_items_selected_style, lv_color_hex(0x9BBFD8));
    lv_style_set_radius(&s_keyboard_items_selected_style, 4);
#else
    lv_style_set_bg_color(&s_keyboard_items_selected_style, lv_color_hex(themePalette().highlight_bg));
    lv_style_set_bg_opa(&s_keyboard_items_selected_style, LV_OPA_COVER);
    lv_style_set_text_color(&s_keyboard_items_selected_style, lv_color_hex(themePalette().highlight_fg));
    lv_style_set_border_width(&s_keyboard_items_selected_style, 0);
    lv_style_set_outline_width(&s_keyboard_items_selected_style, 0);
    lv_style_set_shadow_width(&s_keyboard_items_selected_style, 0);
#endif

    s_keyboard_child_styles_ready = true;
}

static void initOverlayFeedbackStyles()
{
    if (s_overlay_feedback_styles_ready)
        return;

    lv_style_init(&s_overlay_label_all_states_style);
#if LV_USE_THEME_MONO
    lv_style_set_text_color(&s_overlay_label_all_states_style, lv_color_hex(themePalette().fg));
#else
    lv_style_set_text_color(&s_overlay_label_all_states_style, lv_color_hex(themePalette().overlay_fg));
#endif
    lv_style_set_text_opa(&s_overlay_label_all_states_style, LV_OPA_COVER);
    lv_style_set_bg_opa(&s_overlay_label_all_states_style, LV_OPA_TRANSP);
    lv_style_set_text_align(&s_overlay_label_all_states_style, LV_TEXT_ALIGN_CENTER);
    setOverlayTextSpacing(&s_overlay_label_all_states_style);

    lv_style_init(&s_alert_box_style);
    lv_style_set_bg_color(&s_alert_box_style, lv_color_hex(themePalette().overlay_bg));
    lv_style_set_bg_opa(&s_alert_box_style, LV_OPA_COVER);
    lv_style_set_border_color(&s_alert_box_style, lv_color_hex(themePalette().overlay_fg));
    lv_style_set_border_width(&s_alert_box_style, 1);
    lv_style_set_border_opa(&s_alert_box_style, LV_OPA_COVER);
    lv_style_set_outline_width(&s_alert_box_style, 0);
    lv_style_set_radius(&s_alert_box_style, 3);
    lv_style_set_pad_all(&s_alert_box_style, 8);

    lv_style_init(&s_calibration_panel_style);
    lv_style_set_radius(&s_calibration_panel_style, 0);
    lv_style_set_bg_color(&s_calibration_panel_style, lv_color_hex(themePalette().overlay_bg));
    lv_style_set_bg_opa(&s_calibration_panel_style, LV_OPA_COVER);
    lv_style_set_border_width(&s_calibration_panel_style, 0);
    lv_style_set_outline_width(&s_calibration_panel_style, 0);
    lv_style_set_pad_all(&s_calibration_panel_style, 8);
    lv_style_set_pad_row(&s_calibration_panel_style, reference_card_gap());

    s_overlay_feedback_styles_ready = true;
}

static void initCommonWidgetStyles()
{
    if (s_common_widget_styles_ready)
        return;

    lv_style_init(&s_surface_root_common_style);
    lv_style_set_text_color(&s_surface_root_common_style, lv_color_hex(themePalette().fg));

    lv_style_init(&s_screen_root_layout_style);
    lv_style_set_pad_hor(&s_screen_root_layout_style, kScreenPadding);
    lv_style_set_pad_row(&s_screen_root_layout_style, reference_card_gap());

    lv_style_init(&s_container_layout_style);

#if defined(ENV_INCLUDE_MAP) && ENV_INCLUDE_MAP && defined(HELTEC_UI_HAS_TOUCH) && HELTEC_UI_HAS_TOUCH
    lv_style_init(&s_tracker_root_layout_style);
    lv_style_set_pad_top(&s_tracker_root_layout_style, 1);
    lv_style_set_pad_row(&s_tracker_root_layout_style, 2);
#endif

    lv_style_init(&s_preview_root_layout_style);
    lv_style_set_pad_all(&s_preview_root_layout_style, 4);
    lv_style_set_pad_row(&s_preview_root_layout_style, reference_card_gap());

    lv_style_init(&s_splash_root_layout_style);
    lv_style_set_pad_all(&s_splash_root_layout_style, 8);
    lv_style_set_pad_row(&s_splash_root_layout_style, reference_card_gap());
#if defined(HELTEC_HAS_KEY_FRAME) && HELTEC_HAS_KEY_FRAME
    lv_style_set_bg_color(&s_splash_root_layout_style, lv_color_white());
    lv_style_set_bg_opa(&s_splash_root_layout_style, LV_OPA_COVER);
#elif LV_USE_THEME_MONO
    lv_style_set_bg_color(&s_splash_root_layout_style, lv_color_hex(themePalette().bg));
    lv_style_set_bg_opa(&s_splash_root_layout_style, LV_OPA_COVER);
#elif HELTEC_UI_HAS_TOUCH
    lv_style_set_bg_color(&s_splash_root_layout_style, lv_color_hex(0x001765));
    lv_style_set_bg_opa(&s_splash_root_layout_style, LV_OPA_COVER);
#endif

    lv_style_init(&s_plain_container_style);
    lv_style_set_bg_opa(&s_plain_container_style, LV_OPA_TRANSP);
    lv_style_set_border_width(&s_plain_container_style, 0);
    lv_style_set_pad_all(&s_plain_container_style, 0);

    lv_style_init(&s_no_chrome_style);
    lv_style_set_border_width(&s_no_chrome_style, 0);
    lv_style_set_outline_width(&s_no_chrome_style, 0);
    lv_style_set_shadow_width(&s_no_chrome_style, 0);

    lv_style_init(&s_hidden_scrollbar_style);
    lv_style_set_width(&s_hidden_scrollbar_style, 0);
    lv_style_set_bg_opa(&s_hidden_scrollbar_style, LV_OPA_TRANSP);

    s_common_widget_styles_ready = true;
}

static void initTopPaneStyles(lv_obj_t *obj)
{
    if (s_top_pane_styles_ready)
        return;

    (void)obj;
    const UiTopPaneStyleConfig &geometry = themeProfile().top_pane;
    const lv_coord_t top_radius = themeProfile().app_frame.bar_radius;
    const lv_coord_t bat_w = geometry.battery_width;
    const lv_coord_t bat_h = geometry.battery_height;
    const lv_color_t bar_fg = lv_color_hex(themePalette().top_pane_fg);
#if HELTEC_UI_HAS_TOUCH
    const lv_color_t battery_chrome = lv_color_white();
#else
    const lv_color_t battery_chrome = bar_fg;
#endif
    const int16_t cap_i = (bat_h + 1) / 3;
    const int16_t cap_px = (cap_i < 2 ? 2 : (cap_i > 6 ? 6 : cap_i));
    const lv_coord_t body_w = bat_w - cap_px + 1;
#if HELTEC_UI_HAS_TOUCH
    const lv_coord_t cap_h = LV_MAX(4, static_cast<lv_coord_t>((bat_h * 2 + 2) / 3));
#else
    const lv_coord_t cap_h = LV_MAX(2, bat_h / 2);
#endif
    lv_style_init(&s_top_pane_root_style);
    lv_style_set_bg_opa(&s_top_pane_root_style, effectiveOpacity(themePalette().top_pane_bg_opa));
    lv_style_set_bg_color(&s_top_pane_root_style, lv_color_hex(themePalette().top_pane_bg));
    lv_style_set_text_color(&s_top_pane_root_style, bar_fg);
    lv_style_set_height(&s_top_pane_root_style, themeProfile().app_frame.bar_height);
    lv_style_set_pad_hor(&s_top_pane_root_style, top_radius);
    lv_style_set_pad_top(&s_top_pane_root_style, geometry.pad_top);
    lv_style_set_pad_bottom(&s_top_pane_root_style, 0);
    lv_style_set_radius(&s_top_pane_root_style, top_radius);
    lv_style_set_clip_corner(&s_top_pane_root_style, top_radius > 0);

    lv_style_init(&s_top_pane_battery_style);
    lv_style_set_width(&s_top_pane_battery_style, bat_w);
    lv_style_set_height(&s_top_pane_battery_style, bat_h);

    lv_style_init(&s_top_pane_title_style);
    lv_style_set_text_align(&s_top_pane_title_style, LV_TEXT_ALIGN_CENTER);

    lv_style_init(&s_top_pane_battery_outline_style);
#if HELTEC_UI_HAS_TOUCH
    lv_style_set_pad_all(&s_top_pane_battery_outline_style, 0);
#else
    lv_style_set_pad_all(&s_top_pane_battery_outline_style, 1);
#endif
    lv_style_set_width(&s_top_pane_battery_outline_style, body_w);
    lv_style_set_height(&s_top_pane_battery_outline_style, bat_h);
    lv_style_set_border_width(&s_top_pane_battery_outline_style, 1);
    lv_style_set_border_color(&s_top_pane_battery_outline_style, battery_chrome);

    lv_style_init(&s_top_pane_battery_cap_style);
    lv_style_set_width(&s_top_pane_battery_cap_style, cap_px);
    lv_style_set_height(&s_top_pane_battery_cap_style, cap_h);
    lv_style_set_bg_opa(&s_top_pane_battery_cap_style, LV_OPA_COVER);
    lv_style_set_bg_color(&s_top_pane_battery_cap_style, battery_chrome);

    lv_style_init(&s_top_pane_battery_fill_style);
    lv_style_set_width(&s_top_pane_battery_fill_style, 0);
    lv_style_set_height(&s_top_pane_battery_fill_style, LV_MAX(0, bat_h - 2));

    lv_style_init(&s_top_pane_battery_fill_low_style);
    lv_style_set_bg_opa(&s_top_pane_battery_fill_low_style, LV_OPA_COVER);
    lv_style_set_bg_color(&s_top_pane_battery_fill_low_style,
#if LV_COLOR_DEPTH == 1
                          bar_fg
#else
                          lv_color_hex(themePalette().bat_low)
#endif
    );

    lv_style_init(&s_top_pane_battery_fill_mid_style);
    lv_style_set_bg_opa(&s_top_pane_battery_fill_mid_style, LV_OPA_COVER);
    lv_style_set_bg_color(&s_top_pane_battery_fill_mid_style,
#if LV_COLOR_DEPTH == 1
                          bar_fg
#else
                          lv_color_hex(themePalette().bat_mid)
#endif
    );

    lv_style_init(&s_top_pane_battery_fill_high_style);
    lv_style_set_bg_opa(&s_top_pane_battery_fill_high_style, LV_OPA_COVER);
    lv_style_set_bg_color(&s_top_pane_battery_fill_high_style,
#if LV_COLOR_DEPTH == 1
                          bar_fg
#else
                          lv_color_hex(themePalette().bat_high)
#endif
    );

    s_top_pane_styles_ready = true;
}

static void initSurfaceAppStyles(lv_obj_t *obj = nullptr)
{
    if (s_surface_app_styles_ready)
        return;

    (void)obj;
    const UiAppFrameStyleConfig &frame_config = themeProfile().app_frame;
    const lv_coord_t screen_pad = frame_config.screen_pad;
    const lv_coord_t content_radius = frame_config.content_radius;
#if !HELTEC_UI_HAS_TOUCH
    const UiContextMenuStyleConfig &context_config = themeProfile().context_menu;
#endif

    lv_style_init(&s_alert_overlay_root_style);
    lv_style_set_bg_opa(&s_alert_overlay_root_style, LV_OPA_TRANSP);

    lv_style_init(&s_calibration_overlay_root_style);
    lv_style_set_bg_color(&s_calibration_overlay_root_style, lv_color_hex(themePalette().overlay_bg));
    lv_style_set_bg_opa(&s_calibration_overlay_root_style, LV_OPA_COVER);
    lv_style_set_border_width(&s_calibration_overlay_root_style, 0);

#if !HELTEC_UI_HAS_TOUCH
    const lv_coord_t context_border_w = kContextMenuBorderWidth;

    lv_style_init(&s_context_menu_root_style);
    lv_style_set_bg_opa(&s_context_menu_root_style, LV_OPA_COVER);
    lv_style_set_bg_color(&s_context_menu_root_style, lv_color_hex(themePalette().bg));
    lv_style_set_text_color(&s_context_menu_root_style, lv_color_hex(themePalette().fg));
    lv_style_set_border_width(&s_context_menu_root_style, kContextMenuBorderWidth);
    lv_style_set_border_color(&s_context_menu_root_style, lv_color_hex(themePalette().fg));
    lv_style_set_border_opa(&s_context_menu_root_style, LV_OPA_COVER);
    lv_style_set_outline_width(&s_context_menu_root_style, context_border_w);
    lv_style_set_outline_color(&s_context_menu_root_style, lv_color_hex(themePalette().bg));
    lv_style_set_outline_opa(&s_context_menu_root_style, LV_OPA_COVER);
    lv_style_set_outline_pad(&s_context_menu_root_style, 1);
    lv_style_set_radius(&s_context_menu_root_style, context_config.border_radius);
    lv_style_set_pad_all(&s_context_menu_root_style, 0);
    lv_style_set_pad_row(&s_context_menu_root_style, reference_card_gap());
    const lv_coord_t display_w = lv_disp_get_hor_res(nullptr);
    const lv_coord_t display_h = lv_disp_get_ver_res(nullptr);
    const lv_coord_t frame_w = LV_MAX(1, display_w - 2 * kContextMenuFrameInsetX);
    const lv_coord_t frame_h = LV_MAX(1, display_h - 2 * context_config.frame_inset_y);
    lv_style_set_width(&s_context_menu_root_style, frame_w);
    lv_style_set_height(&s_context_menu_root_style, frame_h);
    lv_style_set_align(&s_context_menu_root_style, LV_ALIGN_TOP_LEFT);
    lv_style_set_x(&s_context_menu_root_style, kContextMenuFrameInsetX);
    lv_style_set_y(&s_context_menu_root_style, context_config.frame_inset_y);
#endif

    lv_style_init(&s_app_frame_layout_style);
    lv_style_set_border_width(&s_app_frame_layout_style, 0);
    lv_style_set_bg_color(&s_app_frame_layout_style, lv_color_hex(themePalette().frame_bg));
    lv_style_set_bg_opa(&s_app_frame_layout_style, effectiveOpacity(themePalette().frame_bg_opa));
    lv_style_set_pad_all(&s_app_frame_layout_style, 0);
    lv_style_set_pad_row(&s_app_frame_layout_style, 0);

    lv_style_init(&s_app_content_style);
    lv_style_set_bg_opa(&s_app_content_style, LV_OPA_TRANSP);
    lv_style_set_border_width(&s_app_content_style, 0);
    lv_style_set_pad_all(&s_app_content_style, 0);
    lv_style_set_pad_hor(&s_app_content_style, frame_config.frame_margin_horizontal);
    lv_style_set_pad_bottom(&s_app_content_style, frame_config.frame_margin_bottom);

    lv_style_init(&s_app_screen_root_style);
    lv_style_set_bg_opa(&s_app_screen_root_style, LV_OPA_TRANSP);
    lv_style_set_border_width(&s_app_screen_root_style, 0);
    lv_style_set_radius(&s_app_screen_root_style, 0);
    lv_style_set_clip_corner(&s_app_screen_root_style, false);
    lv_style_set_pad_all(&s_app_screen_root_style, 0);
#if !HELTEC_UI_HAS_TOUCH
    if (screen_pad > 0 || content_radius > 0)
    {
        lv_style_set_bg_color(&s_app_screen_root_style, lv_color_hex(themePalette().panel_bg));
        lv_style_set_bg_opa(&s_app_screen_root_style, LV_OPA_COVER);
        lv_style_set_radius(&s_app_screen_root_style, content_radius > 0 ? content_radius : 0);
        lv_style_set_clip_corner(&s_app_screen_root_style, content_radius > 0);
    }
#endif
#if HELTEC_UI_HAS_TOUCH
    lv_style_init(&s_app_tileview_style);
    lv_style_set_pad_all(&s_app_tileview_style, 0);
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

    s_surface_app_styles_ready = true;
}

#if HELTEC_UI_HAS_TOUCH
static void initQuickPingStyles()
{
    if (s_quick_ping_styles_ready)
        return;

    initKeyboardChildStyles();
    const lv_color_t pane_bg = lv_color_hex(0xEEF6FF);
    const lv_color_t control_bg = lv_color_hex(0xE8E8E8);

    lv_style_init(&s_quick_ping_root_style);
    lv_style_set_bg_opa(&s_quick_ping_root_style, LV_OPA_COVER);
    lv_style_set_bg_color(&s_quick_ping_root_style, pane_bg);
    lv_style_set_text_color(&s_quick_ping_root_style, lv_color_hex(themePalette().fg));
    lv_style_set_border_width(&s_quick_ping_root_style, 1);
    lv_style_set_border_color(&s_quick_ping_root_style, lv_color_hex(themePalette().fg));
    lv_style_set_border_opa(&s_quick_ping_root_style, LV_OPA_COVER);
    lv_style_set_radius(&s_quick_ping_root_style, 10);
    lv_style_set_clip_corner(&s_quick_ping_root_style, true);
    lv_style_set_pad_all(&s_quick_ping_root_style, 0);

    lv_style_init(&s_quick_ping_backdrop_style);
    lv_style_set_bg_color(&s_quick_ping_backdrop_style, lv_color_hex(0x001765));
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
    lv_style_set_text_align(&s_quick_ping_title_style, LV_TEXT_ALIGN_CENTER);
#if defined(LV_FONT_MONTSERRAT_14) && LV_FONT_MONTSERRAT_14
    lv_style_set_text_font(&s_quick_ping_title_style, &lv_font_montserrat_14);
#endif
    setOverlayTextSpacing(&s_quick_ping_title_style);

    lv_style_init(&s_quick_ping_row_style);
    lv_style_set_bg_color(&s_quick_ping_row_style, lv_color_white());
    lv_style_set_bg_opa(&s_quick_ping_row_style, LV_OPA_COVER);
    lv_style_set_border_width(&s_quick_ping_row_style, 1);
    lv_style_set_border_color(&s_quick_ping_row_style, lv_color_hex(themePalette().fg));
    lv_style_set_border_opa(&s_quick_ping_row_style, LV_OPA_COVER);
    lv_style_set_radius(&s_quick_ping_row_style, 6);
    lv_style_set_clip_corner(&s_quick_ping_row_style, true);
    lv_style_set_pad_all(&s_quick_ping_row_style, 0);

    lv_style_init(&s_quick_ping_row_focus_style);
    lv_style_set_bg_color(&s_quick_ping_row_focus_style, lv_color_hex(0xE2F2FF));
    lv_style_set_bg_opa(&s_quick_ping_row_focus_style, LV_OPA_COVER);
    lv_style_set_border_width(&s_quick_ping_row_focus_style, 1);
    lv_style_set_border_color(&s_quick_ping_row_focus_style, lv_color_hex(themePalette().accent));
    lv_style_set_border_opa(&s_quick_ping_row_focus_style, LV_OPA_COVER);
    lv_style_set_outline_width(&s_quick_ping_row_focus_style, 0);
    lv_style_set_shadow_width(&s_quick_ping_row_focus_style, 0);
    lv_style_set_radius(&s_quick_ping_row_focus_style, 6);

    lv_style_init(&s_quick_ping_label_style);
    lv_style_set_text_align(&s_quick_ping_label_style, LV_TEXT_ALIGN_LEFT);
    setOverlayTextSpacing(&s_quick_ping_label_style);

    lv_style_init(&s_quick_ping_dropdown_style);
    lv_style_set_radius(&s_quick_ping_dropdown_style, 6);
    lv_style_set_pad_hor(&s_quick_ping_dropdown_style, 6);
    // Keep the selected value centered in the fixed-height control.
    lv_style_set_pad_ver(&s_quick_ping_dropdown_style, 6);
    lv_style_set_bg_color(&s_quick_ping_dropdown_style, control_bg);
    lv_style_set_bg_opa(&s_quick_ping_dropdown_style, LV_OPA_COVER);
    lv_style_set_pad_right(&s_quick_ping_dropdown_style, 8);

    lv_style_init(&s_quick_ping_message_dropdown_style);
    lv_style_set_text_align(&s_quick_ping_message_dropdown_style, LV_TEXT_ALIGN_CENTER);
    lv_style_set_pad_ver(&s_quick_ping_message_dropdown_style, 6);
    lv_style_set_bg_color(&s_quick_ping_message_dropdown_style, lv_color_white());
    lv_style_set_bg_opa(&s_quick_ping_message_dropdown_style, LV_OPA_COVER);

    lv_style_init(&s_quick_ping_control_focus_style);
    lv_style_set_bg_color(&s_quick_ping_control_focus_style, lv_color_hex(0xBDE1FF));
    lv_style_set_bg_opa(&s_quick_ping_control_focus_style, LV_OPA_COVER);

    lv_style_init(&s_quick_ping_dropdown_disabled_style);
    lv_style_set_opa(&s_quick_ping_dropdown_disabled_style, effectiveOpacity(LV_OPA_50));

    lv_style_init(&s_quick_ping_dropdown_indicator_style);

    lv_style_init(&s_quick_ping_message_input_style);
    lv_style_set_radius(&s_quick_ping_message_input_style, 6);
    lv_style_set_pad_hor(&s_quick_ping_message_input_style, 6);
    lv_style_set_pad_ver(&s_quick_ping_message_input_style, 0);
    lv_style_set_bg_color(&s_quick_ping_message_input_style, lv_color_white());
    lv_style_set_bg_opa(&s_quick_ping_message_input_style, LV_OPA_TRANSP);

    lv_style_init(&s_quick_ping_message_input_label_editing_style);
    lv_style_set_min_width(&s_quick_ping_message_input_label_editing_style, lv_pct(100));

    lv_style_init(&s_quick_ping_keyboard_style);
    lv_style_set_pad_left(&s_quick_ping_keyboard_style, 4);
    lv_style_set_pad_right(&s_quick_ping_keyboard_style, 4);
    lv_style_set_pad_top(&s_quick_ping_keyboard_style, 48);
    lv_style_set_pad_bottom(&s_quick_ping_keyboard_style, 4);
    lv_style_set_pad_row(&s_quick_ping_keyboard_style, 3);
    lv_style_set_pad_column(&s_quick_ping_keyboard_style, 2);
    lv_style_set_bg_color(&s_quick_ping_keyboard_style, lv_color_white());
    lv_style_set_bg_opa(&s_quick_ping_keyboard_style, LV_OPA_COVER);

    lv_style_init(&s_quick_ping_keyboard_editor_style);
    lv_style_set_bg_color(&s_quick_ping_keyboard_editor_style, lv_color_white());
    lv_style_set_bg_opa(&s_quick_ping_keyboard_editor_style, LV_OPA_COVER);
    lv_style_set_border_width(&s_quick_ping_keyboard_editor_style, 1);
    lv_style_set_border_color(&s_quick_ping_keyboard_editor_style, lv_color_hex(0x9BBFD8));
    lv_style_set_radius(&s_quick_ping_keyboard_editor_style, 5);
    lv_style_set_pad_left(&s_quick_ping_keyboard_editor_style, 7);
    lv_style_set_pad_right(&s_quick_ping_keyboard_editor_style, 48);
#if defined(LV_FONT_MONTSERRAT_14) && LV_FONT_MONTSERRAT_14
    lv_style_set_text_font(&s_quick_ping_keyboard_editor_style, &lv_font_montserrat_14);
#endif
    setOverlayTextSpacing(&s_quick_ping_keyboard_editor_style);

    lv_style_init(&s_quick_ping_keyboard_placeholder_style);
    lv_style_set_text_color(&s_quick_ping_keyboard_placeholder_style, lv_color_hex(0xA8B0B8));
    lv_style_set_text_opa(&s_quick_ping_keyboard_placeholder_style, LV_OPA_COVER);

    lv_style_init(&s_quick_ping_keyboard_counter_style);
    lv_style_set_text_color(&s_quick_ping_keyboard_counter_style, lv_color_hex(0x7F8992));
    lv_style_set_text_align(&s_quick_ping_keyboard_counter_style, LV_TEXT_ALIGN_RIGHT);
#if defined(LV_FONT_MONTSERRAT_12) && LV_FONT_MONTSERRAT_12
    lv_style_set_text_font(&s_quick_ping_keyboard_counter_style, &lv_font_montserrat_12);
#endif
    setOverlayTextSpacing(&s_quick_ping_keyboard_counter_style);

    lv_style_init(&s_quick_ping_keyboard_cursor_style);
    lv_style_set_border_color(&s_quick_ping_keyboard_cursor_style, lv_color_hex(themePalette().fg));
    lv_style_set_border_width(&s_quick_ping_keyboard_cursor_style, 2);
    lv_style_set_border_side(&s_quick_ping_keyboard_cursor_style, LV_BORDER_SIDE_LEFT);
    lv_style_set_pad_left(&s_quick_ping_keyboard_cursor_style, -1);
    lv_style_set_anim_time(&s_quick_ping_keyboard_cursor_style, 400);

    lv_style_init(&s_quick_ping_icon_badge_style);
    lv_style_set_bg_color(&s_quick_ping_icon_badge_style, lv_color_hex(0xBDE1FF));
    lv_style_set_bg_opa(&s_quick_ping_icon_badge_style, LV_OPA_COVER);
    lv_style_set_radius(&s_quick_ping_icon_badge_style, 8);

    s_quick_ping_styles_ready = true;
}
#endif

static void initScreenSystemStyles()
{
    if (s_screen_system_styles_ready)
        return;

    lv_style_init(&s_screen_row_layout_style);
    lv_style_set_pad_all(&s_screen_row_layout_style, 1);

    lv_style_init(&s_system_screen_root_layout_style);
    lv_style_set_pad_hor(&s_system_screen_root_layout_style, kScreenPadding);
    lv_style_set_pad_ver(&s_system_screen_root_layout_style, 0);

    lv_style_init(&s_container_focus_style);
#if HELTEC_UI_HAS_TOUCH
    lv_style_set_bg_color(&s_container_focus_style, lv_color_hex(themePalette().accent));
    lv_style_set_bg_opa(&s_container_focus_style, LV_OPA_30);
#else
    lv_style_set_bg_color(&s_container_focus_style, lv_color_hex(themePalette().highlight_bg));
    lv_style_set_bg_opa(&s_container_focus_style, LV_OPA_COVER);
    lv_style_set_text_color(&s_container_focus_style, lv_color_hex(themePalette().highlight_fg));
#endif
    lv_style_set_border_width(&s_container_focus_style, 0);
    lv_style_set_border_opa(&s_container_focus_style, LV_OPA_TRANSP);
    lv_style_set_outline_width(&s_container_focus_style, 0);
    lv_style_set_outline_opa(&s_container_focus_style, LV_OPA_TRANSP);
    lv_style_set_shadow_width(&s_container_focus_style, 0);
    lv_style_set_shadow_opa(&s_container_focus_style, LV_OPA_TRANSP);
    lv_style_set_radius(&s_container_focus_style, themePalette().widget_radius_px);

    lv_style_init(&s_find_friend_dial_layout_style);
    const lv_coord_t dial_height = isCompactDisplay()
                                       ? 0
                                       : (LV_COLOR_DEPTH == 1 ? LV_DPX(32) : LV_DPX(96));
    lv_style_set_min_height(&s_find_friend_dial_layout_style, dial_height);

    lv_style_init(&s_screen_dropdown_list_layout_style);
    lv_style_set_bg_color(&s_screen_dropdown_list_layout_style, lv_color_hex(themePalette().panel_bg));
    lv_style_set_bg_opa(&s_screen_dropdown_list_layout_style, LV_OPA_COVER);
    lv_style_set_text_color(&s_screen_dropdown_list_layout_style, lv_color_hex(themePalette().fg));
    lv_style_set_text_opa(&s_screen_dropdown_list_layout_style, LV_OPA_COVER);
#if HELTEC_UI_HAS_TOUCH
    lv_style_set_pad_top(&s_screen_dropdown_list_layout_style, 2);
    lv_style_set_pad_bottom(&s_screen_dropdown_list_layout_style, 2);
#endif

    lv_style_init(&s_system_dropdown_style);
#if LV_COLOR_DEPTH == 1
    lv_style_set_pad_top(&s_system_dropdown_style, 1);
    lv_style_set_pad_bottom(&s_system_dropdown_style, 1);
    lv_style_set_pad_left(&s_system_dropdown_style, 1);
    lv_style_set_pad_right(&s_system_dropdown_style, 1);
    lv_style_set_radius(&s_system_dropdown_style, 0);
    lv_style_set_border_width(&s_system_dropdown_style, 1);
    lv_style_set_border_color(&s_system_dropdown_style, lv_color_hex(themePalette().fg));
    lv_style_set_border_opa(&s_system_dropdown_style, LV_OPA_COVER);
#else
#if HELTEC_UI_HAS_TOUCH
    lv_style_set_pad_all(&s_system_dropdown_style, reference_card_gap());
#else
    lv_style_set_pad_hor(&s_system_dropdown_style, 4);
    lv_style_set_pad_ver(&s_system_dropdown_style, 2);
#endif
    lv_style_set_radius(&s_system_dropdown_style, themePalette().widget_radius_px);
    lv_style_set_border_width(&s_system_dropdown_style, 0);
    lv_style_set_border_opa(&s_system_dropdown_style, LV_OPA_TRANSP);
    lv_style_set_border_side(&s_system_dropdown_style, LV_BORDER_SIDE_NONE);
#endif
#if HELTEC_UI_HAS_TOUCH
    // Screen focus is shown on the complete logical row. Keeping a permanent
    // blue dropdown fill makes an unfocused row look selected as well.
    lv_style_set_bg_opa(&s_system_dropdown_style, LV_OPA_TRANSP);
#else
    lv_style_set_bg_color(&s_system_dropdown_style, lv_color_hex(themePalette().panel_bg));
    lv_style_set_bg_opa(&s_system_dropdown_style, LV_OPA_COVER);
#endif
    lv_style_set_text_color(&s_system_dropdown_style, lv_color_hex(themePalette().fg));
    lv_style_set_text_align(&s_system_dropdown_style, LV_TEXT_ALIGN_CENTER);
    lv_style_set_outline_width(&s_system_dropdown_style, 0);
    lv_style_set_outline_opa(&s_system_dropdown_style, LV_OPA_TRANSP);
    lv_style_set_shadow_width(&s_system_dropdown_style, 0);
    lv_style_set_shadow_opa(&s_system_dropdown_style, LV_OPA_TRANSP);

    lv_style_init(&s_system_dropdown_focus_style);
#if HELTEC_UI_HAS_TOUCH
    /*
     * Touch dropdowns use the same filled focus treatment as QuickPing.
     * Keep this in the shared widget theme so waypoint and other screens do
     * not need a local focus-color override.
     */
    lv_style_set_bg_color(&s_system_dropdown_focus_style, lv_color_hex(0xBDE1FF));
    lv_style_set_bg_opa(&s_system_dropdown_focus_style, LV_OPA_COVER);
    lv_style_set_text_color(&s_system_dropdown_focus_style, lv_color_black());
#else
    lv_style_set_bg_color(&s_system_dropdown_focus_style, lv_color_hex(themePalette().highlight_bg));
    lv_style_set_bg_opa(&s_system_dropdown_focus_style, LV_OPA_COVER);
    lv_style_set_text_color(&s_system_dropdown_focus_style, lv_color_hex(themePalette().highlight_fg));
#endif
    lv_style_set_text_opa(&s_system_dropdown_focus_style, LV_OPA_COVER);
#if LV_COLOR_DEPTH == 1
    lv_style_set_border_width(&s_system_dropdown_focus_style, 1);
    lv_style_set_border_color(&s_system_dropdown_focus_style, lv_color_hex(themePalette().fg));
    lv_style_set_border_opa(&s_system_dropdown_focus_style, LV_OPA_COVER);
#else
#if HELTEC_UI_HAS_TOUCH
    lv_style_set_border_width(&s_system_dropdown_focus_style, 1);
    lv_style_set_border_color(&s_system_dropdown_focus_style, lv_color_hex(0x9BBFD8));
    lv_style_set_border_opa(&s_system_dropdown_focus_style, LV_OPA_COVER);
#else
    lv_style_set_border_width(&s_system_dropdown_focus_style, 0);
    lv_style_set_border_opa(&s_system_dropdown_focus_style, LV_OPA_TRANSP);
    lv_style_set_border_side(&s_system_dropdown_focus_style, LV_BORDER_SIDE_NONE);
#endif
#endif
    lv_style_set_outline_width(&s_system_dropdown_focus_style, 0);
    lv_style_set_outline_opa(&s_system_dropdown_focus_style, LV_OPA_TRANSP);
    lv_style_set_shadow_width(&s_system_dropdown_focus_style, 0);
    lv_style_set_shadow_opa(&s_system_dropdown_focus_style, LV_OPA_TRANSP);

    lv_style_init(&s_system_volume_controls_style);
    lv_style_set_pad_all(&s_system_volume_controls_style, 0);
    lv_style_set_pad_hor(&s_system_volume_controls_style, 2);
#if HELTEC_UI_HAS_TOUCH
    lv_style_set_pad_column(&s_system_volume_controls_style, 8);
#else
    lv_style_set_pad_column(&s_system_volume_controls_style, 6);
#endif
    lv_style_set_bg_opa(&s_system_volume_controls_style, LV_OPA_TRANSP);
    lv_style_set_border_width(&s_system_volume_controls_style, 0);

    lv_style_init(&s_system_volume_button_style);
    lv_style_set_pad_all(&s_system_volume_button_style, 0);
    lv_style_set_bg_opa(&s_system_volume_button_style, LV_OPA_TRANSP);
    lv_style_set_border_width(&s_system_volume_button_style, 0);
    lv_style_set_border_opa(&s_system_volume_button_style, LV_OPA_TRANSP);

    lv_style_init(&s_system_volume_button_pressed_style);
    lv_style_set_bg_opa(&s_system_volume_button_pressed_style, LV_OPA_TRANSP);

    lv_style_init(&s_system_volume_button_focus_style);
    lv_style_set_bg_opa(&s_system_volume_button_focus_style, LV_OPA_TRANSP);
    lv_style_set_border_width(&s_system_volume_button_focus_style, 1);
    lv_style_set_border_color(&s_system_volume_button_focus_style,
                              lv_palette_main(LV_PALETTE_GREEN));
    lv_style_set_border_opa(&s_system_volume_button_focus_style, LV_OPA_COVER);
    lv_style_set_border_side(&s_system_volume_button_focus_style, LV_BORDER_SIDE_FULL);

    lv_style_init(&s_system_volume_slider_style);
    lv_style_set_bg_color(&s_system_volume_slider_style,
                          lv_palette_lighten(LV_PALETTE_GREY, 2));
    lv_style_set_bg_opa(&s_system_volume_slider_style, LV_OPA_COVER);
    lv_style_set_border_width(&s_system_volume_slider_style, 1);
    lv_style_set_border_color(&s_system_volume_slider_style,
                              lv_palette_main(LV_PALETTE_GREY));
    lv_style_set_radius(&s_system_volume_slider_style, LV_RADIUS_CIRCLE);

    lv_style_init(&s_system_volume_slider_indicator_style);
    lv_style_set_bg_color(&s_system_volume_slider_indicator_style,
                          lv_palette_main(LV_PALETTE_GREEN));
    lv_style_set_bg_opa(&s_system_volume_slider_indicator_style, LV_OPA_COVER);
    lv_style_set_radius(&s_system_volume_slider_indicator_style, LV_RADIUS_CIRCLE);

    lv_style_init(&s_system_volume_slider_knob_style);
    lv_style_set_bg_color(&s_system_volume_slider_knob_style, lv_color_white());
    lv_style_set_bg_opa(&s_system_volume_slider_knob_style, LV_OPA_COVER);
    lv_style_set_border_width(&s_system_volume_slider_knob_style, 1);
    lv_style_set_border_color(&s_system_volume_slider_knob_style,
                              lv_palette_main(LV_PALETTE_GREY));
    lv_style_set_pad_ver(&s_system_volume_slider_knob_style, 2);
    lv_style_set_transform_width(&s_system_volume_slider_knob_style, -1);

    lv_disp_t *const system_disp = lv_disp_get_default();
    const bool compact_dialog = system_disp && (lv_disp_get_hor_res(system_disp) <= 160 ||
                                                lv_disp_get_ver_res(system_disp) <= 128);

    lv_style_init(&s_confirm_overlay_backdrop_style);
    lv_style_set_bg_color(&s_confirm_overlay_backdrop_style, lv_color_black());
    lv_style_set_bg_opa(&s_confirm_overlay_backdrop_style, LV_OPA_50);
    lv_style_set_border_width(&s_confirm_overlay_backdrop_style, 0);
    lv_style_set_pad_all(&s_confirm_overlay_backdrop_style, 0);

    lv_style_init(&s_dialog_panel_style);
    lv_style_set_width(&s_dialog_panel_style, lv_pct(80));
    lv_style_set_height(&s_dialog_panel_style, LV_SIZE_CONTENT);
    lv_style_set_bg_color(&s_dialog_panel_style, lv_color_hex(themePalette().overlay_bg));
    lv_style_set_bg_opa(&s_dialog_panel_style, LV_OPA_COVER);
    lv_style_set_border_color(&s_dialog_panel_style, lv_color_hex(themePalette().overlay_fg));
    lv_style_set_border_width(&s_dialog_panel_style, 1);
    const lv_coord_t dialog_radius = themeProfile().app_frame.bar_radius;
    lv_style_set_radius(&s_dialog_panel_style, dialog_radius);
    lv_style_set_clip_corner(&s_dialog_panel_style, dialog_radius > 0);
    lv_style_set_pad_all(&s_dialog_panel_style, 0);
    lv_style_set_pad_row(&s_dialog_panel_style, compact_dialog ? 4 : LV_DPX(12));
    lv_style_set_text_color(&s_dialog_panel_style, lv_color_hex(themePalette().overlay_fg));

    lv_style_init(&s_confirm_overlay_text_style);
    lv_style_set_text_align(&s_confirm_overlay_text_style, LV_TEXT_ALIGN_CENTER);
    lv_style_set_pad_hor(&s_confirm_overlay_text_style, compact_dialog ? 4 : LV_DPX(12));

    lv_style_init(&s_confirm_overlay_button_row_style);
    lv_style_set_bg_opa(&s_confirm_overlay_button_row_style, LV_OPA_TRANSP);
    lv_style_set_border_width(&s_confirm_overlay_button_row_style, 0);
    lv_style_set_pad_all(&s_confirm_overlay_button_row_style, 0);
    lv_style_set_pad_column(&s_confirm_overlay_button_row_style, compact_dialog ? 4 : LV_DPX(10));
#if defined(HELTEC_UI_HAS_TOUCH) && HELTEC_UI_HAS_TOUCH
    lv_style_set_pad_hor(&s_confirm_overlay_button_row_style,
                         compact_dialog ? 4 : LV_DPX(12));
    lv_style_set_pad_bottom(&s_confirm_overlay_button_row_style,
                            compact_dialog ? 4 : LV_DPX(12));
#endif

    lv_style_init(&s_confirm_overlay_button_style);
    lv_style_set_bg_color(&s_confirm_overlay_button_style, lv_color_hex(themePalette().panel_bg));
    lv_style_set_bg_opa(&s_confirm_overlay_button_style, LV_OPA_COVER);
    lv_style_set_border_color(&s_confirm_overlay_button_style, lv_color_hex(themePalette().panel_border));
    lv_style_set_border_width(&s_confirm_overlay_button_style, 1);
    lv_style_set_radius(&s_confirm_overlay_button_style, compact_dialog ? 1 : LV_DPX(5));
    lv_style_set_pad_all(&s_confirm_overlay_button_style, 0);

    lv_style_init(&s_confirm_overlay_button_pressed_style);
    lv_style_set_bg_color(&s_confirm_overlay_button_pressed_style, lv_color_hex(themePalette().highlight_bg));
    lv_style_set_text_color(&s_confirm_overlay_button_pressed_style, lv_color_hex(themePalette().highlight_fg));

    s_screen_system_styles_ready = true;
}

static void initNavigationStyles(lv_obj_t *obj = nullptr)
{
    if (s_navigation_styles_ready)
        return;
    (void)obj;
    const NavigationStyleConfig &config = kNavigationStyleConfig;
    const lv_coord_t panel_radius = themeProfile().app_frame.content_radius;

#if HELTEC_UI_HAS_TOUCH
    lv_style_init(&s_nav_content_host_style);
    lv_style_set_pad_all(&s_nav_content_host_style, 0);

    const lv_coord_t nav_radius = config.grid_cell_radius;
    const lv_coord_t title_radius = panel_radius;
    lv_style_init(&s_nav_grid_cell_style);
    lv_style_set_bg_color(&s_nav_grid_cell_style, lv_color_white());
    lv_style_set_bg_opa(&s_nav_grid_cell_style, LV_OPA_COVER);
    lv_style_set_border_color(&s_nav_grid_cell_style, lv_color_hex(themePalette().panel_border));
    lv_style_set_border_width(&s_nav_grid_cell_style, 1);
    lv_style_set_radius(&s_nav_grid_cell_style, nav_radius);

    lv_style_init(&s_nav_grid_cell_focus_style);
    lv_style_set_border_width(&s_nav_grid_cell_focus_style, 2);

    lv_style_init(&s_nav_grid_title_bar_style);
    lv_style_set_width(&s_nav_grid_title_bar_style, lv_pct(100));
    lv_style_set_height(&s_nav_grid_title_bar_style, config.grid_label_height);
    lv_style_set_pad_hor(&s_nav_grid_title_bar_style, 2);
    lv_style_set_bg_color(&s_nav_grid_title_bar_style, lv_color_hex(themePalette().accent));
    lv_style_set_bg_opa(&s_nav_grid_title_bar_style, LV_OPA_COVER);
    lv_style_set_border_width(&s_nav_grid_title_bar_style, 1);
    lv_style_set_border_color(&s_nav_grid_title_bar_style, lv_color_white());
    lv_style_set_radius(&s_nav_grid_title_bar_style, title_radius);
    lv_style_set_clip_corner(&s_nav_grid_title_bar_style, title_radius > 0);
    lv_style_set_text_color(&s_nav_grid_title_bar_style, lv_color_hex(themePalette().fg_inv));

    lv_style_init(&s_nav_grid_title_label_style);
    lv_style_set_text_align(&s_nav_grid_title_label_style, LV_TEXT_ALIGN_CENTER);

    lv_style_init(&s_nav_panel_grid_style);
    lv_style_set_bg_color(&s_nav_panel_grid_style, lv_color_hex(0x001765));
    lv_style_set_bg_opa(&s_nav_panel_grid_style, LV_OPA_COVER);
    lv_style_set_border_width(&s_nav_panel_grid_style, 0);
    lv_style_set_border_color(&s_nav_panel_grid_style, lv_color_hex(themePalette().panel_border));
    lv_style_set_border_opa(&s_nav_panel_grid_style, effectiveOpacity(LV_OPA_50));
    lv_style_set_radius(&s_nav_panel_grid_style, panel_radius);
    lv_style_set_clip_corner(&s_nav_panel_grid_style, panel_radius > 0);
#endif

#if HELTEC_UI_HAS_TOUCH
    lv_style_init(&s_nav_footer_bar_style);
    lv_style_set_height(&s_nav_footer_bar_style, lv_pct(100));
    lv_style_set_flex_grow(&s_nav_footer_bar_style, 1);
    lv_style_set_bg_color(&s_nav_footer_bar_style, lv_color_hex(themePalette().panel_bg));
    lv_style_set_bg_opa(&s_nav_footer_bar_style, LV_OPA_COVER);
    lv_style_set_text_color(&s_nav_footer_bar_style, lv_color_hex(themePalette().fg));
#else
    lv_style_init(&s_nav_root_ring_style);
    lv_style_set_bg_color(&s_nav_root_ring_style, lv_color_hex(themePalette().bg));
    lv_style_set_bg_opa(&s_nav_root_ring_style, effectiveOpacity(LV_OPA_50));

    lv_style_init(&s_nav_ring_style);
    lv_style_set_pad_all(&s_nav_ring_style, config.ring_edge_pad);
#endif

    lv_style_init(&s_visibility_hidden_style);
    lv_style_set_opa(&s_visibility_hidden_style, LV_OPA_TRANSP);

    s_navigation_styles_ready = true;
}

static lv_style_t *overlayRootChromeStyle()
{
    if (!s_overlayRootChromeStyle_ready)
    {
        lv_style_init(&s_overlayRootChromeStyle);
        lv_style_set_bg_opa(&s_overlayRootChromeStyle, LV_OPA_COVER);
        lv_style_set_bg_color(&s_overlayRootChromeStyle, lv_color_hex(themePalette().overlay_bg));
        lv_style_set_text_color(&s_overlayRootChromeStyle, lv_color_hex(themePalette().overlay_fg));
        s_overlayRootChromeStyle_ready = true;
    }
    return &s_overlayRootChromeStyle;
}

static bool isScreenRoot(_lv_obj_t *obj)
{
    if (!obj)
        return false;
#if defined(ENV_INCLUDE_MAP) && ENV_INCLUDE_MAP
    if (lv_obj_check_type(obj, &ht_tracker_screen_root_class))
        return true;
#endif
    _lv_obj_t *const parent = lv_obj_get_parent(obj);
#if LV_USE_TILEVIEW
    return parent && lv_obj_check_type(parent, &lv_tileview_tile_class);
#else
    return false;
#endif
}

static bool isOverlayRootClass(_lv_obj_t *obj)
{
    return obj &&
           (lv_obj_check_type(obj, &ht_preview_overlay_class) ||
            lv_obj_check_type(obj, &ht_radio_sync_overlay_class) ||
            lv_obj_check_type(obj, &ht_repeat_mode_overlay_class) ||
#if HELTEC_UI_HAS_TOUCH
            lv_obj_check_type(obj, &ht_send_message_touch_overlay_class) ||
#else
            lv_obj_check_type(obj, &ht_send_message_selector_overlay_class) ||
#endif
            lv_obj_check_type(obj, &ht_splash_overlay_class));
}

static void applySurfaceRootCommon(_lv_obj_t *obj)
{
    if (!obj)
        return;
    initCommonWidgetStyles();
    lv_obj_add_style(obj, &s_surface_root_common_style, LV_PART_MAIN);
}

static void stylePlainContainer(_lv_obj_t *obj)
{
    if (!obj)
        return;
    initCommonWidgetStyles();
    lv_obj_add_style(obj, &s_plain_container_style, LV_PART_MAIN);
}

#if HELTEC_UI_HAS_TOUCH
static void resetTouchObject(lv_obj_t *obj)
{
    if (!obj)
        return;
    lv_obj_remove_style_all(obj);
}
#endif

static void applyNoChrome(_lv_obj_t *obj)
{
    if (!obj)
        return;
    initCommonWidgetStyles();
    for (lv_style_selector_t selector : kMainNoChromeSelectors)
    {
        lv_obj_add_style(obj, &s_no_chrome_style, selector);
    }
}

static void applySystemControlNoChrome(_lv_obj_t *obj)
{
    if (!obj)
        return;
    initCommonWidgetStyles();
    for (lv_style_selector_t selector : kSystemControlNoChromeSelectors)
    {
        lv_obj_add_style(obj, &s_no_chrome_style, selector);
    }
}

static void hideScrollbarChrome(_lv_obj_t *obj)
{
    if (!obj)
        return;
    initCommonWidgetStyles();
    for (lv_style_selector_t selector : kScrollbarHiddenSelectors)
    {
        lv_obj_add_style(obj, &s_hidden_scrollbar_style, selector);
    }
}

static void styleScreenLabel(_lv_obj_t *obj, lv_text_align_t align = LV_TEXT_ALIGN_LEFT)
{
    if (!obj)
        return;

    _lv_obj_t *const parent = lv_obj_get_parent(obj);
    if (parent && lv_obj_has_class(parent, &ht_container_class))
    {
        applyNoChrome(obj);
        return;
    }

    lv_style_t *style = cachedLabelStyle(lv_color_hex(themePalette().fg), align);
    if (style)
        lv_obj_add_style(obj, style, LV_PART_MAIN);
    applyNoChrome(obj);
}

static bool isScreenTreeObject(_lv_obj_t *obj)
{
    for (_lv_obj_t *current = obj; current; current = lv_obj_get_parent(current))
    {
        if (isScreenRoot(current))
        {
            return true;
        }
    }
    return false;
}

static bool applyScreenClassTheme(_lv_obj_t *obj)
{
    if (!obj || !isScreenTreeObject(obj))
    {
        return false;
    }

    if (lv_obj_check_type(obj, &ht_volume_controls_class))
    {
        initScreenSystemStyles();
        lv_obj_add_style(obj, &s_system_volume_controls_style, LV_PART_MAIN);
        return true;
    }

    if (lv_obj_check_type(obj, &ht_screen_info_label_class))
    {
        styleScreenLabel(obj);
        initScreenSystemStyles();
        lv_obj_add_style(obj, &s_screen_row_layout_style, LV_PART_MAIN);
        return true;
    }

    if (lv_obj_check_type(obj, &ht_find_friend_dial_host_class))
    {
        stylePlainContainer(obj);
        initScreenSystemStyles();
        lv_obj_add_style(obj, &s_find_friend_dial_layout_style, LV_PART_MAIN);
        return true;
    }

    if (lv_obj_check_type(obj, &lv_label_class))
    {
        styleScreenLabel(obj);
        return true;
    }

    return false;
}

static void styleOverlayLabel(_lv_obj_t *obj, lv_color_t color,
                                lv_text_align_t align = LV_TEXT_ALIGN_CENTER)
{
    if (!obj)
        return;
    lv_style_t *style = cachedLabelStyle(color, align);
    if (style)
        lv_obj_add_style(obj, style, LV_PART_MAIN);
    lv_obj_set_style_text_line_space(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_text_letter_space(obj, 0, LV_PART_MAIN);
}

static void applyOverlayRootChrome(_lv_obj_t *obj)
{
    if (!obj)
        return;
    lv_obj_add_style(obj, overlayRootChromeStyle(), LV_PART_MAIN);
}

static bool applyTopPaneRoot(_lv_obj_t *obj)
{
    if (!lv_obj_check_type(obj, &ht_top_pane_class))
        return false;
    initTopPaneStyles(obj);
    lv_obj_add_style(obj, &s_top_pane_root_style, LV_PART_MAIN);

    return true;
}

static bool applyTopPaneDirectChild(_lv_obj_t *obj, _lv_obj_t *parent)
{
    if (!parent || !lv_obj_check_type(parent, &ht_top_pane_class))
        return false;
    (void)obj;
    return true;
}

static bool applyTopPaneNestedChild(_lv_obj_t *obj)
{
    _lv_obj_t *const parent = lv_obj_get_parent(obj);

    if (lv_obj_check_type(obj, &ht_top_pane_battery_class))
    {
        initTopPaneStyles(obj);
        lv_obj_add_style(obj, &s_top_pane_battery_style, LV_PART_MAIN);
        return true;
    }

    if (lv_obj_check_type(obj, &ht_top_pane_title_class))
    {
        initTopPaneStyles(obj);
        lv_obj_add_style(obj, &s_top_pane_title_style, LV_PART_MAIN);
        return true;
    }

    if (lv_obj_check_type(obj, &ht_top_pane_battery_outline_class))
    {
        initTopPaneStyles(obj);
        lv_obj_add_style(obj, &s_top_pane_battery_outline_style, LV_PART_MAIN);
#if HELTEC_UI_HAS_TOUCH
        lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN);
        lv_obj_set_style_border_color(obj, lv_color_white(), LV_PART_MAIN);
        lv_obj_set_style_border_opa(obj, LV_OPA_COVER, LV_PART_MAIN);
#endif
        return true;
    }

    if (lv_obj_check_type(obj, &ht_top_pane_battery_cap_class))
    {
        initTopPaneStyles(obj);
        lv_obj_add_style(obj, &s_top_pane_battery_cap_style, LV_PART_MAIN);
        return true;
    }

    if (parent && lv_obj_check_type(parent, &ht_top_pane_battery_outline_class))
    {
        initTopPaneStyles(obj);
        lv_obj_add_style(obj, &s_top_pane_battery_fill_style, LV_PART_MAIN);
        lv_obj_add_style(obj, &s_top_pane_battery_fill_low_style, LV_PART_MAIN | LV_STATE_USER_1);
        lv_obj_add_style(obj, &s_top_pane_battery_fill_mid_style, LV_PART_MAIN | LV_STATE_USER_2);
        lv_obj_add_style(obj, &s_top_pane_battery_fill_high_style, LV_PART_MAIN | LV_STATE_USER_3);
        return true;
    }
    return false;
}

static bool applySurfaceRootTheme(_lv_obj_t *obj)
{
    if (lv_obj_check_type(obj, &ht_dialog_overlay_class))
    {
        applySurfaceRootCommon(obj);
        initScreenSystemStyles();
        initSurfaceAppStyles(obj);
        lv_obj_add_style(obj, &s_confirm_overlay_backdrop_style, LV_PART_MAIN);
        for (lv_style_selector_t selector : kInteractiveSelectors)
        {
            lv_obj_add_style(obj, &s_alert_overlay_root_style, selector | LV_STATE_USER_1);
        }
        lv_obj_add_style(obj, &s_calibration_overlay_root_style,
                         LV_PART_MAIN | LV_STATE_USER_2);
        return true;
    }

#if HELTEC_UI_ENABLE_KEYBOARD
    if (lv_obj_check_type(obj, &ht_keyboard_overlay_class))
    {
        applySurfaceRootCommon(obj);
        applyOverlayRootChrome(obj);
        initKeyboardChildStyles();
        lv_obj_add_style(obj, &s_keyboard_root_layout_style, LV_PART_MAIN);
#if LV_USE_THEME_MONO
        lv_obj_add_style(obj, &s_keyboard_surface_color_style, LV_PART_MAIN);
#endif
        lv_obj_add_style(obj, &s_keyboard_root_waypoint_layout_style,
                         LV_PART_MAIN | LV_STATE_USER_1);
#if HELTEC_UI_HAS_TOUCH
        lv_obj_add_style(obj, &s_keyboard_waypoint_surface_style,
                         LV_PART_MAIN | LV_STATE_USER_1);
#endif
        return true;
    }
#endif

    if (isScreenRoot(obj))
    {
        applySurfaceRootCommon(obj);
        initCommonWidgetStyles();
        lv_obj_add_style(obj, &s_screen_root_layout_style, LV_PART_MAIN);
        if (lv_obj_check_type(obj, &ht_system_screen_root_class))
        {
            initScreenSystemStyles();
            lv_obj_add_style(obj, &s_system_screen_root_layout_style, LV_PART_MAIN);
        }
#if defined(ENV_INCLUDE_MAP) && ENV_INCLUDE_MAP && defined(HELTEC_UI_HAS_TOUCH) && HELTEC_UI_HAS_TOUCH
        if (lv_obj_check_type(obj, &ht_tracker_screen_root_class))
        {
            lv_obj_add_style(obj, &s_tracker_root_layout_style, LV_PART_MAIN);
        }
#endif
        applyNoChrome(obj);
        hideScrollbarChrome(obj);
        return true;
    }
    if (isOverlayRootClass(obj))
    {
        applySurfaceRootCommon(obj);
        if (lv_obj_check_type(obj, &ht_preview_overlay_class))
        {
            applyOverlayRootChrome(obj);
            initCommonWidgetStyles();
            lv_obj_add_style(obj, &s_preview_root_layout_style, LV_PART_MAIN);
        }
        else if (lv_obj_check_type(obj, &ht_radio_sync_overlay_class))
        {
            applyOverlayRootChrome(obj);
            initRadioSyncStyles();
            lv_obj_add_style(obj, &s_radio_sync_root_layout_style, LV_PART_MAIN);
        }
        else if (lv_obj_check_type(obj, &ht_repeat_mode_overlay_class))
        {
            applyOverlayRootChrome(obj);
            initRepeatModeStyles();
            lv_obj_add_style(obj, &s_repeat_root_layout_style, LV_PART_MAIN);
        }
#if HELTEC_UI_HAS_TOUCH
        else if (lv_obj_check_type(obj, &ht_send_message_touch_overlay_class))
#else
        else if (lv_obj_check_type(obj, &ht_send_message_selector_overlay_class))
#endif
        {
            applyOverlayRootChrome(obj);
            initSendMessageStyles();
            lv_obj_add_style(obj, &s_send_root_layout_style, LV_PART_MAIN);
        }
        else if (lv_obj_check_type(obj, &ht_splash_overlay_class))
        {
            applyOverlayRootChrome(obj);
            initCommonWidgetStyles();
            lv_obj_add_style(obj, &s_splash_root_layout_style, LV_PART_MAIN);
            const lv_font_t *font = lv_theme_get_font_normal(obj);
            if (!font)
                font = LV_FONT_DEFAULT;
            if (!s_splash_overlay_text_style_ready)
            {
                lv_style_init(&s_splash_overlay_text_style);
                s_splash_overlay_text_style_ready = true;
            }
            else if (s_splash_overlay_text_font != font)
            {
                lv_style_reset(&s_splash_overlay_text_style);
            }
            if (s_splash_overlay_text_font != font)
            {
                s_splash_overlay_text_font = font;
                lv_style_set_text_font(&s_splash_overlay_text_style, font);
                setOverlayTextSpacing(&s_splash_overlay_text_style);
            }
#if defined(HELTEC_HAS_KEY_FRAME) && HELTEC_HAS_KEY_FRAME
            lv_style_set_text_color(&s_splash_overlay_text_style, lv_color_hex(themePalette().fg));
#elif LV_USE_THEME_MONO
            lv_style_set_text_color(&s_splash_overlay_text_style, lv_color_hex(themePalette().fg));
#else
            lv_style_set_text_color(&s_splash_overlay_text_style, lv_color_hex(themePalette().overlay_fg));
#endif
            lv_style_set_text_align(&s_splash_overlay_text_style, LV_TEXT_ALIGN_CENTER);
            lv_obj_add_style(obj, &s_splash_overlay_text_style, LV_PART_MAIN);
        }
        return true;
    }
#if !HELTEC_UI_HAS_TOUCH
    if (lv_obj_check_type(obj, &ht_context_menu_class))
    {
        applySurfaceRootCommon(obj);
        initSurfaceAppStyles(obj);
        for (lv_style_selector_t selector : kContextMenuRootSelectors)
        {
            lv_obj_add_style(obj, &s_context_menu_root_style, selector);
        }
        return true;
    }
#endif
    return false;
}

static bool applyAppFrameTheme(_lv_obj_t *obj)
{
    if (!obj)
        return false;

#if LV_USE_TILEVIEW
    if (lv_obj_check_type(obj, &lv_tileview_class))
    {
        initSurfaceAppStyles(obj);
#if HELTEC_UI_HAS_TOUCH
        lv_obj_add_style(obj, &s_app_tileview_style, LV_PART_MAIN);
#endif
        applyNoChrome(obj);
        hideScrollbarChrome(obj);
        return true;
    }

    if (lv_obj_check_type(obj, &lv_tileview_tile_class))
    {
        initSurfaceAppStyles(obj);
        hideScrollbarChrome(obj);
        lv_obj_add_style(obj, &s_app_tile_style, LV_PART_MAIN);
        applyNoChrome(obj);
        return true;
    }
#endif

    if (lv_obj_check_type(obj, &ht_app_frame_class))
    {
        initSurfaceAppStyles(obj);
        lv_obj_add_style(obj, &s_app_frame_layout_style, LV_PART_MAIN);
        return true;
    }

    if (lv_obj_check_type(obj, &ht_app_content_class))
    {
        initSurfaceAppStyles(obj);
        lv_obj_add_style(obj, &s_app_content_style, LV_PART_MAIN);
        return true;
    }

    if (lv_obj_check_type(obj, &ht_app_screen_host_class))
    {
        initSurfaceAppStyles(obj);
        lv_obj_add_style(obj, &s_app_screen_root_style, LV_PART_MAIN);
        applyNoChrome(obj);
        hideScrollbarChrome(obj);
        return true;
    }
    return false;
}

#if !HELTEC_UI_HAS_TOUCH
static bool applyClassicContextMenuChildTheme(_lv_obj_t *obj)
{
    if (!obj)
        return false;
#if LV_USE_MENU
    if (lv_obj_check_type(obj, &lv_menu_class))
    {
        initClassicContextStyles(obj);
        lv_obj_add_style(obj, &s_ctx_menu_style, LV_PART_MAIN);
        return true;
    }

    if (lv_obj_check_type(obj, &lv_menu_main_header_cont_class))
    {
        initClassicContextStyles(obj);
        for (lv_style_selector_t selector : kContextMenuRootSelectors)
        {
            lv_obj_add_style(obj, &s_ctx_header_style, selector);
        }
        return true;
    }
#endif
    if (lv_obj_check_type(obj, &ht_context_menu_icon_row_class))
    {
        initClassicContextStyles(obj);
        for (lv_style_selector_t selector : kContextMenuRootSelectors)
        {
            lv_obj_add_style(obj, &s_ctx_header_row_style, selector);
        }
        lv_obj_add_style(obj, &s_ctx_icon_row_layout_style, LV_PART_MAIN);
        return true;
    }

    if (lv_obj_check_type(obj, &ht_context_menu_nav_row_class))
    {
        initClassicContextStyles(obj);
        for (lv_style_selector_t selector : kContextMenuRootSelectors)
        {
            lv_obj_add_style(obj, &s_ctx_header_row_style, selector);
        }
        return true;
    }

    if (lv_obj_check_type(obj, &ht_context_menu_icon_button_class))
    {
        initClassicContextStyles(obj);
        lv_obj_add_style(obj, &s_ctx_icon_btn_main_style, LV_PART_MAIN);
        lv_obj_add_style(obj, &s_ctx_icon_btn_checked_style, LV_PART_MAIN | LV_STATE_CHECKED);
        return true;
    }

    if (lv_obj_check_type(obj, &ht_context_menu_icon_class))
    {
        initClassicContextStyles(obj);
        lv_obj_add_style(obj, &s_ctx_icon_style, LV_PART_MAIN);
        lv_obj_add_style(obj, &s_ctx_icon_checked_style,
                         LV_PART_MAIN | LV_STATE_CHECKED);
        return true;
    }
    return false;
}
#endif

#if defined(HELTEC_UI_HAS_TOUCH) && HELTEC_UI_HAS_TOUCH
static bool applyQuickPingOverlayTheme(_lv_obj_t *obj)
{
    if (!obj)
        return false;
#if LV_USE_TEXTAREA
    if (lv_obj_check_type(obj, &ht_quick_ping_message_editor_class))
    {
        initQuickPingStyles();
        resetTouchObject(obj);
        lv_obj_add_style(obj, &s_quick_ping_message_input_style, LV_PART_MAIN);
        lv_obj_add_style(obj, &s_quick_ping_control_focus_style,
                         LV_PART_MAIN | LV_STATE_FOCUSED);
        lv_obj_add_style(obj, &s_quick_ping_control_focus_style,
                         LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_add_style(obj, &s_quick_ping_dropdown_disabled_style,
                         LV_PART_MAIN | LV_STATE_DISABLED);
        if (_lv_obj_t *const label = lv_textarea_get_label(obj))
        {
            resetTouchObject(label);
            lv_obj_add_style(label, &s_quick_ping_label_style, LV_PART_MAIN);
            lv_obj_add_style(label, &s_quick_ping_message_input_label_editing_style,
                             LV_PART_MAIN | LV_STATE_USER_1);
        }
        return true;
    }
#endif
#if LV_USE_KEYBOARD
    if (lv_obj_check_type(obj, &ht_quick_ping_keyboard_class))
    {
        initQuickPingStyles();
        resetTouchObject(obj);
        lv_obj_add_style(obj, &s_quick_ping_keyboard_style, LV_PART_MAIN);
        lv_obj_add_style(obj, &s_keyboard_items_base_style, LV_PART_ITEMS);
        static const lv_style_selector_t selected_item_sels[] = {
            LV_PART_ITEMS | LV_STATE_PRESSED,
            LV_PART_ITEMS | LV_STATE_FOCUSED,
            LV_PART_ITEMS | LV_STATE_FOCUS_KEY,
            LV_PART_ITEMS | LV_STATE_EDITED,
            LV_PART_ITEMS | (LV_STATE_FOCUSED | LV_STATE_CHECKED),
            LV_PART_ITEMS | (LV_STATE_FOCUS_KEY | LV_STATE_CHECKED),
            LV_PART_ITEMS | (LV_STATE_EDITED | LV_STATE_CHECKED),
        };
        for (lv_style_selector_t selector : selected_item_sels)
        {
            lv_obj_add_style(obj, &s_keyboard_items_selected_style, selector);
        }
        return true;
    }
#endif
    if (lv_obj_check_type(obj, &ht_quick_ping_overlay_class))
    {
        initQuickPingStyles();
        resetTouchObject(obj);
        lv_obj_add_style(obj, &s_quick_ping_root_style, LV_PART_MAIN);
        return true;
    }
    if (lv_obj_check_type(obj, &ht_quick_ping_backdrop_class))
    {
        initQuickPingStyles();
        resetTouchObject(obj);
        lv_obj_add_style(obj, &s_quick_ping_backdrop_style, LV_PART_MAIN);
        return true;
    }
    if (lv_obj_check_type(obj, &ht_quick_ping_title_bar_class))
    {
        initQuickPingStyles();
        resetTouchObject(obj);
        lv_obj_add_style(obj, &s_quick_ping_title_bar_style, LV_PART_MAIN);
        return true;
    }
    if (lv_obj_check_type(obj, &ht_quick_ping_content_class))
    {
        initQuickPingStyles();
        resetTouchObject(obj);
        lv_obj_add_style(obj, &s_quick_ping_content_style, LV_PART_MAIN);
        return true;
    }
    if (lv_obj_check_type(obj, &ht_quick_ping_message_controls_class))
    {
        initQuickPingStyles();
        resetTouchObject(obj);
        lv_obj_add_style(obj, &s_quick_ping_row_style, LV_PART_MAIN);
        lv_obj_add_style(obj, &s_quick_ping_row_focus_style,
                         LV_PART_MAIN | LV_STATE_FOCUSED);
        lv_obj_add_style(obj, &s_quick_ping_row_focus_style,
                         LV_PART_MAIN | LV_STATE_FOCUS_KEY);
        return true;
    }
    if (lv_obj_check_type(obj, &ht_quick_ping_keyboard_editor_class))
    {
        initQuickPingStyles();
        resetTouchObject(obj);
        lv_obj_add_style(obj, &s_quick_ping_keyboard_editor_style, LV_PART_MAIN);
        lv_obj_add_style(obj, &s_quick_ping_keyboard_placeholder_style,
                         LV_PART_TEXTAREA_PLACEHOLDER);
        lv_obj_add_style(obj, &s_quick_ping_keyboard_cursor_style, LV_PART_CURSOR);
        if (_lv_obj_t *const label = lv_textarea_get_label(obj))
        {
            resetTouchObject(label);
            lv_obj_add_style(label, &s_quick_ping_label_style, LV_PART_MAIN);
            lv_obj_add_style(label, &s_quick_ping_message_input_label_editing_style,
                             LV_PART_MAIN | LV_STATE_USER_1);
        }
        return true;
    }
    if (lv_obj_check_type(obj, &ht_quick_ping_keyboard_counter_class))
    {
        initQuickPingStyles();
        resetTouchObject(obj);
        lv_obj_add_style(obj, &s_quick_ping_keyboard_counter_style, LV_PART_MAIN);
        return true;
    }
    if (lv_obj_check_type(obj, &ht_quick_ping_title_class))
    {
        initQuickPingStyles();
        resetTouchObject(obj);
        lv_obj_add_style(obj, &s_quick_ping_title_style, LV_PART_MAIN);
        return true;
    }

    if (lv_obj_check_type(obj, &ht_quick_ping_row_class))
    {
        initQuickPingStyles();
        resetTouchObject(obj);
        lv_obj_add_style(obj, &s_quick_ping_row_style, LV_PART_MAIN);
        lv_obj_add_style(obj, &s_quick_ping_row_focus_style, LV_PART_MAIN | LV_STATE_FOCUSED);
        lv_obj_add_style(obj, &s_quick_ping_row_focus_style, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
        return true;
    }

    if (lv_obj_check_type(obj, &ht_quick_ping_label_class))
    {
        initQuickPingStyles();
        resetTouchObject(obj);
        lv_obj_add_style(obj, &s_quick_ping_label_style, LV_PART_MAIN);
        return true;
    }

    if (lv_obj_check_type(obj, &ht_quick_ping_dropdown_class))
    {
        initQuickPingStyles();
        resetTouchObject(obj);
        lv_obj_add_style(obj, &s_quick_ping_dropdown_style, LV_PART_MAIN);
        if (lv_obj_check_type(obj, &ht_quick_ping_message_dropdown_class))
        {
            lv_obj_add_style(obj, &s_quick_ping_message_dropdown_style, LV_PART_MAIN);
        }
        lv_obj_add_style(obj, &s_quick_ping_control_focus_style, LV_PART_MAIN | LV_STATE_FOCUSED);
        lv_obj_add_style(obj, &s_quick_ping_dropdown_disabled_style,
                         LV_PART_MAIN | LV_STATE_DISABLED);
        lv_obj_add_style(obj, &s_quick_ping_dropdown_indicator_style, LV_PART_INDICATOR);
        lv_obj_add_style(obj, &s_quick_ping_dropdown_indicator_style,
                         LV_PART_INDICATOR | LV_STATE_FOCUSED);
        return true;
    }

    if (lv_obj_check_type(obj, &ht_quick_ping_badge_class))
    {
        initQuickPingStyles();
        resetTouchObject(obj);
        lv_obj_add_style(obj, &s_quick_ping_icon_badge_style, LV_PART_MAIN);
        return true;
    }

    if (lv_obj_check_type(obj, &ht_quick_ping_message_row_class))
    {
        initQuickPingStyles();
        resetTouchObject(obj);
        return true;
    }

    return false;
}
#else
static bool applyQuickPingOverlayTheme(_lv_obj_t *)
{
    return false;
}
#endif

static bool applyContextMenuChildTheme(_lv_obj_t *obj)
{
#if HELTEC_UI_HAS_TOUCH
    return applyQuickPingOverlayTheme(obj);
#else
    return applyClassicContextMenuChildTheme(obj);
#endif
}

static bool applyDropdownBaseTheme(_lv_obj_t *obj)
{
    if (!obj)
        return false;

    if (lv_obj_check_type(obj, &lv_dropdownlist_class))
    {
        initScreenSystemStyles();
        initCommonWidgetStyles();
        lv_obj_add_style(obj, &s_screen_dropdown_list_layout_style, LV_PART_MAIN);
        lv_obj_add_style(obj, &s_system_dropdown_focus_style, LV_PART_SELECTED);
        static const lv_style_selector_t no_chrome_selectors[] = {
            LV_PART_MAIN,
            LV_PART_MAIN | LV_STATE_FOCUSED,
            LV_PART_MAIN | LV_STATE_FOCUS_KEY,
            LV_PART_MAIN | LV_STATE_PRESSED,
            LV_PART_ITEMS,
            LV_PART_ITEMS | LV_STATE_FOCUSED,
            LV_PART_ITEMS | LV_STATE_FOCUS_KEY,
            LV_PART_ITEMS | LV_STATE_PRESSED,
            LV_PART_SELECTED,
            LV_PART_SCROLLBAR,
        };
        for (lv_style_selector_t selector : no_chrome_selectors)
        {
            lv_obj_add_style(obj, &s_no_chrome_style, selector);
        }
#if HELTEC_UI_HAS_TOUCH
        constexpr lv_coord_t kDropdownOptionHeight = 32;
        lv_obj_t *const label = lv_obj_get_child(obj, 0);
        const lv_font_t *const font = label ? lv_obj_get_style_text_font(label, LV_PART_MAIN)
                                            : lv_obj_get_style_text_font(obj, LV_PART_MAIN);
        const lv_coord_t font_height = font ? lv_font_get_line_height(font) : 0;
        const lv_coord_t line_space = font_height < kDropdownOptionHeight
                                          ? kDropdownOptionHeight - font_height
                                          : 0;
        lv_obj_set_style_text_line_space(obj, line_space, LV_PART_MAIN);
        lv_obj_set_style_text_line_space(obj, line_space, LV_PART_SELECTED);
        if (label)
        {
            lv_obj_set_style_text_line_space(label, line_space, LV_PART_MAIN);
        }
#endif
        return true;
    }

    if (!lv_obj_has_class(obj, &lv_dropdown_class))
        return false;

    initScreenSystemStyles();
    applySystemControlNoChrome(obj);
    lv_obj_add_style(obj, &s_system_dropdown_style, LV_PART_MAIN);
    hideScrollbarChrome(obj);
    static const lv_style_selector_t selectors[] = {
        LV_PART_MAIN | LV_STATE_FOCUSED,
        LV_PART_MAIN | LV_STATE_FOCUS_KEY,
        LV_PART_MAIN | LV_STATE_EDITED,
    };
    for (lv_style_selector_t selector : selectors)
    {
        lv_obj_add_style(obj, &s_system_dropdown_focus_style, selector);
    }
    return true;
}

static bool applyConfirmOverlayChildTheme(_lv_obj_t *obj)
{
    if (!obj)
        return false;
    if (lv_obj_check_type(obj, &ht_confirm_text_class))
    {
        initScreenSystemStyles();
        lv_obj_add_style(obj, &s_confirm_overlay_text_style, LV_PART_MAIN);
        return true;
    }

    if (lv_obj_check_type(obj, &ht_confirm_button_class))
    {
        initScreenSystemStyles();
        lv_obj_add_style(obj, &s_confirm_overlay_button_style, LV_PART_MAIN);
        lv_obj_add_style(obj, &s_confirm_overlay_button_pressed_style,
                         LV_PART_MAIN | LV_STATE_PRESSED);
        return true;
    }
    return false;
}

static bool applyHtContainerChildTheme(_lv_obj_t *obj)
{
    if (!obj)
    {
        return false;
    }

    if (!lv_obj_check_type(obj, &lv_label_class))
    {
        return false;
    }

    _lv_obj_t *const parent = lv_obj_get_parent(obj);
    if (!parent || !lv_obj_check_type(parent, &ht_container_class))
    {
        return false;
    }

    styleScreenLabel(obj);
    return true;
}

static bool applyPreviewOverlayChildTheme(_lv_obj_t *obj)
{
    if (lv_obj_check_type(obj, &ht_preview_origin_class))
    {
        styleOverlayLabel(obj, lv_color_hex(themePalette().accent_alt), LV_TEXT_ALIGN_LEFT);
        return true;
    }

    if (lv_obj_check_type(obj, &ht_preview_text_class))
    {
        styleOverlayLabel(obj, lv_color_hex(themePalette().overlay_fg), LV_TEXT_ALIGN_LEFT);
        return true;
    }

    if (lv_obj_check_type(obj, &ht_preview_metadata_label_class))
    {
        styleOverlayLabel(obj, lv_color_hex(themePalette().overlay_fg));
        return true;
    }
    return false;
}

static void styleOverlayLabelAllStates(_lv_obj_t *label)
{
    if (!label)
        return;
    initOverlayFeedbackStyles();
    for (lv_style_selector_t sel : kInteractiveSelectors)
    {
        lv_obj_add_style(label, &s_overlay_label_all_states_style, sel);
    }
}

static bool applyAlertOverlayChildTheme(_lv_obj_t *obj)
{
    if (lv_obj_check_type(obj, &ht_overlay_feedback_label_class))
    {
        styleOverlayLabelAllStates(obj);
        return true;
    }
    return false;
}

static bool applyCalibrationOverlayChildTheme(_lv_obj_t *obj)
{
    return applyAlertOverlayChildTheme(obj);
}

#if HELTEC_UI_ENABLE_KEYBOARD
static bool applyKeyboardOverlayChildTheme(_lv_obj_t *obj)
{
    if (lv_obj_check_type(obj, &ht_keyboard_title_class))
    {
        initKeyboardChildStyles();
        lv_obj_add_style(obj, &s_keyboard_title_style, LV_PART_MAIN);
        lv_obj_add_style(obj, &s_keyboard_waypoint_title_style,
                         LV_PART_MAIN | LV_STATE_USER_1);
        return true;
    }

#if HELTEC_UI_HAS_TOUCH
    if (lv_obj_check_type(obj, &ht_coordinate_row_class))
    {
        initKeyboardChildStyles();
        lv_obj_add_style(obj, &s_coordinate_row_style, LV_PART_MAIN);
        return true;
    }
    if (lv_obj_check_type(obj, &ht_coordinate_label_class))
    {
        initKeyboardChildStyles();
        lv_obj_add_style(obj, &s_coordinate_label_style, LV_PART_MAIN);
        return true;
    }
#endif

    if (lv_obj_check_type(obj, &ht_keyboard_editor_class))
    {
        initKeyboardChildStyles();
        lv_obj_add_style(obj, &s_keyboard_textarea_style, LV_PART_MAIN);
        lv_obj_add_style(obj, &s_keyboard_waypoint_textarea_style,
                         LV_PART_MAIN | LV_STATE_USER_1);
        lv_obj_add_style(obj, &s_keyboard_active_textarea_style,
                         LV_PART_MAIN | LV_STATE_USER_1 | LV_STATE_CHECKED);
        return true;
    }

    if (lv_obj_check_type(obj, &ht_keyboard_matrix_class))
    {
        initKeyboardChildStyles();
        lv_obj_add_style(obj, &s_keyboard_main_style, LV_PART_MAIN);
#if LV_USE_THEME_MONO
        lv_obj_add_style(obj, &s_keyboard_surface_color_style, LV_PART_MAIN);
#endif
        lv_obj_add_style(obj, &s_keyboard_waypoint_main_style, LV_PART_MAIN | LV_STATE_USER_1);
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
        for (lv_style_selector_t sel : base_item_sels)
        {
            lv_obj_add_style(obj, &s_keyboard_items_base_style, sel);
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
        for (lv_style_selector_t sel : selected_item_sels)
        {
            lv_obj_add_style(obj, &s_keyboard_items_selected_style, sel);
        }
        return true;
    }
    return false;
}

#endif

static bool applyRadioParamSyncOverlayChildTheme(_lv_obj_t *obj)
{
    if (lv_obj_check_type(obj, &ht_radio_sync_list_class))
    {
        initRadioSyncStyles();
        lv_obj_add_style(obj, &s_radio_sync_list_style, LV_PART_MAIN);
        return true;
    }

#if !HELTEC_UI_HAS_TOUCH
    if (lv_obj_check_type(obj, &ht_radio_sync_row_class))
    {
        initRadioSyncStyles();
        lv_obj_add_style(obj, &s_radio_sync_row_main_style, LV_PART_MAIN);
        lv_obj_add_style(obj, &s_radio_sync_row_checked_style, LV_PART_MAIN | LV_STATE_CHECKED);
        return true;
    }
#endif
    return false;
}

static bool applyRepeatModeOverlayChildTheme(_lv_obj_t *obj)
{
#if !HELTEC_UI_HAS_TOUCH
    static const lv_state_t selected_states[] = {
        LV_STATE_CHECKED,
        LV_STATE_FOCUSED,
        LV_STATE_FOCUS_KEY,
        LV_STATE_PRESSED,
        static_cast<lv_state_t>(LV_STATE_CHECKED | LV_STATE_FOCUSED),
        static_cast<lv_state_t>(LV_STATE_CHECKED | LV_STATE_FOCUS_KEY),
    };

    if (lv_obj_check_type(obj, &ht_repeat_mode_list_class))
    {
        initRepeatModeStyles();
        lv_obj_add_style(obj, &s_repeat_list_style, LV_PART_MAIN);
        return true;
    }

    if (lv_obj_check_type(obj, &ht_repeat_mode_item_class))
    {
        initRepeatModeStyles();
        lv_obj_add_style(obj, &s_repeat_item_style, LV_PART_MAIN);
        for (lv_state_t state : selected_states)
        {
            lv_obj_add_style(obj, &s_repeat_item_selected_style, LV_PART_MAIN | state);
        }
        return true;
    }

#endif
    return false;
}

static bool applySendMessageOverlayChildTheme(_lv_obj_t *obj)
{
    if (lv_obj_check_type(obj, &ht_send_message_list_class))
    {
        initSendMessageStyles();
        lv_obj_add_style(obj, &s_send_list_style, LV_PART_MAIN);
        return true;
    }

#if HELTEC_UI_HAS_TOUCH
    if (lv_obj_check_type(obj, &ht_send_message_touch_list_class))
    {
        initSendMessageStyles();
        lv_obj_add_style(obj, &s_send_touch_list_style, LV_PART_MAIN);
        return true;
    }
#endif

#if HELTEC_UI_HAS_TOUCH
    if (lv_obj_check_type(obj, &ht_send_message_touch_row_class))
#else
    if (lv_obj_check_type(obj, &ht_send_message_row_class) ||
        lv_obj_check_type(obj, &ht_send_message_text_row_class))
#endif
    {
        initSendMessageStyles();
        lv_obj_add_style(obj, &s_send_row_main_style, LV_PART_MAIN);
        lv_obj_add_style(obj, &s_send_row_active_style, LV_PART_MAIN | LV_STATE_FOCUSED);
        lv_obj_add_style(obj, &s_send_row_active_style, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_add_style(obj, &s_send_row_active_style, LV_PART_MAIN | LV_STATE_PRESSED);
        return true;
    }

    return false;
}

static bool applySplashOverlayChildTheme(_lv_obj_t *obj)
{
    if (lv_obj_check_type(obj, &ht_splash_logo_class))
    {
#if !LV_USE_THEME_MONO
        lv_obj_set_style_img_recolor(obj, lv_color_hex(themePalette().overlay_fg), LV_PART_MAIN);
        lv_obj_set_style_img_recolor_opa(obj, LV_OPA_COVER, LV_PART_MAIN);
#endif
        return true;
    }
    return false;
}

static bool applyOverlayChildTheme(_lv_obj_t *obj)
{
    if (lv_obj_has_class(obj, &ht_overlay_bar_class))
    {
        applyOverlayHeaderFooterStyle(obj);
        return true;
    }

#if HELTEC_UI_HAS_TOUCH && LV_USE_ROLLER
    if (lv_obj_check_type(obj, &lv_roller_class))
    {
        initRadioSyncStyles();
        for (lv_style_selector_t selector : kInteractiveSelectors)
        {
            lv_obj_add_style(obj, &s_overlay_roller_style, selector);
        }
        lv_obj_add_style(obj, &s_overlay_roller_selected_style, LV_PART_SELECTED);
        return true;
    }
#endif

    if (lv_obj_check_type(obj, &ht_dialog_panel_class))
    {
        initScreenSystemStyles();
        initOverlayFeedbackStyles();
        lv_obj_add_style(obj, &s_dialog_panel_style, LV_PART_MAIN);
        for (lv_style_selector_t selector : kInteractiveSelectors)
        {
            lv_obj_add_style(obj, &s_alert_box_style, selector | LV_STATE_USER_1);
            lv_obj_add_style(obj, &s_calibration_panel_style, selector | LV_STATE_USER_2);
        }
        return true;
    }
    if (lv_obj_check_type(obj, &ht_dialog_content_class))
    {
        stylePlainContainer(obj);
#if LV_USE_THEME_MONO
        // Keep dialog chrome inverted while the content remains readable.
        lv_obj_set_style_bg_color(obj, lv_color_hex(themePalette().panel_bg), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_text_color(obj, lv_color_hex(themePalette().fg), LV_PART_MAIN);
#endif
        return true;
    }
    if (lv_obj_check_type(obj, &ht_dialog_actions_class))
    {
        initScreenSystemStyles();
        initCommonWidgetStyles();
        lv_obj_add_style(obj, &s_confirm_overlay_button_row_style, LV_PART_MAIN);
#if LV_USE_THEME_MONO
        lv_obj_set_style_bg_color(obj, lv_color_hex(themePalette().overlay_fg), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN);
#endif
        lv_obj_add_style(obj, &s_plain_container_style, LV_PART_MAIN | LV_STATE_USER_2);
        return true;
    }

    if (lv_obj_check_type(obj, &ht_framed_overlay_content_class))
    {
        initOverlayHeaderFooterStyle();
        lv_obj_add_style(obj, &s_framed_overlay_content_style, LV_PART_MAIN);
#if !HELTEC_UI_HAS_TOUCH
        if (lv_obj_check_type(obj, &ht_send_message_content_class))
        {
            initSendMessageStyles();
            lv_obj_add_style(obj, &s_send_content_style, LV_PART_MAIN);
        }
#endif
        return true;
    }

    return applyAlertOverlayChildTheme(obj) || applyCalibrationOverlayChildTheme(obj) ||
           applyConfirmOverlayChildTheme(obj) ||
#if HELTEC_UI_ENABLE_KEYBOARD
           applyKeyboardOverlayChildTheme(obj) ||
#endif
           applyPreviewOverlayChildTheme(obj) ||
           applyRadioParamSyncOverlayChildTheme(obj) ||
           applyRepeatModeOverlayChildTheme(obj) ||
           applySendMessageOverlayChildTheme(obj) || applySplashOverlayChildTheme(obj);
}

static void styleGridNavContentHost(lv_obj_t *content)
{
    if (!content)
        return;
    initNavigationStyles(content);
    lv_obj_remove_style_all(content);
#if HELTEC_UI_HAS_TOUCH
    lv_obj_add_style(content, &s_nav_content_host_style, LV_PART_MAIN);
#endif
}

#if HELTEC_UI_HAS_TOUCH
static void applyGridNavGridCellTheme(lv_obj_t *cell)
{
    if (!cell)
        return;
    initNavigationStyles(cell);

    lv_obj_add_style(cell, &s_nav_grid_cell_style, LV_PART_MAIN);
    lv_obj_add_style(cell, &s_nav_grid_cell_focus_style,
                     LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_add_style(cell, &s_nav_grid_cell_focus_style,
                     LV_PART_MAIN | LV_STATE_FOCUSED);
}

static void applyGridNavGridTitleBarTheme(lv_obj_t *bar)
{
    if (!bar)
        return;
    initNavigationStyles(bar);
    lv_obj_add_style(bar, &s_nav_grid_title_bar_style, LV_PART_MAIN);
    lv_obj_add_style(bar, &s_nav_footer_bar_style, LV_PART_MAIN | LV_STATE_USER_1);
}

static void applyGridNavGridTitleLabelTheme(lv_obj_t *label)
{
    if (!label)
        return;
    initNavigationStyles(label);
    lv_obj_add_style(label, &s_nav_grid_title_label_style, LV_PART_MAIN);
}
#endif

static void applyGridNavPanelChrome(lv_obj_t *obj)
{
    if (!obj)
        return;
    initNavigationStyles(obj);

#if HELTEC_UI_HAS_TOUCH
    lv_obj_add_style(obj, &s_nav_panel_grid_style, LV_PART_MAIN);
#endif
    lv_obj_add_style(obj, &s_visibility_hidden_style, LV_PART_MAIN | LV_STATE_USER_4);
}

} // namespace

#if !HELTEC_UI_HAS_TOUCH
void applyContextMenuTitleTheme(_lv_obj_t *title)
{
    if (!title)
        return;
    initClassicContextStyles(title);
    for (lv_style_selector_t selector : kContextMenuRootSelectors)
        lv_obj_add_style(title, &s_ctx_title_style, selector);
}
#endif

void configureWidgetTheme(const UiThemeProfile &profile)
{
    if (s_surface_app_styles_ready || s_navigation_styles_ready || s_top_pane_styles_ready)
    {
        return;
    }
#if !HELTEC_UI_HAS_TOUCH
    if (s_ctx_styles_ready)
    {
        return;
    }
#else
    if (s_quick_ping_styles_ready)
    {
        return;
    }
#endif

    s_themeProfile = &profile;
    configureComponentTheme();
}


static bool applyTopPaneTheme(_lv_obj_t *obj)
{
    if (!obj)
        return false;
    _lv_obj_t *parent = lv_obj_get_parent(obj);
    if (applyTopPaneRoot(obj))
        return true;
    if (applyTopPaneDirectChild(obj, parent))
        return true;
    if (applyTopPaneNestedChild(obj))
        return true;
    return false;
}

void applyActiveScreenTheme(_lv_obj_t *obj)
{
    if (!obj)
        return;
    initSurfaceAppStyles(obj);
    lv_obj_add_style(obj, &s_app_frame_layout_style, LV_PART_MAIN);
}

static bool applyNavigationTheme(_lv_obj_t *obj)
{
    if (!obj)
        return false;

    if (lv_obj_check_type(obj, &ht_gridNav_class) &&
        !lv_obj_check_type(obj, &ht_navigation_ring_class))
    {
        applyGridNavPanelChrome(obj);
        if (_lv_obj_t *content = gridNavContent(obj))
        {
            if (content != obj)
                styleGridNavContentHost(content);
        }
        return true;
    }

    if (lv_obj_check_type(obj, &ht_navigation_root_class))
    {
        initNavigationStyles(obj);
#if !HELTEC_UI_HAS_TOUCH
        lv_obj_add_style(obj, &s_nav_root_ring_style, LV_PART_MAIN);
#endif
        return true;
    }

    if (lv_obj_check_type(obj, &ht_navigation_content_class))
    {
        styleGridNavContentHost(obj);
        return true;
    }
#if HELTEC_UI_HAS_TOUCH
    if (lv_obj_check_type(obj, &ht_navigation_cell_class))
    {
        applyGridNavGridCellTheme(obj);
        return true;
    }
    if (lv_obj_check_type(obj, &ht_navigation_title_bar_class))
    {
        applyGridNavGridTitleBarTheme(obj);
        return true;
    }
    if (lv_obj_check_type(obj, &ht_navigation_title_label_class))
    {
        applyGridNavGridTitleLabelTheme(obj);
        return true;
    }
#endif
    return false;
}

static bool applyNavigationRingTheme(_lv_obj_t *obj)
{
#if !HELTEC_UI_HAS_TOUCH
    if (!lv_obj_check_type(obj, &ht_navigation_ring_class))
        return false;
    initNavigationStyles(obj);
    lv_obj_add_style(obj, &s_nav_ring_style, LV_PART_MAIN);
    lv_obj_add_style(obj, &s_visibility_hidden_style, LV_PART_MAIN | LV_STATE_USER_4);
    return true;
#else
    (void)obj;
    return false;
#endif
}

bool applyWidgetTheme(_lv_obj_t *obj)
{
    if (!obj)
        return false;

    if (lv_obj_check_type(obj, &ht_volume_button_class))
    {
        initScreenSystemStyles();
        lv_obj_add_style(obj, &s_system_volume_button_style, LV_PART_MAIN);
        lv_obj_add_style(obj, &s_system_volume_button_pressed_style,
                         LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_add_style(obj, &s_system_volume_button_focus_style,
                         LV_PART_MAIN | LV_STATE_FOCUSED);
        lv_obj_add_style(obj, &s_system_volume_button_focus_style,
                         LV_PART_MAIN | LV_STATE_FOCUS_KEY);
        return true;
    }

    if (lv_obj_check_type(obj, &ht_volume_controls_class))
    {
        initScreenSystemStyles();
        lv_obj_add_style(obj, &s_system_volume_controls_style, LV_PART_MAIN);
        return true;
    }

    if (lv_obj_check_type(obj, &ht_container_class))
    {
        initScreenSystemStyles();
        lv_obj_add_style(obj, &s_screen_row_layout_style, LV_PART_MAIN);
        lv_obj_add_style(obj, &s_container_focus_style,
                         LV_PART_MAIN | LV_STATE_USER_1);
        lv_obj_add_style(obj, &s_container_focus_style,
                         LV_PART_MAIN | LV_STATE_FOCUSED);
        lv_obj_add_style(obj, &s_container_focus_style,
                         LV_PART_MAIN | LV_STATE_FOCUS_KEY);
        return true;
    }

#if defined(HAS_BUZZER_VOLUME_CONTROL) && HAS_BUZZER_VOLUME_CONTROL && LV_USE_SLIDER
    if (lv_obj_check_type(obj, &lv_slider_class))
    {
        initScreenSystemStyles();
        lv_obj_add_style(obj, &s_system_volume_slider_style, LV_PART_MAIN);
        lv_obj_add_style(obj, &s_system_volume_slider_indicator_style, LV_PART_INDICATOR);
        lv_obj_add_style(obj, &s_system_volume_slider_knob_style, LV_PART_KNOB);
        return true;
    }
#endif

    if (lv_obj_has_class(obj, &ht_container_class))
    {
        initScreenSystemStyles();
        lv_obj_add_style(obj, &s_container_focus_style,
                         LV_PART_MAIN | LV_STATE_USER_1);
        lv_obj_add_style(obj, &s_container_focus_style,
                         LV_PART_MAIN | LV_STATE_FOCUSED);
        lv_obj_add_style(obj, &s_container_focus_style,
                         LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    }

    const bool screen_applied = applyScreenClassTheme(obj);
    const bool dropdown_applied = applyDropdownBaseTheme(obj);
    const bool custom_applied = applyAppFrameTheme(obj) || applyTopPaneTheme(obj) ||
                                applySurfaceRootTheme(obj) ||
                                applyHtContainerChildTheme(obj) ||
                                applyOverlayChildTheme(obj) ||
                                applyContextMenuChildTheme(obj) ||
                                applyNavigationTheme(obj) ||
                                applyNavigationRingTheme(obj) ||
                                applyRadialNavigationIconTheme(obj) ||
                                applyLicenseGateTheme(obj) ||
                                applyButtonRollerTheme(obj) ||
                                applyCompassTheme(obj) ||
                                applyMapWidgetTheme(obj);
    return screen_applied || dropdown_applied || custom_applied;
}

} // namespace Heltec::MeshCore::Ui
