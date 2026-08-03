#pragma once

#include <stdint.h>

namespace heltec::meshcore::ui {

/** Host surface for transient overlays driven by UiTask (preview, alert). */
class IUiHost {
 public:
  virtual ~IUiHost() = default;

  virtual bool isReady() const = 0;
  virtual void openPreviewOverlay(uint8_t unread, uint32_t received_ms,
                                  const char* origin, const char* text) = 0;
  virtual void closePreviewOverlay() = 0;
  virtual void openAlertOverlay(const char* text) = 0;
  virtual void closeAlertOverlay() = 0;
};

}  // namespace heltec::meshcore::ui
