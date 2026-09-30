#pragma once

#if defined(ARDUINO_ARCH_NRF52)
#include "WVariant.h"

#define USE_LFXO
#define VARIANT_MCK                 (64000000ul)
#define PINS_COUNT                  (48)
#define NUM_DIGITAL_PINS 48
#define NUM_ANALOG_INPUTS 8
#define NUM_ANALOG_OUTPUTS 0

#define LED_BUILTIN (-1)
#define LED_RED (-1)
#define LED_BLUE (-1)
#define LED_STATE_ON LOW

#define EXTERNAL_FLASH_USE_QSPI
#endif
