#pragma once

namespace heltec::meshcore::dal::nv3001b
{

bool init();
void deinit();
void setBacklightOn(bool on);
bool isBacklightOn();
void suspend();
void resume();

} // namespace heltec::meshcore::dal::nv3001b
