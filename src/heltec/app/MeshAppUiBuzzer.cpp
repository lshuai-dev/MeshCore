#include "HeltecMesh.h"
#include "MeshAppUi.hpp"
#include "ui/core/UiTask.hpp"

namespace Heltec::MeshCore::Biz
{

bool MeshAppUi::buzzerEnabled() const
{
    NodePrefs *p = the_mesh.getNodePrefs();
    return p && p->buzzer_quiet == 0;
}

void MeshAppUi::setBuzzerEnabled(bool enabled)
{
    Heltec::MeshCore::Ui::uiTask().setBuzzerEnabled(enabled);
    notifyAppState(Heltec::MeshCore::Ui::AppStateEventType::ConfigChanged);
}

uint8_t MeshAppUi::buzzerVolumeLevel() const
{
    return Heltec::MeshCore::Ui::uiTask().buzzerVolumeLevel();
}

void MeshAppUi::setBuzzerVolumeLevel(uint8_t level)
{
    if (buzzerVolumeLevel() == level)
        return;
    Heltec::MeshCore::Ui::uiTask().setBuzzerVolumeLevel(level);
    notifyAppState(Heltec::MeshCore::Ui::AppStateEventType::ConfigChanged);
}

} // namespace Heltec::MeshCore::Biz
