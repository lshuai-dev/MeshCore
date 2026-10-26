#include "HeltecMesh.h"
#include "config/NodePrefs.h"
#include "MeshAppUi.hpp"
#include "ui/core/UiTask.hpp"

namespace Heltec::MeshCore::Biz
{

bool MeshAppUi::companionLinkEnabled() const
{
    const NodePrefs *p = the_mesh.getNodePrefs();
    if (p)
        return p->companion_link_enabled != 0;
    return Heltec::MeshCore::Ui::uiTask().isSerialEnabled();
}

void MeshAppUi::setCompanionLinkEnabled(bool enabled)
{
    NodePrefs *p = the_mesh.getNodePrefs();
    const uint8_t next = enabled ? 1 : 0;
    if (p && p->companion_link_enabled == next &&
        Heltec::MeshCore::Ui::uiTask().isSerialEnabled() == enabled)
    {
        return;
    }

    if (enabled)
    {
        Heltec::MeshCore::Ui::uiTask().enableSerial();
    }
    else
    {
        Heltec::MeshCore::Ui::uiTask().disableSerial();
    }

    if (p)
    {
        p->companion_link_enabled = next;
        the_mesh.savePrefs();
    }
    notifyAppState(Heltec::MeshCore::Ui::AppStateEventType::ConfigChanged);
    notifyAppState(Heltec::MeshCore::Ui::AppStateEventType::CompanionChanged);
}

} // namespace Heltec::MeshCore::Biz
