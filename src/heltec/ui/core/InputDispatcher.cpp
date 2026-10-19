#include "InputDispatcher.hpp"

#include "InputHost.hpp"
#include "heltec/drivers/input/KeyInput.hpp"
#include "ui/core/HtEvent.h"

#include <lvgl.h>

namespace heltec::meshcore::ui {

bool InputDispatcher::dispatch(InputHost &host, InputCommand command, uint32_t now_ms) {
    (void)now_ms;
    if (!host.isReady()) return true;

    host.reconcileInput();

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
        if (host.blocksCloseTopLayer())
        {
            return true;
        }
        // Let the active LVGL focused object decide what ESC means for its local state.
        return false;

    case InputCommand::CycleKeyboardNext:
        if (!host.acceptsKeyboardCycle())
        {
            return true;
        }
        return heltec::meshcore::dal::key_input::post(
            LV_KEY_RIGHT,
            heltec::meshcore::dal::key_input::KeyDelivery::IndependentPulse);

    default:
        return false;
    }
}

} // namespace heltec::meshcore::ui
