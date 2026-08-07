#include "ui/app/ui_theme.hpp"

#if LV_USE_THEME_MONO && defined(UI_THEME_MONO) && UI_THEME_MONO
#include "ui/theme/ui_theme_mono_flat.hpp"
#endif

#if defined(UI_THEME_COLOR) && UI_THEME_COLOR
#include "ui/theme/ui_theme_pixel.hpp"
#endif

namespace heltec::meshcore::ui {
namespace {

lv_style_t s_screen_focus_highlight;
bool s_screen_focus_highlight_ready = false;
lv_style_t s_screen_row_focus_highlight;
bool s_screen_row_focus_highlight_ready = false;
lv_style_t s_screen_focus_control;
bool s_screen_focus_control_ready = false;

}  // namespace

void ui_theme_apply_focus_frame(_lv_obj_t* frame) {
  if (!frame) return;
  if (!s_screen_focus_highlight_ready) {
    lv_style_init(&s_screen_focus_highlight);
#if defined(HELTEC_V4_R8_TFT)
    lv_style_set_bg_color(&s_screen_focus_highlight, ui_color_accent());
    lv_style_set_bg_opa(&s_screen_focus_highlight, LV_OPA_30);
    lv_style_set_border_width(&s_screen_focus_highlight, 0);
    lv_style_set_border_opa(&s_screen_focus_highlight, LV_OPA_TRANSP);
    lv_style_set_border_side(&s_screen_focus_highlight, LV_BORDER_SIDE_NONE);
    lv_style_set_outline_width(&s_screen_focus_highlight, 1);
    lv_style_set_outline_color(&s_screen_focus_highlight, ui_color_accent());
    lv_style_set_outline_opa(&s_screen_focus_highlight, LV_OPA_COVER);
    lv_style_set_outline_pad(&s_screen_focus_highlight, 0);
#else
    lv_style_set_bg_opa(&s_screen_focus_highlight, LV_OPA_TRANSP);
    lv_style_set_border_width(&s_screen_focus_highlight, LV_DPX(1));
    lv_style_set_border_color(&s_screen_focus_highlight,
                              ui_color_highlight_bg());
    lv_style_set_border_opa(&s_screen_focus_highlight, LV_OPA_COVER);
    lv_style_set_border_side(&s_screen_focus_highlight, LV_BORDER_SIDE_FULL);
    lv_style_set_outline_width(&s_screen_focus_highlight, 0);
    lv_style_set_outline_opa(&s_screen_focus_highlight, LV_OPA_TRANSP);
#endif
    lv_style_set_radius(&s_screen_focus_highlight, ui_widget_radius_px());
    s_screen_focus_highlight_ready = true;
  }
  lv_obj_add_style(frame, &s_screen_focus_highlight,
                   LV_PART_MAIN | LV_STATE_FOCUSED);
  lv_obj_add_style(frame, &s_screen_focus_highlight,
                   LV_PART_MAIN | LV_STATE_FOCUS_KEY);
}

void ui_theme_apply_focus_row(_lv_obj_t* row) {
  if (!row) return;
#if defined(HELTEC_V4_R8_TFT)
  if (!s_screen_row_focus_highlight_ready) {
    lv_style_init(&s_screen_row_focus_highlight);
    lv_style_set_bg_color(&s_screen_row_focus_highlight, ui_color_accent());
    lv_style_set_bg_opa(&s_screen_row_focus_highlight, LV_OPA_30);
    lv_style_set_border_width(&s_screen_row_focus_highlight, 0);
    lv_style_set_border_opa(&s_screen_row_focus_highlight, LV_OPA_TRANSP);
    lv_style_set_border_side(&s_screen_row_focus_highlight,
                             LV_BORDER_SIDE_NONE);
    lv_style_set_outline_width(&s_screen_row_focus_highlight, 0);
    lv_style_set_outline_opa(&s_screen_row_focus_highlight, LV_OPA_TRANSP);
    lv_style_set_shadow_width(&s_screen_row_focus_highlight, 0);
    lv_style_set_shadow_opa(&s_screen_row_focus_highlight, LV_OPA_TRANSP);
    lv_style_set_radius(&s_screen_row_focus_highlight, ui_widget_radius_px());
    s_screen_row_focus_highlight_ready = true;
  }
  lv_obj_add_style(row, &s_screen_row_focus_highlight,
                   LV_PART_MAIN | LV_STATE_FOCUSED);
  lv_obj_add_style(row, &s_screen_row_focus_highlight,
                   LV_PART_MAIN | LV_STATE_FOCUS_KEY);
#else
  ui_theme_apply_focus_frame(row);
#endif
}

void ui_theme_apply_focus_control(_lv_obj_t* control) {
  if (!control) return;
  if (!s_screen_focus_control_ready) {
    lv_style_init(&s_screen_focus_control);
    lv_style_set_border_width(&s_screen_focus_control, 0);
    lv_style_set_border_opa(&s_screen_focus_control, LV_OPA_TRANSP);
    lv_style_set_outline_width(&s_screen_focus_control, 0);
    lv_style_set_outline_opa(&s_screen_focus_control, LV_OPA_TRANSP);
    s_screen_focus_control_ready = true;
  }
  lv_obj_add_style(control, &s_screen_focus_control,
                   LV_PART_MAIN | LV_STATE_FOCUSED);
  lv_obj_add_style(control, &s_screen_focus_control,
                   LV_PART_MAIN | LV_STATE_FOCUS_KEY);
}

void ui_theme_apply_switch_row_focus(_lv_obj_t* row, _lv_obj_t* sw) {
  if (!row || !sw) return;
#if defined(UI_THEME_COLOR) && UI_THEME_COLOR
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
  constexpr lv_coord_t kDropdownOptionHeight = 32;
  lv_obj_t* const label = lv_obj_get_child(list, 0);
  const lv_font_t* const font =
      label ? lv_obj_get_style_text_font(label, LV_PART_MAIN)
            : lv_obj_get_style_text_font(list, LV_PART_MAIN);
  const lv_coord_t font_height = font ? lv_font_get_line_height(font) : 0;
  const lv_coord_t line_space = font_height < kDropdownOptionHeight
                                    ? kDropdownOptionHeight - font_height
                                    : 0;

  lv_obj_set_style_text_line_space(list, line_space, LV_PART_MAIN);
  lv_obj_set_style_text_line_space(list, line_space, LV_PART_SELECTED);
  if (label) lv_obj_set_style_text_line_space(label, line_space, LV_PART_MAIN);
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

void ui_dropdown_fit_list_to_viewport(_lv_obj_t* dropdown, _lv_obj_t* viewport,
                                      _lv_obj_t* scroll_parent) {
#if LV_USE_DROPDOWN != 0
  if (!dropdown || !viewport || !lv_obj_is_valid(dropdown) ||
      !lv_obj_is_valid(viewport) || !lv_dropdown_is_open(dropdown)) {
    return;
  }

  _lv_obj_t* const list = lv_dropdown_get_list(dropdown);
  if (!list || !lv_obj_is_valid(list)) return;

  lv_obj_update_layout(viewport);
  lv_obj_update_layout(dropdown);
  lv_obj_update_layout(list);

  _lv_obj_t* const label = lv_obj_get_child(list, 0);
  const lv_coord_t border = lv_obj_get_style_border_width(list, LV_PART_MAIN);
  lv_coord_t content_height =
      label ? lv_obj_get_height(label) : lv_obj_get_height(list);
  content_height += lv_obj_get_style_pad_top(list, LV_PART_MAIN) +
                    lv_obj_get_style_pad_bottom(list, LV_PART_MAIN) + border * 2;

  lv_area_t viewport_area{};
  lv_area_t dropdown_area{};
  lv_obj_get_coords(viewport, &viewport_area);
  lv_obj_get_coords(dropdown, &dropdown_area);
  const lv_coord_t viewport_height = lv_area_get_height(&viewport_area);
  const lv_coord_t dropdown_height = lv_area_get_height(&dropdown_area);
  const lv_coord_t max_popup_height =
      viewport_height > dropdown_height ? viewport_height - dropdown_height : 0;
  if (content_height <= 0 || max_popup_height <= 0) return;

  const lv_coord_t desired_height =
      content_height < max_popup_height ? content_height : max_popup_height;
  const bool prefer_above = lv_dropdown_get_dir(dropdown) == LV_DIR_TOP;

  lv_coord_t available_below = viewport_area.y2 - dropdown_area.y2;
  lv_coord_t available_above = dropdown_area.y1 - viewport_area.y1;
  if (available_below < 0) available_below = 0;
  if (available_above < 0) available_above = 0;

  if (scroll_parent && lv_obj_is_valid(scroll_parent) &&
      lv_obj_has_flag(scroll_parent, LV_OBJ_FLAG_SCROLLABLE)) {
    const lv_coord_t available = prefer_above ? available_above : available_below;
    if (available < desired_height) {
      lv_coord_t delta = desired_height - available;
      const lv_coord_t capacity =
          prefer_above ? lv_obj_get_scroll_top(scroll_parent)
                       : lv_obj_get_scroll_bottom(scroll_parent);
      if (delta > capacity) delta = capacity;
      if (delta > 0) {
        const lv_coord_t current = lv_obj_get_scroll_y(scroll_parent);
        lv_obj_scroll_to_y(scroll_parent,
                           prefer_above ? current - delta : current + delta,
                           LV_ANIM_OFF);
        lv_obj_update_layout(scroll_parent);
        lv_obj_update_layout(viewport);
        lv_obj_update_layout(dropdown);
        lv_obj_get_coords(viewport, &viewport_area);
        lv_obj_get_coords(dropdown, &dropdown_area);
        available_below = viewport_area.y2 - dropdown_area.y2;
        available_above = dropdown_area.y1 - viewport_area.y1;
        if (available_below < 0) available_below = 0;
        if (available_above < 0) available_above = 0;
      }
    }
  }

  bool place_above = prefer_above;
  if (prefer_above) {
    if (available_above < desired_height && available_below > available_above) {
      place_above = false;
    }
  } else if (available_below < desired_height &&
             available_above > available_below) {
    place_above = true;
  }

  const lv_coord_t available_height =
      place_above ? available_above : available_below;
  if (available_height <= 0) return;
  const lv_coord_t popup_height =
      desired_height < available_height ? desired_height : available_height;
  lv_obj_set_height(list, popup_height);
  lv_obj_align_to(list, dropdown,
                  place_above ? LV_ALIGN_OUT_TOP_LEFT
                              : LV_ALIGN_OUT_BOTTOM_LEFT,
                  0, 0);
  lv_obj_update_layout(list);
#else
  (void)dropdown;
  (void)viewport;
  (void)scroll_parent;
#endif
}

void ui_theme_center_dropdown_value(_lv_obj_t* dropdown) {
  if (!dropdown) return;
#if defined(HELTEC_V4_R8_TFT)
  lv_obj_update_layout(dropdown);
  const lv_coord_t height = lv_obj_get_height(dropdown);
  const lv_coord_t border =
      lv_obj_get_style_border_width(dropdown, LV_PART_MAIN);
  const lv_font_t* const font =
      lv_obj_get_style_text_font(dropdown, LV_PART_MAIN);
  const lv_coord_t font_height = font ? lv_font_get_line_height(font) : 0;
  const lv_coord_t free_height = height > font_height + border * 2
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
