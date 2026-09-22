#include <gtest/gtest.h>

#include "heltec/app/team_geofence.hpp"

using heltec::meshcore::team::GeofenceAddResult;
using heltec::meshcore::team::GeofenceMemberResult;
using heltec::meshcore::team::GeofenceState;
using heltec::meshcore::team::TeamGeofence;

namespace
{
void makeFence(TeamGeofence &fence)
{
    ASSERT_TRUE(fence.beginEdit());
    EXPECT_EQ(GeofenceAddResult::Added, fence.addPoint(100000000, 100000000));
    EXPECT_EQ(GeofenceAddResult::Added, fence.addPoint(100000000, 100010000));
    EXPECT_EQ(GeofenceAddResult::Added, fence.addPoint(100010000, 100010000));
    EXPECT_EQ(GeofenceAddResult::Completed, fence.addPoint(100000000, 100000000));
    ASSERT_EQ(GeofenceState::Active, fence.state());
}
}

TEST(TeamGeofence, RejectsDegeneratePolygon)
{
    TeamGeofence fence;
    ASSERT_TRUE(fence.beginEdit());
    EXPECT_EQ(GeofenceAddResult::Added, fence.addPoint(100000000, 100000000));
    EXPECT_EQ(GeofenceAddResult::Added, fence.addPoint(100001000, 100001000));
    EXPECT_EQ(GeofenceAddResult::Added, fence.addPoint(100002000, 100002000));
    EXPECT_EQ(GeofenceAddResult::Invalid, fence.addPoint(100000000, 100000000));
    EXPECT_EQ(GeofenceState::Editing, fence.state());
}

TEST(TeamGeofence, AlarmsOnFourthConsecutiveOutsideSample)
{
    TeamGeofence fence;
    makeFence(fence);
    const uint8_t member[6] = {1, 2, 3, 4, 5, 6};
    for (uint8_t count = 1; count < TeamGeofence::kOutsideSamplesBeforeAlarm; ++count)
    {
        EXPECT_EQ(GeofenceMemberResult::Unchanged,
                  fence.updateMember(member, true, 100020000, 100020000));
    }
    EXPECT_EQ(GeofenceMemberResult::AlarmStarted,
              fence.updateMember(member, true, 100020000, 100020000));
    EXPECT_TRUE(fence.memberAlarmActive(member));
    EXPECT_EQ(GeofenceMemberResult::Unchanged,
              fence.updateMember(member, true, 100020000, 100020000));
}

TEST(TeamGeofence, UnknownPreservesCountAndInsideClearsAlarm)
{
    TeamGeofence fence;
    makeFence(fence);
    const uint8_t member[6] = {7, 8, 9, 10, 11, 12};
    for (uint8_t count = 0; count < TeamGeofence::kOutsideSamplesBeforeAlarm; ++count)
        (void)fence.updateMember(member, true, 100020000, 100020000);
    EXPECT_EQ(GeofenceMemberResult::Unchanged, fence.updateMember(member, false, 0, 0));
    EXPECT_TRUE(fence.memberAlarmActive(member));
    EXPECT_EQ(GeofenceMemberResult::AlarmCleared,
              fence.updateMember(member, true, 100005000, 100005000));
    EXPECT_FALSE(fence.memberAlarmActive(member));
}

TEST(TeamGeofence, SweepRemovesMembersNoLongerInTeam)
{
    TeamGeofence fence;
    makeFence(fence);
    const uint8_t member[6] = {1, 1, 1, 1, 1, 1};
    for (uint8_t count = 0; count < TeamGeofence::kOutsideSamplesBeforeAlarm; ++count)
        (void)fence.updateMember(member, true, 100020000, 100020000);
    ASSERT_TRUE(fence.memberAlarmActive(member));
    fence.beginMemberSweep();
    fence.endMemberSweep();
    EXPECT_FALSE(fence.memberAlarmActive(member));
}
