#include "ui/app/ui_app.hpp"

#include "heltec/drivers/display/display_port.hpp"
#include "heltec/drivers/input/touch_port.hpp"
#include <Arduino.h>

#ifndef HELTEC_TOUCH_EDGE_PX
#define HELTEC_TOUCH_EDGE_PX 24
#endif
#ifndef HELTEC_TOUCH_ACTION_EDGE_PX
#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
#define HELTEC_TOUCH_ACTION_EDGE_PX 24
#else
#define HELTEC_TOUCH_ACTION_EDGE_PX HELTEC_TOUCH_EDGE_PX
#endif
#endif
#ifndef HELTEC_TOUCH_TOP_ACTION_SIDE_MARGIN_PCT
#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
#define HELTEC_TOUCH_TOP_ACTION_SIDE_MARGIN_PCT 10
#else
#define HELTEC_TOUCH_TOP_ACTION_SIDE_MARGIN_PCT 0
#endif
#endif

namespace heltec::meshcore::ui {
namespace {
lv_coord_t current_display_width() {
  lv_disp_t* disp = lv_disp_get_default();
  return disp ? lv_disp_get_hor_res(disp) : 0;
}
lv_coord_t current_display_height() {
  lv_disp_t* disp = lv_disp_get_default();
  return disp ? lv_disp_get_ver_res(disp) : 0;
}
bool is_in_top_action_x_band(int16_t start_x) {
  const lv_coord_t w = current_display_width();
  if (w <= 0) return true;
  const lv_coord_t margin = (w * HELTEC_TOUCH_TOP_ACTION_SIDE_MARGIN_PCT) / 100;
  return start_x >= margin && start_x <= (w - margin);
}
bool is_left_edge_right_swipe(lv_dir_t direction, int16_t start_x) {
  return direction == LV_DIR_RIGHT && start_x <= HELTEC_TOUCH_ACTION_EDGE_PX;
}
bool is_top_edge_down_swipe(lv_dir_t direction, int16_t start_y) {
  return direction == LV_DIR_BOTTOM && start_y <= HELTEC_TOUCH_ACTION_EDGE_PX;
}
#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
bool is_right_edge_left_swipe(lv_dir_t direction, int16_t start_x) {
  const lv_coord_t w = current_display_width();
  if (w <= HELTEC_TOUCH_ACTION_EDGE_PX) return false;
  return direction == LV_DIR_LEFT &&
         start_x >= (w - HELTEC_TOUCH_ACTION_EDGE_PX);
}
bool is_bottom_edge_up_swipe(lv_dir_t direction, int16_t start_y) {
  const lv_coord_t h = current_display_height();
  if (h <= HELTEC_TOUCH_ACTION_EDGE_PX) return false;
  return direction == LV_DIR_TOP &&
         start_y >= (h - HELTEC_TOUCH_ACTION_EDGE_PX);
}
#endif
bool is_top_action_down_swipe(lv_dir_t direction, int16_t start_x,
                              int16_t start_y) {
  return is_top_edge_down_swipe(direction, start_y) &&
         is_in_top_action_x_band(start_x);
}
bool point_inside_obj(const _lv_obj_t* obj, int16_t x, int16_t y) {
  if (!obj || !lv_obj_is_valid(obj) || lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN)) return false;
  lv_point_t point{static_cast<lv_coord_t>(x), static_cast<lv_coord_t>(y)};
  return lv_obj_hit_test(const_cast<_lv_obj_t*>(obj), &point);
}

bool point_inside_open_dropdown_list(const _lv_obj_t* root, int16_t x, int16_t y) {
#if LV_USE_DROPDOWN
  if (!root || !lv_obj_is_valid(root) || lv_obj_has_flag(root, LV_OBJ_FLAG_HIDDEN)) return false;
  if (lv_obj_check_type(root, &lv_dropdownlist_class) && point_inside_obj(root, x, y)) {
    return true;
  }
  const uint32_t count = lv_obj_get_child_cnt(root);
  for (uint32_t i = 0; i < count; ++i) {
    if (point_inside_open_dropdown_list(lv_obj_get_child(root, i), x, y)) return true;
  }
#else
  (void)root;
  (void)x;
  (void)y;
#endif
  return false;
}

bool point_inside_any_open_dropdown_list(int16_t x, int16_t y) {
  _lv_obj_t* const screen = lv_scr_act();
  if (point_inside_open_dropdown_list(screen, x, y)) return true;
  _lv_obj_t* const top = lv_layer_top();
  return top != screen && point_inside_open_dropdown_list(top, x, y);
}
}  // namespace

#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH

void UiApp::openNavigationPaneFromEdgeSwipe() {
  if (!_surfaces.contains(&_navigation) && inputOnActiveScreen()) openNavigationPane();
}

void UiApp::closeNavigationPaneFromSwipe() {
  if (!_surfaces.contains(&_navigation)) return;
  _nav_last_activity_ms = 0;
  stopNavigationAutoCommitTimer();
  if (!_navigation.requestCloseAnimation()) closeNavigationPane();
}

void UiApp::openQuickPingFromTopSwipe() {
  if (_surfaces.contains(&_quickPingOverlay)) return;

  if (_surfaces.contains(&_navigation)) {
    closeNavigationImmediate();
  }
  if (inputOnActiveScreen()) {
    (void)_surfaces.present(&_quickPingOverlay, _surfaces.root());
  }
}

void UiApp::closeQuickPingFromSwipe() {
  if (_surfaces.contains(&_quickPingOverlay)) {
    if (!_quickPingOverlay.requestCloseAnimation()) {
      (void)_surfaces.dismissBranch(&_quickPingOverlay);
    }
  }
}

void UiApp::deferTouchAction(DeferredTouchAction action) {
  if (action == DeferredTouchAction::None) return;
  _deferred_touch_action = action;

  if (!_deferred_touch_timer) {
    runDeferredTouchAction();
    return;
  }

  lv_timer_set_period(_deferred_touch_timer, 40U);
  lv_timer_reset(_deferred_touch_timer);
  lv_timer_resume(_deferred_touch_timer);
}

void UiApp::runDeferredTouchAction() {
  const DeferredTouchAction action = _deferred_touch_action;
  _deferred_touch_action = DeferredTouchAction::None;
  if (!_inited || action == DeferredTouchAction::None) return;

  switch (action) {
    case DeferredTouchAction::OpenNavigation:
      openNavigationPaneFromEdgeSwipe();
      break;
    case DeferredTouchAction::CloseNavigation:
      closeNavigationPaneFromSwipe();
      break;
    case DeferredTouchAction::OpenQuickPing:
      openQuickPingFromTopSwipe();
      break;
    case DeferredTouchAction::CloseQuickPing:
      closeQuickPingFromSwipe();
      break;
    case DeferredTouchAction::None:
    default:
      break;
  }
}

void UiApp::deferredTouchActionTimerCb(lv_timer_t* timer) {
  auto* app = timer ? static_cast<UiApp*>(timer->user_data) : nullptr;
  if (!app) return;
  lv_timer_pause(timer);
  app->runDeferredTouchAction();
}

#endif

void UiApp::onTouchSwipe(lv_dir_t direction, int16_t start_x, int16_t start_y) {
  if (!_inited || direction == LV_DIR_NONE) return;
  if (!heltec::meshcore::dal::display_port::isBacklightOn()) {
    notifyDisplayActivity(millis());
    return;
  }

#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
  if (_surfaces.isActive(&_radioParamSyncOvl) ||
      _surfaces.isActive(&_repeatModeOvl)) {
    notifyDisplayActivity(millis());
    return;
  }

  const bool nav_open = _surfaces.contains(&_navigation);
  const bool quick_ping_open = _surfaces.contains(&_quickPingOverlay);
  const lv_coord_t w = current_display_width();
  const lv_coord_t h = current_display_height();

  // Open from an edge, dismiss in the direction the pane leaves the screen.
  // Pane dismissal owns the reverse swipe before normal page navigation does.
  if (nav_open &&
      direction == LV_DIR_LEFT &&
      point_inside_obj(_navigation.root(), start_x, start_y) &&
      (w <= 0 || start_x < (w - HELTEC_TOUCH_ACTION_EDGE_PX))) {
    notifyDisplayActivity(millis());
    deferTouchAction(DeferredTouchAction::CloseNavigation);
    return;
  }

  if (quick_ping_open &&
      direction == LV_DIR_TOP &&
      _quickPingOverlay.hitSwipeDismissRegion(start_x, start_y) &&
      (h <= 0 || start_y < (h - HELTEC_TOUCH_ACTION_EDGE_PX))) {
    notifyDisplayActivity(millis());
    deferTouchAction(DeferredTouchAction::CloseQuickPing);
    return;
  }

  if (is_right_edge_left_swipe(direction, start_x) ||
      is_bottom_edge_up_swipe(direction, start_y)) {
    notifyDisplayActivity(millis());
    return;
  }
#endif

  if (is_left_edge_right_swipe(direction, start_x)) {
    const lv_coord_t h = current_display_height();
    if (start_y <= HELTEC_TOUCH_ACTION_EDGE_PX ||
        (h > 0 && start_y >= (h - HELTEC_TOUCH_ACTION_EDGE_PX))) {
      return;
    }
    notifyDisplayActivity(millis());
#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
    if (!_surfaces.contains(&_navigation)) {
      deferTouchAction(DeferredTouchAction::OpenNavigation);
    }
#else
    if (_surfaces.contains(&_contextMenu)) return;
    const bool nav_open = _surfaces.contains(&_navigation);
    if (nav_open) {
      closeNavigationPane();
    } else if (inputOnActiveScreen()) {
      openNavigationPane();
    }
#endif
    return;
  }

  if (is_top_action_down_swipe(direction, start_x, start_y)) {
    notifyDisplayActivity(millis());
#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
    if (!_surfaces.contains(&_quickPingOverlay)) {
      deferTouchAction(DeferredTouchAction::OpenQuickPing);
    }
#else
    if (_surfaces.contains(&_navigation)) return;
    if (_surfaces.contains(&_contextMenu)) {
      dismissTopContextMenu();
    } else {
      (void)openContextMenu();
    }
#endif
    return;
  }

  if (direction != LV_DIR_LEFT && direction != LV_DIR_RIGHT) return;
  if (_surfaces.contains(&_navigation)) return;
#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
  if (_surfaces.contains(&_quickPingOverlay)) return;
#else
  if (_surfaces.contains(&_contextMenu)) return;
#endif
  const int8_t step = direction == LV_DIR_LEFT ? 1 : -1;
  if (_surfaces.isActive(&_radioParamSyncOvl)) {
    _radioParamSyncOvl.stepSelection(step);
    notifyDisplayActivity(millis());
    return;
  }
  (void)switchAdjacentTile(step);
}

bool UiApp::hitActiveTrackerViewport(lv_coord_t x, lv_coord_t y) const {
#if defined(ENV_INCLUDE_MAP) && ENV_INCLUDE_MAP
  if (static_cast<uint8_t>(eScreenId::Tracker) != activeTileIndex()) return false;
  return _scrTracker.hitMapViewport(x, y);
#else
  (void)x;
  (void)y;
  return false;
#endif
}

#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH

void UiApp::nativeTouchGestureEvent(lv_event_t* e) {
  auto* app = static_cast<UiApp*>(lv_event_get_user_data(e));
  if (app) app->handleNativeTouchGesture(e);
}

void UiApp::handleNativeTouchGesture(lv_event_t* e) {
  if (!_inited || !e || lv_event_get_code(e) != LV_EVENT_GESTURE) return;
  lv_indev_t* indev = lv_event_get_indev(e);
  if (!indev) indev = lv_indev_get_act();
  if (!indev || lv_indev_get_type(indev) != LV_INDEV_TYPE_POINTER) return;

  const lv_dir_t direction = lv_indev_get_gesture_dir(indev);
  if (direction != LV_DIR_LEFT && direction != LV_DIR_RIGHT &&
      direction != LV_DIR_TOP && direction != LV_DIR_BOTTOM) {
    return;
  }

  int16_t start_x = 0;
  int16_t start_y = 0;
  if (!heltec::meshcore::dal::touch_port::getPressStart(start_x, start_y)) {
    lv_point_t point{};
    lv_indev_get_point(indev, &point);
    start_x = point.x;
    start_y = point.y;
  }

  if (_surfaces.isActive(&_radioParamSyncOvl) &&
      _radioParamSyncOvl.hitRoller(start_x, start_y)) {
    return;
  }
  if (_surfaces.isActive(&_repeatModeOvl) &&
      _repeatModeOvl.hitRoller(start_x, start_y)) {
    return;
  }
  if (_surfaces.isActive(&_quickPingOverlay) &&
      (direction == LV_DIR_TOP || direction == LV_DIR_BOTTOM) &&
      _quickPingOverlay.hitVerticalSwipeControl(start_x, start_y)) {
    return;
  }
#if defined(ENV_INCLUDE_MAP) && ENV_INCLUDE_MAP
  if (inputOnActiveScreen() && hitActiveTrackerViewport(start_x, start_y)) return;
#endif

  // Native LVGL scrolling wins before LV_EVENT_GESTURE is emitted. Once a
  // non-scroll gesture reaches here, consume its release so it cannot become
  // a click on the object where the swipe started.
  lv_indev_wait_release(indev);
  lv_event_stop_processing(e);
  lv_event_stop_bubbling(e);
  onTouchSwipe(direction, start_x, start_y);
}

#endif

#if defined(HELTEC_TOUCH_GESTURE_INPUT) && HELTEC_TOUCH_GESTURE_INPUT

bool UiApp::touchGestureBlockLongPress(int16_t x, int16_t y) {
  // An open dropdown owns the pointer until release. Converting a hold into a
  // synthetic ENTER key suppresses that release and prevents item selection.
  if (point_inside_any_open_dropdown_list(x, y)) return true;
#if defined(ENV_INCLUDE_MAP) && ENV_INCLUDE_MAP
  if (x <= HELTEC_TOUCH_EDGE_PX || y <= HELTEC_TOUCH_EDGE_PX) return false;
  auto& app = UiApp::instance();
  if (!app.inputOnActiveScreen()) return false;
  return app.hitActiveTrackerViewport(x, y);
#else
  return false;
#endif
}

#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH

bool UiApp::touchGestureRawPointerPassthrough(int16_t x, int16_t y) {
  auto& app = UiApp::instance();
  return app._surfaces.isActive(&app._radioParamSyncOvl) &&
         app._radioParamSyncOvl.hitRoller(x, y);
}

bool UiApp::touchGestureBlockQuickPingDoubleTap(int16_t x, int16_t y) {
  auto& app = UiApp::instance();
  return app._surfaces.isActive(&app._quickPingOverlay) &&
         point_inside_obj(app._quickPingOverlay.root(), x, y);
}

#endif
#endif

}  // namespace heltec::meshcore::ui
