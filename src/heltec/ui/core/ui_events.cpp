#include "ui_events.h"

namespace heltec::meshcore::ui {
namespace {

lv_event_code_t s_ui_event_code = LV_EVENT_ALL;

}  // namespace

void ui_events_init() {
  if (s_ui_event_code == LV_EVENT_ALL) {
    s_ui_event_code = static_cast<lv_event_code_t>(lv_event_register_id());
  }
}

lv_event_code_t ui_event_code() {
  LV_ASSERT(s_ui_event_code != LV_EVENT_ALL);
  return s_ui_event_code;
}

bool ui_event_send(lv_obj_t* target, UiEventType type, const void* payload) {
  if (!target || s_ui_event_code == LV_EVENT_ALL) return false;
  const UiEvent event{type, payload};
  return lv_event_send(target, s_ui_event_code,
                       const_cast<UiEvent*>(&event)) == LV_RES_OK;
}

const UiEvent* ui_event_get(const lv_event_t* event) {
  if (!event || s_ui_event_code == LV_EVENT_ALL ||
      lv_event_get_code(const_cast<lv_event_t*>(event)) != s_ui_event_code) {
    return nullptr;
  }
  return static_cast<const UiEvent*>(
      lv_event_get_param(const_cast<lv_event_t*>(event)));
}

}  // namespace heltec::meshcore::ui
