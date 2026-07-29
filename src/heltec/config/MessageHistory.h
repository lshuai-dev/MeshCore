#pragma once

#include <stddef.h>
#include <stdint.h>

namespace heltec::meshcore::history {

constexpr uint16_t kMessageCapacity = 128;
constexpr uint16_t kMessageTextMax = 160;

enum class ConversationType : uint8_t {
  Direct = 0,
  Channel = 1,
};

enum class MessageDirection : uint8_t {
  Incoming = 0,
  Outgoing = 1,
};

struct ConversationKey {
  ConversationType type = ConversationType::Direct;
  uint8_t channel_idx = 0;
  uint8_t peer_prefix[6] = {};
};

inline bool sameConversation(const ConversationKey& a, const ConversationKey& b) {
  if (a.type != b.type) return false;
  if (a.type == ConversationType::Channel) return a.channel_idx == b.channel_idx;
  for (size_t i = 0; i < sizeof(a.peer_prefix); ++i) {
    if (a.peer_prefix[i] != b.peer_prefix[i]) return false;
  }
  return true;
}

struct ConversationSummary {
  ConversationKey key{};
  uint32_t latest_sequence = 0;
  uint32_t timestamp = 0;
  MessageDirection direction = MessageDirection::Incoming;
  uint16_t unread = 0;
};

struct MessageItem {
  uint32_t sequence = 0;
  uint32_t timestamp = 0;
  MessageDirection direction = MessageDirection::Incoming;
  char text[kMessageTextMax + 1] = {};
};

}  // namespace heltec::meshcore::history
