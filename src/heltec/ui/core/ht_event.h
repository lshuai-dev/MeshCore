#pragma once

#include <lvgl.h>
#include <stdint.h>

namespace heltec::meshcore::ui
{

enum class EventType : uint8_t
{
    NAV_OPEN,
    NAV_CLOSE,
    NAV_ACTIVITY,
    TILE_PREVIEW,
    TILE_COMMIT,
    ACTION_OPEN,
    CONFIRM_OPEN,
    CONFIRM_ACCEPTED,
    CONFIRM_CANCELLED,
    CONTEXT_OPEN,
    CONTEXT_CLOSE,
    SEND_MESSAGE_OPEN,
    CALIBRATION_OPEN,
    REPEAT_MODE_OPEN,
    QUICK_PING_OPEN,
    QUICK_PING_CLOSE,
    PREVIEW_NEXT,
    PREVIEW_CLOSE,
    ALERT_CLOSE,
    RADIO_SYNC_CLOSE,
    REPEAT_MODE_CLOSE,
    CALIBRATION_CLOSE,
    KEYBOARD_CLOSE,
    SEND_MESSAGE_CLOSE,
    MESSAGE_KEYBOARD_OPEN,
    WAYPOINT_KEYBOARD_OPEN,
    WAYPOINT_KEYBOARD_CLOSED,
    MESSAGE_KEYBOARD_SUBMIT,
    WAYPOINT_KEYBOARD_SUBMIT,
    REBIND_INPUT,
    APP_STATE_CHANGED,
    SURFACE_REFRESH,
};

struct UiEvent
{
    EventType type;
    const void *payload;
};

enum class UiConfirmAction : uint8_t
{
    FactoryReset,
    ClearData,
};

struct UiConfirmRequest
{
    UiConfirmAction action = UiConfirmAction::FactoryReset;
};

struct UiMessageKeyboardRequest
{
    const char *title = nullptr;
};

enum class UiMessageTargetKind : uint8_t
{
    Direct = 0,
    Channel = 1,
};

struct UiSendMessageTarget
{
    UiMessageTargetKind kind = UiMessageTargetKind::Direct;
    uint8_t pub_key_prefix[6] = {};
    int channel_idx = -1;
    char label[24] = {};
};

struct UiMessageKeyboardSubmit
{
    const char *text = nullptr;
};

struct UiWaypointKeyboardSubmit
{
    double lat = 0;
    double lon = 0;
};

void ht_event_init();
extern lv_event_code_t HT_EVENT_USER;
bool ht_event_send(lv_obj_t *target, EventType type, const void *payload = nullptr);
const UiEvent *ht_event_get(const lv_event_t *event);

} // namespace heltec::meshcore::ui
