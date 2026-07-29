#include "geodesic.hpp"

#include <math.h>

namespace heltec::meshcore::geo {
namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kDeg2Rad = kPi / 180.0;
constexpr double kRad2Deg = 180.0 / kPi;
constexpr double kWgs84A = 6378137.0;
constexpr double kWgs84F = 1.0 / 298.257223563;
constexpr double kWgs84B = (1.0 - kWgs84F) * kWgs84A;

double wrap_degrees_360(double degrees) {
  degrees = fmod(degrees, 360.0);
  if (degrees < 0.0) degrees += 360.0;
  return degrees;
}

double normalize_longitude_delta_rad(double radians) {
  radians = fmod(radians + kPi, 2.0 * kPi);
  if (radians < 0.0) radians += 2.0 * kPi;
  return radians - kPi;
}

void geodesic_inverse_spherical(double lat1_deg, double lon1_deg,
                                double lat2_deg, double lon2_deg,
                                double& distance_m, double* bearing_deg) {
  const double phi1 = lat1_deg * kDeg2Rad;
  const double phi2 = lat2_deg * kDeg2Rad;
  const double dphi = (lat2_deg - lat1_deg) * kDeg2Rad;
  const double dlambda = normalize_longitude_delta_rad((lon2_deg - lon1_deg) * kDeg2Rad);
  const double sin_half_dphi = sin(dphi * 0.5);
  const double sin_half_dlambda = sin(dlambda * 0.5);
  double a = sin_half_dphi * sin_half_dphi +
             cos(phi1) * cos(phi2) * sin_half_dlambda * sin_half_dlambda;
  if (a < 0.0) a = 0.0;
  if (a > 1.0) a = 1.0;
  distance_m = kWgs84A * 2.0 * atan2(sqrt(a), sqrt(1.0 - a));

  if (bearing_deg) {
    const double y = sin(dlambda) * cos(phi2);
    const double x = cos(phi1) * sin(phi2) - sin(phi1) * cos(phi2) * cos(dlambda);
    *bearing_deg = wrap_degrees_360(atan2(y, x) * kRad2Deg);
  }
}

}  // namespace

void geodesic_inverse_wgs84(double lat1_deg, double lon1_deg,
                            double lat2_deg, double lon2_deg,
                            double& distance_m, double* initial_bearing_deg) {
  if (lat1_deg == lat2_deg && lon1_deg == lon2_deg) {
    distance_m = 0.0;
    if (initial_bearing_deg) *initial_bearing_deg = 0.0;
    return;
  }

  const double phi1 = lat1_deg * kDeg2Rad;
  const double phi2 = lat2_deg * kDeg2Rad;
  const double longitude_delta =
      normalize_longitude_delta_rad((lon2_deg - lon1_deg) * kDeg2Rad);

  const double u1 = atan((1.0 - kWgs84F) * tan(phi1));
  const double u2 = atan((1.0 - kWgs84F) * tan(phi2));
  const double sin_u1 = sin(u1);
  const double cos_u1 = cos(u1);
  const double sin_u2 = sin(u2);
  const double cos_u2 = cos(u2);

  double lambda = longitude_delta;
  double sin_sigma = 0.0;
  double cos_sigma = 0.0;
  double sigma = 0.0;
  double sin_alpha = 0.0;
  double cos_sq_alpha = 0.0;
  double cos_2sigma_m = 0.0;
  bool converged = false;

  for (int iteration = 0; iteration < 200; ++iteration) {
    const double sin_lambda = sin(lambda);
    const double cos_lambda = cos(lambda);
    const double t1 = cos_u2 * sin_lambda;
    const double t2 = cos_u1 * sin_u2 - sin_u1 * cos_u2 * cos_lambda;
    sin_sigma = sqrt(t1 * t1 + t2 * t2);
    if (sin_sigma == 0.0) {
      distance_m = 0.0;
      if (initial_bearing_deg) *initial_bearing_deg = 0.0;
      return;
    }

    cos_sigma = sin_u1 * sin_u2 + cos_u1 * cos_u2 * cos_lambda;
    sigma = atan2(sin_sigma, cos_sigma);
    sin_alpha = cos_u1 * cos_u2 * sin_lambda / sin_sigma;
    cos_sq_alpha = 1.0 - sin_alpha * sin_alpha;
    cos_2sigma_m = cos_sq_alpha != 0.0
                       ? cos_sigma - 2.0 * sin_u1 * sin_u2 / cos_sq_alpha
                       : 0.0;
    const double c = kWgs84F / 16.0 * cos_sq_alpha *
                     (4.0 + kWgs84F * (4.0 - 3.0 * cos_sq_alpha));
    const double next_lambda =
        longitude_delta +
        (1.0 - c) * kWgs84F * sin_alpha *
            (sigma + c * sin_sigma *
                         (cos_2sigma_m + c * cos_sigma *
                                              (-1.0 + 2.0 * cos_2sigma_m * cos_2sigma_m)));
    if (fabs(next_lambda - lambda) < 1.0e-12) {
      lambda = next_lambda;
      converged = true;
      break;
    }
    lambda = next_lambda;
  }

  if (!converged) {
    geodesic_inverse_spherical(lat1_deg, lon1_deg, lat2_deg, lon2_deg,
                               distance_m, initial_bearing_deg);
    return;
  }

  const double u_sq = cos_sq_alpha *
                      (kWgs84A * kWgs84A - kWgs84B * kWgs84B) /
                      (kWgs84B * kWgs84B);
  const double a =
      1.0 + u_sq / 16384.0 *
                (4096.0 + u_sq * (-768.0 + u_sq * (320.0 - 175.0 * u_sq)));
  const double b =
      u_sq / 1024.0 *
      (256.0 + u_sq * (-128.0 + u_sq * (74.0 - 47.0 * u_sq)));
  const double delta_sigma =
      b * sin_sigma *
      (cos_2sigma_m +
       b / 4.0 *
           (cos_sigma * (-1.0 + 2.0 * cos_2sigma_m * cos_2sigma_m) -
            b / 6.0 * cos_2sigma_m * (-3.0 + 4.0 * sin_sigma * sin_sigma) *
                (-3.0 + 4.0 * cos_2sigma_m * cos_2sigma_m)));
  distance_m = kWgs84B * a * (sigma - delta_sigma);

  if (initial_bearing_deg) {
    *initial_bearing_deg = wrap_degrees_360(
        atan2(cos_u2 * sin(lambda),
              cos_u1 * sin_u2 - sin_u1 * cos_u2 * cos(lambda)) *
        kRad2Deg);
  }
}

double geodesic_distance_m(double lat1_deg, double lon1_deg,
                           double lat2_deg, double lon2_deg) {
  double distance_m = 0.0;
  geodesic_inverse_wgs84(lat1_deg, lon1_deg, lat2_deg, lon2_deg,
                         distance_m);
  return distance_m;
}

}  // namespace heltec::meshcore::geo
