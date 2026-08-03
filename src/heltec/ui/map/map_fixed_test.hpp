#pragma once

#if defined(ENV_INCLUDE_MAP) && ENV_INCLUDE_MAP

#include "ui/core/biz_facade.hpp"

#if defined(HELTEC_MAP_FIXED_TEST) && HELTEC_MAP_FIXED_TEST

#ifndef HELTEC_MAP_FIXED_GPS_LAT
#define HELTEC_MAP_FIXED_GPS_LAT 30.936165
#endif
#ifndef HELTEC_MAP_FIXED_GPS_LON
#define HELTEC_MAP_FIXED_GPS_LON 103.471795
#endif
#ifndef HELTEC_MAP_FIXED_A_LAT
#define HELTEC_MAP_FIXED_A_LAT 30.930002418068053
#endif
#ifndef HELTEC_MAP_FIXED_A_LON
#define HELTEC_MAP_FIXED_A_LON 103.47664533302161
#endif
#ifndef HELTEC_MAP_FIXED_B_LAT
#define HELTEC_MAP_FIXED_B_LAT 30.92864194760588
#endif
#ifndef HELTEC_MAP_FIXED_B_LON
#define HELTEC_MAP_FIXED_B_LON 103.47935731242038
#endif

namespace heltec::meshcore::ui::map {

inline bool mapFixedTestEnabled() { return true; }

inline void mapFixedTestOverrideGps(heltec::meshcore::biz::IBizFacade::GpsStatus& gps) {
  gps.available = true;
  gps.enabled = true;
  gps.powered = true;
  gps.fix_valid = true;
  gps.fix_valid_ms = 0;
  gps.lat_deg = HELTEC_MAP_FIXED_GPS_LAT;
  gps.lon_deg = HELTEC_MAP_FIXED_GPS_LON;
  gps.lat_micro = (int32_t)(gps.lat_deg * 1000000.0 + (gps.lat_deg >= 0.0 ? 0.5 : -0.5));
  gps.lon_micro = (int32_t)(gps.lon_deg * 1000000.0 + (gps.lon_deg >= 0.0 ? 0.5 : -0.5));
}

}  // namespace heltec::meshcore::ui::map

#else

namespace heltec::meshcore::ui::map {

inline bool mapFixedTestEnabled() { return false; }

inline void mapFixedTestOverrideGps(heltec::meshcore::biz::IBizFacade::GpsStatus&) {}

}  // namespace heltec::meshcore::ui::map

#endif  // HELTEC_MAP_FIXED_TEST

#endif  // ENV_INCLUDE_MAP
