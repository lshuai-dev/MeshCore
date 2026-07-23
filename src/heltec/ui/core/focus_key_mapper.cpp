#include "focus_key_mapper.hpp"

#include "ui/core/screen_id.hpp"
#include "ui/core/ht_meta_data.hpp"
#include "ui/navigation/ui_navigator.hpp"
#include "ui/overlays/preview_overlay.hpp"
#include "ui/overlays/radio_pram_sync_overlay.hpp"
#include "ui/overlays/send_message_overlay_ids.hpp"
#include "ui/screens/system_screen.hpp"

#include <lvgl.h>

namespace heltec::meshcore::ui {
namespace {

static bool is_scroll_focused_screen_root(lv_obj_t* obj) {
  switch (ht_id(obj)) {
    case meta_id::ScreenRoot:
    case meta_id::HomeScreenRoot:
    case meta_id::GpsScreenRoot:
    case meta_id::RadioScreenRoot:
    case meta_id::RecentScreenRoot:
    case meta_id::CompassScreenRoot:
    case meta_id::FindFriendScreenRoot:
      return true;
#if defined(ENV_INCLUDE_MAP) && ENV_INCLUDE_MAP
    case meta_id::TrackerScreenRoot:
      return false;
#endif
    case meta_id::SystemRoot:
    default:
      return false;
  }
}

}  // namespace

uint32_t FocusKeyMapper::translate(uint32_t lv_key) {
  return translateForGroup(lv_group_get_default(), lv_key);
}

uint32_t FocusKeyMapper::translateForGroup(lv_group_t* group, uint32_t lv_key) {
  if (!group) return lv_key;
  return translateForObject(lv_group_get_focused(group), lv_key);
}

uint32_t FocusKeyMapper::translateForObject(lv_obj_t* obj, uint32_t lv_key) {
  if (!obj) return lv_key;

  if (is_scroll_focused_screen_root(obj)) {
    if (lv_key == LV_KEY_NEXT) return LV_KEY_DOWN;
    if (lv_key == LV_KEY_PREV) return LV_KEY_UP;
    return lv_key;
  }

  if (ht_id(obj) == meta_id::SendMessageOverlayRoot) {
    if (lv_key == LV_KEY_NEXT) {
      return LV_KEY_DOWN;
    }
    if (lv_key == LV_KEY_PREV) {
      return LV_KEY_UP;
    }
    return lv_key;
  }

  if (ht_id(obj) == meta_id::RadioParamSyncOverlayRoot) {
    if (lv_key == LV_KEY_NEXT) return LV_KEY_RIGHT;
    if (lv_key == LV_KEY_PREV) return LV_KEY_LEFT;
    return lv_key;
  }

  if (ht_id(obj) == meta_id::PreviewText) {
    if (lv_key == LV_KEY_NEXT || lv_key == LV_KEY_PREV) return LV_KEY_ENTER;
    return lv_key;
  }

  const MetaId id = ht_id(obj);
  if (id == meta_id::NavigationPanel || id == meta_id::NavigationRing) {
    if (lv_key == LV_KEY_PREV) return LV_KEY_LEFT;
    if (lv_key == LV_KEY_NEXT) return LV_KEY_RIGHT;
    return lv_key;
  }

#if LV_USE_ROLLER
  if (lv_obj_check_type(obj, &lv_roller_class)) {
    if (lv_key == LV_KEY_NEXT || lv_key == LV_KEY_RIGHT) return LV_KEY_DOWN;
    if (lv_key == LV_KEY_PREV || lv_key == LV_KEY_LEFT) return LV_KEY_UP;
    return lv_key;
  }
#endif

#if LV_USE_KEYBOARD
  if (lv_obj_has_class(obj, &lv_keyboard_class)) {
    if (lv_keyboard_get_mode(obj) == LV_KEYBOARD_MODE_USER_1) {
      if (lv_key == LV_KEY_NEXT) return LV_KEY_RIGHT;
      if (lv_key == LV_KEY_PREV) return LV_KEY_LEFT;
      if (lv_key == LV_KEY_RIGHT) return LV_KEY_DOWN;
      if (lv_key == LV_KEY_LEFT) return LV_KEY_UP;
      return lv_key;
    }
    if (lv_key == LV_KEY_NEXT || lv_key == LV_KEY_RIGHT) return LV_KEY_RIGHT;
    if (lv_key == LV_KEY_PREV || lv_key == LV_KEY_LEFT) return LV_KEY_LEFT;
    return lv_key;
  }
#endif

#if LV_USE_DROPDOWN
  if (lv_obj_check_type(obj, &lv_dropdown_class) && lv_dropdown_is_open(obj)) {
    if (lv_key == LV_KEY_NEXT || lv_key == LV_KEY_RIGHT) return LV_KEY_DOWN;
    if (lv_key == LV_KEY_PREV || lv_key == LV_KEY_LEFT) return LV_KEY_UP;
    return lv_key;
  }
#endif

  return lv_key;
}

}  // namespace heltec::meshcore::ui
