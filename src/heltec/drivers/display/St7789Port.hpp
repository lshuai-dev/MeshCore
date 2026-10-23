#pragma once

namespace Heltec::MeshCore::Dal::ST7789
{
bool init();
void deinit();
void setBacklightOn(bool on);
bool isBacklightOn();
void suspend();
void resume();
/** FPC RST shared with CHSC6x; re-run panel init after GPIO21 pulse. */
void reinitPanelAfterSharedReset();
} // namespace Heltec::MeshCore::Dal::ST7789
