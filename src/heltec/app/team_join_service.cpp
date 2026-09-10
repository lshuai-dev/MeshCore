/**
 * @file    team_join_service.cpp
 * @brief   Fixed-memory, user-confirmed Team pairing service.
 * @version 1.0
 * @date    2026-09-04
 */
#include "app/team_join_service.hpp"

#if HELTEC_TEAM_FEATURE

#include "app/HeltecMesh.h"
#include "app/team_log.hpp"

#include <Arduino.h>
#include <ChaChaPoly.h>
#include <HKDF.h>
#include <SHA256.h>
#include <string.h>
#if HELTEC_TEAM_LOG && defined(ESP32)
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#endif

namespace heltec::meshcore::team
{
namespace
{

constexpr uint32_t kSearchDurationMs = 15000UL;
constexpr uint32_t kSessionDurationMs = 30000UL;
constexpr uint8_t kDecisionApprove = 1;
constexpr uint8_t kDecisionReject = 0;
constexpr uint8_t kAckOk = 1;
constexpr uint32_t kDiscoverMinIntervalMs = 250UL;
constexpr uint32_t kDiscoverRateWindowMs = 10000UL;
constexpr uint8_t kDiscoverRateLimit = 12;

void put_u32(uint8_t *data, uint32_t value)
{
    data[0] = static_cast<uint8_t>(value);
    data[1] = static_cast<uint8_t>(value >> 8);
    data[2] = static_cast<uint8_t>(value >> 16);
    data[3] = static_cast<uint8_t>(value >> 24);
}

uint32_t get_u32(const uint8_t *data)
{
    return static_cast<uint32_t>(data[0]) | (static_cast<uint32_t>(data[1]) << 8) |
           (static_cast<uint32_t>(data[2]) << 16) | (static_cast<uint32_t>(data[3]) << 24);
}

void put_u64(uint8_t *data, uint64_t value)
{
    put_u32(data, static_cast<uint32_t>(value));
    put_u32(data + 4, static_cast<uint32_t>(value >> 32));
}

uint64_t get_u64(const uint8_t *data)
{
    return static_cast<uint64_t>(get_u32(data)) |
           (static_cast<uint64_t>(get_u32(data + 4)) << 32);
}

bool expired(uint32_t now, uint32_t deadline)
{
    return deadline != 0 && static_cast<int32_t>(now - deadline) >= 0;
}

void secure_clear(void *data, size_t length)
{
    volatile uint8_t *bytes = static_cast<volatile uint8_t *>(data);
    while (length-- != 0)
        *bytes++ = 0;
}

void increment(uint16_t &counter)
{
    if (counter != UINT16_MAX)
        ++counter;
}

void make_aad(const uint8_t transcript[32], Subtype subtype, uint8_t direction,
              uint8_t aad[34])
{
    memcpy(aad, transcript, 32);
    aad[32] = static_cast<uint8_t>(subtype);
    aad[33] = direction;
}

bool aead_encrypt(const uint8_t key[32], const uint8_t nonce[kAeadNonceSize],
                  const uint8_t aad[34], const uint8_t *plain, size_t plain_len,
                  uint8_t *cipher, uint8_t tag[kAeadTagSize])
{
    ChaChaPoly aead;
    if (!aead.setKey(key, 32) || !aead.setIV(nonce, kAeadNonceSize))
        return false;
    aead.addAuthData(aad, 34);
    aead.encrypt(cipher, plain, plain_len);
    aead.computeTag(tag, kAeadTagSize);
    aead.clear();
    return true;
}

bool aead_decrypt(const uint8_t key[32], const uint8_t nonce[kAeadNonceSize],
                  const uint8_t aad[34], const uint8_t *cipher, size_t cipher_len,
                  const uint8_t tag[kAeadTagSize], uint8_t *plain)
{
    ChaChaPoly aead;
    if (!aead.setKey(key, 32) || !aead.setIV(nonce, kAeadNonceSize))
        return false;
    aead.addAuthData(aad, 34);
    aead.decrypt(plain, cipher, cipher_len);
    const bool valid = aead.checkTag(tag, kAeadTagSize);
    aead.clear();
    if (!valid)
        secure_clear(plain, cipher_len);
    return valid;
}

} // namespace

void TeamJoinService::begin(HeltecMesh &mesh, TeamService &team)
{
    _mesh = &mesh;
    _team = &team;
    _state = JoinState::Idle;
    clearAllSessions();
    TEAM_INFO("join service begin role=%u", HELTEC_TEAM_ROLE);
#if HELTEC_TEAM_LOG && defined(ESP32)
    TEAM_LOG("boot reset_reason=%u heap=%lu stack_hwm=%lu",
             static_cast<unsigned>(esp_reset_reason()),
             static_cast<unsigned long>(ESP.getFreeHeap()),
             static_cast<unsigned long>(uxTaskGetStackHighWaterMark(nullptr)));
#endif
}

JoinState TeamJoinService::state() const
{
    return _state;
}

uint16_t TeamJoinService::pairingCode() const
{
    return _pairing_code;
}

bool TeamJoinService::sendJoin(const uint8_t *data, size_t length, uint32_t delay_ms)
{
    if (!_mesh || !data || length < 2 || length > MAX_GROUP_DATA_LENGTH)
    {
        TEAM_INFO("join tx rejected bytes=%u", static_cast<unsigned int>(length));
        return false;
    }
    ChannelDetails channel{};
    if (!_mesh->getChannel(HELTEC_TEAM_JOIN_CHANNEL_INDEX, channel))
        return false;
    const bool sent = _mesh->sendGroupDataZeroHop(channel.channel, kDataTypeDev, data,
                                                  static_cast<int>(length), delay_ms);
    TEAM_LOG("join tx subtype=%u bytes=%u delay_ms=%lu queued=%u", data[1],
             static_cast<unsigned int>(length), static_cast<unsigned long>(delay_ms), sent);
    return sent;
}

bool TeamJoinService::openJoinWindow()
{
#if HELTEC_TEAM_ROLE != 1
    return false;
#else
    if (!_team || !_team->hasTeam())
        return false;
    if (_join_window_end_ms != 0)
        closeJoinWindow();
    else
        clearAllSessions();
    _join_window_end_ms = millis() + HELTEC_TEAM_JOIN_WINDOW_SEC * 1000UL;
    _state = JoinState::Idle;
    TEAM_INFO("join window open seconds=%u", HELTEC_TEAM_JOIN_WINDOW_SEC);
    return true;
#endif
}

void TeamJoinService::closeJoinWindow()
{
    _join_window_end_ms = 0;
#if HELTEC_TEAM_ROLE == 1
    for (Session &session : _sessions)
    {
        if (session.occupied && session.request_received && !session.decision_sent)
        {
            (void)sendDecision(session, false);
            increment(_stats.rejected);
        }
    }
    clearAllSessions();
#endif
    if (_state == JoinState::PendingApproval || _state == JoinState::SendingCredentials)
        _state = JoinState::TimedOut;
    TEAM_INFO("join window closed state=%u", static_cast<unsigned int>(_state));
}

bool TeamJoinService::joinWindowOpen() const
{
#if HELTEC_TEAM_ROLE == 1
    return _join_window_end_ms != 0 && !expired(millis(), _join_window_end_ms);
#else
    return false;
#endif
}

uint16_t TeamJoinService::joinWindowRemainingSec() const
{
    if (!joinWindowOpen())
        return 0;
    return static_cast<uint16_t>((_join_window_end_ms - millis() + 999UL) / 1000UL);
}

bool TeamJoinService::discoverTeams()
{
#if HELTEC_TEAM_ROLE == 1
    return false;
#else
    if (!_mesh)
        return false;
    cancel();
    _mesh->getRNG()->random(reinterpret_cast<uint8_t *>(&_request_id), sizeof(_request_id));
    _search_end_ms = millis() + kSearchDurationMs;
    _next_discover_ms = millis();
    _discover_count = 0;
    _state = JoinState::Searching;
    TEAM_INFO("discovery started request=%08lX", static_cast<unsigned long>(_request_id));
    return true;
#endif
}

bool TeamJoinService::sendDiscover()
{
    uint8_t frame[6] = { kProtocolVersion, static_cast<uint8_t>(Subtype::JoinDiscover) };
    put_u32(frame + 2, _request_id);
    return sendJoin(frame, sizeof(frame));
}

void TeamJoinService::poll()
{
    const uint32_t now = millis();
#if HELTEC_TEAM_ROLE == 1
    if (_join_window_end_ms != 0 && expired(now, _join_window_end_ms))
        closeJoinWindow();
    for (Session &session : _sessions)
    {
        // RX only records the request. Build/encrypt the offer after RX has unwound.
        if (session.occupied && session.offer_pending &&
            !expired(now, session.expires_ms) && joinWindowOpen())
        {
            session.offer_pending = false;
#if HELTEC_TEAM_LOG && defined(ESP32)
            TEAM_LOG("offer poll heap=%lu stack_hwm=%lu",
                     static_cast<unsigned long>(ESP.getFreeHeap()),
                     static_cast<unsigned long>(uxTaskGetStackHighWaterMark(nullptr)));
#endif
            const bool sent = sendOffer(session);
            TEAM_LOG("offer poll complete queued=%u", sent ? 1U : 0U);
            (void)sent;
        }
        if (session.occupied && expired(now, session.expires_ms))
        {
            if (session.request_received && !session.decision_sent)
            {
                (void)sendDecision(session, false);
                increment(_stats.rejected);
            }
            clearSession(session);
            _state = JoinState::TimedOut;
            increment(_stats.timeout);
        }
    }
    PendingJoin pending{};
    if (pendingJoin(pending))
    {
        _pairing_code = pending.pairing_code;
        _state = JoinState::PendingApproval;
    }
#else
    if (_state == JoinState::Searching)
    {
        if (expired(now, _search_end_ms))
        {
            _state = _offer_count == 0 ? JoinState::TimedOut : JoinState::OffersAvailable;
        }
        else if (_discover_count < 3 && expired(now, _next_discover_ms))
        {
            if (sendDiscover())
                ++_discover_count;
            _next_discover_ms = now + 4000UL + _mesh->getRNG()->nextInt(0, 1501);
        }
    }
    else if (_state == JoinState::WaitingForApproval && expired(now, _decision_end_ms))
    {
        clearMemberEphemeral();
        _state = JoinState::TimedOut;
        increment(_stats.timeout);
    }
#endif
}

void TeamJoinService::handleJoinData(const uint8_t *data, size_t length)
{
    TEAM_LOG("join rx subtype=%u bytes=%u", static_cast<unsigned int>(frame_subtype(data, length)),
             static_cast<unsigned int>(length));
    switch (frame_subtype(data, length))
    {
    case Subtype::JoinDiscover:
        receiveDiscover(data, length);
        break;
    case Subtype::JoinOffer:
        receiveOffer(data, length);
        break;
    case Subtype::JoinRequest:
        receiveRequest(data, length);
        break;
    case Subtype::JoinDecision:
        receiveDecision(data, length);
        break;
    case Subtype::JoinAck:
        receiveAck(data, length);
        break;
    default:
        break;
    }
}

TeamJoinService::Session *TeamJoinService::findSession(uint64_t session_id)
{
#if HELTEC_TEAM_ROLE == 1
    for (Session &session : _sessions)
    {
        if (session.occupied && session.session_id == session_id)
            return &session;
    }
#else
    (void)session_id;
#endif
    return nullptr;
}

TeamJoinService::Session *TeamJoinService::findSessionByRequest(uint32_t request_id)
{
#if HELTEC_TEAM_ROLE == 1
    for (Session &session : _sessions)
    {
        if (session.occupied && session.request_id == request_id)
            return &session;
    }
#else
    (void)request_id;
#endif
    return nullptr;
}

TeamJoinService::Session *TeamJoinService::allocateSession()
{
#if HELTEC_TEAM_ROLE == 1
    for (Session &session : _sessions)
    {
        if (!session.occupied)
        {
            session = Session{};
            session.occupied = true;
            return &session;
        }
    }
#endif
    return nullptr;
}

void TeamJoinService::clearSession(Session &session)
{
    secure_clear(&session, sizeof(session));
}

void TeamJoinService::clearAllSessions()
{
#if HELTEC_TEAM_ROLE == 1
    secure_clear(_sessions, sizeof(_sessions));
#else
    secure_clear(_offers, sizeof(_offers));
    secure_clear(&_selected, sizeof(_selected));
    secure_clear(_member_nonce, sizeof(_member_nonce));
    _offer_count = 0;
#endif
    _pairing_code = 0;
}

void TeamJoinService::clearMemberEphemeral()
{
#if HELTEC_TEAM_ROLE == 0
    secure_clear(&_selected, sizeof(_selected));
    secure_clear(_member_nonce, sizeof(_member_nonce));
#endif
}

void TeamJoinService::receiveDiscover(const uint8_t *data, size_t length)
{
#if HELTEC_TEAM_ROLE == 1
    TEAM_LOG("discover rx begin data=%p length=%u window=%u mesh=%p team=%p",
             static_cast<const void *>(data), static_cast<unsigned>(length),
             joinWindowOpen() ? 1U : 0U, static_cast<void *>(_mesh),
             static_cast<void *>(_team));
    if (!_mesh || !_team || !data || !joinWindowOpen() || length != 6)
        return;
    const uint32_t now = millis();
    if (_discover_rate_window_ms == 0 ||
        now - _discover_rate_window_ms >= kDiscoverRateWindowMs)
    {
        _discover_rate_window_ms = now;
        _discover_rate_count = 0;
    }
    if ((_last_discover_rx_ms != 0 && now - _last_discover_rx_ms <
         kDiscoverMinIntervalMs) || _discover_rate_count >= kDiscoverRateLimit)
    {
        increment(_stats.rate_limited);
        return;
    }
    _last_discover_rx_ms = now;
    ++_discover_rate_count;
    increment(_stats.discover_rx);
#if HELTEC_TEAM_LOG && defined(ESP32)
    TEAM_LOG("discover rx heap=%lu stack_hwm=%lu",
             static_cast<unsigned long>(ESP.getFreeHeap()),
             static_cast<unsigned long>(uxTaskGetStackHighWaterMark(nullptr)));
#endif
    const uint32_t request_id = get_u32(data + 2);
    TEAM_LOG("discover request=%08lX", static_cast<unsigned long>(request_id));
    Session *session = findSessionByRequest(request_id);
    if (!session)
    {
        session = allocateSession();
        if (!session)
        {
            TEAM_INFO("discover dropped: no session");
            return;
        }
        session->request_id = request_id;
        _mesh->getRNG()->random(reinterpret_cast<uint8_t *>(&session->session_id),
                                sizeof(session->session_id));
        if (session->session_id == 0)
            session->session_id = 1; // Zero is the UI's no-pending-session sentinel.
        _mesh->getRNG()->random(session->leader_nonce, sizeof(session->leader_nonce));
        session->expires_ms = millis() + kSessionDurationMs;
        TEAM_LOG("discover session allocated id=%08lX", static_cast<unsigned long>(session->session_id));
    }
    session->offer_pending = true;
    TEAM_LOG("discover accepted request=%08lX offer deferred",
             static_cast<unsigned long>(request_id));
#else
    (void)data;
    (void)length;
#endif
}

bool TeamJoinService::sendOffer(Session &session)
{
#if HELTEC_TEAM_ROLE != 1
    (void)session;
    return false;
#else
    if (!_mesh || !_team)
        return false;
    const TeamCredentials &credentials = _team->credentials();
    TEAM_LOG("offer build request=%08lX session=%08lX name_len=%u",
             static_cast<unsigned long>(session.request_id),
             static_cast<unsigned long>(session.session_id),
             static_cast<unsigned>(credentials.name_len));
    TEAM_LOG("offer tx begin frame_cap=%u", 102U);
    if (credentials.name_len > kMaxNameBytes)
    {
        TEAM_INFO("offer rejected invalid name length=%u",
                 static_cast<unsigned>(credentials.name_len));
        return false;
    }
    uint8_t frame[102] = {};
    size_t offset = 0;
    frame[offset++] = kProtocolVersion;
    frame[offset++] = static_cast<uint8_t>(Subtype::JoinOffer);
    put_u32(frame + offset, session.request_id);
    offset += 4;
    put_u64(frame + offset, session.session_id);
    offset += 8;
    put_u64(frame + offset, credentials.team_id);
    offset += 8;
    memcpy(frame + offset, _mesh->self_id.pub_key, kPublicKeySize);
    offset += kPublicKeySize;
    memcpy(frame + offset, session.leader_nonce, kNonceSize);
    offset += kNonceSize;
    frame[offset++] = credentials.name_len;
    memcpy(frame + offset, credentials.name, credentials.name_len);
    offset += credentials.name_len;
    const bool sent = sendJoin(frame, offset, _mesh->getRNG()->nextInt(0, 2001));
    TEAM_LOG("offer tx end bytes=%u sent=%u", static_cast<unsigned>(offset), sent ? 1U : 0U);
    if (sent)
        increment(_stats.offer_tx);
    return sent;
#endif
}

void TeamJoinService::receiveOffer(const uint8_t *data, size_t length)
{
#if HELTEC_TEAM_ROLE == 0
    constexpr size_t kFixed = 2 + 4 + 8 + 8 + kPublicKeySize + kNonceSize + 1;
    if ((_state != JoinState::Searching && _state != JoinState::OffersAvailable) ||
        length < kFixed || get_u32(data + 2) != _request_id)
    {
        return;
    }
    const uint8_t name_len = data[kFixed - 1];
    if (name_len == 0 || name_len > kMaxNameBytes || length != kFixed + name_len ||
        !valid_utf8(data + kFixed, name_len))
    {
        return;
    }
    const uint8_t *leader_key = data + 22;
    for (uint8_t i = 0; i < _offer_count; ++i)
    {
        if (memcmp(_offers[i].leader_public_key, leader_key, kPublicKeySize) == 0)
            return;
    }
    if (_offer_count >= HELTEC_TEAM_JOIN_MAX_SESSIONS)
    {
        TEAM_INFO("offer dropped: table full");
        return;
    }
    TeamOffer &offer = _offers[_offer_count++];
    offer.session_id = get_u64(data + 6);
    offer.team_id = get_u64(data + 14);
    memcpy(offer.leader_public_key, leader_key, kPublicKeySize);
    memcpy(offer.leader_nonce, data + 54, kNonceSize);
    offer.name_len = name_len;
    memcpy(offer.name, data + kFixed, name_len);
    offer.name[name_len] = '\0';
    _state = JoinState::OffersAvailable;
    TEAM_INFO("offer accepted index=%u team=%08lX leader=%02X%02X%02X",
             static_cast<unsigned int>(_offer_count - 1),
             static_cast<unsigned long>(offer.team_id), offer.leader_public_key[0],
             offer.leader_public_key[1], offer.leader_public_key[2]);
#else
    (void)data;
    (void)length;
#endif
}

size_t TeamJoinService::offerCount() const
{
#if HELTEC_TEAM_ROLE == 0
    return _offer_count;
#else
    return 0;
#endif
}

bool TeamJoinService::offerAt(size_t index, TeamOffer &offer) const
{
#if HELTEC_TEAM_ROLE == 0
    if (index >= _offer_count)
        return false;
    offer = _offers[index];
    return true;
#else
    (void)index;
    (void)offer;
    return false;
#endif
}

bool TeamJoinService::selectOffer(size_t index)
{
#if HELTEC_TEAM_ROLE == 1
    (void)index;
    return false;
#else
    if (index >= _offer_count)
        return false;
    _selected = _offers[index];
    _mesh->getRNG()->random(_member_nonce, sizeof(_member_nonce));
    uint8_t decision_key[32] = {};
    uint8_t ack_key[32] = {};
    uint8_t transcript[32] = {};
    const bool valid = deriveKeys(_selected.leader_public_key, _selected.session_id,
                                  _selected.team_id, _selected.leader_nonce, _member_nonce,
                                  decision_key, ack_key, transcript, _pairing_code);
    secure_clear(decision_key, sizeof(decision_key));
    secure_clear(ack_key, sizeof(ack_key));
    secure_clear(transcript, sizeof(transcript));
    if (!valid || !sendRequest())
    {
        _state = JoinState::Error;
        clearMemberEphemeral();
        TEAM_INFO("offer selection failed index=%u", static_cast<unsigned int>(index));
        return false;
    }
    _state = JoinState::WaitingForApproval;
    _decision_end_ms = millis() + kSessionDurationMs;
    TEAM_INFO("request sent index=%u sas_ready=1", static_cast<unsigned int>(index));
    return true;
#endif
}

bool TeamJoinService::sendRequest()
{
#if HELTEC_TEAM_ROLE == 1
    return false;
#else
    size_t name_len = strnlen(_mesh->getNodeName(), kMaxNameBytes);
    while (name_len > 0 && !valid_utf8(
               reinterpret_cast<const uint8_t *>(_mesh->getNodeName()), name_len))
        --name_len;
    if (name_len == 0)
        return false;
    uint8_t frame[91] = {};
    size_t offset = 0;
    frame[offset++] = kProtocolVersion;
    frame[offset++] = static_cast<uint8_t>(Subtype::JoinRequest);
    put_u64(frame + offset, _selected.session_id);
    offset += 8;
    memcpy(frame + offset, _mesh->self_id.pub_key, kPublicKeySize);
    offset += kPublicKeySize;
    memcpy(frame + offset, _member_nonce, kNonceSize);
    offset += kNonceSize;
    frame[offset++] = static_cast<uint8_t>(name_len);
    memcpy(frame + offset, _mesh->getNodeName(), name_len);
    offset += name_len;
    return sendJoin(frame, offset);
#endif
}

void TeamJoinService::receiveRequest(const uint8_t *data, size_t length)
{
#if HELTEC_TEAM_ROLE == 1
    constexpr size_t kFixed = 2 + 8 + kPublicKeySize + kNonceSize + 1;
    if (!joinWindowOpen() || length < kFixed)
        return;
    Session *session = findSession(get_u64(data + 2));
    const uint8_t name_len = data[kFixed - 1];
    if (!session || static_cast<int32_t>(millis() - session->expires_ms) >= 0 ||
        name_len == 0 || name_len > kMaxNameBytes ||
        length != kFixed + name_len || !valid_utf8(data + kFixed, name_len))
    {
        return;
    }
    if (session->request_received)
        return;
    memcpy(session->member_public_key, data + 10, kPublicKeySize);
    memcpy(session->member_nonce, data + 42, kNonceSize);
    session->member_name_len = name_len;
    memcpy(session->member_name, data + kFixed, name_len);
    session->member_name[name_len] = '\0';
    uint8_t decision_key[32] = {};
    uint8_t ack_key[32] = {};
    uint8_t transcript[32] = {};
    if (!deriveKeys(session->member_public_key, session->session_id,
                    _team->credentials().team_id, session->leader_nonce,
                    session->member_nonce, decision_key, ack_key, transcript,
                    session->pairing_code))
    {
        TEAM_INFO("request rejected: key derivation failed member=%02X%02X%02X",
                 session->member_public_key[0], session->member_public_key[1],
                 session->member_public_key[2]);
        clearSession(*session);
        return;
    }
    secure_clear(decision_key, sizeof(decision_key));
    secure_clear(ack_key, sizeof(ack_key));
    secure_clear(transcript, sizeof(transcript));
    session->request_received = true;
    increment(_stats.request_rx);
    session->expires_ms = millis() + kSessionDurationMs;
    _pairing_code = session->pairing_code;
    _state = JoinState::PendingApproval;
    TEAM_INFO("request pending member=%02X%02X%02X sas_ready=1",
             session->member_public_key[0], session->member_public_key[1],
             session->member_public_key[2]);
#else
    (void)data;
    (void)length;
#endif
}

bool TeamJoinService::pendingJoin(PendingJoin &pending) const
{
    pending = PendingJoin{};
#if HELTEC_TEAM_ROLE == 1
    if (!joinWindowOpen())
        return false;
    for (const Session &session : _sessions)
    {
        if (!session.occupied || !session.request_received || session.decision_sent ||
            static_cast<int32_t>(millis() - session.expires_ms) >= 0)
            continue;
        pending.valid = true;
        pending.session_id = session.session_id;
        pending.pairing_code = session.pairing_code;
        pending.name_len = session.member_name_len;
        memcpy(pending.name, session.member_name, session.member_name_len + 1);
        memcpy(pending.member_public_key, session.member_public_key, kPublicKeySize);
        return true;
    }
#endif
    return false;
}

bool TeamJoinService::approvePending(uint64_t session_id)
{
#if HELTEC_TEAM_ROLE == 1
    Session *session = findSession(session_id);
    if (!joinWindowOpen() || !session || !session->request_received ||
        session->decision_sent || static_cast<int32_t>(millis() - session->expires_ms) >= 0)
        return false;
    if (!sendDecision(*session, true))
    {
        TEAM_INFO("approval failed");
        return false;
    }
    session->decision_sent = true;
    session->approved = true;
    increment(_stats.approved);
    _state = JoinState::SendingCredentials;
    TEAM_INFO("approval sent member=%02X%02X%02X", session->member_public_key[0],
             session->member_public_key[1], session->member_public_key[2]);
    return true;
#else
    (void)session_id;
    return false;
#endif
}

bool TeamJoinService::rejectPending(uint64_t session_id)
{
#if HELTEC_TEAM_ROLE == 1
    Session *session = findSession(session_id);
    if (!joinWindowOpen() || !session || !session->request_received ||
        session->decision_sent || static_cast<int32_t>(millis() - session->expires_ms) >= 0)
        return false;
    const bool sent = sendDecision(*session, false);
    TEAM_INFO("rejection sent member=%02X%02X%02X queued=%u", session->member_public_key[0],
             session->member_public_key[1], session->member_public_key[2], sent);
    clearSession(*session);
    _state = JoinState::Rejected;
    increment(_stats.rejected);
    return sent;
#else
    (void)session_id;
    return false;
#endif
}

bool TeamJoinService::deriveKeys(const uint8_t peer_public_key[kPublicKeySize],
                                 uint64_t session_id, uint64_t team_id,
                                 const uint8_t leader_nonce[kNonceSize],
                                 const uint8_t member_nonce[kNonceSize],
                                 uint8_t decision_key[32], uint8_t ack_key[32],
                                 uint8_t transcript_hash[32], uint16_t &pairing_code) const
{
    if (!_mesh)
        return false;
    uint8_t shared[32] = {};
    _mesh->self_id.calcSharedSecret(shared, peer_public_key);
    bool any = false;
    for (uint8_t value : shared)
        any = any || value != 0;
    if (!any)
        return false;

    SHA256 hash;
    const uint8_t version = kProtocolVersion;
    hash.update(&version, 1);
    uint8_t ids[16] = {};
    put_u64(ids, session_id);
    put_u64(ids + 8, team_id);
    hash.update(ids, sizeof(ids));
#if HELTEC_TEAM_ROLE == 1
    hash.update(_mesh->self_id.pub_key, kPublicKeySize);
    hash.update(peer_public_key, kPublicKeySize);
#else
    hash.update(peer_public_key, kPublicKeySize);
    hash.update(_mesh->self_id.pub_key, kPublicKeySize);
#endif
    hash.update(leader_nonce, kNonceSize);
    hash.update(member_nonce, kNonceSize);
    hash.finalize(transcript_hash, 32);

    static const char kDecisionInfo[] = "Heltec Team Leader to Member v1";
    static const char kAckInfo[] = "Heltec Team Member to Leader v1";
    static const char kPairInfo[] = "Heltec Team Pairing Code v1";
    hkdf<SHA256>(decision_key, 32, shared, sizeof(shared), transcript_hash, 32,
                 kDecisionInfo, sizeof(kDecisionInfo) - 1);
    hkdf<SHA256>(ack_key, 32, shared, sizeof(shared), transcript_hash, 32,
                 kAckInfo, sizeof(kAckInfo) - 1);
    uint8_t pair_key[32] = {};
    uint8_t pair_mac[32] = {};
    hkdf<SHA256>(pair_key, sizeof(pair_key), shared, sizeof(shared), transcript_hash, 32,
                 kPairInfo, sizeof(kPairInfo) - 1);
    hash.resetHMAC(pair_key, sizeof(pair_key));
    hash.update(transcript_hash, 32);
    hash.finalizeHMAC(pair_key, sizeof(pair_key), pair_mac, sizeof(pair_mac));
    pairing_code = static_cast<uint16_t>(get_u32(pair_mac) % 10000UL);
    secure_clear(shared, sizeof(shared));
    secure_clear(pair_key, sizeof(pair_key));
    secure_clear(pair_mac, sizeof(pair_mac));
    return true;
}

bool TeamJoinService::sendDecision(Session &session, bool approved)
{
#if HELTEC_TEAM_ROLE != 1
    (void)session;
    (void)approved;
    return false;
#else
    uint8_t decision_key[32] = {};
    uint8_t ack_key[32] = {};
    uint8_t transcript[32] = {};
    uint16_t code = 0;
    if (!deriveKeys(session.member_public_key, session.session_id,
                    _team->credentials().team_id, session.leader_nonce,
                    session.member_nonce, decision_key, ack_key, transcript, code))
    {
        return false;
    }
    uint8_t plain[109] = {};
    size_t plain_len = 1;
    plain[0] = approved ? kDecisionApprove : kDecisionReject;
    if (approved)
    {
        const TeamCredentials &credentials = _team->credentials();
        put_u64(plain + plain_len, credentials.team_id);
        plain_len += 8;
        put_u32(plain + plain_len, credentials.key_version);
        plain_len += 4;
        memcpy(plain + plain_len, credentials.secret, 32);
        plain_len += 32;
        plain[plain_len++] = credentials.name_len;
        memcpy(plain + plain_len, credentials.name, credentials.name_len);
        plain_len += credentials.name_len;
        memcpy(plain + plain_len, _mesh->self_id.pub_key, kPublicKeySize);
        plain_len += kPublicKeySize;
    }

    uint8_t frame[148] = {};
    size_t offset = 0;
    frame[offset++] = kProtocolVersion;
    frame[offset++] = static_cast<uint8_t>(Subtype::JoinDecision);
    put_u64(frame + offset, session.session_id);
    offset += 8;
    uint8_t *nonce = frame + offset;
    _mesh->getRNG()->random(nonce, kAeadNonceSize);
    offset += kAeadNonceSize;
    frame[offset++] = static_cast<uint8_t>(plain_len);
    uint8_t aad[34] = {};
    make_aad(transcript, Subtype::JoinDecision, 0, aad);
    const bool encrypted = aead_encrypt(decision_key, nonce, aad, plain, plain_len,
                                        frame + offset, frame + offset + plain_len);
    offset += plain_len + kAeadTagSize;
    secure_clear(decision_key, sizeof(decision_key));
    secure_clear(ack_key, sizeof(ack_key));
    secure_clear(transcript, sizeof(transcript));
    secure_clear(plain, sizeof(plain));
    return encrypted && sendJoin(frame, offset);
#endif
}

void TeamJoinService::receiveDecision(const uint8_t *data, size_t length)
{
#if HELTEC_TEAM_ROLE == 0
    constexpr size_t kFixed = 2 + 8 + kAeadNonceSize + 1 + kAeadTagSize;
    if (_state != JoinState::WaitingForApproval || length < kFixed ||
        get_u64(data + 2) != _selected.session_id)
    {
        return;
    }
    const uint8_t cipher_len = data[2 + 8 + kAeadNonceSize];
    if (cipher_len == 0 || length != kFixed + cipher_len)
        return;
    uint8_t decision_key[32] = {};
    uint8_t ack_key[32] = {};
    uint8_t transcript[32] = {};
    uint16_t code = 0;
    if (!deriveKeys(_selected.leader_public_key, _selected.session_id, _selected.team_id,
                    _selected.leader_nonce, _member_nonce, decision_key, ack_key,
                    transcript, code))
    {
        _state = JoinState::Error;
        clearMemberEphemeral();
        return;
    }
    uint8_t aad[34] = {};
    make_aad(transcript, Subtype::JoinDecision, 0, aad);
    const uint8_t *cipher = data + 2 + 8 + kAeadNonceSize + 1;
    const uint8_t *tag = cipher + cipher_len;
    uint8_t plain[109] = {};
    const bool decrypted = aead_decrypt(decision_key, data + 10, aad, cipher, cipher_len,
                                        tag, plain);
    secure_clear(decision_key, sizeof(decision_key));
    secure_clear(ack_key, sizeof(ack_key));
    secure_clear(transcript, sizeof(transcript));
    if (!decrypted)
    {
        increment(_stats.aead_error);
        _state = JoinState::Error;
        clearMemberEphemeral();
        TEAM_INFO("decision rejected: authentication failed");
        return;
    }
    if (plain[0] == kDecisionReject)
    {
        secure_clear(plain, sizeof(plain));
        clearMemberEphemeral();
        _state = JoinState::Rejected;
        increment(_stats.rejected);
        TEAM_INFO("decision received: rejected");
        return;
    }
    if (plain[0] != kDecisionApprove || cipher_len < 1 + 8 + 4 + 32 + 1 + 32)
    {
        secure_clear(plain, sizeof(plain));
        _state = JoinState::Error;
        clearMemberEphemeral();
        return;
    }
    TeamCredentials credentials{};
    size_t offset = 1;
    credentials.team_id = get_u64(plain + offset);
    offset += 8;
    credentials.key_version = get_u32(plain + offset);
    offset += 4;
    memcpy(credentials.secret, plain + offset, 32);
    offset += 32;
    credentials.name_len = plain[offset++];
    if (credentials.name_len == 0 || credentials.name_len > kMaxNameBytes ||
        cipher_len != 1 + 8 + 4 + 32 + 1 + credentials.name_len + 32)
    {
        secure_clear(plain, sizeof(plain));
        _state = JoinState::Error;
        clearMemberEphemeral();
        return;
    }
    memcpy(credentials.name, plain + offset, credentials.name_len);
    offset += credentials.name_len;
    memcpy(credentials.leader_public_key, plain + offset, kPublicKeySize);
    const bool valid = credentials.team_id == _selected.team_id &&
                       memcmp(credentials.leader_public_key, _selected.leader_public_key,
                              kPublicKeySize) == 0 &&
                       _team->installTeam(credentials);
    secure_clear(plain, sizeof(plain));
    if (!valid)
    {
        secure_clear(&credentials, sizeof(credentials));
        _state = JoinState::Error;
        clearMemberEphemeral();
        TEAM_INFO("decision rejected: credentials invalid");
        return;
    }
    (void)sendAck(credentials.key_version);
    secure_clear(&credentials, sizeof(credentials));
    clearMemberEphemeral();
    _state = JoinState::Joined;
    TEAM_INFO("decision accepted: joined ack attempted; status/sync scheduled");
#else
    (void)data;
    (void)length;
#endif
}

bool TeamJoinService::sendAck(uint32_t key_version)
{
#if HELTEC_TEAM_ROLE == 1
    (void)key_version;
    return false;
#else
    uint8_t decision_key[32] = {};
    uint8_t ack_key[32] = {};
    uint8_t transcript[32] = {};
    uint16_t code = 0;
    if (!deriveKeys(_selected.leader_public_key, _selected.session_id, _selected.team_id,
                    _selected.leader_nonce, _member_nonce, decision_key, ack_key,
                    transcript, code))
    {
        return false;
    }
    uint8_t plain[5] = { kAckOk };
    put_u32(plain + 1, key_version);
    uint8_t frame[44] = {};
    frame[0] = kProtocolVersion;
    frame[1] = static_cast<uint8_t>(Subtype::JoinAck);
    put_u64(frame + 2, _selected.session_id);
    _mesh->getRNG()->random(frame + 10, kAeadNonceSize);
    frame[22] = sizeof(plain);
    uint8_t aad[34] = {};
    make_aad(transcript, Subtype::JoinAck, 1, aad);
    const bool encrypted = aead_encrypt(ack_key, frame + 10, aad, plain, sizeof(plain),
                                        frame + 23, frame + 23 + sizeof(plain));
    secure_clear(decision_key, sizeof(decision_key));
    secure_clear(ack_key, sizeof(ack_key));
    secure_clear(transcript, sizeof(transcript));
    return encrypted && sendJoin(frame, 23 + sizeof(plain) + kAeadTagSize);
#endif
}

void TeamJoinService::receiveAck(const uint8_t *data, size_t length)
{
#if HELTEC_TEAM_ROLE == 1
    constexpr size_t kExpected = 2 + 8 + kAeadNonceSize + 1 + 5 + kAeadTagSize;
    if (length != kExpected || data[22] != 5)
        return;
    Session *session = findSession(get_u64(data + 2));
    if (!session || !session->request_received || !session->decision_sent ||
        !session->approved)
        return;
    uint8_t decision_key[32] = {};
    uint8_t ack_key[32] = {};
    uint8_t transcript[32] = {};
    uint16_t code = 0;
    if (!deriveKeys(session->member_public_key, session->session_id,
                    _team->credentials().team_id, session->leader_nonce,
                    session->member_nonce, decision_key, ack_key, transcript, code))
    {
        return;
    }
    uint8_t aad[34] = {};
    uint8_t plain[5] = {};
    make_aad(transcript, Subtype::JoinAck, 1, aad);
    const bool valid = aead_decrypt(ack_key, data + 10, aad, data + 23, 5,
                                    data + 28, plain) &&
                       plain[0] == kAckOk &&
                       get_u32(plain + 1) == _team->credentials().key_version;
    secure_clear(decision_key, sizeof(decision_key));
    secure_clear(ack_key, sizeof(ack_key));
    secure_clear(transcript, sizeof(transcript));
    secure_clear(plain, sizeof(plain));
    if (valid)
    {
        TEAM_LOG("ack authenticated member=%02X%02X%02X", session->member_public_key[0],
                 session->member_public_key[1], session->member_public_key[2]);
        clearSession(*session);
        _state = JoinState::Joined;
    }
    else
    {
        increment(_stats.aead_error);
        TEAM_INFO("ack rejected: authentication/version failed");
    }
#else
    (void)data;
    (void)length;
#endif
}

void TeamJoinService::cancel()
{
    _search_end_ms = 0;
    _decision_end_ms = 0;
    _next_discover_ms = 0;
    _request_id = 0;
    _discover_count = 0;
    _discover_rate_window_ms = 0;
    _last_discover_rx_ms = 0;
    _discover_rate_count = 0;
    clearAllSessions();
    _state = JoinState::Idle;
    TEAM_INFO("join operation cancelled");
}

} // namespace heltec::meshcore::team

#endif
