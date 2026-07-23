#include "mesh_app_ui.hpp"

#include "HeltecMesh.h"
#include "config/LoRaBandPresets.h"
#include "config/NodePrefs.h"
#include "ui/app/ui_app.hpp"

#include <Arduino.h>
#include <target.h>

namespace heltec::meshcore::biz {

void MeshAppUi::requestRadioParamPresetPicker() {
  heltec::meshcore::ui::UiApp::instance().openRadioParamSyncOverlay();
}

void MeshAppUi::setLoRaBandPresetIndex(int preset_index) {
  NodePrefs* p = the_mesh.getNodePrefs();
  if (!p) return;
  if (preset_index < 0 || preset_index >= radioParamPresetCount()) return;

  const RadioParamPreset& pr = radioParamPreset(preset_index);
  p->freq = constrain(pr.freqMhz, 400.0f, 2500.0f);
  p->bw = constrain(pr.bwKhz, 7.8f, 500.0f);
  p->sf = (uint8_t)constrain((int)pr.sf, 5, 12);
  p->cr = (uint8_t)constrain((int)pr.cr, 5, 8);
  p->lora_band_configured = 1;

  radio_set_params(p->freq, p->bw, p->sf, p->cr);
  radio_set_tx_power(p->tx_power_dbm);
  the_mesh.savePrefs();
  notifyRadioChanged();
  notifyAppState(heltec::meshcore::ui::AppStateEventType::ConfigChanged);
}

int MeshAppUi::currentLoRaBandPresetIndex() const {
  return radioParamPresetIndexForPrefs(the_mesh.getNodePrefs());
}

int MeshAppUi::loRaBandPresetCount() const {
  return radioParamPresetCount();
}

const char* MeshAppUi::loRaBandPresetName(int preset_index) const {
  return radioParamPresetName(preset_index);
}

}  // namespace heltec::meshcore::biz
