#include "ui/app/theme.hpp"
#include "ui/core/widget_classes.hpp"
#include "ui/screens/compass_dial_widget.hpp"
#include "ui/theme/widget_theme.hpp"
#include "ui/theme/theme_internal.hpp"
#include "ui/widgets/button_roller.hpp"

namespace heltec::meshcore::ui
{
namespace
{

static lv_style_t s_license_gate_root_style;
static lv_style_t s_license_gate_label_style;
static lv_style_t s_license_gate_chip_style;
static bool s_license_gate_styles_ready = false;

void init_license_gate_styles()
{
    if (s_license_gate_styles_ready)
        return;

    lv_style_init(&s_license_gate_root_style);
    lv_style_set_width(&s_license_gate_root_style, lv_pct(100));
    lv_style_set_height(&s_license_gate_root_style, lv_pct(100));
    lv_style_set_radius(&s_license_gate_root_style, 0);
#if HELTEC_HAS_TOUCH
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
    lv_style_set_flex_main_place(&s_license_gate_root_style, LV_FLEX_ALIGN_CENTER);
    lv_style_set_flex_cross_place(&s_license_gate_root_style, LV_FLEX_ALIGN_CENTER);
    lv_style_set_flex_track_place(&s_license_gate_root_style, LV_FLEX_ALIGN_CENTER);
#if HELTEC_HAS_TOUCH
    lv_style_set_pad_row(&s_license_gate_root_style, 0);
    lv_style_set_pad_column(&s_license_gate_root_style, 0);
#else
    lv_style_set_pad_row(&s_license_gate_root_style, 12);
#endif

    lv_style_init(&s_license_gate_label_style);
    lv_style_set_text_color(&s_license_gate_label_style, lv_color_white());
    lv_style_set_text_align(&s_license_gate_label_style, LV_TEXT_ALIGN_CENTER);
#if HELTEC_HAS_TOUCH
    lv_style_set_pad_all(&s_license_gate_label_style, 0);
    lv_style_set_border_width(&s_license_gate_label_style, 0);
    lv_style_set_outline_width(&s_license_gate_label_style, 0);
#endif

    lv_style_init(&s_license_gate_chip_style);
    lv_style_set_width(&s_license_gate_chip_style, lv_pct(100));
#if HELTEC_HAS_TOUCH
    lv_style_set_pad_all(&s_license_gate_chip_style, 0);
    lv_style_set_border_width(&s_license_gate_chip_style, 0);
    lv_style_set_outline_width(&s_license_gate_chip_style, 0);
#endif

    s_license_gate_styles_ready = true;
}

#if !defined(UI_NAVIGATION_GRID) || !UI_NAVIGATION_GRID
static lv_style_t s_radial_icon_style;
static lv_style_t s_radial_icon_focus_style;
static bool s_radial_icon_styles_ready = false;

void init_radial_icon_styles()
{
    if (s_radial_icon_styles_ready)
        return;

    lv_style_init(&s_radial_icon_style);
    lv_style_set_bg_opa(&s_radial_icon_style, LV_OPA_TRANSP);
    lv_style_set_border_width(&s_radial_icon_style, 0);
    lv_style_set_outline_width(&s_radial_icon_style, 0);
    lv_style_set_shadow_width(&s_radial_icon_style, 0);
    lv_style_set_pad_all(&s_radial_icon_style, 0);

    lv_style_init(&s_radial_icon_focus_style);
    lv_style_set_border_width(&s_radial_icon_focus_style, 0);
    lv_style_set_shadow_width(&s_radial_icon_focus_style, 0);
    lv_style_set_pad_all(&s_radial_icon_focus_style, 0);

#if defined(UI_EINK_DISPLAY) && UI_EINK_DISPLAY
    lv_style_set_bg_opa(&s_radial_icon_focus_style, LV_OPA_TRANSP);
    lv_style_set_outline_color(&s_radial_icon_focus_style, lv_color_black());
    lv_style_set_outline_width(&s_radial_icon_focus_style, 1);
    lv_style_set_outline_pad(&s_radial_icon_focus_style, 1);
#elif (defined(HELTEC_DISPLAY_ST7735) && HELTEC_DISPLAY_ST7735) ||                         \
    (defined(HELTEC_DISPLAY_ST7789) && HELTEC_DISPLAY_ST7789) ||                          \
    (defined(HELTEC_DISPLAY_NV3001B) && HELTEC_DISPLAY_NV3001B)
    lv_style_set_img_recolor(&s_radial_icon_style, lv_color_white());
    lv_style_set_img_recolor_opa(&s_radial_icon_style, LV_OPA_COVER);
    lv_style_set_bg_opa(&s_radial_icon_focus_style, LV_OPA_TRANSP);
    lv_style_set_border_width(&s_radial_icon_focus_style, 0);
    lv_style_set_border_opa(&s_radial_icon_focus_style, LV_OPA_TRANSP);
    lv_style_set_outline_width(&s_radial_icon_focus_style, 0);
    lv_style_set_outline_opa(&s_radial_icon_focus_style, LV_OPA_TRANSP);
    lv_style_set_img_recolor(&s_radial_icon_focus_style, lv_color_hex(0x00C853));
    lv_style_set_img_recolor_opa(&s_radial_icon_focus_style, LV_OPA_COVER);
#elif (defined(HELTEC_DISPLAY_SSD1306) && HELTEC_DISPLAY_SSD1306) || LV_COLOR_DEPTH == 1
    lv_style_set_img_recolor(&s_radial_icon_style, lv_color_white());
    lv_style_set_img_recolor_opa(&s_radial_icon_style, LV_OPA_COVER);
    lv_style_set_bg_color(&s_radial_icon_focus_style, lv_color_white());
    lv_style_set_bg_opa(&s_radial_icon_focus_style, LV_OPA_COVER);
    lv_style_set_img_recolor(&s_radial_icon_focus_style, lv_color_black());
    lv_style_set_img_recolor_opa(&s_radial_icon_focus_style, LV_OPA_COVER);
    lv_style_set_outline_color(&s_radial_icon_focus_style, lv_color_white());
    lv_style_set_outline_width(&s_radial_icon_focus_style, 1);
#else
    lv_style_set_img_recolor(&s_radial_icon_style, lv_color_white());
    lv_style_set_img_recolor_opa(&s_radial_icon_style, LV_OPA_COVER);
    lv_style_set_bg_opa(&s_radial_icon_focus_style, LV_OPA_TRANSP);
    lv_style_set_outline_width(&s_radial_icon_focus_style, 0);
    lv_style_set_img_recolor(&s_radial_icon_focus_style, lv_color_hex(0x00C853));
    lv_style_set_img_recolor_opa(&s_radial_icon_focus_style, LV_OPA_COVER);
#endif

    s_radial_icon_styles_ready = true;
}
#endif

static lv_style_t s_roller_root_style;
static lv_style_t s_roller_button_style;
static lv_style_t s_roller_button_selected_style;
static bool s_roller_styles_ready = false;

static const lv_style_selector_t kRollerItemSelectedSelectors[] = {
    LV_PART_MAIN | LV_STATE_CHECKED,
    LV_PART_MAIN | LV_STATE_FOCUSED,
    LV_PART_MAIN | LV_STATE_FOCUS_KEY,
    LV_PART_MAIN | (LV_STATE_CHECKED | LV_STATE_FOCUSED),
    LV_PART_MAIN | (LV_STATE_CHECKED | LV_STATE_FOCUS_KEY),
    LV_PART_MAIN | LV_STATE_PRESSED,
};

static UiCascadingMenuStyles s_cascading_menu_styles;

static const lv_style_selector_t kCascadingMenuSelectedSelectors[] = {
    LV_PART_MAIN | LV_STATE_CHECKED,
    LV_PART_MAIN | LV_STATE_FOCUSED,
    LV_PART_MAIN | LV_STATE_FOCUS_KEY,
    LV_PART_MAIN | LV_STATE_PRESSED,
    LV_PART_MAIN | (LV_STATE_CHECKED | LV_STATE_FOCUSED),
    LV_PART_MAIN | (LV_STATE_CHECKED | LV_STATE_FOCUS_KEY),
};

void init_roller_styles(lv_coord_t pad = 2, lv_coord_t button_height = 12)
{
    if (s_roller_styles_ready)
        return;

    lv_style_init(&s_roller_root_style);
    lv_style_set_border_width(&s_roller_root_style, 0);
    lv_style_set_pad_all(&s_roller_root_style, pad);
    lv_style_set_bg_opa(&s_roller_root_style, LV_OPA_TRANSP);

    lv_style_init(&s_roller_button_style);
    lv_style_set_height(&s_roller_button_style, button_height);
    lv_style_set_pad_all(&s_roller_button_style, pad);
    lv_style_set_border_width(&s_roller_button_style, 0);
    lv_style_set_bg_opa(&s_roller_button_style, LV_OPA_TRANSP);
    lv_style_set_text_color(&s_roller_button_style, ui_color_fg());
    lv_style_set_text_line_space(&s_roller_button_style, 0);
    lv_style_set_text_letter_space(&s_roller_button_style, 0);
    lv_style_set_shadow_width(&s_roller_button_style, 0);
    lv_style_set_radius(&s_roller_button_style, 0);

    lv_style_init(&s_roller_button_selected_style);
    lv_style_set_border_width(&s_roller_button_selected_style, 0);
    lv_style_set_outline_width(&s_roller_button_selected_style, 0);
    lv_style_set_shadow_width(&s_roller_button_selected_style, 0);
    lv_style_set_radius(&s_roller_button_selected_style, 0);
    lv_style_set_bg_color(&s_roller_button_selected_style, ui_color_highlight_bg());
    lv_style_set_bg_opa(&s_roller_button_selected_style, LV_OPA_COVER);
    lv_style_set_text_color(&s_roller_button_selected_style, ui_color_highlight_fg());

    s_roller_styles_ready = true;
}

void style_roller_root(lv_obj_t *obj)
{
    init_roller_styles();
    lv_obj_add_style(obj, &s_roller_root_style, LV_PART_MAIN);
}

void style_roller_button(lv_obj_t *obj)
{
    init_roller_styles();
    lv_obj_add_style(obj, &s_roller_button_style, LV_PART_MAIN);
    for (lv_style_selector_t selector : kRollerItemSelectedSelectors)
    {
        lv_obj_add_style(obj, &s_roller_button_selected_style, selector);
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

struct MapColorStyleSlot
{
    bool ready = false;
    uint32_t color = 0;
    lv_style_t style;
};

struct MapOpaStyleSlot
{
    bool ready = false;
    lv_opa_t opa = LV_OPA_TRANSP;
    lv_style_t style;
};

static MapColorStyleSlot s_map_marker_color_styles[8];
static MapOpaStyleSlot s_map_range_opa_styles[8];

void init_map_styles()
{
    if (s_map_styles_ready)
        return;

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
    lv_style_set_border_color(&s_map_toolbar_button_checked_style, ui_color_success());
    lv_style_set_bg_opa(&s_map_toolbar_button_checked_style, LV_OPA_TRANSP);

    lv_style_init(&s_map_toolbar_button_pressed_style);
    lv_style_set_border_color(&s_map_toolbar_button_pressed_style, ui_color_error());
    lv_style_set_bg_opa(&s_map_toolbar_button_pressed_style, LV_OPA_TRANSP);

    lv_style_init(&s_map_toolbar_button_disabled_style);
    lv_style_set_border_opa(&s_map_toolbar_button_disabled_style, ui_effective_opa(LV_OPA_50));
    lv_style_set_bg_opa(&s_map_toolbar_button_disabled_style, LV_OPA_TRANSP);

    lv_style_init(&s_map_toolbar_label_style);
    lv_style_set_text_color(&s_map_toolbar_label_style, ui_color_accent());
    lv_style_set_text_font(&s_map_toolbar_label_style, LV_FONT_DEFAULT);

    lv_style_init(&s_map_toolbar_label_checked_style);
    lv_style_set_text_color(&s_map_toolbar_label_checked_style, ui_color_success());

    lv_style_init(&s_map_toolbar_label_pressed_style);
    lv_style_set_text_color(&s_map_toolbar_label_pressed_style, ui_color_error());

    lv_style_init(&s_map_toolbar_label_disabled_style);
    lv_style_set_text_color(&s_map_toolbar_label_disabled_style, lv_color_hex(0x607080));

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
    lv_style_set_text_color(&s_map_tile_placeholder_style, lv_color_hex(0x606060));
    lv_style_set_text_align(&s_map_tile_placeholder_style, LV_TEXT_ALIGN_CENTER);

    lv_style_init(&s_map_marker_label_style);
    lv_style_set_text_color(&s_map_marker_label_style, ui_color_bg());

    lv_style_init(&s_map_range_label_style);
    lv_style_set_text_color(&s_map_range_label_style, ui_color_fg());
    lv_style_set_bg_opa(&s_map_range_label_style, LV_OPA_TRANSP);
    lv_style_set_pad_all(&s_map_range_label_style, 0);

    s_map_styles_ready = true;
}

lv_style_t *marker_color_style(lv_color_t color)
{
    const uint32_t color32 = lv_color_to32(color);
    for (MapColorStyleSlot &slot : s_map_marker_color_styles)
    {
        if (slot.ready && slot.color == color32)
            return &slot.style;
    }
    for (MapColorStyleSlot &slot : s_map_marker_color_styles)
    {
        if (slot.ready)
            continue;
        slot.ready = true;
        slot.color = color32;
        lv_style_init(&slot.style);
        lv_style_set_bg_color(&slot.style, color);
        return &slot.style;
    }
    return nullptr;
}

lv_style_t *range_opa_style(lv_opa_t opa)
{
    for (MapOpaStyleSlot &slot : s_map_range_opa_styles)
    {
        if (slot.ready && slot.opa == opa)
            return &slot.style;
    }
    for (MapOpaStyleSlot &slot : s_map_range_opa_styles)
    {
        if (slot.ready)
            continue;
        slot.ready = true;
        slot.opa = opa;
        lv_style_init(&slot.style);
        lv_style_set_border_opa(&slot.style, ui_effective_opa(opa));
        return &slot.style;
    }
    return nullptr;
}
#endif

#if (defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS) || \
    (defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS)
static lv_style_t s_compass_plain_overlay_style;
static lv_style_t s_compass_hub_style;
static lv_style_t s_compass_needle_style;
static lv_style_t s_compass_label_style;
static bool s_compass_styles_ready = false;

void init_compass_styles()
{
    if (s_compass_styles_ready)
        return;

    lv_style_init(&s_compass_plain_overlay_style);
    lv_style_set_bg_opa(&s_compass_plain_overlay_style, LV_OPA_TRANSP);
    lv_style_set_border_width(&s_compass_plain_overlay_style, 0);
    lv_style_set_pad_all(&s_compass_plain_overlay_style, 0);

    lv_style_init(&s_compass_hub_style);
    lv_style_set_bg_color(&s_compass_hub_style, ui_color_panel_dark_bg());
    lv_style_set_bg_opa(&s_compass_hub_style, LV_OPA_COVER);
    lv_style_set_radius(&s_compass_hub_style, LV_RADIUS_CIRCLE);
#if defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
    lv_style_set_bg_color(&s_compass_hub_style, ui_color_bg());
    lv_style_set_bg_grad_dir(&s_compass_hub_style, LV_GRAD_DIR_NONE);
    lv_style_set_border_width(&s_compass_hub_style, 0);
    lv_style_set_clip_corner(&s_compass_hub_style, false);
#else
    lv_style_set_border_width(&s_compass_hub_style, 1);
    lv_style_set_border_color(&s_compass_hub_style, ui_color_fg_on_dark());
    lv_style_set_clip_corner(&s_compass_hub_style, true);
#endif
    lv_style_set_border_opa(&s_compass_hub_style, LV_OPA_COVER);
    lv_style_set_pad_all(&s_compass_hub_style, 0);

    lv_style_init(&s_compass_needle_style);
    lv_style_set_bg_opa(&s_compass_needle_style, LV_OPA_TRANSP);
    lv_style_set_border_width(&s_compass_needle_style, 0);
    lv_style_set_outline_width(&s_compass_needle_style, 0);
    lv_style_set_radius(&s_compass_needle_style, LV_RADIUS_CIRCLE);
    lv_style_set_clip_corner(&s_compass_needle_style, true);
    lv_style_set_pad_all(&s_compass_needle_style, 0);

    lv_style_init(&s_compass_label_style);
    lv_style_set_text_color(&s_compass_label_style, ui_color_fg());
    lv_style_set_text_align(&s_compass_label_style, LV_TEXT_ALIGN_LEFT);

    s_compass_styles_ready = true;
}

void style_compass_plain_overlay(_lv_obj_t *obj)
{
    init_compass_styles();
    lv_obj_add_style(obj, &s_compass_plain_overlay_style, LV_PART_MAIN);
}
#endif

} // namespace

bool ui_license_gate_apply_theme(_lv_obj_t *obj)
{
    if (!obj)
        return false;

#if LV_USE_MENU
    if (lv_obj_check_type(obj, &ht_cascading_menu_page_class))
    {
        if (s_cascading_menu_styles.page)
            lv_obj_add_style(obj, s_cascading_menu_styles.page, LV_PART_MAIN);
        return true;
    }
#endif

    if (lv_obj_check_type(obj, &ht_license_gate_class))
    {
        init_license_gate_styles();
        lv_obj_add_style(obj, &s_license_gate_root_style, LV_PART_MAIN);
        return true;
    }

    if (!lv_obj_check_type(obj, &ht_license_gate_label_class) &&
        !lv_obj_check_type(obj, &ht_license_gate_chip_class))
    {
        return false;
    }

    init_license_gate_styles();
    lv_obj_add_style(obj, &s_license_gate_label_style, LV_PART_MAIN);
    if (lv_obj_check_type(obj, &ht_license_gate_chip_class))
    {
        init_license_gate_styles();
        lv_obj_add_style(obj, &s_license_gate_chip_style, LV_PART_MAIN);
    }
    return true;
}

bool ui_radial_navigation_icon_apply_theme(_lv_obj_t *icon)
{
#if !defined(UI_NAVIGATION_GRID) || !UI_NAVIGATION_GRID
    if (!icon || !lv_obj_check_type(icon, &ht_radial_navigation_icon_class))
        return false;
    init_radial_icon_styles();
    lv_obj_remove_style_all(icon);
    lv_obj_add_style(icon, &s_radial_icon_style, LV_PART_MAIN);
    lv_obj_add_style(icon, &s_radial_icon_focus_style,
                     LV_PART_MAIN | LV_STATE_CHECKED);
    return true;
#else
    (void)icon;
    return false;
#endif
}

void ui_component_theme_configure(lv_coord_t bar_height)
{
    if (s_roller_styles_ready)
        return;
    const lv_coord_t button_height = LV_MIN(bar_height, 12);
    const lv_coord_t pad = button_height < 12 ? 0 : 2;
    init_roller_styles(pad, button_height);
}

void ui_widget_theme_set_cascading_menu_styles(const UiCascadingMenuStyles &styles)
{
    s_cascading_menu_styles = styles;
}

bool ui_button_roller_apply_theme(_lv_obj_t *obj)
{
    if (!obj)
        return false;

    if (lv_obj_check_type(obj, &ht_cascading_menu_roller_class))
    {
        style_roller_root(obj);
        if (s_cascading_menu_styles.page)
            lv_obj_add_style(obj, s_cascading_menu_styles.page, LV_PART_MAIN);
        return true;
    }

    if (lv_obj_check_type(obj, &ht_cascading_menu_item_class))
    {
        style_roller_button(obj);
        if (s_cascading_menu_styles.item)
            lv_obj_add_style(obj, s_cascading_menu_styles.item, LV_PART_MAIN);
        if (s_cascading_menu_styles.item_selected)
        {
            for (lv_style_selector_t selector : kCascadingMenuSelectedSelectors)
                lv_obj_add_style(obj, s_cascading_menu_styles.item_selected, selector);
        }
        return true;
    }

    if (lv_obj_check_type(obj, &ht_cascading_menu_label_class))
    {
        return true;
    }

    if (lv_obj_has_class(obj, &ht_button_roller_class))
    {
        style_roller_root(obj);
        return true;
    }

    if (lv_obj_check_type(obj, &ht_button_roller_item_class))
    {
        style_roller_button(obj);
        return true;
    }

    if (lv_obj_check_type(obj, &ht_button_roller_label_class))
    {
        return true;
    }
    return false;
}

#if defined(ENV_INCLUDE_MAP) && ENV_INCLUDE_MAP
bool ui_map_widget_apply_theme(_lv_obj_t *obj)
{
    if (!obj)
        return false;
    init_map_styles();

    if (lv_obj_check_type(obj, &ht_map_status_label_class))
    {
        lv_obj_add_style(obj, &s_map_status_label_style, LV_PART_MAIN);
        return true;
    }
    if (lv_obj_check_type(obj, &ht_map_viewport_class))
    {
        lv_obj_add_style(obj, &s_map_opaque_surface_style, LV_PART_MAIN);
        lv_obj_add_style(obj, &s_map_viewport_layout_style, LV_PART_MAIN);
        return true;
    }
    if (lv_obj_check_type(obj, &ht_map_toolbar_class))
    {
        lv_obj_add_style(obj, &s_map_toolbar_style, LV_PART_MAIN);
        return true;
    }

#if defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
    if (lv_obj_check_type(obj, &ht_tracker_toolbar_button_class))
    {
        lv_obj_add_style(obj, &s_map_toolbar_button_style, LV_PART_MAIN);
        lv_obj_add_style(obj, &s_map_toolbar_button_checked_style, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_add_style(obj, &s_map_toolbar_button_pressed_style, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_add_style(obj, &s_map_toolbar_button_disabled_style,
                         LV_PART_MAIN | LV_STATE_DISABLED);
        return true;
    }
#endif
    if (lv_obj_check_type(obj, &ht_map_toolbar_button_class))
    {
        lv_obj_add_style(obj, &s_map_toolbar_button_style, LV_PART_MAIN);
        lv_obj_add_style(obj, &s_map_toolbar_button_checked_style, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_add_style(obj, &s_map_toolbar_button_pressed_style, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_add_style(obj, &s_map_toolbar_button_disabled_style,
                         LV_PART_MAIN | LV_STATE_DISABLED);
        return true;
    }
    if (lv_obj_check_type(obj, &ht_map_toolbar_dropdown_class))
    {
        lv_obj_add_style(obj, &s_map_toolbar_button_style, LV_PART_MAIN);
        lv_obj_add_style(obj, &s_map_toolbar_label_style, LV_PART_MAIN);
        lv_obj_add_style(obj, &s_map_toolbar_button_disabled_style,
                         LV_PART_MAIN | LV_STATE_DISABLED);
        lv_obj_add_style(obj, &s_map_toolbar_label_disabled_style,
                         LV_PART_MAIN | LV_STATE_DISABLED);
        return true;
    }
    if (lv_obj_check_type(obj, &ht_map_toolbar_label_class))
    {
        lv_obj_add_style(obj, &s_map_toolbar_label_style, LV_PART_MAIN);
        lv_obj_add_style(obj, &s_map_toolbar_label_checked_style, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_add_style(obj, &s_map_toolbar_label_pressed_style, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_add_style(obj, &s_map_toolbar_label_disabled_style,
                         LV_PART_MAIN | LV_STATE_DISABLED);
        return true;
    }
    if (lv_obj_check_type(obj, &ht_map_layer_class))
    {
        lv_obj_add_style(obj, &s_map_transparent_layer_style, LV_PART_MAIN);
        return true;
    }
    if (lv_obj_check_type(obj, &ht_map_tile_class))
    {
        lv_obj_add_style(obj, &s_map_opaque_surface_style, LV_PART_MAIN);
        return true;
    }
    if (lv_obj_check_type(obj, &ht_map_marker_class))
    {
        lv_obj_add_style(obj, &s_map_marker_style, LV_PART_MAIN);
        return true;
    }

    if (lv_obj_check_type(obj, &ht_map_tile_placeholder_class))
    {
        lv_obj_add_style(obj, &s_map_tile_placeholder_style, LV_PART_MAIN);
        return true;
    }
    if (lv_obj_check_type(obj, &ht_map_marker_label_class))
    {
        lv_obj_add_style(obj, &s_map_marker_label_style, LV_PART_MAIN);
        return true;
    }
    if (lv_obj_check_type(obj, &ht_map_range_label_class))
    {
        lv_obj_add_style(obj, &s_map_range_label_style, LV_PART_MAIN);
        return true;
    }
    if (lv_obj_check_type(obj, &ht_map_range_ring_class))
    {
        lv_obj_add_style(obj, &s_map_range_ring_style, LV_PART_MAIN);
        return true;
    }
    return false;
}

void ui_map_marker_apply_color(_lv_obj_t *obj, lv_color_t color)
{
    if (!obj)
        return;
    if (lv_style_t *style = marker_color_style(color))
    {
        lv_obj_add_style(obj, style, LV_PART_MAIN);
    }
}

void ui_map_range_ring_apply_opa(_lv_obj_t *obj, lv_opa_t opa)
{
    if (!obj)
        return;
    if (lv_style_t *style = range_opa_style(opa))
    {
        lv_obj_add_style(obj, style, LV_PART_MAIN);
    }
}
#else
bool ui_map_widget_apply_theme(_lv_obj_t *obj)
{
    (void)obj;
    return false;
}

void ui_map_marker_apply_color(_lv_obj_t *obj, lv_color_t color)
{
    (void)obj;
    (void)color;
}

void ui_map_range_ring_apply_opa(_lv_obj_t *obj, lv_opa_t opa)
{
    (void)obj;
    (void)opa;
}
#endif

#if (defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS) || \
    (defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS)
void ui_theme_set_compass_info_layout(_lv_obj_t *obj, lv_coord_t pad_left, lv_coord_t pad_row)
{
    if (!obj)
        return;
    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_left(obj, pad_left, LV_PART_MAIN);
    lv_obj_set_style_pad_row(obj, pad_row, LV_PART_MAIN);
}

void ui_theme_set_compass_dial_row_layout(_lv_obj_t *obj, lv_coord_t pad_column)
{
    if (!obj)
        return;
    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(obj, pad_column, LV_PART_MAIN);
}

bool ui_compass_apply_theme(_lv_obj_t *obj)
{
    if (!obj)
        return false;
    init_compass_styles();
    if (lv_obj_check_type(obj, &ht_compass_hub_class))
        lv_obj_add_style(obj, &s_compass_hub_style, LV_PART_MAIN);
    else if (lv_obj_check_type(obj, &ht_compass_needle_class))
    {
        style_compass_plain_overlay(obj);
        lv_obj_add_style(obj, &s_compass_needle_style, LV_PART_MAIN);
    }
    else if (lv_obj_check_type(obj, &ht_compass_plain_class))
        style_compass_plain_overlay(obj);
    else if (lv_obj_check_type(obj, &ht_compass_label_class))
        lv_obj_add_style(obj, &s_compass_label_style, LV_PART_MAIN);
    else
        return false;
    return true;
}
#else
void ui_theme_set_compass_info_layout(_lv_obj_t *obj, lv_coord_t pad_left, lv_coord_t pad_row)
{
    (void)obj;
    (void)pad_left;
    (void)pad_row;
}

void ui_theme_set_compass_dial_row_layout(_lv_obj_t *obj, lv_coord_t pad_column)
{
    (void)obj;
    (void)pad_column;
}

bool ui_compass_apply_theme(_lv_obj_t *obj)
{
    (void)obj;
    return false;
}
#endif

} // namespace heltec::meshcore::ui
