#include "ui/app/BehaviorProfile.hpp"

namespace Heltec::MeshCore::Ui
{
namespace
{

const UiBehaviorProfile kDefaultBehaviorProfile = {
    { 0 },
    { 5000, 560, 380, 4, 2 },
};

const UiBehaviorProfile *s_active_behavior_profile = nullptr;

} // namespace

const UiBehaviorProfile &behaviorProfile()
{
    return s_active_behavior_profile ? *s_active_behavior_profile : kDefaultBehaviorProfile;
}

void setBehaviorProfile(const UiBehaviorProfile *profile)
{
    s_active_behavior_profile = profile;
}

} // namespace Heltec::MeshCore::Ui
