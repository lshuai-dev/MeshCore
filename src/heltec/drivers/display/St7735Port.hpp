#pragma once

namespace Heltec::MeshCore::Dal::St7735
{
bool init();
void deinit();
void setBacklightOn(bool on);
bool isBacklightOn();
void suspend();
void resume();
} // namespace Heltec::MeshCore::Dal::St7735
