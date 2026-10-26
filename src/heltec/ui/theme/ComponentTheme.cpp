#include "ui/theme/UiTheme.hpp"
#include "ui/widgets/SemanticClasses.hpp"
#include "ui/screens/CompassDialWidget.hpp"
#include "ui/theme/WidgetTheme.hpp"
#include "ui/theme/ThemeUtil.hpp"
#include "ui/widgets/ButtonRoller.hpp"

namespace Heltec::MeshCore::Ui
{
namespace
{

static lv_style_t s_license_gate_root_style;
static lv_style_t s_license_gate_label_style;
static lv_style_t s_license_gate_chip_style;
static bool s_license_gate_styles_ready = false;

void initLicenseGateStyles()
{
    if (s_license_gate_styles_ready)
        return;

    lv_style_init(&s_license_gate_root_style);
    lv_style_set_width(&s_license_gate_root_style, lv_pct(100));
    lv_style_set_height(&s_license_gate_root_style, lv_pct(100));
#if HELTEC_UI_HAS_TOUCH
    lv_style_set_pad_all(&s_license_gate_root_style, 0);
    lv_style_set_border_width(&s_license_gate_root_style, 0);
    lv_style_set_outline_width(&s_license_gate_root_style, 0);
#else
    lv_style_set_pad_all(&s_license_gate_root_style, 8);
#endif
    lv_style_set_bg_color(&s_license_gate_root_style, lv_color_black());
    lv_style_set_bg_opa(&s_license_gate_root_style, LV_OPA_COVER);
    lv_style_set_text_color(&s_license_gate_root_style, lv_color_white());
    lv_style_set_layout(&s_license_gate_root_style, LV_LAYOUT_FLEX);
    lv_style_set_flex_flow(&s_license_gate_root_style, LV_FLEX_FLOW_COLUMN);
    lv_style_set_flex_main_place(&s_license_gate_root_style, LV_FLEX_ALIGN_CENTER);
    lv_style_set_flex_cross_place(&s_license_gate_root_style, LV_FLEX_ALIGN_CENTER);
    lv_style_set_flex_track_place(&s_license_gate_root_style, LV_FLEX_ALIGN_CENTER);
#if HELTEC_UI_HAS_TOUCH
    lv_style_set_pad_row(&s_license_gate_root_style, 0);
    lv_style_set_pad_column(&s_license_gate_root_style, 0);
#else
    lv_style_set_pad_row(&s_license_gate_root_style, 12);
#endif

    lv_style_init(&s_license_gate_label_style);
    lv_style_set_text_align(&s_license_gate_label_style, LV_TEXT_ALIGN_CENTER);
#if HELTEC_UI_HAS_TOUCH
    lv_style_set_pad_all(&s_license_gate_label_style, 0);
    lv_style_set_border_width(&s_license_gate_label_style, 0);
    lv_style_set_outline_width(&s_license_gate_label_style, 0);
#endif

    lv_style_init(&s_license_gate_chip_style);
    lv_style_set_width(&s_license_gate_chip_style, lv_pct(100));
#if HELTEC_UI_HAS_TOUCH
    lv_style_set_pad_all(&s_license_gate_chip_style, 0);
    lv_style_set_border_width(&s_license_gate_chip_style, 0);
    lv_style_set_outline_width(&s_license_gate_chip_style, 0);
#endif

    s_license_gate_styles_ready = true;
}

#if !HELTEC_UI_HAS_TOUCH
static lv_style_t s_radial_icon_style;
static lv_style_t s_radial_icon_focus_style;
static bool s_radial_icon_styles_ready = false;

void initRadialIconStyles()
{
    if (s_radial_icon_styles_ready)
        return;

    lv_style_init(&s_radial_icon_style);

    lv_style_init(&s_radial_icon_focus_style);

#if defined(HELTEC_HAS_KEY_FRAME) && HELTEC_HAS_KEY_FRAME
    lv_style_set_outline_color(&s_radial_icon_focus_style, lv_color_black());
    lv_style_set_outline_width(&s_radial_icon_focus_style, 1);
    lv_style_set_outline_pad(&s_radial_icon_focus_style, 1);
#elif LV_COLOR_DEPTH > 1
    lv_style_set_img_recolor(&s_radial_icon_style, lv_color_white());
    lv_style_set_img_recolor_opa(&s_radial_icon_style, LV_OPA_COVER);
    lv_style_set_img_recolor(&s_radial_icon_focus_style, lv_color_hex(0x00C853));
    lv_style_set_img_recolor_opa(&s_radial_icon_focus_style, LV_OPA_COVER);
#elif LV_COLOR_DEPTH == 1
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

static UiContextMenuRollerStyles s_context_menu_roller_styles;

static const lv_style_selector_t kContextMenuRollerSelectedSelectors[] = {
    LV_PART_MAIN | LV_STATE_CHECKED,
    LV_PART_MAIN | LV_STATE_FOCUSED,
    LV_PART_MAIN | LV_STATE_FOCUS_KEY,
    LV_PART_MAIN | LV_STATE_PRESSED,
    LV_PART_MAIN | (LV_STATE_CHECKED | LV_STATE_FOCUSED),
    LV_PART_MAIN | (LV_STATE_CHECKED | LV_STATE_FOCUS_KEY),
};

void initRollerStyles(lv_coord_t pad = 2, lv_coord_t button_height = 12)
{
    if (s_roller_styles_ready)
        return;

    lv_style_init(&s_roller_root_style);
    lv_style_set_pad_all(&s_roller_root_style, pad);

    lv_style_init(&s_roller_button_style);
    lv_style_set_height(&s_roller_button_style, button_height);
    lv_style_set_pad_all(&s_roller_button_style, pad);
    lv_style_set_text_color(&s_roller_button_style, lv_color_hex(themePalette().fg));
    lv_style_set_text_line_space(&s_roller_button_style, 0);
    lv_style_set_text_letter_space(&s_roller_button_style, 0);

    lv_style_init(&s_roller_button_selected_style);
    lv_style_set_bg_color(&s_roller_button_selected_style, lv_color_hex(themePalette().highlight_bg));
    lv_style_set_bg_opa(&s_roller_button_selected_style, LV_OPA_COVER);
    lv_style_set_text_color(&s_roller_button_selected_style, lv_color_hex(themePalette().highlight_fg));

    s_roller_styles_ready = true;
}

void styleRollerRoot(lv_obj_t *obj)
{
    initRollerStyles();
    lv_obj_add_style(obj, &s_roller_root_style, LV_PART_MAIN);
}

void styleRollerButton(lv_obj_t *obj)
{
    initRollerStyles();
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
static lv_style_t s_map_marker_style;
static lv_style_t s_map_marker_alarm_style;
static lv_style_t s_map_range_ring_style;
static lv_style_t s_map_tile_placeholder_style;
static lv_style_t s_map_marker_label_style;
static lv_style_t s_map_range_label_style;
static bool s_map_styles_ready = false;

void initMapStyles()
{
    if (s_map_styles_ready)
        return;

    lv_style_init(&s_map_status_label_style);
    lv_style_set_text_color(&s_map_status_label_style, lv_color_hex(themePalette().fg));
    lv_style_set_text_align(&s_map_status_label_style, LV_TEXT_ALIGN_LEFT);

    lv_style_init(&s_map_opaque_surface_style);
    lv_style_set_bg_opa(&s_map_opaque_surface_style, LV_OPA_COVER);

    lv_style_init(&s_map_viewport_layout_style);
    lv_style_set_min_height(&s_map_viewport_layout_style, 140);

    lv_style_init(&s_map_toolbar_style);
    lv_style_set_radius(&s_map_toolbar_style, 4);
    lv_style_set_pad_all(&s_map_toolbar_style, 3);
    lv_style_set_pad_column(&s_map_toolbar_style, 3);

    lv_style_init(&s_map_toolbar_button_style);
    lv_style_set_radius(&s_map_toolbar_button_style, 3);
    lv_style_set_border_width(&s_map_toolbar_button_style, 1);
    lv_style_set_border_color(&s_map_toolbar_button_style, lv_color_hex(themePalette().accent));

    lv_style_init(&s_map_toolbar_button_checked_style);
    lv_style_set_border_color(&s_map_toolbar_button_checked_style, lv_color_hex(themePalette().success));

    lv_style_init(&s_map_toolbar_button_pressed_style);
    lv_style_set_border_color(&s_map_toolbar_button_pressed_style, lv_color_hex(themePalette().error));

    lv_style_init(&s_map_toolbar_button_disabled_style);
    lv_style_set_border_opa(&s_map_toolbar_button_disabled_style, effectiveOpacity(LV_OPA_50));

    lv_style_init(&s_map_toolbar_label_style);
    lv_style_set_text_color(&s_map_toolbar_label_style, lv_color_hex(themePalette().accent));

    lv_style_init(&s_map_toolbar_label_checked_style);
    lv_style_set_text_color(&s_map_toolbar_label_checked_style, lv_color_hex(themePalette().success));

    lv_style_init(&s_map_toolbar_label_pressed_style);
    lv_style_set_text_color(&s_map_toolbar_label_pressed_style, lv_color_hex(themePalette().error));

    lv_style_init(&s_map_toolbar_label_disabled_style);
    lv_style_set_text_color(&s_map_toolbar_label_disabled_style, lv_color_hex(0x607080));

    lv_style_init(&s_map_marker_style);
    lv_style_set_radius(&s_map_marker_style, LV_RADIUS_CIRCLE);
    lv_style_set_bg_color(&s_map_marker_style, lv_color_hex(themePalette().success));
    lv_style_set_bg_opa(&s_map_marker_style, LV_OPA_COVER);
    lv_style_set_border_width(&s_map_marker_style, 1);
    lv_style_set_border_color(&s_map_marker_style, lv_color_hex(themePalette().fg));

    lv_style_init(&s_map_marker_alarm_style);
    lv_style_set_bg_color(&s_map_marker_alarm_style, lv_color_hex(themePalette().error));

    lv_style_init(&s_map_range_ring_style);
    lv_style_set_radius(&s_map_range_ring_style, LV_RADIUS_CIRCLE);
    lv_style_set_border_width(&s_map_range_ring_style, 1);
    lv_style_set_border_color(&s_map_range_ring_style, lv_color_hex(themePalette().fg));

    lv_style_init(&s_map_tile_placeholder_style);
    lv_style_set_text_color(&s_map_tile_placeholder_style, lv_color_hex(0x606060));
    lv_style_set_text_align(&s_map_tile_placeholder_style, LV_TEXT_ALIGN_CENTER);

    lv_style_init(&s_map_marker_label_style);
    lv_style_set_text_color(&s_map_marker_label_style, lv_color_hex(themePalette().bg));

    lv_style_init(&s_map_range_label_style);
    lv_style_set_text_color(&s_map_range_label_style, lv_color_hex(themePalette().fg));

    s_map_styles_ready = true;
}

#endif

#if (defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS) || \
    (defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS)
static lv_style_t s_compass_hub_style;
static lv_style_t s_compass_needle_style;
static lv_style_t s_compass_label_style;
static bool s_compass_styles_ready = false;

void initCompassStyles()
{
    if (s_compass_styles_ready)
        return;

    lv_style_init(&s_compass_hub_style);
    lv_style_set_bg_color(&s_compass_hub_style, lv_color_hex(themePalette().panel_dark_bg));
    lv_style_set_bg_opa(&s_compass_hub_style, LV_OPA_COVER);
    lv_style_set_radius(&s_compass_hub_style, LV_RADIUS_CIRCLE);
#if defined(HELTEC_UI_HAS_TOUCH) && HELTEC_UI_HAS_TOUCH
    lv_style_set_bg_color(&s_compass_hub_style, lv_color_hex(themePalette().bg));
    lv_style_set_bg_grad_dir(&s_compass_hub_style, LV_GRAD_DIR_NONE);
    lv_style_set_border_width(&s_compass_hub_style, 0);
    lv_style_set_clip_corner(&s_compass_hub_style, false);
#else
    lv_style_set_border_width(&s_compass_hub_style, 1);
    lv_style_set_border_color(&s_compass_hub_style, lv_color_hex(themePalette().fg_on_dark));
    lv_style_set_clip_corner(&s_compass_hub_style, true);
#endif

    lv_style_init(&s_compass_needle_style);
    lv_style_set_radius(&s_compass_needle_style, LV_RADIUS_CIRCLE);
    lv_style_set_clip_corner(&s_compass_needle_style, true);

    lv_style_init(&s_compass_label_style);
    lv_style_set_text_align(&s_compass_label_style, LV_TEXT_ALIGN_LEFT);

    s_compass_styles_ready = true;
}

void styleCompassPlainOverlay(_lv_obj_t *obj)
{
    (void)obj;
}
#endif

} // namespace

bool applyLicenseGateTheme(_lv_obj_t *obj)
{
    if (!obj)
        return false;

#if LV_USE_MENU
    if (lv_obj_check_type(obj, &ht_context_menu_page_class))
    {
        if (s_context_menu_roller_styles.page)
            lv_obj_add_style(obj, s_context_menu_roller_styles.page, LV_PART_MAIN);
        return true;
    }
#endif

    if (lv_obj_check_type(obj, &ht_license_gate_class))
    {
        initLicenseGateStyles();
        lv_obj_add_style(obj, &s_license_gate_root_style, LV_PART_MAIN);
        return true;
    }

    if (!lv_obj_check_type(obj, &ht_license_gate_label_class) &&
        !lv_obj_check_type(obj, &ht_license_gate_chip_class))
    {
        return false;
    }

    initLicenseGateStyles();
    lv_obj_add_style(obj, &s_license_gate_label_style, LV_PART_MAIN);
    if (lv_obj_check_type(obj, &ht_license_gate_chip_class))
    {
        initLicenseGateStyles();
        lv_obj_add_style(obj, &s_license_gate_chip_style, LV_PART_MAIN);
    }
    return true;
}

bool applyRadialNavigationIconTheme(_lv_obj_t *icon)
{
#if !HELTEC_UI_HAS_TOUCH
    if (!icon || !lv_obj_check_type(icon, &ht_radial_navigation_icon_class))
        return false;
    initRadialIconStyles();
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

void configureComponentTheme()
{
    if (s_roller_styles_ready)
        return;
    constexpr lv_coord_t pad = 2;
    const lv_coord_t text_height = lv_font_get_line_height(LV_FONT_DEFAULT);
    const lv_coord_t button_height = text_height + (2 * pad);
    initRollerStyles(pad, button_height);
}

void setContextMenuRollerStyles(const UiContextMenuRollerStyles &styles)
{
    s_context_menu_roller_styles = styles;
}

bool applyButtonRollerTheme(_lv_obj_t *obj)
{
    if (!obj)
        return false;

    if (lv_obj_check_type(obj, &ht_context_menu_roller_class))
    {
        styleRollerRoot(obj);
        if (s_context_menu_roller_styles.page)
            lv_obj_add_style(obj, s_context_menu_roller_styles.page, LV_PART_MAIN);
        return true;
    }

    if (lv_obj_check_type(obj, &ht_context_menu_item_class))
    {
        styleRollerButton(obj);
        if (s_context_menu_roller_styles.item)
            lv_obj_add_style(obj, s_context_menu_roller_styles.item, LV_PART_MAIN);
        if (s_context_menu_roller_styles.item_selected)
        {
            for (lv_style_selector_t selector : kContextMenuRollerSelectedSelectors)
                lv_obj_add_style(obj, s_context_menu_roller_styles.item_selected, selector);
        }
        return true;
    }

    if (lv_obj_has_class(obj, &ht_button_roller_class))
    {
        styleRollerRoot(obj);
        return true;
    }

    if (lv_obj_check_type(obj, &ht_button_roller_item_class))
    {
        styleRollerButton(obj);
        return true;
    }

    return false;
}

#if defined(ENV_INCLUDE_MAP) && ENV_INCLUDE_MAP
bool applyMapWidgetTheme(_lv_obj_t *obj)
{
    if (!obj)
        return false;
    initMapStyles();

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

    if (lv_obj_check_type(obj, &ht_map_toolbar_button_class))
    {
        lv_obj_add_style(obj, &s_map_toolbar_button_style, LV_PART_MAIN);
        lv_obj_add_style(obj, &s_map_toolbar_button_checked_style, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_add_style(obj, &s_map_toolbar_button_pressed_style, LV_PART_MAIN | LV_STATE_PRESSED);
        lv_obj_add_style(obj, &s_map_toolbar_button_disabled_style,
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
    if (lv_obj_check_type(obj, &ht_map_tile_class))
    {
        lv_obj_add_style(obj, &s_map_opaque_surface_style, LV_PART_MAIN);
        return true;
    }
    if (lv_obj_check_type(obj, &ht_map_marker_class))
    {
        lv_obj_add_style(obj, &s_map_marker_style, LV_PART_MAIN);
        lv_obj_add_style(obj, &s_map_marker_alarm_style,
                         LV_PART_MAIN | LV_STATE_USER_1);
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

#else
bool applyMapWidgetTheme(_lv_obj_t *obj)
{
    (void)obj;
    return false;
}

#endif

#if (defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS) || \
    (defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS)
bool applyCompassTheme(_lv_obj_t *obj)
{
    if (!obj)
        return false;
    initCompassStyles();
    if (lv_obj_check_type(obj, &ht_compass_hub_class))
        lv_obj_add_style(obj, &s_compass_hub_style, LV_PART_MAIN);
    else if (lv_obj_check_type(obj, &ht_compass_needle_class))
    {
        styleCompassPlainOverlay(obj);
        lv_obj_add_style(obj, &s_compass_needle_style, LV_PART_MAIN);
    }
    else if (lv_obj_check_type(obj, &ht_compass_plain_class))
        styleCompassPlainOverlay(obj);
    else if (lv_obj_check_type(obj, &ht_compass_label_class))
        lv_obj_add_style(obj, &s_compass_label_style, LV_PART_MAIN);
    else
        return false;
    return true;
}
#else
bool applyCompassTheme(_lv_obj_t *obj)
{
    (void)obj;
    return false;
}
#endif

} // namespace Heltec::MeshCore::Ui
