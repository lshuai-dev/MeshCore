#include "mesh_app_ui.hpp"

#include <Arduino.h>

namespace heltec::meshcore::biz {

void MeshAppUi::requestHibernate() {
  _power.requestPowerOff(millis());
}

}  // namespace heltec::meshcore::biz
