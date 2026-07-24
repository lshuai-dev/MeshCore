#include "ui/app/ui_app.hpp"

#include "heltec/drivers/display/display_port.hpp"
#include "ui/core/screen_id.hpp"
#include "ui/core/ui_deferred_queue.hpp"
#include "ui/core/ui_events.h"
#include "ui/theme/ui_theme_metrics.hpp"
#include <Arduino.h>

namespace heltec::meshcore::ui {

void UiApp::openNavigationPane() {
  if (!_inited || _surfaces.contains(&_navigation)) return;

  reconcileInput();
  if (_surfaces.contains(&_navigation)) return;

  _navigation.setSelectedIndex(activeTileIndex());
  notifyNavActivity(millis());
  (void)_surfaces.present(&_navigation);
}

void UiApp::scheduleNavTileCommit(uint8_t tile_idx) {
  const bool nav_active = _surfaces.isActive(&_navigation);
  const bool transitioning = _navigation.isTransitioning();
  if (!nav_active || transitioning) return;
  if (!_tileview || tile_idx >= kScreenCnt) return;
  if (kNoScheduledTile != _scheduled_nav_tile) return;
  stopNavigationAutoCommitTimer();
  _scheduled_nav_tile = tile_idx;
  if (!ui_defer(+[](void* user_data) {
        auto* app = static_cast<UiApp*>(user_data);
        if (!app || kNoScheduledTile == app->_scheduled_nav_tile) return;

        const uint8_t pending_tile = app->_scheduled_nav_tile;
        app->_scheduled_nav_tile = kNoScheduledTile;
        if (!app->_tileview || pending_tile >= kScreenCnt) return;

        (void)app->selectTile(pending_tile);
        if (AbstractScreen* scr = app->screenAt(pending_tile)) {
          app->bindScreen(scr);
          app->setTopPaneTitle(scr->title());
        }
        app->closeNavigationPane();
      }, this)) {
    _scheduled_nav_tile = kNoScheduledTile;
  }
}

bool UiApp::switchAdjacentTile(int8_t dir) {
#if defined(UI_NAVIGATION_GRID) && UI_NAVIGATION_GRID
  (void)dir;
  return false;
#else
  if (!_tileview || 0 == dir) return false;

  const uint8_t cur = activeTileIndex();
  int next = ((int)cur + (int)dir) % kScreenCnt;
  if (next < 0) next += kScreenCnt;
  if (next < 0 || next >= (int)kScreenCnt) return false;

  if (!selectTile(static_cast<uint8_t>(next))) return false;
  if (lv_obj_t* scr_obj = lv_scr_act()) lv_obj_invalidate(scr_obj);

  if (AbstractScreen* scr = screenAt((uint8_t)next)) {
    bindScreen(scr);
    setTopPaneTitle(scr->title());
  }
  _navigation.setSelectedIndex((uint8_t)next);
  notifyDisplayActivity(millis());
  return true;
#endif
}

void UiApp::closeNavigationPane() {
  if (!_surfaces.contains(&_navigation)) return;
  _nav_last_activity_ms = 0;
  stopNavigationAutoCommitTimer();
  (void)_surfaces.dismissBranch(&_navigation);
  if (AbstractScreen* scr = activeScreen()) {
    setTopPaneTitle(scr->title());
  }
}

bool UiApp::initNavigationPane(_lv_obj_t* parent) {
  if (!_navigation.init(parent)) return false;

  struct NavSlot {
    eScreenId id;
    AbstractScreen* scr;
    bool footer;
  };
#if defined(UI_NAVIGATION_GRID) && UI_NAVIGATION_GRID
  const NavSlot slots[] = {
      {eScreenId::Home, &_scrHome, false},
      {eScreenId::Radio, &_scrRadio, false},
      {eScreenId::Recent, &_scrRecent, false},
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
      {eScreenId::Compass, &_scrCompass, false},
#else
      {eScreenId::GPS, &_scrGPS, false},
#endif
#if defined(ENV_INCLUDE_MAP) && ENV_INCLUDE_MAP
      {eScreenId::Tracker, &_scrTracker, false},
#endif
      {eScreenId::System, &_scrSystem, false},
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
      {eScreenId::FindFriend, &_scrFindFriend, true},
#endif
  };
#else
  const NavSlot slots[] = {
      {eScreenId::Home, &_scrHome, false},
      {eScreenId::Recent, &_scrRecent, false},
      {eScreenId::Radio, &_scrRadio, false},
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
      {eScreenId::Compass, &_scrCompass, false},
      {eScreenId::FindFriend, &_scrFindFriend, false},
#endif
      {eScreenId::GPS, &_scrGPS, false},
#if defined(ENV_INCLUDE_MAP) && ENV_INCLUDE_MAP
      {eScreenId::Tracker, &_scrTracker, false},
#endif
      {eScreenId::System, &_scrSystem, false},
  };
#endif
  UiNavigationItem items[sizeof(slots) / sizeof(slots[0])] = {};
  uint8_t item_count = 0;
  for (const NavSlot& slot : slots) {
    UiNavigationItem& item = items[item_count++];
    item.screen_index = static_cast<uint8_t>(slot.id);
    item.label = slot.scr->title();
    item.icon = slot.scr->icon();
    item.footer = slot.footer;
  }
  _navigation.configure(items, item_count);

  _navigation.setSelectedIndex(activeTileIndex());

  return true;
}

void UiApp::notifyNavActivity(uint32_t now_ms) {
  _nav_last_activity_ms = now_ms;
  restartNavigationAutoCommitTimer();
  notifyDisplayActivity(now_ms);
}

void UiApp::ensureTileKeypadFocus() {
  _scheduled_nav_tile = kNoScheduledTile;
  _nav_last_activity_ms = 0;
  stopNavigationAutoCommitTimer();

  if (_surfaces.contains(&_navigation)) {
    closeNavigationPane();
    return;
  }
  if (_surfaces.contains(&_navigation)) {
    closeNavigationImmediate();
  }
#if !defined(HELTEC_V4_R8_TFT) || !defined(HELTEC_HAS_TOUCH) || !HELTEC_HAS_TOUCH
  if (_surfaces.contains(&_contextMenu)) dismissContextMenuStack();
#endif
  bindScreen(resolveActiveScreen());
  if (AbstractScreen* scr = activeScreen()) {
    setTopPaneTitle(scr->title());
  }
}

bool UiApp::selectTile(uint8_t tile_idx) {
  if (!_tileview || tile_idx >= kScreenCnt) return false;
  lv_obj_t* const tile = lv_obj_get_child(_tileview, tile_idx);
  if (!tile) return false;
#if !defined(UI_NAVIGATION_GRID) || !UI_NAVIGATION_GRID
  lv_obj_set_scroll_dir(_tileview, LV_DIR_HOR);
#endif
  lv_obj_set_tile(_tileview, tile, LV_ANIM_OFF);
  lv_obj_update_layout(_tileview);
  return true;
}

void UiApp::onTileActiveChanged() {
  if (AbstractScreen* scr = activeScreen()) {
    setTopPaneTitle(scr->title());
    if (!_surfaces.contains(&_navigation)) {
      bindScreen(resolveActiveScreen());
    }
  }
}

void UiApp::closeNavigationImmediate() {
  _nav_last_activity_ms = 0;
  stopNavigationAutoCommitTimer();
  (void)_surfaces.dismissBranch(&_navigation);
}

void UiApp::restartNavigationAutoCommitTimer() {
  if (!_nav_auto_commit_timer) return;
  const uint16_t delay_ms = ui_navigation_metrics(_navigation.root()).auto_hide_ms;
  if (!delay_ms || !_nav_last_activity_ms) {
    stopNavigationAutoCommitTimer();
    return;
  }
  lv_timer_set_period(_nav_auto_commit_timer, delay_ms);
  lv_timer_set_repeat_count(_nav_auto_commit_timer, -1);
  lv_timer_reset(_nav_auto_commit_timer);
  lv_timer_resume(_nav_auto_commit_timer);
}

void UiApp::stopNavigationAutoCommitTimer() {
  if (_nav_auto_commit_timer) lv_timer_pause(_nav_auto_commit_timer);
}

uint8_t UiApp::activeTileIndex() const {
  lv_obj_t* tile = _tileview ? lv_tileview_get_tile_act(_tileview) : nullptr;
  return tile ? static_cast<uint8_t>(lv_obj_get_index(tile)) : 0;
}

void UiApp::handleNavigationAutoCommitTimeout() {
  stopNavigationAutoCommitTimer();
  if (!heltec::meshcore::dal::display_port::isBacklightOn()) return;
  if (!_surfaces.isActive(&_navigation) || _navigation.isTransitioning()) return;
  const uint16_t auto_hide_ms = ui_navigation_metrics(_navigation.root()).auto_hide_ms;
  if (!auto_hide_ms || !_nav_last_activity_ms) return;
  if ((millis() - _nav_last_activity_ms) < auto_hide_ms) {
    restartNavigationAutoCommitTimer();
    return;
  }

  scheduleNavTileCommit(_navigation.focusedIndex());
  _nav_last_activity_ms = 0;
}

void UiApp::navigationAutoCommitTimerCb(lv_timer_t* timer) {
  auto* app = timer ? static_cast<UiApp*>(timer->user_data) : nullptr;
  if (app) app->handleNavigationAutoCommitTimeout();
}

}  // namespace heltec::meshcore::ui
