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
constexpr uint8_t kProtocolVersion = 1;
constexpr size_t kPublicKeySize = 32;
constexpr size_t kSenderIdSize = 6;
constexpr size_t kNonceSize = 16;
constexpr size_t kAeadNonceSize = 12;
constexpr size_t kAeadTagSize = 16;
constexpr size_t kMaxNameBytes = 31;
constexpr size_t kCommonHeaderSize = 18;

enum class Subtype : uint8_t
{
    Presence = 0x01,
    Location = 0x02,
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
    uint32_t timestamp = 0;
};

struct PresenceFrame
{
    CommonHeader header{};
    uint8_t device_type = 0;
    uint8_t public_key[kPublicKeySize] = {};
    uint16_t capabilities = 0;
    uint16_t battery_mv = 0;
    uint8_t name_len = 0;
    char name[kMaxNameBytes + 1] = {};
};

struct LocationFrame
{
    CommonHeader header{};
    int32_t latitude_e6 = 0;
    int32_t longitude_e6 = 0;
    int16_t altitude_dm = 0;
    uint16_t accuracy_dm = 0;
    uint16_t speed_cms = 0;
    uint16_t course_cdeg = 0;
    uint16_t fix_age_sec = 0;
    uint8_t satellites = 0;
    uint16_t battery_mv = 0;
};

struct SyncRequestFrame
{
    CommonHeader header{};
    uint16_t request_id = 0;
    uint8_t response_mask = 0;
    uint8_t response_window_sec = 0;
};

bool encode_presence(const PresenceFrame &frame, uint8_t *output, size_t capacity,
                     size_t &output_length);
bool encode_location(const LocationFrame &frame, uint8_t *output, size_t capacity,
                     size_t &output_length);
bool encode_sync_request(const SyncRequestFrame &frame, uint8_t *output, size_t capacity,
                         size_t &output_length);
DecodeResult decode_presence(const uint8_t *data, size_t length, PresenceFrame &frame);
DecodeResult decode_location(const uint8_t *data, size_t length, LocationFrame &frame);
DecodeResult decode_sync_request(const uint8_t *data, size_t length, SyncRequestFrame &frame);
Subtype frame_subtype(const uint8_t *data, size_t length);
bool sequence_is_newer(uint16_t candidate, uint16_t current);
bool valid_utf8(const uint8_t *data, size_t length);

} // namespace heltec::meshcore::team
