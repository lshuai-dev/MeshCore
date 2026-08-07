#include "system_screen.hpp"

#include "ui/core/ui_events.h"

namespace heltec::meshcore::ui {

void SystemScreen::handleAction(SysAction action) {
  if (action != SysAction::FactoryReset && action != SysAction::ClearData) return;

  UiConfirmRequest request{};
  request.action = action == SysAction::FactoryReset
                       ? UiConfirmAction::FactoryReset
                       : UiConfirmAction::ClearData;
  if (!emitEvent(UiEventType::ConfirmOpen, &request)) {
    _feedback.showAlert("Unable to open confirmation", 2000);
  }
}

void SystemScreen::onUiEvent(const UiEvent& event) {
  if (event.type != UiEventType::ConfirmAccepted || !event.payload) return;
  const auto action = *static_cast<const UiConfirmAction*>(event.payload);
  executeAction(action == UiConfirmAction::FactoryReset
                    ? SysAction::FactoryReset
                    : SysAction::ClearData);
}

void SystemScreen::executeAction(SysAction action) {
  biz::IBizFacade& app = _biz;
  switch (action) {
    case SysAction::FactoryReset:
      if (app.factoryReset()) {
        _feedback.showAlert("Factory reset complete\nRestarting...", 1600);
      } else {
        _feedback.showAlert("Factory reset failed", 2000);
      }
      break;
    case SysAction::ClearData:
      if (app.clearUserData()) {
        syncControlsFromApp(app);
        _feedback.showAlert("Data cleared", 2000);
      } else {
        _feedback.showAlert("Clear failed", 2000);
      }
      break;
    default:
      break;
  }
}

}  // namespace heltec::meshcore::ui
