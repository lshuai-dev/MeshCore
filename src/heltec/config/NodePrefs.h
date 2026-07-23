#pragma once
#include <cstdint> // For uint8_t, uint32_t

#define TELEM_MODE_DENY            0
#define TELEM_MODE_ALLOW_FLAGS     1     // use contact.flags
#define TELEM_MODE_ALLOW_ALL       2

#define ADVERT_LOC_NONE       0
#define ADVERT_LOC_SHARE      1

/** Min GPS movement (meters) to trigger an extra location-share advert between periodic sends. */
#ifndef LOC_SHARE_MOVEMENT_ADVERT_M
#define LOC_SHARE_MOVEMENT_ADVERT_M 50.0
#endif

struct NodePrefs {  // persisted to file
  float airtime_factor;
  char node_name[32];
  float freq;
  uint8_t sf;
  uint8_t cr;
  uint8_t multi_acks;
  uint8_t manual_add_contacts;
  float bw;
  int8_t tx_power_dbm;
  uint8_t telemetry_mode_base;
  uint8_t telemetry_mode_loc;
  uint8_t telemetry_mode_env;
  float rx_delay_base;
  uint32_t ble_pin;
  uint8_t  advert_loc_policy;
  uint8_t  buzzer_quiet;
  uint8_t  gps_enabled;      // GPS enabled flag (0=disabled, 1=enabled)
  uint32_t gps_interval;     // GPS read interval in seconds
  uint8_t autoadd_config;    // bitmask for auto-add contacts config
  uint8_t client_repeat;
  uint8_t path_hash_mode;    // 0=1-byte, 1=2-byte, 2=3-byte path hashes when sending flood packets
  uint8_t lora_band_configured;
  uint8_t display_auto_off_sec;  // 10, 15, 20, 25, or 30; 0 = default (30 s)
  uint8_t companion_link_enabled;  // 0=off, 1=on (BLE/USB companion)
  uint32_t loc_share_adv_sec;      // 30, 60, 180, 300, or 600
  uint8_t gps_track_armed;       // 0=off, 1=resume GPS track recording when GPS on
  uint8_t buzzer_volume_level;   // retained active level: 1=1x, 2=2x, 3=3x; buzzer_quiet means off
  uint8_t lna_enabled;           // 0=LNA bypass, 1=LNA enabled
};
