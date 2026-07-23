#if defined(ENV_INCLUDE_MAP) && ENV_INCLUDE_MAP

#include "map_prefs.hpp"

#include <SPIFFS.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

namespace heltec::meshcore::ui::map {
namespace {

constexpr const char* kMapPrefsPath = "/map_ui_prefs";

// The first implementation wrote MapUiPrefs directly.  Keep decoding that
// 28-byte layout so an upgrade does not discard the user's last map centre.
constexpr size_t kLegacyPrefsSize = 28;

constexpr uint32_t kPrefsMagic = 0x3156504dU;  // "MPV1" in little endian.
constexpr uint16_t kPrefsVersion = 1;

// This is deliberately packed: it is a file format, not a C++ object layout.
struct __attribute__((packed)) MapPrefsFileV1 {
  uint32_t magic;
  uint16_t version;
  uint16_t bytes;
  float home_lat;
  float home_lon;
  uint8_t zoom;
  char tile_style[16];
};

static_assert(sizeof(MapPrefsFileV1) == 33, "map preference file layout changed");

static bool validStyle(const char* style, size_t capacity) {
  if (!style || capacity == 0 || style[0] == '\0') return false;
  bool terminated = false;
  size_t length = 0;
  for (size_t i = 0; i < capacity; ++i) {
    const unsigned char c = static_cast<unsigned char>(style[i]);
    if (c == '\0') {
      terminated = true;
      length = i;
      break;
    }
    // Style names are used as path components.  Keep the persisted value
    // restricted to a portable, single path component.
    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
          (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.')) {
      return false;
    }
  }
  if (!terminated) return false;
  return !(length == 1 && style[0] == '.') &&
         !(length == 2 && style[0] == '.' && style[1] == '.');
}

static bool validPrefs(const MapUiPrefs& prefs) {
  if (!isfinite(prefs.home_lat) || !isfinite(prefs.home_lon)) return false;
  if (prefs.home_lat < -90.0f || prefs.home_lat > 90.0f) return false;
  if (prefs.home_lon < -180.0f || prefs.home_lon > 180.0f) return false;
  if (prefs.home_lat == 0.0f && prefs.home_lon == 0.0f) return false;
  if (prefs.zoom < kZoomMin || prefs.zoom > kZoomMax) return false;
  return validStyle(prefs.tile_style, sizeof(prefs.tile_style));
}

static void normalizePrefs(MapUiPrefs& prefs) {
  if (isfinite(prefs.home_lat) && isfinite(prefs.home_lon) &&
      prefs.home_lat == 0.0f && prefs.home_lon == 0.0f) {
    prefs.home_lat = 51.5074f;
    prefs.home_lon = -0.1278f;
  }
  // A pole cannot be represented by Web Mercator.  Keep persisted centres
  // inside the projection's finite latitude range.
  constexpr float kMercatorLatitude = 85.05112878f;
  if (!isfinite(prefs.home_lat)) prefs.home_lat = 51.5074f;
  if (prefs.home_lat > kMercatorLatitude) prefs.home_lat = kMercatorLatitude;
  if (prefs.home_lat < -kMercatorLatitude) prefs.home_lat = -kMercatorLatitude;

  if (!isfinite(prefs.home_lon)) {
    prefs.home_lon = -0.1278f;
  } else {
    prefs.home_lon = fmodf(prefs.home_lon + 180.0f, 360.0f);
    if (prefs.home_lon < 0.0f) prefs.home_lon += 360.0f;
    prefs.home_lon -= 180.0f;
  }

  if (prefs.zoom < kZoomMin || prefs.zoom > kZoomMax) prefs.zoom = kZoomDefault;
  if (!validStyle(prefs.tile_style, sizeof(prefs.tile_style))) {
    strncpy(prefs.tile_style, "osm", sizeof(prefs.tile_style));
    prefs.tile_style[sizeof(prefs.tile_style) - 1] = '\0';
  }
}

static bool decodeLegacy(const uint8_t* raw, size_t size, MapUiPrefs& out) {
  if (!raw || size != kLegacyPrefsSize) return false;
  MapUiPrefs candidate{};
  // Original layout: float lat @ 0, float lon @ 4, zoom @ 8,
  // one legacy runtime flag @ 9, style[16] @ 10, two bytes tail padding.
  memcpy(&candidate.home_lat, raw, sizeof(candidate.home_lat));
  memcpy(&candidate.home_lon, raw + 4, sizeof(candidate.home_lon));
  candidate.zoom = raw[8];
  memcpy(candidate.tile_style, raw + 10, sizeof(candidate.tile_style));
  if (!validPrefs(candidate)) return false;
  normalizePrefs(candidate);
  out = candidate;
  return true;
}

static bool decodeCurrent(const uint8_t* raw, size_t size, MapUiPrefs& out) {
  if (!raw || size != sizeof(MapPrefsFileV1)) return false;
  MapPrefsFileV1 file{};
  memcpy(&file, raw, sizeof(file));
  if (file.magic != kPrefsMagic || file.version != kPrefsVersion ||
      file.bytes != sizeof(MapPrefsFileV1)) {
    return false;
  }
  MapUiPrefs candidate{};
  candidate.home_lat = file.home_lat;
  candidate.home_lon = file.home_lon;
  candidate.zoom = file.zoom;
  memcpy(candidate.tile_style, file.tile_style, sizeof(candidate.tile_style));
  if (!validPrefs(candidate)) return false;
  normalizePrefs(candidate);
  out = candidate;
  return true;
}

}  // namespace

bool map_prefs_load(MapUiPrefs& out) {
#if defined(ESP32)
  if (SPIFFS.totalBytes() == 0) return false;
#endif
  if (!SPIFFS.exists(kMapPrefsPath)) return false;
  File f = SPIFFS.open(kMapPrefsPath, "r");
  if (!f) return false;
  const size_t file_size = static_cast<size_t>(f.size());
  const size_t read_size = file_size > sizeof(MapPrefsFileV1) ? sizeof(MapPrefsFileV1) : file_size;
  uint8_t raw[sizeof(MapPrefsFileV1)] = {};
  const size_t n = f.read(raw, read_size);
  f.close();
  if (n != read_size) return false;

  MapUiPrefs decoded{};
  if (decodeCurrent(raw, file_size, decoded)) {
    out = decoded;
    return true;
  }
  if (decodeLegacy(raw, file_size, decoded)) {
    out = decoded;
    // Migrate immediately so subsequent boots use the validated format.  A
    // failed migration is harmless; the decoded preferences are still valid.
    map_prefs_save(out);
    return true;
  }
  return false;
}

void map_prefs_save(const MapUiPrefs& prefs) {
  MapUiPrefs normalized = prefs;
  normalizePrefs(normalized);

  MapPrefsFileV1 file{};
  file.magic = kPrefsMagic;
  file.version = kPrefsVersion;
  file.bytes = sizeof(file);
  file.home_lat = normalized.home_lat;
  file.home_lon = normalized.home_lon;
  file.zoom = normalized.zoom;
  memcpy(file.tile_style, normalized.tile_style, sizeof(file.tile_style));

  File f = SPIFFS.open(kMapPrefsPath, "w");
  if (!f) return;
  f.write((const uint8_t*)&file, sizeof(file));
  f.close();
}

}  // namespace heltec::meshcore::ui::map

#endif
