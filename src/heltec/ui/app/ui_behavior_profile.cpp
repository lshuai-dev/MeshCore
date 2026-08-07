#include "ui/app/ui_behavior_profile.hpp"

namespace heltec::meshcore::ui {
namespace {

const UiBehaviorProfile kDefaultBehaviorProfile = {
    {0},
    {5000, 560, 380, 4, 2},
};

const UiBehaviorProfile* s_active_behavior_profile = nullptr;

}  // namespace

const UiBehaviorProfile& ui_behavior_profile() {
  return s_active_behavior_profile ? *s_active_behavior_profile
                                   : kDefaultBehaviorProfile;
}

void ui_set_behavior_profile(const UiBehaviorProfile* profile) {
  s_active_behavior_profile = profile;
}

}  // namespace heltec::meshcore::ui
