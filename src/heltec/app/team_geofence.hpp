#pragma once

#include <stdint.h>

namespace heltec::meshcore::team
{

struct GeofencePoint
{
    int32_t latitude_e7 = 0;
    int32_t longitude_e7 = 0;
};

enum class GeofenceState : uint8_t
{
    Empty,
    Editing,
    Active,
};

enum class GeofenceAddResult : uint8_t
{
    Added,
    Completed,
    Full,
    Invalid,
};

enum class GeofenceMemberResult : uint8_t
{
    Unchanged,
    AlarmStarted,
    AlarmCleared,
};

struct GeofenceMemberState
{
    bool occupied = false;
    bool seen = false;
    bool alarm_active = false;
    uint8_t outside_count = 0;
    uint8_t member_id[6] = {};
};

class TeamGeofence
{
  public:
    static constexpr uint8_t kMaxPoints = 12;
    static constexpr uint8_t kMaxMembers = 32;
    static constexpr uint32_t kCloseDistanceM = 10;
    static constexpr uint8_t kOutsideSamplesBeforeAlarm = 4;

    bool beginEdit();
    void cancelEdit();
    void deleteFence();
    GeofenceAddResult addPoint(int32_t latitude_e7, int32_t longitude_e7);
    bool contains(int32_t latitude_e7, int32_t longitude_e7) const;
    GeofenceMemberResult updateMember(const uint8_t member_id[6], bool location_known,
                                      int32_t latitude_e7, int32_t longitude_e7);
    bool memberAlarmActive(const uint8_t member_id[6]) const;
    void beginMemberSweep();
    void endMemberSweep();
    GeofenceState state() const { return _state; }
    uint8_t pointCount() const { return _point_count; }
    const GeofencePoint *points() const { return _points; }

  private:
    static bool validPoint(int32_t latitude_e7, int32_t longitude_e7);
    static uint32_t distanceMeters(const GeofencePoint &a, const GeofencePoint &b);
    bool hasUsableArea() const;
    GeofenceMemberState *findMember(const uint8_t member_id[6], bool allocate);
    const GeofenceMemberState *findMember(const uint8_t member_id[6]) const;
    void clearMembers();
    GeofencePoint _points[kMaxPoints] = {};
    uint8_t _point_count = 0;
    GeofenceState _state = GeofenceState::Empty;
    GeofenceMemberState _members[kMaxMembers] = {};
};

} // namespace heltec::meshcore::team
