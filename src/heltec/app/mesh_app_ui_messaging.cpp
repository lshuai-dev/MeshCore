#include "mesh_app_ui.hpp"

#include <cstring>
#include <cstdio>

#include "HeltecMesh.h"
#include "MeshCore.h"
#include "Utils.h"
#include "ui/app/ui_app.hpp"
#include "ui/core/ui_task.hpp"

namespace heltec::meshcore::biz {

namespace {

/** Strip control chars (esp. \\n) so lv_roller option strings are not split. */
static void sanitizeContactDisplayName(char* name, size_t cap) {
  if (!name || cap == 0) return;
  name[cap - 1] = '\0';
  size_t len = 0;
  while (len + 1 < cap && name[len] != '\0') ++len;
  for (size_t i = 0; i < len; ++i) {
    const unsigned char c = static_cast<unsigned char>(name[i]);
    if (c < 32 || c == 127) name[i] = ' ';
  }
  while (len > 0 && name[len - 1] == ' ') name[--len] = '\0';
  size_t start = 0;
  while (start < len && name[start] == ' ') ++start;
  if (start > 0) {
    memmove(name, name + start, len - start + 1);
    len -= start;
  }
}

static void formatPersonalContactLabel(const ContactInfo& ci, char* label, size_t label_len) {
  if (!label || label_len == 0) return;
  strncpy(label, ci.name, label_len - 1);
  label[label_len - 1] = '\0';
  sanitizeContactDisplayName(label, label_len);
  if (label[0] != '\0') return;
  char hex[13];
  mesh::Utils::toHex(hex, ci.id.pub_key, 6);
  hex[12] = '\0';
  snprintf(label, label_len, "#%s", hex);
}

static bool personalContactEligible(const ContactInfo& ci) {
  return ci.name[0] != '\0';
}

void showSendResult(int rc) {
  auto& t = heltec::meshcore::ui::ui_task();
  if (rc == MSG_SEND_FAILED) {
    t.showAlert("send failed", 900);
  } else if (rc == MSG_SEND_SENT_DIRECT) {
    t.showAlert("sent (direct)", 900);
  } else {
    t.showAlert("sent (flood)", 900);
  }
}

#ifdef MESH_DEBUG
static void logTextPreview(const char* text, char* out, size_t out_len) {
  if (!out || out_len == 0) return;
  if (!text) {
    out[0] = '\0';
    return;
  }
  strncpy(out, text, out_len - 1);
  out[out_len - 1] = '\0';
  if (strlen(text) > out_len - 1) {
    const size_t n = out_len - 1;
    if (n >= 3) {
      out[n - 3] = '.';
      out[n - 2] = '.';
      out[n - 1] = '.';
      out[n] = '\0';
    }
  }
}
#endif

static bool nameEqualsIgnoreCase(const char* name, const char* expect) {
  if (!name || !expect) return false;
  while (*expect) {
    char a = *name++;
    char b = *expect++;
    if (a >= 'A' && a <= 'Z') a = static_cast<char>(a + ('a' - 'A'));
    if (b >= 'A' && b <= 'Z') b = static_cast<char>(b + ('a' - 'A'));
    if (a != b) return false;
  }
  return *name == '\0';
}

static bool isPublicBroadcastChannelName(const char* name) {
  return nameEqualsIgnoreCase(name, "public") || nameEqualsIgnoreCase(name, "broadcast");
}

#ifdef MAX_GROUP_CHANNELS
static bool resolvePublicChannel(mesh::GroupChannel& out_channel, const char** out_name,
                                 int* out_index = nullptr) {
  ChannelDetails ch;
  for (int idx = 0; idx < MAX_GROUP_CHANNELS; ++idx) {
    if (!the_mesh.getChannel(idx, ch) || ch.name[0] == '\0') continue;
    if (isPublicBroadcastChannelName(ch.name)) {
      out_channel = ch.channel;
      if (out_name) *out_name = ch.name;
      if (out_index) *out_index = idx;
      return true;
    }
  }
  return false;
}
#endif

}  // namespace

bool MeshAppUi::sendAdvert() {
  const bool queued = the_mesh.advert();
  showAlert(queued ? "Advert sent!" : "Advert failed", 1000);
  return queued;
}

int MeshAppUi::sendMessagePersonalCount() const {
  int count = 0;
  const int total = the_mesh.getNumContacts();
  for (int i = 0; i < total; ++i) {
    ContactInfo ci;
    if (!the_mesh.getContactByIdx((uint32_t)i, ci)) continue;
    if (!personalContactEligible(ci)) continue;
    ++count;
  }
  return count;
}

bool MeshAppUi::sendMessagePersonalAt(int index, uint8_t pub_key_prefix[6], char* label,
                                      size_t label_len) const {
  if (!pub_key_prefix || index < 0) return false;
  int seen = 0;
  const int total = the_mesh.getNumContacts();
  for (int i = 0; i < total; ++i) {
    ContactInfo ci;
    if (!the_mesh.getContactByIdx((uint32_t)i, ci)) continue;
    if (!personalContactEligible(ci)) continue;
    if (seen == index) {
      memcpy(pub_key_prefix, ci.id.pub_key, 6);
      if (label && label_len > 0) {
        formatPersonalContactLabel(ci, label, label_len);
      }
      return true;
    }
    ++seen;
  }
  return false;
}

bool MeshAppUi::sendMessageHasGroupChannels() const {
#ifndef MAX_GROUP_CHANNELS
  return false;
#else
  ChannelDetails ch;
  for (int idx = 0; idx < MAX_GROUP_CHANNELS; ++idx) {
    if (the_mesh.getChannel(idx, ch) && ch.name[0] != '\0') return true;
  }
  return false;
#endif
}

int MeshAppUi::sendMessageGroupCount() const {
#ifndef MAX_GROUP_CHANNELS
  return 0;
#else
  int count = 0;
  ChannelDetails ch;
  for (int idx = 0; idx < MAX_GROUP_CHANNELS; ++idx) {
    if (the_mesh.getChannel(idx, ch) && ch.name[0] != '\0') ++count;
  }
  return count;
#endif
}

bool MeshAppUi::sendMessageGroupAt(int index, int* channel_idx, char* label, size_t label_len) const {
  if (index < 0) return false;
#ifndef MAX_GROUP_CHANNELS
  (void)channel_idx;
  (void)label;
  (void)label_len;
  return false;
#else
  int seen = 0;
  ChannelDetails ch;
  for (int idx = 0; idx < MAX_GROUP_CHANNELS; ++idx) {
    if (!the_mesh.getChannel(idx, ch) || ch.name[0] == '\0') continue;
    if (seen == index) {
      if (channel_idx) *channel_idx = idx;
      if (label && label_len > 0) {
        strncpy(label, ch.name, label_len - 1);
        label[label_len - 1] = '\0';
      }
      return true;
    }
    ++seen;
  }
  return false;
#endif
}

bool MeshAppUi::sendDirectMessage(const uint8_t pub_key_prefix[6], const char* text) {
  if (!pub_key_prefix || !text || text[0] == '\0') return false;
  ContactInfo* recipient = the_mesh.lookupContactByPubKey(pub_key_prefix, 6);
  if (!recipient) {
    heltec::meshcore::ui::ui_task().showAlert("Send failed", 900);
    return false;
  }
  const uint32_t ts = the_mesh.getRTCClock()->getCurrentTimeUnique();
  uint32_t expected_ack = 0;
  uint32_t est_timeout = 0;
  const int rc = the_mesh.sendMessage(*recipient, ts, 0, text, expected_ack, est_timeout);
  if (rc != MSG_SEND_FAILED && expected_ack) {
    the_mesh.trackExpectedAck(expected_ack, recipient);
  }
  showSendResult(rc);
  return rc != MSG_SEND_FAILED;
}

bool MeshAppUi::sendGroupMessage(int channel_idx, const char* text) {
  if (!text || text[0] == '\0') return false;
#ifndef MAX_GROUP_CHANNELS
  (void)channel_idx;
  heltec::meshcore::ui::ui_task().showAlert("Group channels not supported", 1000);
  return false;
#else
  mesh::GroupChannel channel{};
  bool found = false;
  if (channel_idx >= 0) {
    ChannelDetails ch;
    if (the_mesh.getChannel(channel_idx, ch) && ch.name[0] != 0) {
      channel = ch.channel;
      found = true;
    }
  } else {
    found = resolvePublicChannel(channel, nullptr, nullptr);
  }
  if (!found) {
    heltec::meshcore::ui::ui_task().showAlert("No channel", 900);
    return false;
  }
  const uint32_t ts = the_mesh.getRTCClock()->getCurrentTimeUnique();
  const int len = static_cast<int>(strlen(text));
  const bool ok = the_mesh.sendGroupMessage(ts, channel, the_mesh.getNodeName(), text, len);
  heltec::meshcore::ui::ui_task().showAlert(ok ? "Queued" : "Failed", 900);
  return ok;
#endif
}

bool MeshAppUi::sendBroadcast(const char* text, int len) {
#ifndef MAX_GROUP_CHANNELS
  (void)text;
  (void)len;
  return false;
#else
  if (!text || len <= 0) return false;
  mesh::GroupChannel channel{};
  if (!resolvePublicChannel(channel, nullptr, nullptr)) {
    return false;
  }
  const uint32_t ts = the_mesh.getRTCClock()->getCurrentTimeUnique();
  const bool ok = the_mesh.sendGroupMessage(ts, channel, the_mesh.getNodeName(), text, len);
#ifdef MESH_DEBUG
  char preview[20];
  logTextPreview(text, preview, sizeof(preview));
#endif
  return ok;
#endif
}

}  // namespace heltec::meshcore::biz
