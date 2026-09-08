/**
 * @file    team_service.cpp
 * @brief   Fixed-memory runtime service for Heltec Team location sharing.
 * @version 1.0
 * @date    2026-09-04
 */
#include "app/team_service.hpp"

#if HELTEC_TEAM_FEATURE

#include "app/HeltecMesh.h"
#include "app/team_log.hpp"
#include "config/DataStore.h"

#include <Arduino.h>
#include <Utils.h>
#include <limits.h>
#include <string.h>

namespace heltec::meshcore::team
{
namespace
{

constexpr uint8_t kLocationValid = 0x10;
constexpr uint8_t kBatteryValid = 0x02;

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

int16_t altitude_dm(long altitude_milli)
{
    const long value = altitude_milli / 100L;
    if (value < INT16_MIN)
        return INT16_MIN;
    if (value > INT16_MAX)
        return INT16_MAX;
    return static_cast<int16_t>(value);
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
    mesh.getRNG()->random(reinterpret_cast<uint8_t *>(&_presence_sequence),
                          sizeof(_presence_sequence));
    mesh.getRNG()->random(reinterpret_cast<uint8_t *>(&_location_sequence),
                          sizeof(_location_sequence));
    mesh.getRNG()->random(reinterpret_cast<uint8_t *>(&_sync_sequence), sizeof(_sync_sequence));
    // Migration only: never read old credentials, even if their deletion fails.
    if (!store.deleteTeamPrefs())
        TEAM_INFO("legacy Team preferences cleanup failed; ignored");
    installSystemChannels();
    scheduleInitialFrames();
    _startup_sync_attempts = 0;
    _startup_sync_ms = enabled() ? millis() + mesh.getRNG()->nextInt(5000, 15001) : 0;
    TEAM_INFO("service begin role=%u volatile=1 has_team=0", HELTEC_TEAM_ROLE);
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
    TEAM_LOG("channel install join_index=%u hash=%02X", HELTEC_TEAM_JOIN_CHANNEL_INDEX,
             installed_join.channel.hash[0]);

    if (hasTeam())
    {
        ChannelDetails team{};
        memcpy(team.channel.secret, _credentials.secret, sizeof(_credentials.secret));
        memcpy(team.name, _credentials.name, _credentials.name_len);
        team.name[_credentials.name_len] = '\0';
        _mesh->setChannel(HELTEC_TEAM_CHANNEL_INDEX, team);
        ChannelDetails installed_team{};
        (void)_mesh->getChannel(HELTEC_TEAM_CHANNEL_INDEX, installed_team);
        TEAM_LOG("channel install team_index=%u hash=%02X team=%08lX",
                 HELTEC_TEAM_CHANNEL_INDEX, installed_team.channel.hash[0],
                 static_cast<unsigned long>(_credentials.team_id));
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
#if HELTEC_TEAM_ROLE != 1
    (void)name;
    return false;
#else
    if (!_mesh || !name)
    {
        TEAM_INFO("create rejected: service/name unavailable");
        return false;
    }
    size_t name_len = strnlen(name, kMaxNameBytes);
    while (name_len > 0 &&
           !valid_utf8(reinterpret_cast<const uint8_t *>(name), name_len))
        --name_len;
    if (name_len == 0)
    {
        TEAM_INFO("create rejected: invalid name");
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
    TEAM_INFO("create result=%u team=%08lX", installed,
             static_cast<unsigned long>(next.team_id));
    return installed;
#endif
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
        TEAM_INFO("install rejected: invalid credentials");
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
    TEAM_INFO("install ok team=%08lX key_version=%lu name_len=%u",
             static_cast<unsigned long>(credentials.team_id),
             static_cast<unsigned long>(credentials.key_version), credentials.name_len);
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
    TEAM_INFO("leave ok");
    return true;
}

bool TeamService::setEnabled(bool enabled)
{
    if (enabled && !hasTeam())
        return false;
    _enabled = enabled;
    if (!enabled)
    {
        _next_presence_ms = 0;
        _next_location_ms = 0;
        _startup_sync_ms = 0;
        _startup_sync_attempts = 0;
        _pending_presence_ms = 0;
        _pending_location_ms = 0;
    }
    else
    {
        scheduleInitialFrames();
        _startup_sync_attempts = 0;
        _startup_sync_ms = millis() + _mesh->getRNG()->nextInt(1000, 5001);
    }
    TEAM_LOG("enable=%u ok", enabled);
    return true;
}

void TeamService::scheduleInitialFrames()
{
    if (!enabled() || !_mesh)
        return;
    _next_presence_ms = millis() + _mesh->getRNG()->nextInt(0, 5001);
    _next_location_ms = enabled() ? millis() + _mesh->getRNG()->nextInt(1000, 5001) : 0;
    TEAM_LOG("location scheduled period_s=%u idle_s=%u gps_on=%u",
             HELTEC_TEAM_LOCATION_PERIOD_SEC, HELTEC_TEAM_LOCATION_IDLE_PERIOD_SEC,
             _mesh->getNodePrefs()->gps_enabled != 0);
}

CommonHeader TeamService::makeHeader(uint8_t flags, uint16_t sequence) const
{
    CommonHeader header{};
    header.flags = flags;
    header.role = HELTEC_TEAM_ROLE;
    memcpy(header.sender_id, _mesh->self_id.pub_key, sizeof(header.sender_id));
    header.boot_id = _boot_id;
    header.sequence = sequence;
    header.timestamp = _mesh->getRTCClock()->getCurrentTime();
    if (header.timestamp != 0)
        header.flags |= 0x01;
    return header;
}

bool TeamService::sendFrame(const uint8_t *data, size_t length)
{
    ChannelDetails channel{};
    if (!_mesh->getChannel(HELTEC_TEAM_CHANNEL_INDEX, channel))
    {
        TEAM_INFO("tx failed subtype=%u: team channel unavailable", data ? data[1] : 0);
        return false;
    }
    const bool sent = _mesh->sendGroupData(channel.channel, nullptr, OUT_PATH_UNKNOWN,
                                           kDataTypeDev, data, static_cast<int>(length));
    increment(sent ? _stats.tx_queued : _stats.tx_dropped);
    TEAM_LOG("tx subtype=%u bytes=%u queued=%u", data ? data[1] : 0,
             static_cast<unsigned int>(length), sent);
    return sent;
}

bool TeamService::sendPresence()
{
    PresenceFrame frame{};
    frame.battery_mv = _mesh->currentBatteryMilliVolts();
    frame.header = makeHeader(frame.battery_mv ? kBatteryValid : 0, _presence_sequence);
    frame.device_type =
#if defined(HELTEC_T1)
        1;
#else
        2;
#endif
    memcpy(frame.public_key, _mesh->self_id.pub_key, sizeof(frame.public_key));
    frame.capabilities = 0;
#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
    frame.capabilities |= 0x0001;
#endif
#if HELTEC_TEAM_ROLE == 1
    frame.capabilities |= 0x0008;
#endif
    size_t name_len = strnlen(_mesh->getNodeName(), kMaxNameBytes);
    while (name_len > 0 && !valid_utf8(
               reinterpret_cast<const uint8_t *>(_mesh->getNodeName()), name_len))
        --name_len;
    frame.name_len = static_cast<uint8_t>(name_len);
    memcpy(frame.name, _mesh->getNodeName(), frame.name_len);
    uint8_t data[87] = {};
    size_t length = 0;
    if (!encode_presence(frame, data, sizeof(data), length) || !sendFrame(data, length))
        return false;
    TEAM_LOG("presence seq=%u battery_mv=%u", frame.header.sequence, frame.battery_mv);
    ++_presence_sequence;
    return true;
}

bool TeamService::sendLocation(bool force_invalid, bool force_update)
{
    HeltecMesh::StableGpsFixSnapshot gps{};
    const NodePrefs *prefs = _mesh->getNodePrefs();
    const bool valid = !force_invalid && prefs && prefs->gps_enabled != 0 &&
                       _mesh->getStableGpsFix(gps) && gps.valid;
    if (!valid && !force_invalid)
    {
        if (!_sent_valid_location && !force_update)
        {
            TEAM_LOG("location skipped reason=%s", prefs && prefs->gps_enabled ?
                     "no_valid_fix" : "gps_off");
            return true;
        }
        TEAM_LOG("location invalidation force_sync=%u", force_update);
        force_invalid = true;
    }
    if (valid && !force_update && _last_location_sent_ms != 0)
    {
        const int32_t lat_delta = abs(static_cast<int32_t>(gps.lat_micro) -
                                      _last_sent_latitude_e6);
        const int32_t lon_delta = abs(static_cast<int32_t>(gps.lon_micro) -
                                      _last_sent_longitude_e6);
        const bool moved = lat_delta >= HELTEC_TEAM_LOCATION_MOVE_THRESHOLD_E6 ||
                           lon_delta >= HELTEC_TEAM_LOCATION_MOVE_THRESHOLD_E6;
        const bool idle_due = millis() - _last_location_sent_ms >=
                              HELTEC_TEAM_LOCATION_IDLE_PERIOD_SEC * 1000UL;
        if (!moved && !idle_due)
        {
            TEAM_LOG("location skipped reason=stationary age_s=%lu",
                     static_cast<unsigned long>((millis() - _last_location_sent_ms) / 1000UL));
            return true;
        }
    }

    LocationFrame frame{};
    frame.battery_mv = _mesh->currentBatteryMilliVolts();
    uint8_t flags = frame.battery_mv ? kBatteryValid : 0;
    if (valid)
        flags |= kLocationValid;
    frame.header = makeHeader(flags, _location_sequence);
    if (valid)
    {
        frame.latitude_e6 = static_cast<int32_t>(gps.lat_micro);
        frame.longitude_e6 = static_cast<int32_t>(gps.lon_micro);
        frame.altitude_dm = altitude_dm(gps.alt_milli);
        frame.fix_age_sec = saturate_u16(gps.age_ms / 1000UL);
        frame.satellites = gps.satellites > UINT8_MAX ? UINT8_MAX
                                                       : static_cast<uint8_t>(gps.satellites);
    }
    uint8_t data[48] = {};
    size_t length = 0;
    if (!encode_location(frame, data, sizeof(data), length) || !sendFrame(data, length))
    {
        TEAM_INFO("location queue_failed seq=%u sync=%u", frame.header.sequence, force_update);
        return false;
    }
    TEAM_LOG("location seq=%u valid=%u fix_age=%u sats=%u", frame.header.sequence,
             valid, frame.fix_age_sec, frame.satellites);
    TEAM_LOG("location queued sync=%u lat_e6=%ld lon_e6=%ld", force_update,
             static_cast<long>(frame.latitude_e6), static_cast<long>(frame.longitude_e6));
    ++_location_sequence;
    _sent_valid_location = valid;
    _last_location_sent_ms = millis();
    if (valid)
    {
        _last_sent_latitude_e6 = frame.latitude_e6;
        _last_sent_longitude_e6 = frame.longitude_e6;
    }
    return true;
}

void TeamService::poll()
{
    if (!enabled())
        return;
    const uint32_t now = millis();
    if (deadline_passed(now, _startup_sync_ms))
    {
        _startup_sync_ms = 0;
        const bool sent = requestSync();
        ++_startup_sync_attempts;
        TEAM_LOG("startup member sync queued=%u attempt=%u", sent,
                 _startup_sync_attempts);
        if (!sent && _startup_sync_attempts < 3)
            _startup_sync_ms = now + 5000UL;
    }
    if (_pending_presence_ms == 0 && deadline_passed(now, _next_presence_ms))
    {
        if (sendPresence())
            _next_presence_ms = now + HELTEC_TEAM_PRESENCE_PERIOD_SEC * 1000UL;
        else
            _next_presence_ms = now + 5000UL;
    }
    if (enabled() && _pending_presence_ms == 0 && _pending_location_ms == 0 &&
        deadline_passed(now, _next_location_ms))
    {
        if (sendLocation())
            _next_location_ms = now + HELTEC_TEAM_LOCATION_PERIOD_SEC * 1000UL;
        else
            _next_location_ms = now + 5000UL;
    }
    if (deadline_passed(now, _pending_presence_ms))
    {
        const bool sent = sendPresence();
        TEAM_LOG("sync presence response due queued=%u", sent);
        ++_sync_presence_attempts;
        _pending_presence_ms = !sent && _sync_presence_attempts < 3 ? now + 1000UL : 0;
        if (sent)
            _next_presence_ms = now + HELTEC_TEAM_PRESENCE_PERIOD_SEC * 1000UL;
        if (!sent && _pending_presence_ms == 0)
        {
            // Do not send a location-only reply when identity could not be queued.
            _pending_location_ms = 0;
        }
        else if (sent && _pending_location_ms != 0 &&
                 static_cast<int32_t>(_pending_location_ms - now) < 300)
        {
            _pending_location_ms = now + 300UL;
        }
    }
    if (enabled() && _pending_presence_ms == 0 &&
        deadline_passed(now, _pending_location_ms))
    {
        const bool sent = sendLocation(false, true);
        TEAM_LOG("sync location response due queued=%u", sent);
        ++_sync_location_attempts;
        _pending_location_ms = !sent && _sync_location_attempts < 3 ? now + 1000UL : 0;
        if (sent)
            _next_location_ms = now + HELTEC_TEAM_LOCATION_PERIOD_SEC * 1000UL;
    }
}

TeamMember *TeamService::findMember(const uint8_t sender_id[kSenderIdSize])
{
    for (TeamMember &member : _members)
    {
        if (member.occupied && memcmp(member.sender_id, sender_id, kSenderIdSize) == 0)
            return &member;
    }
    return nullptr;
}

TeamMember *TeamService::allocateMember(const uint8_t sender_id[kSenderIdSize])
{
    TeamMember *oldest = nullptr;
    for (TeamMember &member : _members)
    {
        if (!member.occupied)
        {
            oldest = &member;
            break;
        }
        if (memcmp(member.sender_id, _pinned_member_id, kSenderIdSize) == 0)
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
    memcpy(oldest->sender_id, sender_id, kSenderIdSize);
    return oldest;
}

void TeamService::receivePresence(const uint8_t *data, size_t length)
{
    PresenceFrame frame{};
    if (decode_presence(data, length, frame) != DecodeResult::Ok)
    {
        increment(_stats.rx_decode_error);
        TEAM_LOG("rx presence decode_error bytes=%u", static_cast<unsigned int>(length));
        return;
    }
    if (memcmp(frame.public_key, _mesh->self_id.pub_key, kPublicKeySize) == 0)
        return;
    if (memcmp(frame.header.sender_id, frame.public_key, kSenderIdSize) != 0)
    {
        increment(_stats.rx_decode_error);
        TEAM_INFO("rx presence rejected: sender/public key mismatch id=%02X%02X%02X",
                 frame.header.sender_id[0], frame.header.sender_id[1],
                 frame.header.sender_id[2]);
        return;
    }
    const bool leader_key = memcmp(frame.public_key, _credentials.leader_public_key,
                                   kPublicKeySize) == 0;
    if ((frame.header.role == 1) != leader_key)
    {
        increment(_stats.rx_decode_error);
        TEAM_LOG("rx presence role_mismatch id=%02X%02X%02X", frame.header.sender_id[0],
                 frame.header.sender_id[1], frame.header.sender_id[2]);
        return;
    }
    TeamMember *member = findMember(frame.header.sender_id);
    if (!member)
        member = allocateMember(frame.header.sender_id);
    if (!member)
        return;
    if (member->presence_seen &&
        memcmp(member->public_key, frame.public_key, kPublicKeySize) != 0)
    {
        increment(_stats.rx_decode_error);
        TEAM_INFO("rx presence rejected: sender collision id=%02X%02X%02X",
                 frame.header.sender_id[0], frame.header.sender_id[1],
                 frame.header.sender_id[2]);
        return;
    }
    if (member->boot_id == frame.header.boot_id && member->presence_seen &&
        !sequence_is_newer(frame.header.sequence, member->presence_sequence))
    {
        increment(_stats.rx_duplicate_or_old);
        TEAM_LOG("rx presence old id=%02X%02X%02X seq=%u current=%u",
                 frame.header.sender_id[0], frame.header.sender_id[1],
                 frame.header.sender_id[2], frame.header.sequence,
                 member->presence_sequence);
        return;
    }
    if (member->boot_id != frame.header.boot_id)
    {
        member->location_valid = false;
        member->location_seen = false;
        member->location_sequence = 0;
    }
    member->boot_id = frame.header.boot_id;
    member->presence_sequence = frame.header.sequence;
    member->presence_seen = true;
    member->role = frame.header.role;
    member->battery_mv = frame.battery_mv;
    member->name_len = frame.name_len;
    memcpy(member->name, frame.name, frame.name_len + 1);
    memcpy(member->public_key, frame.public_key, sizeof(member->public_key));
    member->last_seen_ms = millis();
    ++_change_sequence;
    increment(_stats.rx_presence);
    TEAM_LOG("rx presence id=%02X%02X%02X seq=%u role=%u members=%u",
             frame.header.sender_id[0], frame.header.sender_id[1], frame.header.sender_id[2],
             frame.header.sequence, frame.header.role,
             static_cast<unsigned int>(memberCount()));
}

void TeamService::receiveLocation(const uint8_t *data, size_t length)
{
    LocationFrame frame{};
    if (decode_location(data, length, frame) != DecodeResult::Ok)
    {
        increment(_stats.rx_decode_error);
        TEAM_LOG("rx location decode_error bytes=%u", static_cast<unsigned int>(length));
        return;
    }
    TeamMember *member = findMember(frame.header.sender_id);
    if (!member)
    {
        increment(_stats.rx_duplicate_or_old);
        TEAM_INFO("rx location rejected: no presence id=%02X%02X%02X",
                 frame.header.sender_id[0], frame.header.sender_id[1],
                 frame.header.sender_id[2]);
        return;
    }
    if (member->boot_id != frame.header.boot_id || member->role != frame.header.role)
    {
        increment(_stats.rx_duplicate_or_old);
        TEAM_INFO("rx location rejected: identity state id=%02X%02X%02X",
                 frame.header.sender_id[0], frame.header.sender_id[1],
                 frame.header.sender_id[2]);
        return;
    }
    if (member->location_seen &&
        !sequence_is_newer(frame.header.sequence, member->location_sequence))
    {
        increment(_stats.rx_duplicate_or_old);
        TEAM_LOG("rx location old id=%02X%02X%02X seq=%u current=%u",
                 frame.header.sender_id[0], frame.header.sender_id[1],
                 frame.header.sender_id[2], frame.header.sequence,
                 member->location_sequence);
        return;
    }
    member->location_sequence = frame.header.sequence;
    member->location_seen = true;
    member->location_valid = (frame.header.flags & kLocationValid) != 0;
    member->latitude_e6 = frame.latitude_e6;
    member->longitude_e6 = frame.longitude_e6;
    member->accuracy_dm = frame.accuracy_dm;
    member->fix_age_sec = frame.fix_age_sec;
    member->battery_mv = frame.battery_mv;
    member->satellites = frame.satellites;
    member->location_received_ms = millis();
    member->last_seen_ms = member->location_received_ms;
    ++_change_sequence;
    increment(_stats.rx_location);
    TEAM_LOG("rx location id=%02X%02X%02X seq=%u valid=%u age=%u sats=%u",
             frame.header.sender_id[0], frame.header.sender_id[1], frame.header.sender_id[2],
             frame.header.sequence, member->location_valid, frame.fix_age_sec,
             frame.satellites);
    TEAM_LOG("rx location applied lat_e6=%ld lon_e6=%ld",
             static_cast<long>(frame.latitude_e6), static_cast<long>(frame.longitude_e6));
}

void TeamService::receiveSyncRequest(const uint8_t *data, size_t length)
{
    SyncRequestFrame frame{};
    if (decode_sync_request(data, length, frame) != DecodeResult::Ok)
    {
        increment(_stats.rx_decode_error);
        TEAM_LOG("rx sync decode_error bytes=%u", static_cast<unsigned int>(length));
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
            TEAM_LOG("rx sync duplicate id=%02X%02X%02X request=%04X",
                     frame.header.sender_id[0], frame.header.sender_id[1],
                     frame.header.sender_id[2], frame.request_id);
            return;
        }
        if (seen.seen_ms == 0 ||
            static_cast<int32_t>(seen.seen_ms - oldest->seen_ms) < 0)
            oldest = &seen;
    }
    if (_last_sync_request_rx_ms != 0 && now - _last_sync_request_rx_ms < 5000UL)
    {
        increment(_stats.sync_rate_limited);
        TEAM_LOG("rx sync rate_limited id=%02X%02X%02X", frame.header.sender_id[0],
                 frame.header.sender_id[1], frame.header.sender_id[2]);
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
    uint32_t presence_delay_ms = 0;
    uint32_t location_delay_ms = 0;
    if ((frame.response_mask & 0x01) != 0)
    {
        const uint32_t reserve_ms = ((frame.response_mask & 0x02) != 0 && enabled()) ?
                                    1000UL : 0;
        const uint32_t presence_window_ms = window_ms > reserve_ms ?
                                            window_ms - reserve_ms : 0;
        presence_delay_ms = jitter % (presence_window_ms + 1UL);
        const uint32_t deadline = now + presence_delay_ms;
        if (_pending_presence_ms == 0 ||
            presence_delay_ms < _pending_presence_ms - now)
        {
            if (_pending_presence_ms == 0)
                _sync_presence_attempts = 0;
            _pending_presence_ms = deadline;
        }
    }
    if ((frame.response_mask & 0x02) != 0 && enabled())
    {
        const uint32_t after_presence_ms = 300UL + ((jitter >> 16) % 701UL);
        location_delay_ms = presence_delay_ms + after_presence_ms;
        if (location_delay_ms > window_ms)
            location_delay_ms = window_ms;
        const uint32_t deadline = now + location_delay_ms;
        if (_pending_location_ms == 0 || location_delay_ms < _pending_location_ms - now)
        {
            if (_pending_location_ms == 0)
                _sync_location_attempts = 0;
            _pending_location_ms = deadline;
        }
    }
    increment(_stats.rx_sync_request);
    TEAM_LOG("rx sync id=%02X%02X%02X mask=%02X window=%u presence_ms=%lu location_ms=%lu",
             frame.header.sender_id[0], frame.header.sender_id[1], frame.header.sender_id[2],
             frame.response_mask, frame.response_window_sec,
             static_cast<unsigned long>(presence_delay_ms),
             static_cast<unsigned long>(location_delay_ms));
}

void TeamService::handleTeamData(const uint8_t *data, size_t length)
{
    if (!enabled() || !data)
    {
        TEAM_LOG("team rx ignored enabled=%u data=%u bytes=%u", enabled(), data != nullptr,
                 static_cast<unsigned int>(length));
        return;
    }
    TEAM_LOG("team rx subtype=%u bytes=%u", static_cast<unsigned int>(frame_subtype(data, length)),
             static_cast<unsigned int>(length));
    switch (frame_subtype(data, length))
    {
    case Subtype::Presence:
        receivePresence(data, length);
        break;
    case Subtype::Location:
        receiveLocation(data, length);
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
    TEAM_INFO("rx rejected: wrong channel");
}

bool TeamService::requestSync()
{
    if (!enabled())
    {
        TEAM_INFO("sync request rejected: team disabled");
        return false;
    }
    const uint32_t now = millis();
    if (_sync_request_tx_seen && now - _last_sync_request_tx_ms < 60000UL)
    {
        increment(_stats.sync_rate_limited);
        TEAM_INFO("sync request rejected: tx cooldown remaining_ms=%lu",
                 static_cast<unsigned long>(60000UL - (now - _last_sync_request_tx_ms)));
        return false;
    }
    SyncRequestFrame frame{};
    frame.header = makeHeader(0, _sync_sequence);
    _mesh->getRNG()->random(reinterpret_cast<uint8_t *>(&frame.request_id),
                            sizeof(frame.request_id));
    frame.response_mask = 0x03;
    frame.response_window_sec = 15;
    uint8_t data[32] = {};
    size_t length = 0;
    if (!encode_sync_request(frame, data, sizeof(data), length) || !sendFrame(data, length))
        return false;
    _last_sync_request_tx_ms = now;
    _sync_request_tx_seen = true;
    TEAM_LOG("sync request seq=%u request=%08lX", frame.header.sequence,
             static_cast<unsigned long>(frame.request_id));
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
    _next_presence_ms = 0;
    _next_location_ms = 0;
    _pending_presence_ms = 0;
    _pending_location_ms = 0;
    _startup_sync_ms = 0;
    _startup_sync_attempts = 0;
    _sync_presence_attempts = 0;
    _sync_location_attempts = 0;
    _last_location_sent_ms = 0;
    _last_sent_latitude_e6 = 0;
    _last_sent_longitude_e6 = 0;
    _sent_valid_location = false;
    memset(_members, 0, sizeof(_members));
    memset(_sync_seen, 0, sizeof(_sync_seen));
    memset(_pinned_member_id, 0, sizeof(_pinned_member_id));
    _last_sync_request_rx_ms = 0;
    ++_change_sequence;
}

} // namespace heltec::meshcore::team

#endif
