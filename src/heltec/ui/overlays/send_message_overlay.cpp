#include "send_message_overlay.hpp"

#if !defined(HELTEC_V4_R8_TFT) || !defined(HELTEC_HAS_TOUCH) || !HELTEC_HAS_TOUCH

#include <cstdio>
#include <cstring>
#include <lvgl.h>

#include "heltec/ui/core/biz_facade.hpp"
#include "ui/app/ui_theme.hpp"
#include "ui/core/ht_meta_data.hpp"
#include "ui/core/ui_events.h"
#include "keyboard_overlay.hpp"

#if defined(MESH_DEBUG) && MESH_DEBUG
#include <Arduino.h>
#define SENDMSG_LOG(fmt, ...) \
  do { \
    Serial.printf("[sendmsg] " fmt "\n", ##__VA_ARGS__); \
    Serial.flush(); \
  } while (0)
#else
#define SENDMSG_LOG(fmt, ...) ((void)0)
#endif

namespace heltec::meshcore::ui {

_lv_obj_t* SendMessageOverlay::createRoot(_lv_obj_t* parent) {
  return ht_obj_create(parent, meta_id::SendMessageOverlayRoot);
}

namespace {

static lv_style_t s_opaque_cover_style;
static bool s_opaque_cover_style_ready = false;

lv_coord_t componentGap() {
#if defined(HELTEC_V4_R8_TFT)
  return LV_DPX(10);
#else
  return 3;
#endif
}

lv_coord_t sendRowHeight() {
#if defined(HELTEC_DISPLAY_ST7789) && HELTEC_DISPLAY_ST7789
  return 16;
#elif (defined(HELTEC_DISPLAY_ST7735) && HELTEC_DISPLAY_ST7735) || \
    (defined(HELTEC_DISPLAY_SSD1306) && HELTEC_DISPLAY_SSD1306) || LV_COLOR_DEPTH == 1
  return 12;
#else
  return 28;
#endif
}

static void initOpaqueCoverStyle() {
  if (s_opaque_cover_style_ready) return;
  lv_style_init(&s_opaque_cover_style);
  lv_style_set_bg_opa(&s_opaque_cover_style, LV_OPA_COVER);
  lv_style_set_bg_color(&s_opaque_cover_style, ui_color_overlay_bg());
  s_opaque_cover_style_ready = true;
}

static void applyOpaqueCover(lv_obj_t* obj) {
  if (!obj) return;
  initOpaqueCoverStyle();
  lv_obj_add_style(obj, &s_opaque_cover_style, LV_PART_MAIN);
}

const char* footerForPage(bool back_to_parent) {
  return back_to_parent ? "Menu:OK Back:back" : "Menu:OK Back:cancel";
}

}  // namespace

_lv_obj_t* SendMessageOverlay::focusTarget() const {
  return _root;
}

bool SendMessageOverlay::onKey(uint32_t key) {
  if (key == LV_KEY_NEXT || key == LV_KEY_RIGHT || key == LV_KEY_DOWN ||
      key == LV_KEY_PREV || key == LV_KEY_LEFT || key == LV_KEY_UP) {
    const int count = rowCount();
    if (count <= 0) return true;
    const int dir = (key == LV_KEY_NEXT || key == LV_KEY_RIGHT || key == LV_KEY_DOWN) ? 1 : -1;
    const int before = _model.selectedIndex();
    _model.setSelectedIndex(wrapIndex(_model.selectedIndex() + dir));
    renderRows();
    (void)before;
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
#if (defined(HELTEC_DISPLAY_ST7789) && HELTEC_DISPLAY_ST7789) || \
    (defined(HELTEC_DISPLAY_ST7735) && HELTEC_DISPLAY_ST7735) || \
    (defined(HELTEC_DISPLAY_SSD1306) && HELTEC_DISPLAY_SSD1306) || LV_COLOR_DEPTH == 1
  lv_obj_set_style_pad_hor(_root, 2, LV_PART_MAIN);
  lv_obj_set_style_pad_ver(_root, 1, LV_PART_MAIN);
  lv_obj_set_style_pad_row(_root, 0, LV_PART_MAIN);
#else
  lv_obj_set_style_pad_hor(_root, 6, LV_PART_MAIN);
  lv_obj_set_style_pad_ver(_root, 4, LV_PART_MAIN);
  lv_obj_set_style_pad_row(_root, componentGap(), LV_PART_MAIN);
#endif
  lv_obj_add_event_cb(_root, [](lv_event_t* e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    if (lv_event_get_target(e) != lv_event_get_current_target(e)) return;
    auto* self = static_cast<SendMessageOverlay*>(lv_event_get_user_data(e));
    if (!self || self->_confirm_pending) return;
    self->_confirm_pending = true;
    self->applySelection();
    self->_confirm_pending = false;
    lv_event_stop_processing(e);
    lv_event_stop_bubbling(e);
  }, LV_EVENT_CLICKED, this);

  _title = ht_label_create(_root, meta_id::SendMessageTitle);
  if (!_title) return nullptr;
  lv_obj_set_width(_title, lv_pct(100));
  lv_label_set_long_mode(_title, LV_LABEL_LONG_CLIP);
  lv_label_set_text(_title, "send message");

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

  _footer = ht_label_create(_root, meta_id::SendMessageFooter);
  if (!_footer) return nullptr;
  lv_obj_set_width(_footer, lv_pct(100));
  lv_label_set_long_mode(_footer, LV_LABEL_LONG_CLIP);
  lv_label_set_text(_footer, "Menu: select  Back: cancel");

  if (!ensureRowPool()) return nullptr;

  setTarget(_model.target());
  return _root;
}

void SendMessageOverlay::syncContacts() {
  _model.syncContacts(_biz);
}

void SendMessageOverlay::setTarget(const Target& t) {
  _model.setTarget(t);
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

const char* SendMessageOverlay::rowLabel(int index) const {
  return _model.rowLabel(index);
}

bool SendMessageOverlay::ensureRowPool() {
  if (!_list_mid) return false;
  for (int i = 0; i < kMaxVisibleRows; ++i) {
    if (_row_objs[i]) continue;
    lv_obj_t* row = ht_label_create(_list_mid, meta_id::SendMessageRow);
    if (!row) return false;
    const lv_coord_t row_h = sendRowHeight();
    const lv_coord_t font_h = lv_font_get_line_height(LV_FONT_DEFAULT);
    lv_obj_set_size(row, lv_pct(100), row_h);
    lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_top(row, row_h > font_h ? (row_h - font_h) / 2 : 0,
                             LV_PART_MAIN);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(row, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
    lv_label_set_long_mode(row, LV_LABEL_LONG_DOT);
    lv_obj_add_flag(row, LV_OBJ_FLAG_HIDDEN);
    _row_objs[i] = row;
  }
  return true;
}

int SendMessageOverlay::visibleRowCount() const {
  const int count = rowCount();
  if (count <= 0 || !_root || !_list_mid || !_row_objs[0]) return 0;

  lv_obj_update_layout(_root);
  lv_coord_t list_h = lv_obj_get_content_height(_list_mid);
  if (list_h <= 0) list_h = lv_obj_get_height(_list_mid);

  lv_coord_t row_h = lv_obj_get_height(_row_objs[0]);
  if (row_h <= 0) row_h = lv_obj_get_style_height(_row_objs[0], LV_PART_MAIN);
#if (defined(HELTEC_DISPLAY_ST7789) && HELTEC_DISPLAY_ST7789)
  if (row_h <= 0 || row_h == LV_SIZE_CONTENT) row_h = 16;
#elif (defined(HELTEC_DISPLAY_ST7735) && HELTEC_DISPLAY_ST7735) || \
    (defined(HELTEC_DISPLAY_SSD1306) && HELTEC_DISPLAY_SSD1306) || \
    LV_COLOR_DEPTH == 1
  if (row_h <= 0 || row_h == LV_SIZE_CONTENT) row_h = 12;
#else
  if (row_h <= 0 || row_h == LV_SIZE_CONTENT) row_h = 28;
#endif

  int rows = static_cast<int>(list_h / row_h);
  if (rows < 1) rows = 1;
  if (rows > kMaxVisibleRows) rows = kMaxVisibleRows;
#if (defined(HELTEC_DISPLAY_ST7789) && HELTEC_DISPLAY_ST7789) || \
    (defined(HELTEC_DISPLAY_ST7735) && HELTEC_DISPLAY_ST7735) || \
    (defined(HELTEC_DISPLAY_SSD1306) && HELTEC_DISPLAY_SSD1306) || \
    LV_COLOR_DEPTH == 1
  const int compact_min_rows = count >= 3 ? 3 : count;
  if (rows < compact_min_rows) rows = compact_min_rows;
#endif
  if (rows > 1 && rows < count && (rows % 2) == 0) --rows;
  if (rows > count) rows = count;
  return rows;
}

int SendMessageOverlay::focusRow() const {
  const int rows = _visible_row_count > 0 ? _visible_row_count : 1;
  return rows / 2;
}

int SendMessageOverlay::wrapIndex(int index) const {
  return _model.wrapIndex(index);
}

void SendMessageOverlay::renderRows() {
  if (!_root || !_list_mid) return;
  const int count = rowCount();

  const char* title = "send message";
  const char* footer = footerForPage(false);
  if (_model.page() == Page::TargetCategory) {
    title = "to";
    footer = footerForPage(true);
  } else if (_model.page() == Page::TargetList) {
    title = (_model.listKind() == 1) ? "group" : "personal";
    footer = footerForPage(true);
  }
  lv_label_set_text(_title, title);
  lv_label_set_text(_footer, footer);
  lv_obj_clear_flag(_title, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(_footer, LV_OBJ_FLAG_HIDDEN);

  if (!ensureRowPool()) return;

  if (count <= 0) {
    _visible_row_count = 0;
    for (int i = 0; i < kMaxVisibleRows; ++i) {
      if (_row_objs[i]) lv_obj_add_flag(_row_objs[i], LV_OBJ_FLAG_HIDDEN);
    }
    return;
  }

  int sel = _model.selectedIndex();
  if (sel < 0) sel = 0;
  if (sel >= count) sel = count - 1;
  _model.setSelectedIndex(sel);

  _visible_row_count = static_cast<int8_t>(visibleRowCount());
  const int focus = focusRow();
  for (int slot = 0; slot < kMaxVisibleRows; ++slot) {
    lv_obj_t* row = _row_objs[slot];
    if (!row) continue;
    if (slot >= _visible_row_count) {
      lv_obj_add_flag(row, LV_OBJ_FLAG_HIDDEN);
      lv_obj_clear_state(row, LV_STATE_CHECKED | LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
      continue;
    }

    const int item = wrapIndex(sel + slot - focus);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_HIDDEN);
    ht_set_user_data(row, reinterpret_cast<void*>(static_cast<uintptr_t>(item)));
    lv_label_set_text(row, rowLabel(item));
    if (slot == focus) {
      lv_obj_add_state(row, LV_STATE_CHECKED);
    } else {
      lv_obj_clear_state(row, LV_STATE_CHECKED | LV_STATE_FOCUSED | LV_STATE_FOCUS_KEY);
    }
  }
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
  if (LV_RES_OK != lv_async_call(showSendResultAlertAsync, this)) {
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
  SENDMSG_LOG("custom keyboard emit ok=%d page=%u sel=%d target=%s kind=%u channel=%d",
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
  if (!_root || !_list_mid) return;
  AbstractOverlay::onEnter();
  applyOpaqueCover(_root);
  applyOpaqueCover(_list_mid);
  _confirm_pending = false;
  _model.reset(_biz);
  lv_obj_update_layout(_root);
  renderRows();
  lv_obj_update_layout(_root);
  lv_obj_t* parent = lv_obj_get_parent(_root);
  lv_obj_invalidate(_root);
  if (parent) lv_obj_invalidate(parent);
}

void SendMessageOverlay::onExit() {
  clearFocusObjects();
  for (int i = 0; i < kMaxVisibleRows; ++i) {
    if (_row_objs[i]) lv_obj_add_flag(_row_objs[i], LV_OBJ_FLAG_HIDDEN);
  }
  AbstractOverlay::onExit();
  hideVisual();
}

void SendMessageOverlay::hideVisual() {
  if (!_root) return;
}

}  // namespace heltec::meshcore::ui

#endif  // !HELTEC_V4_R8_TFT || !HELTEC_HAS_TOUCH
