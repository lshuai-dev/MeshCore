#include "ui/theme/UiTheme.hpp"

#include <lvgl.h>

#if LV_USE_THEME_DEFAULT
#include "extra/themes/default/lv_theme_default.h"
#endif

#if LV_USE_THEME_MONO
#include "ui/theme/ThemeMonoFlat.hpp"
#endif
#if defined(HELTEC_HAS_KEY_FRAME) && HELTEC_HAS_KEY_FRAME
#include "E213UiTheme.hpp"
#endif

#if !LV_USE_THEME_MONO
#include "ui/theme/ThemePixel.hpp"
#endif

namespace Heltec::MeshCore::Ui
{

namespace
{
static const ThemePalette *s_active_colors = nullptr;

static const ThemePalette &fallbackColors()
{
    static const ThemePalette colors = defaultThemePalette();
    return colors;
}

} // namespace

ThemePalette defaultThemePalette()
{
    return {
        0xFFFFFF,     // fg
        0x000000,     // bg
        0x000000,     // fg_inv
        0xFFFFFF,     // fg_on_dark
        0x000000,     // panel_dark_bg
        0x00E060,     // success
        0xFF3030,     // error
        0x00E060,     // bat_high
        0xFFB000,     // bat_mid
        0xFF3030,     // bat_low
        0x00FF00,     // accent
        0xFFFF00,     // accent_alt
        0x000000,     // overlay_bg
        0xFFFFFF,     // overlay_fg
        0xFFFFFF,     // highlight_bg
        0x000000,     // highlight_fg
        0x202020,     // switch_bg
        0x303030,     // switch_indicator
        0x000000,     // panel_bg
        0xFFFFFF,     // panel_border
        0xFFFFFF,     // top_pane_bg
        0x000000,     // top_pane_fg
        LV_OPA_COVER, // top_pane_bg_opa
        0x000000,     // frame_bg
        LV_OPA_COVER, // frame_bg_opa
        2,            // widget_radius_px
    };
}

const ThemePalette &themePalette()
{
    return s_active_colors ? *s_active_colors : fallbackColors();
}

void setThemePalette(const ThemePalette *colors)
{
    s_active_colors = colors;
}

bool initUiTheme(lv_disp_t *display)
{
    if (!display)
        return false;

#if !LV_USE_THEME_MONO
    const lv_font_t *font =
#if defined(LV_FONT_UNSCII_8) && LV_FONT_UNSCII_8
        &lv_font_unscii_8;
#else
        LV_FONT_DEFAULT;
#endif
    return initPixelTheme(display, font) != nullptr;

#elif LV_USE_THEME_MONO
    const lv_font_t *font =
#if defined(LV_FONT_UNSCII_8) && LV_FONT_UNSCII_8
        &lv_font_unscii_8;
#else
        LV_FONT_DEFAULT;
#endif
#if defined(HELTEC_HAS_KEY_FRAME) && HELTEC_HAS_KEY_FRAME
    return initE213Theme(display, font) != nullptr;
#elif LV_COLOR_DEPTH == 1
    return initSsd1306Theme(display, font) != nullptr;
#else
    return initMonoTheme(display, font) != nullptr;
#endif

#elif LV_USE_THEME_DEFAULT
    static const ThemePalette default_colors = defaultThemePalette();
    setThemePalette(&default_colors);
    const bool dark_theme = (themePalette().bg == 0x000000);
    lv_theme_t *th =
        lv_theme_default_init(display, lv_palette_main(LV_PALETTE_BLUE),
                              lv_palette_main(LV_PALETTE_GREY), dark_theme, LV_FONT_DEFAULT);
    if (!th)
        return false;
    return true;

#else
    (void)display;
    static const ThemePalette default_colors = defaultThemePalette();
    setThemePalette(&default_colors);
    return true;
#endif
}

} // namespace Heltec::MeshCore::Ui
