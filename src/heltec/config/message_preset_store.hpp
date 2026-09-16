#pragma once

#include <stddef.h>
#include <stdint.h>

class DataStore;

class MessagePresetStore
{
  public:
    static constexpr uint8_t kCount = 5;
    static constexpr size_t kCapacity = 32;

    enum class UpdateResult : uint8_t
    {
        Updated,
        Unchanged,
        Empty,
        TooLong,
        InvalidText,
        InvalidIndex,
    };

    MessagePresetStore();
    void ensureLoaded(DataStore &store);
    const char *at(int index) const;
    UpdateResult update(int index, const char *text);
    bool commit(DataStore &store);

  private:
    void resetDefaults();
    bool load(DataStore &store);
    static bool validText(const char *text, size_t *length = nullptr);

    char _items[kCount][kCapacity] = {};
    bool _loaded = false;
    bool _dirty = false;
};
