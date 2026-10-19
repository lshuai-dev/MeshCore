#include "HeltecMesh.h"
#include "MeshAppUi.hpp"
#include "ui/core/UiTask.hpp"

namespace heltec::meshcore::biz
{

bool MeshAppUi::buzzerEnabled() const
{
    NodePrefs *p = the_mesh.getNodePrefs();
    return p && p->buzzer_quiet == 0;
}

void MeshAppUi::setBuzzerEnabled(bool enabled)
{
    heltec::meshcore::ui::ui_task().setBuzzerEnabled(enabled);
    notifyAppState(heltec::meshcore::ui::AppStateEventType::ConfigChanged);
}

uint8_t MeshAppUi::buzzerVolumeLevel() const
{
    return heltec::meshcore::ui::ui_task().buzzerVolumeLevel();
}

void MeshAppUi::setBuzzerVolumeLevel(uint8_t level)
{
    if (buzzerVolumeLevel() == level)
        return;
    heltec::meshcore::ui::ui_task().setBuzzerVolumeLevel(level);
    notifyAppState(heltec::meshcore::ui::AppStateEventType::ConfigChanged);
}

} // namespace heltec::meshcore::biz
