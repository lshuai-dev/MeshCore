#pragma once

#include <stdint.h>

namespace Heltec::MeshCore::Team
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

class TeamGeofence
{
  public:
    static constexpr uint8_t kMaxPoints = 12;
    static constexpr uint32_t kCloseDistanceM = 10;

    bool beginEdit();
    void cancelEdit();
    void deleteFence();
    GeofenceAddResult addPoint(int32_t latitude_e7, int32_t longitude_e7);
    bool contains(int32_t latitude_e7, int32_t longitude_e7) const;
    GeofenceState state() const { return _state; }
    uint8_t pointCount() const { return _point_count; }
    const GeofencePoint *points() const { return _points; }

  private:
    static bool validPoint(int32_t latitude_e7, int32_t longitude_e7);
    static uint32_t distanceMeters(const GeofencePoint &a, const GeofencePoint &b);
    bool hasUsableArea() const;
    GeofencePoint _points[kMaxPoints] = {};
    uint8_t _point_count = 0;
    GeofenceState _state = GeofenceState::Empty;
};

} // namespace Heltec::MeshCore::Team
