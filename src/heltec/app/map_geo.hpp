#pragma once

#include <stdint.h>

namespace heltec::meshcore::geo {

constexpr double kPi = 3.14159265358979323846;
constexpr double kDeg2Rad = kPi / 180.0;
constexpr double kRad2Deg = 180.0 / kPi;

inline double microdeg_to_deg(int32_t micro) { return (double)micro * 1.0e-6; }

inline bool has_valid_gps_micro(int32_t lat, int32_t lon) { return lat != 0 || lon != 0; }

inline bool has_valid_gps_deg(double lat, double lon) { return lat != 0.0 || lon != 0.0; }

/** Great-circle distance in meters (Haversine-style, MeshTastic GeoCoord). */
float lat_long_to_meters(double lat_a, double lon_a, double lat_b, double lon_b);

/** Initial bearing from point 1 to point 2 in radians; 0 = north. */
float bearing_rad(double lat1, double lon1, double lat2, double lon2);

void offset_from_center(double center_lat, double center_lon, double lat, double lon,
                        float& east_m, float& north_m);

}  // namespace heltec::meshcore::geo
