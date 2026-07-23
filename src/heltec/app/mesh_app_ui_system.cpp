#include "mesh_app_ui.hpp"

#include "HeltecMesh.h"
#include "MeshCore.h"
#include "config/DataStore.h"
#include "config/heltec_license.h"
#include "ui/app/ui_app.hpp"
#include "ui/core/ui_task.hpp"

#include <Arduino.h>
#include <lvgl.h>

namespace heltec::meshcore::biz {

namespace {

constexpr uint8_t kDisplayAutoOffSec[] = {10, 15, 20, 25, 30};
constexpr const char* kDisplayAutoOffLabels[] = {"10 s", "15 s", "20 s", "25 s", "30 s"};

int display_auto_off_index_from_sec(uint8_t sec) {
  for (int i = 0; i < 5; ++i) {
    if (kDisplayAutoOffSec[i] == sec) return i;
  }
  return 4;
}

}  // namespace

bool MeshAppUi::factoryReset() {
  DataStore* ds = the_mesh.getDataStore();
  if (!ds) return false;

  heltec::meshcore::ui::ui_task().disableSerial();
  if (!ds->formatFileSystem()) return false;

  lv_timer_t* const reboot_timer = lv_timer_create(
      [](lv_timer_t*) { board.reboot(); }, 1800, nullptr);
  if (reboot_timer) {
    lv_timer_set_repeat_count(reboot_timer, 1);
    return true;
  }

  delay(500);
  board.reboot();
  return true;
}

bool MeshAppUi::clearUserData() {
  if (!the_mesh.clearAllContacts()) return false;

#if defined(NRF52_PLATFORM) || defined(ESP32) || defined(ESP_PLATFORM)
  heltecLicenseClearStored();
#endif

#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  _ff_target_contact_idx = -1;
#endif

  return true;
}

int MeshAppUi::displayAutoOffIndex() const {
  NodePrefs* p = the_mesh.getNodePrefs();
  if (!p) return display_auto_off_index_from_sec(30);
  return display_auto_off_index_from_sec(p->display_auto_off_sec);
}

void MeshAppUi::setDisplayAutoOffIndex(int index) {
  NodePrefs* p = the_mesh.getNodePrefs();
  if (!p) return;
  if (index < 0) index = 0;
  if (index > 4) index = 4;

  const uint8_t sec = kDisplayAutoOffSec[index];
  if (p->display_auto_off_sec == sec) {
    heltec::meshcore::ui::UiApp::instance().setDisplayAutoOffMs((uint32_t)sec * 1000u);
    return;
  }

  p->display_auto_off_sec = sec;
  the_mesh.savePrefs();
  heltec::meshcore::ui::UiApp::instance().setDisplayAutoOffMs((uint32_t)sec * 1000u);
  notifyAppState(heltec::meshcore::ui::AppStateEventType::ConfigChanged);
}

int MeshAppUi::displayAutoOffOptionCount() const { return 5; }

const char* MeshAppUi::displayAutoOffOptionLabel(int index) const {
  if (index < 0 || index > 4) return "?";
  return kDisplayAutoOffLabels[index];
}

bool MeshAppUi::isLnaCanControl() const {
  return heltec::meshcore::ui::ui_task().isLnaCanControl();
}

bool MeshAppUi::lnaEnabled() const {
  return heltec::meshcore::ui::ui_task().lnaEnabled();
}

bool MeshAppUi::setLnaEnabled(bool enabled) {
  if (!heltec::meshcore::ui::ui_task().setLnaEnabled(enabled)) return false;
  notifyAppState(heltec::meshcore::ui::AppStateEventType::ConfigChanged);
  return true;
}

}  // namespace heltec::meshcore::biz
