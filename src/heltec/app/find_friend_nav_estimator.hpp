#pragma once

#include <stddef.h>
#include <stdint.h>

namespace heltec::meshcore::nav {

enum class TargetFreshness : uint8_t {
  NotApplicable = 0,
  Unknown,
  Fresh,
  Aging,
  Stale,
};

enum class Confidence : uint8_t {
  Unavailable = 0,
  Low,
  Medium,
  High,
};

struct FindFriendNavInput {
  uint32_t now_ms = 0;

  bool gps_valid = false;
  double gps_lat_deg = 0.0;
  double gps_lon_deg = 0.0;
  uint32_t gps_age_ms = 0;
  uint8_t gps_satellites = 0;

  bool target_valid = false;
  double target_lat_deg = 0.0;
  double target_lon_deg = 0.0;
  bool target_is_contact = false;
  bool target_age_known = false;
  uint32_t target_age_ms = 0;

  bool heading_valid = false;
  float heading_deg = 0.0f;
  uint8_t compass_quality = 0;
  bool declination_configured = true;
};

struct FindFriendNavOutput {
  bool gps_valid = false;
  double filtered_lat_deg = 0.0;
  double filtered_lon_deg = 0.0;
  float estimated_accuracy_m = 0.0f;
  bool gps_low_accuracy = false;

  bool target_usable = false;
  TargetFreshness target_freshness = TargetFreshness::NotApplicable;

  bool heading_valid = false;
  float filtered_heading_deg = 0.0f;

  bool bearing_valid = false;
  float bearing_deg = 0.0f;
  bool relative_valid = false;
  float turn_deg = 0.0f;
  double distance_m = -1.0;
  bool near_target = false;
  float nearby_enter_m = 15.0f;
  Confidence confidence = Confidence::Unavailable;
};

class FindFriendNavEstimator {
 public:
  static constexpr uint32_t kTargetFreshMs = 2U * 60U * 1000U;
  static constexpr uint32_t kTargetStaleMs = 15U * 60U * 1000U;

  void reset();
  FindFriendNavOutput update(const FindFriendNavInput& input);

  static bool coordinateValid(double lat_deg, double lon_deg);
  static float normalizeTurn180(float deg);
  static float wrapHeading360(float deg);
  static TargetFreshness classifyTargetFreshness(bool is_contact, bool age_known,
                                                  uint32_t age_ms);

 private:
  static constexpr size_t kGpsWindowSize = 5;

  struct GpsSample {
    double lat_deg = 0.0;
    double lon_deg = 0.0;
    uint32_t source_ms = 0;
  };

  void addGpsSample(double lat_deg, double lon_deg, uint32_t source_ms);
  void filteredGps(double& lat_deg, double& lon_deg, float& scatter_m) const;
  void updateHeading(float heading_deg, uint8_t quality);

  GpsSample _gps_samples[kGpsWindowSize]{};
  size_t _gps_count = 0;
  size_t _gps_next = 0;
  bool _gps_source_seen = false;
  uint32_t _last_gps_source_ms = 0;
  double _last_gps_lat_deg = 0.0;
  double _last_gps_lon_deg = 0.0;

  bool _heading_ready = false;
  float _heading_x = 1.0f;
  float _heading_y = 0.0f;
  bool _near_target = false;
};

}  // namespace heltec::meshcore::nav
