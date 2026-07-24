#pragma once

#include <lvgl.h>
#include <stdint.h>

namespace heltec::meshcore::ui {

enum class UiEventType : uint8_t {
  NavOpen,
  NavClose,
  NavActivity,
  TileCommit,
  ContextOpen,
  ContextClose,
  QuickPingOpen,
  QuickPingClose,
  PreviewClose,
  AlertClose,
  RadioSyncClose,
  CalibrationClose,
  KeyboardClose,
  SendMessageClose,
  MessageKeyboardOpen,
  WaypointKeyboardOpen,
  WaypointKeyboardClosed,
  MessageKeyboardSubmit,
  WaypointKeyboardSubmit,
  RebindInput,
  AppStateChanged,
  SurfaceRefresh,
};

struct UiEvent {
  UiEventType type;
  const void* payload;
};

struct UiMessageKeyboardRequest {
  const char* title = nullptr;
};

struct UiMessageKeyboardSubmit {
  const char* text = nullptr;
};

struct UiWaypointKeyboardSubmit {
  double lat = 0;
  double lon = 0;
};

void ui_events_init();
lv_event_code_t ui_event_code();
bool ui_event_send(lv_obj_t* target, UiEventType type,
                   const void* payload = nullptr);
const UiEvent* ui_event_get(const lv_event_t* event);

}  // namespace heltec::meshcore::ui
