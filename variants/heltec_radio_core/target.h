#pragma once

#define RADIOLIB_STATIC_ONLY 1
#include <RadioLib.h>
#include <helpers/radiolib/RadioLibWrappers.h>
#if defined(RADIOCORE_BOARD_RC52)
#include <HeltecRadioCoreRC52Board.h>
using HeltecRadioCoreTargetBoard = HeltecRadioCoreRC52Board;
#elif defined(RADIOCORE_BOARD_RC32)
#include <HeltecRadioCoreRC32Board.h>
using HeltecRadioCoreTargetBoard = HeltecRadioCoreRC32Board;
#elif defined(RADIOCORE_BOARD_RCC6)
#include <HeltecRadioCoreRCC6Board.h>
using HeltecRadioCoreTargetBoard = HeltecRadioCoreRCC6Board;
#else
#error "A RadioCore board type must be selected"
#endif
#include <helpers/radiolib/CustomSX1262Wrapper.h>
#include <helpers/AutoDiscoverRTCClock.h>
#include <helpers/SensorManager.h>
#if defined(HELTEC_SENSOR_MANAGER) && HELTEC_SENSOR_MANAGER
#include <heltec/sensors/HeltecEnvironmentSensorManager.h>
#else
#include <helpers/sensors/EnvironmentSensorManager.h>
#endif
#ifdef DISPLAY_CLASS
  #include <helpers/ui/ST7735Display.h>
  #include <helpers/ui/MomentaryButton.h>
#endif

extern HeltecRadioCoreTargetBoard board;
extern WRAPPER_CLASS radio_driver;
extern AutoDiscoverRTCClock rtc_clock;
#if defined(HELTEC_SENSOR_MANAGER) && HELTEC_SENSOR_MANAGER
extern HeltecEnvironmentSensorManager sensors;
#else
extern EnvironmentSensorManager sensors;
#endif

#ifdef DISPLAY_CLASS
  extern DISPLAY_CLASS display;
  extern MomentaryButton user_btn;
#endif

bool radio_init();
uint32_t radio_get_rng_seed();
void radio_set_params(float freq, float bw, uint8_t sf, uint8_t cr);
void radio_set_tx_power(int8_t dbm);
mesh::LocalIdentity radio_new_identity();
