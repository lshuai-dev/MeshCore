#pragma once

namespace heltec::meshcore::dal::display_port {

bool init();
void deinit();

/** Backlight / panel power (ST7735 LEDA pin or SSD1306 DISPLAYON/OFF). */
void setBacklightOn(bool on);
bool isBacklightOn();
void reinitPanelAfterSharedReset();

}  // namespace heltec::meshcore::dal::display_port
