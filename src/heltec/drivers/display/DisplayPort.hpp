#pragma once

namespace heltec::meshcore::dal::display_port
{

bool init();
void deinit();

/** Backlight / panel power (ST7735 LEDA pin or SSD1306 DISPLAYON/OFF). */
void setBacklightOn(bool on);
bool isBacklightOn();
void suspend();
void resume();
void reinitPanelAfterSharedReset();

#if defined(HELTEC_HAS_KEY_FRAME) && HELTEC_HAS_KEY_FRAME
bool commitBootFrame(const char *reason = nullptr,
                     bool prepare_white_partial_baseline = false);
void beginNavigationSession();
void navigationActivity();
void endNavigationSession();
void requestRefresh(const char *reason = nullptr);
void requestSelectionRefresh(const char *reason = nullptr);
void requestDeferredRefresh(const char *reason = nullptr);
#endif

} // namespace heltec::meshcore::dal::display_port
