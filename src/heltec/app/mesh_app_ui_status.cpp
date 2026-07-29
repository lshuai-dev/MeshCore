#include "mesh_app_ui.hpp"

#include <cstdio>
#include <cstring>

#include "HeltecMesh.h"
#include "Utils.h"
#include "ui/core/ui_task.hpp"

namespace heltec::meshcore::biz {

const char* MeshAppUi::nodeName() const {
  return the_mesh.getNodeName();
}

void MeshAppUi::formatNodeIdLine(char* buf, size_t buf_len) const {
  if (!buf || buf_len == 0) return;
  const char* name = the_mesh.getNodeName();
  if (!name || !name[0]) name = "--------";
  snprintf(buf, buf_len, "ID: %s", name);
}

int MeshAppUi::messageCount() const {
  return heltec::meshcore::ui::ui_task().msgCount();
}

bool MeshAppUi::hasCompanionConnection() const {
  return heltec::meshcore::ui::ui_task().hasConnection();
}

uint32_t MeshAppUi::companionPairingPin() const {
  return the_mesh.getBLEPin();
}

namespace {

void formatConversationLabel(const heltec::meshcore::history::ConversationKey& key,
                             char* label, size_t label_len) {
  if (!label || label_len == 0) return;
  label[0] = '\0';
  if (key.type == heltec::meshcore::history::ConversationType::Channel) {
    ChannelDetails channel{};
    if (the_mesh.getChannel(key.channel_idx, channel) && channel.name[0] != '\0') {
      snprintf(label, label_len, "#%s", channel.name);
    } else {
      snprintf(label, label_len, "#Channel %u", static_cast<unsigned>(key.channel_idx));
    }
    return;
  }

  ContactInfo* contact = the_mesh.lookupContactByPubKey(key.peer_prefix, sizeof(key.peer_prefix));
  if (contact && contact->name[0] != '\0') {
    strncpy(label, contact->name, label_len - 1);
    label[label_len - 1] = '\0';
    return;
  }
  char hex[13]{};
  mesh::Utils::toHex(hex, key.peer_prefix, sizeof(key.peer_prefix));
  snprintf(label, label_len, "#%s", hex);
}

}  // namespace

int MeshAppUi::fillRecentConversations(int offset, RecentConversationItem* items,
                                       int max_items, int* total_items) const {
  if (total_items) *total_items = 0;
  if (!items || max_items <= 0) return 0;
  DataStore* store = the_mesh.getDataStore();
  if (!store) return 0;
  heltec::meshcore::history::ConversationSummary raw[10]{};
  const int capacity = max_items < 10 ? max_items : 10;
  const int count = store->fillRecentConversations(offset, raw, capacity, total_items);
  mesh::RTCClock* rtc = the_mesh.getRTCClock();
  const uint32_t now = rtc ? rtc->getCurrentTime() : 0;
  for (int i = 0; i < count; ++i) {
    items[i] = RecentConversationItem{};
    items[i].key = raw[i].key;
    formatConversationLabel(raw[i].key, items[i].label, sizeof(items[i].label));
    items[i].age_seconds = (raw[i].timestamp != 0 && now >= raw[i].timestamp)
                               ? static_cast<int32_t>(now - raw[i].timestamp)
                               : 0;
    items[i].unread = raw[i].unread;
  }
  return count;
}

int MeshAppUi::fillConversationMessages(const MessageConversationKey& key,
                                        int offset_from_latest,
                                        ConversationMessageItem* items,
                                        int max_items, int* total_items) const {
  if (total_items) *total_items = 0;
  if (!items || max_items <= 0) return 0;
  DataStore* store = the_mesh.getDataStore();
  if (!store) return 0;
  heltec::meshcore::history::MessageItem raw[1]{};
  const int capacity = max_items < 1 ? max_items : 1;
  const int count = store->fillConversationMessages(key, offset_from_latest, raw,
                                                     capacity, total_items);
  for (int i = 0; i < count; ++i) {
    items[i] = ConversationMessageItem{};
    items[i].sequence = raw[i].sequence;
    items[i].timestamp = raw[i].timestamp;
    items[i].outgoing = raw[i].direction == heltec::meshcore::history::MessageDirection::Outgoing;
    strncpy(items[i].text, raw[i].text, sizeof(items[i].text) - 1);
    items[i].text[sizeof(items[i].text) - 1] = '\0';
  }
  return count;
}

void MeshAppUi::markConversationRead(const MessageConversationKey& key) {
  DataStore* store = the_mesh.getDataStore();
  if (!store || !store->markConversationRead(key)) return;
  heltec::meshcore::ui::ui_task().setMessageCount(store->countUnreadMessages());
  notifyAppState(heltec::meshcore::ui::AppStateEventType::MessageHistoryChanged);
}

int MeshAppUi::deviceUnreadMessageCount() const {
  DataStore* store = the_mesh.getDataStore();
  return store ? store->countUnreadMessages() : 0;
}

}  // namespace heltec::meshcore::biz
