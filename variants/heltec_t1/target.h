#pragma once

#define RADIOLIB_STATIC_ONLY 1
#include <RadioLib.h>
#include <T1Board.h>
#include <helpers/ArduinoHelpers.h>
#include <helpers/radiolib/CustomSX1262Wrapper.h>
#include <helpers/radiolib/RadioLibWrappers.h>
#if defined(HELTEC_SENSOR_MANAGER) && HELTEC_SENSOR_MANAGER
#include "sensors/HeltecEnvironmentSensorManager.h"
#else
#include <helpers/sensors/EnvironmentSensorManager.h>
#endif
#include <helpers/sensors/LocationProvider.h>

#ifdef DISPLAY_CLASS
#if !(defined(HELTEC_MESH_UI) && HELTEC_MESH_UI)
#include <helpers/ui/MomentaryButton.h>
#include <helpers/ui/ST7735Display.h>
extern DISPLAY_CLASS display;
extern MomentaryButton user_btn;
#endif
#endif

extern T1Board board;
extern WRAPPER_CLASS radio_driver;

class T1RTCClock : public VolatileRTCClock {
public:
  void setCurrentTime(uint32_t time) override;
  void tick() override;
};

extern T1RTCClock rtc_clock;
#if defined(HELTEC_SENSOR_MANAGER) && HELTEC_SENSOR_MANAGER
extern HeltecEnvironmentSensorManager sensors;
#else
extern EnvironmentSensorManager sensors;
#endif

#ifdef DISPLAY_CLASS
#if !(defined(HELTEC_MESH_UI) && HELTEC_MESH_UI)
extern DISPLAY_CLASS display;
extern MomentaryButton user_btn;
#endif
#endif

bool radio_init();
uint32_t radio_get_rng_seed();
void radio_set_params(float freq, float bw, uint8_t sf, uint8_t cr);
void radio_set_tx_power(int8_t dbm);
mesh::LocalIdentity radio_new_identity();
