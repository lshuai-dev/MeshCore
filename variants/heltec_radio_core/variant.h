#pragma once

#if defined(ARDUINO_ARCH_NRF52)
#include "WVariant.h"

#define USE_LFXO
#define VARIANT_MCK (64000000ul)
#define PINS_COUNT 48
#define NUM_DIGITAL_PINS 48
#define NUM_ANALOG_INPUTS 8
#define NUM_ANALOG_OUTPUTS 0

#define WIRE_INTERFACES_COUNT 1
#define PIN_WIRE_SDA 5
#define PIN_WIRE_SCL 15

#define SPI_INTERFACES_COUNT 1
#define SPI_32MHZ_INTERFACE 1
#define PIN_SPI_MISO P_LORA_MISO
#define PIN_SPI_MOSI P_LORA_MOSI
#define PIN_SPI_SCK P_LORA_SCLK
#define PIN_SPI_NSS P_LORA_NSS

#define PIN_SERIAL1_RX 7
#define PIN_SERIAL1_TX 8

#define LED_BUILTIN (-1)
#define LED_RED (-1)
#define LED_BLUE (-1)
#define LED_STATE_ON LOW

#define EXTERNAL_FLASH_USE_QSPI
#endif
