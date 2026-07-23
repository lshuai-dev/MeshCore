#include "mesh_app_ui.hpp"

#include "HeltecMesh.h"
#include "ui/core/ui_task.hpp"
#include "config/NodePrefs.h"

namespace heltec::meshcore::biz {

bool MeshAppUi::companionLinkEnabled() const {
  const NodePrefs* p = the_mesh.getNodePrefs();
  if (p) return p->companion_link_enabled != 0;
  return heltec::meshcore::ui::ui_task().isSerialEnabled();
}

void MeshAppUi::setCompanionLinkEnabled(bool enabled) {
  NodePrefs* p = the_mesh.getNodePrefs();
  const uint8_t next = enabled ? 1 : 0;
  if (p && p->companion_link_enabled == next &&
      heltec::meshcore::ui::ui_task().isSerialEnabled() == enabled) {
    return;
  }

  if (enabled) {
    heltec::meshcore::ui::ui_task().enableSerial();
  } else {
    heltec::meshcore::ui::ui_task().disableSerial();
  }

  if (p) {
    p->companion_link_enabled = next;
    the_mesh.savePrefs();
  }
  notifyAppState(heltec::meshcore::ui::AppStateEventType::ConfigChanged);
  notifyCompanionChanged();
}

}  // namespace heltec::meshcore::biz
