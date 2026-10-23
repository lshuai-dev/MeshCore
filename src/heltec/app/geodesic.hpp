#pragma once

/* Select at compile time, so compact builds do not retain Vincenty's solver. */
#ifndef HELTEC_GEO_COMPACT
#define HELTEC_GEO_COMPACT 0
#endif

#if HELTEC_GEO_COMPACT != 0 && HELTEC_GEO_COMPACT != 1
#error "HELTEC_GEO_COMPACT must be 0 or 1"
#endif

namespace Heltec::MeshCore::Geo
{

/**
 * Inverse distance/bearing for WGS-84 coordinates (legacy API name).
 *
 * Distance is returned in meters. When requested, initial bearing is returned
 * in degrees [0, 360); pass nullptr for distance-only callers.
 * Vincenty's ellipsoidal solution is used normally, with a spherical fallback
 * for the near-antipodal cases where the iteration does not converge.
 * HELTEC_GEO_COMPACT=1 instead uses a mean-radius sphere for every call:
 * this is a size/speed tradeoff, not an ellipsoidal-accuracy result.
 */
void geodesic_inverse_wgs84(double lat1_deg, double lon1_deg, double lat2_deg, double lon2_deg,
                            double &distance_m, double *initial_bearing_deg = nullptr);

/** Distance in meters using the selected compile-time model. */
double geodesic_distance_m(double lat1_deg, double lon1_deg, double lat2_deg, double lon2_deg);

} // namespace Heltec::MeshCore::Geo
