#include "ui_theme_pixel.hpp"

#include "ui/app/ui_behavior_profile.hpp"
#include "ui/app/ui_theme.hpp"
#include "ui/core/ht_meta_data.hpp"
#include "ui/menus/context_menu.hpp"
#include "ui/navigation/ui_navigator.hpp"
#include "ui/theme/ui_widget_theme.hpp"

#include <lvgl.h>

namespace heltec::meshcore::ui
{
namespace
{

static lv_style_t s_switch_main;
static lv_style_t s_switch_main_focus;
static lv_style_t s_switch_indicator;
static lv_style_t s_switch_indicator_checked;
static lv_style_t s_switch_knob;
static lv_style_t s_widget_radius;
static lv_style_t s_cascading_page_style;
static lv_style_t s_cascading_item_style;
static lv_style_t s_cascading_item_selected_style;
static lv_style_t s_cascading_label_style;
static lv_style_t s_cascading_label_selected_style;
static bool s_styles_ready = false;

static lv_color_t s_color_fg;
static lv_color_t s_color_border;
static lv_color_t s_color_on;

static const UiThemeColors kPixelColors = {
    0x142850, // fg
    0x142850, // bg
    0xFFFFFF, // fg_inv
    0xFFFFFF, // fg_on_dark
    0x102040, // panel_dark_bg
    0x3A7BC8, // success
    0xD04040, // error
    0xE0A030, // warning
    0x00E060, // bat_high
    0xFFB000, // bat_mid
    0xFF3030, // bat_low
    0x4A90D9, // accent
    0xFFFF00, // accent_alt
    0x142850, // overlay_bg
    0xFFFFFF, // overlay_fg
    0xFFFFFF, // highlight_bg
    0x142850, // highlight_fg
    0x9AB8D8, // switch_bg
    0x7AA0C8, // switch_indicator
    0xB8D4F0, // panel_bg
    0xFFFFFF, // panel_border
    0x142850, // top_pane_bg
    0xFFFFFF, // top_pane_fg
    LV_OPA_TRANSP,
    0x142850, // frame_bg
    LV_OPA_TRANSP,
    2, // widget_radius_px
};

static const UiBehaviorProfile kPixelBehaviorProfile = {
    { 0 },
    { 5000, 560, 380, 4, 2 },
};

#if HELTEC_UI_INPUT_TOUCH
static const UiThemeProfile kPixelThemeProfile = {
    { 8, 10, 22, 8, 32, 12 },
    { 8, 1, 3, 18 },
    { 10, 5, 24, 14, 10 },
};
#else
static const UiThemeProfile kPixelNarrowThemeProfile = {
    { 6, 8, 22, 0, 25, 0 },
    { 12, 1, 3, 6 },
    { 8, 5, 22, 12, 4 },
};
static const UiThemeProfile kPixelWideThemeProfile = {
    { 6, 8, 22, 0, 25, 0 },
    { 10, 1, 3, 18 },
    { 8, 5, 22, 12, 4 },
};
#endif

static void initDeviceStyles(lv_disp_t *disp)
{
    ui_set_behavior_profile(&kPixelBehaviorProfile);

#if HELTEC_UI_INPUT_TOUCH
    (void)disp;
    ui_widget_theme_configure(kPixelThemeProfile);
#else
    const bool wide_display = disp && lv_disp_get_hor_res(disp) >= 200;
    ui_widget_theme_configure(wide_display ? kPixelWideThemeProfile
                                           : kPixelNarrowThemeProfile);
#endif
}

static void styleSetBorderFrame(lv_style_t *st)
{
#if HELTEC_UI_INPUT_TOUCH
    lv_style_set_border_width(st, 0);
    lv_style_set_border_opa(st, LV_OPA_TRANSP);
    lv_style_set_border_side(st, LV_BORDER_SIDE_NONE);
#else
    lv_style_set_border_width(st, 1);
    lv_style_set_border_color(st, s_color_border);
    lv_style_set_border_opa(st, LV_OPA_COVER);
#endif
    lv_style_set_outline_width(st, 0);
    lv_style_set_radius(st, ui_widget_radius_px());
}

static void initWidgetStyles()
{
    if (s_styles_ready)
        return;

    s_color_fg = ui_color_fg();
    s_color_border = ui_color_panel_border();
    s_color_on = ui_color_success();

    lv_style_init(&s_cascading_page_style);
    lv_style_set_width(&s_cascading_page_style, lv_pct(100));
    lv_style_set_height(&s_cascading_page_style, lv_pct(100));
    lv_style_set_pad_all(&s_cascading_page_style, 0);
    lv_style_set_pad_row(&s_cascading_page_style, 0);
    lv_style_set_border_width(&s_cascading_page_style, 0);
    lv_style_set_outline_width(&s_cascading_page_style, 0);

    lv_style_init(&s_cascading_item_style);
    lv_style_set_height(&s_cascading_item_style, 12);
    lv_style_set_pad_hor(&s_cascading_item_style, 4);
    lv_style_set_pad_ver(&s_cascading_item_style, 2);
    lv_style_set_bg_opa(&s_cascading_item_style, LV_OPA_TRANSP);

    lv_style_init(&s_cascading_item_selected_style);
    lv_style_set_bg_color(&s_cascading_item_selected_style, ui_color_highlight_bg());
    lv_style_set_bg_opa(&s_cascading_item_selected_style, LV_OPA_COVER);
    lv_style_set_text_color(&s_cascading_item_selected_style, ui_color_highlight_fg());

    lv_style_init(&s_cascading_label_style);
    lv_style_set_text_color(&s_cascading_label_style, ui_color_fg());

    lv_style_init(&s_cascading_label_selected_style);
    lv_style_set_text_color(&s_cascading_label_selected_style, ui_color_highlight_fg());

    ui_widget_theme_set_cascading_menu_styles(
        {&s_cascading_page_style, &s_cascading_item_style,
         &s_cascading_item_selected_style, &s_cascading_label_style,
         &s_cascading_label_selected_style});

    lv_style_init(&s_switch_main);
    styleSetBorderFrame(&s_switch_main);
#if !HELTEC_UI_INPUT_TOUCH
    lv_style_set_border_width(&s_switch_main, 2);
#endif
    lv_style_set_width(&s_switch_main, 30);
    lv_style_set_height(&s_switch_main, 16);
    lv_style_set_bg_color(&s_switch_main, ui_color_switch_bg());
    lv_style_set_bg_opa(&s_switch_main, LV_OPA_COVER);
    lv_style_set_pad_all(&s_switch_main, 1);

    lv_style_init(&s_switch_main_focus);
    styleSetBorderFrame(&s_switch_main_focus);
#if !HELTEC_UI_INPUT_TOUCH
    lv_style_set_border_width(&s_switch_main_focus, 1);
#endif
    lv_style_set_border_color(&s_switch_main_focus, ui_color_highlight_fg());
    lv_style_set_bg_color(&s_switch_main_focus, ui_color_switch_bg());
    lv_style_set_bg_opa(&s_switch_main_focus, LV_OPA_COVER);

    lv_style_init(&s_switch_indicator);
    styleSetBorderFrame(&s_switch_indicator);
    lv_style_set_border_width(&s_switch_indicator, 0);
    lv_style_set_border_opa(&s_switch_indicator, LV_OPA_TRANSP);
    lv_style_set_bg_color(&s_switch_indicator, ui_color_switch_indicator());
    lv_style_set_bg_opa(&s_switch_indicator, LV_OPA_COVER);

    lv_style_init(&s_switch_indicator_checked);
    lv_style_set_bg_color(&s_switch_indicator_checked, s_color_on);
    lv_style_set_bg_opa(&s_switch_indicator_checked, LV_OPA_COVER);
    lv_style_set_radius(&s_switch_indicator_checked, ui_widget_radius_px());

    lv_style_init(&s_switch_knob);
    lv_style_set_bg_color(&s_switch_knob, s_color_border);
    lv_style_set_bg_opa(&s_switch_knob, LV_OPA_COVER);
#if HELTEC_UI_INPUT_TOUCH
    lv_style_set_border_width(&s_switch_knob, 0);
    lv_style_set_border_opa(&s_switch_knob, LV_OPA_TRANSP);
    lv_style_set_border_side(&s_switch_knob, LV_BORDER_SIDE_NONE);
#else
    lv_style_set_border_width(&s_switch_knob, 1);
    lv_style_set_border_color(&s_switch_knob, s_color_fg);
#endif
    lv_style_set_pad_all(&s_switch_knob, -2);
    lv_style_set_radius(&s_switch_knob, ui_widget_radius_px());

    lv_style_init(&s_widget_radius);
    lv_style_set_radius(&s_widget_radius, ui_widget_radius_px());
    lv_style_set_bg_opa(&s_widget_radius, LV_OPA_TRANSP);
    lv_style_set_border_width(&s_widget_radius, 0);
    lv_style_set_outline_width(&s_widget_radius, 0);
    lv_style_set_shadow_width(&s_widget_radius, 0);
    lv_style_set_text_color(&s_widget_radius, ui_color_fg());

    s_styles_ready = true;
}

static void applySwitch(lv_obj_t *obj)
{
    lv_obj_add_style(obj, &s_switch_main, LV_PART_MAIN);
    lv_obj_add_style(obj, &s_switch_main_focus, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_add_style(obj, &s_switch_main_focus, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_add_style(obj, &s_switch_indicator, LV_PART_INDICATOR);
    lv_obj_add_style(obj, &s_switch_indicator_checked, LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_add_style(obj, &s_switch_knob, LV_PART_KNOB);
}

extern "C" void ui_pixel_theme_apply(lv_theme_t *th, lv_obj_t *obj)
{
    if (!obj)
        return;
    (void)th;

    const bool custom_applied = ui_widget_theme_apply(obj);

#if LV_USE_SWITCH
    if (lv_obj_check_type(obj, &lv_switch_class))
    {
        applySwitch(obj);
        return;
    }
#endif

    if (!custom_applied)
        lv_obj_add_style(obj, &s_widget_radius, LV_PART_MAIN);
}

} // namespace

lv_theme_t *init_ui_pixel_theme(lv_disp_t *disp, const lv_font_t *font)
{
    if (!disp)
        return nullptr;
    ui_set_active_theme_colors(&kPixelColors);
    initDeviceStyles(disp);
    initWidgetStyles();

    if (!font)
        font = LV_FONT_DEFAULT;

    static UiTheme pixel_theme;
    lv_memset_00(&pixel_theme, sizeof(pixel_theme));
    pixel_theme.theme.disp = disp;
    pixel_theme.theme.color_primary = ui_color_accent();
    pixel_theme.theme.color_secondary = ui_color_bg();
    pixel_theme.theme.font_small = font;
    pixel_theme.theme.font_normal = font;
    pixel_theme.theme.font_large = font;
    pixel_theme.theme.apply_cb = ui_pixel_theme_apply;
    ui_set_active_theme_colors(&kPixelColors);
    lv_disp_set_theme(disp, &pixel_theme.theme);
    return &pixel_theme.theme;
}

} // namespace heltec::meshcore::ui
