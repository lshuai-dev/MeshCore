#include "SystemScreen.hpp"
#include "ui/core/HtEvent.h"

namespace heltec::meshcore::ui
{

SystemScreen::SysAction SystemScreen::actionForRow(_lv_obj_t *object) const
{
    if (!object)
        return SysAction::None;
    if (object == _row_factory_reset)
        return SysAction::FactoryReset;
    if (object == _row_clear_data)
        return SysAction::ClearData;
    return SysAction::None;
}

void SystemScreen::handleAction(SysAction action)
{
    if (action != SysAction::FactoryReset && action != SysAction::ClearData)
        return;

    UiConfirmRequest request{};
    request.action = action == SysAction::FactoryReset ? UiConfirmAction::FactoryReset
                                                       : UiConfirmAction::ClearData;
    if (!emitEvent(EventType::ConfirmOpen, &request))
    {
        _feedback.showAlert("Unable to open confirmation", 2000);
    }
}

void SystemScreen::onUIEvent(const UiEvent &event)
{
    if (!event.payload)
        return;
    if (event.type == EventType::MessageKeyboardSubmit)
    {
        const auto *const submit = static_cast<const UiMessageKeyboardSubmit *>(event.payload);
        if (submit->purpose != UiMessageKeyboardPurpose::EditPreset || !submit->text)
            return;
        using Result = biz::IBizFacade::PresetUpdateResult;
        const Result result = _biz.updateMessagePreset(submit->preset_index, submit->text);
        if (result == Result::Updated || result == Result::Unchanged)
        {
            if (!_biz.commitMessagePresets())
            {
                _feedback.showAlert("Preset save failed", 1600);
                return;
            }
            _feedback.showAlert(result == Result::Updated ? "Preset saved" : "Preset unchanged",
                                1000);
            return;
        }
        const char *message = result == Result::Empty       ? "Preset cannot be empty" :
                              result == Result::TooLong     ? "Preset too long" :
                              result == Result::InvalidText ? "Invalid preset text" :
                                                              "Invalid preset";
        _feedback.showAlert(message, 1600);
        return;
    }
    if (event.type != EventType::ConfirmAccepted)
        return;
    const auto action = *static_cast<const UiConfirmAction *>(event.payload);
    executeAction(action == UiConfirmAction::FactoryReset ? SysAction::FactoryReset
                                                          : SysAction::ClearData);
}

void SystemScreen::executeAction(SysAction action)
{
    biz::IBizFacade &app = _biz;
    switch (action)
    {
    case SysAction::FactoryReset:
        if (app.factoryReset())
        {
            _feedback.showAlert("Factory reset complete\nRestarting...", 1600);
        }
        else
        {
            _feedback.showAlert("Factory reset failed", 2000);
        }
        break;
    case SysAction::ClearData:
        if (app.clearUserData())
        {
            syncControlsFromApp(app);
            _feedback.showAlert("Data cleared", 2000);
        }
        else
        {
            _feedback.showAlert("Clear failed", 2000);
        }
        break;
    default:
        break;
    }
}

} // namespace heltec::meshcore::ui
