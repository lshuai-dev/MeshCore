#pragma once

// Keep the application USB serial port, but do not link unused USB device
// classes into the nRF52 BLE firmware.
#define CFG_TUD_CDC 1
#define CFG_TUD_MSC 0
#define CFG_TUD_HID 0
#define CFG_TUD_MIDI 0
#define CFG_TUD_VENDOR 0
#define CFG_TUD_VIDEO 0
#define CFG_TUD_VIDEO_STREAMING 0

// Reuse the framework's nRF52 controller/RTOS settings. The class counts
// above are intentionally defined first because the framework provides them
// as overridable defaults.
#include "arduino/ports/nrf/tusb_config_nrf.h"

// The firmware is a USB device only. The framework default also enables a
// MAX3421E USB-host stack even though these boards do not use one.
#undef CFG_TUH_ENABLED
#define CFG_TUH_ENABLED 0
#undef CFG_TUH_MAX3421
#define CFG_TUH_MAX3421 0
#undef CFG_TUH_HUB
#define CFG_TUH_HUB 0
#undef CFG_TUH_MSC
#define CFG_TUH_MSC 0
#undef CFG_TUH_HID
#define CFG_TUH_HID 0
#undef CFG_TUH_CDC
#define CFG_TUH_CDC 0
#undef CFG_TUH_CDC_FTDI
#define CFG_TUH_CDC_FTDI 0
#undef CFG_TUH_CDC_CP210X
#define CFG_TUH_CDC_CP210X 0
#undef CFG_TUH_CDC_CH34X
#define CFG_TUH_CDC_CH34X 0
