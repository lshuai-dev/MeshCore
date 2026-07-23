#pragma once

#define RADIOLIB_STATIC_ONLY 1
#include <RadioLib.h>
#include <SPI.h>
#include <helpers/radiolib/RadioLibWrappers.h>
#include <HeltecV4R8Board.h>
#include <helpers/radiolib/CustomSX1262Wrapper.h>
#include <helpers/AutoDiscoverRTCClock.h>
#include <helpers/SensorManager.h>

#if defined(HELTEC_DISPLAY_ST7789)
#include <heltec/sensors/HeltecEnvironmentSensorManager.h>
#else
#include <helpers/sensors/EnvironmentSensorManager.h>
#endif

#ifndef HELTEC_DISPLAY_ST7789
#ifdef DISPLAY_CLASS
  #ifdef HELTEC_V4_R8_OLED
    #include <helpers/ui/SSD1306Display.h>
  #elif defined(HELTEC_V4_R8_TFT)
    #include <helpers/ui/ST7789LCDDisplay.h>
  #endif
  #include <helpers/ui/MomentaryButton.h>
#endif
#endif

extern HeltecV4R8Board board;
extern WRAPPER_CLASS radio_driver;
extern AutoDiscoverRTCClock rtc_clock;

#if defined(HELTEC_DISPLAY_ST7789)
extern HeltecEnvironmentSensorManager sensors;
#else
extern EnvironmentSensorManager sensors;
#endif

#ifndef HELTEC_DISPLAY_ST7789
#ifdef DISPLAY_CLASS
  extern DISPLAY_CLASS display;
  extern MomentaryButton user_btn;
#endif
#endif

#if defined(SPI_INTERFACES_COUNT) && (SPI_INTERFACES_COUNT >= 2)
extern SPIClass SPI1;
#endif

bool radio_init();
uint32_t radio_get_rng_seed();
void radio_set_params(float freq, float bw, uint8_t sf, uint8_t cr);
void radio_set_tx_power(int8_t dbm);
mesh::LocalIdentity radio_new_identity();
