#include "mesh_app_ui.hpp"

#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "HeltecMesh.h"
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

int MeshAppUi::fillRecentlyHeard(RecentlyHeardItem* items,
                                 int max_items) const {
  if (!items || max_items <= 0) return 0;
  constexpr int kAdvertPathCapacity = 16;
  AdvertPath paths[kAdvertPathCapacity]{};
  const int output_capacity = max_items < kAdvertPathCapacity
                                  ? max_items
                                  : kAdvertPathCapacity;
  const int raw_count = the_mesh.getRecentlyHeard(paths, kAdvertPathCapacity);
  mesh::RTCClock* rtc = the_mesh.getRTCClock();
  const uint32_t now = rtc ? rtc->getCurrentTime() : 0;

  int count = 0;
  for (int i = 0; i < raw_count && count < output_capacity; ++i) {
    if (paths[i].name[0] == '\0') continue;
    items[count] = RecentlyHeardItem{};
    strncpy(items[count].name, paths[i].name, sizeof(items[count].name) - 1);
    const uint32_t age = paths[i].recv_timestamp != 0 && now >= paths[i].recv_timestamp
                             ? now - paths[i].recv_timestamp
                             : 0;
    items[count].age_seconds = age > static_cast<uint32_t>(INT32_MAX)
                                   ? INT32_MAX
                                   : static_cast<int32_t>(age);
    ++count;
  }
  return count;
}

}  // namespace heltec::meshcore::biz
