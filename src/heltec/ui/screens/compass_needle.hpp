#pragma once

#include "ui/app/ui_theme.hpp"
#include <lvgl.h>
#include <math.h>
#include <string.h>

namespace heltec::meshcore::ui {

/** Dial needle snap (tenths of a degree); 1 = 0.1° */
constexpr int16_t kCompassDialStepTenths = 1;
constexpr int kCompassDialSteps = 72;
/** Needle vertex scale (permille); 1150 �?15% larger outline. */
constexpr int32_t kCompassNeedleGeomPermille = 1150;
static const int16_t kCompassNeedleOffXPermille[4] = {0, -208, 208, 0};
static const int16_t kCompassNeedleOffYPermille[4] = {-560, 300, 300, 263};
static const uint8_t kCompassNeedleEdges[4][2] = {{0, 1}, {0, 2}, {1, 3}, {2, 3}};

/** Find-friend 箭簇: 4 点凹四边�?�?尖顶、底角、向�?V 缺口 (permille, up = negative Y). */
static const int16_t kFriendNeedleOffXPermille[4] = {0, -360, 360, 0};
static const int16_t kFriendNeedleOffYPermille[4] = {-700, 420, 420, 120};
static const uint8_t kFriendNeedleEdges[4][2] = {{0, 1}, {1, 3}, {3, 2}, {2, 0}};
static const uint8_t kFriendNeedleFillTris[6] = {0, 1, 3, 0, 3, 2};
/** Find-friend only; 863 �?kCompassNeedleGeomPermille × 3/4 */
constexpr int32_t kFriendNeedleGeomPermille = 863;

inline int16_t dial_heading_tenths(int16_t tenths) {
  int32_t t = tenths % 3600;
  if (t < 0) t += 3600;
  return (int16_t)(((t + kCompassDialStepTenths / 2) / kCompassDialStepTenths) * kCompassDialStepTenths %
                   3600);
}

inline lv_coord_t compass_snap_coord(float v) {
  return (lv_coord_t)(v >= 0.f ? (v + 0.5f) : (v - 0.5f));
}

/** Arc centerline point; label center = (out_x, out_y), top-left = center - size/2. */
inline void compass_north_label_center_on_arc(lv_coord_t cx, lv_coord_t cy, lv_coord_t arc_radius,
                                              float north_rad, lv_coord_t* out_center_x,
                                              lv_coord_t* out_center_y) {
  const float rr = (float)arc_radius;
  *out_center_x = compass_snap_coord((float)cx + rr * sinf(north_rad));
  *out_center_y = compass_snap_coord((float)cy - rr * cosf(north_rad));
}

/** Q14 sin/cos for heading 0..355° in 5° steps (index = deg / 5). */
static const int16_t kCompassSinQ14[kCompassDialSteps] = {
    0,    1428, 2845, 4240, 5604, 6924, 8192, 9397, 10531, 11585, 12551, 13421, 14189, 14849,
    15396, 15826, 16135, 16322, 16384, 16322, 16135, 15826, 15396, 14849, 14189, 13421, 12551,
    11585, 10531, 9397, 8192, 6924, 5604, 4240, 2845, 1428, 0,    -1428, -2845, -4240, -5604,
    -6924, -8192, -9397, -10531, -11585, -12551, -13421, -14189, -14849, -15396, -15826, -16135,
    -16322, -16384, -16322, -16135, -15826, -15396, -14849, -14189, -13421, -12551, -11585, -10531,
    -9397, -8192, -6924, -5604, -4240, -2845, -1428};
static const int16_t kCompassCosQ14[kCompassDialSteps] = {
    16384, 16322, 16135, 15826, 15396, 14849, 14189, 13421, 12551, 11585, 10531, 9397, 8192, 6924,
    5604, 4240, 2845, 1428, 0, -1428, -2845, -4240, -5604, -6924, -8192, -9397, -10531, -11585,
    -12551, -13421, -14189, -14849, -15396, -15826, -16135, -16322, -16384, -16322, -16135, -15826,
    -15396, -14849, -14189, -13421, -12551, -11585, -10531, -9397, -8192, -6924, -5604, -4240, -2845,
    -1428, 0, 1428, 2845, 4240, 5604, 6924, 8192, 9397, 10531, 11585, 12551, 13421, 14189, 14849,
    15396, 15826, 16135, 16322};

inline int compass_dial_trig_index(int16_t dial_tenths, int heading_offset_deg) {
  int32_t deg = (int32_t)dial_tenths / 10 + heading_offset_deg;
  deg %= 360;
  if (deg < 0) deg += 360;
  return (int)(deg / 5);
}

inline void compass_needle_quad_points(lv_coord_t cx, lv_coord_t cy, lv_coord_t radius,
                                       int16_t dial_tenths, int heading_offset_deg, bool north_up,
                                       lv_point_t p[4]) {
  const int idx = compass_dial_trig_index(dial_tenths, heading_offset_deg);
  const int32_t s = north_up ? -(int32_t)kCompassSinQ14[idx] : (int32_t)kCompassSinQ14[idx];
  const int32_t c = (int32_t)kCompassCosQ14[idx];
  const int32_t r = (int32_t)radius;

  for (int i = 0; i < 4; ++i) {
    int32_t x = (r * (int32_t)kCompassNeedleOffXPermille[i] * kCompassNeedleGeomPermille) / 1000000;
    int32_t y = (r * (int32_t)kCompassNeedleOffYPermille[i] * kCompassNeedleGeomPermille) / 1000000;
    const int32_t rx = (x * c - y * s) >> 14;
    const int32_t ry = (x * s + y * c) >> 14;
    p[i].x = (lv_coord_t)(cx + rx);
    p[i].y = (lv_coord_t)(cy + ry);
  }
}

inline void compass_draw_line_segment(lv_draw_ctx_t* draw_ctx, lv_coord_t x0, lv_coord_t y0,
                                      lv_coord_t x1, lv_coord_t y1, lv_color_t color,
                                      lv_coord_t width = 1) {
  if (!draw_ctx) return;
  lv_draw_line_dsc_t dsc;
  lv_draw_line_dsc_init(&dsc);
  dsc.color = color;
  dsc.width = width;
  dsc.round_start = 0;
  dsc.round_end = 0;
  dsc.opa = LV_OPA_COVER;
  lv_point_t a = {x0, y0};
  lv_point_t b = {x1, y1};
  lv_draw_line(draw_ctx, &dsc, &a, &b);
}

inline void compass_draw_needle_edges(lv_draw_ctx_t* draw_ctx, const lv_point_t p[4], lv_color_t color,
                                      lv_coord_t line_width) {
  for (int i = 0; i < 4; ++i) {
    const uint8_t a = kCompassNeedleEdges[i][0];
    const uint8_t b = kCompassNeedleEdges[i][1];
    compass_draw_line_segment(draw_ctx, p[a].x, p[a].y, p[b].x, p[b].y, color, line_width);
  }
}

/** North arrow outline: tip, left wing, right wing, base. dial_tenths should be dial-snapped. */
inline void compass_draw_needle(lv_draw_ctx_t* draw_ctx, lv_coord_t cx, lv_coord_t cy,
                                lv_coord_t radius, int16_t dial_tenths, int heading_offset_deg = 0,
                                bool north_up = true, lv_color_t color = ui_color_fg(),
                                lv_coord_t line_width = 1) {
  if (!draw_ctx || radius < 4) return;

  lv_point_t p[4];
  compass_needle_quad_points(cx, cy, radius, dial_tenths, heading_offset_deg, north_up, p);
  compass_draw_needle_edges(draw_ctx, p, color, line_width);
}

/** Draw needle at arbitrary clockwise-from-up angle (0.1° via tenths); float trig, no 5° LUT. */
inline void compass_draw_needle_angle(lv_draw_ctx_t* draw_ctx, lv_coord_t cx, lv_coord_t cy,
                                      lv_coord_t radius, float angle_deg_clockwise_from_up,
                                      lv_color_t color = ui_color_fg(), lv_coord_t line_width = 1) {
  if (!draw_ctx || radius < 4) return;

  const float north_rad = angle_deg_clockwise_from_up * (3.14159265f / 180.f);
  const float s = sinf(north_rad);
  const float c = cosf(north_rad);
  const int32_t r = (int32_t)radius;

  lv_point_t p[4];
  for (int i = 0; i < 4; ++i) {
    int32_t x = (r * (int32_t)kCompassNeedleOffXPermille[i] * kCompassNeedleGeomPermille) / 1000000;
    int32_t y = (r * (int32_t)kCompassNeedleOffYPermille[i] * kCompassNeedleGeomPermille) / 1000000;
    const int32_t rx = (int32_t)((float)x * c - (float)y * s);
    const int32_t ry = (int32_t)((float)x * s + (float)y * c);
    p[i].x = (lv_coord_t)(cx + rx);
    p[i].y = (lv_coord_t)(cy + ry);
  }

  compass_draw_needle_edges(draw_ctx, p, color, line_width);
}

inline float compass_wrap_degrees_360(float degrees) {
  degrees = fmodf(degrees, 360.f);
  if (degrees < 0.f) degrees += 360.f;
  return degrees;
}

#ifndef COMPASS_DIAL_BLUE_COLOR
#define COMPASS_DIAL_BLUE_COLOR 0x58B0FF
#endif

#ifndef COMPASS_DIAL_RING_INSET
/** px inside hub square before tick arc */
#define COMPASS_DIAL_RING_INSET 1
#endif
#ifndef COMPASS_DIAL_LABEL_INSET
/** px inside outer tick radius for N/E/S/W */
#define COMPASS_DIAL_LABEL_INSET 4
#endif
#ifndef COMPASS_NEEDLE_RADIUS_INSET
/** px inside hub edge for needle tip reach; 0 = use full hub radius */
#define COMPASS_NEEDLE_RADIUS_INSET 2
#endif
#ifndef COMPASS_NEEDLE_TIP_PCT
#define COMPASS_NEEDLE_TIP_PCT 39
#endif
#ifndef COMPASS_NEEDLE_BASE_PCT
#define COMPASS_NEEDLE_BASE_PCT 17
#endif

inline void compass_draw_cardinal_label(lv_draw_ctx_t* draw_ctx, lv_coord_t cx, lv_coord_t cy,
                                        lv_coord_t arc_radius, float deg_clockwise_from_up,
                                        const char* text, lv_color_t color) {
  if (!draw_ctx || !text || !text[0]) return;
  const float rad = deg_clockwise_from_up * (3.14159265f / 180.f);
  const lv_coord_t lx = compass_snap_coord((float)cx + (float)arc_radius * sinf(rad));
  const lv_coord_t ly = compass_snap_coord((float)cy - (float)arc_radius * cosf(rad));

  lv_draw_label_dsc_t dsc;
  lv_draw_label_dsc_init(&dsc);
  dsc.font = LV_FONT_DEFAULT;
  dsc.color = color;
  dsc.opa = LV_OPA_COVER;
  dsc.align = LV_TEXT_ALIGN_CENTER;

  const lv_coord_t half = (dsc.font ? dsc.font->line_height : 10) / 2;
  lv_area_t coords = {(lv_coord_t)(lx - half), (lv_coord_t)(ly - half), (lv_coord_t)(lx + half),
                      (lv_coord_t)(ly + half)};
  lv_draw_label(draw_ctx, &dsc, &coords, text, nullptr);
}

inline void compass_draw_dial_ticks(lv_draw_ctx_t* draw_ctx, lv_coord_t cx, lv_coord_t cy,
                                    lv_coord_t outer_r, float rot_deg, lv_color_t color) {
  if (!draw_ctx || outer_r < 4) return;
  // Skip N/E/S/W (0/90/180/270); ticks only at 45° intervals between cardinals.
  for (int i = 1; i < 8; i += 2) {
    const float deg = (float)i * 45.f + rot_deg;
    const float rad = deg * (3.14159265f / 180.f);
    const lv_coord_t tick_len = 3;
    const lv_coord_t inner_r = (lv_coord_t)(outer_r - tick_len);
    const lv_coord_t x0 = compass_snap_coord((float)cx + (float)inner_r * sinf(rad));
    const lv_coord_t y0 = compass_snap_coord((float)cy - (float)inner_r * cosf(rad));
    const lv_coord_t x1 = compass_snap_coord((float)cx + (float)outer_r * sinf(rad));
    const lv_coord_t y1 = compass_snap_coord((float)cy - (float)outer_r * cosf(rad));
    compass_draw_line_segment(draw_ctx, x0, y0, x1, y1, color, 1);
  }
}

inline void compass_draw_filled_triangle_abs(lv_draw_ctx_t* draw_ctx, const lv_point_t tri[3],
                                           lv_color_t color) {
  if (!draw_ctx) return;
  lv_draw_rect_dsc_t fill;
  lv_draw_rect_dsc_init(&fill);
  fill.border_width = 0;
  fill.bg_opa = LV_OPA_COVER;
  fill.bg_color = color;
  lv_draw_triangle(draw_ctx, &fill, tri);
}

inline void compass_rotate_point_q14(lv_coord_t cx, lv_coord_t cy, lv_coord_t x, lv_coord_t y,
                                     int32_t s, int32_t c, lv_coord_t* out_x, lv_coord_t* out_y) {
  const int32_t dx = (int32_t)(x - cx);
  const int32_t dy = (int32_t)(y - cy);
  const int32_t rx = (dx * c - dy * s) >> 14;
  const int32_t ry = (dx * s + dy * c) >> 14;
  *out_x = (lv_coord_t)(cx + rx);
  *out_y = (lv_coord_t)(cy + ry);
}

inline void compass_rotate_point(lv_coord_t cx, lv_coord_t cy, lv_coord_t x, lv_coord_t y,
                                 float rot_rad, lv_coord_t* out_x, lv_coord_t* out_y) {
  const float dx = (float)(x - cx);
  const float dy = (float)(y - cy);
  const float c = cosf(rot_rad);
  const float s = sinf(rot_rad);
  *out_x = compass_snap_coord((float)cx + dx * c - dy * s);
  *out_y = compass_snap_coord((float)cy + dx * s + dy * c);
}

inline void compass_draw_filled_triangle_rotated(lv_draw_ctx_t* draw_ctx, lv_coord_t cx,
                                                 lv_coord_t cy, const lv_point_t tri[3],
                                                 float rot_rad, lv_color_t color) {
  if (!draw_ctx) return;
  lv_point_t rot[3];
  for (int i = 0; i < 3; ++i) {
    compass_rotate_point(cx, cy, tri[i].x, tri[i].y, rot_rad, &rot[i].x, &rot[i].y);
  }
  compass_draw_filled_triangle_abs(draw_ctx, rot, color);
}

/** Max bake side for center needle ALPHA_1BIT sprites (stack, no heap). */
constexpr lv_coord_t kCompassNeedleBakeMax = 72;
constexpr int kCompassNeedleBakeStride = (kCompassNeedleBakeMax + 7) / 8;
constexpr int kCompassNeedleBakeMapBytes = kCompassNeedleBakeStride * kCompassNeedleBakeMax;

inline int compass_alpha1_stride(lv_coord_t w) { return (int)((w + 7) / 8); }

inline void compass_alpha1_clear(uint8_t* map, lv_coord_t w, lv_coord_t h) {
  memset(map, 0, (size_t)compass_alpha1_stride(w) * (size_t)h);
}

inline void compass_alpha1_set(uint8_t* map, lv_coord_t w, lv_coord_t x, lv_coord_t y) {
  if (x < 0 || y < 0) return;
  map[(size_t)y * (size_t)compass_alpha1_stride(w) + (size_t)(x >> 3)] |= (uint8_t)(0x80u >> (x & 7));
}

inline int compass_tri_cross(int ax, int ay, int bx, int by, int cx, int cy) {
  return (bx - ax) * (cy - ay) - (by - ay) * (cx - ax);
}

inline bool compass_point_in_tri_int(int px, int py, const lv_point_t tri[3]) {
  const int c0 = compass_tri_cross(tri[0].x, tri[0].y, tri[1].x, tri[1].y, px, py);
  const int c1 = compass_tri_cross(tri[1].x, tri[1].y, tri[2].x, tri[2].y, px, py);
  const int c2 = compass_tri_cross(tri[2].x, tri[2].y, tri[0].x, tri[0].y, px, py);
  const bool has_neg = (c0 < 0) || (c1 < 0) || (c2 < 0);
  const bool has_pos = (c0 > 0) || (c1 > 0) || (c2 > 0);
  return !(has_neg && has_pos);
}

inline void compass_rasterize_tri_alpha1(uint8_t* map, lv_coord_t w, lv_coord_t h,
                                         const lv_point_t tri[3]) {
  lv_coord_t minx = tri[0].x;
  lv_coord_t maxx = tri[0].x;
  lv_coord_t miny = tri[0].y;
  lv_coord_t maxy = tri[0].y;
  for (int i = 1; i < 3; ++i) {
    if (tri[i].x < minx) minx = tri[i].x;
    if (tri[i].x > maxx) maxx = tri[i].x;
    if (tri[i].y < miny) miny = tri[i].y;
    if (tri[i].y > maxy) maxy = tri[i].y;
  }
  if (minx < 0) minx = 0;
  if (miny < 0) miny = 0;
  if (maxx >= w) maxx = (lv_coord_t)(w - 1);
  if (maxy >= h) maxy = (lv_coord_t)(h - 1);
  for (lv_coord_t y = miny; y <= maxy; ++y) {
    for (lv_coord_t x = minx; x <= maxx; ++x) {
      if (compass_point_in_tri_int(x, y, tri)) compass_alpha1_set(map, w, x, y);
    }
  }
}

inline void compass_needle_tris_local(lv_coord_t bake_side, lv_point_t north[3], lv_point_t south[3]) {
  const lv_coord_t cc = bake_side / 2;
  const lv_coord_t r = cc - 1;
  const lv_coord_t tip = (lv_coord_t)(r * COMPASS_NEEDLE_TIP_PCT / 100);
  const lv_coord_t half_base = (lv_coord_t)(r * COMPASS_NEEDLE_BASE_PCT / 100);
  north[0] = {cc, (lv_coord_t)(cc - tip)};
  north[1] = {(lv_coord_t)(cc - half_base), cc};
  north[2] = {(lv_coord_t)(cc + half_base), cc};
  south[0] = {cc, (lv_coord_t)(cc + tip)};
  south[1] = north[1];
  south[2] = north[2];
}

inline void compass_draw_alpha1_sprite(lv_draw_ctx_t* draw_ctx, const lv_area_t* area, const uint8_t* map,
                                       lv_coord_t w, lv_coord_t h, lv_color_t color) {
  if (!draw_ctx || !map || !area) return;
  lv_img_dsc_t dsc;
  dsc.header.cf = LV_IMG_CF_ALPHA_1BIT;
  dsc.header.always_zero = 0;
  dsc.header.reserved = 0;
  dsc.header.w = w;
  dsc.header.h = h;
  dsc.data_size = (uint32_t)compass_alpha1_stride(w) * (uint32_t)h;
  dsc.data = map;

  lv_draw_img_dsc_t img;
  lv_draw_img_dsc_init(&img);
  img.recolor = color;
  img.recolor_opa = LV_OPA_COVER;
  img.opa = LV_OPA_COVER;
  lv_draw_img(draw_ctx, &img, area, &dsc);
}

/** Symmetric north/south diamond; integer raster (no lv_draw_triangle AA). */
inline void compass_draw_center_bicolor_needle(lv_draw_ctx_t* draw_ctx, lv_coord_t cx, lv_coord_t cy,
                                               lv_coord_t radius, int16_t heading_tenths) {
  if (!draw_ctx || radius < 5) return;

  const lv_coord_t bake = (lv_coord_t)(radius * 2 + 4);
  const lv_coord_t cc = bake / 2;

  const int idx = compass_dial_trig_index(heading_tenths, 0);
  const int32_t s = -(int32_t)kCompassSinQ14[idx];
  const int32_t c = (int32_t)kCompassCosQ14[idx];

  lv_point_t north_l[3], south_l[3], north_r[3], south_r[3];
  compass_needle_tris_local(bake, north_l, south_l);
  for (int i = 0; i < 3; ++i) {
    compass_rotate_point_q14(cc, cc, north_l[i].x, north_l[i].y, s, c, &north_r[i].x, &north_r[i].y);
    compass_rotate_point_q14(cc, cc, south_l[i].x, south_l[i].y, s, c, &south_r[i].x, &south_r[i].y);
    north_r[i].x = (lv_coord_t)(north_r[i].x + (cx - cc));
    north_r[i].y = (lv_coord_t)(north_r[i].y + (cy - cc));
    south_r[i].x = (lv_coord_t)(south_r[i].x + (cx - cc));
    south_r[i].y = (lv_coord_t)(south_r[i].y + (cy - cc));
  }

  compass_draw_filled_triangle_abs(draw_ctx, north_r, ui_color_error());
  compass_draw_filled_triangle_abs(draw_ctx, south_r, lv_color_hex(COMPASS_DIAL_BLUE_COLOR));
}

/** Ticks + N/E/S/W labels (no center needle). */
inline void compass_draw_dial_ring(lv_draw_ctx_t* draw_ctx, lv_coord_t cx, lv_coord_t cy,
                                   lv_coord_t side, float heading_deg_clockwise, lv_color_t fg) {
  if (!draw_ctx || side < 16) return;
  const float rot = -heading_deg_clockwise;
  const lv_coord_t outer_r = (lv_coord_t)(side / 2 - COMPASS_DIAL_RING_INSET);
  const lv_coord_t label_r = (lv_coord_t)(outer_r - COMPASS_DIAL_LABEL_INSET);
  compass_draw_dial_ticks(draw_ctx, cx, cy, outer_r, rot, fg);
  compass_draw_cardinal_label(draw_ctx, cx, cy, label_r, rot + 0.f, "N", fg);
  compass_draw_cardinal_label(draw_ctx, cx, cy, label_r, rot + 90.f, "E", fg);
  compass_draw_cardinal_label(draw_ctx, cx, cy, label_r, rot + 180.f, "S", fg);
  compass_draw_cardinal_label(draw_ctx, cx, cy, label_r, rot + 270.f, "W", fg);
}

/** Bicolor center needle only. */
inline void compass_draw_dial_needle(lv_draw_ctx_t* draw_ctx, lv_coord_t cx, lv_coord_t cy,
                                     lv_coord_t side, int16_t heading_tenths) {
  if (!draw_ctx || side < 16) return;
  compass_draw_center_bicolor_needle(draw_ctx, cx, cy,
                                     (lv_coord_t)(side / 2 - COMPASS_NEEDLE_RADIUS_INSET), heading_tenths);
}

inline void compass_draw_line_segment_style(lv_draw_ctx_t* draw_ctx, lv_coord_t x0, lv_coord_t y0,
                                            lv_coord_t x1, lv_coord_t y1, lv_color_t color,
                                            lv_coord_t width, bool dashed) {
  if (!draw_ctx) return;
  lv_draw_line_dsc_t dsc;
  lv_draw_line_dsc_init(&dsc);
  dsc.color = color;
  dsc.width = width;
  dsc.round_start = 0;
  dsc.round_end = 0;
  dsc.opa = LV_OPA_COVER;
  if (dashed) {
    dsc.dash_width = 2;
    dsc.dash_gap = 2;
  }
  lv_point_t a = {x0, y0};
  lv_point_t b = {x1, y1};
  lv_draw_line(draw_ctx, &dsc, &a, &b);
}

inline bool compass_turn_on_target_deg(float turn_deg, float tolerance_deg = 30.f) {
  float t = turn_deg;
  while (t > 180.f) t -= 360.f;
  while (t < -180.f) t += 360.f;
  return fabsf(t) <= tolerance_deg;
}

/** Friend pointer: 凹形箭簇 fill + wire outline. turn = clockwise from screen up. */
inline void compass_draw_friend_needle(lv_draw_ctx_t* draw_ctx, lv_coord_t cx, lv_coord_t cy,
                                       lv_coord_t radius, float turn_deg_clockwise_from_up,
                                       bool gps_fix, bool on_target) {
  if (!draw_ctx || radius < 5) return;

  const float rot_rad = turn_deg_clockwise_from_up * (3.14159265f / 180.f);
  const float s = sinf(rot_rad);
  const float c = cosf(rot_rad);
  const int32_t r = (int32_t)radius;

  lv_point_t p[4];
  for (int i = 0; i < 4; ++i) {
    int32_t x = (r * (int32_t)kFriendNeedleOffXPermille[i] * kFriendNeedleGeomPermille) / 1000000;
    int32_t y = (r * (int32_t)kFriendNeedleOffYPermille[i] * kFriendNeedleGeomPermille) / 1000000;
    const int32_t rx = (int32_t)((float)x * c - (float)y * s);
    const int32_t ry = (int32_t)((float)x * s + (float)y * c);
    p[i].x = (lv_coord_t)(cx + rx);
    p[i].y = (lv_coord_t)(cy + ry);
  }

  const lv_color_t fill = on_target ? ui_color_success() : ui_color_error();
  for (int t = 0; t < 2; ++t) {
    lv_point_t tri[3];
    for (int i = 0; i < 3; ++i) {
      const uint8_t idx = kFriendNeedleFillTris[(size_t)t * 3 + (size_t)i];
      tri[i] = p[idx];
    }
    compass_draw_filled_triangle_abs(draw_ctx, tri, fill);
  }

  const lv_color_t edge = ui_color_fg_on_dark();
  const bool dashed = !gps_fix;
  for (int i = 0; i < 4; ++i) {
    const uint8_t a = kFriendNeedleEdges[i][0];
    const uint8_t b = kFriendNeedleEdges[i][1];
    compass_draw_line_segment_style(draw_ctx, p[a].x, p[a].y, p[b].x, p[b].y, edge, 1, dashed);
  }
}

/** Draw full dial (ring + needle) for single-layer fallback. */
inline void compass_draw_dial_card(lv_draw_ctx_t* draw_ctx, lv_coord_t cx, lv_coord_t cy,
                                   lv_coord_t side, float heading_deg_clockwise, lv_color_t fg) {
  if (!draw_ctx || side < 16) return;
  compass_draw_dial_ring(draw_ctx, cx, cy, side, heading_deg_clockwise, fg);
  const int16_t tenths = (int16_t)(heading_deg_clockwise * 10.f + 0.5f);
  compass_draw_dial_needle(draw_ctx, cx, cy, side, tenths);
}

}  // namespace heltec::meshcore::ui
