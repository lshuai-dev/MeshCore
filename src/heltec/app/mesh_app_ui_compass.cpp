#include "mesh_app_ui.hpp"

#include "target.h"

#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
#include "HeltecMesh.h"
#include "config/DataStore.h"
#include <heltec/sensors/ICMCompassProvider.h>
#include <cstring>

extern ICMCompassProvider compassProvider;
extern HeltecMesh the_mesh;
#endif

namespace heltec::meshcore::biz {

#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS

void MeshAppUi::syncCompassCache() {
  CompassUi next{};
  if (compassProvider.hasHardware()) {
    (void)compassProvider.fillUiSnapshot(next);
  }

  if (memcmp(&next, &_compass_ui, sizeof(next)) != 0) {
    _compass_ui = next;
  }
}

bool MeshAppUi::compassHasHardware() const {
  return compassProvider.hasHardware();
}

void MeshAppUi::beginCompassCalibration() {
  if (compassProvider.hasHardware()) {
    compassProvider.setCalibrationState(true);
  }
}

void MeshAppUi::endCompassCalibration() {
  if (compassProvider.hasHardware()) {
    compassProvider.setCalibrationState(false);
  }
}

bool MeshAppUi::compassHasStoredCalibration() const {
  const DataStore* ds = the_mesh.getDataStore();
  return ds && ds->hasCompassMagCal();
}

bool MeshAppUi::saveCompassCalibration() {
  if (!compassProvider.hasHardware()) return false;
  float hmm[4];
  if (!compassProvider.exportMagCalibration(hmm)) return false;
  DataStore* ds = the_mesh.getDataStore();
  return ds && ds->saveCompassMagCal(hmm);
}

bool MeshAppUi::restoreCompassCalibration() {
  DataStore* ds = the_mesh.getDataStore();
  float hmm[4];
  if (!ds || !ds->loadCompassMagCal(hmm)) return false;
  compassProvider.applyMagCalibration(hmm);
  return true;
}

#endif  // ENV_INCLUDE_COMPASS

}  // namespace heltec::meshcore::biz
