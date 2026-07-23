#include "send_message_model.hpp"

#include <cstdio>
#include <cstring>

#include "heltec/ui/core/biz_facade.hpp"

namespace heltec::meshcore::ui {

void SendMessageModel::safeCopy(char* dest, size_t dest_size, const char* src) {
  if (!dest || dest_size == 0) return;
  if (!src) {
    dest[0] = '\0';
    return;
  }
  strncpy(dest, src, dest_size - 1);
  dest[dest_size - 1] = '\0';
}

void SendMessageModel::reset(const biz::IBizFacade& biz) {
  _page = Page::Main;
  _select = 0;
  _cat_count = 0;
  _list_count = 0;
  _list_kind = 0;
  resetTarget();
  syncContacts(biz);
  rebuildMainRows();
}

void SendMessageModel::syncContacts(const biz::IBizFacade& biz) {
  for (int i = 0; i < kMaxContacts; ++i) _contacts[i] = CachedContact{};
  _contact_count = 0;

  const int total = biz.sendMessagePersonalCount();
  for (int i = 0; i < total && _contact_count < kMaxContacts; ++i) {
    CachedContact& c = _contacts[_contact_count];
    if (!biz.sendMessagePersonalAt(i, c.pub_key_prefix, c.name, sizeof(c.name))) continue;
    c.used = true;
    ++_contact_count;
  }
}

void SendMessageModel::rebuildTargetCategories(const biz::IBizFacade& biz) {
  _cat_count = 0;
  safeCopy(_cat_rows[_cat_count++], sizeof(_cat_rows[0]), "broadcast");

  if (biz.sendMessageHasGroupChannels()) {
    safeCopy(_cat_rows[_cat_count++], sizeof(_cat_rows[0]), "group");
  }
  if (_contact_count > 0) {
    safeCopy(_cat_rows[_cat_count++], sizeof(_cat_rows[0]), "personal");
  }
}

void SendMessageModel::rebuildTargetList(const biz::IBizFacade& biz, int list_kind) {
  _list_kind = list_kind;
  _list_count = 0;
  for (int i = 0; i < kMaxListItems; ++i) {
    _list_rows[i][0] = '\0';
    _list_channel_idx[i] = -1;
  }

  if (_list_kind == 1) {
    const int n = biz.sendMessageGroupCount();
    for (int i = 0; i < n && _list_count < kMaxListItems; ++i) {
      int ch_idx = -1;
      if (!biz.sendMessageGroupAt(i, &ch_idx, _list_rows[_list_count],
                                  sizeof(_list_rows[0]))) {
        continue;
      }
      _list_channel_idx[_list_count] = ch_idx;
      ++_list_count;
    }
  } else if (_list_kind == 2) {
    for (int i = 0; i < _contact_count && _list_count < kMaxListItems; ++i) {
      safeCopy(_list_rows[_list_count], sizeof(_list_rows[0]), _contacts[i].name);
      ++_list_count;
    }
  }
}

void SendMessageModel::rebuildMainRows() {
  char target_line[28];
  snprintf(target_line, sizeof(target_line), "to: %s",
           _target.label[0] ? _target.label : "broadcast");
  safeCopy(_main_rows[0], sizeof(_main_rows[0]), target_line);
  safeCopy(_main_rows[1], sizeof(_main_rows[0]), "hi");
  safeCopy(_main_rows[2], sizeof(_main_rows[0]), "bye");
  safeCopy(_main_rows[3], sizeof(_main_rows[0]), "yes");
  safeCopy(_main_rows[4], sizeof(_main_rows[0]), "no");
  safeCopy(_main_rows[5], sizeof(_main_rows[0]), "ok");
  safeCopy(_main_rows[6], sizeof(_main_rows[0]), "custom...");
  _row_count = kMainRowCount;
}

void SendMessageModel::setPage(Page page) {
  _page = page;
  setSelectedIndex(_select);
}

void SendMessageModel::setSelectedIndex(int index) {
  const int count = rowCount();
  if (count <= 0) {
    _select = 0;
    return;
  }
  if (index < 0) index = 0;
  if (index >= count) index = count - 1;
  _select = static_cast<int8_t>(index);
}

int SendMessageModel::wrapIndex(int index) const {
  const int count = rowCount();
  if (count <= 0) return 0;
  index %= count;
  if (index < 0) index += count;
  return index;
}

int SendMessageModel::rowCount() const {
  switch (_page) {
    case Page::Main:
      return _row_count;
    case Page::TargetCategory:
      return _cat_count;
    case Page::TargetList:
      return _list_count;
  }
  return 0;
}

const char* SendMessageModel::rowLabel(int index) const {
  switch (_page) {
    case Page::Main:
      if (index >= 0 && index < _row_count) return _main_rows[index];
      break;
    case Page::TargetCategory:
      if (index >= 0 && index < _cat_count) return _cat_rows[index];
      break;
    case Page::TargetList:
      if (index >= 0 && index < _list_count) return _list_rows[index];
      break;
  }
  return "";
}

void SendMessageModel::resetTarget() {
  _target = Target{};
  _target.kind = TargetKind::Broadcast;
  safeCopy(_target.label, sizeof(_target.label), "broadcast");
}

int SendMessageModel::listChannelIndex(int index) const {
  if (index < 0 || index >= _list_count) return -1;
  return _list_channel_idx[index];
}

bool SendMessageModel::contactAt(int index, uint8_t pub_key_prefix[6], char* label,
                                 size_t label_len) const {
  if (index < 0 || index >= _contact_count || !pub_key_prefix) return false;
  const CachedContact& c = _contacts[index];
  if (!c.used) return false;
  memcpy(pub_key_prefix, c.pub_key_prefix, 6);
  if (label && label_len > 0) safeCopy(label, label_len, c.name);
  return true;
}

}  // namespace heltec::meshcore::ui
