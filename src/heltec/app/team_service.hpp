/**
 * @file    team_service.hpp
 * @brief   Fixed-memory runtime service for Heltec Team location sharing.
 * @version 1.0
 * @date    2026-09-04
 */
#pragma once

#include "app/team_config.hpp"
#include "app/team_protocol.hpp"

#include <stddef.h>
#include <stdint.h>

class DataStore;
class HeltecMesh;

namespace heltec::meshcore::team
{

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
    bool occupied = false;
    bool location_valid = false;
    bool presence_seen = false;
    bool location_seen = false;
    uint8_t role = 0;
    uint8_t sender_id[kSenderIdSize] = {};
    uint8_t public_key[kPublicKeySize] = {};
    uint16_t boot_id = 0;
    uint16_t presence_sequence = 0;
    uint16_t location_sequence = 0;
    uint16_t battery_mv = 0;
    uint16_t accuracy_dm = 0;
    int32_t latitude_e6 = 0;
    int32_t longitude_e6 = 0;
    uint16_t fix_age_sec = 0;
    uint32_t last_seen_ms = 0;
    uint32_t location_received_ms = 0;
    uint8_t satellites = 0;
    uint8_t name_len = 0;
    char name[kMaxNameBytes + 1] = {};
};

struct TeamStats
{
    uint16_t rx_presence = 0;
    uint16_t rx_location = 0;
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
    void handleTeamData(const uint8_t *data, size_t length);

    bool createTeam(const char *name);
    bool installTeam(const TeamCredentials &credentials);
    bool leaveTeam();
    bool setEnabled(bool enabled);
    bool requestSync();

    bool hasTeam() const;
    bool enabled() const;
    bool locationShareEnabled() const;
    const TeamCredentials &credentials() const;
    size_t memberCount() const;
    bool memberAt(size_t ordinal, TeamMember &member) const;
    const TeamMember *memberSlot(size_t slot) const
    {
        return enabled() && slot < HELTEC_TEAM_MAX_MEMBERS &&
               _members[slot].occupied && _members[slot].presence_seen ? &_members[slot] : nullptr;
    }
    const TeamStats &stats() const { return _stats; }
    void resetStats() { _stats = TeamStats{}; }
    void noteWrongChannel();
    uint16_t changeSequence() const { return _change_sequence; }
    void pinMember(const uint8_t public_key[kPublicKeySize]);

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
    CommonHeader makeHeader(uint8_t flags, uint16_t sequence) const;
    bool sendPresence();
    bool sendLocation(bool force_invalid = false, bool force_update = false);
    bool sendFrame(const uint8_t *data, size_t length);
    void receivePresence(const uint8_t *data, size_t length);
    void receiveLocation(const uint8_t *data, size_t length);
    void receiveSyncRequest(const uint8_t *data, size_t length);
    TeamMember *findMember(const uint8_t sender_id[kSenderIdSize]);
    TeamMember *allocateMember(const uint8_t sender_id[kSenderIdSize]);
    void clearMembers();

    HeltecMesh *_mesh = nullptr;
    TeamCredentials _credentials{};
    TeamMember _members[HELTEC_TEAM_MAX_MEMBERS] = {};
    uint32_t _next_presence_ms = 0;
    uint32_t _next_location_ms = 0;
    uint32_t _pending_presence_ms = 0;
    uint32_t _pending_location_ms = 0;
    uint8_t _sync_presence_attempts = 0;
    uint8_t _sync_location_attempts = 0;
    uint32_t _startup_sync_ms = 0;
    uint8_t _startup_sync_attempts = 0;
    uint16_t _boot_id = 0;
    uint16_t _presence_sequence = 0;
    uint16_t _location_sequence = 0;
    uint16_t _sync_sequence = 0;
    int32_t _last_sent_latitude_e6 = 0;
    int32_t _last_sent_longitude_e6 = 0;
    uint32_t _last_location_sent_ms = 0;
    uint32_t _last_sync_request_rx_ms = 0;
    uint32_t _last_sync_request_tx_ms = 0;
    bool _sync_request_tx_seen = false;
    bool _enabled = false;
    bool _sent_valid_location = false;
    TeamStats _stats{};
    uint16_t _change_sequence = 0;
    uint8_t _pinned_member_id[kSenderIdSize] = {};
    SyncSeen _sync_seen[4] = {};
};

} // namespace heltec::meshcore::team
