/**
 * @file    compass_needle.cpp
 * @brief   Compass dial drawing implementation.
 */
#include "compass_needle.hpp"
#include "ui/app/theme.hpp"
#include <math.h>

#ifndef COMPASS_DIAL_BLUE_COLOR
#define COMPASS_DIAL_BLUE_COLOR 0x58B0FF
#endif
#ifndef COMPASS_DIAL_RING_INSET
#define COMPASS_DIAL_RING_INSET 1
#endif
#ifndef COMPASS_DIAL_LABEL_INSET
#define COMPASS_DIAL_LABEL_INSET 4
#endif
#ifndef COMPASS_NEEDLE_RADIUS_INSET
#define COMPASS_NEEDLE_RADIUS_INSET 2
#endif
#ifndef COMPASS_NEEDLE_TIP_PCT
#define COMPASS_NEEDLE_TIP_PCT 39
#endif
#ifndef COMPASS_NEEDLE_BASE_PCT
#define COMPASS_NEEDLE_BASE_PCT 17
#endif

namespace heltec::meshcore::ui
{
namespace
{
constexpr int kDialSteps = 72;
constexpr int32_t kFriendScale = 863;
constexpr int16_t kFriendX[4] = { 0, -360, 360, 0 };
constexpr int16_t kFriendY[4] = { -700, 420, 420, 120 };
constexpr uint8_t kFriendEdges[4][2] = { { 0, 1 }, { 1, 3 }, { 3, 2 }, { 2, 0 } };
constexpr uint8_t kFriendTriangles[6] = { 0, 1, 3, 0, 3, 2 };
constexpr int16_t kSinQ14[kDialSteps] = {
    0, 1428, 2845, 4240, 5604, 6924, 8192, 9397, 10531, 11585, 12551, 13421,
    14189, 14849, 15396, 15826, 16135, 16322, 16384, 16322, 16135, 15826, 15396, 14849,
    14189, 13421, 12551, 11585, 10531, 9397, 8192, 6924, 5604, 4240, 2845, 1428,
    0, -1428, -2845, -4240, -5604, -6924, -8192, -9397, -10531, -11585, -12551, -13421,
    -14189, -14849, -15396, -15826, -16135, -16322, -16384, -16322, -16135, -15826,
    -15396, -14849, -14189, -13421, -12551, -11585, -10531, -9397, -8192, -6924,
    -5604, -4240, -2845, -1428
};
constexpr int16_t kCosQ14[kDialSteps] = {
    16384, 16322, 16135, 15826, 15396, 14849, 14189, 13421, 12551, 11585, 10531, 9397,
    8192, 6924, 5604, 4240, 2845, 1428, 0, -1428, -2845, -4240, -5604, -6924,
    -8192, -9397, -10531, -11585, -12551, -13421, -14189, -14849, -15396, -15826,
    -16135, -16322, -16384, -16322, -16135, -15826, -15396, -14849, -14189, -13421,
    -12551, -11585, -10531, -9397, -8192, -6924, -5604, -4240, -2845, -1428,
    0, 1428, 2845, 4240, 5604, 6924, 8192, 9397, 10531, 11585, 12551, 13421,
    14189, 14849, 15396, 15826, 16135, 16322
};

int trigIndex(int16_t tenths, int offset)
{
    int degrees = (int)tenths / 10 + offset;
    degrees %= 360;
    if (degrees < 0)
        degrees += 360;
    return degrees / 5;
}

void pointOnArc(lv_coord_t cx, lv_coord_t cy, lv_coord_t radius, int16_t tenths,
                int offset, lv_point_t &point)
{
    const int index = trigIndex(tenths, offset);
    point.x = (lv_coord_t)(cx + ((int32_t)radius * kSinQ14[index] >> 14));
    point.y = (lv_coord_t)(cy - ((int32_t)radius * kCosQ14[index] >> 14));
}

void drawLine(lv_draw_ctx_t *ctx, const lv_point_t &start, const lv_point_t &end,
              lv_color_t color, bool dashed = false)
{
    lv_draw_line_dsc_t descriptor;
    lv_draw_line_dsc_init(&descriptor);
    descriptor.color = color;
    descriptor.width = 1;
    descriptor.opa = LV_OPA_COVER;
    descriptor.dash_width = dashed ? 2 : 0;
    descriptor.dash_gap = dashed ? 2 : 0;
    lv_draw_line(ctx, &descriptor, &start, &end);
}

void drawTriangle(lv_draw_ctx_t *ctx, const lv_point_t points[3], lv_color_t color)
{
    lv_draw_rect_dsc_t descriptor;
    lv_draw_rect_dsc_init(&descriptor);
    descriptor.bg_color = color;
    descriptor.bg_opa = LV_OPA_COVER;
    descriptor.border_width = 0;
    lv_draw_triangle(ctx, &descriptor, points);
}

void drawCardinal(lv_draw_ctx_t *ctx, lv_coord_t cx, lv_coord_t cy, lv_coord_t radius,
                  int16_t tenths, int offset, const char *text, lv_color_t color)
{
    lv_point_t point = {};
    pointOnArc(cx, cy, radius, tenths, offset, point);
    lv_draw_label_dsc_t descriptor;
    lv_draw_label_dsc_init(&descriptor);
    descriptor.font = LV_FONT_DEFAULT;
    descriptor.color = color;
    descriptor.opa = LV_OPA_COVER;
    descriptor.align = LV_TEXT_ALIGN_CENTER;
    const lv_coord_t half = descriptor.font ? descriptor.font->line_height / 2 : 5;
    const lv_area_t area = { (lv_coord_t)(point.x - half), (lv_coord_t)(point.y - half),
                             (lv_coord_t)(point.x + half), (lv_coord_t)(point.y + half) };
    lv_draw_label(ctx, &descriptor, &area, text, nullptr);
}

void rotateQ14(lv_coord_t x, lv_coord_t y, int32_t sine, int32_t cosine,
               lv_coord_t cx, lv_coord_t cy, lv_point_t &output)
{
    output.x = (lv_coord_t)(cx + (((int32_t)x * cosine - (int32_t)y * sine) >> 14));
    output.y = (lv_coord_t)(cy + (((int32_t)x * sine + (int32_t)y * cosine) >> 14));
}
} // namespace

int16_t dial_heading_tenths(int16_t tenths)
{
    int32_t normalized = tenths % 3600;
    if (normalized < 0)
        normalized += 3600;
    return (int16_t)normalized;
}

float compass_wrap_degrees_360(float degrees)
{
    degrees = fmodf(degrees, 360.0f);
    return degrees < 0.0f ? degrees + 360.0f : degrees;
}

bool compass_turn_on_target_deg(float turn_deg, float tolerance_deg)
{
    while (turn_deg > 180.0f)
        turn_deg -= 360.0f;
    while (turn_deg < -180.0f)
        turn_deg += 360.0f;
    return fabsf(turn_deg) <= tolerance_deg;
}

void compass_draw_dial_ring(lv_draw_ctx_t *ctx, lv_coord_t cx, lv_coord_t cy,
                            lv_coord_t side, int16_t tenths, lv_color_t color)
{
    if (!ctx || side < 16)
        return;
    const lv_coord_t outer = (lv_coord_t)(side / 2 - COMPASS_DIAL_RING_INSET);
#if defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
    const lv_coord_t labels = (lv_coord_t)(outer + 11);
#else
    const lv_coord_t labels = (lv_coord_t)(outer - COMPASS_DIAL_LABEL_INSET);
#endif
    const int16_t ring = (int16_t)-tenths;
    for (int offset = 45; offset < 360; offset += 90)
    {
        lv_point_t start = {};
        lv_point_t end = {};
        pointOnArc(cx, cy, (lv_coord_t)(outer - 3), ring, offset, start);
        pointOnArc(cx, cy, outer, ring, offset, end);
        drawLine(ctx, start, end, color);
    }
    drawCardinal(ctx, cx, cy, labels, ring, 0, "N", color);
    drawCardinal(ctx, cx, cy, labels, ring, 90, "E", color);
    drawCardinal(ctx, cx, cy, labels, ring, 180, "S", color);
    drawCardinal(ctx, cx, cy, labels, ring, 270, "W", color);
}

void compass_draw_dial_needle(lv_draw_ctx_t *ctx, lv_coord_t cx, lv_coord_t cy,
                              lv_coord_t side, int16_t tenths)
{
    if (!ctx || side < 16)
        return;
    const lv_coord_t radius = (lv_coord_t)(side / 2 - COMPASS_NEEDLE_RADIUS_INSET);
    const lv_coord_t tip = (lv_coord_t)(radius * COMPASS_NEEDLE_TIP_PCT / 100);
    const lv_coord_t base = (lv_coord_t)(radius * COMPASS_NEEDLE_BASE_PCT / 100);
    const int index = trigIndex(tenths, 0);
    const int32_t sine = -(int32_t)kSinQ14[index];
    const int32_t cosine = kCosQ14[index];
    lv_point_t north[3] = {};
    lv_point_t south[3] = {};
    rotateQ14(0, (lv_coord_t)-tip, sine, cosine, cx, cy, north[0]);
    rotateQ14((lv_coord_t)-base, 0, sine, cosine, cx, cy, north[1]);
    rotateQ14(base, 0, sine, cosine, cx, cy, north[2]);
    rotateQ14(0, tip, sine, cosine, cx, cy, south[0]);
    south[1] = north[1];
    south[2] = north[2];
    drawTriangle(ctx, north, ui_color_error());
    drawTriangle(ctx, south, lv_color_hex(COMPASS_DIAL_BLUE_COLOR));
}

void compass_draw_friend_needle(lv_draw_ctx_t *ctx, lv_coord_t cx, lv_coord_t cy,
                                lv_coord_t radius, int16_t turn_tenths, bool gps_fix, bool on_target)
{
    if (!ctx || radius < 5)
        return;
    const int index = trigIndex(turn_tenths, 0);
    const int32_t sine = kSinQ14[index];
    const int32_t cosine = kCosQ14[index];
    lv_point_t points[4] = {};
    for (int index = 0; index < 4; ++index)
    {
        const int32_t x = ((int32_t)radius * kFriendX[index] * kFriendScale) / 1000000;
        const int32_t y = ((int32_t)radius * kFriendY[index] * kFriendScale) / 1000000;
        points[index].x = (lv_coord_t)(cx + ((x * cosine - y * sine) >> 14));
        points[index].y = (lv_coord_t)(cy + ((x * sine + y * cosine) >> 14));
    }
    const lv_color_t fill = on_target ? ui_color_success() : ui_color_error();
    for (int index = 0; index < 2; ++index)
    {
        lv_point_t triangle[3] = {};
        for (int point = 0; point < 3; ++point)
            triangle[point] = points[kFriendTriangles[index * 3 + point]];
        drawTriangle(ctx, triangle, fill);
    }
#if defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
    const lv_color_t edge_color = lv_color_hex(0x244552);
#else
    const lv_color_t edge_color = ui_color_fg_on_dark();
#endif
    for (int index = 0; index < 4; ++index)
    {
        drawLine(ctx, points[kFriendEdges[index][0]], points[kFriendEdges[index][1]],
                 edge_color, !gps_fix);
    }
}
} // namespace heltec::meshcore::ui
