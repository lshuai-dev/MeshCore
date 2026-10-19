#pragma once

#define RADIOLIB_STATIC_ONLY 1
#include <RadioLib.h>
#include <helpers/radiolib/RadioLibWrappers.h>
#include <../heltec_v3/HeltecV3Board.h>
class HeltecWirelessPaperBoard : public HeltecV3Board {
public:
  const char* getManufacturerName() const override {
    return "Heltec Wireless Paper";
  }
};
#include <helpers/radiolib/CustomSX1262Wrapper.h>
#include <helpers/AutoDiscoverRTCClock.h>
#include <helpers/SensorManager.h>
#include <heltec/sensors/HeltecEnvironmentSensorManager.h>
#if defined(DISPLAY_CLASS) && !(defined(LV_CONF_SKIP) && LV_CONF_SKIP)
#include <helpers/ui/E213Display.h>
#include <helpers/ui/MomentaryButton.h>
#endif

extern HeltecWirelessPaperBoard board;
extern WRAPPER_CLASS radio_driver;
extern AutoDiscoverRTCClock rtc_clock;
#if defined(LV_CONF_SKIP) && LV_CONF_SKIP
extern HeltecEnvironmentSensorManager sensors;
#else
extern SensorManager sensors;
#endif

#if defined(DISPLAY_CLASS) && !(defined(LV_CONF_SKIP) && LV_CONF_SKIP)
extern DISPLAY_CLASS display;
extern MomentaryButton user_btn;
#endif

bool radio_init();
uint32_t radio_get_rng_seed();
void radio_set_params(float freq, float bw, uint8_t sf, uint8_t cr);
void radio_set_tx_power(int8_t dbm);
mesh::LocalIdentity radio_new_identity();
