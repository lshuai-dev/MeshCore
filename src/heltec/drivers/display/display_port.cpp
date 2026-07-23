#include "display_port.hpp"

#if defined(HELTEC_DISPLAY_SSD1306)
#include "ssd1306_port.hpp"
#elif defined(HELTEC_DISPLAY_ST7735)
#include "st7735_port.hpp"
#elif defined(HELTEC_DISPLAY_ST7789)
#include "st7789_port.hpp"
#endif

namespace heltec::meshcore::dal::display_port {
bool init() {
#if defined(HELTEC_DISPLAY_SSD1306)
  return ssd1306::init();
#elif defined(HELTEC_DISPLAY_ST7735)
  return st7735::init();
#elif defined(HELTEC_DISPLAY_ST7789)
  return st7789::init();
#else
  return false;
#endif
}

void deinit() {
#if defined(HELTEC_DISPLAY_SSD1306)
  ssd1306::deinit();
#elif defined(HELTEC_DISPLAY_ST7735)
  st7735::deinit();
#elif defined(HELTEC_DISPLAY_ST7789)
  st7789::deinit();
#endif
}

void setBacklightOn(bool on) {
#if defined(HELTEC_DISPLAY_SSD1306)
  ssd1306::setBacklightOn(on);
#elif defined(HELTEC_DISPLAY_ST7735)
  st7735::setBacklightOn(on);
#elif defined(HELTEC_DISPLAY_ST7789)
  st7789::setBacklightOn(on);
#else
  (void)on;
#endif
}

bool isBacklightOn() {
#if defined(HELTEC_DISPLAY_SSD1306)
  return ssd1306::isBacklightOn();
#elif defined(HELTEC_DISPLAY_ST7735)
  return st7735::isBacklightOn();
#elif defined(HELTEC_DISPLAY_ST7789)
  return st7789::isBacklightOn();
#else
  return true;
#endif
}

void reinitPanelAfterSharedReset() {
#if defined(HELTEC_DISPLAY_ST7789)
  st7789::reinitPanelAfterSharedReset();
#endif
}

}  // namespace heltec::meshcore::dal::display_port
