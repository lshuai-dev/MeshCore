#include "ssd1306_port.hpp"

#include <Arduino.h>
#include <Wire.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <lvgl.h>

#ifndef DISPLAY_ADDRESS
#define DISPLAY_ADDRESS 0x3C
#endif

namespace heltec::meshcore::dal::ssd1306 {
namespace {
static Adafruit_SSD1306* s_oled = nullptr;
static bool s_hw_inited = false;
static bool s_display_on = true;

static constexpr int kDisplayWidth = 128;
static constexpr int kDisplayHeight = 64;
static constexpr uint32_t kFullBufferPx = (uint32_t)kDisplayWidth * (uint32_t)kDisplayHeight;

// LVGL full-screen draw buffer. Adafruit_SSD1306 keeps the hardware 1-bit cache.
LV_ATTRIBUTE_FAST_MEM LV_ATTRIBUTE_MEM_ALIGN static lv_color_t s_buf[kFullBufferPx];

static bool hw_begin(int w, int h) {
  if (s_hw_inited) return s_oled != nullptr;

  // Reset pin may be -1 on some boards; Adafruit handles it.
#ifndef PIN_OLED_RESET
#define PIN_OLED_RESET -1
#endif

  static Adafruit_SSD1306 oled(w, h, &Wire, PIN_OLED_RESET);
  s_oled = &oled;

  // Assume Wire is already configured by board.begin(); still safe to call begin on most cores.
#if !defined(PIN_BOARD_SDA) || !defined(PIN_BOARD_SCL)
  Wire.begin();
#endif
  const bool begin_ok = s_oled->begin(SSD1306_SWITCHCAPVCC, DISPLAY_ADDRESS, true, false);
  if (!begin_ok) {
    s_oled = nullptr;
    return false;
  }
  s_oled->clearDisplay();
  s_oled->display();
  s_hw_inited = true;
  return true;
}

static lv_disp_draw_buf_t s_draw_buf;
static lv_disp_drv_t s_disp_drv;

}  // namespace

bool init() {
  if (!hw_begin(kDisplayWidth, kDisplayHeight)) return false;

  lv_disp_draw_buf_init(&s_draw_buf, s_buf, nullptr, kFullBufferPx);

  lv_disp_drv_init(&s_disp_drv);
  s_disp_drv.hor_res = kDisplayWidth;
  s_disp_drv.ver_res = kDisplayHeight;
  s_disp_drv.draw_buf = &s_draw_buf;
  s_disp_drv.full_refresh = 1;
  s_disp_drv.flush_cb = [](lv_disp_drv_t* drv, const lv_area_t* area, lv_color_t* color_p) {
    (void)drv;
    if (!s_oled || !s_display_on) {
      lv_disp_flush_ready(drv);
      return;
    }

    const int32_t w = area->x2 - area->x1 + 1;
    const int32_t h = area->y2 - area->y1 + 1;
    if (w <= 0 || h <= 0) {
      lv_disp_flush_ready(drv);
      return;
    }

    // Convert the LVGL color buffer to the SSD1306 1-bit cache.
    for (int32_t y = 0; y < h; y++) {
      for (int32_t x = 0; x < w; x++) {
        const int16_t px = (int16_t)(area->x1 + x);
        const int16_t py = (int16_t)(area->y1 + y);
        const uint8_t lum = lv_color_brightness(color_p[y * w + x]);
        if (lum > 96) s_oled->drawPixel(px, py, SSD1306_WHITE);
        else s_oled->drawPixel(px, py, SSD1306_BLACK);
      }
    }

    s_oled->display();

    lv_disp_flush_ready(drv);
  };

  lv_disp_t* disp = lv_disp_drv_register(&s_disp_drv);
  if (!disp) return false;

  return true;
}

void deinit() {
  lv_disp_t* d = lv_disp_get_default();
  if (d) lv_disp_remove(d);
}

void setBacklightOn(bool on) {
  if (!s_oled) return;
  s_display_on = on;
  s_oled->ssd1306_command(on ? SSD1306_DISPLAYON : SSD1306_DISPLAYOFF);
}

bool isBacklightOn() { return s_display_on; }

}  // namespace heltec::meshcore::dal::ssd1306
