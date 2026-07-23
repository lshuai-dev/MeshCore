#include "map_geo.hpp"

#include <math.h>

namespace heltec::meshcore::geo {

float lat_long_to_meters(double lat_a, double lon_a, double lat_b, double lon_b) {
  if (lat_a == lat_b && lon_a == lon_b) return 0.f;

  const double a1 = lat_a * kDeg2Rad;
  const double a2 = lon_a * kDeg2Rad;
  const double b1 = lat_b * kDeg2Rad;
  const double b2 = lon_b * kDeg2Rad;
  const double cos_b1 = cos(b1);
  const double cos_a1 = cos(a1);
  const double t1 = cos_a1 * cos(a2) * cos_b1 * cos(b2);
  const double t2 = cos_a1 * sin(a2) * cos_b1 * sin(b2);
  const double t3 = sin(a1) * sin(b1);
  double tt = acos(t1 + t2 + t3);
  if (isnan(tt)) tt = 0.0;
  return (float)(6366000.0 * tt);
}

float bearing_rad(double lat1, double lon1, double lat2, double lon2) {
  const double lat1_rad = lat1 * kDeg2Rad;
  const double lat2_rad = lat2 * kDeg2Rad;
  const double delta_lon_rad = (lon2 - lon1) * kDeg2Rad;
  const double y = sin(delta_lon_rad) * cos(lat2_rad);
  const double x = cos(lat1_rad) * sin(lat2_rad) - sin(lat1_rad) * cos(lat2_rad) * cos(delta_lon_rad);
  return (float)atan2(y, x);
}

void offset_from_center(double center_lat, double center_lon, double lat, double lon,
                        float& east_m, float& north_m) {
  const float dist_m = lat_long_to_meters(center_lat, center_lon, lat, lon);
  const float bearing = bearing_rad(center_lat, center_lon, lat, lon);
  north_m = cosf(bearing) * dist_m;
  east_m = sinf(bearing) * dist_m;
}

}  // namespace heltec::meshcore::geo
