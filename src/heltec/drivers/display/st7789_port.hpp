#pragma once

namespace heltec::meshcore::dal::st7789 {
bool init();
void deinit();
void setBacklightOn(bool on);
bool isBacklightOn();
/** FPC RST shared with CHSC6x; re-run panel init after GPIO21 pulse. */
void reinitPanelAfterSharedReset();
}  // namespace heltec::meshcore::dal::st7789
