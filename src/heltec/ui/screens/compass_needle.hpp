/**
 * @file    compass_needle.hpp
 * @brief   Compass dial drawing interface.
 */
#ifndef HELTEC_UI_SCREENS_COMPASS_NEEDLE_HPP
#define HELTEC_UI_SCREENS_COMPASS_NEEDLE_HPP

#include <lvgl.h>

namespace heltec::meshcore::ui
{

int16_t dial_heading_tenths(int16_t tenths);
float compass_wrap_degrees_360(float degrees);
bool compass_turn_on_target_deg(float turn_deg, float tolerance_deg = 30.0f);

void compass_draw_dial_ring(lv_draw_ctx_t *draw_ctx, lv_coord_t cx, lv_coord_t cy,
                            lv_coord_t side, int16_t heading_tenths, lv_color_t color);
void compass_draw_dial_needle(lv_draw_ctx_t *draw_ctx, lv_coord_t cx, lv_coord_t cy,
                              lv_coord_t side, int16_t heading_tenths);
void compass_draw_friend_needle(lv_draw_ctx_t *draw_ctx, lv_coord_t cx, lv_coord_t cy,
                                lv_coord_t radius, int16_t turn_tenths_clockwise_from_up,
                                bool gps_fix, bool on_target);

} // namespace heltec::meshcore::ui

#endif // HELTEC_UI_SCREENS_COMPASS_NEEDLE_HPP
