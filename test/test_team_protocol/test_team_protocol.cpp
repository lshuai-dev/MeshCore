#include <gtest/gtest.h>

#include "heltec/app/team_protocol.hpp"

#include <cstring>

using namespace heltec::meshcore::team;

namespace
{
CommonHeader header()
{
    CommonHeader value{};
    value.role = 1;
    value.boot_id = 42;
    value.sequence = 65535;
    value.timestamp = 123456;
    const uint8_t id[kSenderIdSize] = { 1, 2, 3, 4, 5, 6 };
    memcpy(value.sender_id, id, sizeof(id));
    return value;
}
} // namespace

TEST(TeamProtocol, PresenceRoundTrip)
{
    PresenceFrame input{};
    input.header = header();
    input.device_type = 2;
    input.capabilities = 9;
    input.battery_mv = 4012;
    input.name_len = 6;
    memcpy(input.name, "leader", input.name_len);
    for (size_t i = 0; i < sizeof(input.public_key); ++i)
        input.public_key[i] = static_cast<uint8_t>(i);
    uint8_t bytes[96] = {};
    size_t length = 0;
    ASSERT_TRUE(encode_presence(input, bytes, sizeof(bytes), length));
    PresenceFrame output{};
    ASSERT_EQ(DecodeResult::Ok, decode_presence(bytes, length, output));
    EXPECT_EQ(input.header.sequence, output.header.sequence);
    EXPECT_EQ(input.battery_mv, output.battery_mv);
    EXPECT_STREQ("leader", output.name);
    EXPECT_EQ(0, memcmp(input.public_key, output.public_key, kPublicKeySize));
}

TEST(TeamProtocol, LocationRoundTripAndValidation)
{
    LocationFrame input{};
    input.header = header();
    input.header.flags = 0x10;
    input.latitude_e6 = 31230400;
    input.longitude_e6 = 121473700;
    input.course_cdeg = 35999;
    uint8_t bytes[64] = {};
    size_t length = 0;
    ASSERT_TRUE(encode_location(input, bytes, sizeof(bytes), length));
    LocationFrame output{};
    EXPECT_EQ(DecodeResult::Ok, decode_location(bytes, length, output));
    EXPECT_EQ(input.latitude_e6, output.latitude_e6);
    EXPECT_EQ(DecodeResult::InvalidLength, decode_location(bytes, length - 1, output));
    EXPECT_EQ(DecodeResult::InvalidLength, decode_location(bytes, length + 1, output));

    input.latitude_e6 = 90000001;
    EXPECT_FALSE(encode_location(input, bytes, sizeof(bytes), length));
    input.latitude_e6 = 0;
    input.longitude_e6 = 0;
    input.course_cdeg = 36000;
    EXPECT_FALSE(encode_location(input, bytes, sizeof(bytes), length));
}

TEST(TeamProtocol, SyncRoundTrip)
{
    SyncRequestFrame input{};
    input.header = header();
    input.request_id = 77;
    input.response_mask = 3;
    input.response_window_sec = 15;
    uint8_t bytes[32] = {};
    size_t length = 0;
    ASSERT_TRUE(encode_sync_request(input, bytes, sizeof(bytes), length));
    SyncRequestFrame output{};
    ASSERT_EQ(DecodeResult::Ok, decode_sync_request(bytes, length, output));
    EXPECT_EQ(77, output.request_id);
}

TEST(TeamProtocol, RejectsVersionNameAndUtf8Errors)
{
    PresenceFrame input{};
    input.header = header();
    input.name_len = 1;
    input.name[0] = 'A';
    uint8_t bytes[96] = {};
    size_t length = 0;
    ASSERT_TRUE(encode_presence(input, bytes, sizeof(bytes), length));
    PresenceFrame output{};
    bytes[0] = 2;
    EXPECT_EQ(DecodeResult::UnsupportedVersion, decode_presence(bytes, length, output));
    EXPECT_FALSE(valid_utf8(reinterpret_cast<const uint8_t *>("A\0B"), 3));
    const uint8_t truncated_utf8[] = { 0xE4, 0xB8 };
    EXPECT_FALSE(valid_utf8(truncated_utf8, sizeof(truncated_utf8)));
    input.name_len = static_cast<uint8_t>(kMaxNameBytes + 1);
    EXPECT_FALSE(encode_presence(input, bytes, sizeof(bytes), length));
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
            bytes[i] = static_cast<uint8_t>((i * 37U + length) & 0xFFU);
        PresenceFrame presence{};
        LocationFrame location{};
        SyncRequestFrame sync{};
        (void)decode_presence(bytes, length, presence);
        (void)decode_location(bytes, length, location);
        (void)decode_sync_request(bytes, length, sync);
    }
}
