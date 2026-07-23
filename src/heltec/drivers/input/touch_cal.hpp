#pragma once

#include <stdint.h>

namespace heltec::meshcore::dal::touch_cal {

inline uint16_t scaleAxis(uint16_t v, uint16_t in_min, uint16_t in_max, uint16_t out_max) {
  if (in_max <= in_min || out_max == 0) return (v > out_max) ? out_max : v;
  if (v <= in_min) return 0;
  if (v >= in_max) return out_max;
  const uint32_t num = (uint32_t)(v - in_min) * out_max;
  const uint32_t den = (uint32_t)(in_max - in_min);
  return (uint16_t)((num + den / 2U) / den);
}

inline void mapToDisplay(uint16_t& x, uint16_t& y, uint16_t hor_res, uint16_t ver_res) {
#if defined(HELTEC_TOUCH_SWAP_XY) && HELTEC_TOUCH_SWAP_XY
  const uint16_t tx = x;
  x = y;
  y = tx;
#endif
#if defined(HELTEC_TOUCH_MIRROR_X) && HELTEC_TOUCH_MIRROR_X
  if (hor_res > 0) x = (uint16_t)(hor_res - 1U - x);
#endif
#if defined(HELTEC_TOUCH_MIRROR_Y) && HELTEC_TOUCH_MIRROR_Y
  if (ver_res > 0) y = (uint16_t)(ver_res - 1U - y);
#endif

  const uint16_t out_x_max = (hor_res > 0) ? (uint16_t)(hor_res - 1U) : 0U;
  const uint16_t out_y_max = (ver_res > 0) ? (uint16_t)(ver_res - 1U) : 0U;

#if defined(HELTEC_TOUCH_CAL_X_MAX) && defined(HELTEC_TOUCH_CAL_Y_MAX)
#ifndef HELTEC_TOUCH_CAL_X_MIN
#define HELTEC_TOUCH_CAL_X_MIN 0
#endif
#ifndef HELTEC_TOUCH_CAL_Y_MIN
#define HELTEC_TOUCH_CAL_Y_MIN 0
#endif
  x = scaleAxis(x, HELTEC_TOUCH_CAL_X_MIN, HELTEC_TOUCH_CAL_X_MAX, out_x_max);
  y = scaleAxis(y, HELTEC_TOUCH_CAL_Y_MIN, HELTEC_TOUCH_CAL_Y_MAX, out_y_max);
#endif

#if defined(HELTEC_TOUCH_FINE_X_MAX) && defined(HELTEC_TOUCH_FINE_Y_MAX)
#ifndef HELTEC_TOUCH_FINE_X_MIN
#define HELTEC_TOUCH_FINE_X_MIN 0
#endif
#ifndef HELTEC_TOUCH_FINE_Y_MIN
#define HELTEC_TOUCH_FINE_Y_MIN 0
#endif
  x = scaleAxis(x, HELTEC_TOUCH_FINE_X_MIN, HELTEC_TOUCH_FINE_X_MAX, out_x_max);
  y = scaleAxis(y, HELTEC_TOUCH_FINE_Y_MIN, HELTEC_TOUCH_FINE_Y_MAX, out_y_max);
#endif

  if (hor_res > 0 && x >= hor_res) x = (uint16_t)(hor_res - 1U);
  if (ver_res > 0 && y >= ver_res) y = (uint16_t)(ver_res - 1U);
}

}  // namespace heltec::meshcore::dal::touch_cal
