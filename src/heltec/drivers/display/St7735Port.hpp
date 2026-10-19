#pragma once

namespace heltec::meshcore::dal::st7735
{
bool init();
void deinit();
void setBacklightOn(bool on);
bool isBacklightOn();
void suspend();
void resume();
} // namespace heltec::meshcore::dal::st7735
