#pragma once

#include <lvgl.h>
#include <stdint.h>

namespace Heltec::MeshCore::Ui {

enum class EventType : uint8_t {
    NavOpen,
    NavClose,
    NavActivity,
    TilePreview,
    TileCommit,
    ActionOpen,
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

struct UiEvent {
    EventType type;
    const void *payload;
};

struct UiAlertRequest {
    const char *text = nullptr;
    uint16_t duration_ms = 0;
};

enum class UiConfirmAction : uint8_t {
    FactoryReset,
    ClearData,
    TeamJoin,
    TeamLeave = 4, // Preserve existing event values; 3 was the removed reset action.
};

struct UiConfirmRequest {
    UiConfirmAction action = UiConfirmAction::FactoryReset;
};

enum class UiMessageKeyboardPurpose : uint8_t {
    SendCustom,
    EditPreset,
};

struct UiMessageKeyboardRequest {
    const char *title = nullptr;
    const char *initial_text = nullptr;
    UiMessageKeyboardPurpose purpose = UiMessageKeyboardPurpose::SendCustom;
    uint8_t preset_index = 0;
    uint8_t max_bytes = 80;
};

enum class UiMessageTargetKind : uint8_t {
    Direct = 0,
    Channel = 1,
};

struct UiSendMessageTarget {
    UiMessageTargetKind kind = UiMessageTargetKind::Direct;
    uint8_t pub_key_prefix[6] = {};
    int channel_idx = -1;
    char label[24] = {};
};

struct UiMessageKeyboardSubmit {
    const char *text = nullptr;
    UiMessageKeyboardPurpose purpose = UiMessageKeyboardPurpose::SendCustom;
    uint8_t preset_index = 0;
};

struct UiWaypointKeyboardSubmit {
    double lat = 0;
    double lon = 0;
};

void ht_event_init();
extern lv_event_code_t HT_EVENT_USER;
bool ht_event_send(lv_obj_t *target, EventType type, const void *payload = nullptr);
const UiEvent *ht_event_get(const lv_event_t *event);

} // namespace Heltec::MeshCore::Ui
