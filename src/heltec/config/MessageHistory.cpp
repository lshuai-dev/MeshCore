#include "DataStore.h"

#include <Arduino.h>
#include <cstring>

namespace {

using heltec::meshcore::history::ConversationKey;
using heltec::meshcore::history::ConversationSummary;
using heltec::meshcore::history::ConversationType;
using heltec::meshcore::history::MessageDirection;
using heltec::meshcore::history::MessageItem;

constexpr char kMessagesPath[] = "/messages1";
constexpr char kReadStatePath[] = "/msg_read1";
constexpr uint32_t kMessageMagic = 0x3147534dU;   // MSG1
constexpr uint32_t kReadStateMagic = 0x3152444dU; // MDR1

#pragma pack(push, 1)
struct MessageRecordDisk {
  uint32_t magic;
  uint32_t sequence;
  uint32_t timestamp;
  uint8_t type;
  uint8_t direction;
  uint8_t channel_idx;
  uint8_t text_len;
  uint8_t peer_prefix[6];
  uint8_t reserved[8];
  char text[heltec::meshcore::history::kMessageTextMax];
  uint16_t crc;
};

struct ReadStateDisk {
  uint32_t magic;
  uint32_t last_read_sequence;
  uint8_t type;
  uint8_t channel_idx;
  uint8_t peer_prefix[6];
  uint16_t reserved;
  uint16_t crc;
};
#pragma pack(pop)

static_assert(sizeof(MessageRecordDisk) == 192, "message history record must stay fixed");
static_assert(sizeof(ReadStateDisk) == 20, "message read state must stay fixed");

uint16_t crc16(const uint8_t* data, size_t len) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < len; ++i) {
    crc ^= static_cast<uint16_t>(data[i]) << 8;
    for (uint8_t bit = 0; bit < 8; ++bit) {
      crc = (crc & 0x8000U) ? static_cast<uint16_t>((crc << 1) ^ 0x1021U)
                            : static_cast<uint16_t>(crc << 1);
    }
  }
  return crc;
}

template <typename T>
uint16_t recordCrc(const T& record) {
  return crc16(reinterpret_cast<const uint8_t*>(&record), sizeof(T) - sizeof(record.crc));
}

bool validMessageRecord(const MessageRecordDisk& record) {
  return record.magic == kMessageMagic && record.sequence != 0 &&
         record.type <= static_cast<uint8_t>(ConversationType::Channel) &&
         record.direction <= static_cast<uint8_t>(MessageDirection::Outgoing) &&
         record.text_len <= heltec::meshcore::history::kMessageTextMax &&
         record.crc == recordCrc(record);
}

bool validReadState(const ReadStateDisk& record) {
  return record.magic == kReadStateMagic &&
         record.type <= static_cast<uint8_t>(ConversationType::Channel) &&
         record.crc == recordCrc(record);
}

ConversationKey keyFrom(const MessageRecordDisk& record) {
  ConversationKey key{};
  key.type = static_cast<ConversationType>(record.type);
  key.channel_idx = record.channel_idx;
  memcpy(key.peer_prefix, record.peer_prefix, sizeof(key.peer_prefix));
  return key;
}

ConversationKey keyFrom(const ReadStateDisk& record) {
  ConversationKey key{};
  key.type = static_cast<ConversationType>(record.type);
  key.channel_idx = record.channel_idx;
  memcpy(key.peer_prefix, record.peer_prefix, sizeof(key.peer_prefix));
  return key;
}

void setKey(MessageRecordDisk& record, const ConversationKey& key) {
  record.type = static_cast<uint8_t>(key.type);
  record.channel_idx = key.channel_idx;
  memcpy(record.peer_prefix, key.peer_prefix, sizeof(record.peer_prefix));
}

void setKey(ReadStateDisk& record, const ConversationKey& key) {
  record.type = static_cast<uint8_t>(key.type);
  record.channel_idx = key.channel_idx;
  memcpy(record.peer_prefix, key.peer_prefix, sizeof(record.peer_prefix));
}

File openUpdate(FILESYSTEM* fs, const char* path) {
#if defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
  return fs->open(path, FILE_O_WRITE);
#elif defined(RP2040_PLATFORM)
  return fs->open(path, "r+");
#else
  return fs->open(path, "r+", false);
#endif
}

File openReadOnly(FILESYSTEM* fs, const char* path) {
#if defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
  return fs->open(path, FILE_O_READ);
#elif defined(RP2040_PLATFORM)
  return fs->open(path, "r");
#else
  return fs->open(path, "r", false);
#endif
}

File createFile(FILESYSTEM* fs, const char* path) {
  fs->remove(path);
#if defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
  return fs->open(path, FILE_O_WRITE);
#elif defined(RP2040_PLATFORM)
  return fs->open(path, "w");
#else
  return fs->open(path, "w", true);
#endif
}

template <typename T>
bool ensureFixedFile(FILESYSTEM* fs, const char* path, uint16_t count) {
  if (!fs) return false;
  if (fs->exists(path)) {
    File existing = openReadOnly(fs, path);
    if (existing) {
      const size_t expected = sizeof(T) * count;
      const bool correct = existing.size() == expected;
      existing.close();
      if (correct) return true;
    }
  }

  File file = createFile(fs, path);
  if (!file) return false;
  T zero{};
  bool ok = true;
  for (uint16_t i = 0; i < count; ++i) {
    if (file.write(reinterpret_cast<const uint8_t*>(&zero), sizeof(zero)) != sizeof(zero)) {
      ok = false;
      break;
    }
  }
  file.close();
  if (!ok) fs->remove(path);
  return ok;
}

bool readMessageAt(File& file, uint16_t slot, MessageRecordDisk& record) {
  if (!file.seek(static_cast<uint32_t>(slot) * sizeof(record))) return false;
  return file.read(reinterpret_cast<uint8_t*>(&record), sizeof(record)) == sizeof(record);
}

}  // namespace

bool DataStore::ensureMessageHistory() {
  if (_messageHistoryReady) return true;
  FILESYSTEM* fs = _getContactsChannelsFS();
  if (!ensureFixedFile<MessageRecordDisk>(fs, kMessagesPath,
                                           heltec::meshcore::history::kMessageCapacity) ||
      !ensureFixedFile<ReadStateDisk>(fs, kReadStatePath,
                                      heltec::meshcore::history::kMessageCapacity)) {
    return false;
  }

  _messageMaxSequence = 0;
  File file = openReadOnly(fs, kMessagesPath);
  if (!file) return false;
  MessageRecordDisk record{};
  while (file.read(reinterpret_cast<uint8_t*>(&record), sizeof(record)) == sizeof(record)) {
    if (validMessageRecord(record) && record.sequence > _messageMaxSequence) {
      _messageMaxSequence = record.sequence;
    }
  }
  file.close();
  _messageHistoryReady = true;
  return true;
}

bool DataStore::appendMessage(const heltec::meshcore::history::ConversationKey& key,
                              heltec::meshcore::history::MessageDirection direction,
                              const char* text, size_t text_len) {
  if (!ensureMessageHistory()) return false;
  if (!text) {
    text = "";
    text_len = 0;
  }
  if (text_len > heltec::meshcore::history::kMessageTextMax) {
    text_len = heltec::meshcore::history::kMessageTextMax;
  }

  uint32_t sequence = _messageMaxSequence + 1U;
  if (sequence == 0) sequence = 1;
  const uint16_t slot = static_cast<uint16_t>((sequence - 1U) %
      heltec::meshcore::history::kMessageCapacity);

  MessageRecordDisk record{};
  record.magic = kMessageMagic;
  record.sequence = sequence;
  record.timestamp = _clock ? _clock->getCurrentTime() : 0;
  record.direction = static_cast<uint8_t>(direction);
  record.text_len = static_cast<uint8_t>(text_len);
  setKey(record, key);
  if (text_len > 0) memcpy(record.text, text, text_len);
  record.crc = recordCrc(record);

  File file = openUpdate(_getContactsChannelsFS(), kMessagesPath);
  if (!file) return false;
  const bool ok = file.seek(static_cast<uint32_t>(slot) * sizeof(record)) &&
                  file.write(reinterpret_cast<const uint8_t*>(&record), sizeof(record)) == sizeof(record);
  file.close();
  if (ok) _messageMaxSequence = sequence;
  return ok;
}

int DataStore::fillRecentConversations(
    int offset, heltec::meshcore::history::ConversationSummary* items,
    int max_items, int* total_items) {
  if (total_items) *total_items = 0;
  if (!items || max_items <= 0 || !ensureMessageHistory()) return 0;
  if (offset < 0) offset = 0;

  for (int i = 0; i < max_items; ++i) items[i] = ConversationSummary{};
  ConversationKey seen[heltec::meshcore::history::kMessageCapacity]{};
  int seen_count = 0;
  int result_count = 0;

  File file = openReadOnly(_getContactsChannelsFS(), kMessagesPath);
  if (!file) return 0;
  for (uint16_t age = 0; age < heltec::meshcore::history::kMessageCapacity; ++age) {
    if (_messageMaxSequence <= age) break;
    const uint32_t expected_sequence = _messageMaxSequence - age;
    const uint16_t slot = static_cast<uint16_t>((expected_sequence - 1U) %
        heltec::meshcore::history::kMessageCapacity);
    MessageRecordDisk record{};
    if (!readMessageAt(file, slot, record) || !validMessageRecord(record) ||
        record.sequence != expected_sequence) {
      continue;
    }
    const ConversationKey key = keyFrom(record);
    bool duplicate = false;
    for (int i = 0; i < seen_count; ++i) {
      if (heltec::meshcore::history::sameConversation(seen[i], key)) {
        duplicate = true;
        break;
      }
    }
    if (duplicate) continue;
    seen[seen_count++] = key;

    const int rank = seen_count - 1;
    if (rank < offset || result_count >= max_items) continue;
    ConversationSummary& out = items[result_count++];
    out.key = key;
    out.latest_sequence = record.sequence;
    out.timestamp = record.timestamp;
    out.direction = static_cast<MessageDirection>(record.direction);
  }
  file.close();
  if (total_items) *total_items = seen_count;

  uint32_t watermarks[16]{};
  const int watermark_count = result_count < 16 ? result_count : 16;
  File states = openReadOnly(_getContactsChannelsFS(), kReadStatePath);
  if (states) {
    ReadStateDisk state{};
    while (states.read(reinterpret_cast<uint8_t*>(&state), sizeof(state)) == sizeof(state)) {
      if (!validReadState(state)) continue;
      const ConversationKey state_key = keyFrom(state);
      for (int i = 0; i < watermark_count; ++i) {
        if (heltec::meshcore::history::sameConversation(items[i].key, state_key)) {
          watermarks[i] = state.last_read_sequence;
          break;
        }
      }
    }
    states.close();
  }

  file = openReadOnly(_getContactsChannelsFS(), kMessagesPath);
  if (!file) return result_count;
  MessageRecordDisk record{};
  while (file.read(reinterpret_cast<uint8_t*>(&record), sizeof(record)) == sizeof(record)) {
    if (!validMessageRecord(record) ||
        record.direction != static_cast<uint8_t>(MessageDirection::Incoming)) {
      continue;
    }
    const ConversationKey key = keyFrom(record);
    for (int i = 0; i < watermark_count; ++i) {
      if (record.sequence > watermarks[i] &&
          heltec::meshcore::history::sameConversation(items[i].key, key)) {
        if (items[i].unread < 0xFFFFU) ++items[i].unread;
        break;
      }
    }
  }
  file.close();
  return result_count;
}

int DataStore::fillConversationMessages(
    const heltec::meshcore::history::ConversationKey& key, int offset_from_latest,
    heltec::meshcore::history::MessageItem* items, int max_items, int* total_items) {
  if (total_items) *total_items = 0;
  if (!items || max_items <= 0 || !ensureMessageHistory()) return 0;
  if (offset_from_latest < 0) offset_from_latest = 0;
  for (int i = 0; i < max_items; ++i) items[i] = MessageItem{};

  File file = openReadOnly(_getContactsChannelsFS(), kMessagesPath);
  if (!file) return 0;
  int matched = 0;
  int result_count = 0;
  for (uint16_t age = 0; age < heltec::meshcore::history::kMessageCapacity; ++age) {
    if (_messageMaxSequence <= age) break;
    const uint32_t expected_sequence = _messageMaxSequence - age;
    const uint16_t slot = static_cast<uint16_t>((expected_sequence - 1U) %
        heltec::meshcore::history::kMessageCapacity);
    MessageRecordDisk record{};
    if (!readMessageAt(file, slot, record) || !validMessageRecord(record) ||
        record.sequence != expected_sequence ||
        !heltec::meshcore::history::sameConversation(keyFrom(record), key)) {
      continue;
    }
    if (matched >= offset_from_latest && result_count < max_items) {
      MessageItem& out = items[result_count++];
      out.sequence = record.sequence;
      out.timestamp = record.timestamp;
      out.direction = static_cast<MessageDirection>(record.direction);
      const size_t len = record.text_len;
      if (len > 0) memcpy(out.text, record.text, len);
      out.text[len] = '\0';
    }
    ++matched;
  }
  file.close();
  if (total_items) *total_items = matched;

  for (int i = 0; i < result_count / 2; ++i) {
    const MessageItem tmp = items[i];
    items[i] = items[result_count - 1 - i];
    items[result_count - 1 - i] = tmp;
  }
  return result_count;
}

bool DataStore::markConversationRead(const heltec::meshcore::history::ConversationKey& key) {
  if (!ensureMessageHistory()) return false;
  uint32_t latest_sequence = 0;
  File messages = openReadOnly(_getContactsChannelsFS(), kMessagesPath);
  if (!messages) return false;
  MessageRecordDisk message{};
  while (messages.read(reinterpret_cast<uint8_t*>(&message), sizeof(message)) == sizeof(message)) {
    if (validMessageRecord(message) && message.sequence > latest_sequence &&
        heltec::meshcore::history::sameConversation(keyFrom(message), key)) {
      latest_sequence = message.sequence;
    }
  }
  messages.close();
  if (latest_sequence == 0) return false;

  int target_slot = -1;
  int free_slot = -1;
  int oldest_slot = 0;
  uint32_t oldest_sequence = 0xFFFFFFFFUL;
  uint32_t current_watermark = 0;
  File states = openReadOnly(_getContactsChannelsFS(), kReadStatePath);
  if (!states) return false;
  ReadStateDisk state{};
  for (uint16_t slot = 0; slot < heltec::meshcore::history::kMessageCapacity; ++slot) {
    if (states.read(reinterpret_cast<uint8_t*>(&state), sizeof(state)) != sizeof(state)) break;
    if (!validReadState(state)) {
      if (free_slot < 0) free_slot = slot;
      continue;
    }
    if (heltec::meshcore::history::sameConversation(keyFrom(state), key)) {
      target_slot = slot;
      current_watermark = state.last_read_sequence;
      break;
    }
    if (state.last_read_sequence < oldest_sequence) {
      oldest_sequence = state.last_read_sequence;
      oldest_slot = slot;
    }
  }
  states.close();
  if (current_watermark >= latest_sequence) return false;
  if (target_slot < 0) target_slot = free_slot >= 0 ? free_slot : oldest_slot;

  ReadStateDisk replacement{};
  replacement.magic = kReadStateMagic;
  replacement.last_read_sequence = latest_sequence;
  setKey(replacement, key);
  replacement.crc = recordCrc(replacement);

  states = openUpdate(_getContactsChannelsFS(), kReadStatePath);
  if (!states) return false;
  const bool ok = states.seek(static_cast<uint32_t>(target_slot) * sizeof(replacement)) &&
                  states.write(reinterpret_cast<const uint8_t*>(&replacement), sizeof(replacement)) ==
                      sizeof(replacement);
  states.close();
  return ok;
}

int DataStore::countUnreadMessages() {
  if (!ensureMessageHistory()) return 0;
  struct Watermark {
    ConversationKey key{};
    uint32_t sequence = 0;
  } watermarks[heltec::meshcore::history::kMessageCapacity];
  int watermark_count = 0;

  File states = openReadOnly(_getContactsChannelsFS(), kReadStatePath);
  if (states) {
    ReadStateDisk state{};
    while (watermark_count < heltec::meshcore::history::kMessageCapacity &&
           states.read(reinterpret_cast<uint8_t*>(&state), sizeof(state)) == sizeof(state)) {
      if (!validReadState(state)) continue;
      watermarks[watermark_count].key = keyFrom(state);
      watermarks[watermark_count].sequence = state.last_read_sequence;
      ++watermark_count;
    }
    states.close();
  }

  int unread = 0;
  File messages = openReadOnly(_getContactsChannelsFS(), kMessagesPath);
  if (!messages) return 0;
  MessageRecordDisk message{};
  while (messages.read(reinterpret_cast<uint8_t*>(&message), sizeof(message)) == sizeof(message)) {
    if (!validMessageRecord(message) ||
        message.direction != static_cast<uint8_t>(MessageDirection::Incoming)) {
      continue;
    }
    uint32_t watermark = 0;
    const ConversationKey key = keyFrom(message);
    for (int i = 0; i < watermark_count; ++i) {
      if (heltec::meshcore::history::sameConversation(watermarks[i].key, key)) {
        watermark = watermarks[i].sequence;
        break;
      }
    }
    if (message.sequence > watermark) ++unread;
  }
  messages.close();
  return unread;
}

bool DataStore::clearMessageHistory() {
  FILESYSTEM* fs = _getContactsChannelsFS();
  if (!fs) return false;
  const bool messages_ok = !fs->exists(kMessagesPath) || fs->remove(kMessagesPath);
  const bool reads_ok = !fs->exists(kReadStatePath) || fs->remove(kReadStatePath);
  _messageMaxSequence = 0;
  _messageHistoryReady = false;
  return messages_ok && reads_ok;
}
