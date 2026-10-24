#pragma once

#include <lvgl.h>
#include <stdint.h>

namespace Heltec::MeshCore::Ui
{

enum class UiMessageType : uint8_t
{
    None = UINT8_MAX,
    NavOpen = 0,
    NavClose,
    NavActivity,
    TilePreview,
    TileCommit,
    ConfirmOpen,
    ConfirmAccepted,
    ConfirmCancelled,
    ContextOpen,
    ContextClose,
    SendMessageOpen,
    CalibrationOpen,
    RepeatModeOpen,
    QuickPingClose,
    PreviewNext,
    PreviewClose,
    AlertOpen,
    AlertClose,
    RadioSyncClose,
    RepeatModeClose,
    CalibrationClose,
    KeyboardClose,
    SendMessageClose,
    MessageKeyboardOpen,
    WaypointKeyboardOpen,
    WaypointKeyboardClosed,
    MessageKeyboardSubmit,
    WaypointKeyboardSubmit,
};

struct UiMessage
{
    UiMessageType type;
    const void *payload;
    lv_obj_t *source;
};

struct UiAlertRequest
{
    const char *text = nullptr;
    uint16_t duration_ms = 0;
};

enum class UiConfirmAction : uint8_t
{
    FactoryReset,
    ClearData,
    TeamJoin,
    TeamLeave = 4, // Value 3 belonged to a removed action.
};

enum class UiMessageKeyboardPurpose : uint8_t
{
    SendCustom,
    EditPreset,
};

struct UiMessageKeyboardRequest
{
    const char *title = nullptr;
    const char *initial_text = nullptr;
    UiMessageKeyboardPurpose purpose = UiMessageKeyboardPurpose::SendCustom;
    uint8_t preset_index = 0;
    uint8_t max_bytes = 80;
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
    UiMessageKeyboardPurpose purpose = UiMessageKeyboardPurpose::SendCustom;
    uint8_t preset_index = 0;
};

struct UiWaypointKeyboardSubmit
{
    double lat = 0;
    double lon = 0;
};

extern lv_event_code_t UI_MESSAGE_EVENT;

void initUiMessages();
bool sendUiMessage(lv_obj_t *target, UiMessageType type, const void *payload = nullptr,
                   lv_obj_t *source = nullptr);
const UiMessage *getUiMessage(const lv_event_t *event);

} // namespace Heltec::MeshCore::Ui
