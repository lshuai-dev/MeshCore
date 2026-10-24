#pragma once
#if defined(HELTEC_GEOFENCE) && HELTEC_GEOFENCE
#include "heltec/app/TeamGeofence.hpp"
#endif

#include <stddef.h>
#include <stdint.h>

namespace Heltec::MeshCore::Ui {
struct UiSendMessageTarget;
}

namespace Heltec::MeshCore::Biz {
struct CompassUi {
    bool has_hardware = false;
    bool heading_valid = false;
    float heading_deg = 0.f;
    /** Memsic azimuth before UI offset (for debug). */
    float azimuth_deg = 0.f;
    float mag_xyz[3] = { 0.f, 0.f, 0.f };
    int quality = 0;
};

#if defined(ENV_INCLUDE_MAP) && ENV_INCLUDE_MAP
struct MapPlotMarker {
    int32_t lat_micro = 0;
    int32_t lon_micro = 0;
    uint8_t pub_key_prefix[6] = {};
    char label[9] = { '?', '\0' };
    bool is_self = false;
    bool alarm_active = false;
};

struct MapPlotUi {
    static constexpr int kMaxMarkers = 32;
    uint8_t online_count = 0;
    uint8_t marker_count = 0;
    MapPlotMarker markers[kMaxMarkers] = {};
};
#endif

enum class FindFriendTargetFreshness : uint8_t {
    NotApplicable = 0,
    Unknown,
    Fresh,
    Aging,
    Stale,
};

enum class FindFriendConfidence : uint8_t {
    Unavailable = 0,
    Low,
    Medium,
    High,
};

struct FindFriendUi {
    bool compass_hw = false;
    bool heading_valid = false;
    float heading_deg = 0.f;
    int compass_quality = 0;
    bool declination_configured = false;
    int mode = 0;
    bool waypoint_valid = false;
    double waypoint_lat = 0;
    double waypoint_lon = 0;
    bool gps_fix = false;
    uint32_t gps_age_ms = 0;
    uint8_t gps_satellites = 0;
    float estimated_accuracy_m = 0.f;
    bool gps_low_accuracy = false;
    double here_lat = 0;
    double here_lon = 0;
    bool bearing_valid = false;
    float bearing_to_waypoint_deg = 0.f;
    /** Device-relative turn: positive → rotate clockwise to point toward waypoint. */
    bool relative_valid = false;
    float turn_deg = 0.f;
    char target_label[40] = {};
    bool target_selected = false;
    bool target_valid = false;
    bool target_usable = false;
    bool target_age_known = false;
    uint32_t target_age_ms = 0;
    FindFriendTargetFreshness target_freshness = FindFriendTargetFreshness::NotApplicable;
    bool near_target = false;
    double distance_m = -1.0;
    float nearby_enter_m = 20.f;
    FindFriendConfidence confidence = FindFriendConfidence::Unavailable;
};

class IBizFacade {
  public:
    enum class PresetUpdateResult : uint8_t
    {
        Updated,
        Unchanged,
        Empty,
        TooLong,
        InvalidText,
        InvalidIndex,
    };

    virtual ~IBizFacade() = default;

    enum class ForwardingApplyResult : uint8_t {
        Ok,
        InvalidSelection,
        InvalidRadioParams,
        UnsupportedFrequency,
        Unavailable,
    };

    // UI 与业务层的唯一边界。状态按领域返回快照，不拆分为多个细粒度接口。
    struct RadioStatus {
        float freq_mhz = 0.0f;
        float bw_khz = 0.0f;
        int cr = 0;
        int sf = 0;
        int tx_power_dbm = 0;
        int noise_floor_dbm = 0;
        bool rx_valid = false;
        float last_rssi_dbm = 0.0f;
        float last_snr_db = 0.0f;
        uint32_t last_rx_at_ms = 0;
        bool forwarding_enabled = false;
    };

    struct GpsStatus {
        bool enabled = false;
        bool available = false;
        bool powered = false;
        bool fix_valid = false;
        /** Milliseconds since the most recent valid GPS fix was received. */
        uint32_t fix_valid_ms = 0;
        uint8_t satellites = 0;
        long lat_micro = 0; // millionths of a degree (MicroNMEA raw)
        long lon_micro = 0;
        double lat_deg = 0.0;
        double lon_deg = 0.0;
        double alt_m = 0.0;
        float speed_kph = 0.0f;
    };

    struct TeamStatus {
        bool is_leader = false;
        bool has_team = false;
        bool join_window_open = false;
        int member_count = 0;
        uint16_t member_revision = 0;
        int pair_state = 0;
        uint16_t pairing_code = 0;
        uint64_t pending_session = 0;
    };

    // Messaging data and commands.
    /** Send-message overlay: personal contacts (pub key prefix + label). */
    virtual int sendMessagePersonalCount() const = 0;
    virtual bool sendMessagePersonalAt(int index, uint8_t pub_key_prefix[6], char *label,
                                       size_t label_len) const = 0;
    virtual int sendMessageGroupCount() const = 0;
    virtual bool sendMessageGroupAt(int index, int *channel_idx, char *label,
                                    size_t label_len) const = 0;
    virtual int messagePresetCount() const = 0;
    virtual const char *messagePresetAt(int index) const = 0;
    virtual PresetUpdateResult updateMessagePreset(int index, const char *text) = 0;
    virtual bool commitMessagePresets() = 0;
    virtual int currentLoRaBandPresetIndex() const = 0;
    virtual int currentExactLoRaBandPresetIndex() const = 0;
    virtual int loRaBandPresetCount() const = 0;
    virtual const char *loRaBandPresetName(int preset_index) const = 0;
    virtual bool forwardingEnabled() const = 0;
    virtual int forwardingFrequencyCount() const = 0;
    virtual bool forwardingFrequencyRange(int index, uint32_t *lower_khz,
                                          uint32_t *upper_khz) const = 0;
    virtual int currentForwardingFrequencyIndex() const = 0;

    // Read-only device and screen state.
    virtual RadioStatus radioStatus() const = 0;
    virtual GpsStatus gpsStatus() const = 0;
    virtual TeamStatus teamStatus() const = 0;
    virtual bool buzzerEnabled() const = 0;
    virtual uint8_t buzzerVolumeLevel() const {
        return 3;
    }
    virtual bool isLnaCanControl() const {
        return false;
    }
    virtual bool lnaEnabled() const {
        return false;
    }
    virtual bool companionLinkEnabled() const = 0;
    virtual void formatNodeIdLine(char *buf, size_t buf_len) const = 0;
    virtual uint32_t currentTimeSeconds() const = 0;
    virtual int messageCount() const = 0;
    virtual bool hasCompanionConnection() const = 0;
    virtual uint32_t companionPairingPin() const = 0;

    struct RecentlyHeardItem {
        char name[32]{};
        int32_t age_seconds = 0;
    };
    virtual int fillRecentlyHeard(RecentlyHeardItem *items, int max_items) const = 0;

    virtual const CompassUi &compassUi() const = 0;
    virtual FindFriendUi findFriendUi() const = 0;

    virtual int teamOnlineCount() const = 0;
#if defined(HELTEC_GEOFENCE) && HELTEC_GEOFENCE
    virtual Team::GeofenceState teamGeofenceState() const {
        return Team::GeofenceState::Empty;
    }
    virtual uint8_t teamGeofencePointCount() const { return 0; }
    virtual bool teamGeofencePoint(uint8_t index, Team::GeofencePoint &point) const {
        (void)index;
        point = Team::GeofencePoint{};
        return false;
    }
    virtual bool beginTeamGeofenceEdit() { return false; }
    virtual void cancelTeamGeofenceEdit() { }
    virtual void deleteTeamGeofence() { }
    virtual Team::GeofenceAddResult addTeamGeofencePoint(int32_t latitude_e7,
                                                         int32_t longitude_e7) {
        (void)latitude_e7;
        (void)longitude_e7;
        return Team::GeofenceAddResult::Invalid;
    }
    virtual bool teamGeofenceMemberAlarm(const uint8_t member_id[6]) const {
        (void)member_id;
        return false;
    }
#endif

    virtual bool teamPendingDetails(char *name, size_t length, uint64_t &session,
                                    uint16_t &code) const {
        session = 0;
        code = 0;
        if (name && length > 0) name[0] = '\0';
        return false;
    }
    virtual int teamOfferCount() const {
        return 0;
    }
    virtual bool teamOfferLabel(int index, char *label, size_t length) const {
        (void)index;
        if (label && length > 0) label[0] = '\0';
        return false;
    }
    virtual bool teamSelectOffer(int index) {
        (void)index;
        return false;
    }
    virtual bool teamStartSearch() {
        return false;
    }
    virtual bool teamSetRole(bool leader) {
        (void)leader;
        return false;
    }
    virtual bool teamSetActive(bool active) {
        (void)active;
        return false;
    }
    virtual bool teamSetJoining(bool open) {
        (void)open;
        return false;
    }
    virtual bool teamMessageTarget(int member_index, Ui::UiSendMessageTarget &target) const {
        (void)member_index;
        (void)target;
        return false;
    }
    virtual bool mapMessageTarget(const uint8_t *prefix, size_t length,
                                  Ui::UiSendMessageTarget &target) const {
        (void)prefix;
        (void)length;
        (void)target;
        return false;
    }
    virtual bool teamDecidePending(uint64_t session, bool approve) {
        (void)session;
        (void)approve;
        return false;
    }
    virtual bool teamLeave() {
        return false;
    }
    virtual void teamCancel() {
    }

    // Location, compass, and find-friend state.
    virtual bool locationShareEnabled() const = 0;
    virtual bool findFriendEnabled() const = 0;
    virtual int findFriendMode() const = 0;
    virtual void formatFindFriendWaypointInput(char *buf, size_t buf_len) const = 0;
    virtual int findFriendContactCount() const = 0;
    struct FindFriendContactItem {
        int16_t contact_index = -1;
        char label[32] = {};
    };
    /** Fill one contact page; null items queries count/rank without building labels. */
    virtual int fillFindFriendContacts(int offset, int selected_contact_index,
                                       FindFriendContactItem *items, int max_items,
                                       int *total_items, int *selected_rank) const = 0;
    virtual int findFriendTargetContactIndex() const = 0;

    // Device and radio commands.
    virtual bool sendAdvert() = 0;
    virtual void setGpsEnabled(bool enabled) = 0;
    virtual bool sendBroadcast(const char *text, int len) = 0;
    virtual bool sendDirectMessage(const uint8_t pub_key_prefix[6], const char *text) = 0;
    virtual bool sendGroupMessage(int channel_idx, const char *text) = 0;
    virtual void setLoRaBandPresetIndex(int preset_index) = 0;
    virtual ForwardingApplyResult setForwardingEnabled(bool enabled, int frequency_index) = 0;
    virtual void setBuzzerEnabled(bool enabled) = 0;
    virtual void setBuzzerVolumeLevel(uint8_t level) {
        (void)level;
    }
    virtual bool setLnaEnabled(bool enabled) {
        (void)enabled;
        return false;
    }
    virtual void setCompanionLinkEnabled(bool enabled) = 0;
    virtual bool setFindFriendEnabled(bool enabled) = 0;
    virtual void setGpsForegroundActive(bool active) {
        (void)active;
    }
    virtual void setMapForegroundActive(bool active) {
        (void)active;
    }
    virtual void setFindFriendForegroundActive(bool active) {
        (void)active;
    }
    virtual int displayAutoOffIndex() const {
        return 0;
    }
    virtual void setDisplayAutoOffIndex(int index) {
        (void)index;
    }
    virtual int displayAutoOffOptionCount() const {
        return 0;
    }
    virtual const char *displayAutoOffOptionLabel(int index) const {
        (void)index;
        return nullptr;
    }
    virtual bool setFindFriendMode(int mode) = 0;
    virtual bool setFindFriendWaypoint(double lat_deg, double lon_deg) = 0;
    virtual void setFindFriendTargetContactIndex(int index) = 0;
    /** Pick and persist the first contact with a usable stored location. */
    virtual bool tryAutoPickFindFriendTarget() {
        return false;
    }
    virtual bool compassHasHardware() const {
        return false;
    }
    virtual void beginCompassCalibration() {
    }
    virtual void endCompassCalibration() {
    }
    virtual bool compassHasStoredCalibration() const {
        return false;
    }
    virtual bool saveCompassCalibration() {
        return false;
    }
    virtual bool restoreCompassCalibration() {
        return false;
    }

    virtual bool factoryReset() {
        return false;
    }
    virtual bool clearUserData() {
        return false;
    }

    // GPS track storage controls.
    virtual bool gpsTrackRecording() const {
        return false;
    }
    virtual bool setGpsTrackRecording(bool enabled) {
        (void)enabled;
        return false;
    }

#if defined(ENV_INCLUDE_MAP) && ENV_INCLUDE_MAP
    // The reference is valid only until the next mapPlotUi() call on this facade.
    // Callers must consume it synchronously and must not retain it.
    virtual const MapPlotUi &mapPlotUi() const = 0;
#endif
};

} // namespace Heltec::MeshCore::Biz
