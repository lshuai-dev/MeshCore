#pragma once

#include <math.h>
#include <stdint.h>

namespace heltec::meshcore::ui::map {

constexpr int kTileSizePx = 256;
constexpr uint8_t kZoomMin = 2;
constexpr uint8_t kZoomMax = 18;
constexpr uint8_t kZoomDefault = 13;
constexpr float kMercatorMaxLatitude = 85.05112878f;

inline uint8_t clampMapZoom(uint8_t zoom) {
  if (zoom < kZoomMin) return kZoomMin;
  if (zoom > kZoomMax) return kZoomMax;
  return zoom;
}

inline float clampMercatorLatitude(float latitude) {
  if (!isfinite(latitude)) return 0.0f;
  if (latitude > kMercatorMaxLatitude) return kMercatorMaxLatitude;
  if (latitude < -kMercatorMaxLatitude) return -kMercatorMaxLatitude;
  return latitude;
}

/** Normalize longitude to the half-open interval [-180, 180). */
inline float normalizeLongitude(float longitude) {
  if (!isfinite(longitude)) return 0.0f;
  longitude = fmodf(longitude + 180.0f, 360.0f);
  if (longitude < 0.0f) longitude += 360.0f;
  return longitude - 180.0f;
}

/** Web Mercator tile coordinate (EPSG:3857), ported from Meshtastic device-ui GeoPoint. */
struct GeoPoint {
  float latitude = 0.f;
  float longitude = 0.f;
  int16_t x_pos = 0;
  int16_t y_pos = 0;
  uint32_t x_tile = 0;
  uint32_t y_tile = 0;
  uint8_t zoom_level = 255;

  GeoPoint() = default;
  GeoPoint(float lat, float lon, uint8_t zoom) : latitude(lat), longitude(lon) { set_zoom(zoom); }

  void set_zoom(uint8_t zoom) {
    zoom = clampMapZoom(zoom);
    latitude = clampMercatorLatitude(latitude);
    longitude = normalizeLongitude(longitude);

    const uint32_t n = 1U << zoom;
    const uint64_t world_px = static_cast<uint64_t>(n) * kTileSizePx;
    const double lat_rad = static_cast<double>(latitude) * (M_PI / 180.0);
    const double x_raw = (static_cast<double>(longitude) + 180.0) / 360.0 * world_px;
    const double y_raw =
        (1.0 - log(tan(lat_rad) + (1.0 / cos(lat_rad))) / M_PI) / 2.0 * world_px;

    // Work in global pixel coordinates first.  This avoids truncation before
    // the tile split and keeps every position inside the valid world bounds.
    const uint64_t max_pixel = world_px - 1U;
    uint64_t global_x = (!isfinite(x_raw) || x_raw <= 0.0)
                            ? 0U
                            : (x_raw >= static_cast<double>(world_px)
                                   ? max_pixel
                                   : static_cast<uint64_t>(x_raw));
    uint64_t global_y = (!isfinite(y_raw) || y_raw <= 0.0)
                            ? 0U
                            : (y_raw >= static_cast<double>(world_px)
                                   ? max_pixel
                                   : static_cast<uint64_t>(y_raw));
    // longitude is normalized, so this is normally already in range.  Keep
    // the modulo here as a guard against floating-point round-off at ±180°.
    global_x %= world_px;
    if (global_y > max_pixel) global_y = max_pixel;

    x_tile = static_cast<uint32_t>(global_x / kTileSizePx);
    y_tile = static_cast<uint32_t>(global_y / kTileSizePx);
    x_pos = static_cast<int16_t>(global_x % kTileSizePx);
    y_pos = static_cast<int16_t>(global_y % kTileSizePx);
    zoom_level = zoom;
  }

  void move(int16_t scroll_x, int16_t scroll_y) {
    if (zoom_level < kZoomMin || zoom_level > kZoomMax) {
      set_zoom(kZoomDefault);
    }

    const uint32_t n = 1U << zoom_level;
    const int64_t world_px = static_cast<int64_t>(n) * kTileSizePx;
    const int64_t current_x = static_cast<int64_t>(x_tile) * kTileSizePx + x_pos;
    const int64_t current_y = static_cast<int64_t>(y_tile) * kTileSizePx + y_pos;

    // A gesture can deliver a delta larger than one tile.  Calculate in world
    // pixels, then split back into tile and in-tile coordinates.  X wraps
    // around the world; Y is clamped at the Web Mercator poles.
    int64_t global_x = (current_x - static_cast<int64_t>(scroll_x)) % world_px;
    if (global_x < 0) global_x += world_px;
    int64_t global_y = current_y - static_cast<int64_t>(scroll_y);
    if (global_y < 0) global_y = 0;
    if (global_y >= world_px) global_y = world_px - 1;

    x_tile = static_cast<uint32_t>(global_x / kTileSizePx);
    y_tile = static_cast<uint32_t>(global_y / kTileSizePx);
    x_pos = static_cast<int16_t>(global_x % kTileSizePx);
    y_pos = static_cast<int16_t>(global_y % kTileSizePx);

    const double x_fraction = static_cast<double>(global_x) / world_px;
    const double y_fraction = static_cast<double>(global_y) / world_px;
    longitude = normalizeLongitude(static_cast<float>(x_fraction * 360.0 - 180.0));
    const double mercator_y = M_PI * (1.0 - 2.0 * y_fraction);
    latitude = clampMercatorLatitude(
        static_cast<float>((180.0 / M_PI) * atan(sinh(mercator_y))));
  }
};

/** Lat/lon at the center of a slippy-map tile (Web Mercator). */
inline void tileCenterLatLon(uint32_t x_tile, uint32_t y_tile, uint8_t zoom, float& lat, float& lon) {
  zoom = clampMapZoom(zoom);
  const uint32_t n = 1U << zoom;
  x_tile %= n;
  if (y_tile >= n) y_tile = n - 1;
  const float xf = (float)x_tile + 0.5f;
  const float yf = (float)y_tile + 0.5f;
  lon = xf / (float)n * 360.f - 180.f;
  const float t = (float)M_PI * (1.0f - 2.0f * yf / (float)n);
  lat = (float)(180.0 / M_PI) * atanf(sinhf(t));
}

inline uint32_t tile_hash(uint32_t x_tile, uint32_t y_tile) {
  return (x_tile << 16) | (y_tile & 0xFFFFU);
}

/** Web Mercator ground resolution (meters per pixel) at latitude and zoom. */
inline float metersPerPixel(float lat_deg, uint8_t zoom) {
  const float lat_rad = clampMercatorLatitude(lat_deg) * (float)(M_PI / 180.0);
  return (float)(156543.03392 * cosf(lat_rad) / (double)(1U << clampMapZoom(zoom)));
}

}  // namespace heltec::meshcore::ui::map
