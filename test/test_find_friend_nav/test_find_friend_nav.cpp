#include <gtest/gtest.h>

#include "heltec/app/find_friend_nav_estimator.hpp"

#include <cmath>

using heltec::meshcore::nav::Confidence;
using heltec::meshcore::nav::FindFriendNavEstimator;
using heltec::meshcore::nav::FindFriendNavInput;
using heltec::meshcore::nav::TargetFreshness;

namespace {

FindFriendNavInput baseInput() {
  FindFriendNavInput input{};
  input.now_ms = 1000;
  input.gps_valid = true;
  input.gps_lat_deg = 31.2304;
  input.gps_lon_deg = 121.4737;
  input.gps_satellites = 9;
  input.target_valid = true;
  input.target_lat_deg = 31.2310;
  input.target_lon_deg = 121.4737;
  input.heading_valid = true;
  input.heading_deg = 0.0f;
  input.compass_quality = 3;
  return input;
}

}  // namespace

TEST(FindFriendNav, RejectsInvalidCoordinates) {
  EXPECT_TRUE(FindFriendNavEstimator::coordinateValid(0.0, 0.0));
  EXPECT_FALSE(FindFriendNavEstimator::coordinateValid(91.0, 0.0));
  EXPECT_FALSE(FindFriendNavEstimator::coordinateValid(0.0, -181.0));
  EXPECT_FALSE(FindFriendNavEstimator::coordinateValid(NAN, 0.0));
}

TEST(FindFriendNav, ClassifiesContactAge) {
  EXPECT_EQ(TargetFreshness::NotApplicable,
            FindFriendNavEstimator::classifyTargetFreshness(false, false, 0));
  EXPECT_EQ(TargetFreshness::Unknown,
            FindFriendNavEstimator::classifyTargetFreshness(true, false, 0));
  EXPECT_EQ(TargetFreshness::Fresh,
            FindFriendNavEstimator::classifyTargetFreshness(
                true, true, FindFriendNavEstimator::kTargetFreshMs));
  EXPECT_EQ(TargetFreshness::Aging,
            FindFriendNavEstimator::classifyTargetFreshness(
                true, true, FindFriendNavEstimator::kTargetFreshMs + 1));
  EXPECT_EQ(TargetFreshness::Stale,
            FindFriendNavEstimator::classifyTargetFreshness(
                true, true, FindFriendNavEstimator::kTargetStaleMs + 1));
}

TEST(FindFriendNav, MedianFilterRejectsOneGpsOutlier) {
  FindFriendNavEstimator estimator;
  FindFriendNavInput input = baseInput();
  const double samples[] = {31.230400, 31.230410, 35.0, 31.230390, 31.230420};
  heltec::meshcore::nav::FindFriendNavOutput output{};
  for (size_t i = 0; i < 5; ++i) {
    input.now_ms += 1000;
    input.gps_lat_deg = samples[i];
    output = estimator.update(input);
  }
  EXPECT_NEAR(31.230410, output.filtered_lat_deg, 0.000001);
  EXPECT_FALSE(output.gps_low_accuracy);
}

TEST(FindFriendNav, CircularHeadingFilterCrossesNorth) {
  FindFriendNavEstimator estimator;
  FindFriendNavInput input = baseInput();
  input.heading_deg = 359.0f;
  estimator.update(input);
  input.now_ms += 200;
  input.heading_deg = 1.0f;
  const auto output = estimator.update(input);
  EXPECT_TRUE(output.heading_valid);
  EXPECT_TRUE(output.filtered_heading_deg < 5.0f || output.filtered_heading_deg > 355.0f);
}

TEST(FindFriendNav, StaleContactCannotDriveBearing) {
  FindFriendNavEstimator estimator;
  FindFriendNavInput input = baseInput();
  input.target_is_contact = true;
  input.target_age_known = true;
  input.target_age_ms = FindFriendNavEstimator::kTargetStaleMs + 1;
  const auto output = estimator.update(input);
  EXPECT_EQ(TargetFreshness::Stale, output.target_freshness);
  EXPECT_FALSE(output.target_usable);
  EXPECT_FALSE(output.bearing_valid);
}

TEST(FindFriendNav, QualityTwoKeepsDirectionAtLowConfidence) {
  FindFriendNavEstimator estimator;
  FindFriendNavInput input = baseInput();
  input.compass_quality = 2;
  const auto output = estimator.update(input);
  EXPECT_TRUE(output.relative_valid);
  EXPECT_EQ(Confidence::Low, output.confidence);
}

TEST(FindFriendNav, MissingDeclinationConfigurationLowersConfidence) {
  FindFriendNavEstimator estimator;
  FindFriendNavInput input = baseInput();
  input.declination_configured = false;
  const auto output = estimator.update(input);
  EXPECT_TRUE(output.relative_valid);
  EXPECT_EQ(Confidence::Low, output.confidence);
}

TEST(FindFriendNav, NearbyStateUsesExitHysteresis) {
  FindFriendNavEstimator estimator;
  FindFriendNavInput input = baseInput();
  input.gps_lat_deg = 0.0;
  input.gps_lon_deg = 0.0;
  input.target_lat_deg = 0.00025;
  input.target_lon_deg = 0.0;
  auto output = estimator.update(input);
  ASSERT_TRUE(output.near_target);

  input.now_ms += 1000;
  input.target_lat_deg = 0.00032;
  output = estimator.update(input);
  EXPECT_TRUE(output.near_target);

  input.now_ms += 1000;
  input.target_lat_deg = 0.00038;
  output = estimator.update(input);
  EXPECT_FALSE(output.near_target);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
