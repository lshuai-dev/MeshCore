#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS

#include "compass_dial_widget.hpp"

#include "ui/app/ui_theme.hpp"
#include "ui/core/ht_meta_data.hpp"
#include "ui/theme/ui_widget_theme.hpp"

namespace heltec::meshcore::ui {
namespace {

void on_col_size_changed(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_SIZE_CHANGED) return;
  auto* dial = static_cast<CompassDialWidget*>(lv_event_get_user_data(e));
  if (dial) dial->layoutSize();
}

void on_ring_draw(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_DRAW_MAIN) return;
  auto* dial = static_cast<CompassDialWidget*>(lv_event_get_user_data(e));
  if (!dial || dial->side <= 0 || dial->dialHidden()) return;

  lv_draw_ctx_t* draw_ctx = lv_event_get_draw_ctx(e);
  lv_obj_t* obj = lv_event_get_target(e);
  lv_area_t area;
  lv_obj_get_coords(obj, &area);
  const lv_coord_t cx = dial->side / 2;
  const lv_coord_t ax = (lv_coord_t)(area.x1 + cx);
  const lv_coord_t ay = (lv_coord_t)(area.y1 + cx);
  const float hdg = (float)dial->ring_heading_tenths / 10.f;
  compass_draw_dial_ring(draw_ctx, ax, ay, dial->side, hdg, ui_color_fg_on_dark());
}

void on_needle_draw(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_DRAW_MAIN) return;
  auto* dial = static_cast<CompassDialWidget*>(lv_event_get_user_data(e));
  if (!dial || dial->side <= 0 || dial->dialHidden()) return;

  lv_draw_ctx_t* draw_ctx = lv_event_get_draw_ctx(e);
  lv_obj_t* obj = lv_event_get_target(e);
  lv_area_t area;
  lv_obj_get_coords(obj, &area);
  const lv_coord_t cx = dial->side / 2;
  const lv_coord_t ax = (lv_coord_t)(area.x1 + cx);
  const lv_coord_t ay = (lv_coord_t)(area.y1 + cx);

  if (dial->needle_kind == CompassDialNeedleKind::FriendTurn) {
    const lv_coord_t needle_r = (lv_coord_t)(dial->side / 2 - 4);
    compass_draw_friend_needle(draw_ctx, ax, ay, needle_r, dial->friend_turn_deg, dial->friend_gps_fix,
                               dial->friend_on_target);
  } else {
    compass_draw_dial_needle(draw_ctx, ax, ay, dial->side, dial->needle_heading_tenths);
  }
}

static lv_style_t s_compass_plain_overlay_style;
static lv_style_t s_compass_hub_style;
static lv_style_t s_compass_needle_style;
static lv_style_t s_compass_info_label_style;
static lv_style_t s_compass_q_label_style;
static lv_style_t s_compass_q_success_style;
static lv_style_t s_compass_q_error_style;
static bool s_compass_styles_ready = false;

void init_compass_styles() {
  if (s_compass_styles_ready) return;

  lv_style_init(&s_compass_plain_overlay_style);
  lv_style_set_bg_opa(&s_compass_plain_overlay_style, LV_OPA_TRANSP);
  lv_style_set_border_width(&s_compass_plain_overlay_style, 0);

  lv_style_init(&s_compass_hub_style);
  lv_style_set_bg_color(&s_compass_hub_style, ui_color_panel_dark_bg());
  lv_style_set_bg_opa(&s_compass_hub_style, LV_OPA_COVER);
  lv_style_set_radius(&s_compass_hub_style, LV_RADIUS_CIRCLE);
  lv_style_set_border_width(&s_compass_hub_style, 1);
  lv_style_set_border_color(&s_compass_hub_style, ui_color_fg_on_dark());
  lv_style_set_border_opa(&s_compass_hub_style, LV_OPA_COVER);
  lv_style_set_clip_corner(&s_compass_hub_style, true);

  lv_style_init(&s_compass_needle_style);
  lv_style_set_radius(&s_compass_needle_style, LV_RADIUS_CIRCLE);
  lv_style_set_clip_corner(&s_compass_needle_style, true);

  lv_style_init(&s_compass_info_label_style);
  lv_style_set_text_color(&s_compass_info_label_style, ui_color_fg());
  lv_style_set_text_align(&s_compass_info_label_style, LV_TEXT_ALIGN_LEFT);

  lv_style_init(&s_compass_q_label_style);
  lv_style_set_text_color(&s_compass_q_label_style, ui_color_fg());
  lv_style_set_text_align(&s_compass_q_label_style, LV_TEXT_ALIGN_LEFT);

  lv_style_init(&s_compass_q_success_style);
  lv_style_set_text_color(&s_compass_q_success_style, ui_color_success());

  lv_style_init(&s_compass_q_error_style);
  lv_style_set_text_color(&s_compass_q_error_style, ui_color_error());

  s_compass_styles_ready = true;
}

}  // namespace

static void style_compass_plain_overlay(_lv_obj_t* obj) {
  if (!obj) return;
  init_compass_styles();
  lv_obj_add_style(obj, &s_compass_plain_overlay_style, LV_PART_MAIN);
}

bool ui_compass_widget_apply_theme(_lv_obj_t* obj) {
  if (!obj) return false;
  init_compass_styles();

  switch (ht_id(obj)) {
    case meta_id::CompassInfoColumn:
      style_compass_plain_overlay(obj);
      return true;

    case meta_id::CompassQRow:
      style_compass_plain_overlay(obj);
      return true;

    case meta_id::CompassDialCenter:
      style_compass_plain_overlay(obj);
      return true;

    case meta_id::CompassDialHub:
      lv_obj_add_style(obj, &s_compass_hub_style, LV_PART_MAIN);
      return true;

    case meta_id::CompassDialRing:
      style_compass_plain_overlay(obj);
      return true;

    case meta_id::CompassDialNeedle:
      style_compass_plain_overlay(obj);
      lv_obj_add_style(obj, &s_compass_needle_style, LV_PART_MAIN);
      return true;

    case meta_id::CompassInfoLabel:
      lv_obj_add_style(obj, &s_compass_info_label_style, LV_PART_MAIN);
      return true;

    case meta_id::CompassQLabel:
      lv_obj_add_style(obj, &s_compass_q_label_style, LV_PART_MAIN);
      lv_obj_add_style(obj, &s_compass_q_success_style,
                       LV_PART_MAIN | LV_STATE_USER_1);
      lv_obj_add_style(obj, &s_compass_q_error_style,
                       LV_PART_MAIN | LV_STATE_USER_2);
      return true;

    default:
      return false;
  }
}

bool CompassDialWidget::create(lv_obj_t* parent_row) {
  if (!parent_row) return false;

  center_col = ht_obj_create(parent_row, meta_id::CompassDialCenter);
  if (!center_col) return false;
  lv_obj_set_height(center_col, lv_pct(100));
  lv_obj_set_flex_grow(center_col, 1);
  lv_obj_set_flex_flow(center_col, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(center_col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_all(center_col, 0, LV_PART_MAIN);
  lv_obj_clear_flag(center_col, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(center_col, LV_OBJ_FLAG_OVERFLOW_VISIBLE);

  hub = ht_obj_create(center_col, meta_id::CompassDialHub);
  if (!hub) return false;
  lv_obj_set_style_pad_all(hub, 0, LV_PART_MAIN);
  lv_obj_clear_flag(hub, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(hub, LV_OBJ_FLAG_OVERFLOW_VISIBLE);

  ring = ht_obj_create(hub, meta_id::CompassDialRing);
  if (!ring) return false;
  lv_obj_set_style_pad_all(ring, 0, LV_PART_MAIN);
  lv_obj_clear_flag(ring, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(ring, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
  lv_obj_add_event_cb(ring, on_ring_draw, LV_EVENT_DRAW_MAIN, this);

  needle = ht_obj_create(hub, meta_id::CompassDialNeedle);
  if (!needle) return false;
  lv_obj_set_style_pad_all(needle, 0, LV_PART_MAIN);
  lv_obj_clear_flag(needle, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(needle, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
  lv_obj_add_event_cb(needle, on_needle_draw, LV_EVENT_DRAW_MAIN, this);
  lv_obj_move_foreground(needle);

  lv_obj_add_event_cb(center_col, on_col_size_changed, LV_EVENT_SIZE_CHANGED, this);
  return true;
}

void CompassDialWidget::layoutLayers() {
  if (side < min_side) return;
  if (ring) {
    lv_obj_set_size(ring, side, side);
    lv_obj_center(ring);
  }
  if (needle) {
    lv_obj_set_size(needle, side, side);
    lv_obj_center(needle);
    lv_obj_move_foreground(needle);
  }
}

void CompassDialWidget::layoutSize() {
  if (!center_col || !hub || !ring || (flags & kLayoutBusy)) return;
  flags |= kLayoutBusy;

  const lv_coord_t w = lv_obj_get_content_width(center_col);
  const lv_coord_t h = lv_obj_get_content_height(center_col);
  if (w <= 0 || h <= 0) {
    flags &= ~kLayoutBusy;
    return;
  }

  lv_coord_t next = LV_MIN(w, h);
  if (next > edge_margin * 2) next = (lv_coord_t)(next - edge_margin * 2);
  if (next < min_side || next == side) {
    flags &= ~kLayoutBusy;
    return;
  }

  side = next;
  lv_obj_set_size(hub, side, side);
  layoutLayers();
  invalidateRing();
  invalidateNeedle();
  flags &= ~kLayoutBusy;
}

void CompassDialWidget::setLayersVisible(bool visible) {
  if (ring) {
    if (visible)
      lv_obj_clear_flag(ring, LV_OBJ_FLAG_HIDDEN);
    else
      lv_obj_add_flag(ring, LV_OBJ_FLAG_HIDDEN);
  }
  if (needle) {
    if (visible)
      lv_obj_clear_flag(needle, LV_OBJ_FLAG_HIDDEN);
    else
      lv_obj_add_flag(needle, LV_OBJ_FLAG_HIDDEN);
  }
}

void CompassDialWidget::invalidateRing() {
  if (ring) lv_obj_invalidate(ring);
}

void CompassDialWidget::invalidateNeedle() {
  if (needle) lv_obj_invalidate(needle);
}

}  // namespace heltec::meshcore::ui

#endif  // ENV_INCLUDE_COMPASS
