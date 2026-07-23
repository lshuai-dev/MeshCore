#pragma once

namespace heltec::meshcore::dal::ssd1306 {
bool init();
void deinit();
void setBacklightOn(bool on);
bool isBacklightOn();
}  // namespace heltec::meshcore::dal::ssd1306
