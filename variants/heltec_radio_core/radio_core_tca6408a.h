/**
 * @file    radio_core_tca6408a.h
 * @brief   Radio Core TCA6408A GPIO expander interface.
 */
#pragma once

#include <Arduino.h>
#include <Wire.h>

class RadioCoreTCA6408A
{
  public:
    explicit RadioCoreTCA6408A(TwoWire &wire = Wire);

    bool begin(int reset_pin, int interrupt_pin);
    bool refresh();
    uint8_t pollPressedEdges();
    bool isReady() const;
    uint8_t getDipSwitchState() const;
    bool isDipSwitchOn(uint8_t index) const;
    uint8_t getRawInputState() const;

  private:
    bool writeRegister(uint8_t reg, uint8_t value);
    bool readRegister(uint8_t reg, uint8_t &value);

    TwoWire &m_wire;
    uint8_t m_input_state;
    uint8_t m_previous_dip_state;
    uint32_t m_last_press_ms[2];
    bool m_ready;
};
