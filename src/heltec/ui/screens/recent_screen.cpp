#include "recent_screen.hpp"

#include <cstring>
#include <lvgl.h>

#include "ui/core/ht_meta_data.hpp"
#include "ui/core/operation_hints.hpp"
#include "ui/core/ui_events.h"

namespace heltec::meshcore::ui {

_lv_obj_t* RecentScreen::createRoot(_lv_obj_t* parent) {
  return ht_obj_create(parent, meta_id::RecentScreenRoot);
}

namespace {

void formatAge(int32_t secs, char* buf, size_t buf_size) {
  if (secs < 60) {
    lv_snprintf(buf, buf_size, "%ds", static_cast<int>(secs));
  } else if (secs < 3600) {
    lv_snprintf(buf, buf_size, "%dm", static_cast<int>(secs / 60));
  } else if (secs < 86400) {
    lv_snprintf(buf, buf_size, "%dh", static_cast<int>(secs / 3600));
  } else {
    lv_snprintf(buf, buf_size, "%dd", static_cast<int>(secs / 86400));
  }
}

bool isForwardKey(uint32_t key) {
  return key == LV_KEY_DOWN || key == LV_KEY_NEXT || key == LV_KEY_RIGHT;
}

bool isBackwardKey(uint32_t key) {
  return key == LV_KEY_UP || key == LV_KEY_PREV || key == LV_KEY_LEFT;
}

}  // namespace

_lv_obj_t* RecentScreen::create(_lv_obj_t* parent) {
  if (!AbstractScreen::create(parent)) return nullptr;
  _scroll = _root;
  lv_obj_add_flag(_scroll, LV_OBJ_FLAG_EVENT_BUBBLE | LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(_scroll, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(_scroll, LV_SCROLLBAR_MODE_OFF);

  for (int i = 0; i < kMaxRows; ++i) {
    _rows[i] = ht_label_create(_scroll, meta_id::RecentRowLabel, "");
    if (!_rows[i]) continue;
    lv_label_set_text_static(_rows[i], _row_text[i]);
    lv_obj_set_width(_rows[i], lv_pct(100));
    lv_obj_clear_flag(_rows[i], LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(_rows[i], onRowClicked, LV_EVENT_CLICKED, this);
    addFocusItem(_rows[i]);
  }

  _detail_contact =
      ht_label_create(_scroll, meta_id::RecentDetailContact, "");
  _detail_message =
      ht_label_create(_scroll, meta_id::RecentDetailMessage, "");
  if (!_detail_contact || !_detail_message) return nullptr;
  lv_obj_set_width(_detail_contact, lv_pct(100));
  lv_label_set_long_mode(_detail_contact, LV_LABEL_LONG_DOT);
  lv_obj_set_width(_detail_message, lv_pct(100));
  lv_obj_set_flex_grow(_detail_message, 1);
  lv_label_set_long_mode(_detail_message, LV_LABEL_LONG_WRAP);
  lv_obj_clear_flag(_detail_contact,
                    LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ON_FOCUS);
  lv_obj_clear_flag(_detail_message,
                    LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ON_FOCUS);
  showDetailLabels(false);

  _send_button = ht_btn_create(_scroll, meta_id::RecentSendButton);
  if (!_send_button) return nullptr;
  lv_obj_set_size(_send_button, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_style_pad_ver(_send_button, 2, LV_PART_MAIN);
  _lv_obj_t* const send_button_label =
      ht_label_create(
          _send_button, meta_id::RecentSendButtonLabel,
          operation_hint::kRecentDetail
      );
  if (!send_button_label) return nullptr;
  lv_obj_set_width(send_button_label, lv_pct(100));
  lv_label_set_long_mode(send_button_label, LV_LABEL_LONG_CLIP);
  lv_obj_set_style_text_align(send_button_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_center(send_button_label);
  lv_obj_add_event_cb(_send_button, onSendButtonClicked, LV_EVENT_CLICKED, this);
  addFocusItem(_send_button);
  lv_obj_clear_flag(_send_button, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
  showSendButton(false);

  lv_group_set_wrap(group(), false);
  return _scroll;
}

void RecentScreen::onEnter() {
  _entered = true;
  refreshView();
  AbstractScreen::onEnter();
  if (_view == View::Conversations) {
    focusRow(_conversation_selected - _conversation_window_start);
  } else {
    lv_group_focus_obj(_send_button);
  }
}

void RecentScreen::onExit() {
  _entered = false;
  AbstractScreen::onExit();
}

bool RecentScreen::handleHorizontalSwipe(int8_t dir) {
  if (!_entered || _view != View::ConversationDetail || dir == 0) return false;
  // Touch direction is positive for a right-to-left swipe. Both directions
  // wrap so the message pages form one continuous circular sequence.
  moveDetailMessage(dir < 0, true);
  return true;
}

void RecentScreen::showRow(int row, const char* text, bool wrap, bool focusable) {
  if (row < 0 || row >= kMaxRows || !_rows[row]) return;
  if (text != _row_text[row]) {
    lv_snprintf(_row_text[row], sizeof(_row_text[row]), "%s", text ? text : "");
  }
  lv_label_set_text_static(_rows[row], _row_text[row]);
  lv_label_set_long_mode(_rows[row], wrap ? LV_LABEL_LONG_WRAP : LV_LABEL_LONG_DOT);
  if (wrap) {
    lv_obj_set_height(_rows[row], LV_SIZE_CONTENT);
  }
  if (focusable) {
    lv_obj_clear_state(_rows[row], LV_STATE_DISABLED);
  } else {
    lv_obj_add_state(_rows[row], LV_STATE_DISABLED);
  }
  lv_obj_clear_flag(_rows[row], LV_OBJ_FLAG_HIDDEN);
}

void RecentScreen::hideRow(int row) {
  if (row < 0 || row >= kMaxRows || !_rows[row]) return;
  _row_text[row][0] = '\0';
  lv_label_set_text_static(_rows[row], _row_text[row]);
  lv_obj_add_state(_rows[row], LV_STATE_DISABLED);
  lv_obj_add_flag(_rows[row], LV_OBJ_FLAG_HIDDEN);
}

void RecentScreen::focusRow(int row) {
  if (row < 0 || row >= kMaxRows || !_rows[row] ||
      lv_obj_has_flag(_rows[row], LV_OBJ_FLAG_HIDDEN) ||
      lv_obj_has_state(_rows[row], LV_STATE_DISABLED)) {
    return;
  }
  lv_group_focus_obj(_rows[row]);
  lv_obj_scroll_to_view(_rows[row], LV_ANIM_OFF);
}

void RecentScreen::refreshConversations() {
  lv_obj_add_flag(_scroll, LV_OBJ_FLAG_SCROLLABLE);
  showDetailLabels(false);
  showSendButton(false);
  _conversation_count = _biz.fillRecentConversations(
      _conversation_window_start, _conversation_items, kConversationRows,
      &_conversation_total);

  if (_conversation_total <= 0) {
    _conversation_total = 0;
    _conversation_selected = 0;
    _conversation_window_start = 0;
    showRow(0, "(no messages)", false, true);
    for (int i = 1; i < kMaxRows; ++i) hideRow(i);
    return;
  }

  if (_conversation_selected >= _conversation_total) {
    _conversation_selected = _conversation_total - 1;
  }
  const int max_start = _conversation_total > kConversationRows
                            ? _conversation_total - kConversationRows
                            : 0;
  if (_conversation_window_start > max_start) _conversation_window_start = max_start;
  if (_conversation_selected < _conversation_window_start ||
      _conversation_selected >= _conversation_window_start + _conversation_count) {
    _conversation_window_start = (_conversation_selected / kConversationWindowStep) *
                                 kConversationWindowStep;
    if (_conversation_window_start > max_start) _conversation_window_start = max_start;
    _conversation_count = _biz.fillRecentConversations(
        _conversation_window_start, _conversation_items, kConversationRows,
        &_conversation_total);
  }

  for (int i = 0; i < kMaxRows; ++i) {
    if (i >= _conversation_count) {
      hideRow(i);
      continue;
    }
    char age[12]{};
    formatAge(_conversation_items[i].age_seconds, age, sizeof(age));
    if (_conversation_items[i].unread > 0) {
      lv_snprintf(_row_text[i], sizeof(_row_text[i]), "%s [%u]  %s",
                  _conversation_items[i].label,
                  static_cast<unsigned>(_conversation_items[i].unread), age);
    } else {
      lv_snprintf(_row_text[i], sizeof(_row_text[i]), "%s  %s",
                  _conversation_items[i].label, age);
    }
    showRow(i, _row_text[i], false, true);
  }
}

void RecentScreen::refreshConversationDetail() {
  lv_obj_scroll_to_y(_scroll, 0, LV_ANIM_OFF);
  lv_obj_clear_flag(_scroll, LV_OBJ_FLAG_SCROLLABLE);
  for (int i = 0; i < kMaxRows; ++i) hideRow(i);

  biz::IBizFacade::ConversationMessageItem detail{};
  int total = 0;
  int count = _biz.fillConversationMessages(_active_key, _message_offset,
                                             &detail, 1, &total);
  if (total > 0 && _message_offset >= total) {
    _message_offset = total - 1;
    count = _biz.fillConversationMessages(_active_key, _message_offset,
                                           &detail, 1, &total);
  }
  _message_total = total;

  if (count == 1) {
    _detail_sequence = detail.sequence;
    _detail_outgoing = detail.outgoing;
    if (_detail_outgoing) {
      if (_active_key.type == heltec::meshcore::history::ConversationType::Direct) {
        lv_snprintf(_detail_message_text, sizeof(_detail_message_text), "to[%s]:%s",
                    _active_label[0] ? _active_label : "unknown", detail.text);
      } else {
        lv_snprintf(_detail_message_text, sizeof(_detail_message_text), "to:%s",
                    detail.text);
      }
    } else if (_active_key.type ==
               heltec::meshcore::history::ConversationType::Channel) {
      // Received channel text is stored as "sender: message". Keep the sender
      // visible while normalizing it to the same prefix style as direct chat.
      const char* separator = strchr(detail.text, ':');
      if (separator && separator != detail.text) {
        const char* message = separator + 1;
        if (*message == ' ') ++message;
        lv_snprintf(_detail_message_text, sizeof(_detail_message_text),
                    "from[%.*s]:%s", static_cast<int>(separator - detail.text),
                    detail.text, message);
      } else {
        lv_snprintf(_detail_message_text, sizeof(_detail_message_text), "from:%s",
                    detail.text);
      }
    } else {
      lv_snprintf(_detail_message_text, sizeof(_detail_message_text), "from[%s]:%s",
                  _active_label[0] ? _active_label : "unknown", detail.text);
    }
    const int page = _message_total - _message_offset;
    lv_snprintf(_detail_header_text, sizeof(_detail_header_text), "%s  %d/%d",
                _active_label[0] ? _active_label : "conversation", page,
                _message_total);
  } else {
    _detail_sequence = 0;
    _detail_outgoing = false;
    lv_snprintf(_detail_header_text, sizeof(_detail_header_text), "%s",
                _active_label[0] ? _active_label : "conversation");
    lv_snprintf(_detail_message_text, sizeof(_detail_message_text),
                "(no messages)");
  }

  lv_label_set_text_static(_detail_contact, _detail_header_text);
  lv_label_set_text_static(_detail_message, _detail_message_text);
  lv_obj_set_style_text_align(_detail_message, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
  showDetailLabels(true);
  showSendButton(true);
}

void RecentScreen::refreshView() {
  if (_view == View::Conversations) {
    refreshConversations();
  } else {
    refreshConversationDetail();
  }
}

void RecentScreen::moveConversationSelection(int delta) {
  if (_conversation_total <= 0 || delta == 0) return;
  int next = _conversation_selected + (delta > 0 ? 1 : -1);
  if (next < 0) next = 0;
  if (next >= _conversation_total) next = _conversation_total - 1;
  if (next == _conversation_selected) return;
  _conversation_selected = next;

  const int max_start = _conversation_total > kConversationRows
                            ? _conversation_total - kConversationRows
                            : 0;
  const int local = _conversation_selected - _conversation_window_start;
  if (local <= 1 && _conversation_window_start > 0) {
    _conversation_window_start -= kConversationWindowStep;
    if (_conversation_window_start < 0) _conversation_window_start = 0;
  } else if (local >= kConversationRows - 2 && _conversation_window_start < max_start) {
    _conversation_window_start += kConversationWindowStep;
    if (_conversation_window_start > max_start) _conversation_window_start = max_start;
  }
  refreshConversations();
  focusRow(_conversation_selected - _conversation_window_start);
}

void RecentScreen::moveDetailMessage(bool older, bool wrap_at_boundary) {
  if (_message_total <= 1) return;
  if (wrap_at_boundary) {
    _message_offset = older
                          ? (_message_offset + 1) % _message_total
                          : (_message_offset + _message_total - 1) % _message_total;
    refreshConversationDetail();
    lv_group_focus_obj(_send_button);
    return;
  }
  if (older) {
    if (_message_offset + 1 < _message_total) {
      ++_message_offset;
    } else {
      return;
    }
  } else {
    if (_message_offset > 0) {
      --_message_offset;
    } else {
      return;
    }
  }
  refreshConversationDetail();
  lv_group_focus_obj(_send_button);
}

void RecentScreen::openSelectedConversation() {
  if (_conversation_total <= 0) return;
  const int local = _conversation_selected - _conversation_window_start;
  if (local < 0 || local >= _conversation_count) return;
  _active_key = _conversation_items[local].key;
  lv_snprintf(_active_label, sizeof(_active_label), "%s", _conversation_items[local].label);
  _view = View::ConversationDetail;
  _message_offset = 0;
  _message_total = 0;
  _detail_sequence = 0;
  _biz.markConversationRead(_active_key);
  refreshConversationDetail();
  lv_group_focus_obj(_send_button);
}

void RecentScreen::returnToConversations() {
  _view = View::Conversations;
  refreshConversations();
  focusRow(_conversation_selected - _conversation_window_start);
}

void RecentScreen::openReply() {
  UiSendMessageTarget target{};
  if (_active_key.type == heltec::meshcore::history::ConversationType::Channel) {
    target.kind = UiMessageTargetKind::Channel;
    target.channel_idx = _active_key.channel_idx;
  } else {
    target.kind = UiMessageTargetKind::Direct;
    memcpy(target.pub_key_prefix, _active_key.peer_prefix, sizeof(target.pub_key_prefix));
  }
  lv_snprintf(target.label, sizeof(target.label), "%s", _active_label);
  (void)emitEvent(UiEventType::SendMessageOpen, &target);
}

void RecentScreen::showDetailLabels(bool visible) {
  if (!_detail_contact || !_detail_message) return;
  if (visible) {
    lv_label_set_long_mode(_detail_message, LV_LABEL_LONG_WRAP);
    lv_obj_clear_flag(_detail_contact, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(_detail_message, LV_OBJ_FLAG_HIDDEN);
  } else {
    // Stop the marquee while the contact list is visible; hidden LVGL label
    // animations would otherwise continue waking the UI task.
    lv_label_set_long_mode(_detail_message, LV_LABEL_LONG_CLIP);
    lv_obj_add_flag(_detail_contact, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(_detail_message, LV_OBJ_FLAG_HIDDEN);
  }
}

void RecentScreen::showSendButton(bool visible) {
  if (!_send_button) return;
  if (visible) {
    lv_obj_clear_state(_send_button, LV_STATE_DISABLED);
    lv_obj_clear_flag(_send_button, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_state(_send_button, LV_STATE_DISABLED);
    lv_obj_add_flag(_send_button, LV_OBJ_FLAG_HIDDEN);
  }
}

bool RecentScreen::onKey(uint32_t key) {
  if (isForwardKey(key) || isBackwardKey(key)) {
    if (_view == View::Conversations) {
      const int delta = isForwardKey(key) ? 1 : -1;
      moveConversationSelection(delta);
    } else {
      // Older messages are toward PREV/UP/LEFT. On one-key hardware NEXT is
      // translated to UP while this button is focused (see FocusKeyMapper).
      const bool older = isBackwardKey(key);
      moveDetailMessage(older, older);
    }
    return true;
  }
  if (key == LV_KEY_ESC && _view == View::ConversationDetail) {
    returnToConversations();
    return true;
  }
  return AbstractScreen::onKey(key);
}

void RecentScreen::onRowClicked(lv_event_t* event) {
  auto* self = static_cast<RecentScreen*>(lv_event_get_user_data(event));
  lv_obj_t* target = lv_event_get_target(event);
  if (!self || !target) return;
  int row = -1;
  for (int i = 0; i < kMaxRows; ++i) {
    if (self->_rows[i] == target) {
      row = i;
      break;
    }
  }
  if (row < 0 || lv_obj_has_state(target, LV_STATE_DISABLED)) return;
  lv_event_stop_processing(event);
  lv_event_stop_bubbling(event);
  if (self->_view == View::Conversations) {
    if (row >= self->_conversation_count) return;
    self->_conversation_selected = self->_conversation_window_start + row;
    self->openSelectedConversation();
  }
}

void RecentScreen::onSendButtonClicked(lv_event_t* event) {
  auto* self = static_cast<RecentScreen*>(lv_event_get_user_data(event));
  if (!self || self->_view != View::ConversationDetail) return;
  lv_event_stop_processing(event);
  lv_event_stop_bubbling(event);
  self->openReply();
}

void RecentScreen::onAppStateChanged(const AppStateEvent& event) {
  if (event.type != AppStateEventType::MessageHistoryChanged) return;
  if (_view == View::Conversations) {
    refreshConversations();
    return;
  }

  // If the user is reading an older message, keep that record anchored when
  // new messages arrive instead of jumping the page toward the latest one.
  const int old_total = _message_total;
  const uint32_t old_sequence = _detail_sequence;
  const bool was_latest = _message_offset == 0;
  biz::IBizFacade::ConversationMessageItem probe{};
  int new_total = 0;
  (void)_biz.fillConversationMessages(_active_key, _message_offset, &probe, 1,
                                      &new_total);
  if (!was_latest && new_total > old_total) {
    _message_offset += new_total - old_total;
  } else if (!was_latest && new_total == old_total && old_sequence != 0 &&
             probe.sequence > old_sequence && _message_offset + 1 < new_total) {
    // The fixed-size ring may evict one old record while its total remains
    // unchanged. Compensate for the new record shifting this offset.
    ++_message_offset;
  }
  _message_total = new_total;
  if (_entered) _biz.markConversationRead(_active_key);
  refreshConversationDetail();
}

void RecentScreen::onRefreshRequested() {
  refreshView();
}

}  // namespace heltec::meshcore::ui
