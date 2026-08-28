#pragma once

namespace heltec::meshcore::dal::nv3001b
{

bool init();
void deinit();
void setBacklightOn(bool on);
bool isBacklightOn();

} // namespace heltec::meshcore::dal::nv3001b
