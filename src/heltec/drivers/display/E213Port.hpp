#pragma once

namespace heltec::meshcore::dal::e213 {

bool init();
void deinit();

// E213 has no backlight. These methods control the shared peripheral rail
// while leaving the bistable panel image intact.
void setBacklightOn(bool on);
bool isBacklightOn();
void reinitPanelAfterSharedReset();

// Render and synchronously commit a fully constructed boot frame. When the
// retained image is unknown, a fast white baseline can replace a full waveform.
bool commitBootFrame(const char* reason = nullptr,
                     bool prepare_white_partial_baseline = false);

// Navigation is a semantic UI transaction on the bistable E-paper panel.
// While the transaction is active, LVGL may render intermediate states into
// the panel cache without starting a physical update for every state.
void beginNavigationSession();
void navigationActivity();
void endNavigationSession();
bool navigationRefreshPending();
bool consumeNavigationCommitCompleted();
// Queue one physical panel update after the current LVGL frame is flushed.
// This is a critical UI commit. Intermediate LVGL frames are coalesced until
// the UI has been quiet for the configured settle interval.
void requestRefresh(const char* reason = nullptr);

// Commit a changed in-page focus frame. Navigation owns its own coalesced
// refresh transaction, so selection requests raised while it is active are
// folded into that transaction instead of producing another panel update.
void requestSelectionRefresh(const char* reason = nullptr);

// Mark a background/status change for a low-priority, rate-limited commit.
// Background changes are merged with the next critical commit when possible.
void requestDeferredRefresh(const char* reason = nullptr);

}  // namespace heltec::meshcore::dal::e213
