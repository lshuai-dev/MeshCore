#include "ui/app/ui_theme.hpp"

#include <lvgl.h>

#if LV_USE_THEME_DEFAULT
#include "extra/themes/default/lv_theme_default.h"
#endif

#if LV_USE_THEME_MONO && defined(UI_THEME_MONO) && UI_THEME_MONO
#include "ui/theme/ui_theme_mono_flat.hpp"
#endif

#if LV_USE_THEME_DEFAULT && defined(UI_THEME_COLOR) && UI_THEME_COLOR
#include "ui/theme/ui_theme_pixel.hpp"
#endif
#include "ui/theme/ui_theme_metrics.hpp"

namespace heltec::meshcore::ui {
namespace {

static const UiThemeColors* s_active_colors = nullptr;

static const UiThemeColors& fallback_colors() {
  static const UiThemeColors colors = make_ui_theme_colors();
  return colors;
}

}  // namespace

UiThemeColors make_ui_theme_colors() {
  return {
      0xFFFFFF,
      0x000000,
      0x000000,
      0xFFFFFF,
      0x000000,
      0x00E060,
      0xFF3030,
      0xFFB000,
      0x00E060,
      0xFFB000,
      0xFF3030,
      0x00FF00,
      0xFFFF00,
      0x000000,
      0xFFFFFF,
      0xFFFFFF,
      0x000000,
      0x202020,
      0x303030,
      0x000000,
      0xFFFFFF,
      0xFFFFFF,
      0x000000,
      LV_OPA_COVER,
      0x000000,
      LV_OPA_COVER,
      2,
  };
}

const UiThemeColors& ui_theme_colors() {
  return s_active_colors ? *s_active_colors : fallback_colors();
}

void ui_set_active_theme_colors(const UiThemeColors* colors) {
  s_active_colors = colors;
}

bool ui_theme_init(lv_disp_t* disp) {
  if (!disp) return false;
  ui_theme_metrics_init();

#if LV_USE_THEME_DEFAULT && defined(UI_THEME_COLOR) && UI_THEME_COLOR
  const lv_font_t* ui_font =
#if defined(LV_FONT_UNSCII_8) && LV_FONT_UNSCII_8
      &lv_font_unscii_8;
#else
      LV_FONT_DEFAULT;
#endif
  return init_ui_pixel_theme(disp, ui_font) != nullptr;

#elif LV_USE_THEME_MONO && defined(UI_THEME_MONO) && UI_THEME_MONO
  const lv_font_t* ui_font =
#if defined(LV_FONT_UNSCII_8) && LV_FONT_UNSCII_8
      &lv_font_unscii_8;
#else
      LV_FONT_DEFAULT;
#endif
#if (defined(UI_THEME_SSD1306) && UI_THEME_SSD1306) || \
    (defined(HELTEC_DISPLAY_SSD1306) && HELTEC_DISPLAY_SSD1306) || LV_COLOR_DEPTH == 1
  return init_ui_ssd1306_theme(disp, ui_font) != nullptr;
#else
  return init_ui_mono_flat_theme(disp, ui_font) != nullptr;
#endif

#elif LV_USE_THEME_DEFAULT
  static UiThemeColors default_colors = make_ui_theme_colors();
  ui_set_active_theme_colors(&default_colors);
  const bool dark_theme = (ui_theme_colors().bg == 0x000000);
  lv_theme_t* th = lv_theme_default_init(
      disp,
      lv_palette_main(LV_PALETTE_BLUE),
      lv_palette_main(LV_PALETTE_GREY),
      dark_theme,
      LV_FONT_DEFAULT);
  if (!th) return false;
  return true;

#else
  (void)disp;
  static UiThemeColors default_colors = make_ui_theme_colors();
  ui_set_active_theme_colors(&default_colors);
  return true;
#endif
}

void ui_theme_apply_switch_row_focus(_lv_obj_t* row, _lv_obj_t* sw) {
  if (!row || !sw) return;
#if LV_USE_THEME_DEFAULT && defined(UI_THEME_COLOR) && UI_THEME_COLOR
  ui_pixel_apply_switch_row_focus(row, sw);
#elif LV_USE_THEME_MONO && defined(UI_THEME_MONO) && UI_THEME_MONO
  ui_mono_flat_apply_switch_row_focus(row, sw);
#else
  (void)row;
  (void)sw;
#endif
}

void ui_theme_apply_dropdown_list(_lv_obj_t* list) {
  if (!list) return;
  static const lv_style_selector_t selectors[] = {
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

  for (lv_style_selector_t selector : selectors) {
    lv_obj_set_style_border_width(list, 0, selector);
    lv_obj_set_style_border_opa(list, LV_OPA_TRANSP, selector);
    lv_obj_set_style_border_side(list, LV_BORDER_SIDE_NONE, selector);
    lv_obj_set_style_outline_width(list, 0, selector);
    lv_obj_set_style_outline_opa(list, LV_OPA_TRANSP, selector);
    lv_obj_set_style_shadow_width(list, 0, selector);
    lv_obj_set_style_shadow_opa(list, LV_OPA_TRANSP, selector);
  }

  lv_obj_set_style_bg_color(list, ui_color_panel_bg(), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(list, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_text_color(list, ui_color_fg(), LV_PART_MAIN);
  lv_obj_set_style_text_opa(list, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_bg_color(list, ui_color_highlight_bg(), LV_PART_SELECTED);
  lv_obj_set_style_bg_opa(list, LV_OPA_COVER, LV_PART_SELECTED);
  lv_obj_set_style_text_color(list, ui_color_highlight_fg(), LV_PART_SELECTED);

#if defined(HELTEC_V4_R8_TFT)
  // LVGL derives both the visual row and pointer hit-test height from the
  // label's font height plus text line spacing. Outer list padding alone does
  // not make individual dropdown options easier to tap.
  constexpr lv_coord_t kDropdownOptionHeight = 32;
  lv_obj_t* const label = lv_obj_get_child(list, 0);
  const lv_font_t* const font = label
                                    ? lv_obj_get_style_text_font(label, LV_PART_MAIN)
                                    : lv_obj_get_style_text_font(list, LV_PART_MAIN);
  const lv_coord_t font_height = font ? lv_font_get_line_height(font) : 0;
  const lv_coord_t line_space =
      font_height < kDropdownOptionHeight ? kDropdownOptionHeight - font_height : 0;

  lv_obj_set_style_text_line_space(list, line_space, LV_PART_MAIN);
  lv_obj_set_style_text_line_space(list, line_space, LV_PART_SELECTED);
  if (label) {
    lv_obj_set_style_text_line_space(label, line_space, LV_PART_MAIN);
  }
#endif
}

void ui_theme_match_dropdown_list_padding(_lv_obj_t* dropdown,
                                          _lv_obj_t* list) {
  if (!dropdown || !list) return;
#if defined(HELTEC_V4_R8_TFT)
  const lv_coord_t pad_left =
      lv_obj_get_style_pad_left(dropdown, LV_PART_MAIN);
  const lv_coord_t pad_right =
      lv_obj_get_style_pad_right(dropdown, LV_PART_MAIN);
  lv_obj_set_style_pad_left(list, pad_left, LV_PART_MAIN);
  lv_obj_set_style_pad_right(list, pad_right, LV_PART_MAIN);
#else
  (void)dropdown;
  (void)list;
#endif
}

void ui_theme_center_dropdown_value(_lv_obj_t* dropdown) {
  if (!dropdown) return;
#if defined(HELTEC_V4_R8_TFT)
  // Fixed-height dropdowns can still have stale coordinates while their Flex
  // parent is being built. Resolve the layout before deriving vertical pads.
  lv_obj_update_layout(dropdown);
  const lv_coord_t height = lv_obj_get_height(dropdown);
  const lv_coord_t border =
      lv_obj_get_style_border_width(dropdown, LV_PART_MAIN);
  const lv_font_t* const font =
      lv_obj_get_style_text_font(dropdown, LV_PART_MAIN);
  const lv_coord_t font_height = font ? lv_font_get_line_height(font) : 0;
  const lv_coord_t free_height =
      height > font_height + border * 2
          ? height - font_height - border * 2
          : 0;
  const lv_coord_t pad_top = free_height / 2;
  const lv_coord_t pad_bottom = free_height - pad_top;
  lv_obj_set_style_pad_top(dropdown, pad_top, LV_PART_MAIN);
  lv_obj_set_style_pad_bottom(dropdown, pad_bottom, LV_PART_MAIN);
#else
  (void)dropdown;
#endif
}

}  // namespace heltec::meshcore::ui
