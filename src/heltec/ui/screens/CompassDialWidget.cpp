#if (defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS) || \
    (defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS)
#include "CompassDialWidget.hpp"
#include "ui/theme/UiTheme.hpp"
#include "ui/theme/WidgetTheme.hpp"

namespace Heltec::MeshCore::Ui
{
namespace
{

#if defined(HELTEC_UI_HAS_TOUCH) && HELTEC_UI_HAS_TOUCH
constexpr int16_t kTouchDialSinQ10[24] = {
    0, 265, 512, 724, 887, 989, 1024, 989, 887, 724, 512, 265,
    0, -265, -512, -724, -887, -989, -1024, -989, -887, -724, -512, -265,
};
constexpr int16_t kTouchDialCosQ10[24] = {
    1024, 989, 887, 724, 512, 265, 0, -265, -512, -724, -887, -989,
    -1024, -989, -887, -724, -512, -265, 0, 265, 512, 724, 887, 989,
};

void draw_touch_line(lv_draw_ctx_t *ctx, const lv_point_t &start, const lv_point_t &end,
                     lv_color_t color, lv_coord_t width, lv_opa_t opa)
{
    lv_draw_line_dsc_t descriptor;
    lv_draw_line_dsc_init(&descriptor);
    descriptor.color = color;
    descriptor.width = width;
    descriptor.opa = opa;
    descriptor.round_start = true;
    descriptor.round_end = true;
    lv_draw_line(ctx, &descriptor, &start, &end);
}

void draw_touch_circle(lv_draw_ctx_t *ctx, lv_coord_t cx, lv_coord_t cy, lv_coord_t radius,
                       lv_color_t color, lv_coord_t width, lv_opa_t opa)
{
    if (radius <= 0)
    {
        return;
    }
    const lv_area_t area = { static_cast<lv_coord_t>(cx - radius),
                             static_cast<lv_coord_t>(cy - radius),
                             static_cast<lv_coord_t>(cx + radius),
                             static_cast<lv_coord_t>(cy + radius) };
    lv_draw_rect_dsc_t descriptor;
    lv_draw_rect_dsc_init(&descriptor);
    descriptor.bg_opa = LV_OPA_TRANSP;
    descriptor.border_color = color;
    descriptor.border_width = width;
    descriptor.border_opa = opa;
    descriptor.radius = LV_RADIUS_CIRCLE;
    lv_draw_rect(ctx, &descriptor, &area);
}

void draw_touch_dial_face(lv_draw_ctx_t *ctx, lv_coord_t cx, lv_coord_t cy, lv_coord_t side)
{
    const lv_coord_t outer = static_cast<lv_coord_t>(side / 2 - 5);
    if (!ctx || outer < 16)
    {
        return;
    }

    const lv_color_t cyan = lv_color_hex(0x009FC2);
    const lv_color_t ice = lv_color_hex(0x58BDD2);
    const lv_color_t grid = lv_color_hex(0x8FAEB8);

    /* Layered edge and range rings produce a radar-like face without extra objects. */
    draw_touch_circle(ctx, cx, cy, outer, cyan, 2, LV_OPA_COVER);
    draw_touch_circle(ctx, cx, cy, static_cast<lv_coord_t>(outer - 3), cyan, 1, LV_OPA_30);
    draw_touch_circle(ctx, cx, cy, static_cast<lv_coord_t>(outer - 9), ice, 1, LV_OPA_30);
    draw_touch_circle(ctx, cx, cy, static_cast<lv_coord_t>((outer * 2) / 3), grid, 1,
                      LV_OPA_20);
    draw_touch_circle(ctx, cx, cy, static_cast<lv_coord_t>(outer / 3), grid, 1,
                      LV_OPA_20);

    const lv_coord_t cross_radius = static_cast<lv_coord_t>(outer - 16);
    const lv_point_t vertical_a = { cx, static_cast<lv_coord_t>(cy - cross_radius) };
    const lv_point_t vertical_b = { cx, static_cast<lv_coord_t>(cy + cross_radius) };
    const lv_point_t horizontal_a = { static_cast<lv_coord_t>(cx - cross_radius), cy };
    const lv_point_t horizontal_b = { static_cast<lv_coord_t>(cx + cross_radius), cy };
    draw_touch_line(ctx, vertical_a, vertical_b, grid, 1, LV_OPA_20);
    draw_touch_line(ctx, horizontal_a, horizontal_b, grid, 1, LV_OPA_20);

    for (int index = 0; index < 24; ++index)
    {
        const bool major = (index % 3) == 0;
        const lv_coord_t inner = static_cast<lv_coord_t>(outer - (major ? 8 : 4));
        lv_point_t start{};
        lv_point_t end{};
        start.x = static_cast<lv_coord_t>(cx +
                  ((static_cast<int32_t>(inner) * kTouchDialSinQ10[index]) >> 10));
        start.y = static_cast<lv_coord_t>(cy -
                  ((static_cast<int32_t>(inner) * kTouchDialCosQ10[index]) >> 10));
        end.x = static_cast<lv_coord_t>(cx +
                ((static_cast<int32_t>(outer) * kTouchDialSinQ10[index]) >> 10));
        end.y = static_cast<lv_coord_t>(cy -
                ((static_cast<int32_t>(outer) * kTouchDialCosQ10[index]) >> 10));
        draw_touch_line(ctx, start, end, major ? cyan : ice,
                        major ? 2 : 1, major ? LV_OPA_COVER : LV_OPA_50);
    }

    const lv_point_t marker_a = { cx, static_cast<lv_coord_t>(cy - outer + 2) };
    const lv_point_t marker_b = { static_cast<lv_coord_t>(cx - 4),
                                  static_cast<lv_coord_t>(cy - outer + 9) };
    const lv_point_t marker_c = { static_cast<lv_coord_t>(cx + 4),
                                  static_cast<lv_coord_t>(cy - outer + 9) };
    const lv_point_t marker_points[3] = { marker_a, marker_b, marker_c };
    lv_draw_rect_dsc_t marker;
    lv_draw_rect_dsc_init(&marker);
    marker.bg_color = lv_color_hex(themePalette().error);
    marker.bg_opa = LV_OPA_COVER;
    marker.border_width = 0;
    lv_draw_triangle(ctx, &marker, marker_points);
}
#endif

void on_col_size_changed(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_SIZE_CHANGED)
        return;
    auto *dial = static_cast<CompassDialWidget *>(lv_event_get_user_data(e));
    if (dial)
        dial->layoutSize();
}

void on_ring_draw(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_DRAW_MAIN)
        return;
    auto *dial = static_cast<CompassDialWidget *>(lv_event_get_user_data(e));
    if (!dial || dial->side <= 0 || dial->dialHidden())
        return;

    lv_draw_ctx_t *draw_ctx = lv_event_get_draw_ctx(e);
    lv_obj_t *obj = lv_event_get_target(e);
    lv_area_t area;
    lv_obj_get_coords(obj, &area);
    const lv_coord_t cx = dial->side / 2;
    const lv_coord_t ax = (lv_coord_t)(area.x1 + cx);
    const lv_coord_t ay = (lv_coord_t)(area.y1 + cx);
#if defined(HELTEC_UI_HAS_TOUCH) && HELTEC_UI_HAS_TOUCH
    draw_touch_dial_face(draw_ctx, ax, ay, dial->side);
#endif
#if defined(HELTEC_UI_HAS_TOUCH) && HELTEC_UI_HAS_TOUCH
    const lv_color_t ring_color = lv_color_black();
#else
    const lv_color_t ring_color = lv_color_hex(themePalette().fg_on_dark);
#endif
    compass_draw_dial_ring(draw_ctx, ax, ay, dial->side, dial->ring_heading_tenths,
                           ring_color);
}

#if defined(HELTEC_UI_HAS_TOUCH) && HELTEC_UI_HAS_TOUCH
void on_ring_ext_draw_size(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_REFR_EXT_DRAW_SIZE)
    {
        lv_event_set_ext_draw_size(e, 18);
    }
}
#endif

void on_needle_draw(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_DRAW_MAIN)
        return;
    auto *dial = static_cast<CompassDialWidget *>(lv_event_get_user_data(e));
    if (!dial || dial->side <= 0 || dial->dialHidden())
        return;

    lv_draw_ctx_t *draw_ctx = lv_event_get_draw_ctx(e);
    lv_obj_t *obj = lv_event_get_target(e);
    lv_area_t area;
    lv_obj_get_coords(obj, &area);
    const lv_coord_t cx = dial->side / 2;
    const lv_coord_t ax = (lv_coord_t)(area.x1 + cx);
    const lv_coord_t ay = (lv_coord_t)(area.y1 + cx);

    if (dial->needle_kind == CompassDialNeedleKind::FriendTurn)
    {
        if (dial->friend_direction_valid)
        {
            const lv_coord_t needle_r = (lv_coord_t)(dial->side / 2 - 4);
            compass_draw_friend_needle(draw_ctx, ax, ay, needle_r,
                                       dial->friend_turn_tenths,
                                       dial->friend_gps_fix, dial->friend_on_target);
        }
        if (!dial->friend_direction_valid || dial->friend_near_target ||
            dial->center_marker_visible)
        {
#if defined(HELTEC_UI_HAS_TOUCH) && HELTEC_UI_HAS_TOUCH
            if (dial->friend_near_target)
            {
                draw_touch_circle(draw_ctx, ax, ay, 13, lv_color_hex(themePalette().success), 2, LV_OPA_50);
                draw_touch_circle(draw_ctx, ax, ay, 9, lv_color_hex(themePalette().success), 1, LV_OPA_80);
            }
#endif
            lv_coord_t diameter = dial->friend_near_target
                                      ? (lv_coord_t)(dial->side / 8)
                                      : (lv_coord_t)(dial->side / 16);
            const lv_coord_t min_diameter = dial->friend_near_target ? 6 : 3;
            const lv_coord_t max_diameter = dial->friend_near_target ? 10 : 5;
            if (diameter < min_diameter)
                diameter = min_diameter;
            if (diameter > max_diameter)
                diameter = max_diameter;
            const lv_coord_t half = (lv_coord_t)(diameter / 2);
            lv_area_t dot_area = { (lv_coord_t)(ax - half), (lv_coord_t)(ay - half),
                                   (lv_coord_t)(ax - half + diameter - 1),
                                   (lv_coord_t)(ay - half + diameter - 1) };
            lv_draw_rect_dsc_t dot;
            lv_draw_rect_dsc_init(&dot);
#if defined(HELTEC_UI_HAS_TOUCH) && HELTEC_UI_HAS_TOUCH
            dot.bg_color = dial->friend_near_target ? lv_color_hex(themePalette().success) : lv_color_hex(themePalette().accent);
#else
            dot.bg_color = lv_color_hex(themePalette().success);
#endif
            dot.bg_opa = LV_OPA_COVER;
            dot.border_width = 0;
            dot.radius = LV_RADIUS_CIRCLE;
            lv_draw_rect(draw_ctx, &dot, &dot_area);
        }
    }
    else
    {
        compass_draw_dial_needle(draw_ctx, ax, ay, dial->side, dial->needle_heading_tenths);
    }
}

} // namespace
bool CompassDialWidget::create(lv_obj_t *parent_row)
{
    if (!parent_row)
        return false;

    center_col = ht_class_create(&ht_compass_plain_class, parent_row);
    if (!center_col)
        return false;
    lv_obj_set_height(center_col, lv_pct(100));
    lv_obj_set_flex_grow(center_col, 1);
    lv_obj_set_flex_flow(center_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(center_col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(center_col, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(center_col, LV_OBJ_FLAG_OVERFLOW_VISIBLE);

    hub = ht_class_create(&ht_compass_hub_class, center_col);
    if (!hub)
        return false;
    lv_obj_clear_flag(hub, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(hub, LV_OBJ_FLAG_OVERFLOW_VISIBLE);

    lv_obj_add_event_cb(hub, on_ring_draw, LV_EVENT_DRAW_MAIN, this);
#if defined(HELTEC_UI_HAS_TOUCH) && HELTEC_UI_HAS_TOUCH
    lv_obj_add_event_cb(hub, on_ring_ext_draw_size, LV_EVENT_REFR_EXT_DRAW_SIZE, nullptr);
#endif

    needle = ht_class_create(&ht_compass_needle_class, hub);
    if (!needle)
        return false;
    lv_obj_clear_flag(needle, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(needle, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_add_event_cb(needle, on_needle_draw, LV_EVENT_DRAW_MAIN, this);
    lv_obj_move_foreground(needle);

    lv_obj_add_event_cb(center_col, on_col_size_changed, LV_EVENT_SIZE_CHANGED, this);
    return true;
}

void CompassDialWidget::setRingHeading(int16_t heading_tenths)
{
    heading_tenths = dial_heading_tenths(heading_tenths);
    if (ring_heading_tenths == heading_tenths)
        return;
    ring_heading_tenths = heading_tenths;
    invalidateRing();
}

void CompassDialWidget::setVisible(bool visible)
{
    const bool currently_visible = !dialHidden();
    if (currently_visible == visible)
        return;

    setDialHidden(!visible);
    setLayersVisible(visible);
    invalidateRing();
    invalidateNeedle();
}

void CompassDialWidget::setHeadingNeedle(int16_t heading_tenths)
{
    heading_tenths = dial_heading_tenths(heading_tenths);
    if (needle_heading_tenths == heading_tenths)
        return;
    needle_heading_tenths = heading_tenths;
    invalidateNeedle();
}

void CompassDialWidget::setFriendNeedle(int16_t turn_tenths, bool gps_fix, bool direction_valid,
                                        bool on_target, bool near_target)
{
    turn_tenths = dial_heading_tenths(turn_tenths);
    if (friend_turn_tenths == turn_tenths && friend_gps_fix == gps_fix &&
        friend_direction_valid == direction_valid && friend_on_target == on_target &&
        friend_near_target == near_target)
        return;
    friend_turn_tenths = turn_tenths;
    friend_gps_fix = gps_fix;
    friend_direction_valid = direction_valid;
    friend_on_target = on_target;
    friend_near_target = near_target;
    invalidateNeedle();
}

void CompassDialWidget::setCenterMarkerVisible(bool visible)
{
    if (center_marker_visible == visible)
        return;
    center_marker_visible = visible;
    invalidateNeedle();
}

void CompassDialWidget::layoutLayers()
{
    if (side < min_side)
        return;
    if (needle)
    {
        lv_obj_set_size(needle, side, side);
        lv_obj_center(needle);
        lv_obj_move_foreground(needle);
    }
}

void CompassDialWidget::layoutSize()
{
    if (!center_col || !hub || !needle || (flags & kLayoutBusy))
        return;
    flags |= kLayoutBusy;

    const lv_coord_t w = lv_obj_get_content_width(center_col);
    const lv_coord_t h = lv_obj_get_content_height(center_col);
    if (w <= 0 || h <= 0)
    {
        flags &= ~kLayoutBusy;
        return;
    }

    lv_coord_t next = LV_MIN(w, h);
    if (next > edge_margin * 2)
        next = (lv_coord_t)(next - edge_margin * 2);
    if (next < min_side || next == side)
    {
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

void CompassDialWidget::setLayersVisible(bool visible)
{
    if (needle)
    {
        if (visible)
            lv_obj_clear_flag(needle, LV_OBJ_FLAG_HIDDEN);
        else
            lv_obj_add_flag(needle, LV_OBJ_FLAG_HIDDEN);
    }
}

void CompassDialWidget::invalidateRing()
{
    if (hub)
        lv_obj_invalidate(hub);
}

void CompassDialWidget::invalidateNeedle()
{
    if (needle)
        lv_obj_invalidate(needle);
}

} // namespace Heltec::MeshCore::Ui

#endif // ENV_INCLUDE_GPS || ENV_INCLUDE_COMPASS
