#include "e213_ui_theme.hpp"

#include "ui/app/ui_theme.hpp"
#include "ui/theme/ui_theme_mono_flat.hpp"

namespace heltec::meshcore::ui
{
namespace
{
static const UiThemeColors kE213ThemeColors = {
    0x000000, // fg
    0xFFFFFF, // bg
    0xFFFFFF, // fg_inv
    0xFFFFFF, // fg_on_dark
    0x000000, // panel_dark_bg
    0x000000, // success
    0x000000, // error
    0x000000, // warning
    0x000000, // bat_high
    0x000000, // bat_mid
    0x000000, // bat_low
    0x000000, // accent
    0x000000, // accent_alt
    0xFFFFFF, // overlay_bg
    0x000000, // overlay_fg
    0x000000, // highlight_bg
    0xFFFFFF, // highlight_fg
    0xFFFFFF, // switch_bg
    0x000000, // switch_indicator
    0xFFFFFF, // panel_bg
    0x000000, // panel_border
    0x000000, // top_pane_bg
    0xFFFFFF, // top_pane_fg
    LV_OPA_COVER,
    0xFFFFFF, // frame_bg
    LV_OPA_COVER,
    2, // widget_radius_px
};
} // namespace

lv_theme_t *init_ui_e213_theme(lv_disp_t *disp, const lv_font_t *font)
{
    return init_ui_mono_flat_theme_with_colors(disp, font, kE213ThemeColors);
}

} // namespace heltec::meshcore::ui
