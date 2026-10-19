#include "TeamGeofence.hpp"

#include <math.h>

namespace heltec::meshcore::team
{
namespace
{
constexpr double kE7 = 10000000.0;
constexpr double kEarthRadiusM = 6371000.0;
constexpr double kPi = 3.14159265358979323846;
}

bool TeamGeofence::validPoint(int32_t latitude_e7, int32_t longitude_e7)
{
    return latitude_e7 >= -900000000 && latitude_e7 <= 900000000 &&
           longitude_e7 >= -1800000000 && longitude_e7 <= 1800000000;
}

uint32_t TeamGeofence::distanceMeters(const GeofencePoint &a, const GeofencePoint &b)
{
    const double lat1 = static_cast<double>(a.latitude_e7) / kE7 * kPi / 180.0;
    const double lat2 = static_cast<double>(b.latitude_e7) / kE7 * kPi / 180.0;
    const double dlat = lat2 - lat1;
    const double dlon = ((static_cast<double>(b.longitude_e7) - a.longitude_e7) / kE7) *
                        kPi / 180.0;
    const double raw_h = sin(dlat * 0.5) * sin(dlat * 0.5) +
                         cos(lat1) * cos(lat2) * sin(dlon * 0.5) * sin(dlon * 0.5);
    const double h = fmax(0.0, fmin(1.0, raw_h));
    return static_cast<uint32_t>(kEarthRadiusM * 2.0 * atan2(sqrt(h), sqrt(1.0 - h)) + 0.5);
}

bool TeamGeofence::beginEdit()
{
    if (_state == GeofenceState::Active)
        return false;
    _point_count = 0;
    _state = GeofenceState::Editing;
    return true;
}

void TeamGeofence::cancelEdit()
{
    _point_count = 0;
    _state = GeofenceState::Empty;
}

void TeamGeofence::deleteFence()
{
    cancelEdit();
}

GeofenceAddResult TeamGeofence::addPoint(int32_t latitude_e7, int32_t longitude_e7)
{
    if (_state != GeofenceState::Editing || !validPoint(latitude_e7, longitude_e7))
        return GeofenceAddResult::Invalid;
    GeofencePoint candidate;
    candidate.latitude_e7 = latitude_e7;
    candidate.longitude_e7 = longitude_e7;
    if (_point_count > 0 && distanceMeters(_points[_point_count - 1U], candidate) == 0U)
        return GeofenceAddResult::Invalid;
    if (_point_count >= 3 && distanceMeters(_points[0], candidate) <= kCloseDistanceM)
    {
        if (!hasUsableArea())
            return GeofenceAddResult::Invalid;
        _state = GeofenceState::Active;
        return GeofenceAddResult::Completed;
    }
    if (_point_count >= kMaxPoints)
        return GeofenceAddResult::Full;
    _points[_point_count] = candidate;
    ++_point_count;
    return GeofenceAddResult::Added;
}

bool TeamGeofence::hasUsableArea() const
{
    if (_point_count < 3)
        return false;
    double area_twice = 0.0;
    const double origin_latitude = _points[0].latitude_e7;
    const double origin_longitude = _points[0].longitude_e7;
    for (uint8_t index = 1; index + 1U < _point_count; ++index)
    {
        const double ax = _points[index].longitude_e7 - origin_longitude;
        const double ay = _points[index].latitude_e7 - origin_latitude;
        const double bx = _points[index + 1U].longitude_e7 - origin_longitude;
        const double by = _points[index + 1U].latitude_e7 - origin_latitude;
        area_twice += ax * by - bx * ay;
    }
    return fabs(area_twice) > 0.5;
}

bool TeamGeofence::contains(int32_t latitude_e7, int32_t longitude_e7) const
{
    if (_state != GeofenceState::Active || _point_count < 3 || !validPoint(latitude_e7, longitude_e7))
        return false;
    bool inside = false;
    for (uint8_t i = 0, j = _point_count - 1; i < _point_count; j = i++)
    {
        const double yi = _points[i].latitude_e7;
        const double yj = _points[j].latitude_e7;
        if ((yi > latitude_e7) == (yj > latitude_e7))
            continue;
        const double xi = _points[i].longitude_e7;
        const double xj = _points[j].longitude_e7;
        const double edge_longitude =
            xi + (xj - xi) * (static_cast<double>(latitude_e7) - yi) / (yj - yi);
        if (static_cast<double>(longitude_e7) < edge_longitude)
            inside = !inside;
    }
    return inside;
}

} // namespace heltec::meshcore::team
