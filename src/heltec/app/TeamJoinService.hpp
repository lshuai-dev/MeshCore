/**
 * @file    TeamJoinService.hpp
 * @brief   Fixed-memory, user-confirmed Team pairing service.
 * @version 1.0
 * @date    2026-09-04
 */
#pragma once

#include "app/TeamConfig.hpp"
#include "app/TeamService.hpp"

#include <stddef.h>
#include <stdint.h>

class HeltecMesh;

namespace heltec::meshcore::team
{

enum class JoinState : uint8_t
{
    Idle = 0,
    Searching,
    OffersAvailable,
    WaitingForApproval,
    PendingApproval,
    SendingCredentials,
    Joined,
    Rejected,
    TimedOut,
    Error,
};

struct TeamOffer
{
    uint64_t session_id = 0;
    uint64_t team_id = 0;
    uint8_t leader_public_key[kPublicKeySize] = {};
    uint8_t leader_nonce[kNonceSize] = {};
    uint8_t name_len = 0;
    char name[kMaxNameBytes + 1] = {};
};

struct PendingJoin
{
    bool valid = false;
    uint64_t session_id = 0;
    uint16_t pairing_code = 0;
    uint8_t member_public_key[kPublicKeySize] = {};
    uint8_t name_len = 0;
    char name[kMaxNameBytes + 1] = {};
};

struct TeamJoinStats
{
    uint16_t discover_rx = 0;
    uint16_t offer_tx = 0;
    uint16_t request_rx = 0;
    uint16_t rate_limited = 0;
    uint16_t approved = 0;
    uint16_t rejected = 0;
    uint16_t aead_error = 0;
    uint16_t timeout = 0;
};

class TeamJoinService
{
  public:
    void begin(HeltecMesh &mesh, TeamService &team);
    void poll();
    void handleJoinData(const uint8_t *data, size_t length);

    bool openJoinWindow();
    void closeJoinWindow();
    bool joinWindowOpen() const;

    bool discoverTeams();
    size_t offerCount() const;
    bool offerAt(size_t index, TeamOffer &offer) const;
    bool selectOffer(size_t index);
    void cancel();

    bool pendingJoin(PendingJoin &pending) const;
    bool approvePending(uint64_t session_id);
    bool rejectPending(uint64_t session_id);
    JoinState state() const;
    uint16_t pairingCode() const;
    const TeamJoinStats &stats() const { return _stats; }
    void resetStats() { _stats = TeamJoinStats{}; }

  private:
    struct Session
    {
        bool occupied = false;
        bool request_received = false;
        bool decision_sent = false;
        bool approved = false;
        bool offer_pending = false;
        uint32_t request_id = 0;
        uint64_t session_id = 0;
        uint32_t expires_ms = 0;
        uint8_t leader_nonce[kNonceSize] = {};
        uint8_t member_nonce[kNonceSize] = {};
        uint8_t member_public_key[kPublicKeySize] = {};
        uint8_t member_name_len = 0;
        char member_name[kMaxNameBytes + 1] = {};
        uint16_t pairing_code = 0;
    };

    bool sendJoin(const uint8_t *data, size_t length, uint32_t delay_ms = 0);
    bool sendDiscover();
    bool sendOffer(Session &session);
    bool sendRequest();
    bool sendDecision(Session &session, bool approved);
    bool sendAck(uint32_t key_version);
    void receiveDiscover(const uint8_t *data, size_t length);
    void receiveOffer(const uint8_t *data, size_t length);
    void receiveRequest(const uint8_t *data, size_t length);
    void receiveDecision(const uint8_t *data, size_t length);
    void receiveAck(const uint8_t *data, size_t length);
    Session *findSession(uint64_t session_id);
    Session *findSessionByRequest(uint32_t request_id);
    Session *allocateSession();
    void clearSession(Session &session);
    void clearAllSessions();
    void clearMemberEphemeral();
    bool deriveKeys(const uint8_t peer_public_key[kPublicKeySize], uint64_t session_id,
                    uint64_t team_id, const uint8_t leader_nonce[kNonceSize],
                    const uint8_t member_nonce[kNonceSize], uint8_t decision_key[32],
                    uint8_t ack_key[32], uint8_t transcript_hash[32],
                    uint16_t &pairing_code) const;

    HeltecMesh *_mesh = nullptr;
    TeamService *_team = nullptr;
    JoinState _state = JoinState::Idle;
    bool _join_window_open = false;
    uint32_t _search_end_ms = 0;
    uint32_t _decision_end_ms = 0;
    uint32_t _next_discover_ms = 0;
    uint32_t _request_id = 0;
    uint8_t _discover_count = 0;
    uint16_t _pairing_code = 0;
    uint32_t _discover_rate_window_ms = 0;
    uint32_t _last_discover_rx_ms = 0;
    uint8_t _discover_rate_count = 0;
    TeamJoinStats _stats{};

    Session _sessions[HELTEC_TEAM_JOIN_MAX_SESSIONS] = {};
    TeamOffer _offers[HELTEC_TEAM_JOIN_MAX_SESSIONS] = {};
    uint8_t _offer_count = 0;
    TeamOffer _selected{};
    uint8_t _member_nonce[kNonceSize] = {};
};

} // namespace heltec::meshcore::team
