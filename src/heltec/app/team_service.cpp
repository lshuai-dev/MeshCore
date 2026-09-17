/**
 * @file    team_service.cpp
 * @brief   Fixed-memory runtime service for Heltec Team location sharing.
 * @version 1.0
 * @date    2026-09-04
 */
#include "app/team_service.hpp"

#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS

#include "app/HeltecMesh.h"
#include "config/DataStore.h"

#include <Arduino.h>
#include <Utils.h>
#include <limits.h>
#include <string.h>

namespace heltec::meshcore::team
{
namespace
{

const uint8_t kJoinSecret[32] = {
    0x64, 0x98, 0xBB, 0x21, 0x91, 0x31, 0xF4, 0x0D, 0x77, 0x46, 0xA8,
    0x52, 0xEC, 0x5B, 0x99, 0x70, 0x0E, 0xAF, 0x3E, 0x18, 0xDD, 0x79,
    0x13, 0x65, 0x4A, 0xC0, 0x2F, 0x87, 0x9C, 0xB4, 0x56, 0xDA
};

bool deadline_passed(uint32_t now, uint32_t deadline)
{
    return deadline != 0 && static_cast<int32_t>(now - deadline) >= 0;
}

bool any_nonzero(const uint8_t *data, size_t length)
{
    uint8_t combined = 0;
    for (size_t index = 0; index < length; ++index)
        combined |= data[index];
    return combined != 0;
}

uint16_t saturate_u16(uint32_t value)
{
    return value > UINT16_MAX ? UINT16_MAX : static_cast<uint16_t>(value);
}

void increment(uint16_t &counter)
{
    if (counter != UINT16_MAX)
        ++counter;
}

} // namespace

void TeamService::begin(HeltecMesh &mesh, DataStore &store)
{
    _mesh = &mesh;
    _credentials = TeamCredentials{};
    _enabled = false;
    _sync_request_tx_seen = false;
    _last_sync_request_tx_ms = 0;
    clearMembers();
    mesh.getRNG()->random(reinterpret_cast<uint8_t *>(&_boot_id), sizeof(_boot_id));
    mesh.getRNG()->random(reinterpret_cast<uint8_t *>(&_status_sequence),
                          sizeof(_status_sequence));
    mesh.getRNG()->random(reinterpret_cast<uint8_t *>(&_sync_sequence), sizeof(_sync_sequence));
    // Migration only: never read old credentials, even if their deletion fails.
    (void)store.deleteTeamPrefs();
    installSystemChannels();
    scheduleInitialFrames();
    _startup_sync_attempts = 0;
    _startup_sync_ms = enabled() ? millis() + mesh.getRNG()->nextInt(5000, 15001) : 0;
}

bool TeamService::hasTeam() const
{
    return _credentials.key_version != 0;
}

bool TeamService::enabled() const
{
    return hasTeam() && _enabled;
}

bool TeamService::locationShareEnabled() const
{
    return enabled();
}

const TeamCredentials &TeamService::credentials() const
{
    return _credentials;
}

void TeamService::installSystemChannels()
{
    ChannelDetails join{};
    memcpy(join.channel.secret, kJoinSecret, sizeof(kJoinSecret));
    strncpy(join.name, "_heltec_team_join", sizeof(join.name) - 1);
    _mesh->setChannel(HELTEC_TEAM_JOIN_CHANNEL_INDEX, join);
    ChannelDetails installed_join{};
    (void)_mesh->getChannel(HELTEC_TEAM_JOIN_CHANNEL_INDEX, installed_join);

    if (hasTeam())
    {
        ChannelDetails team{};
        memcpy(team.channel.secret, _credentials.secret, sizeof(_credentials.secret));
        memcpy(team.name, _credentials.name, _credentials.name_len);
        team.name[_credentials.name_len] = '\0';
        _mesh->setChannel(HELTEC_TEAM_CHANNEL_INDEX, team);
        ChannelDetails installed_team{};
        (void)_mesh->getChannel(HELTEC_TEAM_CHANNEL_INDEX, installed_team);
    }
    else
    {
        clearTeamChannel();
    }
}

void TeamService::clearTeamChannel()
{
    ChannelDetails empty{};
    _mesh->setChannel(HELTEC_TEAM_CHANNEL_INDEX, empty);
}

bool TeamService::createTeam(const char *name)
{
    if (_role != TeamRole::Leader || !_mesh || !name)
    {
        return false;
    }
    size_t name_len = strnlen(name, kMaxNameBytes);
    while (name_len > 0 &&
           !valid_utf8(reinterpret_cast<const uint8_t *>(name), name_len))
        --name_len;
    if (name_len == 0)
    {
        return false;
    }

    TeamCredentials next{};
    _mesh->getRNG()->random(reinterpret_cast<uint8_t *>(&next.team_id), sizeof(next.team_id));
    _mesh->getRNG()->random(next.secret, sizeof(next.secret));
    if (next.team_id == 0 || !any_nonzero(next.secret, sizeof(next.secret)))
        return false;
    next.key_version = 1;
    next.name_len = static_cast<uint8_t>(name_len);
    memcpy(next.name, name, name_len);
    memcpy(next.leader_public_key, _mesh->self_id.pub_key, sizeof(next.leader_public_key));
    const bool installed = installTeam(next);
    return installed;
}

bool TeamService::setRole(TeamRole role)
{
    if (_role == role)
        return true;
    (void)leaveTeam();
    _role = role;
    return true;
}

bool TeamService::installTeam(const TeamCredentials &credentials)
{
    if (!_mesh || credentials.key_version == 0 ||
        credentials.team_id == 0 || !any_nonzero(credentials.secret, sizeof(credentials.secret)) ||
        !any_nonzero(credentials.leader_public_key,
                     sizeof(credentials.leader_public_key)) ||
        credentials.name_len == 0 || credentials.name_len > kMaxNameBytes ||
        !valid_utf8(reinterpret_cast<const uint8_t *>(credentials.name), credentials.name_len))
    {
        return false;
    }

    _credentials = credentials;
    _credentials.name[_credentials.name_len] = '\0';
    _enabled = true;
    clearMembers();
    _sync_request_tx_seen = false;
    _last_sync_request_tx_ms = 0;
    installSystemChannels();
    scheduleInitialFrames();
    return true;
}

bool TeamService::leaveTeam()
{
    memset(&_credentials, 0, sizeof(_credentials));
    _enabled = false;
    _sent_valid_location = false;
    _sync_request_tx_seen = false;
    _last_sync_request_tx_ms = 0;
    clearMembers();
    clearTeamChannel();
    return true;
}

bool TeamService::setEnabled(bool enabled)
{
    if (enabled && !hasTeam())
        return false;
    _enabled = enabled;
    if (!enabled)
    {
        _next_status_ms = 0;
        _startup_sync_ms = 0;
        _startup_sync_attempts = 0;
        _pending_status_ms = 0;
    }
    else
    {
        scheduleInitialFrames();
    }
    return true;
}

void TeamService::scheduleInitialFrames()
{
    if (!enabled() || !_mesh)
        return;
    _last_status_sent_ms = 0;
    _next_status_ms = millis() + _mesh->getRNG()->nextInt(1000, 5001);
    // Publish our identity before requesting the other members; all TX stays in poll().
    _startup_sync_ms = _next_status_ms + 1000UL;
    _startup_sync_attempts = 0;
}

CommonHeader TeamService::makeHeader(uint8_t flags, uint16_t sequence) const
{
    CommonHeader header{};
    header.flags = flags;
    header.role = static_cast<uint8_t>(_role);
    memcpy(header.sender_id, _mesh->self_id.pub_key, sizeof(header.sender_id));
    header.boot_id = _boot_id;
    header.sequence = sequence;
    return header;
}

bool TeamService::sendFrame(const uint8_t *data, size_t length)
{
    ChannelDetails channel{};
    if (!_mesh->getChannel(HELTEC_TEAM_CHANNEL_INDEX, channel))
    {
        return false;
    }
    const bool sent = _mesh->sendGroupData(channel.channel, nullptr, OUT_PATH_UNKNOWN,
                                           kDataTypeDev, data, static_cast<int>(length));
    increment(sent ? _stats.tx_queued : _stats.tx_dropped);
    return sent;
}

bool TeamService::sendStatus(bool force_update)
{
    HeltecMesh::StableGpsFixSnapshot gps{};
    const NodePrefs *prefs = _mesh->getNodePrefs();
    const bool valid = prefs && prefs->gps_enabled != 0 &&
                       _mesh->getStableGpsFix(gps) && gps.valid;
    const uint32_t now = millis();
    if (!force_update && _last_status_sent_ms != 0 && valid == _sent_valid_location)
    {
        const bool moved = valid &&
            (abs(static_cast<int32_t>(gps.lat_micro) - _last_sent_latitude_e6) >=
                 HELTEC_TEAM_LOCATION_MOVE_THRESHOLD_E6 ||
             abs(static_cast<int32_t>(gps.lon_micro) - _last_sent_longitude_e6) >=
                 HELTEC_TEAM_LOCATION_MOVE_THRESHOLD_E6);
        const uint32_t period = valid ? HELTEC_TEAM_LOCATION_IDLE_PERIOD_SEC :
                                        HELTEC_TEAM_PRESENCE_PERIOD_SEC;
        if (!moved && now - _last_status_sent_ms < period * 1000UL)
        {
            return true;
        }
    }

    StatusFrame frame{};
    frame.battery_mv = _mesh->currentBatteryMilliVolts();
    frame.header = makeHeader((frame.battery_mv ? kStatusBatteryValid : 0) |
                              (valid ? kStatusLocationValid : 0), _status_sequence);
    memcpy(frame.public_key, _mesh->self_id.pub_key, sizeof(frame.public_key));
    size_t name_len = strnlen(_mesh->getNodeName(), kMaxNameBytes);
    while (name_len > 0 && !valid_utf8(
               reinterpret_cast<const uint8_t *>(_mesh->getNodeName()), name_len))
    {
        --name_len;
    }
    frame.name_len = static_cast<uint8_t>(name_len);
    memcpy(frame.name, _mesh->getNodeName(), name_len);
    if (valid)
    {
        frame.latitude_e6 = static_cast<int32_t>(gps.lat_micro);
        frame.longitude_e6 = static_cast<int32_t>(gps.lon_micro);
        frame.fix_age_sec = saturate_u16(gps.age_ms / 1000UL);
    }
    uint8_t data[kStatusMaxSize] = {};
    size_t length = 0;
    if (!encode_status(frame, data, sizeof(data), length) || !sendFrame(data, length))
    {
        return false;
    }
    ++_status_sequence;
    _sent_valid_location = valid;
    _last_status_sent_ms = now;
    _last_sent_latitude_e6 = frame.latitude_e6;
    _last_sent_longitude_e6 = frame.longitude_e6;
    return true;
}

void TeamService::poll()
{
    if (!enabled())
        return;
    const uint32_t now = millis();
    if (deadline_passed(now, _pending_status_ms))
    {
        const bool sent = sendStatus(true);
        ++_sync_status_attempts;
        _pending_status_ms = !sent && _sync_status_attempts < 3 ? now + 1000UL : 0;
        if (sent)
        {
            _next_status_ms = now + HELTEC_TEAM_LOCATION_PERIOD_SEC * 1000UL;
        }
    }
    else if (_pending_status_ms == 0 && deadline_passed(now, _next_status_ms))
    {
        _next_status_ms = now + (sendStatus() ?
            HELTEC_TEAM_LOCATION_PERIOD_SEC * 1000UL : 5000UL);
    }
    if (_last_status_sent_ms != 0 && deadline_passed(now, _startup_sync_ms))
    {
        _startup_sync_ms = 0;
        const bool sent = requestSync();
        ++_startup_sync_attempts;
        if (!sent && _startup_sync_attempts < 3)
            _startup_sync_ms = now + 5000UL;
    }
}

TeamMember *TeamService::findMember(const uint8_t public_key[kPublicKeySize])
{
    for (TeamMember &member : _members)
    {
        if (member.occupied && memcmp(member.public_key, public_key, kPublicKeySize) == 0)
            return &member;
    }
    return nullptr;
}

TeamMember *TeamService::allocateMember(const uint8_t public_key[kPublicKeySize])
{
    TeamMember *oldest = nullptr;
    for (TeamMember &member : _members)
    {
        if (!member.occupied)
        {
            oldest = &member;
            break;
        }
        if (memcmp(member.public_key, _pinned_member_id, kSenderIdSize) == 0)
            continue;
        if (member.role == 1 && oldest && oldest->role != 1)
            continue;
        if (!oldest || (oldest->role == 1 && member.role != 1) ||
            static_cast<int32_t>(member.last_seen_ms - oldest->last_seen_ms) < 0)
            oldest = &member;
    }
    if (!oldest)
    {
        increment(_stats.member_table_full);
        return nullptr;
    }
    *oldest = TeamMember{};
    oldest->occupied = true;
    memcpy(oldest->public_key, public_key, kPublicKeySize);
    return oldest;
}

void TeamService::receiveStatus(const uint8_t *data, size_t length)
{
    StatusFrame frame{};
    if (decode_status(data, length, frame) != DecodeResult::Ok ||
        !any_nonzero(frame.public_key, sizeof(frame.public_key)))
    {
        increment(_stats.rx_decode_error);
        return;
    }
    if (memcmp(frame.public_key, _mesh->self_id.pub_key, kPublicKeySize) == 0)
    {
        return;
    }
    const bool leader_key = memcmp(frame.public_key, _credentials.leader_public_key,
                                   kPublicKeySize) == 0;
    if ((frame.header.role == 1) != leader_key)
    {
        increment(_stats.rx_decode_error);
        return;
    }
    TeamMember *member = findMember(frame.public_key);
    if (member && member->boot_id == frame.header.boot_id &&
        !sequence_is_newer(frame.header.sequence, member->sequence))
    {
        increment(_stats.rx_duplicate_or_old);
        return;
    }
    bool member_changed = member == nullptr;
    if (member && (member->role != frame.header.role || member->name_len != frame.name_len ||
                   memcmp(member->name, frame.name, frame.name_len) != 0))
    {
        member_changed = true;
    }
    if (!member)
    {
        member = allocateMember(frame.public_key);
    }
    if (!member)
    {
        return;
    }
    member->boot_id = frame.header.boot_id;
    member->sequence = frame.header.sequence;
    member->role = frame.header.role;
    member->battery_mv = frame.battery_mv;
    member->name_len = frame.name_len;
    memcpy(member->name, frame.name, frame.name_len + 1);
    member->location_valid = (frame.header.flags & kStatusLocationValid) != 0;
    member->latitude_e6 = frame.latitude_e6;
    member->longitude_e6 = frame.longitude_e6;
    member->fix_age_sec = frame.fix_age_sec;
    member->last_seen_ms = millis();
    if (member_changed)
    {
        ++_member_revision;
    }
    (void)_mesh->syncTeamMemberContact(frame.public_key, frame.name, frame.name_len);
    ++_change_sequence;
    increment(_stats.rx_status);
}

void TeamService::receiveSyncRequest(const uint8_t *data, size_t length)
{
    SyncRequestFrame frame{};
    if (decode_sync_request(data, length, frame) != DecodeResult::Ok)
    {
        increment(_stats.rx_decode_error);
        return;
    }
    if (memcmp(frame.header.sender_id, _mesh->self_id.pub_key, kSenderIdSize) == 0)
        return;
    const uint32_t now = millis();
    SyncSeen *oldest = &_sync_seen[0];
    for (SyncSeen &seen : _sync_seen)
    {
        if (seen.boot_id == frame.header.boot_id && seen.request_id == frame.request_id &&
            memcmp(seen.sender_id, frame.header.sender_id, kSenderIdSize) == 0)
        {
            increment(_stats.rx_duplicate_or_old);
            return;
        }
        if (seen.seen_ms == 0 ||
            static_cast<int32_t>(seen.seen_ms - oldest->seen_ms) < 0)
            oldest = &seen;
    }
    if (_last_sync_request_rx_ms != 0 && now - _last_sync_request_rx_ms < 5000UL)
    {
        increment(_stats.sync_rate_limited);
        return;
    }
    _last_sync_request_rx_ms = now;
    memcpy(oldest->sender_id, frame.header.sender_id, kSenderIdSize);
    oldest->boot_id = frame.header.boot_id;
    oldest->request_id = frame.request_id;
    oldest->seen_ms = now;
    const uint32_t window_ms = static_cast<uint32_t>(frame.response_window_sec) * 1000UL;
    uint32_t jitter = frame.request_id | (static_cast<uint32_t>(frame.header.boot_id) << 16);
    for (size_t index = 0; index < kSenderIdSize; ++index)
        jitter = (jitter ^ _mesh->self_id.pub_key[index]) * 16777619UL;
    jitter ^= jitter << 13;
    jitter ^= jitter >> 17;
    jitter ^= jitter << 5;
    const uint32_t delay_ms = 1UL + jitter % window_ms;
    // Coalesce requests without postponing an already pending reply.
    if (_pending_status_ms == 0)
    {
        _sync_status_attempts = 0;
        _pending_status_ms = now + delay_ms;
        if (_pending_status_ms == 0)
        {
            _pending_status_ms = 1;
        }
    }
    increment(_stats.rx_sync_request);
}

void TeamService::handleTeamData(const uint8_t *data, size_t length)
{
    if (!enabled() || !data)
    {
        return;
    }
    switch (frame_subtype(data, length))
    {
    case Subtype::Status:
        receiveStatus(data, length);
        break;
    case Subtype::SyncRequest:
        receiveSyncRequest(data, length);
        break;
    default:
        break;
    }
}

void TeamService::noteWrongChannel()
{
    increment(_stats.rx_wrong_channel);
}

bool TeamService::requestSync()
{
    if (!enabled())
    {
        return false;
    }
    const uint32_t now = millis();
    if (_sync_request_tx_seen && now - _last_sync_request_tx_ms < 60000UL)
    {
        increment(_stats.sync_rate_limited);
        return false;
    }
    SyncRequestFrame frame{};
    frame.header = makeHeader(0, _sync_sequence);
    _mesh->getRNG()->random(reinterpret_cast<uint8_t *>(&frame.request_id),
                            sizeof(frame.request_id));
    frame.response_window_sec = 15;
    uint8_t data[32] = {};
    size_t length = 0;
    if (!encode_sync_request(frame, data, sizeof(data), length) || !sendFrame(data, length))
    {
        return false;
    }
    _last_sync_request_tx_ms = now;
    _sync_request_tx_seen = true;
    ++_sync_sequence;
    return true;
}

size_t TeamService::memberCount() const
{
    size_t count = 0;
    for (const TeamMember &member : _members)
    {
        if (member.occupied)
            ++count;
    }
    return count;
}

bool TeamService::memberAt(size_t ordinal, TeamMember &member) const
{
    for (const TeamMember &candidate : _members)
    {
        if (!candidate.occupied)
            continue;
        if (ordinal == 0)
        {
            member = candidate;
            return true;
        }
        --ordinal;
    }
    return false;
}

void TeamService::pinMember(const uint8_t public_key[kPublicKeySize])
{
    if (!public_key)
    {
        memset(_pinned_member_id, 0, sizeof(_pinned_member_id));
        return;
    }
    memcpy(_pinned_member_id, public_key, sizeof(_pinned_member_id));
}

void TeamService::clearMembers()
{
    _next_status_ms = 0;
    _pending_status_ms = 0;
    _startup_sync_ms = 0;
    _startup_sync_attempts = 0;
    _sync_status_attempts = 0;
    _last_status_sent_ms = 0;
    _last_sent_latitude_e6 = 0;
    _last_sent_longitude_e6 = 0;
    _sent_valid_location = false;
    memset(_members, 0, sizeof(_members));
    memset(_sync_seen, 0, sizeof(_sync_seen));
    memset(_pinned_member_id, 0, sizeof(_pinned_member_id));
    _last_sync_request_rx_ms = 0;
    ++_member_revision;
    ++_change_sequence;
}

} // namespace heltec::meshcore::team

#endif
