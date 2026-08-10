#pragma once

#include <stdint.h>

namespace heltec::meshcore::ui {

enum class AppStateEventType : uint8_t {
  // Complete snapshot payloads.
  BatteryChanged,
  GpsChanged,
  CompassChanged,
  CompanionChanged,
  UnreadMessageCountChanged,

  // Invalidation-only domains; receivers read the current facade snapshot.
  RadioChanged,
  RecentHeardChanged,
  ContactLocationChanged,
  FindFriendChanged,
  ConfigChanged,
  Count,
};

struct AppStateEvent {
  AppStateEventType type = AppStateEventType::ConfigChanged;

  union {
    struct {
      uint16_t millivolts;
      uint8_t percent;
      bool charging;
      bool source_known;
      bool external_powered;
    } battery;

    struct {
      uint8_t count;
    } unread;

    struct {
      bool connected;
      uint32_t pairing_pin;
    } companion;

    struct {
      bool enabled;
      bool available;
      bool powered;
      bool fix_valid;
      uint32_t fix_valid_ms;
      uint8_t satellites;
      long lat_micro;
      long lon_micro;
      double lat_deg;
      double lon_deg;
      double alt_m;
      float speed_kph;
    } gps;

    struct {
      bool heading_valid;
      float heading_deg;
      int quality;
    } compass;
  };
};

}  // namespace heltec::meshcore::ui
