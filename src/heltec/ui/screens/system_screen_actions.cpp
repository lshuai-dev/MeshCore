#include "system_screen.hpp"

#include "ui/app/ui_theme.hpp"
#include "ui/core/ht_meta_data.hpp"
#include "ui/core/operation_hints.hpp"
#include "ui/core/ui_events.h"
#include <Arduino.h>

namespace heltec::meshcore::ui {

void SystemScreen::handleAction(SysAction action) {
  if (action == SysAction::FactoryReset || action == SysAction::ClearData) {
    openActionConfirmation(action);
    return;
  }
  executeAction(action);
}

void SystemScreen::executeAction(SysAction action) {
  biz::IBizFacade& app = _biz;
  switch (action) {
    case SysAction::WpGps:
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
    {
      const biz::IBizFacade::GpsStatus gps = app.gpsStatus();
      if (gps.fix_valid && app.setFindFriendWaypoint(gps.lat_deg, gps.lon_deg)) {
        char buf[48];
        app.formatFindFriendWaypointInput(buf, sizeof(buf));
        _feedback.showAlert(buf[0] ? buf : "Saved", 3000);
      } else if (!gps.fix_valid) {
        _feedback.showAlert("Need GPS fix", 3000);
      }
    }
#endif
      break;
    case SysAction::WpManual:
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
        emitEvent(UiEventType::WaypointKeyboardOpen);
#endif
      break;
    case SysAction::FactoryReset:
      if (app.factoryReset()) {
        _feedback.showAlert("Factory reset complete\nRestarting...", 1600);
      } else {
        _feedback.showAlert("Factory reset failed", 2000);
      }
      break;
    case SysAction::ClearData:
      if (app.clearUserData()) {
        syncControlsFromApp(app);
        _feedback.showAlert("Data cleared", 2000);
      } else {
        _feedback.showAlert("Clear failed", 2000);
      }
      break;
    default:
      break;
  }
}

bool SystemScreen::createActionConfirmation() {
  if (_action_confirm_root && lv_obj_is_valid(_action_confirm_root)) return true;
  _action_confirm_root = lv_obj_create(lv_layer_top());
  if (!_action_confirm_root) {
    return false;
  }

  lv_obj_remove_style_all(_action_confirm_root);
  lv_obj_set_size(_action_confirm_root, lv_pct(100), lv_pct(100));
  lv_obj_set_pos(_action_confirm_root, 0, 0);
  lv_obj_add_flag(_action_confirm_root, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(_action_confirm_root, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(_action_confirm_root, lv_color_black(), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(_action_confirm_root, LV_OPA_50, LV_PART_MAIN);
  lv_obj_move_foreground(_action_confirm_root);

  _action_confirm_box = lv_obj_create(_action_confirm_root);
  if (!_action_confirm_box) {
    lv_obj_del(_action_confirm_root);
    _action_confirm_root = nullptr;
    return false;
  }

  lv_disp_t* const display = lv_disp_get_default();
  const bool compact = display &&
                       (lv_disp_get_hor_res(display) <= 160 || lv_disp_get_ver_res(display) <= 128);
  const lv_coord_t dialog_pad = compact ? 4 : LV_DPX(12);
  const lv_coord_t dialog_gap = compact ? 4 : LV_DPX(12);

  lv_obj_set_size(_action_confirm_box, compact ? lv_pct(94) : lv_pct(86), LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(_action_confirm_box, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(_action_confirm_box, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_clear_flag(_action_confirm_box, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(_action_confirm_box, ui_color_overlay_bg(), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(_action_confirm_box, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_color(_action_confirm_box, ui_color_overlay_fg(), LV_PART_MAIN);
  lv_obj_set_style_border_width(_action_confirm_box, 1, LV_PART_MAIN);
  lv_obj_set_style_radius(_action_confirm_box, compact ? 2 : LV_DPX(8), LV_PART_MAIN);
  lv_obj_set_style_pad_all(_action_confirm_box, dialog_pad, LV_PART_MAIN);
  lv_obj_set_style_pad_row(_action_confirm_box, dialog_gap, LV_PART_MAIN);
  lv_obj_set_style_text_color(_action_confirm_box, ui_color_overlay_fg(), LV_PART_MAIN);

  lv_obj_t* const title = lv_label_create(_action_confirm_box);
  if (title) {
    lv_obj_set_width(title, lv_pct(100));
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, ui_color_overlay_fg(), LV_PART_MAIN);
    lv_label_set_text_static(title, "Confirm");
  }

  _action_confirm_body = lv_label_create(_action_confirm_box);
  if (!_action_confirm_body) {
    lv_obj_del(_action_confirm_root);
    _action_confirm_root = nullptr;
    _action_confirm_box = nullptr;
    return false;
  }
  lv_obj_set_width(_action_confirm_body, lv_pct(100));
  lv_obj_set_style_text_align(_action_confirm_body, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_set_style_text_color(_action_confirm_body, ui_color_overlay_fg(), LV_PART_MAIN);
  lv_label_set_long_mode(_action_confirm_body, LV_LABEL_LONG_WRAP);
  lv_label_set_text_static(_action_confirm_body, "");

#if defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
  const lv_coord_t button_height = compact ? 20 : LV_DPX(42);
  lv_obj_t* const button_row = lv_obj_create(_action_confirm_box);
  if (!button_row) {
    lv_obj_del(_action_confirm_root);
    _action_confirm_root = nullptr;
    _action_confirm_box = nullptr;
    _action_confirm_body = nullptr;
    return false;
  }
  lv_obj_remove_style_all(button_row);
  lv_obj_set_size(button_row, lv_pct(100), button_height);
  lv_obj_set_flex_flow(button_row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(button_row, LV_FLEX_ALIGN_SPACE_BETWEEN,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(button_row, compact ? 4 : LV_DPX(10), LV_PART_MAIN);
  lv_obj_clear_flag(button_row, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

  const auto create_button = [this, button_row, compact](const char* label_text,
                                                          lv_obj_t** out_button) {
    lv_obj_t* const button = lv_btn_create(button_row);
    if (!button) return;
    lv_obj_set_height(button, lv_pct(100));
    lv_obj_set_width(button, lv_pct(46));
    lv_obj_set_style_bg_color(button, ui_color_panel_bg(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(button, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(button, ui_color_panel_border(), LV_PART_MAIN);
    lv_obj_set_style_border_width(button, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(button, compact ? 1 : LV_DPX(5), LV_PART_MAIN);
    lv_obj_set_style_pad_all(button, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(button, ui_color_highlight_bg(),
                              LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_text_color(button, ui_color_highlight_fg(),
                                LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_add_event_cb(button, onActionConfirmationEvent, LV_EVENT_CLICKED, this);
    lv_obj_t* const label = lv_label_create(button);
    if (label) {
      lv_label_set_text_static(label, label_text);
      lv_obj_set_style_text_color(label, ui_color_overlay_fg(), LV_PART_MAIN);
      lv_obj_center(label);
    }
    *out_button = button;
  };

  create_button("Cancel", &_action_confirm_cancel);
  create_button("Confirm", &_action_confirm_accept);
  if (!_action_confirm_cancel || !_action_confirm_accept) {
    lv_obj_del(_action_confirm_root);
    _action_confirm_root = nullptr;
    _action_confirm_box = nullptr;
    _action_confirm_body = nullptr;
    _action_confirm_cancel = nullptr;
    _action_confirm_accept = nullptr;
    return false;
  }
#else
  lv_obj_t* const key_hint = lv_label_create(_action_confirm_box);
  if (key_hint) {
    lv_obj_set_width(key_hint, lv_pct(100));
    lv_label_set_long_mode(key_hint, LV_LABEL_LONG_CLIP);
    lv_label_set_text_static(key_hint, operation_hint::kDestructiveConfirm);
    lv_obj_set_style_text_align(key_hint, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_color(key_hint, ui_color_overlay_fg(), LV_PART_MAIN);
  }
#endif

  lv_obj_update_layout(_action_confirm_root);
  lv_obj_center(_action_confirm_box);
  lv_obj_add_flag(_action_confirm_root, LV_OBJ_FLAG_HIDDEN);
  return true;
}

void SystemScreen::openActionConfirmation(SysAction action) {
  if (action != SysAction::FactoryReset && action != SysAction::ClearData) return;
  closeActionConfirmation();
  if (!_action_confirm_root || !_action_confirm_body ||
      !lv_obj_is_valid(_action_confirm_root)) {
    _feedback.showAlert("Unable to open confirmation", 2000);
    return;
  }

  const char* text = action == SysAction::FactoryReset
                         ? "Erase all settings and data?"
                         : "Clear contacts and user data?";
  _pending_action = action;
  lv_label_set_text_static(_action_confirm_body, text);
  lv_obj_clear_flag(_action_confirm_root, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(_action_confirm_root);
  lv_obj_update_layout(_action_confirm_root);
  lv_obj_center(_action_confirm_box);
}

void SystemScreen::closeActionConfirmation() {
  _pending_action = SysAction::None;
  if (_action_confirm_root && lv_obj_is_valid(_action_confirm_root))
    lv_obj_add_flag(_action_confirm_root, LV_OBJ_FLAG_HIDDEN);
}

void SystemScreen::acceptActionConfirmation() {
  if (!_action_confirm_root || _pending_action == SysAction::None ||
      lv_obj_has_flag(_action_confirm_root, LV_OBJ_FLAG_HIDDEN)) return;
  const SysAction action = _pending_action;
  _pending_action = SysAction::None;
  lv_obj_add_flag(_action_confirm_root, LV_OBJ_FLAG_HIDDEN);
  executeAction(action);
}

bool SystemScreen::handleConfirmationKey(uint32_t key) {
  if (!_action_confirm_root || _pending_action == SysAction::None ||
      lv_obj_has_flag(_action_confirm_root, LV_OBJ_FLAG_HIDDEN)) return false;
  if (key == LV_KEY_ESC) {
    closeActionConfirmation();
  } else if (key == LV_KEY_ENTER) {
    // LVGL keypad input emits CLICKED on the still-focused action row when
    // this ENTER is released. Suppress that companion click or it immediately
    // opens the confirmation dialog again over the result Alert.
    _suppress_action_click_until_ms = lv_tick_get() + 500;
    acceptActionConfirmation();
  }
  return true;
}

void SystemScreen::onActionConfirmationEvent(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  auto* self = static_cast<SystemScreen*>(lv_event_get_user_data(e));
  if (!self || !self->_action_confirm_root || self->_pending_action == SysAction::None ||
      lv_obj_has_flag(self->_action_confirm_root, LV_OBJ_FLAG_HIDDEN)) return;

  lv_obj_t* const target = lv_event_get_target(e);
  if (target == self->_action_confirm_cancel) {
    self->closeActionConfirmation();
  } else if (target == self->_action_confirm_accept) {
    self->acceptActionConfirmation();
  }
  lv_event_stop_processing(e);
  lv_event_stop_bubbling(e);
}

}  // namespace heltec::meshcore::ui
