#include "MessagePresetStore.hpp"

#include "DataStore.h"

#include <cstring>

namespace
{
constexpr char kPath[] = "/message_presets";
constexpr char kMagic[] = { 'M', 'P', 'S', '1' };
constexpr const char *kDefaults[MessagePresetStore::kCount] = {
    "hi", "bye", "yes", "no", "ok",
};

struct PresetFileV1
{
    char magic[4];
    uint8_t count;
    uint8_t capacity;
    uint16_t payload_size;
    char items[MessagePresetStore::kCount][MessagePresetStore::kCapacity];
    uint32_t crc32;
};

static_assert(sizeof(PresetFileV1) == 172, "Unexpected message preset file layout");

uint32_t calculate_crc32(const uint8_t *data, size_t length)
{
    uint32_t crc = 0xFFFFFFFFU;
    for (size_t index = 0; index < length; ++index)
    {
        crc ^= data[index];
        for (uint8_t bit = 0; bit < 8; ++bit)
        {
            const uint32_t mask =
                static_cast<uint32_t>(-(static_cast<int32_t>(crc & 1U)));
            crc = (crc >> 1U) ^ (0xEDB88320U & mask);
        }
    }
    return ~crc;
}

bool valid_utf8(const char *text, size_t length)
{
    size_t index = 0;
    while (index < length)
    {
        const uint8_t first = static_cast<uint8_t>(text[index]);
        if (first < 0x80U)
        {
            if (first < 0x20U || first == 0x7FU)
                return false;
            ++index;
            continue;
        }

        size_t continuation = 0;
        uint32_t codepoint = 0;
        if ((first & 0xE0U) == 0xC0U)
        {
            continuation = 1;
            codepoint = first & 0x1FU;
            if (codepoint < 2U)
                return false;
        }
        else if ((first & 0xF0U) == 0xE0U)
        {
            continuation = 2;
            codepoint = first & 0x0FU;
        }
        else if ((first & 0xF8U) == 0xF0U)
        {
            continuation = 3;
            codepoint = first & 0x07U;
        }
        else
        {
            return false;
        }
        if (index + continuation >= length)
            return false;
        for (size_t offset = 1; offset <= continuation; ++offset)
        {
            const uint8_t next = static_cast<uint8_t>(text[index + offset]);
            if ((next & 0xC0U) != 0x80U)
                return false;
            codepoint = (codepoint << 6U) | (next & 0x3FU);
        }
        if ((continuation == 2 && codepoint < 0x800U) ||
            (continuation == 3 && codepoint < 0x10000U) || codepoint > 0x10FFFFU ||
            (codepoint >= 0xD800U && codepoint <= 0xDFFFU))
            return false;
        index += continuation + 1;
    }
    return true;
}
} // namespace

MessagePresetStore::MessagePresetStore()
{
    resetDefaults();
}

void MessagePresetStore::resetDefaults()
{
    for (uint8_t index = 0; index < kCount; ++index)
    {
        strncpy(_items[index], kDefaults[index], kCapacity - 1);
        _items[index][kCapacity - 1] = '\0';
    }
}

bool MessagePresetStore::validText(const char *text, size_t *length)
{
    if (!text)
        return false;
    size_t text_length = 0;
    while (text_length < kCapacity && text[text_length] != '\0')
        ++text_length;
    if (text_length == 0 || text_length >= kCapacity || text[0] == ' ' ||
        text[text_length - 1] == ' ' || !valid_utf8(text, text_length))
        return false;
    if (length)
        *length = text_length;
    return true;
}

void MessagePresetStore::ensureLoaded(DataStore &store)
{
    if (_loaded)
        return;
    resetDefaults();
    (void)load(store);
    _loaded = true;
    _dirty = false;
}

bool MessagePresetStore::load(DataStore &store)
{
    PresetFileV1 file{};
    if (store.readSmallConfig(kPath, &file, sizeof(file)) != sizeof(file) ||
        memcmp(file.magic, kMagic, sizeof(kMagic)) != 0 || file.count != kCount ||
        file.capacity != kCapacity || file.payload_size != sizeof(file.items))
        return false;
    const uint32_t crc = calculate_crc32(reinterpret_cast<const uint8_t *>(&file),
                                         offsetof(PresetFileV1, crc32));
    if (file.crc32 != crc)
        return false;
    for (uint8_t index = 0; index < kCount; ++index)
    {
        if (!validText(file.items[index]))
            return false;
    }
    memcpy(_items, file.items, sizeof(_items));
    return true;
}

const char *MessagePresetStore::at(int index) const
{
    return index >= 0 && index < kCount ? _items[index] : "";
}

MessagePresetStore::UpdateResult MessagePresetStore::update(int index, const char *text)
{
    if (index < 0 || index >= kCount)
        return UpdateResult::InvalidIndex;
    if (!text || text[0] == '\0')
        return UpdateResult::Empty;
    size_t length = 0;
    if (!validText(text, &length))
    {
        size_t measured = 0;
        while (measured < kCapacity && text[measured] != '\0')
            ++measured;
        return measured >= kCapacity ? UpdateResult::TooLong : UpdateResult::InvalidText;
    }
    if (strcmp(_items[index], text) == 0)
        return UpdateResult::Unchanged;
    memcpy(_items[index], text, length + 1);
    _dirty = true;
    return UpdateResult::Updated;
}

bool MessagePresetStore::commit(DataStore &store)
{
    if (!_dirty)
        return true;
    PresetFileV1 file{};
    memcpy(file.magic, kMagic, sizeof(kMagic));
    file.count = kCount;
    file.capacity = kCapacity;
    file.payload_size = sizeof(file.items);
    memcpy(file.items, _items, sizeof(file.items));
    file.crc32 = calculate_crc32(reinterpret_cast<const uint8_t *>(&file),
                                 offsetof(PresetFileV1, crc32));
    if (!store.writeSmallConfig(kPath, &file, sizeof(file)))
        return false;
    _dirty = false;
    return true;
}
