#pragma once

#include <Arduino.h>
#include <helpers/ESP32Board.h>
#include <helpers/RefCountedDigitalPin.h>

class HeltecRadioCoreRC32Board : public ESP32Board
{
  public:
    HeltecRadioCoreRC32Board();
    void begin();
    void onBeforeTransmit() override;
    void onAfterTransmit() override;
    void enterDeepSleep(uint32_t secs, int pin_wake_btn = -1);
    void powerOff() override;
    uint16_t getBattMilliVolts() override;
    const char *getManufacturerName() const override;

    RefCountedDigitalPin periph_power;
};
