#include "mesh_app_ui.hpp"

#include "target.h"
#include "ui/core/ui_task.hpp"

namespace heltec::meshcore::biz {

void MeshAppUi::requestHibernate() {
  heltec::meshcore::ui::ui_task().playShutdownMelody();
// #ifdef DISPLAY_CLASS
//   display.turnOff();
// #endif
  radio_driver.powerOff();
  board.powerOff();
}

}  // namespace heltec::meshcore::biz

