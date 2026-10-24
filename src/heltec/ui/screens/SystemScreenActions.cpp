#include "SystemScreen.hpp"
#include "ui/core/UiMessage.hpp"

namespace Heltec::MeshCore::Ui
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

    const UiConfirmAction request = action == SysAction::FactoryReset
                                        ? UiConfirmAction::FactoryReset
                                        : UiConfirmAction::ClearData;
    if (!notifyApp(UiMessageType::ConfirmOpen, &request))
    {
        showAlert("Unable to open confirmation", 2000);
    }
}

bool SystemScreen::onKey(uint32_t key)
{
    if (key == LV_KEY_ENTER && _gridNav.mode() == HtGridNav::Mode::Controls)
    {
        const SysAction action = actionForRow(_gridNav.focused());
        if (action != SysAction::None)
        {
            handleAction(action);
            return true;
        }
    }
    return AbstractScreen::onKey(key);
}

void SystemScreen::onAppEvent(const UiMessage &event)
{
    if (!event.payload)
        return;
    if (event.type == UiMessageType::MessageKeyboardSubmit)
    {
        const auto *const submit = static_cast<const UiMessageKeyboardSubmit *>(event.payload);
        if (submit->purpose != UiMessageKeyboardPurpose::EditPreset || !submit->text)
            return;
        using Result = Biz::IBizFacade::PresetUpdateResult;
        const Result result = _biz.updateMessagePreset(submit->preset_index, submit->text);
        if (result == Result::Updated || result == Result::Unchanged)
        {
            if (!_biz.commitMessagePresets())
            {
                showAlert("Preset save failed", 1600);
                return;
            }
            showAlert(result == Result::Updated ? "Preset saved" : "Preset unchanged",
                                1000);
            return;
        }
        const char *message = result == Result::Empty       ? "Preset cannot be empty" :
                              result == Result::TooLong     ? "Preset too long" :
                              result == Result::InvalidText ? "Invalid preset text" :
                                                              "Invalid preset";
        showAlert(message, 1600);
        return;
    }
    if (event.type != UiMessageType::ConfirmAccepted)
        return;
    const auto action = *static_cast<const UiConfirmAction *>(event.payload);
    executeAction(action == UiConfirmAction::FactoryReset ? SysAction::FactoryReset
                                                          : SysAction::ClearData);
}

void SystemScreen::executeAction(SysAction action)
{
    Biz::IBizFacade &app = _biz;
    switch (action)
    {
    case SysAction::FactoryReset:
        if (app.factoryReset())
        {
            showAlert("Factory reset complete\nRestarting...", 1600);
        }
        else
        {
            showAlert("Factory reset failed", 2000);
        }
        break;
    case SysAction::ClearData:
        if (app.clearUserData())
        {
            syncControlsFromApp(app);
            showAlert("Data cleared", 2000);
        }
        else
        {
            showAlert("Clear failed", 2000);
        }
        break;
    default:
        break;
    }
}

} // namespace Heltec::MeshCore::Ui
