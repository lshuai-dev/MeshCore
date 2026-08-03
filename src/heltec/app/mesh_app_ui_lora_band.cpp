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
  const uint32_t freq_khz =
      (uint32_t)lroundf(constrain(pr.freqMhz, 400.0f, 2500.0f) * 1000.0f);
  const uint32_t bw_hz =
      (uint32_t)lroundf(constrain(pr.bwKhz, 7.8f, 500.0f) * 1000.0f);
  const uint8_t sf = (uint8_t)constrain((int)pr.sf, 5, 12);
  const uint8_t cr = (uint8_t)constrain((int)pr.cr, 5, 8);

  const HeltecMesh::RadioConfigApplyResult result =
      the_mesh.applyRadioConfig(freq_khz, bw_hz, sf, cr, false, false);
  if (result != HeltecMesh::RadioConfigApplyResult::Ok) return;

  p->lora_band_configured = 1;
  the_mesh.savePrefs();
  notifyRadioChanged();
  notifyAppState(heltec::meshcore::ui::AppStateEventType::ConfigChanged);
}

int MeshAppUi::currentLoRaBandPresetIndex() const {
  return radioParamPresetIndexForPrefs(the_mesh.getNodePrefs());
}

int MeshAppUi::currentExactLoRaBandPresetIndex() const {
  const NodePrefs* p = the_mesh.getNodePrefs();
  if (!p) return -1;
  for (int i = 0; i < radioParamPresetCount(); ++i) {
    if (radioParamPresetMatchesPrefs(i, p)) return i;
  }
  return -1;
}

int MeshAppUi::loRaBandPresetCount() const {
  return radioParamPresetCount();
}

const char* MeshAppUi::loRaBandPresetName(int preset_index) const {
  return radioParamPresetName(preset_index);
}

bool MeshAppUi::forwardingEnabled() const {
  return the_mesh.clientRepeatEnabled();
}

int MeshAppUi::forwardingFrequencyCount() const {
  return (int)the_mesh.clientRepeatFrequencyCount();
}

bool MeshAppUi::forwardingFrequencyRange(int index, uint32_t* lower_khz,
                                         uint32_t* upper_khz) const {
  if (!lower_khz || !upper_khz || index < 0) return false;
  HeltecMesh::ClientRepeatFreqRange range{};
  if (!the_mesh.clientRepeatFrequencyAt((size_t)index, range)) return false;
  *lower_khz = range.lower_khz;
  *upper_khz = range.upper_khz;
  return true;
}

int MeshAppUi::currentForwardingFrequencyIndex() const {
  return the_mesh.currentClientRepeatFrequencyIndex();
}

IBizFacade::ForwardingApplyResult MeshAppUi::setForwardingEnabled(
    bool enabled, int frequency_index) {
  NodePrefs* p = the_mesh.getNodePrefs();
  if (!p) return ForwardingApplyResult::Unavailable;

  uint32_t freq_khz = (uint32_t)lroundf(p->freq * 1000.0f);
  if (enabled) {
    HeltecMesh::ClientRepeatFreqRange range{};
    if (frequency_index < 0 ||
        !the_mesh.clientRepeatFrequencyAt((size_t)frequency_index, range)) {
      return ForwardingApplyResult::InvalidSelection;
    }
    // The current Heltec forwarding catalog contains three single-frequency
    // entries. Keep range support in the domain model and use its lower value
    // as the selectable carrier if a range is introduced later.
    freq_khz = range.lower_khz;
  }

  const uint32_t bw_hz = (uint32_t)lroundf(p->bw * 1000.0f);
  const HeltecMesh::RadioConfigApplyResult result =
      the_mesh.applyRadioConfig(freq_khz, bw_hz, p->sf, p->cr, enabled, true);
  switch (result) {
    case HeltecMesh::RadioConfigApplyResult::Ok:
      notifyRadioChanged();
      notifyAppState(heltec::meshcore::ui::AppStateEventType::ConfigChanged);
      return ForwardingApplyResult::Ok;
    case HeltecMesh::RadioConfigApplyResult::UnsupportedForwardingFrequency:
      return ForwardingApplyResult::UnsupportedFrequency;
    case HeltecMesh::RadioConfigApplyResult::InvalidFrequency:
    case HeltecMesh::RadioConfigApplyResult::InvalidBandwidth:
    case HeltecMesh::RadioConfigApplyResult::InvalidSpreadingFactor:
    case HeltecMesh::RadioConfigApplyResult::InvalidCodingRate:
      return ForwardingApplyResult::InvalidRadioParams;
  }
  return ForwardingApplyResult::Unavailable;
}

}  // namespace heltec::meshcore::biz
