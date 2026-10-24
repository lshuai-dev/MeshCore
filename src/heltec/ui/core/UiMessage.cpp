#include "UiMessage.hpp"

namespace Heltec::MeshCore::Ui
{

lv_event_code_t UI_MESSAGE_EVENT = LV_EVENT_ALL;

void initUiMessages()
{
    if (UI_MESSAGE_EVENT == LV_EVENT_ALL)
    {
        UI_MESSAGE_EVENT = static_cast<lv_event_code_t>(lv_event_register_id());
    }
}

bool sendUiMessage(lv_obj_t *target, UiMessageType type, const void *payload, lv_obj_t *source)
{
    if (!target || UI_MESSAGE_EVENT == LV_EVENT_ALL)
    {
        return false;
    }

    const UiMessage message{type, payload, source};
    return lv_event_send(target, UI_MESSAGE_EVENT, const_cast<UiMessage *>(&message)) == LV_RES_OK;
}

const UiMessage *getUiMessage(const lv_event_t *event)
{
    if (!event || UI_MESSAGE_EVENT == LV_EVENT_ALL ||
        lv_event_get_code(const_cast<lv_event_t *>(event)) != UI_MESSAGE_EVENT)
    {
        return nullptr;
    }

    return static_cast<const UiMessage *>(
        lv_event_get_param(const_cast<lv_event_t *>(event)));
}

} // namespace Heltec::MeshCore::Ui
