#pragma once

#include <Arduino.h>
#include <helpers/RefCountedDigitalPin.h>

#if defined(RADIOCORE_BOARD_RC52)
#include <helpers/NRF52Board.h>

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

    RefCountedDigitalPin periph_power;
};

using HeltecRadioCoreBoard = HeltecRadioCoreRC52Board;
#else
#include <helpers/ESP32Board.h>

class HeltecRadioCoreESP32Board : public ESP32Board
{
  public:
    explicit HeltecRadioCoreESP32Board(const char *board_name);
    void begin();
    void onBeforeTransmit() override;
    void onAfterTransmit() override;
    void enterDeepSleep(uint32_t secs, int pin_wake_btn = -1);
    void powerOff() override;
    uint16_t getBattMilliVolts() override;
    const char *getManufacturerName() const override;

    RefCountedDigitalPin periph_power;

  private:
    const char *m_board_name;
};

class HeltecRadioCoreRC32Board : public HeltecRadioCoreESP32Board
{
  public:
    HeltecRadioCoreRC32Board();
};

class HeltecRadioCoreRCC6Board : public HeltecRadioCoreESP32Board
{
  public:
    HeltecRadioCoreRCC6Board();
};

#if defined(RADIOCORE_BOARD_RC32)
using HeltecRadioCoreBoard = HeltecRadioCoreRC32Board;
#else
using HeltecRadioCoreBoard = HeltecRadioCoreRCC6Board;
#endif
#endif
