#include "backlight_policy.hpp"

#include "input_host.hpp"

namespace heltec::meshcore::ui {

BacklightMode BacklightPolicy::mode() {
#if defined(HELTEC_INPUT_BACKLIGHT_MANUAL_TOGGLE) && HELTEC_INPUT_BACKLIGHT_MANUAL_TOGGLE
  return BacklightMode::ManualToggle;
#else
  return BacklightMode::AutoTimeout;
#endif
}

bool BacklightPolicy::handle(InputHost& host, const InputEvent& event) {
  const uint32_t now_ms = event.timestamp_ms;
  const bool backlight_on = host.isDisplayOn();

  switch (mode()) {
    case BacklightMode::ManualToggle:
      if (!backlight_on) return true;
      break;

    case BacklightMode::AutoTimeout:
    default:
      if (!backlight_on) {
        host.notifyDisplayActivity(now_ms);
        return true;
      }
      if (event.source != InputSource::None) host.notifyDisplayActivity(now_ms);
      break;
  }

  return false;
}

bool BacklightPolicy::handleCommand(InputHost& host, InputCommand command, uint32_t now_ms) {
  if (command == InputCommand::ToggleBacklight) {
    host.toggleDisplay(now_ms);
    return true;
  }

  if (command == InputCommand::WakeBacklight) {
    if (!host.isDisplayOn()) {
      host.notifyDisplayActivity(now_ms);
      return true;
    }
  }

  return false;
}

}  // namespace heltec::meshcore::ui
