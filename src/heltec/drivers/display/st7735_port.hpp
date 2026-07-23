#pragma once

namespace heltec::meshcore::dal::st7735 {
bool init();
void deinit();
void setBacklightOn(bool on);
bool isBacklightOn();
}  // namespace heltec::meshcore::dal::st7735
