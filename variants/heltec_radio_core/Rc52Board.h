#pragma once

#include <Arduino.h>
#include <helpers/NRF52Board.h>
#include <helpers/RefCountedDigitalPin.h>
#include "RadioCoreTca6408a.h"

class HeltecRadioCoreRC52Board : public NRF52BoardDCDC
{
  public:
    HeltecRadioCoreRC52Board();
    void begin() override;
    void onBeforeTransmit() override;
    void onAfterTransmit() override;
    void powerOff() override;
    uint16_t getBattMilliVolts() override;
    const char *getManufacturerName() const override;

    RefCountedDigitalPin display_power;
    RadioCoreTCA6408A io_expander;
};
