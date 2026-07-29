#include <Arduino.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "DataStore.h"

#if defined(EXTRAFS) || defined(QSPIFLASH)
  #define MAX_BLOBRECS 100
#else
  #define MAX_BLOBRECS 20
#endif

DataStore::DataStore(FILESYSTEM& fs, mesh::RTCClock& clock) : _fs(&fs), _fsExtra(nullptr), _clock(&clock),
#if defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
    identity_store(fs, "")
#elif defined(RP2040_PLATFORM)
    identity_store(fs, "/identity")
#else
    identity_store(fs, "/identity")
#endif
{
}

#if defined(EXTRAFS) || defined(QSPIFLASH)
DataStore::DataStore(FILESYSTEM& fs, FILESYSTEM& fsExtra, mesh::RTCClock& clock) : _fs(&fs), _fsExtra(&fsExtra), _clock(&clock),
#if defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
    identity_store(fs, "")
#elif defined(RP2040_PLATFORM)
    identity_store(fs, "/identity")
#else
    identity_store(fs, "/identity")
#endif
{
}
#endif

static File openWrite(FILESYSTEM* fs, const char* filename) {
#if defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
  fs->remove(filename);
  return fs->open(filename, FILE_O_WRITE);
#elif defined(RP2040_PLATFORM)
  return fs->open(filename, "w");
#else
  return fs->open(filename, "w", true);
#endif
}

void DataStore::notifyBootRegionMapStorageDone() {
#if defined(ENV_INCLUDE_COMPASS) && (ENV_INCLUDE_COMPASS) && (defined(NRF52_PLATFORM) || defined(STM32_PLATFORM))
  _allowGpsTrackFileOpen = true;
#endif
}

#if defined(ENV_INCLUDE_COMPASS) && (ENV_INCLUDE_COMPASS)

static const char kCompassCfgPath[] = "/compass_cfg";
static const char kCompassMagCalPath[] = "/compass_mag_cal";
static const char kGpsTrackPath[] = "/gps_track.csv";

/** nth comma-separated field (0-based); nullptr if missing. */
static const char* compass_cfg_field(const char* line, int index) {
  if (!line || index < 0) return nullptr;
  const char* p = line;
  for (int i = 0; i < index; ++i) {
    p = strchr(p, ',');
    if (!p) return nullptr;
    ++p;
  }
  return p;
}

static bool parse_int_field(const char* field, int* out) {
  if (!field || !*field || !out) return false;
  char* end = nullptr;
  const long v = strtol(field, &end, 10);
  if (end == field) return false;
  *out = (int)v;
  return true;
}

/** Decimal degrees (legacy) or integer micro-degrees (current). */
static bool parse_coord_field(const char* field, double& out) {
  if (!field || !*field) return false;
  char* end = nullptr;
  if (strchr(field, '.')) {
    out = strtod(field, &end);
    return end != field;
  }
  const long e6 = strtol(field, &end, 10);
  if (end == field) return false;
  out = e6 / 1000000.0;
  return true;
}

static long deg_to_e6(double deg) {
  return (long)(deg * 1000000.0 + (deg >= 0.0 ? 0.5 : -0.5));
}

bool DataStore::loadFindFriendCompassSettings(int& mode, int& wpValid, double& wpLat, double& wpLon,
                                              uint16_t& trackMinDistCm, int* friendIdx,
                                              uint32_t* advSec, uint16_t* trackIntervalMin) {
  if (!_fs || !_fs->exists(kCompassCfgPath)) return false;
#if defined(RP2040_PLATFORM)
  File f = _fs->open(kCompassCfgPath, "r");
#elif defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
  File f = _fs->open(kCompassCfgPath, FILE_O_READ);
#else
  File f = _fs->open(kCompassCfgPath);
#endif
  if (!f) return false;
  char buf[96];
  int n = f.readBytesUntil('\n', buf, sizeof(buf) - 1);
  buf[n] = 0;
  f.close();

  if (!parse_int_field(compass_cfg_field(buf, 0), &mode)) return false;
  if (!parse_int_field(compass_cfg_field(buf, 1), &wpValid)) return false;

  wpLat = 0;
  wpLon = 0;
  if (wpValid) {
    parse_coord_field(compass_cfg_field(buf, 2), wpLat);
    parse_coord_field(compass_cfg_field(buf, 3), wpLon);
  }

  int dist = 100;
  int ff_idx = -1;
  unsigned adv = 0;
  parse_int_field(compass_cfg_field(buf, 4), &dist);
  parse_int_field(compass_cfg_field(buf, 5), &ff_idx);
  int adv_i = 0;
  if (parse_int_field(compass_cfg_field(buf, 6), &adv_i) && adv_i >= 0) adv = (unsigned)adv_i;

  if (dist >= 10 && dist <= 20000) trackMinDistCm = (uint16_t)dist;
  if (friendIdx) *friendIdx = ff_idx;
  if (advSec && adv >= 30) *advSec = adv;
  if (trackIntervalMin) {
    int interval_min = 1;
    if (parse_int_field(compass_cfg_field(buf, 7), &interval_min) && interval_min >= 1 &&
        interval_min <= 120) {
      *trackIntervalMin = (uint16_t)interval_min;
    } else {
      *trackIntervalMin = 1;
    }
  }
  return true;
}

void DataStore::saveFindFriendCompassSettings(int mode, int wpValid, double wpLat, double wpLon,
                                              uint16_t trackMinDistCm, int friendIdx,
                                              uint32_t advSec, uint16_t trackIntervalMin) {
  if (!_fs) return;
#if defined(RP2040_PLATFORM)
  File f = _fs->open(kCompassCfgPath, "w");
#elif defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
  File f = _fs->open(kCompassCfgPath, FILE_O_WRITE);
  if (f) {
    f.seek(0);
    f.truncate(0);
  }
#else
  File f = _fs->open(kCompassCfgPath, "w", true);
#endif
  if (!f) return;
  if (trackIntervalMin < 1) trackIntervalMin = 1;
  char buf[128];
  snprintf(buf, sizeof(buf), "%d,%d,%ld,%ld,%u,%d,%u,%u\n", mode, wpValid,
           (long)deg_to_e6(wpLat), (long)deg_to_e6(wpLon), (unsigned)trackMinDistCm, friendIdx,
           (unsigned)advSec, (unsigned)trackIntervalMin);
  f.print(buf);
  f.close();
}

bool DataStore::hasCompassMagCal() const {
  float hmm[4];
  return loadCompassMagCal(hmm);
}

bool DataStore::loadCompassMagCal(float hmm[4]) const {
  if (!hmm || !_fs || !_fs->exists(kCompassMagCalPath)) return false;
#if defined(RP2040_PLATFORM)
  File f = _fs->open(kCompassMagCalPath, "r");
#elif defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
  File f = _fs->open(kCompassMagCalPath, FILE_O_READ);
#else
  File f = _fs->open(kCompassMagCalPath, "r");
#endif
  if (!f) return false;
  char buf[80] = {0};
  const size_t n = f.readBytes(buf, sizeof(buf) - 1);
  f.close();
  if (n < 8) return false;
  if (strncmp(buf, "CAL1,", 5) != 0) return false;
  const char* p = buf + 5;
  for (int i = 0; i < 4; ++i) {
    char* end = nullptr;
    const float v = strtof(p, &end);
    if (end == p) return false;
    hmm[i] = v;
    p = end;
    if (i < 3) {
      if (*p != ',') return false;
      ++p;
    }
  }
  return true;
}

bool DataStore::saveCompassMagCal(const float hmm[4]) {
  if (!hmm || !_fs) return false;
  File f = openWrite(_fs, kCompassMagCalPath);
  if (!f) return false;
  char buf[80];
  snprintf(buf, sizeof(buf), "CAL1,%.6f,%.6f,%.6f,%.6f\n", (double)hmm[0], (double)hmm[1],
           (double)hmm[2], (double)hmm[3]);
  const size_t n = f.print(buf);
  f.close();
  if (n == 0) return false;
  float verify[4];
  return loadCompassMagCal(verify);
}

bool DataStore::isGpsTrackRecordingOpen() const {
#if defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
  return _gpsTrackFile != nullptr && _gpsTrackFile->isOpen();
#else
  return _gpsTrackFileOpen;
#endif
}

bool DataStore::beginGpsTrackRecording(uint32_t startUtcEpoch) {
  if (isGpsTrackRecordingOpen()) return true;
  if (!_fs) return false;
#if defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
  if (!_allowGpsTrackFileOpen) return false;
  _gpsTrackFile = new File(_fs->open(kGpsTrackPath, FILE_O_WRITE));
  if (!_gpsTrackFile || !(*_gpsTrackFile)) {
    delete _gpsTrackFile;
    _gpsTrackFile = nullptr;
    return false;
  }
  _gpsTrackFile->printf("START,%lu\n", (unsigned long)startUtcEpoch);
#elif defined(RP2040_PLATFORM)
  _gpsTrackFile = _fs->open(kGpsTrackPath, "a");
  if (!_gpsTrackFile) return false;
  _gpsTrackFile.printf("START,%lu\n", (unsigned long)startUtcEpoch);
  _gpsTrackFileOpen = true;
#else
  _gpsTrackFile = _fs->open(kGpsTrackPath, "a", true);
  if (!_gpsTrackFile) return false;
  _gpsTrackFile.printf("START,%lu\n", (unsigned long)startUtcEpoch);
  _gpsTrackFileOpen = true;
#endif
  return true;
}

void DataStore::endGpsTrackRecording(uint32_t endUtcEpoch) {
#if defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
  if (!_gpsTrackFile) return;
  if (endUtcEpoch) {
    _gpsTrackFile->printf("END,%lu\n", (unsigned long)endUtcEpoch);
  } else {
    _gpsTrackFile->print("END,0\n");
  }
  _gpsTrackFile->close();
  delete _gpsTrackFile;
  _gpsTrackFile = nullptr;
#else
  if (!_gpsTrackFileOpen) return;
  if (endUtcEpoch) {
    _gpsTrackFile.printf("END,%lu\n", (unsigned long)endUtcEpoch);
  } else {
    _gpsTrackFile.print("END,0\n");
  }
  _gpsTrackFile.close();
  _gpsTrackFileOpen = false;
#endif
}

bool DataStore::appendGpsTrackPoint(uint32_t offMs, int32_t latE6, int32_t lonE6) {
#if defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
  if (!_gpsTrackFile || !(*_gpsTrackFile)) return false;
  _gpsTrackFile->printf("P,%lu,%ld,%ld\n", (unsigned long)offMs, (long)latE6, (long)lonE6);
#else
  if (!_gpsTrackFileOpen) return false;
  _gpsTrackFile.printf("P,%lu,%ld,%ld\n", (unsigned long)offMs, (long)latE6, (long)lonE6);
#endif
  return true;
}

void DataStore::closeFindFriendGpsTrackIfOpen() {
#if defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
  if (_gpsTrackFile) {
    _gpsTrackFile->close();
    delete _gpsTrackFile;
    _gpsTrackFile = nullptr;
  }
#else
  if (_gpsTrackFileOpen) {
    _gpsTrackFile.close();
    _gpsTrackFileOpen = false;
  }
#endif
}

static void gpsTrimEndCrLf(char* line) {
  size_t n = strlen(line);
  while (n > 0 && (line[n - 1] == '\r' || line[n - 1] == '\n')) line[--n] = '\0';
}

static bool gpsReadLine(File& f, char* buf, size_t bufSz, int& outLen) {
  if (bufSz < 2) return false;
  outLen = f.readBytesUntil('\n', buf, (int)bufSz - 1);
  if (outLen < 0) outLen = 0;
  buf[outLen] = '\0';
  gpsTrimEndCrLf(buf);
  if (outLen == 0 && !f.available()) return false;
  return true;
}

static bool gpsTrackIsStart(const char* line) {
  return line && strncmp(line, "START,", 6) == 0;
}

static bool gpsTrackIsSegmentEnd(const char* line) {
  return line && (strncmp(line, "START,", 6) == 0 || strncmp(line, "END,", 4) == 0);
}

static bool gpsParseTrackStart(const char* line, uint32_t& outStartTs) {
  if (!gpsTrackIsStart(line)) return false;
  char* end = nullptr;
  const unsigned long v = strtoul(line + 6, &end, 10);
  if (end == line + 6) return false;
  outStartTs = (uint32_t)v;
  return true;
}

static bool gpsParseTrackEnd(const char* line, uint32_t& outEndTs) {
  if (!line || strncmp(line, "END,", 4) != 0) return false;
  char* end = nullptr;
  const unsigned long v = strtoul(line + 4, &end, 10);
  if (end == line + 4) return false;
  outEndTs = (uint32_t)v;
  return true;
}

static bool gpsParseTrackPoint(const char* line, GpsTrackExportPoint& outPoint) {
  if (!line || line[0] != 'P' || line[1] != ',') return false;

  char* end = nullptr;
  const unsigned long tm = strtoul(line + 2, &end, 10);
  if (end == line + 2 || *end != ',') return false;

  const char* lat = end + 1;
  const long la = strtol(lat, &end, 10);
  if (end == lat || *end != ',') return false;

  const char* lon = end + 1;
  const long lo = strtol(lon, &end, 10);
  if (end == lon) return false;

  outPoint.t_ms = (uint32_t)tm;
  outPoint.lat_e6 = (int32_t)la;
  outPoint.lon_e6 = (int32_t)lo;
  return true;
}

GpsTrackExportResult DataStore::readGpsTrackExportPage(uint32_t point_offset, uint32_t point_limit,
                                                       uint32_t& out_start_ts, uint32_t& out_end_ts,
                                                       uint32_t& out_total_points, uint32_t& out_returned,
                                                       bool& out_more, GpsTrackExportPoint* out_points,
                                                       size_t max_out_points) {
  out_start_ts = 0;
  out_end_ts = 0;
  out_total_points = 0;
  out_returned = 0;
  out_more = false;
  if (!_fs) return GpsTrackExportResult::NoFs;
  if (!out_points || max_out_points == 0) return GpsTrackExportResult::BadFormat;
  if (point_limit == 0) point_limit = 8;
  if (point_limit > 32) point_limit = 32;
  if (point_limit > max_out_points) point_limit = (uint32_t)max_out_points;

  if (!_fs->exists(kGpsTrackPath)) return GpsTrackExportResult::NoFile;

  File f = openRead(kGpsTrackPath);
  if (!f) return GpsTrackExportResult::NoFile;

  uint32_t seg_start = 0;
  bool have_start = false;
  char line[144];

  while (true) {
    const uint32_t line_begin = (uint32_t)f.position();
    int n = 0;
    if (!gpsReadLine(f, line, sizeof(line), n)) {
      if (!f.available()) break;
      continue;
    }
    if (gpsTrackIsStart(line)) {
      seg_start = line_begin;
      have_start = true;
    }
    if (!f.available()) break;
  }

  if (!have_start) {
    f.close();
    return GpsTrackExportResult::Empty;
  }

  if (!f.seek(seg_start)) {
    f.close();
    return GpsTrackExportResult::IoError;
  }

  int ln = 0;
  if (!gpsReadLine(f, line, sizeof(line), ln)) {
    f.close();
    return GpsTrackExportResult::BadFormat;
  }
  if (!gpsParseTrackStart(line, out_start_ts)) {
    f.close();
    return GpsTrackExportResult::BadFormat;
  }

  out_total_points = 0;
  uint32_t copied = 0;
  uint32_t last_t_ms = 0;
  bool have_end = false;
  uint32_t end_ts = 0;
  while (true) {
    if (!gpsReadLine(f, line, sizeof(line), ln)) {
      if (!f.available()) break;
      continue;
    }
    if (line[0] == '\0') continue;
    if (gpsTrackIsStart(line)) break;
    if (gpsParseTrackEnd(line, end_ts)) {
      have_end = true;
      break;
    }

    GpsTrackExportPoint point;
    if (gpsParseTrackPoint(line, point)) {
      last_t_ms = point.t_ms;
      if (out_total_points >= point_offset && copied < point_limit) out_points[copied++] = point;
      out_total_points++;
    } else if (line[0] == 'P' && line[1] == ',') {
      f.close();
      return GpsTrackExportResult::BadFormat;
    }
  }

  f.close();

  if (have_end && end_ts) {
    out_end_ts = end_ts;
  } else if (out_total_points > 0) {
    out_end_ts = out_start_ts + last_t_ms / 1000U;
  } else {
    out_end_ts = out_start_ts;
  }

  out_returned = copied;
  out_more = (point_offset + copied) < out_total_points;
  return GpsTrackExportResult::Ok;
}

#endif // ENV_INCLUDE_COMPASS

#if defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
  static uint32_t _ContactsChannelsTotalBlocks = 0;
#endif

void DataStore::begin() {
#if defined(RP2040_PLATFORM)
  identity_store.begin();
#endif

#if defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
  _ContactsChannelsTotalBlocks = _getContactsChannelsFS()->_getFS()->cfg->block_count;
  checkAdvBlobFile();
  #if defined(EXTRAFS) || defined(QSPIFLASH)
  migrateToSecondaryFS();
  #endif
#else
  // init 'blob store' support
  _fs->mkdir("/bl");
#endif

  (void)ensureMessageHistory();
}

#if defined(ESP32)
  #include <SPIFFS.h>
  #include <nvs_flash.h>
#elif defined(RP2040_PLATFORM)
  #include <LittleFS.h>
#elif defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
  #if defined(QSPIFLASH)
    #include <CustomLFS_QSPIFlash.h>
  #elif defined(EXTRAFS)
    #include <CustomLFS.h>
  #else 
    #include <InternalFileSystem.h>
  #endif
#endif

#if defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
int _countLfsBlock(void *p, lfs_block_t block){
      if (block > _ContactsChannelsTotalBlocks) {
        MESH_DEBUG_PRINTLN("ERROR: Block %d exceeds filesystem bounds - CORRUPTION DETECTED!", block);
        return LFS_ERR_CORRUPT;  // return error to abort lfs_traverse() gracefully
    }
  lfs_size_t *size = (lfs_size_t*) p;
  *size += 1;
    return 0;
}

lfs_ssize_t _getLfsUsedBlockCount(FILESYSTEM* fs) {
  lfs_size_t size = 0;
  int err = lfs_traverse(fs->_getFS(), _countLfsBlock, &size);
  if (err) {
    MESH_DEBUG_PRINTLN("ERROR: lfs_traverse() error: %d", err);
    return 0;
  }
  return size;
}
#endif

uint32_t DataStore::getStorageUsedKb() const {
#if defined(ESP32)
  return SPIFFS.usedBytes() / 1024;
#elif defined(RP2040_PLATFORM)
  FSInfo info;
  info.usedBytes = 0;
  _fs->info(info);
  return info.usedBytes / 1024;
#elif defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
  const lfs_config* config = _getContactsChannelsFS()->_getFS()->cfg;
  int usedBlockCount = _getLfsUsedBlockCount(_getContactsChannelsFS());
  int usedBytes = config->block_size * usedBlockCount;
  return usedBytes / 1024;
#else
  return 0;
#endif
}

uint32_t DataStore::getStorageTotalKb() const {
#if defined(ESP32)
  return SPIFFS.totalBytes() / 1024;
#elif defined(RP2040_PLATFORM)
  FSInfo info;
  info.totalBytes = 0;
  _fs->info(info);
  return info.totalBytes / 1024;
#elif defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
  const lfs_config* config = _getContactsChannelsFS()->_getFS()->cfg;
  int totalBytes = config->block_size * config->block_count;
  return totalBytes / 1024;
#else
  return 0;
#endif
}

File DataStore::openRead(const char* filename) {
#if defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
  return _fs->open(filename, FILE_O_READ);
#elif defined(RP2040_PLATFORM)
  return _fs->open(filename, "r");
#else
  return _fs->open(filename, "r", false);
#endif
}

File DataStore::openRead(FILESYSTEM* fs, const char* filename) {
#if defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
  return fs->open(filename, FILE_O_READ);
#elif defined(RP2040_PLATFORM)
  return fs->open(filename, "r");
#else
  return fs->open(filename, "r", false);
#endif
}

bool DataStore::removeFile(const char* filename) {
  return _fs->remove(filename);
}

bool DataStore::removeFile(FILESYSTEM* fs, const char* filename) {
  return fs->remove(filename);
}

bool DataStore::formatFileSystem() {
#if defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
  if (_fsExtra == nullptr) {
    return _fs->format();
  } else {
    return _fs->format() && _fsExtra->format();
  }
#elif defined(RP2040_PLATFORM)
  return LittleFS.format();
#elif defined(ESP32)
  bool fs_success = ((fs::SPIFFSFS *)_fs)->format();
  esp_err_t nvs_err = nvs_flash_erase(); // no need to reinit, will be done by reboot
  return fs_success && (nvs_err == ESP_OK);
#else
  #error "need to implement format()"
#endif
}

bool DataStore::loadMainIdentity(mesh::LocalIdentity &identity) {
  return identity_store.load("_main", identity);
}

bool DataStore::saveMainIdentity(const mesh::LocalIdentity &identity) {
  return identity_store.save("_main", identity);
}

void DataStore::loadPrefs(NodePrefs& prefs, double& node_lat, double& node_lon) {
  if (_fs && _fs->exists("/new_prefs")) {
    loadPrefsInt("/new_prefs", prefs, node_lat, node_lon); // new filename
    return;
  }
  if (_fsExtra && _fsExtra->exists("/new_prefs")) {
    loadPrefsInt("/new_prefs", prefs, node_lat, node_lon); // new filename
    return;
  }
  if (_fs && _fs->exists("/node_prefs")) {
    loadPrefsInt("/node_prefs", prefs, node_lat, node_lon);
    savePrefs(prefs, node_lat, node_lon);                // save to new filename
    _fs->remove("/node_prefs"); // remove old
  }
}

void DataStore::loadPrefsInt(const char *filename, NodePrefs& _prefs, double& node_lat, double& node_lon) {
  File file = openRead(_fs, filename);
  if (!file && _fsExtra) {
    file = openRead(_fsExtra, filename);
  }
  if (file) {
    uint8_t pad[8];

    file.read((uint8_t *)&_prefs.airtime_factor, sizeof(float));                            // 0
    file.read((uint8_t *)_prefs.node_name, sizeof(_prefs.node_name));                       // 4
    file.read(pad, 4);                                                                      // 36
    file.read((uint8_t *)&node_lat, sizeof(node_lat));                                      // 40
    file.read((uint8_t *)&node_lon, sizeof(node_lon));                                      // 48
    file.read((uint8_t *)&_prefs.freq, sizeof(_prefs.freq));                                // 56
    file.read((uint8_t *)&_prefs.sf, sizeof(_prefs.sf));                                    // 60
    file.read((uint8_t *)&_prefs.cr, sizeof(_prefs.cr));                                    // 61
    file.read((uint8_t *)&_prefs.client_repeat, sizeof(_prefs.client_repeat));              // 62
    file.read((uint8_t *)&_prefs.manual_add_contacts, sizeof(_prefs.manual_add_contacts));  // 63
    file.read((uint8_t *)&_prefs.bw, sizeof(_prefs.bw));                                    // 64
    file.read((uint8_t *)&_prefs.tx_power_dbm, sizeof(_prefs.tx_power_dbm));                // 68
    file.read((uint8_t *)&_prefs.telemetry_mode_base, sizeof(_prefs.telemetry_mode_base));  // 69
    file.read((uint8_t *)&_prefs.telemetry_mode_loc, sizeof(_prefs.telemetry_mode_loc));    // 70
    file.read((uint8_t *)&_prefs.telemetry_mode_env, sizeof(_prefs.telemetry_mode_env));    // 71
    file.read((uint8_t *)&_prefs.rx_delay_base, sizeof(_prefs.rx_delay_base));              // 72
    file.read((uint8_t *)&_prefs.advert_loc_policy, sizeof(_prefs.advert_loc_policy));      // 76
    file.read((uint8_t *)&_prefs.multi_acks, sizeof(_prefs.multi_acks));                    // 77
    file.read((uint8_t *)&_prefs.path_hash_mode, sizeof(_prefs.path_hash_mode));            // 78
    file.read(pad, 1);                                                                      // 79
    file.read((uint8_t *)&_prefs.ble_pin, sizeof(_prefs.ble_pin));                          // 80
    file.read((uint8_t *)&_prefs.buzzer_quiet, sizeof(_prefs.buzzer_quiet));                // 84
    file.read((uint8_t *)&_prefs.gps_enabled, sizeof(_prefs.gps_enabled));                  // 85
    file.read((uint8_t *)&_prefs.gps_interval, sizeof(_prefs.gps_interval));                // 86
    file.read((uint8_t *)&_prefs.autoadd_config, sizeof(_prefs.autoadd_config));            // 87
    file.read((uint8_t *)&_prefs.lora_band_configured, sizeof(_prefs.lora_band_configured));// 88
    if (file.available()) {
      file.read((uint8_t *)&_prefs.display_auto_off_sec, sizeof(_prefs.display_auto_off_sec)); // 89
    } else {
      _prefs.display_auto_off_sec = 0;
    }
    if (file.available()) {
      file.read((uint8_t *)&_prefs.companion_link_enabled, sizeof(_prefs.companion_link_enabled)); // 90
    } else {
      _prefs.companion_link_enabled = 1;
    }
    if (file.available()) {
      file.read((uint8_t *)&_prefs.loc_share_adv_sec, sizeof(_prefs.loc_share_adv_sec)); // 91
    } else {
      _prefs.loc_share_adv_sec = 60;
    }
    if (file.available()) {
      file.read((uint8_t *)&_prefs.gps_track_armed, sizeof(_prefs.gps_track_armed)); // 92
    } else {
      _prefs.gps_track_armed = 0;
    }
    if (file.available()) {
      file.read((uint8_t *)&_prefs.buzzer_volume_level, sizeof(_prefs.buzzer_volume_level)); // 93
    } else {
      _prefs.buzzer_volume_level = 3;
    }
    if (file.available()) {
      file.read((uint8_t *)&_prefs.lna_enabled, sizeof(_prefs.lna_enabled)); // 94
    } else {
      _prefs.lna_enabled = 0;
    }

    file.close();
  }
}

void DataStore::savePrefs(const NodePrefs& _prefs, double node_lat, double node_lon) {
#if defined(ESP32)
  if (_fs && _fs->exists("/new_prefs")) {
    _fs->remove("/new_prefs");
  }
#endif
  File file = openWrite(_fs, "/new_prefs");
  bool usingSecondary = false;
  if (!file && _fsExtra) {
    file = openWrite(_fsExtra, "/new_prefs");
    usingSecondary = (bool)file;
  }
  if (file) {
    uint8_t pad[8];
    memset(pad, 0, sizeof(pad));

    file.write((uint8_t *)&_prefs.airtime_factor, sizeof(float));                             // 0
    file.write((uint8_t *)_prefs.node_name, sizeof(_prefs.node_name));                        // 4
    file.write(pad, 4);                                                                       // 36
    file.write((uint8_t *)&node_lat, sizeof(node_lat));                                       // 40
    file.write((uint8_t *)&node_lon, sizeof(node_lon));                                       // 48
    file.write((uint8_t *)&_prefs.freq, sizeof(_prefs.freq));                                 // 56
    file.write((uint8_t *)&_prefs.sf, sizeof(_prefs.sf));                                     // 60
    file.write((uint8_t *)&_prefs.cr, sizeof(_prefs.cr));                                     // 61
    file.write((uint8_t *)&_prefs.client_repeat, sizeof(_prefs.client_repeat));               // 62
    file.write((uint8_t *)&_prefs.manual_add_contacts, sizeof(_prefs.manual_add_contacts));   // 63
    file.write((uint8_t *)&_prefs.bw, sizeof(_prefs.bw));                                     // 64
    file.write((uint8_t *)&_prefs.tx_power_dbm, sizeof(_prefs.tx_power_dbm));                 // 68
    file.write((uint8_t *)&_prefs.telemetry_mode_base, sizeof(_prefs.telemetry_mode_base));   // 69
    file.write((uint8_t *)&_prefs.telemetry_mode_loc, sizeof(_prefs.telemetry_mode_loc));     // 70
    file.write((uint8_t *)&_prefs.telemetry_mode_env, sizeof(_prefs.telemetry_mode_env));     // 71
    file.write((uint8_t *)&_prefs.rx_delay_base, sizeof(_prefs.rx_delay_base));               // 72
    file.write((uint8_t *)&_prefs.advert_loc_policy, sizeof(_prefs.advert_loc_policy));       // 76
    file.write((uint8_t *)&_prefs.multi_acks, sizeof(_prefs.multi_acks));                     // 77
    file.write((uint8_t *)&_prefs.path_hash_mode, sizeof(_prefs.path_hash_mode));             // 78
    file.write(pad, 1);                                                                       // 79
    file.write((uint8_t *)&_prefs.ble_pin, sizeof(_prefs.ble_pin));                           // 80
    file.write((uint8_t *)&_prefs.buzzer_quiet, sizeof(_prefs.buzzer_quiet));                 // 84
    file.write((uint8_t *)&_prefs.gps_enabled, sizeof(_prefs.gps_enabled));                   // 85
    file.write((uint8_t *)&_prefs.gps_interval, sizeof(_prefs.gps_interval));                 // 86
    file.write((uint8_t *)&_prefs.autoadd_config, sizeof(_prefs.autoadd_config));             // 87
    file.write((uint8_t *)&_prefs.lora_band_configured, sizeof(_prefs.lora_band_configured)); // 88
    file.write((uint8_t *)&_prefs.display_auto_off_sec, sizeof(_prefs.display_auto_off_sec));   // 89
    file.write((uint8_t *)&_prefs.companion_link_enabled, sizeof(_prefs.companion_link_enabled)); // 90
    file.write((uint8_t *)&_prefs.loc_share_adv_sec, sizeof(_prefs.loc_share_adv_sec));           // 91
    file.write((uint8_t *)&_prefs.gps_track_armed, sizeof(_prefs.gps_track_armed));               // 92
    file.write((uint8_t *)&_prefs.buzzer_volume_level, sizeof(_prefs.buzzer_volume_level));       // 93
    file.write((uint8_t *)&_prefs.lna_enabled, sizeof(_prefs.lna_enabled));                         // 94

    file.close();
  }
}

void DataStore::loadContacts(DataStoreHost* host) {
File file = openRead(_getContactsChannelsFS(), "/contacts3");
    if (file) {
      bool full = false;
      while (!full) {
        ContactInfo c;
        uint8_t pub_key[32];
        uint8_t unused;

        bool success = (file.read(pub_key, 32) == 32);
        success = success && (file.read((uint8_t *)&c.name, 32) == 32);
        success = success && (file.read(&c.type, 1) == 1);
        success = success && (file.read(&c.flags, 1) == 1);
        success = success && (file.read(&unused, 1) == 1);
        success = success && (file.read((uint8_t *)&c.sync_since, 4) == 4); // was 'reserved'
        success = success && (file.read((uint8_t *)&c.out_path_len, 1) == 1);
        success = success && (file.read((uint8_t *)&c.last_advert_timestamp, 4) == 4);
        success = success && (file.read(c.out_path, 64) == 64);
        success = success && (file.read((uint8_t *)&c.lastmod, 4) == 4);
        success = success && (file.read((uint8_t *)&c.gps_lat, 4) == 4);
        success = success && (file.read((uint8_t *)&c.gps_lon, 4) == 4);

        if (!success) break; // EOF

        c.name[sizeof(c.name) - 1] = '\0';
        c.id = mesh::Identity(pub_key);
        if (!host->onContactLoaded(c)) full = true;
      }
      file.close();
    }
}

void DataStore::saveContacts(DataStoreHost* host) {
  File file = openWrite(_getContactsChannelsFS(), "/contacts3");
  if (file) {
    uint32_t idx = 0;
    ContactInfo c;
    uint8_t unused = 0;

    while (host->getContactForSave(idx, c)) {
      bool success = (file.write(c.id.pub_key, 32) == 32);
      success = success && (file.write((uint8_t *)&c.name, 32) == 32);
      success = success && (file.write(&c.type, 1) == 1);
      success = success && (file.write(&c.flags, 1) == 1);
      success = success && (file.write(&unused, 1) == 1);
      success = success && (file.write((uint8_t *)&c.sync_since, 4) == 4);
      success = success && (file.write((uint8_t *)&c.out_path_len, 1) == 1);
      success = success && (file.write((uint8_t *)&c.last_advert_timestamp, 4) == 4);
      success = success && (file.write(c.out_path, 64) == 64);
      success = success && (file.write((uint8_t *)&c.lastmod, 4) == 4);
      success = success && (file.write((uint8_t *)&c.gps_lat, 4) == 4);
      success = success && (file.write((uint8_t *)&c.gps_lon, 4) == 4);

      if (!success) break; // write failed

      idx++;  // advance to next contact
    }
    file.close();
  }
}

void DataStore::loadChannels(DataStoreHost* host) {
    File file = openRead(_getContactsChannelsFS(), "/channels2");
    if (file) {
      bool full = false;
      uint8_t channel_idx = 0;
      while (!full) {
        ChannelDetails ch;
        uint8_t unused[4];

        bool success = (file.read(unused, 4) == 4);
        success = success && (file.read((uint8_t *)ch.name, 32) == 32);
        success = success && (file.read((uint8_t *)ch.channel.secret, 32) == 32);

        if (!success) break; // EOF

        if (host->onChannelLoaded(channel_idx, ch)) {
          channel_idx++;
        } else {
          full = true;
        }
      }
      file.close();
    }
}

void DataStore::saveChannels(DataStoreHost* host) {
  File file = openWrite(_getContactsChannelsFS(), "/channels2");
  if (file) {
    uint8_t channel_idx = 0;
    ChannelDetails ch;
    uint8_t unused[4];
    memset(unused, 0, 4);

    while (host->getChannelForSave(channel_idx, ch)) {
      bool success = (file.write(unused, 4) == 4);
      success = success && (file.write((uint8_t *)ch.name, 32) == 32);
      success = success && (file.write((uint8_t *)ch.channel.secret, 32) == 32);

      if (!success) break; // write failed
      channel_idx++;
    }
    file.close();
  }
}

#if defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)

#define MAX_ADVERT_PKT_LEN   (2 + 32 + PUB_KEY_SIZE + 4 + SIGNATURE_SIZE + MAX_ADVERT_DATA_SIZE)

struct BlobRec {
  uint32_t timestamp;
  uint8_t  key[7];
  uint8_t  len;
  uint8_t  data[MAX_ADVERT_PKT_LEN];
};

void DataStore::checkAdvBlobFile() {
  if (!_getContactsChannelsFS()->exists("/adv_blobs")) {
    File file = openWrite(_getContactsChannelsFS(), "/adv_blobs");
    if (file) {
      BlobRec zeroes;
      memset(&zeroes, 0, sizeof(zeroes));
      for (int i = 0; i < MAX_BLOBRECS; i++) {     // pre-allocate to fixed size
        file.write((uint8_t *) &zeroes, sizeof(zeroes));
      }
      file.close();
    }
  }
}

void DataStore::migrateToSecondaryFS() {
  // migrate old adv_blobs, contacts3 and channels2 files to secondary FS if they don't already exist
  if (!_fsExtra->exists("/adv_blobs")) {
    if (_fs->exists("/adv_blobs")) {
    File oldAdvBlobs = openRead(_fs, "/adv_blobs");
    File newAdvBlobs = openWrite(_fsExtra, "/adv_blobs");

    if (oldAdvBlobs && newAdvBlobs) {
      BlobRec rec;
      size_t count = 0;

      // Copy 20 BlobRecs from old to new
      while (count < 20 && oldAdvBlobs.read((uint8_t *)&rec, sizeof(rec)) == sizeof(rec)) {
        newAdvBlobs.seek(count * sizeof(BlobRec));
        newAdvBlobs.write((uint8_t *)&rec, sizeof(rec));
        count++;
      }
    }
    if (oldAdvBlobs) oldAdvBlobs.close();
    if (newAdvBlobs) newAdvBlobs.close();
    _fs->remove("/adv_blobs");
    }
  }
  if (!_fsExtra->exists("/contacts3")) {
    if (_fs->exists("/contacts3")) {
      File oldFile = openRead(_fs, "/contacts3");
      File newFile = openWrite(_fsExtra, "/contacts3");

      if (oldFile && newFile) {
        uint8_t buf[64];
        int n;
        while ((n = oldFile.read(buf, sizeof(buf))) > 0) {
          newFile.write(buf, n);
        }
      }
      if (oldFile) oldFile.close();
      if (newFile) newFile.close();
      _fs->remove("/contacts3");
    }
  }
  if (!_fsExtra->exists("/channels2")) {
    if (_fs->exists("/channels2")) {
      File oldFile = openRead(_fs, "/channels2");
      File newFile = openWrite(_fsExtra, "/channels2");

      if (oldFile && newFile) {
        uint8_t buf[64];
        int n;
        while ((n = oldFile.read(buf, sizeof(buf))) > 0) {
          newFile.write(buf, n);
        }
      }
      if (oldFile) oldFile.close();
      if (newFile) newFile.close();
      _fs->remove("/channels2");
    }
  }
  // cleanup nodes which have been testing the extra fs, copy _main.id and new_prefs back to primary
  if (_fsExtra->exists("/_main.id")) {
      if (_fs->exists("/_main.id")) {_fs->remove("/_main.id");}
      File oldFile = openRead(_fsExtra, "/_main.id");
      File newFile = openWrite(_fs, "/_main.id");

      if (oldFile && newFile) {
        uint8_t buf[64];
        int n;
        while ((n = oldFile.read(buf, sizeof(buf))) > 0) {
          newFile.write(buf, n);
        }
      }
      if (oldFile) oldFile.close();
      if (newFile) newFile.close();
      // Only remove from secondary if copy to primary succeeded.
      if (oldFile && newFile) {
        _fsExtra->remove("/_main.id");
      }
  }
  if (_fsExtra->exists("/new_prefs")) {
    if (_fs->exists("/new_prefs")) {_fs->remove("/new_prefs");}
      File oldFile = openRead(_fsExtra, "/new_prefs");
      File newFile = openWrite(_fs, "/new_prefs");

      if (oldFile && newFile) {
        uint8_t buf[64];
        int n;
        while ((n = oldFile.read(buf, sizeof(buf))) > 0) {
          newFile.write(buf, n);
        }
      }
      if (oldFile) oldFile.close();
      if (newFile) newFile.close();
      // Only remove from secondary if copy to primary succeeded.
      if (oldFile && newFile) {
        _fsExtra->remove("/new_prefs");
      }
  }
  // remove files from where they should not be anymore
  if (_fs->exists("/adv_blobs")) {
    _fs->remove("/adv_blobs");
  }
  if (_fs->exists("/contacts3")) {
    _fs->remove("/contacts3");
  }
  if (_fs->exists("/channels2")) {
    _fs->remove("/channels2");
  }
  // Keep these on secondary if primary copy failed above.
}

uint8_t DataStore::getBlobByKey(const uint8_t key[], int key_len, uint8_t dest_buf[]) {
  File file = openRead(_getContactsChannelsFS(), "/adv_blobs");
  uint8_t len = 0;  // 0 = not found
  if (file) {
    BlobRec tmp;
    while (file.read((uint8_t *) &tmp, sizeof(tmp)) == sizeof(tmp)) {
      if (memcmp(key, tmp.key, sizeof(tmp.key)) == 0) {  // only match by 7 byte prefix
        len = tmp.len;
        memcpy(dest_buf, tmp.data, len);
        break;
      }
    }
    file.close();
  }
  return len;
}

bool DataStore::putBlobByKey(const uint8_t key[], int key_len, const uint8_t src_buf[], uint8_t len) {
  if (len < PUB_KEY_SIZE+4+SIGNATURE_SIZE || len > MAX_ADVERT_PKT_LEN) return false;
  checkAdvBlobFile();
  File file = _getContactsChannelsFS()->open("/adv_blobs", FILE_O_WRITE);
  if (file) {
    uint32_t pos = 0, found_pos = 0;
    uint32_t min_timestamp = 0xFFFFFFFF;

    // search for matching key OR evict by oldest timestmap
    BlobRec tmp;
    file.seek(0);
    while (file.read((uint8_t *) &tmp, sizeof(tmp)) == sizeof(tmp)) {
      if (memcmp(key, tmp.key, sizeof(tmp.key)) == 0) {  // only match by 7 byte prefix
        found_pos = pos;
        break;
      }
      if (tmp.timestamp < min_timestamp) {
        min_timestamp = tmp.timestamp;
        found_pos = pos;
      }

      pos += sizeof(tmp);
    }

    memcpy(tmp.key, key, sizeof(tmp.key));  // just record 7 byte prefix of key
    memcpy(tmp.data, src_buf, len);
    tmp.len = len;
    tmp.timestamp = _clock->getCurrentTime();

    file.seek(found_pos);
    file.write((uint8_t *) &tmp, sizeof(tmp));

    file.close();
    return true;
  }
  return false; // error
}
bool DataStore::deleteBlobByKey(const uint8_t key[], int key_len) {
  return true; // this is just a stub on NRF52/STM32 platforms
}
#else
inline void makeBlobPath(const uint8_t key[], int key_len, char* path, size_t path_size) {
  char fname[18];
  if (key_len > 8) key_len = 8; // just use first 8 bytes (prefix)
  mesh::Utils::toHex(fname, key, key_len);
  snprintf(path, path_size, "/bl/%s", fname);
}

uint8_t DataStore::getBlobByKey(const uint8_t key[], int key_len, uint8_t dest_buf[]) {
  char path[64];
  makeBlobPath(key, key_len, path, sizeof(path));

  if (_fs->exists(path)) {
    File f = openRead(_fs, path);
    if (f) {
      int len = f.read(dest_buf, 255); // currently MAX 255 byte blob len supported!!
      f.close();
      return len;
    }
  }
  return 0; // not found
}

bool DataStore::putBlobByKey(const uint8_t key[], int key_len, const uint8_t src_buf[], uint8_t len) {
  char path[64];
  makeBlobPath(key, key_len, path, sizeof(path));

  File f = openWrite(_fs, path);
  if (f) {
    int n = f.write(src_buf, len);
    f.close();
    if (n == len) return true; // success!

    _fs->remove(path); // blob was only partially written!
  }
  return false; // error
}

bool DataStore::deleteBlobByKey(const uint8_t key[], int key_len) {
  char path[64];
  makeBlobPath(key, key_len, path, sizeof(path));

  _fs->remove(path);
  
  return true; // return true even if file did not exist
}
#endif
