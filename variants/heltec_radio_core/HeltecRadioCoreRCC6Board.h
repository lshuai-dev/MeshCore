#pragma once

#include <Arduino.h>
#include <helpers/ESP32Board.h>
#include <helpers/RefCountedDigitalPin.h>

class HeltecRadioCoreRCC6Board : public ESP32Board
{
  public:
    HeltecRadioCoreRCC6Board();
    void begin();
    void onBeforeTransmit() override;
    void onAfterTransmit() override;
    void enterDeepSleep(uint32_t secs, int pin_wake_btn = -1);
    void powerOff() override;
    uint16_t getBattMilliVolts() override;
    const char *getManufacturerName() const override;

    RefCountedDigitalPin periph_power;
};
