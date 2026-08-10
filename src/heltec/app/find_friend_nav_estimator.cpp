#include "find_friend_nav_estimator.hpp"

#include "geodesic.hpp"

#include <math.h>

namespace heltec::meshcore::nav {
namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr float kMinimumNearbyM = 15.0f;
constexpr float kNearbyHysteresisM = 5.0f;
constexpr float kAssumedTargetErrorM = 10.0f;

template <typename T>
void sortValues(T* values, size_t count) {
  for (size_t i = 1; i < count; ++i) {
    const T value = values[i];
    size_t j = i;
    while (j > 0 && values[j - 1] > value) {
      values[j] = values[j - 1];
      --j;
    }
    values[j] = value;
  }
}

double wrapLongitudeDelta(double deg) {
  while (deg > 180.0) deg -= 360.0;
  while (deg < -180.0) deg += 360.0;
  return deg;
}

float satelliteBaseErrorM(uint8_t satellites) {
  if (satellites == 0) return 35.0f;
  if (satellites <= 3) return 60.0f;
  if (satellites == 4) return 40.0f;
  if (satellites == 5) return 25.0f;
  if (satellites <= 7) return 15.0f;
  return 8.0f;
}

}  // namespace

void FindFriendNavEstimator::reset() {
  _gps_count = 0;
  _gps_next = 0;
  _gps_source_seen = false;
  _last_gps_source_ms = 0;
  _last_gps_lat_deg = 0.0;
  _last_gps_lon_deg = 0.0;
  _heading_ready = false;
  _heading_x = 1.0f;
  _heading_y = 0.0f;
  _near_target = false;
}

bool FindFriendNavEstimator::coordinateValid(double lat_deg, double lon_deg) {
  return isfinite(lat_deg) && isfinite(lon_deg) && lat_deg >= -90.0 && lat_deg <= 90.0 &&
         lon_deg >= -180.0 && lon_deg <= 180.0;
}

float FindFriendNavEstimator::wrapHeading360(float deg) {
  if (!isfinite(deg)) return 0.0f;
  deg = fmodf(deg, 360.0f);
  if (deg < 0.0f) deg += 360.0f;
  return deg;
}

float FindFriendNavEstimator::normalizeTurn180(float deg) {
  if (!isfinite(deg)) return 0.0f;
  return fmodf(deg + 540.0f, 360.0f) - 180.0f;
}

TargetFreshness FindFriendNavEstimator::classifyTargetFreshness(bool is_contact, bool age_known,
                                                                 uint32_t age_ms) {
  if (!is_contact) return TargetFreshness::NotApplicable;
  if (!age_known) return TargetFreshness::Unknown;
  if (age_ms <= kTargetFreshMs) return TargetFreshness::Fresh;
  if (age_ms <= kTargetStaleMs) return TargetFreshness::Aging;
  return TargetFreshness::Stale;
}

void FindFriendNavEstimator::addGpsSample(double lat_deg, double lon_deg, uint32_t source_ms) {
  GpsSample& sample = _gps_samples[_gps_next];
  sample.lat_deg = lat_deg;
  sample.lon_deg = lon_deg;
  sample.source_ms = source_ms;
  _gps_next = (_gps_next + 1U) % kGpsWindowSize;
  if (_gps_count < kGpsWindowSize) ++_gps_count;
}

void FindFriendNavEstimator::filteredGps(double& lat_deg, double& lon_deg, float& scatter_m) const {
  if (_gps_count == 0) {
    lat_deg = 0.0;
    lon_deg = 0.0;
    scatter_m = 0.0f;
    return;
  }

  double latitudes[kGpsWindowSize];
  double longitude_deltas[kGpsWindowSize];
  const double lon_reference = _gps_samples[0].lon_deg;
  for (size_t i = 0; i < _gps_count; ++i) {
    latitudes[i] = _gps_samples[i].lat_deg;
    longitude_deltas[i] = wrapLongitudeDelta(_gps_samples[i].lon_deg - lon_reference);
  }
  sortValues(latitudes, _gps_count);
  sortValues(longitude_deltas, _gps_count);
  lat_deg = latitudes[_gps_count / 2U];
  lon_deg = lon_reference + longitude_deltas[_gps_count / 2U];
  if (lon_deg > 180.0) lon_deg -= 360.0;
  if (lon_deg < -180.0) lon_deg += 360.0;

  double deviations[kGpsWindowSize];
  for (size_t i = 0; i < _gps_count; ++i) {
    deviations[i] = geo::geodesic_distance_m(lat_deg, lon_deg, _gps_samples[i].lat_deg,
                                             _gps_samples[i].lon_deg);
  }
  sortValues(deviations, _gps_count);
  const size_t robust_high_index = (_gps_count * 3U) / 4U;
  scatter_m = static_cast<float>(deviations[robust_high_index]);
}

void FindFriendNavEstimator::updateHeading(float heading_deg, uint8_t quality) {
  const float radians = wrapHeading360(heading_deg) * static_cast<float>(kPi / 180.0);
  const float x = cosf(radians);
  const float y = sinf(radians);
  if (!_heading_ready) {
    _heading_x = x;
    _heading_y = y;
    _heading_ready = true;
    return;
  }

  const float alpha = quality >= 3 ? 0.25f : 0.12f;
  _heading_x = (1.0f - alpha) * _heading_x + alpha * x;
  _heading_y = (1.0f - alpha) * _heading_y + alpha * y;
  const float norm = sqrtf(_heading_x * _heading_x + _heading_y * _heading_y);
  if (norm > 0.0001f) {
    _heading_x /= norm;
    _heading_y /= norm;
  }
}

FindFriendNavOutput FindFriendNavEstimator::update(const FindFriendNavInput& input) {
  FindFriendNavOutput out{};
  out.target_freshness = classifyTargetFreshness(input.target_is_contact,
                                                  input.target_age_known,
                                                  input.target_age_ms);

  const bool gps_coordinate_valid =
      input.gps_valid && coordinateValid(input.gps_lat_deg, input.gps_lon_deg);
  if (gps_coordinate_valid) {
    const uint32_t source_ms = input.now_ms - input.gps_age_ms;
    const bool coordinate_changed = input.gps_lat_deg != _last_gps_lat_deg ||
                                    input.gps_lon_deg != _last_gps_lon_deg;
    const bool source_advanced = !_gps_source_seen ||
                                 (int32_t)(source_ms - _last_gps_source_ms) >= 100;
    if (coordinate_changed || source_advanced) {
      addGpsSample(input.gps_lat_deg, input.gps_lon_deg, source_ms);
      _gps_source_seen = true;
      _last_gps_source_ms = source_ms;
      _last_gps_lat_deg = input.gps_lat_deg;
      _last_gps_lon_deg = input.gps_lon_deg;
    }
  }

  if (input.heading_valid && input.compass_quality >= 2 && isfinite(input.heading_deg)) {
    updateHeading(input.heading_deg, input.compass_quality);
  }

  out.gps_valid = gps_coordinate_valid && _gps_count > 0;
  if (out.gps_valid) {
    float scatter_m = 0.0f;
    filteredGps(out.filtered_lat_deg, out.filtered_lon_deg, scatter_m);
    const float base_error_m = satelliteBaseErrorM(input.gps_satellites);
    out.estimated_accuracy_m = fmaxf(base_error_m, scatter_m * 1.5f);
    out.gps_low_accuracy = input.gps_satellites < 5 || out.estimated_accuracy_m > 30.0f;
  }

  out.heading_valid = input.heading_valid && input.compass_quality >= 2 && _heading_ready;
  if (out.heading_valid) {
    out.filtered_heading_deg =
        wrapHeading360(atan2f(_heading_y, _heading_x) * static_cast<float>(180.0 / kPi));
  }

  const bool target_coordinate_valid =
      input.target_valid && coordinateValid(input.target_lat_deg, input.target_lon_deg);
  out.target_usable = target_coordinate_valid &&
      out.target_freshness != TargetFreshness::Unknown &&
      out.target_freshness != TargetFreshness::Stale;
  if (!input.target_is_contact && target_coordinate_valid) out.target_usable = true;

  if (!out.gps_valid || !out.target_usable) {
    _near_target = false;
    return out;
  }

  double bearing_deg = 0.0;
  geo::geodesic_inverse_wgs84(out.filtered_lat_deg, out.filtered_lon_deg,
                              input.target_lat_deg, input.target_lon_deg,
                              out.distance_m, &bearing_deg);

  const float combined_error_m =
      sqrtf(out.estimated_accuracy_m * out.estimated_accuracy_m +
            kAssumedTargetErrorM * kAssumedTargetErrorM);
  out.nearby_enter_m = fmaxf(kMinimumNearbyM, combined_error_m * 2.5f);
  const float nearby_exit_m = out.nearby_enter_m + kNearbyHysteresisM;
  if (_near_target) {
    if (out.distance_m > nearby_exit_m) _near_target = false;
  } else if (out.distance_m <= out.nearby_enter_m) {
    _near_target = true;
  }

  out.near_target = _near_target;
  out.bearing_valid = true;
  out.bearing_deg = static_cast<float>(bearing_deg);
  if (out.heading_valid && !out.near_target) {
    out.turn_deg = normalizeTurn180(out.bearing_deg - out.filtered_heading_deg);
    out.relative_valid = true;
  }

  const bool target_aging = out.target_freshness == TargetFreshness::Aging;
  if (out.gps_low_accuracy || !out.heading_valid || input.compass_quality == 2 || target_aging ||
      !input.declination_configured) {
    out.confidence = Confidence::Low;
  } else if (input.gps_satellites >= 8 && out.estimated_accuracy_m <= 10.0f &&
             input.compass_quality >= 3) {
    out.confidence = Confidence::High;
  } else {
    out.confidence = Confidence::Medium;
  }
  return out;
}

}  // namespace heltec::meshcore::nav
