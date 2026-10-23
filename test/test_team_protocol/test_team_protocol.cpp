#include <gtest/gtest.h>

#include "heltec/app/TeamProtocol.hpp"

#include <cstring>

using namespace Heltec::MeshCore::Team;

namespace
{
StatusFrame status()
{
    StatusFrame value{};
    value.header.role = 1;
    value.header.boot_id = 42;
    value.header.sequence = 65535;
    value.header.flags = kStatusBatteryValid | kStatusLocationValid;
    value.battery_mv = 4012;
    value.latitude_e6 = -31230400;
    value.longitude_e6 = 121473700;
    value.fix_age_sec = 12;
    value.name_len = 6;
    memcpy(value.name, "leader", value.name_len);
    for (size_t i = 0; i < sizeof(value.public_key); ++i)
    {
        value.public_key[i] = static_cast<uint8_t>(i + 1);
    }
    return value;
}
} // namespace

TEST(TeamProtocol, StatusRoundTrip)
{
    const StatusFrame input = status();
    uint8_t bytes[kStatusMaxSize] = {};
    size_t length = 0;
    ASSERT_TRUE(encode_status(input, bytes, sizeof(bytes), length));
    EXPECT_EQ(kStatusBaseSize + input.name_len, length);
    StatusFrame output{};
    ASSERT_EQ(DecodeResult::Ok, decode_status(bytes, length, output));
    EXPECT_EQ(input.header.sequence, output.header.sequence);
    EXPECT_EQ(input.header.boot_id, output.header.boot_id);
    EXPECT_EQ(input.header.role, output.header.role);
    EXPECT_EQ(input.header.flags, output.header.flags);
    EXPECT_EQ(input.battery_mv, output.battery_mv);
    EXPECT_EQ(input.latitude_e6, output.latitude_e6);
    EXPECT_EQ(input.longitude_e6, output.longitude_e6);
    EXPECT_EQ(input.fix_age_sec, output.fix_age_sec);
    EXPECT_STREQ("leader", output.name);
    EXPECT_EQ(0, memcmp(input.public_key, output.public_key, kPublicKeySize));
    EXPECT_EQ(0, memcmp(input.public_key, output.header.sender_id, kSenderIdSize));
}

TEST(TeamProtocol, StatusLengthAndFieldValidation)
{
    StatusFrame input = status();
    uint8_t bytes[kStatusMaxSize + 1] = {};
    size_t length = 0;
    ASSERT_TRUE(encode_status(input, bytes, sizeof(bytes), length));
    StatusFrame output{};
    for (size_t size = 0; size < length; ++size)
    {
        EXPECT_NE(DecodeResult::Ok, decode_status(bytes, size, output));
    }
    EXPECT_EQ(DecodeResult::InvalidLength, decode_status(bytes, length + 1, output));
    size_t short_length = 123;
    EXPECT_FALSE(encode_status(input, bytes, length - 1, short_length));
    EXPECT_EQ(0U, short_length);
    EXPECT_FALSE(encode_status(input, nullptr, sizeof(bytes), short_length));
    EXPECT_EQ(DecodeResult::InvalidLength, decode_status(nullptr, length, output));
    bytes[2] |= 0x80;
    EXPECT_EQ(DecodeResult::InvalidField, decode_status(bytes, length, output));
    input.latitude_e6 = 90000001;
    EXPECT_FALSE(encode_status(input, bytes, sizeof(bytes), length));
    input = status();
    input.longitude_e6 = -180000001;
    EXPECT_FALSE(encode_status(input, bytes, sizeof(bytes), length));
    input = status();
    input.header.role = 2;
    EXPECT_FALSE(encode_status(input, bytes, sizeof(bytes), length));
}

TEST(TeamProtocol, StatusWithoutGpsStillCarriesIdentity)
{
    StatusFrame input = status();
    input.header.flags = kStatusBatteryValid;
    uint8_t bytes[kStatusMaxSize] = {};
    size_t length = 0;
    EXPECT_FALSE(encode_status(input, bytes, sizeof(bytes), length));
    input.latitude_e6 = 0;
    input.longitude_e6 = 0;
    input.fix_age_sec = 0;
    ASSERT_TRUE(encode_status(input, bytes, sizeof(bytes), length));
    StatusFrame output{};
    ASSERT_EQ(DecodeResult::Ok, decode_status(bytes, length, output));
    EXPECT_EQ(0, output.header.flags & kStatusLocationValid);
    EXPECT_STREQ("leader", output.name);
    input.header.flags = 0;
    EXPECT_FALSE(encode_status(input, bytes, sizeof(bytes), length));
    input.battery_mv = 0;
    EXPECT_TRUE(encode_status(input, bytes, sizeof(bytes), length));
}

TEST(TeamProtocol, SyncRoundTripAndWindowLimits)
{
    SyncRequestFrame input{};
    input.header = status().header;
    memcpy(input.header.sender_id, status().public_key, kSenderIdSize);
    input.request_id = 77;
    input.response_window_sec = 15;
    uint8_t bytes[32] = {};
    size_t length = 0;
    ASSERT_TRUE(encode_sync_request(input, bytes, sizeof(bytes), length));
    EXPECT_EQ(kCommonHeaderSize + 3, length);
    SyncRequestFrame output{};
    ASSERT_EQ(DecodeResult::Ok, decode_sync_request(bytes, length, output));
    EXPECT_EQ(77, output.request_id);
    EXPECT_EQ(15, output.response_window_sec);
    EXPECT_NE(DecodeResult::Ok, decode_sync_request(bytes, length - 1, output));
    EXPECT_NE(DecodeResult::Ok, decode_sync_request(bytes, length + 1, output));
    input.response_window_sec = 4;
    EXPECT_FALSE(encode_sync_request(input, bytes, sizeof(bytes), length));
    input.response_window_sec = 31;
    EXPECT_FALSE(encode_sync_request(input, bytes, sizeof(bytes), length));
}

TEST(TeamProtocol, RejectsOldVersionAndInvalidNames)
{
    StatusFrame input = status();
    uint8_t bytes[kStatusMaxSize] = {};
    size_t length = 0;
    ASSERT_TRUE(encode_status(input, bytes, sizeof(bytes), length));
    StatusFrame output{};
    bytes[0] = 1;
    EXPECT_EQ(DecodeResult::UnsupportedVersion, decode_status(bytes, length, output));
    EXPECT_EQ(static_cast<Subtype>(0), frame_subtype(bytes, length));
    EXPECT_FALSE(valid_utf8(reinterpret_cast<const uint8_t *>("A\0B"), 3));
    const uint8_t truncated_utf8[] = { 0xE4, 0xB8 };
    EXPECT_FALSE(valid_utf8(truncated_utf8, sizeof(truncated_utf8)));
    input.name_len = static_cast<uint8_t>(kMaxNameBytes + 1);
    EXPECT_FALSE(encode_status(input, bytes, sizeof(bytes), length));
    input.name_len = kMaxNameBytes;
    memset(input.name, 'X', input.name_len);
    ASSERT_TRUE(encode_status(input, bytes, sizeof(bytes), length));
    EXPECT_EQ(kStatusMaxSize, length);
    ASSERT_EQ(DecodeResult::Ok, decode_status(bytes, length, output));
    EXPECT_EQ('\0', output.name[kMaxNameBytes]);
    bytes[length - 1] = 0;
    EXPECT_EQ(DecodeResult::InvalidUtf8, decode_status(bytes, length, output));
}

TEST(TeamProtocol, SequenceWrapBoundaries)
{
    EXPECT_TRUE(sequence_is_newer(0, 65535));
    EXPECT_FALSE(sequence_is_newer(1, 1));
    EXPECT_TRUE(sequence_is_newer(0x7FFF, 0));
    EXPECT_FALSE(sequence_is_newer(0x8000, 0));
    EXPECT_FALSE(sequence_is_newer(0x8001, 0));
}

TEST(TeamProtocol, ArbitraryInputNeverProducesInvalidSuccess)
{
    uint8_t bytes[166] = {};
    for (size_t length = 0; length <= sizeof(bytes); ++length)
    {
        for (size_t i = 0; i < length; ++i)
        {
            bytes[i] = static_cast<uint8_t>((i * 37U + length) & 0xFFU);
        }
        StatusFrame frame{};
        SyncRequestFrame sync{};
        if (decode_status(bytes, length, frame) == DecodeResult::Ok)
        {
            EXPECT_LE(frame.name_len, kMaxNameBytes);
            EXPECT_LE(frame.header.role, 1);
            EXPECT_GE(frame.latitude_e6, -90000000);
            EXPECT_LE(frame.latitude_e6, 90000000);
        }
        (void)decode_sync_request(bytes, length, sync);
    }
}
