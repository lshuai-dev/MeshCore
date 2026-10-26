#include "MeshAppUi.hpp"
#include "Target.h"
#include "app/PowerMgr.hpp"

#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
#include "HeltecMesh.h"
#include "config/DataStore.h"

#include <heltec/sensors/IcmCompassProvider.h>

extern ICMCompassProvider compassProvider;
extern HeltecMesh the_mesh;
#endif

namespace Heltec::MeshCore::Biz
{

#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS

CompassUi MeshAppUi::compassUi() const
{
    CompassUi snapshot{};
    if (compassProvider.hasHardware())
    {
        (void)compassProvider.fillUiSnapshot(snapshot);
    }
    return snapshot;
}

bool MeshAppUi::compassHasHardware() const
{
    return compassProvider.hasHardware();
}

void MeshAppUi::beginCompassCalibration()
{
    _power.setCompassDemand(Power::CompassDemand::Calibration, true);
    if (compassProvider.hasHardware())
    {
        compassProvider.setCalibrationState(true);
    }
}

void MeshAppUi::endCompassCalibration()
{
    _power.setCompassDemand(Power::CompassDemand::Calibration, false);
    if (compassProvider.hasHardware())
    {
        compassProvider.setCalibrationState(false);
    }
}

bool MeshAppUi::compassHasStoredCalibration() const
{
    const DataStore *ds = the_mesh.getDataStore();
    return ds && ds->hasCompassMagCal();
}

bool MeshAppUi::saveCompassCalibration()
{
    if (!compassProvider.hasHardware())
        return false;
    float hmm[4];
    if (!compassProvider.exportMagCalibration(hmm))
        return false;
    DataStore *ds = the_mesh.getDataStore();
    return ds && ds->saveCompassMagCal(hmm);
}

bool MeshAppUi::restoreCompassCalibration()
{
    DataStore *ds = the_mesh.getDataStore();
    float hmm[4];
    if (!ds || !ds->loadCompassMagCal(hmm))
        return false;
    compassProvider.applyMagCalibration(hmm);
    return true;
}

#endif // ENV_INCLUDE_COMPASS

} // namespace Heltec::MeshCore::Biz
