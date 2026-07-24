#if defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH

#include "touch_port.hpp"

#include <Arduino.h>
#include <Wire.h>
#include <lvgl.h>

#include "heltec/drivers/display/display_port.hpp"
#include "touch_cal.hpp"

#if defined(HELTEC_TOUCH_USE_CHSC6X) && HELTEC_TOUCH_USE_CHSC6X
#include "chsc6x.h"
#else
#include <TouchDrv.hpp>
#ifndef HELTEC_TOUCH_I2C_ADDR
#define HELTEC_TOUCH_I2C_ADDR 0x2E
#endif
#endif

namespace heltec::meshcore::dal::touch_port {
namespace {

#ifndef PIN_TOUCH_RST
#define PIN_TOUCH_RST -1
#endif
#ifndef PIN_TOUCH_IRQ
#define PIN_TOUCH_IRQ -1
#endif
#if defined(PIN_TFT_RST) && (PIN_TOUCH_RST == PIN_TFT_RST)
#undef PIN_TOUCH_RST
#define PIN_TOUCH_RST -1
#endif

#if defined(PIN_BOARD_SDA) && defined(PIN_BOARD_SCL) && defined(PIN_TFT_SDA) && \
    defined(PIN_TFT_SCL) && \
    ((PIN_TFT_SDA) != (PIN_BOARD_SDA) || (PIN_TFT_SCL) != (PIN_BOARD_SCL))
#define TOUCH_I2C_SDA PIN_BOARD_SDA
#define TOUCH_I2C_SCL PIN_BOARD_SCL
#elif defined(PIN_TFT_SDA) && defined(PIN_TFT_SCL)
#define TOUCH_I2C_SDA PIN_TFT_SDA
#define TOUCH_I2C_SCL PIN_TFT_SCL
#endif

struct Sample {
  bool pressed = false;
  uint16_t x = 0;
  uint16_t y = 0;
};

static lv_indev_drv_t s_indev_drv;
static lv_indev_t* s_indev = nullptr;
static bool s_ready = false;
static WakeFn s_wake_fn = nullptr;
static SampleHookFn s_sample_hook = nullptr;
static uint16_t s_hor_res = 240;
static uint16_t s_ver_res = 320;
static int16_t s_last_x = 0;
static int16_t s_last_y = 0;
static bool s_last_pressed = false;
static bool s_touch_down = false;
static bool s_hw_pressed = false;

enum class ReleaseBarrier : uint8_t {
  Inactive,
  WaitForHardwareRelease,
};
static ReleaseBarrier s_release_barrier = ReleaseBarrier::Inactive;

static void mapSample(Sample& s) {
  touch_cal::mapToDisplay(s.x, s.y, s_hor_res, s_ver_res);
}

static void request_release_barrier() {
  s_release_barrier = ReleaseBarrier::WaitForHardwareRelease;
  if (s_indev) lv_indev_wait_release(s_indev);
}

static bool release_barrier_active() {
  return s_release_barrier == ReleaseBarrier::WaitForHardwareRelease;
}

static void deliverReleased(lv_indev_data_t* data, int16_t x, int16_t y) {
  data->point.x = x;
  data->point.y = y;
  data->state = LV_INDEV_STATE_RELEASED;
}

static void resetGestureHook() {
  if (!s_sample_hook) return;
  bool suppress = false;
  s_sample_hook(false, s_last_x, s_last_y, &suppress);
}

static void deliverPointer(lv_indev_data_t* data, const Sample& s, bool hw_pressed) {
  int16_t x = (int16_t)s.x;
  int16_t y = (int16_t)s.y;
  bool suppress = false;

  if (s_sample_hook) {
    s_sample_hook(hw_pressed, x, y, &suppress);
  }
  if (release_barrier_active()) suppress = true;

  if (hw_pressed) {
    s_last_x = x;
    s_last_y = y;
    s_last_pressed = true;
  } else if (s_last_pressed) {
    x = s_last_x;
    y = s_last_y;
    s_last_pressed = false;
    s_touch_down = false;
  }

  data->point.x = x;
  data->point.y = y;
  data->state = (hw_pressed && !suppress) ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

static bool handleBacklightWakeTouch(lv_indev_data_t* data) {
  if (display_port::isBacklightOn()) {
    if (s_wake_fn) (void)s_wake_fn();
    return false;
  }
  bool swallow = true;
  if (s_wake_fn) {
    swallow = s_wake_fn();
  } else {
    display_port::setBacklightOn(true);
  }
  if (swallow) {
    request_release_barrier();
    data->state = LV_INDEV_STATE_RELEASED;
    return true;
  }
  return false;
}

#if defined(HELTEC_TOUCH_USE_CHSC6X) && HELTEC_TOUCH_USE_CHSC6X
#if defined(PIN_TFT_SDA) && defined(PIN_TFT_SCL)
static chsc6x s_touch(&Wire, PIN_TFT_SDA, PIN_TFT_SCL, PIN_TOUCH_IRQ, PIN_TOUCH_RST);
#else
static chsc6x s_touch(&Wire, -1, -1, PIN_TOUCH_IRQ, PIN_TOUCH_RST);
#endif

static bool readHardware(Sample& out) {
  const int rc = s_touch.chsc6x_read_touch_info(&out.x, &out.y);
  if (rc != 0) {
    out.pressed = false;
    return false;
  }
  out.pressed = true;
  return true;
}

static bool initHardware(uint16_t hor_res, uint16_t ver_res) {
  (void)hor_res;
  (void)ver_res;
  s_touch.chsc6x_init();
  return true;
}

#else  // TouchDrvCHSC5816

static TouchDrvCHSC5816 s_touch;

static bool readHardware(Sample& out) {
  const TouchPoints& pts = s_touch.getTouchPoints();
  if (!pts.hasPoints()) {
    out.pressed = false;
    return false;
  }
  const TouchPoint& p = pts.getPoint(0);
  out.x = p.x;
  out.y = p.y;
  out.pressed = true;
  return true;
}

static bool initHardware(uint16_t hor_res, uint16_t ver_res) {
  static TwoWire s_touch_wire(1);
  s_touch_wire.begin(TOUCH_I2C_SDA, TOUCH_I2C_SCL, 100000);
  delay(10);

  s_touch.setPins(PIN_TOUCH_RST, PIN_TOUCH_IRQ);
  s_touch.setResolution(hor_res, ver_res);
  s_touch.setTargetResolution(hor_res, ver_res);
#if defined(HELTEC_TOUCH_MIRROR_X) && HELTEC_TOUCH_MIRROR_X
  s_touch.setMirrorXY(true, false);
#elif defined(HELTEC_TOUCH_MIRROR_Y) && HELTEC_TOUCH_MIRROR_Y
  s_touch.setMirrorXY(false, true);
#endif
#if defined(HELTEC_TOUCH_SWAP_XY) && HELTEC_TOUCH_SWAP_XY
  s_touch.setSwapXY(true);
#endif

  return s_touch.begin(s_touch_wire, (uint8_t)HELTEC_TOUCH_I2C_ADDR, TOUCH_I2C_SDA, TOUCH_I2C_SCL);
}

#endif  // HELTEC_TOUCH_USE_CHSC6X

static void readCb(lv_indev_drv_t* drv, lv_indev_data_t* data) {
  (void)drv;
  if (!s_ready) {
    s_hw_pressed = false;
    data->state = LV_INDEV_STATE_RELEASED;
    return;
  }

  Sample sample;
  const bool has_touch = readHardware(sample);
  s_hw_pressed = has_touch;

  if (!has_touch) {
    if (release_barrier_active()) {
      s_release_barrier = ReleaseBarrier::Inactive;
      resetGestureHook();
      s_touch_down = false;
      s_last_pressed = false;
      deliverReleased(data, s_last_x, s_last_y);
      return;
    }
    if (s_touch_down) {
      s_touch_down = false;
      if (handleBacklightWakeTouch(data)) return;
      Sample release;
      release.pressed = false;
      release.x = (uint16_t)s_last_x;
      release.y = (uint16_t)s_last_y;
      // s_last_x/s_last_y are already mapped display coordinates.
      deliverPointer(data, release, false);
    } else {
      Sample idle;
      idle.pressed = false;
      idle.x = 0;
      idle.y = 0;
      deliverPointer(data, idle, false);
    }
    return;
  }

  if (handleBacklightWakeTouch(data)) return;

  mapSample(sample);
  if (release_barrier_active()) {
    s_touch_down = true;
    s_last_pressed = true;
    s_last_x = (int16_t)sample.x;
    s_last_y = (int16_t)sample.y;
    deliverReleased(data, s_last_x, s_last_y);
    return;
  }
  s_touch_down = true;
  deliverPointer(data, sample, true);
}

static bool registerPointerIndev() {
  lv_indev_drv_init(&s_indev_drv);
  s_indev_drv.type = LV_INDEV_TYPE_POINTER;
  s_indev_drv.read_cb = readCb;
  s_indev = lv_indev_drv_register(&s_indev_drv);
  s_ready = (s_indev != nullptr);
  return s_ready;
}

}  // namespace

bool init(uint16_t hor_res, uint16_t ver_res) {
  if (s_indev != nullptr) return true;

  s_hw_pressed = false;
  s_touch_down = false;
  s_last_pressed = false;
  s_release_barrier = ReleaseBarrier::Inactive;
  s_hor_res = hor_res;
  s_ver_res = ver_res;

  if (!initHardware(hor_res, ver_res)) {
    s_ready = false;
    return false;
  }
  return registerPointerIndev();
}

bool isReady() { return s_ready; }

// Keep the input logically active while a consumed gesture is waiting for the
// hardware release.  LVGL may dispatch the corresponding RELEASED/CLICKED
// event in this window even though the controller has already reported no
// contact; callers must not treat that event as a tap.
bool isPressed() { return s_hw_pressed || release_barrier_active(); }

void requestReleaseBarrier() { request_release_barrier(); }

void set_wake_handler(WakeFn fn) { s_wake_fn = fn; }

void set_sample_hook(SampleHookFn fn) { s_sample_hook = fn; }

}  // namespace heltec::meshcore::dal::touch_port

#endif  // HELTEC_HAS_TOUCH
