#pragma once

#if defined(MESH_DEBUG) && MESH_DEBUG
#include <Arduino.h>
#include <stdarg.h>

#ifndef MAP_UI_PERF_THRESHOLD_US
/** Log [map][perf] when a scoped section exceeds this (microseconds). */
#define MAP_UI_PERF_THRESHOLD_US 500
#endif

#define MAP_UI_LOG(fmt, ...) \
  do { \
    Serial.printf("[map] " fmt "\n", ##__VA_ARGS__); \
    Serial.flush(); \
  } while (0)

inline uint32_t mapUiPerfNowUs() { return micros(); }

inline void mapUiLogPerf(const char* tag, uint32_t t0_us) {
  const uint32_t dt = micros() - t0_us;
  if (dt < (uint32_t)MAP_UI_PERF_THRESHOLD_US) return;
  Serial.printf("[map][perf] %s %lu us (%.2f ms)\n", tag, (unsigned long)dt, dt / 1000.0f);
  Serial.flush();
}

inline void mapUiLogPerfDetail(const char* tag, uint32_t t0_us, const char* fmt, ...) {
  const uint32_t dt = micros() - t0_us;
  if (dt < (uint32_t)MAP_UI_PERF_THRESHOLD_US) return;
  Serial.printf("[map][perf] %s %lu us (%.2f ms) | ", tag, (unsigned long)dt, dt / 1000.0f);
  va_list ap;
  va_start(ap, fmt);
  char buf[96];
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  Serial.println(buf);
  Serial.flush();
}

#define MAP_UI_PERF(tag, t0) mapUiLogPerf(tag, t0)

#if defined(ESP_PLATFORM)
#include <esp_heap_caps.h>
inline void mapUiLogHeap(const char* tag) {
  const uint32_t free8 = heap_caps_get_free_size(MALLOC_CAP_8BIT);
  const uint32_t min8 = heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT);
  const uint32_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
  const uint32_t min_psram = heap_caps_get_minimum_free_size(MALLOC_CAP_SPIRAM);
  Serial.printf("[map] heap %s 8bit free=%u min=%u | psram free=%u min=%u psram_size=%u\n", tag,
                (unsigned)free8, (unsigned)min8, (unsigned)free_psram, (unsigned)min_psram,
                (unsigned)ESP.getPsramSize());
  Serial.flush();
}
#else
inline void mapUiLogHeap(const char*) {}
#endif
#else
#define MAP_UI_LOG(fmt, ...) ((void)0)
#define MAP_UI_PERF(tag, t0) ((void)0)
inline uint32_t mapUiPerfNowUs() { return 0; }
inline void mapUiLogPerfDetail(const char*, uint32_t, const char*, ...) {}
inline void mapUiLogHeap(const char*) {}
#endif
