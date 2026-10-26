#pragma once
#include "app/FindFriendNavEstimator.hpp"
#include "config/MessagePresetStore.hpp"
#include "ui/core/AppStateEvent.hpp"
#include "ui/core/BizFacade.hpp"

#include <stdint.h>

namespace Heltec::MeshCore::Power
{
class PowerMgr;
struct PowerSnapshot;
using PowerChangeMask = uint16_t;
}

#ifndef HELTEC_GPS_TRACK_MIN_DIST_CM
#define HELTEC_GPS_TRACK_MIN_DIST_CM 5000 // 50 m
#endif
#ifndef HELTEC_GPS_TRACK_MAX_POINTS
#define HELTEC_GPS_TRACK_MAX_POINTS 600
#endif

namespace Heltec::MeshCore::Biz
{
class MeshAppUi final : public IBizFacade
{
  public:
    explicit MeshAppUi(Heltec::MeshCore::Power::PowerMgr &power);

    void poll();
    void reconcileGpsPower();
    void handlePowerChanged(Heltec::MeshCore::Power::PowerChangeMask changes,
                            const Heltec::MeshCore::Power::PowerSnapshot &snapshot);

    bool sendAdvert() override;
    void setGpsEnabled(bool enabled) override;
    bool buzzerEnabled() const override;
    void setBuzzerEnabled(bool enabled) override;
    uint8_t buzzerVolumeLevel() const override;
    void setBuzzerVolumeLevel(uint8_t level) override;
    bool isLnaCanControl() const override;
    bool lnaEnabled() const override;
    bool setLnaEnabled(bool enabled) override;
    bool sendBroadcast(const char *text, int len) override;
    bool sendDirectMessage(const uint8_t pub_key_prefix[6], const char *text) override;
    bool sendGroupMessage(int channel_idx, const char *text) override;
    int sendMessagePersonalCount() const override;
    bool sendMessagePersonalAt(int index, uint8_t pub_key_prefix[6], char *label,
                               size_t label_len) const override;
    int sendMessageGroupCount() const override;
    bool sendMessageGroupAt(int index, int *channel_idx, char *label,
                            size_t label_len) const override;
    int messagePresetCount() const override;
    const char *messagePresetAt(int index) const override;
    PresetUpdateResult updateMessagePreset(int index, const char *text) override;
    bool commitMessagePresets() override;
    void setLoRaBandPresetIndex(int preset_index) override;
    int currentLoRaBandPresetIndex() const override;
    int currentExactLoRaBandPresetIndex() const override;
    int loRaBandPresetCount() const override;
    const char *loRaBandPresetName(int preset_index) const override;
    bool forwardingEnabled() const override;
    int forwardingFrequencyCount() const override;
    bool forwardingFrequencyRange(int index, uint32_t *lower_khz,
                                  uint32_t *upper_khz) const override;
    int currentForwardingFrequencyIndex() const override;
    ForwardingApplyResult setForwardingEnabled(bool enabled, int frequency_index) override;

    RadioStatus radioStatus() const override;
    GpsStatus gpsStatus() const override;
    TeamStatus teamStatus() const override;
    int teamOnlineCount() const override;
#if defined(HELTEC_GEOFENCE) && HELTEC_GEOFENCE
    Team::GeofenceState teamGeofenceState() const override;
    uint8_t teamGeofencePointCount() const override;
    bool teamGeofencePoint(uint8_t index, Team::GeofencePoint &point) const override;
    bool beginTeamGeofenceEdit() override;
    void cancelTeamGeofenceEdit() override;
    void deleteTeamGeofence() override;
    Team::GeofenceAddResult addTeamGeofencePoint(int32_t latitude_e7,
                                                 int32_t longitude_e7) override;
    bool teamGeofenceMemberAlarm(const uint8_t member_id[6]) const override;
#endif

    bool companionLinkEnabled() const override;
    void setCompanionLinkEnabled(bool enabled) override;

    void formatNodeIdLine(char *buf, size_t buf_len) const override;
    uint32_t currentTimeSeconds() const override;
    int messageCount() const override;
    bool hasCompanionConnection() const override;
    uint32_t companionPairingPin() const override;

    int fillRecentlyHeard(RecentlyHeardItem *items, int max_items) const override;

    CompassUi compassUi() const override;
    FindFriendUi findFriendUi() const override;
    bool teamPendingDetails(char *name, size_t length, uint64_t &session,
                            uint16_t &code) const override;
    int teamOfferCount() const override;
    bool teamOfferLabel(int index, char *label, size_t length) const override;
    bool teamSelectOffer(int index) override;
    bool teamStartSearch() override;
    bool teamSetRole(bool leader) override;
    bool teamSetActive(bool active) override;
    bool teamSetJoining(bool open) override;
    bool teamMessageTarget(int member_index, Ui::UiSendMessageTarget &target) const override;
    bool mapMessageTarget(const uint8_t *prefix, size_t length,
                          Ui::UiSendMessageTarget &target) const override;
    bool teamDecidePending(uint64_t session, bool approve) override;
    bool teamLeave() override;
    void teamCancel() override;

    bool locationShareEnabled() const override;
    bool findFriendEnabled() const override;
    bool setFindFriendEnabled(bool enabled) override;
    void setGpsForegroundActive(bool active) override;
    void setMapForegroundActive(bool active) override;
    void setFindFriendForegroundActive(bool active) override;

    int displayAutoOffIndex() const override;
    void setDisplayAutoOffIndex(int index) override;
    int displayAutoOffOptionCount() const override;
    const char *displayAutoOffOptionLabel(int index) const override;

    int findFriendMode() const override;
    bool setFindFriendMode(int mode) override;
    bool setFindFriendWaypoint(double lat_deg, double lon_deg) override;
    void formatFindFriendWaypointInput(char *buf, size_t buf_len) const override;

    int findFriendContactCount() const override;
    int fillFindFriendContacts(int offset, int selected_contact_index, FindFriendContactItem *items,
                               int max_items, int *total_items, int *selected_rank) const override;
    int findFriendTargetContactIndex() const override;
    void setFindFriendTargetContactIndex(int index) override;
    bool tryAutoPickFindFriendTarget() override;
    bool compassHasHardware() const override;
    void setCompassForegroundActive(bool active) override;
    void beginCompassCalibration() override;
    void endCompassCalibration() override;
    bool compassHasStoredCalibration() const override;
    bool saveCompassCalibration() override;
    bool restoreCompassCalibration() override;

    bool factoryReset() override;
    bool clearUserData() override;

    bool gpsTrackRecording() const override;
    bool setGpsTrackRecording(bool enabled) override;

#if defined(ENV_INCLUDE_MAP) && ENV_INCLUDE_MAP
    int fillMapPlot(MapPlotMarker *markers, int max_markers,
                    uint8_t &online_count) const override;
#endif

  private:
    Heltec::MeshCore::Power::PowerMgr &_power;
    mutable MessagePresetStore _message_presets{};
    static void notifyAppState(Heltec::MeshCore::Ui::AppStateEventType type);
    void notifyRadioChanged();
    bool applyGpsPowerPolicy();
    void notifyGpsChanged();
    void pollGpsTrack();
    void pollRadioStatus();
    void ensureFindFriendPrefsLoaded() const;
    void setFfWaypointCache(double lat_deg, double lon_deg);
    void resetFindFriendNavState() const;
    void syncFindFriendTeamPin() const;
#if defined(HELTEC_GEOFENCE) && HELTEC_GEOFENCE
    void pollTeamGeofenceAlerts();
#endif
    bool findFriendMemberLabel(int index, char *buf, size_t buf_len) const;
    bool findFriendMemberHasGps(int index) const;
    void persistFfPrefs(int mode, int wp_valid, double wp_lat, double wp_lon) const;
    int resolveFindFriendTargetContactIndex() const;
    bool setFindFriendTargetKeyFromIndex(int index) const;

    struct FindFriendState
    {
        Nav::FindFriendNavEstimator estimator{};
        uint8_t target_pub_key[32] = {};
        int32_t waypoint_lat_e6 = 0;
        int32_t waypoint_lon_e6 = 0;
        int target_contact_index = -1;
        uint16_t track_interval_min = 1;
        uint8_t mode = 0;
        bool prefs_loaded = false;
        bool target_pub_key_valid = false;
        bool waypoint_valid = false;
        bool enabled = false;
        bool foreground_active = false;
    };

    mutable FindFriendState _find_friend{};

    RadioStatus _radio_status_cache{};
    uint32_t _radio_status_poll_ms = 0;
    bool _radio_status_cache_valid = false;

    bool _gps_track_armed = false;
    bool _gps_track_has_last = false;
    int32_t _gps_track_last_lat_e6 = 0;
    int32_t _gps_track_last_lon_e6 = 0;
    uint16_t _gps_track_point_count = 0;
    uint32_t _gps_track_start_ms = 0;

    struct TeamRuntimeState
    {
        uint16_t sequence = 0;
        uint8_t join_state = 0;
        uint8_t offer_count = 0;
        uint8_t flags = 0;

        bool operator==(const TeamRuntimeState &other) const
        {
            return sequence == other.sequence && join_state == other.join_state &&
                   offer_count == other.offer_count && flags == other.flags;
        }
    };

    TeamRuntimeState _team_runtime{};
    uint32_t _team_runtime_poll_ms = 0;
    bool _team_runtime_valid = false;

    static constexpr uint8_t kCompassDemandForeground = 1U << 0;
    static constexpr uint8_t kCompassDemandFindFriend = 1U << 1;
    static constexpr uint8_t kCompassDemandCalibration = 1U << 2;
    uint8_t _compass_demand_mask = 0;
    uint32_t _compass_idle_since_ms = 0;
    bool _compass_sampling_enabled = true;

    void setCompassDemand(uint8_t demand, bool active);
    void reconcileCompassSampling(uint32_t now_ms);
};

} // namespace Heltec::MeshCore::Biz
