#include "mesh_app_ui.hpp"

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

int MeshAppUi::fillRecentHeard(RecentHeardItem* items, int max_items) const {
  if (!items || max_items <= 0) return 0;
  constexpr int kPaths = 16;
  AdvertPath paths[kPaths];
  the_mesh.getRecentlyHeard(paths, kPaths);
  mesh::RTCClock* rtc = the_mesh.getRTCClock();
  const uint32_t now = rtc ? rtc->getCurrentTime() : 0;
  int n = 0;
  for (int i = 0; i < kPaths && n < max_items; ++i) {
    if (paths[i].name[0] == '\0') continue;
    strncpy(items[n].name, paths[i].name, sizeof(items[n].name) - 1);
    items[n].name[sizeof(items[n].name) - 1] = '\0';
    items[n].age_seconds =
        (now >= paths[i].recv_timestamp) ? (int32_t)(now - paths[i].recv_timestamp) : 0;
    n++;
  }
  return n;
}

}  // namespace heltec::meshcore::biz
