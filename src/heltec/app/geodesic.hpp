#pragma once

namespace heltec::meshcore::geo {

/**
 * Solve the WGS-84 inverse geodesic problem.
 *
 * Distance is returned in meters. When requested, initial bearing is returned
 * in degrees [0, 360); pass nullptr for distance-only callers.
 * Vincenty's ellipsoidal solution is used normally, with a spherical fallback
 * for the near-antipodal cases where the iteration does not converge.
 */
void geodesic_inverse_wgs84(double lat1_deg, double lon1_deg,
                            double lat2_deg, double lon2_deg,
                            double& distance_m,
                            double* initial_bearing_deg = nullptr);

/** WGS-84 geodesic distance in meters. */
double geodesic_distance_m(double lat1_deg, double lon1_deg,
                           double lat2_deg, double lon2_deg);

}  // namespace heltec::meshcore::geo
