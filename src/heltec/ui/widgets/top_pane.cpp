#include "top_pane.hpp"
#include "ui/app/ui_theme.hpp"
#include "ui/core/ht_meta_data.hpp"
#include "ui/theme/ui_theme_metrics.hpp"
#include <lvgl.h>

#ifndef BAT_MIN_MV
#ifdef BATT_MIN_MILLIVOLTS
#define BAT_MIN_MV BATT_MIN_MILLIVOLTS
#else
#define BAT_MIN_MV 3000
#endif
#endif
#ifndef BAT_MAX_MV
#ifdef BATT_MAX_MILLIVOLTS
#define BAT_MAX_MV BATT_MAX_MILLIVOLTS
#else
#define BAT_MAX_MV 4200
#endif
#endif

namespace heltec::meshcore::ui {
namespace {

uint8_t battery_percent_from_mv(uint16_t mv) {
  int pct = 0;
  if (mv > BAT_MIN_MV) {
    pct = ((int)mv - BAT_MIN_MV) * 100 / ((int)BAT_MAX_MV - BAT_MIN_MV);
  }
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  return static_cast<uint8_t>(pct);
}

#if defined(HELTEC_V4_R8_TFT)
lv_color_t battery_fill_color(uint8_t pct) {
#if LV_COLOR_DEPTH == 1
  LV_UNUSED(pct);
  return ui_color_top_pane_fg();
#else
  if (pct < 20) return ui_color_battery_low();
  if (pct < 60) return ui_color_battery_mid();
  return ui_color_battery_high();
#endif
}

void draw_solid_rect(lv_draw_ctx_t* draw_ctx, const lv_area_t& area, lv_color_t color) {
  if (!draw_ctx || area.x2 < area.x1 || area.y2 < area.y1) return;

  lv_draw_rect_dsc_t dsc;
  lv_draw_rect_dsc_init(&dsc);
  dsc.bg_opa = LV_OPA_COVER;
  dsc.bg_color = color;
  dsc.border_width = 0;
  dsc.radius = 0;
  lv_draw_rect(draw_ctx, &dsc, &area);
}

void draw_battery_outline(lv_draw_ctx_t* draw_ctx, const lv_area_t& body, lv_color_t color) {
  lv_area_t line;

  lv_area_set(&line, body.x1, body.y1, body.x2, body.y1);
  draw_solid_rect(draw_ctx, line, color);
  lv_area_set(&line, body.x1, body.y2, body.x2, body.y2);
  draw_solid_rect(draw_ctx, line, color);
  lv_area_set(&line, body.x1, body.y1, body.x1, body.y2);
  draw_solid_rect(draw_ctx, line, color);
  lv_area_set(&line, body.x2, body.y1, body.x2, body.y2);
  draw_solid_rect(draw_ctx, line, color);
}
#else
struct BatteryFillBox {
  lv_coord_t w;
  lv_coord_t h;
};

static lv_style_t s_battery_fill_empty_style;
static lv_style_t s_battery_fill_low_style;
static lv_style_t s_battery_fill_mid_style;
static lv_style_t s_battery_fill_high_style;
static bool s_battery_fill_styles_ready = false;

lv_coord_t clamp_nonnegative(lv_coord_t v) {
  return v > 0 ? v : 0;
}

BatteryFillBox battery_fill_box(_lv_obj_t* outline) {
  if (!outline) return {0, 0};

  lv_obj_update_layout(outline);

  lv_coord_t w = lv_obj_get_content_width(outline);
  lv_coord_t h = lv_obj_get_content_height(outline);
  if (w < 1 || h < 1) {
    const lv_coord_t border = lv_obj_get_style_border_width(outline, LV_PART_MAIN);
    const lv_coord_t pad_w = lv_obj_get_style_pad_left(outline, LV_PART_MAIN) +
                             lv_obj_get_style_pad_right(outline, LV_PART_MAIN);
    const lv_coord_t pad_h = lv_obj_get_style_pad_top(outline, LV_PART_MAIN) +
                             lv_obj_get_style_pad_bottom(outline, LV_PART_MAIN);
    if (w < 1) w = lv_obj_get_width(outline) - border * 2 - pad_w;
    if (h < 1) h = lv_obj_get_height(outline) - border * 2 - pad_h;
  }

  return {clamp_nonnegative(w), clamp_nonnegative(h)};
}

void init_battery_fill_styles() {
  if (s_battery_fill_styles_ready) return;

  lv_style_init(&s_battery_fill_empty_style);
  lv_style_set_bg_opa(&s_battery_fill_empty_style, LV_OPA_TRANSP);

  lv_style_init(&s_battery_fill_low_style);
  lv_style_set_bg_opa(&s_battery_fill_low_style, LV_OPA_COVER);
#if LV_COLOR_DEPTH == 1
  lv_style_set_bg_color(&s_battery_fill_low_style, ui_color_top_pane_fg());
#else
  lv_style_set_bg_color(&s_battery_fill_low_style, ui_color_battery_low());
#endif

  lv_style_init(&s_battery_fill_mid_style);
  lv_style_set_bg_opa(&s_battery_fill_mid_style, LV_OPA_COVER);
#if LV_COLOR_DEPTH == 1
  lv_style_set_bg_color(&s_battery_fill_mid_style, ui_color_top_pane_fg());
#else
  lv_style_set_bg_color(&s_battery_fill_mid_style, ui_color_battery_mid());
#endif

  lv_style_init(&s_battery_fill_high_style);
  lv_style_set_bg_opa(&s_battery_fill_high_style, LV_OPA_COVER);
#if LV_COLOR_DEPTH == 1
  lv_style_set_bg_color(&s_battery_fill_high_style, ui_color_top_pane_fg());
#else
  lv_style_set_bg_color(&s_battery_fill_high_style, ui_color_battery_high());
#endif

  s_battery_fill_styles_ready = true;
}

void install_battery_fill_styles(_lv_obj_t* fill) {
  if (!fill) return;
  init_battery_fill_styles();
  lv_obj_add_style(fill, &s_battery_fill_empty_style, LV_PART_MAIN);
  lv_obj_add_style(fill, &s_battery_fill_low_style,
                   LV_PART_MAIN | LV_STATE_USER_1);
  lv_obj_add_style(fill, &s_battery_fill_mid_style,
                   LV_PART_MAIN | LV_STATE_USER_2);
  lv_obj_add_style(fill, &s_battery_fill_high_style,
                   LV_PART_MAIN | LV_STATE_USER_3);
}

void apply_battery_state(_lv_obj_t* fill, uint8_t pct, bool present) {
  if (!fill) return;
  lv_obj_clear_state(fill, LV_STATE_USER_1 | LV_STATE_USER_2 | LV_STATE_USER_3);
  if (!present || pct == 0) return;
  if (pct < 20) {
    lv_obj_add_state(fill, LV_STATE_USER_1);
  } else if (pct < 60) {
    lv_obj_add_state(fill, LV_STATE_USER_2);
  } else {
    lv_obj_add_state(fill, LV_STATE_USER_3);
  }
}
#endif

}  // namespace

bool TopPane::create(_lv_obj_t* parent) {
  if (!parent) return false;

  _root = ht_obj_create(parent, meta_id::TopPaneRoot);
  if (!_root) return false;
  const UiTopPaneMetrics& metrics = ui_top_pane_metrics(_root);
  const UiAppFrameMetrics& frame = ui_app_frame_metrics(_root);
  const lv_coord_t pad_left =
      (frame.frame_margin_left > 0 || frame.frame_margin_right > 0)
          ? frame.frame_margin_left
          : metrics.radius;
  const lv_coord_t pad_right =
      (frame.frame_margin_left > 0 || frame.frame_margin_right > 0)
          ? frame.frame_margin_right
          : metrics.radius;
  const lv_coord_t battery_h =
      metrics.battery_h > 0 ? metrics.battery_h
                             : static_cast<lv_coord_t>((metrics.height * 7 + 5) / 10);
  lv_obj_set_size(_root, lv_pct(100), metrics.height);
  lv_obj_set_flex_flow(_root, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(_root, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_top(_root, LV_MAX(0, metrics.pad_top), LV_PART_MAIN);
  lv_obj_set_style_pad_bottom(_root, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_left(_root, pad_left, LV_PART_MAIN);
  lv_obj_set_style_pad_right(_root, pad_right, LV_PART_MAIN);
  lv_obj_clear_flag(_root, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(_root, onRootSizeChanged, LV_EVENT_SIZE_CHANGED, this);

  _lv_obj_t* left_slot = ht_obj_create(_root, meta_id::TopPaneLeftSlot);
  if (!left_slot) {
    lv_obj_del(_root);
    _root = nullptr;
    return false;
  }
  lv_obj_set_height(left_slot, lv_pct(100));
  lv_obj_set_flex_grow(left_slot, 1);
  lv_obj_set_style_pad_all(left_slot, 0, LV_PART_MAIN);
  lv_obj_clear_flag(left_slot, LV_OBJ_FLAG_SCROLLABLE);

  _lv_obj_t* center_slot = ht_obj_create(_root, meta_id::TopPaneCenterSlot);
  if (!center_slot) {
    lv_obj_del(_root);
    _root = nullptr;
    return false;
  }
  lv_obj_set_height(center_slot, lv_pct(100));
  lv_obj_set_flex_grow(center_slot, 2);
  lv_obj_set_flex_flow(center_slot, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(center_slot, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_all(center_slot, 0, LV_PART_MAIN);
  lv_obj_clear_flag(center_slot, LV_OBJ_FLAG_SCROLLABLE);

  _title = ht_label_create(center_slot, meta_id::TopPaneTitle, "");
  if (!_title) {
    lv_obj_del(_root);
    _root = nullptr;
    return false;
  }
  lv_obj_set_size(_title, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_align(_title, LV_ALIGN_CENTER, 0, 0);
  lv_obj_clear_flag(_title, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ON_FOCUS);
  lv_label_set_long_mode(_title, LV_LABEL_LONG_SCROLL_CIRCULAR);
  lv_obj_add_flag(_title, LV_OBJ_FLAG_EVENT_BUBBLE);

#if defined(HELTEC_TOPBAR_TOUCH_SHELL) && HELTEC_TOPBAR_TOUCH_SHELL
  _touch_slot = center_slot;
#endif

  _lv_obj_t* right_slot = ht_obj_create(_root, meta_id::TopPaneRightSlot);
  if (!right_slot) {
    lv_obj_del(_root);
    _root = _title = nullptr;
    return false;
  }
  lv_obj_set_height(right_slot, lv_pct(100));
  lv_obj_set_flex_grow(right_slot, 1);
  lv_obj_set_style_pad_all(right_slot, 0, LV_PART_MAIN);
  lv_obj_clear_flag(right_slot, LV_OBJ_FLAG_SCROLLABLE);

  _bat_cont = ht_obj_create(_root, meta_id::TopPaneBattery);
  if (!_bat_cont) {
    lv_obj_del(_root);
    _root = nullptr;
    return false;
  }
  lv_obj_set_size(_bat_cont, metrics.battery_w, battery_h);
  lv_obj_add_flag(_bat_cont, LV_OBJ_FLAG_FLOATING);
  // The cap is aligned against the right edge of this container. Some LVGL
  // layouts round the final coordinate one pixel outside the content box, so
  // keep child overflow visible on every display profile.
  lv_obj_add_flag(_bat_cont, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
  lv_obj_clear_flag(_bat_cont, LV_OBJ_FLAG_SCROLLABLE);

#if defined(HELTEC_V4_R8_TFT)
  lv_obj_add_event_cb(_bat_cont, onBatteryDraw, LV_EVENT_DRAW_MAIN, this);
#else
  lv_obj_t* bat_outline = ht_obj_create(_bat_cont, meta_id::TopPaneBatteryOutline);
  if (!bat_outline) {
    lv_obj_del(_root);
    _root = nullptr;
    return false;
  }
  const lv_coord_t cap_w = LV_MIN(6, LV_MAX(2, static_cast<lv_coord_t>((battery_h + 1) / 3)));
  const lv_coord_t body_w = metrics.battery_w - cap_w + 1;
  lv_obj_set_size(bat_outline, body_w, battery_h);
  lv_obj_align(bat_outline, LV_ALIGN_LEFT_MID, 0, 0);
  lv_obj_add_flag(bat_outline, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
  lv_obj_clear_flag(bat_outline, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(bat_outline, onBatteryOutlineSizeChanged, LV_EVENT_SIZE_CHANGED, this);

  _bat_fill = ht_obj_create(bat_outline, meta_id::TopPaneBatteryFill);
  if (!_bat_fill) {
    lv_obj_del(_root);
    _root = nullptr;
    return false;
  }
  lv_obj_set_size(_bat_fill, 0, LV_MAX(0, battery_h - 2));
  lv_obj_align(_bat_fill, LV_ALIGN_LEFT_MID, 0, 0);
  lv_obj_add_flag(_bat_fill, LV_OBJ_FLAG_FLOATING);
  lv_obj_clear_flag(_bat_fill, LV_OBJ_FLAG_SCROLLABLE);
  install_battery_fill_styles(_bat_fill);

  lv_obj_t* bat_cap = ht_obj_create(_bat_cont, meta_id::TopPaneBatteryCap);
  if (!bat_cap) {
    lv_obj_del(_root);
    _root = _bat_fill = nullptr;
    return false;
  }
  lv_obj_set_size(bat_cap, cap_w, LV_MAX(2, battery_h / 2));
  lv_obj_align_to(bat_cap, bat_outline, LV_ALIGN_OUT_RIGHT_MID, -1, 0);
  lv_obj_clear_flag(bat_cap, LV_OBJ_FLAG_SCROLLABLE);
#endif

  lv_obj_update_layout(_root);
  layoutBattery();
  lv_obj_invalidate(_root);

  renderBattery(0);
  return true;
}

void TopPane::setTitle(const char* text) {
  if (_title) lv_label_set_text_static(_title, text ? text : "");
}

void TopPane::setBatteryMilliVolts(uint16_t mv) {
  _bat_mv = mv;
  renderBattery(_bat_mv);
}

void TopPane::onRootSizeChanged(lv_event_t* e) {
  if (!e || lv_event_get_code(e) != LV_EVENT_SIZE_CHANGED) return;
  auto* self = static_cast<TopPane*>(lv_event_get_user_data(e));
  if (self) self->layoutBattery();
}

void TopPane::layoutBattery() {
  if (!_root || !_bat_cont) return;

  const UiTopPaneMetrics& metrics = ui_top_pane_metrics(_root);
  const lv_coord_t margin_r =
      metrics.battery_pad_r >= 0
          ? metrics.battery_pad_r
          : static_cast<lv_coord_t>((metrics.height + 2) / 3);
  const lv_coord_t parent_w = lv_obj_get_content_width(_root);
  const lv_coord_t parent_h = lv_obj_get_content_height(_root);
  const lv_coord_t battery_w = lv_obj_get_width(_bat_cont);
  const lv_coord_t battery_h = lv_obj_get_height(_bat_cont);
  const lv_coord_t x = LV_MAX(0, static_cast<lv_coord_t>(parent_w - battery_w - margin_r));
  const lv_coord_t y = LV_MAX(0, static_cast<lv_coord_t>((parent_h - battery_h) / 2));

  lv_obj_set_pos(_bat_cont, x, y);
}

#if defined(HELTEC_V4_R8_TFT)
void TopPane::onBatteryDraw(lv_event_t* e) {
  if (!e || lv_event_get_code(e) != LV_EVENT_DRAW_MAIN) return;

  auto* self = static_cast<TopPane*>(lv_event_get_user_data(e));
  lv_obj_t* obj = lv_event_get_target(e);
  lv_draw_ctx_t* draw_ctx = lv_event_get_draw_ctx(e);
  if (!self || !obj || !draw_ctx) return;

  lv_area_t coords;
  lv_obj_get_coords(obj, &coords);
  const lv_coord_t w = lv_area_get_width(&coords);
  const lv_coord_t h = lv_area_get_height(&coords);
  if (w < 8 || h < 6) return;

  const lv_coord_t cap_w = (w >= 12 ? 3 : 2);
  lv_coord_t cap_h = h >= 12 ? 6 : h - 4;
  if (cap_h < 3) cap_h = 3;
  if (cap_h > h - 2) cap_h = h - 2;

  const lv_coord_t body_w = w - cap_w;
  if (body_w < 4) return;

  lv_area_t body;
  lv_area_set(&body, coords.x1, coords.y1, coords.x1 + body_w - 1, coords.y2);

  lv_area_t cap;
  const lv_coord_t cap_y = body.y1 + (h - cap_h) / 2;
  lv_area_set(&cap, body.x2 + 1, cap_y, coords.x2, cap_y + cap_h - 1);

  const uint8_t pct = battery_percent_from_mv(self->_bat_mv);
  const bool present = self->_bat_mv > 0;
  const lv_coord_t inner_w = body_w - 2;
  const lv_coord_t inner_h = h - 2;
  if (present && pct > 0 && inner_w > 0 && inner_h > 0) {
    lv_coord_t fill_w = (inner_w * pct + 99) / 100;
    if (fill_w < 1) fill_w = 1;
    if (fill_w > inner_w) fill_w = inner_w;

    lv_area_t fill;
    lv_area_set(&fill, body.x1 + 1, body.y1 + 1,
                body.x1 + fill_w, body.y2 - 1);
    draw_solid_rect(draw_ctx, fill, battery_fill_color(pct));
  }

  const lv_color_t chrome = lv_color_white();
  draw_battery_outline(draw_ctx, body, chrome);
  draw_solid_rect(draw_ctx, cap, chrome);
}

#else
void TopPane::onBatteryOutlineSizeChanged(lv_event_t* e) {
  if (!e || lv_event_get_code(e) != LV_EVENT_SIZE_CHANGED) return;
  auto* self = static_cast<TopPane*>(lv_event_get_user_data(e));
  if (self) self->renderBattery(self->_bat_mv);
}
#endif

void TopPane::renderBattery(uint16_t mv) {
#if defined(HELTEC_V4_R8_TFT)
  LV_UNUSED(mv);
  if (_bat_cont) lv_obj_invalidate(_bat_cont);
#else
  if (!_bat_fill) return;
  lv_obj_t* outline = lv_obj_get_parent(_bat_fill);
  if (!outline) return;

  const BatteryFillBox box = battery_fill_box(outline);
  if (box.w < 1 || box.h < 1) return;

  const uint8_t pct = battery_percent_from_mv(mv);
  lv_coord_t fill_w = (box.w * pct + 99) / 100;
  if (pct > 0 && fill_w < 1) fill_w = 1;
  if (fill_w > box.w) fill_w = box.w;

  lv_obj_set_size(_bat_fill, fill_w, box.h);
  lv_obj_align(_bat_fill, LV_ALIGN_TOP_LEFT, 0, 0);
  apply_battery_state(_bat_fill, pct, mv > 0);
  lv_obj_invalidate(outline);
#endif
}

#if defined(HELTEC_TOPBAR_TOUCH_SHELL) && HELTEC_TOPBAR_TOUCH_SHELL

void TopPane::enableTouchShell(lv_event_cb_t on_short_press, lv_event_cb_t on_long_press,
                              void* user_data) {
  if (!_touch_slot || !on_short_press) return;
  lv_obj_add_flag(_touch_slot, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(_touch_slot, on_short_press, LV_EVENT_SHORT_CLICKED, user_data);
  if (on_long_press) {
    lv_obj_add_event_cb(_touch_slot, on_long_press, LV_EVENT_LONG_PRESSED, user_data);
  }
}

#endif

}  // namespace heltec::meshcore::ui
