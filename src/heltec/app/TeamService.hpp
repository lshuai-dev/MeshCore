/**
 * @file    TeamService.hpp
 * @brief   Fixed-memory runtime service for Heltec Team location sharing.
 * @version 1.0
 * @date    2026-09-04
 */
#pragma once

#include "app/TeamConfig.hpp"
#include "app/TeamProtocol.hpp"
#if defined(HELTEC_GEOFENCE) && HELTEC_GEOFENCE
#include "app/TeamGeofence.hpp"
#endif

#include <stddef.h>
#include <stdint.h>

class DataStore;
class HeltecMesh;

namespace Heltec::MeshCore::Team
{

enum class TeamRole : uint8_t
{
    Member = 0,
    Leader = 1,
};

struct TeamCredentials
{
    uint64_t team_id = 0;
    uint32_t key_version = 0;
    uint8_t secret[32] = {};
    uint8_t leader_public_key[32] = {};
    uint8_t name_len = 0;
    char name[kMaxNameBytes + 1] = {};
};

struct TeamMember
{
    uint8_t public_key[kPublicKeySize] = {};
    char name[kMaxNameBytes + 1] = {};
    uint32_t last_seen_ms = 0;
    int32_t latitude_e6 = 0;
    int32_t longitude_e6 = 0;
    uint16_t boot_id = 0;
    uint16_t sequence = 0;
    uint16_t battery_mv = 0;
    uint16_t fix_age_sec = 0;
    uint8_t name_len = 0;
    uint8_t role = 0;
    bool occupied = false;
    bool location_valid = false;
#if defined(HELTEC_GEOFENCE) && HELTEC_GEOFENCE
    bool geofence_alarm_active = false;
#endif
};

#if defined(HELTEC_GEOFENCE) && HELTEC_GEOFENCE
static_assert(sizeof(TeamMember) <= 92, "TeamMember contains unexpected padding");
#else
static_assert(sizeof(TeamMember) <= 88, "TeamMember contains unexpected padding");
#endif

#if defined(HELTEC_GEOFENCE) && HELTEC_GEOFENCE
struct GeofenceAlert
{
    GeofenceMemberResult result = GeofenceMemberResult::Unchanged;
    uint8_t name_len = 0;
    char name[kMaxNameBytes + 1] = {};
};
#endif

struct TeamStats
{
    uint16_t rx_status = 0;
    // Preserve the existing 10-counter Companion ABI; former Location slot stays zero.
    uint16_t reserved = 0;
    uint16_t rx_sync_request = 0;
    uint16_t rx_decode_error = 0;
    uint16_t rx_wrong_channel = 0;
    uint16_t rx_duplicate_or_old = 0;
    uint16_t member_table_full = 0;
    uint16_t tx_queued = 0;
    uint16_t tx_dropped = 0;
    uint16_t sync_rate_limited = 0;
};

class TeamService
{
  public:
    void begin(HeltecMesh &mesh, DataStore &store);
    void poll();
    void notifyLocationChanged(bool valid, int32_t latitude_e6, int32_t longitude_e6);
    void handleTeamData(const uint8_t *data, size_t length);

    bool createTeam(const char *name);
    bool installTeam(const TeamCredentials &credentials);
    bool leaveTeam();
    bool setEnabled(bool enabled);
    bool requestSync();
    TeamRole role() const { return _role; }
    bool setRole(TeamRole role);

    bool hasTeam() const;
    bool enabled() const;
    bool locationShareEnabled() const;
    const TeamCredentials &credentials() const;
    size_t memberCount() const;
    bool memberAt(size_t ordinal, TeamMember &member) const;
    const TeamMember *memberSlot(size_t slot) const
    {
        return enabled() && slot < HELTEC_TEAM_MAX_MEMBERS &&
               _members[slot].occupied ? &_members[slot] : nullptr;
    }
    const TeamStats &stats() const { return _stats; }
    void resetStats() { _stats = TeamStats{}; }
    void noteWrongChannel();
    uint16_t changeSequence() const { return _change_sequence; }
    uint16_t memberRevision() const { return _member_revision; }
    void pinMember(const uint8_t public_key[kPublicKeySize]);
#if defined(HELTEC_GEOFENCE) && HELTEC_GEOFENCE
    GeofenceState geofenceState() const { return _geofence.state(); }
    uint8_t geofencePointCount() const { return _geofence.pointCount(); }
    const GeofencePoint *geofencePoints() const { return _geofence.points(); }
    bool beginGeofenceEdit();
    void cancelGeofenceEdit();
    void deleteGeofence();
    GeofenceAddResult addGeofencePoint(int32_t latitude_e7, int32_t longitude_e7);
    bool geofenceMemberAlarm(const uint8_t member_id[kSenderIdSize]) const;
    bool popGeofenceAlert(GeofenceAlert &alert);
#endif

  private:
    struct SyncSeen
    {
        uint8_t sender_id[kSenderIdSize] = {};
        uint16_t boot_id = 0;
        uint16_t request_id = 0;
        uint32_t seen_ms = 0;
    };
    void installSystemChannels();
    void clearTeamChannel();
    void scheduleInitialFrames();
    void scheduleStatus(uint32_t due_ms);
    CommonHeader makeHeader(uint8_t flags, uint16_t sequence) const;
    bool sendStatus();
    bool sendFrame(const uint8_t *data, size_t length);
    void receiveStatus(const uint8_t *data, size_t length);
    void receiveSyncRequest(const uint8_t *data, size_t length);
    TeamMember *findMember(const uint8_t public_key[kPublicKeySize]);
    TeamMember *allocateMember(const uint8_t public_key[kPublicKeySize]);
    void clearMembers();
#if defined(HELTEC_GEOFENCE) && HELTEC_GEOFENCE
    void evaluateGeofenceMember(TeamMember &member);
    void resetGeofenceMemberStates();
#endif

    HeltecMesh *_mesh = nullptr;
    TeamCredentials _credentials{};
    TeamMember _members[HELTEC_TEAM_MAX_MEMBERS] = {};
    uint32_t _status_due_ms = 0;
    uint32_t _startup_sync_ms = 0;
    uint8_t _startup_sync_attempts = 0;
    uint16_t _boot_id = 0;
    uint16_t _status_sequence = 0;
    uint16_t _sync_sequence = 0;
    int32_t _last_sent_latitude_e6 = 0;
    int32_t _last_sent_longitude_e6 = 0;
    uint32_t _last_status_sent_ms = 0;
    uint32_t _last_sync_request_rx_ms = 0;
    uint32_t _last_sync_request_tx_ms = 0;
    uint32_t _last_leader_seen_ms = 0;
    bool _sync_request_tx_seen = false;
    bool _enabled = false;
    TeamRole _role = TeamRole::Member;
    bool _sent_valid_location = false;
    TeamStats _stats{};
    uint16_t _change_sequence = 0;
    uint16_t _member_revision = 0;
    uint8_t _pinned_member_id[kSenderIdSize] = {};
    SyncSeen _sync_seen[4] = {};
#if defined(HELTEC_GEOFENCE) && HELTEC_GEOFENCE
    TeamGeofence _geofence{};
    uint32_t _geofence_alarm_started_mask = 0;
    uint32_t _geofence_alarm_cleared_mask = 0;
#endif
};

} // namespace Heltec::MeshCore::Team
