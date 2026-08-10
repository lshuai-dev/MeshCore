#pragma once

#include <helpers/IdentityStore.h>
#include <helpers/ContactInfo.h>
#include <helpers/ChannelDetails.h>
#include "config/NodePrefs.h"
#include <stddef.h>

#if defined(ENV_INCLUDE_GPS) && (ENV_INCLUDE_GPS)
struct GpsTrackExportPoint {
  uint32_t t_ms;
  int32_t lat_e6;
  int32_t lon_e6;
};

enum class GpsTrackExportResult : uint8_t {
  Ok = 0,
  NoFs,
  NoFile,
  Empty,
  IoError,
  BadFormat,
};
#endif

class DataStoreHost {
public:
  virtual bool onContactLoaded(const ContactInfo& contact) =0;
  virtual bool getContactForSave(uint32_t idx, ContactInfo& contact) =0;
  virtual bool onChannelLoaded(uint8_t channel_idx, const ChannelDetails& ch) =0;
  virtual bool getChannelForSave(uint8_t channel_idx, ChannelDetails& ch) =0;
};

class DataStore {
  FILESYSTEM* _fs;
  FILESYSTEM* _fsExtra;
  mesh::RTCClock* _clock;
  IdentityStore identity_store;

  void loadPrefsInt(const char *filename, NodePrefs& prefs, double& node_lat, double& node_lon);
#if defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
  void checkAdvBlobFile();
#endif

public:
  DataStore(FILESYSTEM& fs, mesh::RTCClock& clock);
  DataStore(FILESYSTEM& fs, FILESYSTEM& fsExtra, mesh::RTCClock& clock);
  void begin();
  bool formatFileSystem();
  FILESYSTEM* getPrimaryFS() const { return _fs; }
  FILESYSTEM* getSecondaryFS() const { return _fsExtra; }

  /** After boot FS ready; allows GPS track log to open on primary LittleFS (one file open/volume). */
  void notifyBootRegionMapStorageDone();
#if defined(ENV_INCLUDE_COMPASS) && (ENV_INCLUDE_COMPASS)
  bool loadFindFriendCompassSettings(int& mode, int& wpValid, double& wpLat, double& wpLon,
                                     uint16_t& trackMinDistCm, int* friendIdx = nullptr,
                                     uint16_t* trackIntervalMin = nullptr,
                                     bool* enabled = nullptr,
                                     uint8_t* friendPubKey = nullptr,
                                     bool* friendPubKeyValid = nullptr);
  void saveFindFriendCompassSettings(int mode, int wpValid, double wpLat, double wpLon,
                                     uint16_t trackMinDistCm, int friendIdx = -1,
                                     uint16_t trackIntervalMin = 1,
                                     bool enabled = false,
                                     const uint8_t* friendPubKey = nullptr);
  /** Memsic soft-iron Hmm[4] from figure-8 calibration (/compass_mag_cal). */
  bool hasCompassMagCal() const;
  bool loadCompassMagCal(float hmm[4]) const;
  bool saveCompassMagCal(const float hmm[4]);
#endif
#if defined(ENV_INCLUDE_GPS) && (ENV_INCLUDE_GPS)
  bool isGpsTrackRecordingOpen() const;
  bool beginGpsTrackRecording(uint32_t startUtcEpoch);
  void endGpsTrackRecording(uint32_t endUtcEpoch);
  bool appendGpsTrackPoint(uint32_t offMs, int32_t latE6, int32_t lonE6);
  void closeFindFriendGpsTrackIfOpen();
  /** Export one page of points from last START segment in `/gps_track.csv`. Do not call while `isGpsTrackRecordingOpen()`. */
  GpsTrackExportResult readGpsTrackExportPage(uint32_t point_offset, uint32_t point_limit, uint32_t& out_start_ts,
                                              uint32_t& out_end_ts, uint32_t& out_total_points, uint32_t& out_returned,
                                              bool& out_more, GpsTrackExportPoint* out_points, size_t max_out_points);
#endif
  bool loadMainIdentity(mesh::LocalIdentity &identity);
  bool saveMainIdentity(const mesh::LocalIdentity &identity);
  void loadPrefs(NodePrefs& prefs, double& node_lat, double& node_lon);
  void savePrefs(const NodePrefs& prefs, double node_lat, double node_lon);
  void loadContacts(DataStoreHost* host);
  void saveContacts(DataStoreHost* host);
  void loadChannels(DataStoreHost* host);
  void saveChannels(DataStoreHost* host);
  void migrateToSecondaryFS();
  uint8_t getBlobByKey(const uint8_t key[], int key_len, uint8_t dest_buf[]);
  bool putBlobByKey(const uint8_t key[], int key_len, const uint8_t src_buf[], uint8_t len);
  bool deleteBlobByKey(const uint8_t key[], int key_len);
  File openRead(const char* filename);
  File openRead(FILESYSTEM* fs, const char* filename);
  bool removeFile(const char* filename);
  bool removeFile(FILESYSTEM* fs, const char* filename);
  bool clearLegacyMessageFiles();
  uint32_t getStorageUsedKb() const;
  uint32_t getStorageTotalKb() const;

private:
  FILESYSTEM* _getContactsChannelsFS() const { if (_fsExtra) return _fsExtra; return _fs;};
#if defined(ENV_INCLUDE_GPS) && (ENV_INCLUDE_GPS)
#if defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
  bool _allowGpsTrackFileOpen = false;
  File* _gpsTrackFile = nullptr;
#else
  File _gpsTrackFile{};
  bool _gpsTrackFileOpen = false;
#endif
#endif
};
