#pragma once

#define RADIOLIB_STATIC_ONLY 1
#include <RadioLib.h>
#include <T114Board.h>
#include <helpers/AutoDiscoverRTCClock.h>
#include <helpers/radiolib/CustomSX1262Wrapper.h>
#include <helpers/radiolib/RadioLibWrappers.h>
#if defined(HELTEC_SENSOR_MANAGER) && HELTEC_SENSOR_MANAGER
#include <heltec/sensors/HeltecEnvironmentSensorManager.h>
#else
#include <helpers/sensors/EnvironmentSensorManager.h>
#endif
#include <helpers/sensors/LocationProvider.h>

#ifdef DISPLAY_CLASS
#include <helpers/ui/MomentaryButton.h>
#ifdef HELTEC_T114_WITH_DISPLAY
#include <helpers/ui/ST7789Display.h>
#else
#include "helpers/ui/NullDisplayDriver.h"
#endif
#endif

extern T114Board board;
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
