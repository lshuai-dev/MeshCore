#include "input_dispatcher.hpp"

#include "input_host.hpp"
#include "ui/core/ht_event.h"

#include <lvgl.h>

namespace heltec::meshcore::ui
{

bool InputDispatcher::dispatch(InputHost &host, InputCommand command, uint32_t now_ms)
{
    (void)now_ms;
    if (!host.isReady())
        return true;

    host.reconcileInput();

    switch (command)
    {

    case InputCommand::GoHome:
        host.ensureTileKeypadFocus();
        return true;

    case InputCommand::OpenNavigation:
        if (lv_obj_t *frame = host.frameRoot())
        {
            ht_event_send(frame, EventType::NAV_OPEN);
        }
        return true;

    case InputCommand::OpenAction:
        if (lv_obj_t *frame = host.frameRoot())
        {
            ht_event_send(frame, EventType::ACTION_OPEN);
        }
        return true;

    case InputCommand::CloseTopLayer:
        // Let the active LVGL focused object decide what ESC means for its local state.
        return false;

    default:
        return false;
    }
}

} // namespace heltec::meshcore::ui
