#pragma once
#include <stddef.h>
#include <stdint.h>
#include "ui_feedback.hpp"

namespace heltec::meshcore::biz {
struct CompassUi {
  bool has_hardware = false;
  bool heading_valid = false;
  float heading_deg = 0.f;
  /** Memsic azimuth before UI offset (for debug). */
  float azimuth_deg = 0.f;
  float mag_xyz[3] = {0.f, 0.f, 0.f};
  int quality = 0;
};

#if defined(ENV_INCLUDE_MAP) && ENV_INCLUDE_MAP
struct MapPlotMarker {
  char label[32] = {};
  double lat_deg = 0.0;
  double lon_deg = 0.0;
  float east_m = 0.f;
  float north_m = 0.f;
  bool is_self = false;
  int contact_index = -1;
};

struct MapPlotUi {
  static constexpr int kMaxMarkers = 40;
  bool drawable = false;
  int contact_gps_count = 0;
  int marker_count = 0;
  double center_lat = 0.0;
  double center_lon = 0.0;
  uint32_t span_w_m = 0;
  uint32_t span_h_m = 0;
  MapPlotMarker markers[kMaxMarkers];
};
#endif

struct FindFriendUi {
  bool compass_hw = false;
  bool heading_valid = false;
  float heading_deg = 0.f;
  int mode = 0;
  bool waypoint_valid = false;
  double waypoint_lat = 0;
  double waypoint_lon = 0;
  bool gps_fix = false;
  double here_lat = 0;
  double here_lon = 0;
  bool bearing_valid = false;
  float bearing_to_waypoint_deg = 0.f;
  /** Device-relative turn: positive → rotate clockwise to point toward waypoint. */
  bool relative_valid = false;
  float turn_deg = 0.f;
  char target_label[40] = {};
  bool target_valid = false;
  bool arrived = false;
  double distance_m = -1.0;
};

class IBizFacade : public ui::IFeedback {
 public:
  virtual ~IBizFacade() = default;

  struct RadioStatus {
    float freq_mhz = 0.0f;
    float bw_khz = 0.0f;
    int cr = 0;
    int sf = 0;
    int tx_power_dbm = 0;
    int noise_floor_dbm = 0;
  };

  struct GpsStatus {
    bool enabled = false;
    bool available = false;
    bool fix_valid = false;
    uint8_t satellites = 0;
    long lat_micro = 0;  // millionths of a degree (MicroNMEA raw)
    long lon_micro = 0;
    double lat_deg = 0.0;
    double lon_deg = 0.0;
    double alt_m = 0.0;
  };

  virtual void requestRadioParamPresetPicker() = 0;
  virtual void requestSendMessageOverlay() = 0;
  virtual void requestCompassCalibration() = 0;

  /** Send-message overlay: personal contacts (pub key prefix + label). */
  virtual int sendMessagePersonalCount() const = 0;
  virtual bool sendMessagePersonalAt(int index, uint8_t pub_key_prefix[6], char* label,
                                     size_t label_len) const = 0;
  virtual bool sendMessageHasGroupChannels() const = 0;
  virtual int sendMessageGroupCount() const = 0;
  virtual bool sendMessageGroupAt(int index, int* channel_idx, char* label, size_t label_len) const = 0;
  virtual int currentLoRaBandPresetIndex() const = 0;
  virtual int loRaBandPresetCount() const = 0;
  virtual const char* loRaBandPresetName(int preset_index) const = 0;

  virtual RadioStatus radioStatus() const = 0;
  virtual GpsStatus gpsStatus() const = 0;
  virtual bool buzzerEnabled() const = 0;
  virtual uint8_t buzzerVolumeLevel() const { return 3; }
  virtual bool isLnaCanControl() const { return false; }
  virtual bool lnaEnabled() const { return false; }
  virtual bool companionLinkEnabled() const = 0;
  virtual const char* nodeName() const = 0;
  virtual void formatNodeIdLine(char* buf, size_t buf_len) const = 0;
  virtual int messageCount() const = 0;
  virtual bool hasCompanionConnection() const = 0;
  virtual uint32_t companionPairingPin() const = 0;

  struct RecentHeardItem {
    char name[32];
    int32_t age_seconds;
  };
  virtual int fillRecentHeard(RecentHeardItem* items, int max_items) const = 0;

  virtual const CompassUi& compassUi() const = 0;
  virtual FindFriendUi findFriendUi() const = 0;

  virtual bool locationShareEnabled() const = 0;
  virtual int locShareIntervalIndex() const = 0;
  virtual int locShareIntervalOptionCount() const = 0;
  virtual const char* locShareIntervalOptionLabel(int index) const = 0;
  virtual int findFriendMode() const = 0;
  virtual void formatFindFriendWaypointInput(char* buf, size_t buf_len) const = 0;
  virtual int findFriendContactCount() const = 0;
  virtual bool findFriendContactLabel(int index, char* buf, size_t buf_len) const = 0;
  /** Build dropdown text (recent first) and optional mesh-index map for each listed row. */
  virtual int buildFindFriendDropdownOptions(char* buf, size_t buf_len, int* mesh_map,
                                             int mesh_map_cap) const = 0;
  virtual bool findFriendContactHasGps(int index) const = 0;
  virtual int findFriendTargetContactIndex() const = 0;

  virtual bool sendAdvert() = 0;
  virtual void sendAdvertWithFeedback() = 0;
  virtual bool toggleGPS() = 0;
  virtual void setGpsEnabled(bool enabled) = 0;
  virtual void adjustTxPowerDbm(int delta_db) = 0;
  virtual void adjustSpreadingFactor(int delta) = 0;
  virtual bool sendBroadcast(const char* text, int len) = 0;
  virtual bool sendDirectMessage(const uint8_t pub_key_prefix[6], const char* text) = 0;
  virtual bool sendGroupMessage(int channel_idx, const char* text) = 0;
  virtual void setLoRaBandPresetIndex(int preset_index) = 0;
  virtual void requestHibernate() = 0;
  virtual void setBuzzerEnabled(bool enabled) = 0;
  virtual void setBuzzerVolumeLevel(uint8_t level) { (void)level; }
  virtual bool setLnaEnabled(bool enabled) { (void)enabled; return false; }
  virtual void setCompanionLinkEnabled(bool enabled) = 0;
  virtual void setLocationShareEnabled(bool enabled) = 0;
  virtual void setLocShareIntervalIndex(int index) = 0;
  virtual int displayAutoOffIndex() const { return 0; }
  virtual void setDisplayAutoOffIndex(int index) { (void)index; }
  virtual int displayAutoOffOptionCount() const { return 0; }
  virtual const char* displayAutoOffOptionLabel(int index) const { (void)index; return nullptr; }
  virtual bool setFindFriendMode(int mode) = 0;
  virtual bool saveFindFriendWaypointFromGps() = 0;
  virtual bool setFindFriendWaypoint(double lat_deg, double lon_deg) = 0;
  virtual void setFindFriendTargetContactIndex(int index) = 0;
  virtual void cycleFindFriendTargetContact(int delta) = 0;
  /** Pick first GPS contact in memory only (no flash write). For screen enter. */
  virtual bool tryAutoPickFindFriendTarget() { return false; }
  virtual void syncFindFriendContactList() = 0;
  virtual void syncCompassCache() = 0;
  virtual bool compassHasHardware() const { return false; }
  virtual void beginCompassCalibration() {}
  virtual void endCompassCalibration() {}
  virtual bool compassHasStoredCalibration() const { return false; }
  virtual bool saveCompassCalibration() { return false; }
  virtual bool restoreCompassCalibration() { return false; }

  virtual bool factoryReset() { return false; }
  virtual bool clearUserData() { return false; }

  virtual bool gpsTrackRecording() const { return false; }
  virtual bool setGpsTrackRecording(bool enabled) { (void)enabled; return false; }
  virtual int gpsTrackIntervalIndex() const { return 0; }
  virtual bool setGpsTrackIntervalIndex(int index) {
    (void)index;
    return true;
  }
  virtual int gpsTrackIntervalOptionCount() const { return 0; }
  virtual const char* gpsTrackIntervalOptionLabel(int index) const { (void)index; return nullptr; }

#if defined(ENV_INCLUDE_MAP) && ENV_INCLUDE_MAP
  // The reference is valid only until the next mapPlotUi() call on this facade.
  // Callers must consume it synchronously and must not retain it.
  virtual const MapPlotUi& mapPlotUi() const = 0;
#endif
};

}  // namespace heltec::meshcore::biz
