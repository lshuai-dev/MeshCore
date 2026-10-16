#pragma once

#include "node_prefs.h"

#include <Arduino.h>
#include <math.h>
#include <string.h>

/** Named radio presets; pathHashSize=0 preserves the current path hash setting. */
struct RadioParamPreset
{
    const char *name;
    float freqMhz;
    uint8_t sf;
    uint8_t cr;
    float bwKhz;
    uint8_t pathHashSize;
};

static constexpr int kRadioParamPresetCount = 17;
static constexpr size_t kRadioParamPresetUiScratchSize = 640;

/** Shared startup/refresh scratch; LVGL dropdown and roller copy the options. */
inline char *radioParamPresetUiScratch()
{
    static char buffer[kRadioParamPresetUiScratchSize];
    return buffer;
}

inline int radioParamPresetCount()
{
    return kRadioParamPresetCount;
}

inline int loraBandPresetCount()
{
    return radioParamPresetCount();
}

inline const RadioParamPreset &radioParamPreset(int index)
{
    static const RadioParamPreset kPresets[] = {
        { "Australia", 915.800f, 10, 5, 250.0f, 0 },
        { "Australia (Narrow)", 916.575f, 7, 7, 62.5f, 0 },
        { "Australia (Mid)", 915.075f, 9, 5, 125.0f, 0 },
        { "Australia: SA, WA", 923.125f, 8, 8, 62.5f, 0 },
        { "Australia: QLD", 923.125f, 8, 5, 62.5f, 0 },
        { "EU/UK (Narrow)", 869.618f, 8, 8, 62.5f, 0 },
        { "EU/UK (Deprecated)", 869.525f, 11, 5, 250.0f, 0 },
        { "Czech Republic (Narrow)", 869.432f, 7, 5, 62.5f, 2 },
        { "EU 433MHz (Long Range)", 433.650f, 11, 5, 250.0f, 0 },
        { "EU 433MHz (Narrow)", 433.650f, 8, 8, 62.5f, 0 },
        { "New Zealand", 917.375f, 11, 5, 250.0f, 1 },
        { "New Zealand (Narrow)", 917.375f, 7, 5, 62.5f, 2 },
        { "Portugal 433", 433.375f, 9, 6, 62.5f, 0 },
        { "Portugal 868", 869.618f, 7, 6, 62.5f, 0 },
        { "Switzerland", 869.618f, 8, 8, 62.5f, 0 },
        { "USA/Canada (Rec)", 910.525f, 7, 5, 62.5f, 0 },
        { "Vietnam (Narrow)", 920.250f, 8, 5, 62.5f, 0 },
    };
    if (index < 0 || index >= kRadioParamPresetCount)
    {
        return kPresets[0];
    }
    return kPresets[index];
}

inline const RadioParamPreset &loraBandPreset(int index)
{
    return radioParamPreset(index);
}

inline const char *radioParamPresetName(int index)
{
    return radioParamPreset(index).name;
}

inline const char *loraBandPresetName(int index)
{
    return radioParamPresetName(index);
}

inline const char *radioParamPresetDropdownName(int index)
{
    const char *name = radioParamPresetName(index);
    if (!name)
        return "";
    return name;
}

inline float loraBandPresetFreqMhz(int index)
{
    return radioParamPreset(index).freqMhz;
}

inline bool radioParamPresetMatchesPrefs(int index, const NodePrefs *p)
{
    if (!p)
        return false;
    const RadioParamPreset &pr = radioParamPreset(index);
    const float df = fabsf(pr.freqMhz - p->freq);
    const float dbw = fabsf(pr.bwKhz - p->bw);
    const bool path_hash_matches =
        pr.pathHashSize == 0 || pr.pathHashSize == static_cast<uint8_t>(p->path_hash_mode + 1U);
    return (df < 0.05f) && (dbw < 2.0f) && (pr.sf == p->sf) && (pr.cr == p->cr) &&
           path_hash_matches;
}

inline int radioParamPresetIndexForPrefs(const NodePrefs *p)
{
    if (!p)
        return 0;
    for (int i = 0; i < radioParamPresetCount(); ++i)
    {
        if (radioParamPresetMatchesPrefs(i, p))
            return i;
    }
    if (p->freq > 0.0f)
    {
        for (int i = 0; i < radioParamPresetCount(); ++i)
        {
            const float f = loraBandPresetFreqMhz(i);
            const float d = fabsf(f - p->freq);
            if (d < 0.05f)
                return i;
        }
    }
    return 0;
}

inline int loraBandPresetIndexForFreqMHz(float mhz)
{
    NodePrefs stub{};
    stub.freq = mhz;
    stub.bw = 250.0f;
    stub.sf = 10;
    stub.cr = 5;
    return radioParamPresetIndexForPrefs(&stub);
}

/** Build lv_dropdown options string ("A\\nB\\nC"). Returns bytes written (excl. NUL). */
inline size_t radioParamPresetDropdownOptions(char *buf, size_t buf_size)
{
    if (!buf || buf_size < 2)
        return 0;
    size_t pos = 0;
    buf[0] = '\0';
    for (int i = 0; i < radioParamPresetCount(); ++i)
    {
        const char *name = radioParamPresetDropdownName(i);
        if (!name)
            name = "";
        if (i > 0)
        {
            if (pos + 1 >= buf_size)
                break;
            buf[pos++] = '\n';
            buf[pos] = '\0';
        }
        const size_t room = buf_size - pos;
        if (room <= 1)
            break;
        const size_t n = strnlen(name, room - 1);
        memcpy(buf + pos, name, n);
        pos += n;
        buf[pos] = '\0';
    }
    return pos;
}
