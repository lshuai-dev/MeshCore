#include <lvgl.h>

#if LV_USE_THEME_MONO
#include "ui/app/BehaviorProfile.hpp"
#include "ui/theme/UiTheme.hpp"
#include "ui/theme/ThemeUtil.hpp"
#include "ui/Images.h"
#include "ui/menus/ContextMenu.hpp"
#include "ui/navigation/NavigationWidget.hpp"
#include "ui/theme/WidgetTheme.hpp"
#include "ThemeMonoFlat.hpp"

static lv_theme_t s_ui_mono_flat;
static lv_theme_t s_ui_ssd1306;
static lv_theme_t s_ui_mono_custom;

static bool s_widget_styles_inited = false;

static lv_color_t s_color_fg;
static lv_color_t s_color_bg;
static lv_color_t s_color_border;
static lv_color_t s_color_on;
static lv_color_t s_color_switch_bg;
static lv_color_t s_color_switch_indicator;

static lv_style_t s_switch_main;
static lv_style_t s_switch_main_focus;
static lv_style_t s_switch_indicator;
static lv_style_t s_switch_indicator_checked;
static lv_style_t s_switch_knob;
#if LV_COLOR_DEPTH == 1
static lv_style_t s_switch_knob_checked;
#endif
static lv_style_t s_cascading_page_style;
static lv_style_t s_cascading_item_style;
static lv_style_t s_cascading_item_selected_style;


static const Heltec::MeshCore::Ui::ThemePalette kMonoFlatColors = {
    0xFFFFFF, // fg
    0x000000, // bg
    0x000000, // fg_inv
    0xFFFFFF, // fg_on_dark
    0x000000, // panel_dark_bg
    0x00E060, // success
    0xFF3030, // error
    0x00E060, // bat_high
    0xFFB000, // bat_mid
    0xFF3030, // bat_low
    0x00FF00, // accent
    0xFFFF00, // accent_alt
    0x000000, // overlay_bg
    0xFFFFFF, // overlay_fg
    0xFFFFFF, // highlight_bg
    0x000000, // highlight_fg
    0x202020, // switch_bg
    0x303030, // switch_indicator
    0x000000, // panel_bg
    0xFFFFFF, // panel_border
    0xFFFFFF, // top_pane_bg
    0x000000, // top_pane_fg
    LV_OPA_COVER,
    0x000000, // frame_bg
    LV_OPA_COVER,
    2, // widget_radius_px
};

static constexpr uint16_t kContextMenuPageAnimMs = 0;
#if defined(HELTEC_RADIO_CORE) && HELTEC_RADIO_CORE
static constexpr lv_coord_t kContextMenuItemHeight = 16;
#else
static constexpr lv_coord_t kContextMenuItemHeight = 12;
#endif
static const Heltec::MeshCore::Ui::UiBehaviorProfile kMonoBehaviorProfile = {
    { kContextMenuPageAnimMs },
    { 5000, 560, 380, 4, 2 },
};

static const Heltec::MeshCore::Ui::UiThemeProfile kMonoNarrowThemeProfile = {
    { 2, 0, 0, 0, 12, 4 },
    { 12, 1, 3, 6 },
    { 0, 18, 8 },
};
static const Heltec::MeshCore::Ui::UiThemeProfile kMonoWideThemeProfile = {
    { 2, 0, 0, 0, 18, 4 },
    { 12, 1, 3, 6 },
    { 0, 24, 13 },
};
static const Heltec::MeshCore::Ui::UiThemeProfile kSsd1306ThemeProfile = {
    { 2, 0, 0, 0, 10, 2 },
    { static_cast<lv_coord_t>(icon_home_img.header.w), 0, 2, 6 },
    { 0, 15, 7 },
};

static void initDeviceStyles(lv_disp_t *disp, bool ssd1306)
{
    using namespace Heltec::MeshCore::Ui;
    setBehaviorProfile(&kMonoBehaviorProfile);

    if (ssd1306)
    {
        configureWidgetTheme(kSsd1306ThemeProfile);
    }
    else
    {
        const bool wide_display = disp && lv_disp_get_hor_res(disp) >= 200;
        configureWidgetTheme(wide_display ? kMonoWideThemeProfile
                                               : kMonoNarrowThemeProfile);
    }
}

static void styleSetBorderFrame(lv_style_t *st)
{
    lv_style_set_border_width(st, 1);
    lv_style_set_border_color(st, s_color_border);
}

static void initWidgetStyles(bool ssd1306)
{
    if (s_widget_styles_inited)
        return;

    s_color_fg = lv_color_hex(Heltec::MeshCore::Ui::themePalette().fg);
    s_color_bg = lv_color_hex(Heltec::MeshCore::Ui::themePalette().bg);
    s_color_border = s_color_fg;
    s_color_on = lv_color_hex(Heltec::MeshCore::Ui::themePalette().success);
    s_color_switch_bg = lv_color_hex(Heltec::MeshCore::Ui::themePalette().switch_bg);
    s_color_switch_indicator =
        lv_color_hex(Heltec::MeshCore::Ui::themePalette().switch_indicator);

    lv_style_init(&s_cascading_page_style);
    lv_style_set_width(&s_cascading_page_style, lv_pct(100));
    lv_style_set_height(&s_cascading_page_style, lv_pct(100));
    lv_style_set_pad_all(&s_cascading_page_style, 0);
    lv_style_set_pad_row(&s_cascading_page_style, 0);
    lv_style_set_border_width(&s_cascading_page_style, 0);
    lv_style_set_outline_width(&s_cascading_page_style, 0);

    lv_style_init(&s_cascading_item_style);
    lv_style_set_height(&s_cascading_item_style,
                        ssd1306 ? 10 : kContextMenuItemHeight);
    lv_style_set_pad_hor(&s_cascading_item_style, 4);
    lv_style_set_pad_ver(&s_cascading_item_style, ssd1306 ? 0 : 2);

    lv_style_init(&s_cascading_item_selected_style);
    lv_style_set_bg_color(&s_cascading_item_selected_style, s_color_fg);
    lv_style_set_bg_opa(&s_cascading_item_selected_style, LV_OPA_COVER);
    lv_style_set_text_color(&s_cascading_item_selected_style, s_color_bg);

    Heltec::MeshCore::Ui::setContextMenuRollerStyles(
        {&s_cascading_page_style, &s_cascading_item_style,
         &s_cascading_item_selected_style});
    lv_style_init(&s_switch_main);
    styleSetBorderFrame(&s_switch_main);
    lv_style_set_border_width(&s_switch_main, 2);
#if LV_COLOR_DEPTH == 1
    // One physical pixel is the thinnest useful frame on a 1-bit panel. Give
    // it a little more area so it does not dominate the control visually.
    lv_style_set_width(&s_switch_main, 24);
    lv_style_set_height(&s_switch_main, 12);
    lv_style_set_bg_color(&s_switch_main, s_color_bg);
#else
    lv_style_set_width(&s_switch_main, 30);
    lv_style_set_height(&s_switch_main, 16);
    lv_style_set_bg_color(&s_switch_main, s_color_switch_bg);
#endif
    lv_style_set_bg_opa(&s_switch_main, LV_OPA_COVER);
#if LV_COLOR_DEPTH == 1
    lv_style_set_pad_all(&s_switch_main, 1);
    lv_style_set_radius(&s_switch_main, LV_RADIUS_CIRCLE);
#else
    lv_style_set_pad_all(&s_switch_main, 1);
    lv_style_set_radius(&s_switch_main, 8);
#endif

    lv_style_init(&s_switch_main_focus);
    styleSetBorderFrame(&s_switch_main_focus);
#if LV_COLOR_DEPTH == 1
    lv_style_set_border_width(&s_switch_main_focus, 2);
    lv_style_set_bg_color(&s_switch_main_focus,
                          lv_color_hex(Heltec::MeshCore::Ui::themePalette().highlight_bg));
    lv_style_set_border_color(&s_switch_main_focus,
                              lv_color_hex(Heltec::MeshCore::Ui::themePalette().highlight_fg));
#else
    lv_style_set_border_width(&s_switch_main_focus, 1);
    lv_style_set_bg_color(&s_switch_main_focus, s_color_switch_bg);
    lv_style_set_border_color(&s_switch_main_focus,
                              lv_color_hex(Heltec::MeshCore::Ui::themePalette().highlight_fg));
#endif
    lv_style_set_bg_opa(&s_switch_main_focus, LV_OPA_COVER);

    lv_style_init(&s_switch_indicator);
#if LV_COLOR_DEPTH != 1
    lv_style_set_bg_color(&s_switch_indicator, s_color_switch_indicator);
#endif
    lv_style_set_bg_opa(&s_switch_indicator, LV_OPA_COVER);
#if LV_COLOR_DEPTH == 1
    lv_style_set_bg_color(&s_switch_indicator, s_color_bg);
    lv_style_set_radius(&s_switch_indicator, LV_RADIUS_CIRCLE);
#else
    lv_style_set_radius(&s_switch_indicator, 8);
#endif

    lv_style_init(&s_switch_indicator_checked);
#if LV_COLOR_DEPTH == 1
    // ON reverses the track to white; the checked knob below reverses to black.
    lv_style_set_bg_color(&s_switch_indicator_checked, s_color_fg);
#else
    lv_style_set_bg_color(&s_switch_indicator_checked, s_color_on);
#endif
    lv_style_set_bg_opa(&s_switch_indicator_checked, LV_OPA_COVER);
#if LV_COLOR_DEPTH == 1
    lv_style_set_radius(&s_switch_indicator_checked, LV_RADIUS_CIRCLE);
#else
    lv_style_set_radius(&s_switch_indicator_checked, 8);
#endif

    lv_style_init(&s_switch_knob);
    lv_style_set_bg_color(&s_switch_knob, s_color_fg);
    lv_style_set_bg_opa(&s_switch_knob, LV_OPA_COVER);
#if LV_COLOR_DEPTH != 1
    lv_style_set_border_width(&s_switch_knob, 1);
    lv_style_set_border_color(&s_switch_knob, s_color_border);
#endif
    lv_style_set_pad_all(&s_switch_knob, -2);
    lv_style_set_radius(&s_switch_knob, LV_RADIUS_CIRCLE);

#if LV_COLOR_DEPTH == 1
    lv_style_init(&s_switch_knob_checked);
    lv_style_set_bg_color(&s_switch_knob_checked, s_color_bg);
    lv_style_set_bg_opa(&s_switch_knob_checked, LV_OPA_COVER);
    lv_style_set_pad_all(&s_switch_knob_checked, -2);
    lv_style_set_radius(&s_switch_knob_checked, LV_RADIUS_CIRCLE);
#endif

    s_widget_styles_inited = true;
}

#if LV_USE_SWITCH
static void monoFlatApplySwitch(lv_obj_t *obj)
{
    lv_obj_add_style(obj, &s_switch_main, LV_PART_MAIN);
    lv_obj_add_style(obj, &s_switch_main_focus, LV_PART_MAIN | LV_STATE_FOCUS_KEY);
    lv_obj_add_style(obj, &s_switch_main_focus, LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_add_style(obj, &s_switch_indicator, LV_PART_INDICATOR);
    lv_obj_add_style(obj, &s_switch_indicator_checked, LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_add_style(obj, &s_switch_knob, LV_PART_KNOB);
#if LV_COLOR_DEPTH == 1
    lv_obj_add_style(obj, &s_switch_knob_checked, LV_PART_KNOB | LV_STATE_CHECKED);
#endif
}
#endif

static void applyMonoTheme(lv_theme_t *th, lv_obj_t *obj)
{
    if (!obj)
        return;
    (void)th;

    Heltec::MeshCore::Ui::applyWidgetTheme(obj);

#if LV_USE_SWITCH
    if (lv_obj_check_type(obj, &lv_switch_class))
    {
        monoFlatApplySwitch(obj);
        return;
    }
#endif
}

namespace Heltec::MeshCore::Ui
{

static lv_theme_t *initMonoFlatThemeVariant(lv_disp_t *disp, const lv_font_t *font,
                                                bool ssd1306, lv_theme_t &theme_storage,
                                                const ThemePalette &colors)
{
    if (!disp)
        return nullptr;
    setThemePalette(&colors);
    initDeviceStyles(disp, ssd1306);
    initWidgetStyles(ssd1306);

    if (!font)
        font = LV_FONT_DEFAULT;

    lv_memset_00(&theme_storage, sizeof(theme_storage));
    theme_storage.disp = disp;
    theme_storage.color_primary = lv_color_hex(themePalette().accent);
    theme_storage.color_secondary = lv_color_hex(themePalette().bg);
    theme_storage.font_small = font;
    theme_storage.font_normal = font;
    theme_storage.font_large = font;
    theme_storage.apply_cb = applyMonoTheme;
    setThemePalette(&colors);
    lv_disp_set_theme(disp, &theme_storage);
    return &theme_storage;
}

lv_theme_t *initMonoTheme(lv_disp_t *disp, const lv_font_t *font)
{
    return initMonoFlatThemeVariant(disp, font, false, s_ui_mono_flat, kMonoFlatColors);
}

lv_theme_t *initSsd1306Theme(lv_disp_t *disp, const lv_font_t *font)
{
    return initMonoFlatThemeVariant(disp, font, true, s_ui_ssd1306, kMonoFlatColors);
}

lv_theme_t *initMonoThemeWithPalette(lv_disp_t *disp, const lv_font_t *font,
                                                const ThemePalette &colors)
{
    return initMonoFlatThemeVariant(disp, font, false, s_ui_mono_custom, colors);
}

} // namespace Heltec::MeshCore::Ui
#endif // LV_USE_THEME_MONO
