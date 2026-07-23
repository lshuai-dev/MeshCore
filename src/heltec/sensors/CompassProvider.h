#pragma once

#include "Mesh.h"
struct CompassResult {
    float coordinates[3];
    float fAzimuth;
    float fPitch;
    float fRoll;
    float fQuality;
    float fSimpleHeadingDegrees;
    float fFilteredHeadingDegrees;
};

class CompassProvider {
public:
    virtual void begin() = 0;
    virtual void loop() = 0;
    virtual bool getResult(CompassResult* result) = 0;
    virtual void setCalibrationState(bool bEnable) {}
    virtual bool hasHardware() const { return true; }
};
