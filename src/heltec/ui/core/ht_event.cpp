#include "ht_event.h"

namespace heltec::meshcore::ui
{

lv_event_code_t HT_EVENT_USER = LV_EVENT_ALL;

void ht_event_init()
{
    if (HT_EVENT_USER == LV_EVENT_ALL)
    {
        HT_EVENT_USER = static_cast<lv_event_code_t>(lv_event_register_id());
    }
}

bool ht_event_send(lv_obj_t *target, EventType type, const void *payload)
{
    if (!target || HT_EVENT_USER == LV_EVENT_ALL)
        return false;
    const UiEvent event{ type, payload };
    return lv_event_send(target, HT_EVENT_USER, const_cast<UiEvent *>(&event)) == LV_RES_OK;
}

const UiEvent *ht_event_get(const lv_event_t *event)
{
    if (!event || HT_EVENT_USER == LV_EVENT_ALL ||
        lv_event_get_code(const_cast<lv_event_t *>(event)) != HT_EVENT_USER)
    {
        return nullptr;
    }
    return static_cast<const UiEvent *>(lv_event_get_param(const_cast<lv_event_t *>(event)));
}

} // namespace heltec::meshcore::ui
