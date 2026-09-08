/**
 * @file    team_protocol.cpp
 * @brief   Compact wire codec for Heltec Team runtime and join frames.
 * @version 1.0
 * @date    2026-09-04
 */
#include "app/team_protocol.hpp"

#include <string.h>

namespace heltec::meshcore::team
{
namespace
{

class Writer
{
  public:
    Writer(uint8_t *data, size_t capacity) : _data(data), _capacity(capacity)
    {
    }

    bool put_u8(uint8_t value)
    {
        return put(&value, sizeof(value));
    }

    bool put_u16(uint16_t value)
    {
        const uint8_t bytes[2] = { static_cast<uint8_t>(value),
                                   static_cast<uint8_t>(value >> 8) };
        return put(bytes, sizeof(bytes));
    }

    bool put_u32(uint32_t value)
    {
        const uint8_t bytes[4] = { static_cast<uint8_t>(value),
                                   static_cast<uint8_t>(value >> 8),
                                   static_cast<uint8_t>(value >> 16),
                                   static_cast<uint8_t>(value >> 24) };
        return put(bytes, sizeof(bytes));
    }

    bool put(const void *source, size_t length)
    {
        if (!_data || !source || length > _capacity - _offset)
            return false;
        memcpy(_data + _offset, source, length);
        _offset += length;
        return true;
    }

    size_t size() const
    {
        return _offset;
    }

  private:
    uint8_t *_data = nullptr;
    size_t _capacity = 0;
    size_t _offset = 0;
};

class Reader
{
  public:
    Reader(const uint8_t *data, size_t length) : _data(data), _length(length)
    {
    }

    bool get_u8(uint8_t &value)
    {
        return get(&value, sizeof(value));
    }

    bool get_u16(uint16_t &value)
    {
        uint8_t bytes[2] = {};
        if (!get(bytes, sizeof(bytes)))
            return false;
        value = static_cast<uint16_t>(bytes[0]) |
                static_cast<uint16_t>(static_cast<uint16_t>(bytes[1]) << 8);
        return true;
    }

    bool get_u32(uint32_t &value)
    {
        uint8_t bytes[4] = {};
        if (!get(bytes, sizeof(bytes)))
            return false;
        value = static_cast<uint32_t>(bytes[0]) |
                (static_cast<uint32_t>(bytes[1]) << 8) |
                (static_cast<uint32_t>(bytes[2]) << 16) |
                (static_cast<uint32_t>(bytes[3]) << 24);
        return true;
    }

    bool get(void *destination, size_t length)
    {
        if (!_data || !destination || length > _length - _offset)
            return false;
        memcpy(destination, _data + _offset, length);
        _offset += length;
        return true;
    }

    size_t remaining() const
    {
        return _length - _offset;
    }

  private:
    const uint8_t *_data = nullptr;
    size_t _length = 0;
    size_t _offset = 0;
};

bool encode_header(Writer &writer, Subtype subtype, const CommonHeader &header)
{
    return writer.put_u8(kProtocolVersion) && writer.put_u8(static_cast<uint8_t>(subtype)) &&
           writer.put_u8(header.flags) && writer.put_u8(header.role) &&
           writer.put(header.sender_id, sizeof(header.sender_id)) &&
           writer.put_u16(header.boot_id) && writer.put_u16(header.sequence) &&
           writer.put_u32(header.timestamp);
}

DecodeResult decode_header(Reader &reader, Subtype subtype, CommonHeader &header)
{
    uint8_t version = 0;
    uint8_t encoded_subtype = 0;
    if (!reader.get_u8(version) || !reader.get_u8(encoded_subtype))
        return DecodeResult::InvalidLength;
    if (version != kProtocolVersion)
        return DecodeResult::UnsupportedVersion;
    if (encoded_subtype != static_cast<uint8_t>(subtype))
        return DecodeResult::UnsupportedSubtype;
    if (!reader.get_u8(header.flags) || !reader.get_u8(header.role) ||
        !reader.get(header.sender_id, sizeof(header.sender_id)) ||
        !reader.get_u16(header.boot_id) || !reader.get_u16(header.sequence) ||
        !reader.get_u32(header.timestamp))
    {
        return DecodeResult::InvalidLength;
    }
    return header.role <= 1 ? DecodeResult::Ok : DecodeResult::InvalidField;
}

bool valid_common(const CommonHeader &header)
{
    return header.role <= 1;
}

} // namespace

bool valid_utf8(const uint8_t *data, size_t length)
{
    if (!data && length != 0)
        return false;
    for (size_t i = 0; i < length;)
    {
        const uint8_t c = data[i++];
        if (c == 0 || c < 0x20 || c == 0x7F)
            return false;
        if (c < 0x80)
            continue;
        size_t continuation = 0;
        uint32_t value = 0;
        if ((c & 0xE0) == 0xC0)
        {
            continuation = 1;
            value = c & 0x1F;
            if (value < 2)
                return false;
        }
        else if ((c & 0xF0) == 0xE0)
        {
            continuation = 2;
            value = c & 0x0F;
        }
        else if ((c & 0xF8) == 0xF0)
        {
            continuation = 3;
            value = c & 0x07;
        }
        else
        {
            return false;
        }
        if (continuation > length - i)
            return false;
        for (size_t j = 0; j < continuation; ++j)
        {
            const uint8_t next = data[i++];
            if ((next & 0xC0) != 0x80)
                return false;
            value = (value << 6) | (next & 0x3F);
        }
        if ((continuation == 2 && value < 0x800) ||
            (continuation == 3 && value < 0x10000) || value > 0x10FFFF ||
            (value >= 0xD800 && value <= 0xDFFF))
        {
            return false;
        }
    }
    return true;
}

bool sequence_is_newer(uint16_t candidate, uint16_t current)
{
    const uint16_t difference = static_cast<uint16_t>(candidate - current);
    return difference != 0 && difference < 0x8000;
}

Subtype frame_subtype(const uint8_t *data, size_t length)
{
    if (!data || length < 2 || data[0] != kProtocolVersion)
        return static_cast<Subtype>(0);
    return static_cast<Subtype>(data[1]);
}

bool encode_presence(const PresenceFrame &frame, uint8_t *output, size_t capacity,
                     size_t &output_length)
{
    output_length = 0;
    if (!valid_common(frame.header) || frame.name_len > kMaxNameBytes ||
        !valid_utf8(reinterpret_cast<const uint8_t *>(frame.name), frame.name_len) ||
        memcmp(frame.header.sender_id, frame.public_key, kSenderIdSize) != 0)
    {
        return false;
    }
    Writer writer(output, capacity);
    if (!encode_header(writer, Subtype::Presence, frame.header) ||
        !writer.put_u8(frame.device_type) ||
        !writer.put(frame.public_key, sizeof(frame.public_key)) ||
        !writer.put_u16(frame.capabilities) || !writer.put_u16(frame.battery_mv) ||
        !writer.put_u8(frame.name_len) || !writer.put(frame.name, frame.name_len))
    {
        return false;
    }
    output_length = writer.size();
    return true;
}

bool encode_location(const LocationFrame &frame, uint8_t *output, size_t capacity,
                     size_t &output_length)
{
    output_length = 0;
    const bool location_valid = (frame.header.flags & 0x10) != 0;
    if (!valid_common(frame.header) || frame.latitude_e6 < -90000000 ||
        frame.latitude_e6 > 90000000 || frame.longitude_e6 < -180000000 ||
        frame.longitude_e6 > 180000000 || frame.course_cdeg > 35999 ||
        (!location_valid &&
         (frame.latitude_e6 != 0 || frame.longitude_e6 != 0 || frame.altitude_dm != 0 ||
          frame.accuracy_dm != 0 || frame.speed_cms != 0 || frame.course_cdeg != 0 ||
          frame.fix_age_sec != 0 || frame.satellites != 0)))
    {
        return false;
    }
    Writer writer(output, capacity);
    if (!encode_header(writer, Subtype::Location, frame.header) ||
        !writer.put_u32(static_cast<uint32_t>(frame.latitude_e6)) ||
        !writer.put_u32(static_cast<uint32_t>(frame.longitude_e6)) ||
        !writer.put_u16(static_cast<uint16_t>(frame.altitude_dm)) ||
        !writer.put_u16(frame.accuracy_dm) || !writer.put_u16(frame.speed_cms) ||
        !writer.put_u16(frame.course_cdeg) || !writer.put_u16(frame.fix_age_sec) ||
        !writer.put_u8(frame.satellites) || !writer.put_u16(frame.battery_mv))
    {
        return false;
    }
    output_length = writer.size();
    return true;
}

bool encode_sync_request(const SyncRequestFrame &frame, uint8_t *output, size_t capacity,
                         size_t &output_length)
{
    output_length = 0;
    if (!valid_common(frame.header) || (frame.response_mask & 0x03) == 0 ||
        frame.response_window_sec < 5 || frame.response_window_sec > 30)
    {
        return false;
    }
    Writer writer(output, capacity);
    if (!encode_header(writer, Subtype::SyncRequest, frame.header) ||
        !writer.put_u16(frame.request_id) || !writer.put_u8(frame.response_mask) ||
        !writer.put_u8(frame.response_window_sec))
    {
        return false;
    }
    output_length = writer.size();
    return true;
}

DecodeResult decode_presence(const uint8_t *data, size_t length, PresenceFrame &frame)
{
    frame = PresenceFrame{};
    Reader reader(data, length);
    DecodeResult result = decode_header(reader, Subtype::Presence, frame.header);
    if (result != DecodeResult::Ok)
        return result;
    if (!reader.get_u8(frame.device_type) ||
        !reader.get(frame.public_key, sizeof(frame.public_key)) ||
        !reader.get_u16(frame.capabilities) || !reader.get_u16(frame.battery_mv) ||
        !reader.get_u8(frame.name_len) || frame.name_len > kMaxNameBytes ||
        reader.remaining() != frame.name_len || !reader.get(frame.name, frame.name_len))
    {
        return DecodeResult::InvalidLength;
    }
    if (!valid_utf8(reinterpret_cast<const uint8_t *>(frame.name), frame.name_len))
        return DecodeResult::InvalidUtf8;
    if (memcmp(frame.header.sender_id, frame.public_key, kSenderIdSize) != 0)
        return DecodeResult::InvalidField;
    frame.name[frame.name_len] = '\0';
    return DecodeResult::Ok;
}

DecodeResult decode_location(const uint8_t *data, size_t length, LocationFrame &frame)
{
    frame = LocationFrame{};
    Reader reader(data, length);
    DecodeResult result = decode_header(reader, Subtype::Location, frame.header);
    uint32_t latitude = 0;
    uint32_t longitude = 0;
    uint16_t altitude = 0;
    if (result != DecodeResult::Ok)
        return result;
    if (!reader.get_u32(latitude) || !reader.get_u32(longitude) ||
        !reader.get_u16(altitude) || !reader.get_u16(frame.accuracy_dm) ||
        !reader.get_u16(frame.speed_cms) || !reader.get_u16(frame.course_cdeg) ||
        !reader.get_u16(frame.fix_age_sec) || !reader.get_u8(frame.satellites) ||
        !reader.get_u16(frame.battery_mv) || reader.remaining() != 0)
    {
        return DecodeResult::InvalidLength;
    }
    frame.latitude_e6 = static_cast<int32_t>(latitude);
    frame.longitude_e6 = static_cast<int32_t>(longitude);
    frame.altitude_dm = static_cast<int16_t>(altitude);
    if (frame.latitude_e6 < -90000000 || frame.latitude_e6 > 90000000 ||
        frame.longitude_e6 < -180000000 || frame.longitude_e6 > 180000000 ||
        frame.course_cdeg > 35999 ||
        ((frame.header.flags & 0x10) == 0 &&
         (frame.latitude_e6 != 0 || frame.longitude_e6 != 0 || frame.altitude_dm != 0 ||
          frame.accuracy_dm != 0 || frame.speed_cms != 0 || frame.course_cdeg != 0 ||
          frame.fix_age_sec != 0 || frame.satellites != 0)))
    {
        return DecodeResult::InvalidField;
    }
    return DecodeResult::Ok;
}

DecodeResult decode_sync_request(const uint8_t *data, size_t length, SyncRequestFrame &frame)
{
    frame = SyncRequestFrame{};
    Reader reader(data, length);
    DecodeResult result = decode_header(reader, Subtype::SyncRequest, frame.header);
    if (result != DecodeResult::Ok)
        return result;
    if (!reader.get_u16(frame.request_id) || !reader.get_u8(frame.response_mask) ||
        !reader.get_u8(frame.response_window_sec) || reader.remaining() != 0)
    {
        return DecodeResult::InvalidLength;
    }
    if ((frame.response_mask & 0x03) == 0 || frame.response_window_sec < 5 ||
        frame.response_window_sec > 30)
    {
        return DecodeResult::InvalidField;
    }
    return DecodeResult::Ok;
}

} // namespace heltec::meshcore::team
