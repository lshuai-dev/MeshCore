#pragma once

namespace Heltec::MeshCore::Dal::NV3001B
{

bool init();
void deinit();
void setBacklightOn(bool on);
bool isBacklightOn();
void suspend();
void resume();

} // namespace Heltec::MeshCore::Dal::NV3001B
