#pragma once
#include <stdint.h>

class LoRaFEMControl
{
  public:
    LoRaFEMControl() {}
    virtual ~LoRaFEMControl() {}
    void init(void);
    void setSleepModeEnable(void);
    void setTxModeEnable(void);
    void setRxModeEnable(void);
    void setRxModeEnableWhenMCUSleep(void);
    void setLNAEnable(bool enabled);
    bool isLnaCanControl(void) const { return true; }

  private:
    bool lna_enabled = false;
};
