#include "top_pane.hpp"
#include "ui/app/ui_theme.hpp"
#include "ui/core/ht_meta_data.hpp"
#include <lvgl.h>

namespace heltec::meshcore::ui {
namespace {

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
  lv_obj_set_width(_root, lv_pct(100));
  lv_obj_set_flex_flow(_root, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(_root, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_clear_flag(_root, LV_OBJ_FLAG_SCROLLABLE);

  _lv_obj_t* left_slot = ht_obj_create(_root, meta_id::TopPaneLeftSlot);
  if (!left_slot) {
    lv_obj_del(_root);
    _root = nullptr;
    return false;
  }
  lv_obj_set_height(left_slot, lv_pct(100));
  lv_obj_set_flex_grow(left_slot, 1);
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
  lv_obj_set_flex_flow(right_slot, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(right_slot, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_clear_flag(right_slot, LV_OBJ_FLAG_SCROLLABLE);

  _bat_cont = ht_obj_create(right_slot, meta_id::TopPaneBattery);
  if (!_bat_cont) {
    lv_obj_del(_root);
    _root = nullptr;
    return false;
  }
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
  lv_obj_align(_bat_fill, LV_ALIGN_LEFT_MID, 0, 0);
  lv_obj_add_flag(_bat_fill, LV_OBJ_FLAG_FLOATING);
  lv_obj_clear_flag(_bat_fill, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t* bat_cap = ht_obj_create(_bat_cont, meta_id::TopPaneBatteryCap);
  if (!bat_cap) {
    lv_obj_del(_root);
    _root = _bat_fill = nullptr;
    return false;
  }
  lv_obj_align_to(bat_cap, bat_outline, LV_ALIGN_OUT_RIGHT_MID, -1, 0);
  lv_obj_clear_flag(bat_cap, LV_OBJ_FLAG_SCROLLABLE);
#endif

  lv_obj_update_layout(_root);
  lv_obj_invalidate(_root);

  renderBattery();
  return true;
}

void TopPane::setTitle(const char* text) {
  if (_title) lv_label_set_text_static(_title, text ? text : "");
}

void TopPane::setBatteryStatus(uint16_t millivolts, uint8_t percent) {
  _bat_present = millivolts > 0;
  _bat_percent = percent > 100 ? 100 : percent;
  renderBattery();
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

  const uint8_t pct = self->_bat_percent;
  const bool present = self->_bat_present;
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
  if (self) self->renderBattery();
}
#endif

void TopPane::renderBattery() {
#if defined(HELTEC_V4_R8_TFT)
  if (_bat_cont) lv_obj_invalidate(_bat_cont);
#else
  if (!_bat_fill) return;
  lv_obj_t* outline = lv_obj_get_parent(_bat_fill);
  if (!outline) return;

  const BatteryFillBox box = battery_fill_box(outline);
  if (box.w < 1 || box.h < 1) return;

  const uint8_t pct = _bat_percent;
  lv_coord_t fill_w = (box.w * pct + 99) / 100;
  if (pct > 0 && fill_w < 1) fill_w = 1;
  if (fill_w > box.w) fill_w = box.w;

  lv_obj_set_size(_bat_fill, fill_w, box.h);
  lv_obj_align(_bat_fill, LV_ALIGN_TOP_LEFT, 0, 0);
  apply_battery_state(_bat_fill, pct, _bat_present);
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
