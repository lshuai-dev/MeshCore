#pragma once
#include "ui/core/biz_facade.hpp"
#include "ui/core/app_state_event.hpp"

#ifndef HELTEC_GPS_TRACK_MIN_DIST_CM
#define HELTEC_GPS_TRACK_MIN_DIST_CM 5000  // 50 m
#endif
#ifndef HELTEC_GPS_TRACK_MAX_POINTS
#define HELTEC_GPS_TRACK_MAX_POINTS 600
#endif

namespace heltec::meshcore::biz {
class MeshAppUi final : public IBizFacade {
 public:
  void pollRuntime();

  bool sendAdvert() override;
  void sendAdvertWithFeedback() override;
  void requestRadioParamPresetPicker() override;
  bool toggleGPS() override;
  void setGpsEnabled(bool enabled) override;
  bool buzzerEnabled() const override;
  void setBuzzerEnabled(bool enabled) override;
  uint8_t buzzerVolumeLevel() const override;
  void setBuzzerVolumeLevel(uint8_t level) override;
  bool isLnaCanControl() const override;
  bool lnaEnabled() const override;
  bool setLnaEnabled(bool enabled) override;
  void adjustTxPowerDbm(int delta_db) override;
  void adjustSpreadingFactor(int delta) override;
  bool sendBroadcast(const char* text, int len) override;
  bool sendDirectMessage(const uint8_t pub_key_prefix[6], const char* text) override;
  bool sendGroupMessage(int channel_idx, const char* text) override;
  void requestSendMessageOverlay() override;
  void requestCompassCalibration() override;
  int sendMessagePersonalCount() const override;
  bool sendMessagePersonalAt(int index, uint8_t pub_key_prefix[6], char* label,
                             size_t label_len) const override;
  bool sendMessageHasGroupChannels() const override;
  int sendMessageGroupCount() const override;
  bool sendMessageGroupAt(int index, int* channel_idx, char* label, size_t label_len) const override;
  void setLoRaBandPresetIndex(int preset_index) override;
  int currentLoRaBandPresetIndex() const override;
  int loRaBandPresetCount() const override;
  const char* loRaBandPresetName(int preset_index) const override;

  void requestHibernate() override;

  RadioStatus radioStatus() const override;
  GpsStatus gpsStatus() const override;

  bool companionLinkEnabled() const override;
  void setCompanionLinkEnabled(bool enabled) override;

  const char* nodeName() const override;
  void formatNodeIdLine(char* buf, size_t buf_len) const override;
  int messageCount() const override;
  bool hasCompanionConnection() const override;
  uint32_t companionPairingPin() const override;

  int fillRecentHeard(RecentHeardItem* items, int max_items) const override;

  const CompassUi& compassUi() const override { return _compass_ui; }
  FindFriendUi findFriendUi() const override;

  bool locationShareEnabled() const override;
  void setLocationShareEnabled(bool enabled) override;
  int locShareIntervalIndex() const override;
  void setLocShareIntervalIndex(int index) override;
  int locShareIntervalOptionCount() const override;
  const char* locShareIntervalOptionLabel(int index) const override;

  int displayAutoOffIndex() const override;
  void setDisplayAutoOffIndex(int index) override;
  int displayAutoOffOptionCount() const override;
  const char* displayAutoOffOptionLabel(int index) const override;

  int findFriendMode() const override;
  bool setFindFriendMode(int mode) override;
  bool saveFindFriendWaypointFromGps() override;
  bool setFindFriendWaypoint(double lat_deg, double lon_deg) override;
  void formatFindFriendWaypointInput(char* buf, size_t buf_len) const override;

  int findFriendContactCount() const override;
  bool findFriendContactLabel(int index, char* buf, size_t buf_len) const override;
  int buildFindFriendDropdownOptions(char* buf, size_t buf_len, int16_t* mesh_map,
                                     int mesh_map_cap) const override;
  bool findFriendContactHasGps(int index) const override;
  int findFriendTargetContactIndex() const override;
  void setFindFriendTargetContactIndex(int index) override;
  void cycleFindFriendTargetContact(int delta) override;
  bool tryAutoPickFindFriendTarget() override;
  void syncFindFriendContactList() override;
  void syncCompassCache() override;
  bool compassHasHardware() const override;
  void beginCompassCalibration() override;
  void endCompassCalibration() override;
  bool compassHasStoredCalibration() const override;
  bool saveCompassCalibration() override;
  bool restoreCompassCalibration() override;

  bool factoryReset() override;
  bool clearUserData() override;

  bool gpsTrackRecording() const override;
  bool setGpsTrackRecording(bool enabled) override;
  int gpsTrackIntervalIndex() const override;
  bool setGpsTrackIntervalIndex(int index) override;
  int gpsTrackIntervalOptionCount() const override;
  const char* gpsTrackIntervalOptionLabel(int index) const override;

#if defined(ENV_INCLUDE_MAP) && ENV_INCLUDE_MAP
  const MapPlotUi& mapPlotUi() const override;
#endif

  void showAlert(const char* text, int duration_ms) override;
  void dismissMessagePreview() override;
  void requestWaypointManualInput() override;
  void notifyWaypointKeyboardClosed() override;
  void requestCloseSendMessageOverlay() override;

 private:
  static void notifyAppState(heltec::meshcore::ui::AppStateEventType type);
  static void notifyCompanionChanged();
  void notifyRadioChanged();
  void pollGpsTrack();
  void pollRadioStatus();
  void ensureFindFriendPrefsLoaded() const;
  void syncFfCacheFromStore() const;
  void setFfWaypointCache(double lat_deg, double lon_deg);
  void resetFindFriendNavState() const;
  void persistFfPrefs(int mode, int wp_valid, double wp_lat, double wp_lon) const;

  CompassUi _compass_ui{};
  mutable bool _ff_prefs_loaded = false;
  mutable int _ff_target_contact_idx = -1;
  mutable int _ff_mode = 0;
  mutable int _ff_wp_valid = 0;
  mutable int32_t _ff_wp_lat_e6 = 0;
  mutable int32_t _ff_wp_lon_e6 = 0;
  mutable uint16_t _ff_gps_track_interval_min = 1;

  RadioStatus _radio_status_cache{};
  uint32_t _radio_status_poll_ms = 0;
  bool _radio_status_cache_valid = false;

  bool _gps_track_armed = false;
  bool _gps_track_has_last = false;
  int32_t _gps_track_last_lat_e6 = 0;
  int32_t _gps_track_last_lon_e6 = 0;
  uint16_t _gps_track_point_count = 0;
  uint32_t _gps_track_start_ms = 0;

#if defined(ENV_INCLUDE_MAP) && ENV_INCLUDE_MAP
  mutable MapPlotUi _map_plot_cache{};
  mutable uint32_t _map_plot_fp = 0;
  mutable bool _map_plot_cache_valid = false;
#endif
};

}  // namespace heltec::meshcore::biz

