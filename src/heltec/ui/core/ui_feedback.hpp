#pragma once

namespace heltec::meshcore::ui {

class IFeedback {
 public:
  virtual ~IFeedback() = default;
  virtual void showAlert(const char* text, int duration_ms) = 0;
};

}  // namespace heltec::meshcore::ui
