#include "alert_overlay.hpp"

#include "ui/core/ui_events.h"
#include "ui/core/ht_meta_data.hpp"

#include <lvgl.h>

#if defined(MESH_DEBUG) && MESH_DEBUG && defined(ARDUINO)
#include <Arduino.h>
#define ALERT_LOG(fmt, ...) \
  do { \
    Serial.printf("[alert:ovl] " fmt "\n", ##__VA_ARGS__); \
    Serial.flush(); \
  } while (0)
#else
#define ALERT_LOG(fmt, ...) ((void)0)
#endif

namespace heltec::meshcore::ui {
namespace {

constexpr uint32_t kClickGuardMs = 500;

lv_coord_t parent_width(lv_obj_t* obj) {
  lv_obj_t* parent = obj ? lv_obj_get_parent(obj) : nullptr;
  if (parent) {
    lv_obj_update_layout(parent);
    const lv_coord_t w = lv_obj_get_width(parent);
    if (w > 0) return w;
  }
  lv_disp_t* disp = lv_disp_get_default();
  return disp ? lv_disp_get_hor_res(disp) : 0;
}

lv_coord_t parent_height(lv_obj_t* obj) {
  lv_obj_t* parent = obj ? lv_obj_get_parent(obj) : nullptr;
  if (parent) {
    lv_obj_update_layout(parent);
    const lv_coord_t h = lv_obj_get_height(parent);
    if (h > 0) return h;
  }
  lv_disp_t* disp = lv_disp_get_default();
  return disp ? lv_disp_get_ver_res(disp) : 0;
}

}  // namespace

_lv_obj_t* AlertOverlay::createRoot(_lv_obj_t* parent) {
  return ht_obj_create(parent, meta_id::AlertOverlayRoot);
}

_lv_obj_t* AlertOverlay::create(lv_obj_t* parent) {
  if (!AbstractOverlay::create(parent)) return nullptr;
  lv_obj_set_layout(_root, 0);

  _box = ht_obj_create(_root, meta_id::AlertBox);
  if (!_box) return nullptr;
  lv_obj_set_size(_box, lv_pct(92), LV_SIZE_CONTENT);
  lv_obj_set_style_pad_all(_box, 8, LV_PART_MAIN);
  lv_obj_clear_flag(_box, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(_box, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(_box, [](lv_event_t* e) {
    auto* self = static_cast<AlertOverlay*>(lv_event_get_user_data(e));
    if (!self) return;
    const uint32_t age_ms = lv_tick_elaps(self->_entered_ms);
    ALERT_LOG("box clicked root=%p box=%p label=%p age=%lu",
              self->_root,
              self->_box,
              self->_label,
              (unsigned long)age_ms);
    lv_event_stop_bubbling(e);
    lv_event_stop_processing(e);
    if (age_ms < kClickGuardMs) {
      ALERT_LOG("box click ignored age=%lu guard=%lu",
                (unsigned long)age_ms,
                (unsigned long)kClickGuardMs);
      return;
    }
    (void)self->emitEvent(UiEventType::AlertClose);
  }, LV_EVENT_CLICKED, this);

  _label = ht_label_create(_box, meta_id::AlertLabel);
  if (!_label) return nullptr;
  lv_obj_set_width(_label, lv_pct(100));
  lv_label_set_long_mode(_label, LV_LABEL_LONG_WRAP);
  lv_label_set_text_static(_label, _text_buffer);

  return _root;
}

void AlertOverlay::onEnter() {
  AbstractOverlay::onEnter();
  if (!_root) return;
  _entered_ms = lv_tick_get();

  const lv_coord_t w = parent_width(_root);
  const lv_coord_t h = parent_height(_root);
  if (w > 0 && h > 0) {
    lv_obj_set_size(_root, w, h);
    lv_obj_set_pos(_root, 0, 0);
  }
  lv_obj_set_layout(_root, 0);

  if (_box) {
    const lv_coord_t box_w = w > 24 ? (lv_coord_t)(w - 16) : w;
    if (box_w > 0) lv_obj_set_width(_box, box_w);
    if (_label && box_w > 18) lv_obj_set_width(_label, (lv_coord_t)(box_w - 18));
    lv_obj_update_layout(_box);
    lv_obj_align(_box, LV_ALIGN_CENTER, 0, 0);
  }
  lv_obj_update_layout(_root);
  lv_obj_invalidate(_root);
  ALERT_LOG("enter root=%p hidden=%d root=%dx%d box=%p box=%dx%d label=%p label=%dx%d",
            _root, lv_obj_has_flag(_root, LV_OBJ_FLAG_HIDDEN) ? 1 : 0,
            (int)lv_obj_get_width(_root), (int)lv_obj_get_height(_root),
            _box, _box ? (int)lv_obj_get_width(_box) : -1,
            _box ? (int)lv_obj_get_height(_box) : -1,
            _label, _label ? (int)lv_obj_get_width(_label) : -1,
            _label ? (int)lv_obj_get_height(_label) : -1);
}

void AlertOverlay::onExit() {
  ALERT_LOG("exit root=%p hidden_before=%d box=%p label=%p",
            _root,
            (_root && lv_obj_has_flag(_root, LV_OBJ_FLAG_HIDDEN)) ? 1 : 0,
            _box,
            _label);
  AbstractOverlay::onExit();
  ALERT_LOG("exit done root=%p hidden_after=%d",
            _root,
            (_root && lv_obj_has_flag(_root, LV_OBJ_FLAG_HIDDEN)) ? 1 : 0);
}

void AlertOverlay::setText(const char* text) {
  lv_snprintf(_text_buffer, sizeof(_text_buffer), "%s", text ? text : "");
  if (_label) lv_label_set_text_static(_label, _text_buffer);
  if (_box) {
    const lv_coord_t w = parent_width(_root);
    const lv_coord_t box_w = w > 24 ? (lv_coord_t)(w - 16) : w;
    if (box_w > 0) lv_obj_set_width(_box, box_w);
    if (_label && box_w > 18) lv_obj_set_width(_label, (lv_coord_t)(box_w - 18));
    lv_obj_update_layout(_box);
    lv_obj_align(_box, LV_ALIGN_CENTER, 0, 0);
  }
  ALERT_LOG("set text=%s root=%p box=%p label=%p", text ? text : "", _root, _box, _label);
}

bool AlertOverlay::onKey(uint32_t key) {
  ALERT_LOG("key key=0x%lX root=%p box=%p", (unsigned long)key, _root, _box);
  if (key == LV_KEY_ENTER) {
    ALERT_LOG("key enter swallowed");
    return true;
  }
  if (key == LV_KEY_ESC) {
    ALERT_LOG("key esc close");
    return emitEvent(UiEventType::AlertClose);
  }
  return false;
}

}  // namespace heltec::meshcore::ui
