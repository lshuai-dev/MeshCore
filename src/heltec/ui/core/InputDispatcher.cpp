#include "InputDispatcher.hpp"

#include "UiInputHost.hpp"
#include "heltec/drivers/input/KeyInput.hpp"
#include "ui/core/HtEvent.h"

#include <lvgl.h>

namespace Heltec::MeshCore::Ui {

bool InputDispatcher::dispatch(IUiInputHost &host, InputCommand command, uint32_t now_ms) {
    (void)now_ms;
    if (!host.isReady()) return true;

    /* Long-repeat gestures are emitted from the keypad read callback. Avoid
       rebinding its group while LVGL is processing that same input device. */
    if (command == InputCommand::CycleKeyboardNext)
    {
        if (!host.acceptsKeyboardCycle())
        {
            return true;
        }
        return Heltec::MeshCore::Dal::KeyInput::post(
            LV_KEY_RIGHT,
            Heltec::MeshCore::Dal::KeyInput::KeyDelivery::IndependentPulse);
    }

    switch (command) {

    case InputCommand::GoHome:
        host.ensureTileKeypadFocus();
        return true;

    case InputCommand::OpenNavigation:
        if (lv_obj_t *frame = host.frameRoot()) {
            ht_event_send(frame, EventType::NavOpen);
        }
        return true;

    case InputCommand::OpenAction:
        if (lv_obj_t *frame = host.frameRoot()) {
            ht_event_send(frame, EventType::ActionOpen);
        }
        return true;

    case InputCommand::CloseTopLayer:
        // Let the active LVGL focused object decide what ESC means for its local state.
        return false;

    default:
        return false;
    }
}

} // namespace Heltec::MeshCore::Ui
