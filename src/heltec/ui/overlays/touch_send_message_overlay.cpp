#include "touch_send_message_overlay.hpp"

#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH

#include <cstdio>
#include <cstring>
#include <lvgl.h>

#include "heltec/drivers/input/btn_debug.hpp"
#include "heltec/ui/core/biz_facade.hpp"
#include "ui/core/ht_meta_data.hpp"
#include "ui/core/operation_hints.hpp"
#include "ui/core/ui_deferred_queue.hpp"
#include "ui/core/ui_events.h"
#include "keyboard_overlay.hpp"

namespace heltec::meshcore::ui {

_lv_obj_t* SendMessageOverlay::createRoot(_lv_obj_t* parent) {
  return ht_obj_create(parent, meta_id::SendMessageOverlayRoot);
}

namespace {

lv_coord_t componentGap() {
  return LV_DPX(10);
}

lv_coord_t sendRowHeight() {
#if defined(HELTEC_DISPLAY_ST7789) && HELTEC_DISPLAY_ST7789
  return 16;
#elif (defined(HELTEC_DISPLAY_ST7735) && HELTEC_DISPLAY_ST7735) || LV_COLOR_DEPTH == 1
  return 12;
#else
  return 28;
#endif
}

void fitToDisplay(_lv_obj_t* obj) {
  if (!obj) return;
  lv_disp_t* disp = lv_disp_get_default();
  const lv_coord_t w = disp ? lv_disp_get_hor_res(disp) : lv_obj_get_width(lv_scr_act());
  const lv_coord_t h = disp ? lv_disp_get_ver_res(disp) : lv_obj_get_height(lv_scr_act());
  lv_obj_set_pos(obj, 0, 0);
  lv_obj_set_size(obj, w, h);
}

}  // namespace

void SendMessageOverlay::syncOverlayGroup() {
  if (!_list || !_focus_group) return;
  clearFocusObjects();
  const int count = rowCount();
  for (int i = 0; i < count && i < kMaxListItems; ++i) {
    if (_row_objs[i]) addFocusObject(_row_objs[i]);
  }
}

bool SendMessageOverlay::onKey(uint32_t key) {
  if (!_root) return false;
  BTN_UI_LOG("sendmsg key=0x%lX page=%u sel=%d count=%d",
             (unsigned long)key, (unsigned)static_cast<uint8_t>(_model.page()),
             selectedIndex(), rowCount());
  if (key == LV_KEY_PREV || key == LV_KEY_LEFT) {
    const int count = rowCount();
    if (count <= 0) return true;
    _model.moveSelection(_biz, -1);
    renderRows();
    return true;
  }
  if (key == LV_KEY_NEXT || key == LV_KEY_RIGHT) {
    const int count = rowCount();
    if (count <= 0) return true;
    _model.moveSelection(_biz, 1);
    renderRows();
    return true;
  }
  if (key == LV_KEY_ESC) {
    handleBack();
    return true;
  }
  return false;
}

_lv_obj_t* SendMessageOverlay::create(lv_obj_t* parent) {
  if (!AbstractOverlay::create(parent)) return nullptr;

  fitToDisplay(_root);
  lv_obj_add_flag(_root, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_pad_hor(_root, 2, LV_PART_MAIN);
  lv_obj_set_style_pad_ver(_root, 1, LV_PART_MAIN);
  lv_obj_set_style_pad_row(_root, 0, LV_PART_MAIN);

  _title = ht_label_create(_root, meta_id::SendMessageTitle);
  if (!_title) return nullptr;
  lv_obj_set_width(_title, lv_pct(100));
  lv_label_set_long_mode(_title, LV_LABEL_LONG_CLIP);
  lv_label_set_text_static(_title, "send message");

  _list_mid = ht_obj_create(_root, meta_id::SendMessageList);
  if (!_list_mid) return nullptr;
  lv_obj_set_width(_list_mid, lv_pct(100));
  lv_obj_set_flex_grow(_list_mid, 1);
  lv_obj_set_flex_flow(_list_mid, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(_list_mid, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_all(_list_mid, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_row(_list_mid, componentGap(), LV_PART_MAIN);
  lv_obj_clear_flag(_list_mid, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(_list_mid, LV_OBJ_FLAG_CLICKABLE);

  _list = ht_obj_create(_list_mid, meta_id::SendMessageTouchList);
  if (!_list) return nullptr;
  lv_obj_set_width(_list, lv_pct(100));
  lv_obj_set_flex_grow(_list, 1);
  lv_obj_set_flex_flow(_list, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(_list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_all(_list, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_row(_list, componentGap(), LV_PART_MAIN);
  lv_obj_clear_flag(_list, LV_OBJ_FLAG_SCROLLABLE);

  _footer = ht_label_create(_root, meta_id::SendMessageFooter);
  if (!_footer) return nullptr;
  lv_obj_set_width(_footer, lv_pct(100));
  lv_label_set_long_mode(_footer, LV_LABEL_LONG_CLIP);
  lv_label_set_text_static(_footer, operation_hint::kMessageSelect);

  for (uint8_t i = 0; i < kMaxListItems; ++i) {
    if (!createTouchRow(i, "")) return nullptr;
    lv_obj_add_flag(_row_objs[i], LV_OBJ_FLAG_HIDDEN);
  }

  setTarget(_model.target());
  return _root;
}

void SendMessageOverlay::syncContacts() {
  _model.syncContacts(_biz);
}

void SendMessageOverlay::setTarget(const Target& t) {
  _model.setTarget(t);
}

void SendMessageOverlay::prepareTarget(const UiSendMessageTarget* target) {
  _pending_target_valid = false;
  if (!target) return;
  Target pending{};
  if (target->kind == UiMessageTargetKind::Channel) {
    pending.kind = TargetKind::Group;
    pending.channel_idx = target->channel_idx;
  } else {
    pending.kind = TargetKind::Personal;
    memcpy(pending.pub_key_prefix, target->pub_key_prefix, sizeof(pending.pub_key_prefix));
  }
  SendMessageModel::safeCopy(pending.label, sizeof(pending.label), target->label);
  _pending_target = pending;
  _pending_target_valid = true;
}

void SendMessageOverlay::rebuildTargetCategories() {
  _model.rebuildTargetCategories(_biz);
}

void SendMessageOverlay::rebuildTargetList() {
  _model.rebuildTargetList(_biz, _model.listKind());
}

void SendMessageOverlay::rebuildMainRows() {
  _model.rebuildMainRows();
}

int SendMessageOverlay::rowCount() const {
  return _model.rowCount();
}

int SendMessageOverlay::selectedIndex() const {
  return _model.selectedIndex();
}

const char* SendMessageOverlay::rowLabel(int index) const {
  return _model.rowLabel(index);
}

void SendMessageOverlay::clearTouchList() {
  if (!_list) return;
  clearFocusObjects();
  for (int i = 0; i < kMaxListItems; ++i) {
    if (!_row_objs[i]) continue;
    lv_obj_add_flag(_row_objs[i], LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_state(_row_objs[i], LV_STATE_FOCUSED);
    if (_row_labels[i]) lv_obj_clear_state(_row_labels[i], LV_STATE_FOCUSED);
  }
}

_lv_obj_t* SendMessageOverlay::createTouchRow(uint8_t index, const char* text) {
  if (!_list) return nullptr;
  lv_obj_t* row = ht_btn_create(
      _list, meta_id::SendMessageRow,
      reinterpret_cast<void*>(static_cast<uintptr_t>(index)));
  if (!row) return nullptr;
  lv_obj_set_size(row, lv_pct(100), sendRowHeight());
  lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN);
  lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scrollbar_mode(row, LV_SCROLLBAR_MODE_OFF);
  lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_t* label = ht_label_create(row, meta_id::SendMessageRowLabel, text ? text : "");
  if (!label) return nullptr;
  lv_obj_set_width(label, lv_pct(100));
  lv_obj_set_flex_grow(label, 1);
  lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);

  lv_obj_add_event_cb(row, [](lv_event_t* e) {
    if (LV_EVENT_FOCUSED != lv_event_get_code(e)) return;
    auto* self = static_cast<SendMessageOverlay*>(lv_event_get_user_data(e));
    if (!self) return;
    if (self->_syncing_focus) return;
    self->_model.setSelectedIndex(static_cast<int>(reinterpret_cast<uintptr_t>(
        ht_user_data(lv_event_get_target(e)))));
    self->syncSelectionVisual();
  }, LV_EVENT_FOCUSED, this);

  lv_obj_add_event_cb(row, [](lv_event_t* e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    auto* self = static_cast<SendMessageOverlay*>(lv_event_get_user_data(e));
    if (!self) return;
    self->_model.setSelectedIndex(static_cast<int>(reinterpret_cast<uintptr_t>(
        ht_user_data(lv_event_get_target(e)))));
    lv_event_stop_bubbling(e);
    lv_event_stop_processing(e);
    self->applySelection();
    if (self->_root && !lv_obj_has_flag(self->_root, LV_OBJ_FLAG_HIDDEN)) {
      self->syncOverlayGroup();
      self->syncSelectionVisual();
    }
  }, LV_EVENT_CLICKED, this);

  if (index < kMaxListItems) {
    _row_objs[index] = row;
    _row_labels[index] = label;
    SendMessageModel::safeCopy(_row_text[index], sizeof(_row_text[index]), text);
    lv_label_set_text_static(label, _row_text[index]);
  }
  return row;
}

void SendMessageOverlay::syncSelectionVisual() {
  if (!_list) return;
  const int count = rowCount();
  if (count <= 0) return;
  _model.setSelectedIndex(_model.selectedIndex());
  const int selected = _model.selectedIndex();
  for (int i = 0; i < count && i < kMaxListItems; ++i) {
    lv_obj_t* row = _row_objs[i];
    if (!row) continue;
    lv_obj_t* label = lv_obj_get_child_cnt(row) > 0 ? lv_obj_get_child(row, 0) : nullptr;
    if (i == selected) {
      lv_obj_add_state(row, LV_STATE_FOCUSED);
      if (label) lv_obj_add_state(label, LV_STATE_FOCUSED);
    } else {
      lv_obj_clear_state(row, LV_STATE_FOCUSED);
      if (label) lv_obj_clear_state(label, LV_STATE_FOCUSED);
    }
  }
  if (_focus_group && selected < kMaxListItems && _row_objs[selected]) {
    _syncing_focus = true;
    lv_group_focus_obj(_row_objs[selected]);
    _syncing_focus = false;
  }
}

void SendMessageOverlay::renderTouchList() {
  if (!_list) return;
  clearTouchList();

  const int count = rowCount();
  if (count <= 0) return;
  _model.setSelectedIndex(_model.selectedIndex());

  for (int i = 0; i < count && i < kMaxListItems; ++i) {
    if (!_row_objs[i] || !_row_labels[i]) continue;
    SendMessageModel::safeCopy(_row_text[i], sizeof(_row_text[i]), rowLabel(i));
    lv_label_set_text_static(_row_labels[i], _row_text[i]);
    lv_obj_clear_flag(_row_objs[i], LV_OBJ_FLAG_HIDDEN);
  }
  syncOverlayGroup();
  syncSelectionVisual();
}

void SendMessageOverlay::renderRows() {
  if (!_root) return;

  const char* title = "send message";
  const char* footer = operation_hint::kMessageSelect;
  if (_model.page() == Page::TargetCategory) {
    title = "to";
  } else if (_model.page() == Page::TargetList) {
    title = (_model.listKind() == 1) ? "group" : "personal";
  }
  lv_label_set_text_static(_title, title);
  lv_label_set_text_static(_footer, footer);

  renderTouchList();
}

void SendMessageOverlay::sendMessageText(const char* text, bool alert_on_direct_failure) {
  if (!text || text[0] == '\0') return;

  const Target& target = _model.target();
  const TargetKind kind = target.kind;
  const int channel_idx = target.channel_idx;
  uint8_t pub_key_prefix[6];
  memcpy(pub_key_prefix, target.pub_key_prefix, 6);

  // Close before alert/send so the result overlay becomes the top surface.
  emitEvent(UiEventType::SendMessageClose);

  bool ok = false;
  if (kind == TargetKind::Personal) {
    ok = _biz.sendDirectMessage(pub_key_prefix, text);
  } else if (kind == TargetKind::Group) {
    ok = _biz.sendGroupMessage(channel_idx, text);
  } else {
    const int len = static_cast<int>(strlen(text));
    ok = _biz.sendBroadcast(text, len);
  }
  if (ok || alert_on_direct_failure || kind == TargetKind::Broadcast) {
    scheduleSendResultAlert(ok);
  }
}

void SendMessageOverlay::scheduleSendResultAlert(bool ok) {
  _send_alert_ok = ok;
  if (_send_alert_scheduled) return;
  _send_alert_scheduled = true;
  if (!ui_defer(showSendResultAlertAsync, this)) {
    _send_alert_scheduled = false;
    _biz.showAlert(ok ? "Queued" : "Failed", 1600);
  }
}

void SendMessageOverlay::showSendResultAlertAsync(void* user_data) {
  auto* self = static_cast<SendMessageOverlay*>(user_data);
  if (!self) return;
  self->_send_alert_scheduled = false;
  self->_biz.showAlert(self->_send_alert_ok ? "Queued" : "Failed", 1600);
}

void SendMessageOverlay::submitCustomMessage(const char* text) {
  sendMessageText(text, true);
}

void SendMessageOverlay::openCustomKeyboard() {
  const Target& target = _model.target();
  UiMessageKeyboardRequest req;
  req.title = target.label;
  const bool ok = emitEvent(UiEventType::MessageKeyboardOpen, &req);
  BTN_UI_LOG("sendmsg custom keyboard emit ok=%d page=%u sel=%d target=%s kind=%u channel=%d",
             ok ? 1 : 0,
             (unsigned)static_cast<uint8_t>(_model.page()),
             _model.selectedIndex(),
             req.title ? req.title : "",
             (unsigned)static_cast<uint8_t>(target.kind),
             target.channel_idx);
}

void SendMessageOverlay::handleBack() {
  if (_model.page() == Page::TargetList) {
    _model.setPage(Page::TargetCategory);
    _model.setSelectedIndex(0);
    renderRows();
    return;
  }
  if (_model.page() == Page::TargetCategory) {
    _model.setPage(Page::Main);
    rebuildMainRows();
    _model.setSelectedIndex(0);
    renderRows();
    return;
  }
  emitEvent(UiEventType::SendMessageClose);
}

void SendMessageOverlay::applySelection() {
  const int count = rowCount();
  if (count <= 0) return;
  const int idx = _model.selectedIndex();
  if (idx < 0 || idx >= count) return;

  if (_model.page() == Page::Main) {
    if (idx == 0) {
      _model.setPage(Page::TargetCategory);
      rebuildTargetCategories();
      _model.setSelectedIndex(0);
      renderRows();
      return;
    }
    if (idx >= 1 && idx <= 5) {
      sendMessageText(rowLabel(idx));
      return;
    }
    if (idx == 6) {
      openCustomKeyboard();
      return;
    }
  }

  if (_model.page() == Page::TargetCategory) {
    const char* label = rowLabel(idx);
    if (!label) return;
    if (strcmp(label, "broadcast") == 0) {
      Target t{};
      t.kind = TargetKind::Broadcast;
      t.channel_idx = -1;
      SendMessageModel::safeCopy(t.label, sizeof(t.label), "broadcast");
      setTarget(t);
      _model.setPage(Page::Main);
      rebuildMainRows();
      _model.setSelectedIndex(0);
      renderRows();
      return;
    }
    if (strcmp(label, "group") == 0) {
      _model.setListKind(1);
      rebuildTargetList();
      if (_model.listCount() <= 0) {
        _biz.showAlert("No group channels", 900);
        return;
      }
      _model.setPage(Page::TargetList);
      _model.setSelectedIndex(0);
      renderRows();
      return;
    }
    if (strcmp(label, "personal") == 0) {
      _model.setListKind(2);
      rebuildTargetList();
      if (_model.listCount() <= 0) {
        _biz.showAlert("No contacts", 900);
        return;
      }
      _model.setPage(Page::TargetList);
      _model.setSelectedIndex(0);
      renderRows();
      return;
    }
  }

  if (_model.page() == Page::TargetList) {
    Target t{};
    if (_model.listKind() == 1) {
      if (idx >= 0 && idx < _model.listCount()) {
        t.kind = TargetKind::Group;
        t.channel_idx = _model.listChannelIndex(idx);
        SendMessageModel::safeCopy(t.label, sizeof(t.label), rowLabel(idx));
      }
    } else if (_model.listKind() == 2) {
      if (idx >= 0 && idx < _model.listCount()) {
        t.kind = TargetKind::Personal;
        (void)_model.contactAt(idx, t.pub_key_prefix, t.label, sizeof(t.label));
      }
    }
    setTarget(t);
    _model.setPage(Page::Main);
    rebuildMainRows();
    _model.setSelectedIndex(0);
    renderRows();
  }
}

void SendMessageOverlay::onEnter() {
  if (!_root) return;
  if (!_list) return;
  AbstractOverlay::onEnter();
  fitToDisplay(lv_obj_get_parent(_root));
  fitToDisplay(_root);
  _model.reset(_biz);
  if (_pending_target_valid) {
    setTarget(_pending_target);
    rebuildMainRows();
    _pending_target_valid = false;
  }
  renderRows();
  lv_obj_update_layout(_root);
  syncOverlayGroup();
  syncSelectionVisual();
}

void SendMessageOverlay::onExit() {
  AbstractOverlay::onExit();
  hideVisual();
}

void SendMessageOverlay::hideVisual() {
  if (!_root) return;
  clearTouchList();
}

}  // namespace heltec::meshcore::ui

#endif  // HELTEC_V4_R8_TFT && HELTEC_HAS_TOUCH
