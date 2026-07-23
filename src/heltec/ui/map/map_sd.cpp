#if defined(ENV_INCLUDE_MAP) && ENV_INCLUDE_MAP

#include "map_sd.hpp"
#include "map_debug.hpp"
#include "map_prefs.hpp"
#include "geo_point.hpp"
#include "heltec/drivers/display/spi1_bus_lock.hpp"

#if !defined(SPI_INTERFACES_COUNT) || (SPI_INTERFACES_COUNT < 2) || !defined(PIN_SPI1_SCK) || \
    (PIN_SPI1_SCK < 0)
#error "map_sd requires SPI1 (SPI_INTERFACES_COUNT>=2 and PIN_SPI1_SCK)"
#endif
#include "target.h"

#include <Arduino.h>
#include <SdFat.h>
#include <SPI.h>
#include <lvgl.h>
#include <stdio.h>
#include <string.h>
#if defined(ESP_PLATFORM)
#include <esp_task_wdt.h>
#endif

#if defined(MESH_DEBUG) && MESH_DEBUG && defined(ESP_PLATFORM)
extern "C" void heltec_map_png_trace(const char* path, uint32_t bytes, uint32_t io_us,
                                      uint32_t decode_us, uint32_t convert_us,
                                      uint32_t total_us) {
  Serial.printf("[map][png] %s bytes=%lu io=%luus decode=%luus convert=%luus total=%luus\n",
                path ? path : "?", (unsigned long)bytes, (unsigned long)io_us,
                (unsigned long)decode_us, (unsigned long)convert_us, (unsigned long)total_us);
}
#endif

namespace heltec::meshcore::ui::map {
namespace {

#ifndef MAP_UI_SD_SPI_MAX_HZ
#define MAP_UI_SD_SPI_MAX_HZ 80000000U
#endif

// Keep this list in descending order.  One entry is tried per map work tick so
// a slow or incompatible card does not monopolise the LVGL task while probing.
static constexpr uint32_t kSdProbeHz[] = {
    80000000U,
    75000000U,
    60000000U,
    50000000U,
    40000000U,
    25000000U,
    16000000U,
    8000000U,
    4000000U,
    1000000U,
    400000U,
};

#ifndef PIN_MAP_SD_SCK
#if defined(PIN_SPI1_SCK)
#define PIN_MAP_SD_SCK PIN_SPI1_SCK
#elif defined(PIN_TFT_SCL)
#define PIN_MAP_SD_SCK PIN_TFT_SCL
#endif
#endif
#ifndef PIN_MAP_SD_MOSI
#if defined(PIN_SPI1_MOSI)
#define PIN_MAP_SD_MOSI PIN_SPI1_MOSI
#elif defined(PIN_TFT_SDA)
#define PIN_MAP_SD_MOSI PIN_TFT_SDA
#endif
#endif
#ifndef PIN_MAP_SD_MISO
#if defined(PIN_SPI1_MISO)
#define PIN_MAP_SD_MISO PIN_SPI1_MISO
#elif defined(PIN_TFT_MISO)
#define PIN_MAP_SD_MISO PIN_TFT_MISO
#endif
#endif

#if defined(PIN_MAP_SD_SCK)
constexpr int kMapSdSckPin = PIN_MAP_SD_SCK;
#else
constexpr int kMapSdSckPin = -1;
#endif
#if defined(PIN_MAP_SD_MOSI)
constexpr int kMapSdMosiPin = PIN_MAP_SD_MOSI;
#else
constexpr int kMapSdMosiPin = -1;
#endif
#if defined(PIN_MAP_SD_MISO)
constexpr int kMapSdMisoPin = PIN_MAP_SD_MISO;
#else
constexpr int kMapSdMisoPin = -1;
#endif
#if defined(PIN_TFT_CS)
constexpr int kMapTftCsPin = PIN_TFT_CS;
#else
constexpr int kMapTftCsPin = -1;
#endif

static SdFs s_sd;
static bool s_sd_ready = false;
static bool s_fs_registered = false;
static bool s_pins_ready = false;
static uint8_t s_probe_hz_idx = 0;
static bool s_probe_complete = false;
static bool s_probe_started = false;
static bool s_io_failed = false;
static uint32_t s_active_hz = 0;
static uint8_t s_last_sd_error_code = 0;
static uint8_t s_last_sd_error_data = 0;
static uint8_t s_last_root_error = 0;

// Keep diagnostics useful without turning a bad card or a missing tile pack
// into an endless Serial flood.  These counters are reset when Tracker is
// entered, so each map attempt gets a fresh bounded trace.
constexpr uint8_t kFsDiagLimit = 8;
constexpr uint8_t kResolveDiagLimit = 16;
constexpr uint8_t kResolveDetailLimit = 24;
constexpr uint8_t kPngDiagLimit = 24;
static uint8_t s_fs_open_fail_logs = 0;
static uint8_t s_fs_read_fail_logs = 0;
static uint8_t s_fs_seek_fail_logs = 0;
static uint8_t s_fs_short_read_logs = 0;
static uint8_t s_resolve_diag_logs = 0;
static uint8_t s_resolve_detail_logs = 0;
static uint8_t s_png_diag_logs = 0;

constexpr int kExistCacheSize = 32;
struct ExistCacheEntry {
  char path[96];
  uint8_t exists = 0;
};
static ExistCacheEntry s_exist_cache[kExistCacheSize];
static int s_exist_cache_n = 0;
static int s_exist_cache_next = 0;

enum class TileLayout : uint8_t {
  Unknown = 0,
  MapsFlat,
  MapsNested,
  TilesFlat,
};

static const char* tileLayoutName(TileLayout layout) {
  switch (layout) {
    case TileLayout::MapsFlat:
      return "maps-flat";
    case TileLayout::MapsNested:
      return "maps-nested";
    case TileLayout::TilesFlat:
      return "tiles-flat";
    default:
      return "unknown";
  }
}

constexpr int kLayoutCacheSize = 8;
struct LayoutCacheEntry {
  char style[24] = {};
  uint8_t zoom = 0;
  TileLayout layout = TileLayout::Unknown;
  bool used = false;
};
static LayoutCacheEntry s_layout_cache[kLayoutCacheSize];
static int s_layout_cache_next = 0;

static void clearExistCache() {
  s_exist_cache_n = 0;
  s_exist_cache_next = 0;
}

static void clearLayoutCache() {
  for (LayoutCacheEntry& entry : s_layout_cache) entry = LayoutCacheEntry{};
  s_layout_cache_next = 0;
}

static void rememberSdError() {
  s_last_sd_error_code = s_sd.sdErrorCode();
  s_last_sd_error_data = s_sd.sdErrorData();
}

static void resetDiagnostics() {
  s_fs_open_fail_logs = 0;
  s_fs_read_fail_logs = 0;
  s_fs_seek_fail_logs = 0;
  s_fs_short_read_logs = 0;
  s_resolve_diag_logs = 0;
  s_resolve_detail_logs = 0;
  s_png_diag_logs = 0;
}

#ifndef PIN_MAP_SD_CS
#error "PIN_MAP_SD_CS required when ENV_INCLUDE_MAP=1"
#endif

struct SdLvFile {
  FsFile file;
  bool in_use = false;
  char path[112] = {};
  uint32_t bytes_read = 0;
};

#ifndef MAP_SD_LVGL_FILE_POOL_SIZE
constexpr uint8_t kLvglFilePoolSize = 2;
#else
constexpr uint8_t kLvglFilePoolSize = MAP_SD_LVGL_FILE_POOL_SIZE;
#endif

static SdLvFile s_lvgl_file_pool[kLvglFilePoolSize];

static SPIClass& sdSpi() { return SPI1; }

static void afterSdTransfer(SPIClass& spi) {
  (void)spi;
#if defined(PIN_TFT_CS)
  digitalWrite(PIN_TFT_CS, HIGH);
#endif
}

static void prepareSdBus() {
  dal::spi1::prepareSdBus(sdSpi());
  sdSpi().setFrequency(s_active_hz);
}

static bool sdUsable() { return s_sd_ready && !s_io_failed; }

/** ESP32-S3 SPI1 has no default pins after SdFat ends the shared bus. */
static void ensureSdSpiBus(SPIClass& spi) {
#if defined(SPI_INTERFACES_COUNT) && (SPI_INTERFACES_COUNT >= 2) && defined(PIN_SPI1_SCK) && \
    (PIN_SPI1_SCK >= 0) && defined(PIN_SPI1_MOSI) && (PIN_SPI1_MOSI >= 0)
#if defined(PIN_SPI1_MISO) && (PIN_SPI1_MISO >= 0)
  spi.begin(PIN_SPI1_SCK, PIN_SPI1_MISO, PIN_SPI1_MOSI, -1);
#else
  spi.begin(PIN_SPI1_SCK, -1, PIN_SPI1_MOSI, -1);
#endif
#else
  (void)spi;
#endif
}

static bool stripLvPath(const char* in, char* out, size_t out_len) {
  if (!in || !out || out_len < 2) return false;
  const char* p = in;
  if ('S' == p[0] && ':' == p[1]) p += 2;
  if ('/' == p[0]) p++;
  strncpy(out, p, out_len - 1);
  out[out_len - 1] = '\0';
  return out[0] != '\0';
}

static bool sdIsDir(const char* rel_path) {
  if (!sdUsable() || !rel_path || !rel_path[0]) return false;
  prepareSdBus();
  FsFile f;
  const bool ok = f.open(rel_path, O_RDONLY);
  if (!ok) {
    afterSdTransfer(sdSpi());
    return false;
  }
  const bool dir = f.isDir();
  f.close();
  afterSdTransfer(sdSpi());
  return dir;
}

static bool sdBeginAtHz(SPIClass& spi, uint32_t hz) {
  ensureSdSpiBus(spi);
  dal::spi1::prepareSdBus(spi);
  spi.setFrequency(hz);
  const SdSpiConfig cfg(PIN_MAP_SD_CS, SHARED_SPI, SD_SCK_HZ(hz), &spi);
#if defined(MESH_DEBUG) && MESH_DEBUG
  Serial.printf("[spi] SD.begin CS=%d @%luHz (TFT_CS idle)\n", PIN_MAP_SD_CS, (unsigned long)hz);
#endif
  const bool ok = s_sd.begin(cfg);
  rememberSdError();
#if defined(MESH_DEBUG) && MESH_DEBUG
  Serial.printf("[spi] SD.begin -> %s fat=%u err=0x%02X data=0x%02X\n",
                ok ? "OK" : "FAIL", ok ? (unsigned)s_sd.fatType() : 0U,
                (unsigned)s_last_sd_error_code, (unsigned)s_last_sd_error_data);
#endif
  if (!ok) {
    s_sd.end();
  }
  return ok;
}

static SdLvFile* allocLvFile() {
  for (uint8_t i = 0; i < kLvglFilePoolSize; ++i) {
    if (s_lvgl_file_pool[i].in_use) continue;
    s_lvgl_file_pool[i].in_use = true;
    s_lvgl_file_pool[i].path[0] = '\0';
    s_lvgl_file_pool[i].bytes_read = 0;
    return &s_lvgl_file_pool[i];
  }
  return nullptr;
}

static void releaseLvFile(SdLvFile* h) {
  if (!h) return;
  h->file.close();
  h->path[0] = '\0';
  h->bytes_read = 0;
  h->in_use = false;
}

static void resetTileLookupState() {
  clearExistCache();
  clearLayoutCache();
}

static bool lvglFilesInUse() {
  for (uint8_t i = 0; i < kLvglFilePoolSize; ++i) {
    if (s_lvgl_file_pool[i].in_use) return true;
  }
  return false;
}

static void closeLvglFiles() {
  for (uint8_t i = 0; i < kLvglFilePoolSize; ++i) {
    if (s_lvgl_file_pool[i].in_use) releaseLvFile(&s_lvgl_file_pool[i]);
  }
}

/** Drop a card mount and invalidate all path/existence state tied to it. */
static void dropSdMount() {
  closeLvglFiles();
  // SdFat::end() is safe after a failed begin and is needed to release the
  // shared SPI card object before trying the next clock.
  s_sd.end();
  s_sd_ready = false;
  s_active_hz = 0;
  resetTileLookupState();
}

static void noteSdIoFailure(const char* operation) {
  rememberSdError();
  if (!s_io_failed) {
    MAP_UI_LOG("SD %s failed at %luHz err=0x%02X data=0x%02X; probe will retry",
               operation ? operation : "I/O", (unsigned long)s_active_hz,
               (unsigned)s_last_sd_error_code, (unsigned)s_last_sd_error_data);
  }
  s_io_failed = true;
  resetTileLookupState();
}

static bool probeHzEnabled(uint32_t hz) {
#if MAP_UI_SD_SPI_MAX_HZ == 0
  // Preserve the old opt-out behaviour: only use the conservative 4MHz
  // mount when high-speed probing is disabled by the build.
  return hz <= 4000000U;
#else
  return hz <= (uint32_t)MAP_UI_SD_SPI_MAX_HZ;
#endif
}

static bool nextProbeHz(uint32_t& hz) {
  while (s_probe_hz_idx < (sizeof(kSdProbeHz) / sizeof(kSdProbeHz[0]))) {
    const uint32_t candidate = kSdProbeHz[s_probe_hz_idx++];
    if (!probeHzEnabled(candidate)) continue;
    hz = candidate;
    return true;
  }
  return false;
}

static bool probeHzPending() {
  uint8_t i = s_probe_hz_idx;
  while (i < (sizeof(kSdProbeHz) / sizeof(kSdProbeHz[0]))) {
    if (probeHzEnabled(kSdProbeHz[i])) return true;
    ++i;
  }
  return false;
}

/**
 * Validate the card/volume itself.  This deliberately does not inspect the
 * optional maps/ directory: a perfectly usable SD card may contain no map
 * assets, and that must not force the SPI clock back to 4MHz.
 */
static bool validateSdRoot() {
  if (!s_sd_ready) return false;
  prepareSdBus();
  FsFile root;
  const bool opened = root.openRoot(&s_sd);
  bool ok = opened;
  if (opened) {
    // Force one directory metadata read as part of the probe.  An empty root
    // is valid, so failure to find an entry is not itself an error.
    FsFile entry;
    if (entry.openNext(&root, O_RDONLY)) entry.close();
    s_last_root_error = (uint8_t)root.getError();
    ok = s_last_root_error == 0;
    root.close();
  } else {
    s_last_root_error = 0xFF;
  }
  afterSdTransfer(sdSpi());
  rememberSdError();
  return ok;
}

static void* sd_fs_open(lv_fs_drv_t* drv, const char* path, lv_fs_mode_t mode) {
  (void)drv;
  if (!sdUsable() || !path) return nullptr;
  prepareSdBus();
  const char* p = path;
  if ('S' == p[0] && ':' == p[1]) p += 2;
  if ('/' == p[0]) p++;
  SdLvFile* h = allocLvFile();
  if (!h) {
    if (s_fs_open_fail_logs < kFsDiagLimit) {
      MAP_UI_LOG("fs_open pool full path=%s pool=%u", p, (unsigned)kLvglFilePoolSize);
      ++s_fs_open_fail_logs;
    }
    afterSdTransfer(sdSpi());
    return nullptr;
  }
  strncpy(h->path, p, sizeof(h->path) - 1);
  h->path[sizeof(h->path) - 1] = '\0';
  const oflag_t flags = (LV_FS_MODE_WR == mode) ? (O_WRONLY | O_CREAT) : O_RDONLY;
  if (!h->file.open(p, flags)) {
    rememberSdError();
    const uint8_t card_error = s_last_sd_error_code;
    if (s_fs_open_fail_logs < kFsDiagLimit) {
      MAP_UI_LOG("fs_open fail path=%s mode=%u hz=%lu err=0x%02X data=0x%02X",
                 h->path, (unsigned)mode, (unsigned long)s_active_hz,
                 (unsigned)s_last_sd_error_code, (unsigned)s_last_sd_error_data);
      ++s_fs_open_fail_logs;
    }
    releaseLvFile(h);
    afterSdTransfer(sdSpi());
    if (card_error != 0) noteSdIoFailure("open");
    return nullptr;
  }
  afterSdTransfer(sdSpi());
  return h;
}

static lv_fs_res_t sd_fs_close(lv_fs_drv_t* drv, void* file_p) {
  (void)drv;
  auto* h = static_cast<SdLvFile*>(file_p);
  if (!h || !h->in_use) return LV_FS_RES_INV_PARAM;
  prepareSdBus();
  releaseLvFile(h);
  afterSdTransfer(sdSpi());
  return LV_FS_RES_OK;
}

static lv_fs_res_t sd_fs_read(lv_fs_drv_t* drv, void* file_p, void* buf, uint32_t btr, uint32_t* br) {
  (void)drv;
  auto* h = static_cast<SdLvFile*>(file_p);
  if (!h || !h->in_use || !h->file.isOpen() || !buf || !br) return LV_FS_RES_INV_PARAM;
  *br = 0;
  if (!sdUsable()) return LV_FS_RES_HW_ERR;
  if (btr == 0) return LV_FS_RES_OK;
  prepareSdBus();
  const uint32_t pos = h->file.curPosition();
  const int read_count = h->file.read(buf, btr);
  afterSdTransfer(sdSpi());
  rememberSdError();
  if (read_count < 0) {
    if (s_fs_read_fail_logs < kFsDiagLimit) {
      MAP_UI_LOG("fs_read fail path=%s pos=%lu req=%lu hz=%lu err=0x%02X data=0x%02X",
                 h->path, (unsigned long)pos, (unsigned long)btr,
                 (unsigned long)s_active_hz, (unsigned)s_last_sd_error_code,
                 (unsigned)s_last_sd_error_data);
      ++s_fs_read_fail_logs;
    }
    noteSdIoFailure("read");
    return LV_FS_RES_FS_ERR;
  }
  *br = (uint32_t)read_count;
  h->bytes_read += (uint32_t)read_count;
  if ((uint32_t)read_count != btr && s_fs_short_read_logs < kFsDiagLimit) {
    MAP_UI_LOG("fs_read short path=%s pos=%lu req=%lu got=%lu total=%lu err=0x%02X data=0x%02X",
               h->path, (unsigned long)pos, (unsigned long)btr,
               (unsigned long)read_count, (unsigned long)h->bytes_read,
               (unsigned)s_last_sd_error_code, (unsigned)s_last_sd_error_data);
    ++s_fs_short_read_logs;
  }
  if ((uint32_t)read_count != btr && s_last_sd_error_code != 0) {
    noteSdIoFailure("short-read");
  }
  return LV_FS_RES_OK;
}

static lv_fs_res_t sd_fs_seek(lv_fs_drv_t* drv, void* file_p, uint32_t pos, lv_fs_whence_t whence) {
  (void)drv;
  auto* h = static_cast<SdLvFile*>(file_p);
  if (!h || !h->in_use || !h->file.isOpen()) return LV_FS_RES_INV_PARAM;
  if (!sdUsable()) return LV_FS_RES_HW_ERR;
  prepareSdBus();
  bool ok = false;
  if (LV_FS_SEEK_SET == whence) {
    ok = h->file.seekSet(pos);
  } else if (LV_FS_SEEK_CUR == whence) {
    ok = h->file.seekCur((int32_t)pos);
  } else if (LV_FS_SEEK_END == whence) {
    ok = h->file.seekEnd((int32_t)pos);
  } else {
    afterSdTransfer(sdSpi());
    if (s_fs_seek_fail_logs < kFsDiagLimit) {
      MAP_UI_LOG("fs_seek invalid path=%s pos=%lu whence=%u hz=%lu",
                 h->path, (unsigned long)pos, (unsigned)whence,
                 (unsigned long)s_active_hz);
      ++s_fs_seek_fail_logs;
    }
    return LV_FS_RES_INV_PARAM;
  }
  afterSdTransfer(sdSpi());
  rememberSdError();
  if (!ok) {
    if (s_fs_seek_fail_logs < kFsDiagLimit) {
      MAP_UI_LOG("fs_seek fail path=%s pos=%lu whence=%u hz=%lu err=0x%02X data=0x%02X",
                 h->path, (unsigned long)pos, (unsigned)whence,
                 (unsigned long)s_active_hz, (unsigned)s_last_sd_error_code,
                 (unsigned)s_last_sd_error_data);
      ++s_fs_seek_fail_logs;
    }
    noteSdIoFailure("seek");
    return LV_FS_RES_FS_ERR;
  }
  return LV_FS_RES_OK;
}

static lv_fs_res_t sd_fs_tell(lv_fs_drv_t* drv, void* file_p, uint32_t* pos_p) {
  (void)drv;
  auto* h = static_cast<SdLvFile*>(file_p);
  if (!h || !h->in_use || !h->file.isOpen() || !pos_p) return LV_FS_RES_INV_PARAM;
  *pos_p = h->file.curPosition();
  return LV_FS_RES_OK;
}

}  // namespace

#if defined(MESH_DEBUG) && MESH_DEBUG && defined(ESP_PLATFORM)
extern "C" void heltec_map_png_diag(const char* phase, const char* path, uint32_t code,
                                     const char* detail) {
  if (s_png_diag_logs >= kPngDiagLimit) return;
  ++s_png_diag_logs;
  Serial.printf("[map][png] diag phase=%s path=%s code=%lu detail=%s\n",
                phase ? phase : "?", path ? path : "?", (unsigned long)code,
                detail ? detail : "?");
  Serial.flush();
}
#endif

static bool safeStyleComponent(const char* name);
static bool findFirstTileUnder(const char* base, uint8_t zoom, bool allow_nested,
                               uint32_t& x_tile, uint32_t& y_tile, char* found_path,
                               size_t found_path_len);

void map_sd_prepare_pins() {
  if (s_pins_ready) return;
  pinMode(PIN_MAP_SD_CS, OUTPUT);
  digitalWrite(PIN_MAP_SD_CS, HIGH);
#if defined(PIN_TFT_CS)
  pinMode(PIN_TFT_CS, OUTPUT);
  digitalWrite(PIN_TFT_CS, HIGH);
#endif
#if defined(PIN_TFT_DC)
  pinMode(PIN_TFT_DC, OUTPUT);
  digitalWrite(PIN_TFT_DC, HIGH);
#endif
  s_pins_ready = true;
  MAP_UI_LOG("[sd] pins CS=%d SCK=%d MOSI=%d MISO=%d TFT_CS=%d",
             PIN_MAP_SD_CS, kMapSdSckPin, kMapSdMosiPin, kMapSdMisoPin, kMapTftCsPin);
}

void map_sd_on_screen_enter() {
  resetDiagnostics();
  MAP_UI_LOG("[sd] enter ready=%d io_failed=%d probe_complete=%d probe_idx=%u active_hz=%lu "
             "last_err=0x%02X data=0x%02X pins=%d/%d/%d/%d tft_cs=%d",
             sdUsable() ? 1 : 0, s_io_failed ? 1 : 0, s_probe_complete ? 1 : 0,
             (unsigned)s_probe_hz_idx, (unsigned long)s_active_hz,
             (unsigned)s_last_sd_error_code, (unsigned)s_last_sd_error_data,
             PIN_MAP_SD_CS, kMapSdSckPin, kMapSdMosiPin, kMapSdMisoPin, kMapTftCsPin);
  if (sdUsable()) {
    // Re-scan paths on each entry so an SD card updated while mounted does
    // not keep stale negative existence or layout hints indefinitely.
    resetTileLookupState();
    return;
  }
  // A previous screen may have observed a card I/O error.  End that mount and
  // continue from the next lower probe clock on the next screen entry.
  const bool retry_lower = s_io_failed;
  if (retry_lower && !lvglFilesInUse()) {
    dropSdMount();
  }
  if (!s_sd_ready) {
    if (!retry_lower) s_probe_hz_idx = 0;
    s_probe_complete = false;
    s_probe_started = retry_lower;
    s_io_failed = false;
    resetTileLookupState();
  }
  MAP_UI_LOG("[sd] enter scheduled retry_lower=%d probe_idx=%u ready=%d",
             retry_lower ? 1 : 0, (unsigned)s_probe_hz_idx, sdUsable() ? 1 : 0);
}

bool map_sd_probe_once() {
  if (sdUsable()) {
    s_probe_complete = true;
    return true;
  }

  // Do not tear down a failed mount while LVGL is still unwinding an open
  // file.  The next scheduled probe tick will continue once it is closed.
  if (s_io_failed) {
    if (lvglFilesInUse()) return false;
    dropSdMount();
    s_io_failed = false;
    s_probe_complete = false;
    // s_probe_hz_idx already points after the active clock that failed.
    s_probe_started = true;
  }
  if (s_probe_complete) return true;

#if defined(ESP_PLATFORM)
  esp_task_wdt_reset();
#endif
  map_sd_prepare_pins();

  uint32_t hz = 0;
  if (!nextProbeHz(hz)) {
    s_probe_complete = true;
#if defined(MESH_DEBUG) && MESH_DEBUG
    Serial.printf("[spi] SD probe exhausted CS=%d attempts=%u last_err=0x%02X data=0x%02X root=0x%02X\n",
                  PIN_MAP_SD_CS, (unsigned)s_probe_hz_idx, (unsigned)s_last_sd_error_code,
                  (unsigned)s_last_sd_error_data, (unsigned)s_last_root_error);
#endif
    return true;
  }

  if (!s_probe_started) {
    s_probe_started = true;
    // Give cards a short power-up settling period without delaying every
    // frequency attempt.
    delay(10);
  }

  resetTileLookupState();
  if (!sdBeginAtHz(sdSpi(), hz)) {
    afterSdTransfer(sdSpi());
    if (!probeHzPending()) {
      s_probe_complete = true;
#if defined(MESH_DEBUG) && MESH_DEBUG
      Serial.printf("[spi] SD probe exhausted CS=%d attempts=%u last_err=0x%02X data=0x%02X root=0x%02X\n",
                    PIN_MAP_SD_CS, (unsigned)s_probe_hz_idx, (unsigned)s_last_sd_error_code,
                    (unsigned)s_last_sd_error_data, (unsigned)s_last_root_error);
#endif
      return true;
    }
    return false;
  }

  s_sd_ready = true;
  s_active_hz = hz;
  s_io_failed = false;

  if (!validateSdRoot()) {
    MAP_UI_LOG("SD root validation failed @%luHz root=0x%02X err=0x%02X data=0x%02X",
               (unsigned long)hz, (unsigned)s_last_root_error,
               (unsigned)s_last_sd_error_code, (unsigned)s_last_sd_error_data);
    dropSdMount();
    if (!probeHzPending()) {
      s_probe_complete = true;
      return true;
    }
    return false;
  }

  s_probe_complete = true;
  map_sd_register_lvgl_fs();
#if defined(MESH_DEBUG) && MESH_DEBUG
  Serial.printf("[spi] SD ready CS=%d fs=%s @%luHz err=0x%02X data=0x%02X root=0x%02X\n",
                PIN_MAP_SD_CS, map_sd_fs_label(), (unsigned long)s_active_hz,
                (unsigned)s_last_sd_error_code, (unsigned)s_last_sd_error_data,
                (unsigned)s_last_root_error);
#endif
  return true;
}

bool map_sd_try_mount_once() { return map_sd_probe_once(); }

bool map_sd_try_boost_speed_once() {
  // Kept for source compatibility.  Probing now starts at the configured
  // maximum and descends one clock per call, so there is no separate boost
  // phase anymore.
  return map_sd_probe_once();
}

bool map_sd_init() {
#if defined(MESH_DEBUG) && MESH_DEBUG
  Serial.printf("[spi] map_sd CS=%d t=%lu\n", PIN_MAP_SD_CS, (unsigned long)millis());
#endif
  if (!sdUsable() && s_probe_complete && !s_io_failed) {
    s_probe_hz_idx = 0;
    s_probe_complete = false;
    s_probe_started = false;
    s_io_failed = false;
    resetTileLookupState();
  }
  map_sd_prepare_pins();
  while (!map_sd_probe_once()) {
#if defined(ESP_PLATFORM)
    esp_task_wdt_reset();
#endif
    yield();
  }
#if defined(MESH_DEBUG) && MESH_DEBUG
  if (!map_sd_ready()) {
    Serial.printf("[spi] SD init failed CS=%d\n", PIN_MAP_SD_CS);
  }
#endif
  return map_sd_ready();
}

bool map_sd_ready() { return sdUsable(); }

uint32_t map_sd_active_hz() { return sdUsable() ? s_active_hz : 0U; }

const char* map_sd_fs_label() {
  if (!sdUsable()) return nullptr;
  switch (s_sd.fatType()) {
    case FAT_TYPE_EXFAT:
      return "exFAT";
    case FAT_TYPE_FAT32:
      return "FAT32";
    case FAT_TYPE_FAT16:
      return "FAT16";
    default:
      return "?";
  }
}

void map_sd_register_lvgl_fs() {
  if (s_fs_registered || !sdUsable()) return;
  static lv_fs_drv_t drv;
  lv_fs_drv_init(&drv);
  drv.letter = 'S';
  drv.open_cb = sd_fs_open;
  drv.close_cb = sd_fs_close;
  drv.read_cb = sd_fs_read;
  drv.seek_cb = sd_fs_seek;
  drv.tell_cb = sd_fs_tell;
  lv_fs_drv_register(&drv);
  s_fs_registered = true;
  MAP_UI_LOG("lvgl fs S: registered");
}

bool map_sd_tile_path(char* out, size_t out_len, const char* style, uint8_t zoom, uint32_t x_tile,
                      uint32_t y_tile) {
  if (!out || out_len < 8) return false;
  const char* st = (style && style[0]) ? style : "osm";
  if (!safeStyleComponent(st)) return false;
  // This is the non-validating fallback used by callers after a failed
  // resolve.  Prefer the conventional maps/ path; resolve_tile_path() below
  // probes all supported roots/layouts when the card is available.
  const int n = snprintf(out, out_len, "%smaps/%s/%u/%lu/%lu.png",
                         sdUsable() ? "S:" : "", st, (unsigned)zoom,
                         (unsigned long)x_tile, (unsigned long)y_tile);
  return n > 0 && (size_t)n < out_len;
}

static bool parseUnsignedDecimal(const char* text, uint32_t& value_out) {
  if (!text || !text[0]) return false;
  uint64_t value = 0;
  for (const char* p = text; *p; ++p) {
    if (*p < '0' || *p > '9') return false;
    const uint32_t digit = (uint32_t)(*p - '0');
    if (value > (0xFFFFFFFFULL - digit) / 10U) return false;
    value = value * 10U + digit;
  }
  value_out = (uint32_t)value;
  return true;
}

static bool parsePngTileName(const char* name, uint32_t& y_out) {
  if (!name || !name[0]) return false;
  const char* dot = strrchr(name, '.');
  if (!dot || dot == name || strlen(dot) != 4) return false;
  if (0 != strcmp(dot, ".png")) return false;
  char digits[20];
  const size_t len = (size_t)(dot - name);
  if (len >= sizeof(digits)) return false;
  memcpy(digits, name, len);
  digits[len] = '\0';
  return parseUnsignedDecimal(digits, y_out);
}

static bool safeStyleComponent(const char* name) {
  if (!name || !name[0]) return false;
  const size_t len = strlen(name);
  if ((len == 1 && name[0] == '.') || (len == 2 && name[0] == '.' && name[1] == '.')) {
    return false;
  }
  for (const unsigned char* p = reinterpret_cast<const unsigned char*>(name); *p; ++p) {
    const unsigned char c = *p;
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.') {
      continue;
    }
    return false;
  }
  return true;
}

static bool tileIndexValid(uint32_t index, uint8_t zoom) {
  if (zoom >= 32) return true;
  return index < (1UL << zoom);
}

static LayoutCacheEntry* layoutCacheEntry(const char* style, uint8_t zoom, bool create) {
  if (!style || !style[0] || strlen(style) >= sizeof(s_layout_cache[0].style)) return nullptr;
  for (LayoutCacheEntry& entry : s_layout_cache) {
    if (entry.used && entry.zoom == zoom && 0 == strcmp(entry.style, style)) return &entry;
  }
  if (!create) return nullptr;

  LayoutCacheEntry* entry = nullptr;
  for (LayoutCacheEntry& candidate : s_layout_cache) {
    if (!candidate.used) {
      entry = &candidate;
      break;
    }
  }
  if (!entry) {
    entry = &s_layout_cache[s_layout_cache_next];
    s_layout_cache_next = (s_layout_cache_next + 1) % kLayoutCacheSize;
  }
  *entry = LayoutCacheEntry{};
  strncpy(entry->style, style, sizeof(entry->style) - 1);
  entry->zoom = zoom;
  entry->used = true;
  return entry;
}

static bool formatTileRelPath(char* out, size_t out_len, TileLayout layout, const char* style,
                              uint8_t zoom, uint32_t x_tile, uint32_t y_tile) {
  if (!out || out_len == 0 || !style) return false;
  int n = -1;
  switch (layout) {
    case TileLayout::MapsFlat:
      n = snprintf(out, out_len, "maps/%s/%u/%lu/%lu.png", style, (unsigned)zoom,
                   (unsigned long)x_tile, (unsigned long)y_tile);
      break;
    case TileLayout::MapsNested:
      n = snprintf(out, out_len, "maps/%s/%u/%lu/%lu/%lu.png", style, (unsigned)zoom,
                   (unsigned long)x_tile, (unsigned long)y_tile, (unsigned long)y_tile);
      break;
    case TileLayout::TilesFlat:
      n = snprintf(out, out_len, "tiles/%u/%lu/%lu.png", (unsigned)zoom,
                   (unsigned long)x_tile, (unsigned long)y_tile);
      break;
    default:
      return false;
  }
  return n > 0 && (size_t)n < out_len;
}

static void logTileResolve(const char* style, uint8_t zoom, uint32_t x_tile, uint32_t y_tile,
                           TileLayout preferred, TileLayout selected, bool found,
                           const char* path) {
  if (s_resolve_diag_logs >= kResolveDiagLimit) return;
  ++s_resolve_diag_logs;
  MAP_UI_LOG("tile resolve %s style=%s z=%u xy=%lu,%lu preferred=%s layout=%s path=%s "
             "sd=%d hz=%lu err=0x%02X data=0x%02X",
             found ? "ok" : "miss", style ? style : "?", (unsigned)zoom,
             (unsigned long)x_tile, (unsigned long)y_tile, tileLayoutName(preferred),
             tileLayoutName(selected), path && path[0] ? path : "-", sdUsable() ? 1 : 0,
             (unsigned long)s_active_hz, (unsigned)s_last_sd_error_code,
             (unsigned)s_last_sd_error_data);

}

static bool resolveWithLayout(char* out, size_t out_len, TileLayout layout, const char* style,
                              uint8_t zoom, uint32_t x_tile, uint32_t y_tile) {
  char rel[128] = {};
  const bool formatted = formatTileRelPath(rel, sizeof(rel), layout, style, zoom, x_tile, y_tile);
  const bool exists = formatted && map_sd_exists(rel);
  if (formatted && s_resolve_detail_logs < kResolveDetailLimit) {
    MAP_UI_LOG("tile resolve attempt layout=%s rel=%s exists=%d err=0x%02X data=0x%02X",
               tileLayoutName(layout), rel, exists ? 1 : 0,
               (unsigned)s_last_sd_error_code, (unsigned)s_last_sd_error_data);
    ++s_resolve_detail_logs;
  }
  if (!exists) {
    return false;
  }
  const int n = snprintf(out, out_len, "S:%s", rel);
  return n > 0 && (size_t)n < out_len;
}

bool map_sd_resolve_tile_path(char* out, size_t out_len, const char* style, uint8_t zoom,
                              uint32_t x_tile, uint32_t y_tile) {
  if (!out || out_len < 12 || !sdUsable()) return false;
  if (!tileIndexValid(x_tile, zoom) || !tileIndexValid(y_tile, zoom)) {
    MAP_UI_LOG("tile resolve skip invalid index z=%u xy=%lu,%lu", (unsigned)zoom,
               (unsigned long)x_tile, (unsigned long)y_tile);
    return false;
  }
  const char* st = (style && style[0]) ? style : "osm";
  if (!safeStyleComponent(st)) {
    MAP_UI_LOG("tile resolve skip unsafe style=%s", st ? st : "(null)");
    return false;
  }

  // Prefer the layout that most recently succeeded for this style/zoom, but
  // always fall back to every other supported layout.  This keeps normal
  // /tiles packs fast without breaking cards that mix layouts.
  LayoutCacheEntry* cached = layoutCacheEntry(st, zoom, false);
  const TileLayout preferred = cached ? cached->layout : TileLayout::Unknown;
  if (preferred != TileLayout::Unknown &&
      resolveWithLayout(out, out_len, preferred, st, zoom, x_tile, y_tile)) {
    logTileResolve(st, zoom, x_tile, y_tile, preferred, preferred, true, out);
    return true;
  }

  constexpr TileLayout kLayouts[] = {
      TileLayout::MapsFlat,
      TileLayout::MapsNested,
      TileLayout::TilesFlat,
  };
  for (TileLayout layout : kLayouts) {
    if (layout == preferred) continue;
    if (!resolveWithLayout(out, out_len, layout, st, zoom, x_tile, y_tile)) continue;
    if (LayoutCacheEntry* entry = layoutCacheEntry(st, zoom, true)) entry->layout = layout;
    logTileResolve(st, zoom, x_tile, y_tile, preferred, layout, true, out);
    return true;
  }
  logTileResolve(st, zoom, x_tile, y_tile, preferred, TileLayout::Unknown, false, nullptr);
  return false;
}

bool map_sd_exists(const char* rel_path) {
  if (!sdUsable() || !rel_path || !rel_path[0]) return false;

  for (int i = 0; i < s_exist_cache_n; ++i) {
    if (0 == strcmp(s_exist_cache[i].path, rel_path)) {
      return s_exist_cache[i].exists != 0;
    }
  }

  prepareSdBus();
  const bool ok = s_sd.exists(rel_path);
  rememberSdError();
  const uint8_t card_error = s_last_sd_error_code;
  afterSdTransfer(sdSpi());
  if (!ok && card_error != 0) {
    MAP_UI_LOG("exists error path=%s hz=%lu err=0x%02X data=0x%02X", rel_path,
               (unsigned long)s_active_hz, (unsigned)s_last_sd_error_code,
               (unsigned)s_last_sd_error_data);
    noteSdIoFailure("exists");
    return false;
  }

  int slot = 0;
  if (s_exist_cache_n < kExistCacheSize) {
    slot = s_exist_cache_n++;
  } else {
    slot = s_exist_cache_next;
    s_exist_cache_next = (s_exist_cache_next + 1) % kExistCacheSize;
  }
  strncpy(s_exist_cache[slot].path, rel_path, sizeof(s_exist_cache[slot].path) - 1);
  s_exist_cache[slot].path[sizeof(s_exist_cache[slot].path) - 1] = '\0';
  s_exist_cache[slot].exists = ok ? 1 : 0;
  return ok;
}

static bool tileTreeHasAnyTile(const char* root_name, bool allow_nested) {
  if (!sdUsable() || !root_name || !root_name[0]) return false;
  prepareSdBus();
  FsFile root;
  if (!root.open(root_name, O_RDONLY) || !root.isDir()) {
    root.close();
    afterSdTransfer(sdSpi());
    return false;
  }

  bool found = false;
  FsFile ent;
  while (ent.openNext(&root, O_RDONLY)) {
#if defined(ESP_PLATFORM)
    esp_task_wdt_reset();
#endif
    if (!ent.isDir()) {
      ent.close();
      continue;
    }
    char zoom_name[16];
    ent.getName(zoom_name, sizeof(zoom_name));
    ent.close();
    uint32_t zoom = 0;
    if (!parseUnsignedDecimal(zoom_name, zoom) || zoom < kZoomMin || zoom > kZoomMax) {
      continue;
    }
    char base[80];
    const int n = snprintf(base, sizeof(base), "%s/%s", root_name, zoom_name);
    if (n <= 0 || (size_t)n >= sizeof(base)) continue;
    uint32_t x = 0;
    uint32_t y = 0;
    if (findFirstTileUnder(base, (uint8_t)zoom, allow_nested, x, y, nullptr, 0)) {
      found = true;
      break;
    }
  }
  root.close();
  afterSdTransfer(sdSpi());
  return found;
}

static bool mapStyleHasAnyTile(const char* style) {
  if (!safeStyleComponent(style)) return false;
  char root[48];
  const int n = snprintf(root, sizeof(root), "maps/%s", style);
  if (n <= 0 || (size_t)n >= sizeof(root)) return false;
  return tileTreeHasAnyTile(root, true);
}

void map_sd_resolve_style(char* style, size_t style_len) {
  if (!sdUsable() || !style || style_len < 2) return;
  if (!safeStyleComponent(style)) {
    if (style_len <= 3) return;
    strncpy(style, "osm", style_len - 1);
    style[style_len - 1] = '\0';
  }

  if (mapStyleHasAnyTile(style)) return;

  // Collect candidate names first, then validate each one after the parent
  // directory is closed.  This avoids interleaving SdFat directory iteration
  // with the nested tile probes.
  char candidates[16][24]{};
  int candidate_count = 0;
  prepareSdBus();
  FsFile maps;
  if (maps.open("maps", O_RDONLY)) {
    FsFile ent;
    while (candidate_count < (int)(sizeof(candidates) / sizeof(candidates[0])) &&
           ent.openNext(&maps, O_RDONLY)) {
      if (ent.isDir()) {
        char candidate[24];
        const size_t name_len = ent.getName(candidate, sizeof(candidate));
        if (name_len > 0 && name_len < sizeof(candidate) && candidate[0] != '.' &&
            safeStyleComponent(candidate)) {
          strncpy(candidates[candidate_count], candidate, sizeof(candidates[0]) - 1);
          candidates[candidate_count][sizeof(candidates[0]) - 1] = '\0';
          ++candidate_count;
        }
      }
      ent.close();
    }
    maps.close();
  }
  afterSdTransfer(sdSpi());

  for (int i = 0; i < candidate_count; ++i) {
    if (strlen(candidates[i]) >= style_len || 0 == strcmp(candidates[i], style)) continue;
    if (!mapStyleHasAnyTile(candidates[i])) continue;
    strncpy(style, candidates[i], style_len - 1);
    style[style_len - 1] = '\0';
    MAP_UI_LOG("style fallback -> %s", style);
    return;
  }

  // tiles/{z}/{x}/{y}.png has no style component.  Keep the preference value
  // intact so switching back to a maps/ card still restores the same style.
  if (tileTreeHasAnyTile("tiles", false)) {
    MAP_UI_LOG("SD: using styleless tiles/ layout");
    return;
  }

  if (!sdIsDir("maps")) {
    MAP_UI_LOG("SD: maps/ missing");
  } else {
    MAP_UI_LOG("SD: no valid map style tiles");
  }
}

static bool tileZoomDirExists(const char* style, uint8_t zoom) {
  char path[56];
  const char* st = (style && style[0]) ? style : "osm";
  if (!safeStyleComponent(st)) return false;
  snprintf(path, sizeof(path), "maps/%s/%u", st, (unsigned)zoom);
  const bool maps_dir = sdIsDir(path);
  snprintf(path, sizeof(path), "tiles/%u", (unsigned)zoom);
  const bool tiles_dir = sdIsDir(path);
  if (!maps_dir && !tiles_dir) return false;

  // A directory alone is not a usable zoom level.  Confirm at least one
  // strictly numeric PNG tile; this also handles cards containing an empty
  // maps/style directory alongside a populated tiles/ tree.
  uint32_t x = 0;
  uint32_t y = 0;
  return map_sd_find_first_tile(st, zoom, x, y);
}

uint8_t map_sd_best_zoom(const char* style, uint8_t preferred) {
  preferred = clampMapZoom(preferred);
  if (!sdUsable() || !safeStyleComponent(style)) return preferred;
  if (tileZoomDirExists(style, preferred)) return preferred;

  // Search by distance from the requested zoom.  At equal distance prefer the
  // lower level, which normally provides broader coverage.
  for (int delta = 1; delta <= (int)kZoomMax - (int)kZoomMin; ++delta) {
    const int lower = (int)preferred - delta;
    if (lower >= (int)kZoomMin && tileZoomDirExists(style, (uint8_t)lower)) {
      MAP_UI_LOG("zoom %u missing on SD, use %d", (unsigned)preferred, lower);
      return (uint8_t)lower;
    }
    const int upper = (int)preferred + delta;
    if (upper <= (int)kZoomMax && tileZoomDirExists(style, (uint8_t)upper)) {
      MAP_UI_LOG("zoom %u missing on SD, use %d", (unsigned)preferred, upper);
      return (uint8_t)upper;
    }
  }
  MAP_UI_LOG("zoom %u: no maps/%s/{z} or tiles/{z} on SD", (unsigned)preferred, style);
  return preferred;
}

void map_sd_log_catalog(const char* style) {
  if (!sdUsable()) return;
  prepareSdBus();

  FsFile maps;
  if (maps.open("maps", O_RDONLY)) {
    char styles[80] = "";
    FsFile ent;
    while (ent.openNext(&maps, O_RDONLY)) {
#if defined(ESP_PLATFORM)
      esp_task_wdt_reset();
#endif
      if (!ent.isDir()) {
        ent.close();
        continue;
      }
      char name[20];
      ent.getName(name, sizeof(name));
      if (styles[0]) strncat(styles, ",", sizeof(styles) - strlen(styles) - 1);
      strncat(styles, name, sizeof(styles) - strlen(styles) - 1);
      ent.close();
    }
    maps.close();
    MAP_UI_LOG("SD maps styles: %s", styles[0] ? styles : "(none)");
  } else {
    MAP_UI_LOG("SD: maps/ missing");
  }

  if (safeStyleComponent(style)) {
    char base[32];
    snprintf(base, sizeof(base), "maps/%s", style);
    FsFile zdir;
    if (zdir.open(base, O_RDONLY)) {
      char zooms[96] = "";
      FsFile ent;
      while (ent.openNext(&zdir, O_RDONLY)) {
#if defined(ESP_PLATFORM)
        esp_task_wdt_reset();
#endif
        if (!ent.isDir()) {
          ent.close();
          continue;
        }
        char name[16];
        ent.getName(name, sizeof(name));
        uint32_t zoom = 0;
        if (parseUnsignedDecimal(name, zoom) && zoom <= 255U) {
          if (zooms[0]) strncat(zooms, ",", sizeof(zooms) - strlen(zooms) - 1);
          strncat(zooms, name, sizeof(zooms) - strlen(zooms) - 1);
        }
        ent.close();
      }
      zdir.close();
      MAP_UI_LOG("SD %s zooms: %s", base, zooms[0] ? zooms : "(none)");
    } else {
      MAP_UI_LOG("SD: %s missing", base);
    }
  }

  FsFile tiles;
  if (tiles.open("tiles", O_RDONLY)) {
    char zooms[96] = "";
    FsFile ent;
    while (ent.openNext(&tiles, O_RDONLY)) {
#if defined(ESP_PLATFORM)
      esp_task_wdt_reset();
#endif
      if (ent.isDir()) {
        char name[16];
        ent.getName(name, sizeof(name));
        uint32_t zoom = 0;
        if (parseUnsignedDecimal(name, zoom) && zoom <= 255U) {
          if (zooms[0]) strncat(zooms, ",", sizeof(zooms) - strlen(zooms) - 1);
          strncat(zooms, name, sizeof(zooms) - strlen(zooms) - 1);
        }
      }
      ent.close();
    }
    tiles.close();
    MAP_UI_LOG("SD tiles zooms: %s", zooms[0] ? zooms : "(none)");
  } else {
    MAP_UI_LOG("SD: tiles/ missing");
  }

  afterSdTransfer(sdSpi());
}

void map_sd_apply_tile_prefs(MapUiPrefs& prefs) {
  if (!sdUsable()) return;
  char before_style[sizeof(prefs.tile_style)];
  const uint8_t before_zoom = prefs.zoom;
  strncpy(before_style, prefs.tile_style, sizeof(before_style));
  before_style[sizeof(before_style) - 1] = '\0';

  map_sd_resolve_style(prefs.tile_style, sizeof(prefs.tile_style));
  prefs.zoom = map_sd_best_zoom(prefs.tile_style, prefs.zoom);
#if defined(MESH_DEBUG) && MESH_DEBUG
  map_sd_log_catalog(prefs.tile_style);
#endif

  if (before_zoom != prefs.zoom || 0 != strncmp(before_style, prefs.tile_style, sizeof(before_style))) {
    MAP_UI_LOG("tile prefs adjusted style %s->%s zoom %u->%u", before_style, prefs.tile_style,
               (unsigned)before_zoom, (unsigned)prefs.zoom);
  }
}

static bool tileExistsAt(const char* style, uint8_t zoom, uint32_t x_tile, uint32_t y_tile) {
  char path[96];
  return map_sd_resolve_tile_path(path, sizeof(path), style, zoom, x_tile, y_tile);
}

static bool findFirstTileUnder(const char* base, uint8_t zoom, bool allow_nested,
                               uint32_t& x_tile, uint32_t& y_tile, char* found_path,
                               size_t found_path_len) {
  if (!base || !base[0]) return false;
  FsFile zdir;
  if (!zdir.open(base, O_RDONLY)) return false;

  FsFile xent;
  while (xent.openNext(&zdir, O_RDONLY)) {
#if defined(ESP_PLATFORM)
    esp_task_wdt_reset();
#endif
    if (!xent.isDir()) {
      xent.close();
      continue;
    }
    char xname[16];
    xent.getName(xname, sizeof(xname));
    xent.close();
    uint32_t x = 0;
    if (!parseUnsignedDecimal(xname, x) || !tileIndexValid(x, zoom)) continue;

    char xpath[80];
    snprintf(xpath, sizeof(xpath), "%s/%s", base, xname);
    FsFile xdir;
    if (!xdir.open(xpath, O_RDONLY)) continue;

    FsFile yent;
    while (yent.openNext(&xdir, O_RDONLY)) {
#if defined(ESP_PLATFORM)
      esp_task_wdt_reset();
#endif
      if (yent.isDir()) {
        char yname[16];
        yent.getName(yname, sizeof(yname));
        yent.close();
        if (!allow_nested) continue;
        uint32_t y = 0;
        if (!parseUnsignedDecimal(yname, y) || !tileIndexValid(y, zoom)) continue;
        char nested[112];
        snprintf(nested, sizeof(nested), "%s/%s/%s.png", xpath, yname, yname);
        if (!map_sd_exists(nested)) continue;
        x_tile = x;
        y_tile = y;
        if (found_path && found_path_len) {
          strncpy(found_path, nested, found_path_len - 1);
          found_path[found_path_len - 1] = '\0';
        }
        xdir.close();
        zdir.close();
        return true;
      }

      char fname[20];
      yent.getName(fname, sizeof(fname));
      yent.close();
      uint32_t y = 0;
      if (!parsePngTileName(fname, y) || !tileIndexValid(y, zoom)) continue;
      x_tile = x;
      y_tile = y;
      if (found_path && found_path_len) {
        snprintf(found_path, found_path_len, "%s/%s", xpath, fname);
      }
      xdir.close();
      zdir.close();
      return true;
    }
    xdir.close();
  }
  zdir.close();
  return false;
}

bool map_sd_find_first_tile(const char* style, uint8_t zoom, uint32_t& x_tile, uint32_t& y_tile) {
  if (!sdUsable()) return false;
  const char* st = (style && style[0]) ? style : "osm";
  if (!safeStyleComponent(st)) return false;
  char base[64];
  char found[112] = "";
  prepareSdBus();
  snprintf(base, sizeof(base), "maps/%s/%u", st, (unsigned)zoom);
  bool found_tile = findFirstTileUnder(base, zoom, true, x_tile, y_tile, found, sizeof(found));
  if (!found_tile) {
    snprintf(base, sizeof(base), "tiles/%u", (unsigned)zoom);
    found_tile = findFirstTileUnder(base, zoom, false, x_tile, y_tile, found, sizeof(found));
  }
  afterSdTransfer(sdSpi());
  if (found_tile) MAP_UI_LOG("first tile %s", found);
  return found_tile;
}

bool map_sd_snap_center_to_tiles(const char* style, uint8_t zoom, float& lat, float& lon) {
  if (!sdUsable() || !safeStyleComponent((style && style[0]) ? style : "osm")) return false;
  GeoPoint gp(lat, lon, zoom);
  if (tileExistsAt(style, zoom, gp.x_tile, gp.y_tile)) return false;

  uint32_t xt = 0;
  uint32_t yt = 0;
  if (!map_sd_find_first_tile(style, zoom, xt, yt)) {
    MAP_UI_LOG("snap: no tiles under maps/%s/%u or tiles/%u",
               (style && style[0]) ? style : "osm", (unsigned)zoom, (unsigned)zoom);
    return false;
  }
  tileCenterLatLon(xt, yt, zoom, lat, lon);
  MAP_UI_LOG("snap center -> tile %lu/%lu (%.4f,%.4f)", (unsigned long)xt, (unsigned long)yt, (double)lat,
             (double)lon);
  return true;
}

}  // namespace heltec::meshcore::ui::map

#endif
