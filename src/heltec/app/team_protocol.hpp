/**
 * @file    team_protocol.hpp
 * @brief   Compact wire codec for Heltec Team runtime and join frames.
 * @version 1.0
 * @date    2026-09-04
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

namespace heltec::meshcore::team
{

constexpr uint16_t kDataTypeDev = 0xFF48;
constexpr uint8_t kProtocolVersion = 2;
constexpr size_t kPublicKeySize = 32;
constexpr size_t kSenderIdSize = 6;
constexpr size_t kNonceSize = 16;
constexpr size_t kAeadNonceSize = 12;
constexpr size_t kAeadTagSize = 16;
constexpr size_t kMaxNameBytes = 31;
constexpr size_t kCommonHeaderSize = 14;

enum class Subtype : uint8_t
{
    Status = 0x01,
    SyncRequest = 0x03,
    JoinDiscover = 0x10,
    JoinOffer = 0x11,
    JoinRequest = 0x12,
    JoinDecision = 0x13,
    JoinAck = 0x14,
};

enum class DecodeResult : uint8_t
{
    Ok = 0,
    UnsupportedVersion,
    UnsupportedSubtype,
    InvalidLength,
    InvalidField,
    InvalidUtf8,
};

struct CommonHeader
{
    uint8_t flags = 0;
    uint8_t role = 0;
    uint8_t sender_id[kSenderIdSize] = {};
    uint16_t boot_id = 0;
    uint16_t sequence = 0;
};

// One self-contained status format; name is the only variable-length field.
// Wire flags: bit 0 leader, bit 1 valid battery, bit 4 valid coordinates.
constexpr uint8_t kStatusLeader = 0x01;
constexpr uint8_t kStatusBatteryValid = 0x02;
constexpr uint8_t kStatusLocationValid = 0x10;
constexpr size_t kStatusBaseSize = 52;
constexpr size_t kStatusMaxSize = kStatusBaseSize + kMaxNameBytes;

struct StatusFrame
{
    CommonHeader header{};
    uint8_t public_key[kPublicKeySize] = {};
    uint16_t battery_mv = 0;
    int32_t latitude_e6 = 0;
    int32_t longitude_e6 = 0;
    uint16_t fix_age_sec = 0;
    uint8_t name_len = 0;
    char name[kMaxNameBytes + 1] = {};
};

struct SyncRequestFrame
{
    CommonHeader header{};
    uint16_t request_id = 0;
    uint8_t response_window_sec = 0;
};

/** Encode a self-contained v2 status; false leaves output_length zero. */
bool encode_status(const StatusFrame &frame, uint8_t *output, size_t capacity,
                   size_t &output_length);
bool encode_sync_request(const SyncRequestFrame &frame, uint8_t *output, size_t capacity,
                         size_t &output_length);
/** Decode and validate v2 status; callers must discard frame on any error. */
DecodeResult decode_status(const uint8_t *data, size_t length, StatusFrame &frame);
DecodeResult decode_sync_request(const uint8_t *data, size_t length, SyncRequestFrame &frame);
Subtype frame_subtype(const uint8_t *data, size_t length);
bool sequence_is_newer(uint16_t candidate, uint16_t current);
bool valid_utf8(const uint8_t *data, size_t length);

} // namespace heltec::meshcore::team
