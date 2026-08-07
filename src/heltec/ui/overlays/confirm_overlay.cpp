#include "confirm_overlay.hpp"

#include "ui/core/ht_meta_data.hpp"
#include "ui/core/operation_hints.hpp"

#include <lvgl.h>

namespace heltec::meshcore::ui {

namespace {

const char* confirmation_body(UiConfirmAction action) {
  return action == UiConfirmAction::FactoryReset
             ? "Erase all settings and data?"
             : "Clear contacts and user data?";
}

}  // namespace

_lv_obj_t* ConfirmOverlay::createRoot(_lv_obj_t* parent) {
  return ht_obj_create(parent, meta_id::ConfirmOverlayRoot);
}

_lv_obj_t* ConfirmOverlay::create(_lv_obj_t* parent) {
  if (!AbstractOverlay::create(parent)) return nullptr;

  lv_obj_set_layout(_root, 0);
  lv_obj_set_flex_align(_root, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_add_flag(_root, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(_root, onBackdropClicked, LV_EVENT_CLICKED, this);

  _box = ht_obj_create(_root, meta_id::ConfirmBox);
  if (!_box) return nullptr;

  lv_disp_t* const display = lv_disp_get_default();
  const bool compact = display &&
                       (lv_disp_get_hor_res(display) <= 160 ||
                        lv_disp_get_ver_res(display) <= 128);
  lv_obj_set_size(_box, compact ? lv_pct(94) : lv_pct(86), LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(_box, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(_box, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_clear_flag(_box, LV_OBJ_FLAG_SCROLLABLE);

  _lv_obj_t* const title = ht_label_create(_box, meta_id::ConfirmTitle, "Confirm");
  if (title) lv_obj_set_width(title, lv_pct(100));

  _body = ht_label_create(_box, meta_id::ConfirmBody,
                          confirmation_body(_action));
  if (!_body) return nullptr;
  lv_obj_set_width(_body, lv_pct(100));
  lv_label_set_long_mode(_body, LV_LABEL_LONG_WRAP);

#if defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
  const lv_coord_t button_height = compact ? 20 : LV_DPX(42);
  _lv_obj_t* const button_row = ht_obj_create(_box, meta_id::ConfirmButtonRow);
  if (!button_row) return nullptr;
  lv_obj_set_size(button_row, lv_pct(100), button_height);
  lv_obj_set_flex_flow(button_row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(button_row, LV_FLEX_ALIGN_SPACE_BETWEEN,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_clear_flag(button_row, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

  const auto create_button = [this, button_row](const char* label_text,
                                                _lv_obj_t** out_button) {
    _lv_obj_t* const button = ht_btn_create(button_row, meta_id::ConfirmButton);
    if (!button) return;
    lv_obj_set_height(button, lv_pct(100));
    lv_obj_set_width(button, lv_pct(46));
    lv_obj_add_event_cb(button, onButtonClicked, LV_EVENT_CLICKED, this);
    _lv_obj_t* const label = ht_label_create(button, meta_id::ConfirmButtonLabel,
                                             label_text);
    if (label) lv_obj_center(label);
    *out_button = button;
  };

  create_button("Cancel", &_cancel);
  create_button("Confirm", &_accept);
  if (!_cancel || !_accept) return nullptr;
#else
  _lv_obj_t* const key_hint = ht_label_create(
      _box, meta_id::ConfirmKeyHint, operation_hint::kDestructiveConfirm);
  if (key_hint) {
    lv_obj_set_width(key_hint, lv_pct(100));
    lv_label_set_long_mode(key_hint, LV_LABEL_LONG_CLIP);
  }
#endif

  lv_obj_update_layout(_root);
  lv_obj_center(_box);
  return _root;
}

_lv_obj_t* ConfirmOverlay::focusTarget() const {
  return _root;
}

void ConfirmOverlay::setRequest(UiConfirmAction action) {
  _action = action;
  if (_body) lv_label_set_text_static(_body, confirmation_body(_action));
}

bool ConfirmOverlay::onKey(uint32_t key) {
  if (key == LV_KEY_ESC) return emitEvent(UiEventType::ConfirmCancelled);
  if (key == LV_KEY_ENTER) return emitEvent(UiEventType::ConfirmAccepted, &_action);
  return false;
}

void ConfirmOverlay::onBackdropClicked(lv_event_t* e) {
  auto* self = static_cast<ConfirmOverlay*>(lv_event_get_user_data(e));
  if (!self || lv_event_get_target(e) != self->_root) return;
  (void)self->emitEvent(UiEventType::ConfirmCancelled);
  lv_event_stop_processing(e);
  lv_event_stop_bubbling(e);
}

void ConfirmOverlay::onButtonClicked(lv_event_t* e) {
  auto* self = static_cast<ConfirmOverlay*>(lv_event_get_user_data(e));
  if (!self) return;
  _lv_obj_t* const target = lv_event_get_target(e);
  if (target == self->_cancel) {
    (void)self->emitEvent(UiEventType::ConfirmCancelled);
  } else if (target == self->_accept) {
    (void)self->emitEvent(UiEventType::ConfirmAccepted, &self->_action);
  }
  lv_event_stop_processing(e);
  lv_event_stop_bubbling(e);
}

}  // namespace heltec::meshcore::ui
