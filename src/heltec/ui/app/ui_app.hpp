#pragma once

#include <stdint.h>
#include <lvgl.h>

#include "app/power_mgr.hpp"
#include "ui/app/ui_theme.hpp"
#include "ui/core/biz_facade.hpp"
#include "ui/core/input_host.hpp"
#include "ui/core/app_state_ui_dispatcher.hpp"
#include "ui/core/ui_surface.hpp"
#include "ui/core/ui_host.hpp"
#include "ui/core/ui_events.h"
#include "ui/core/surface_manager.hpp"
#include "ui/widgets/top_pane.hpp"

#if defined(UI_NAVIGATION_GRID) && UI_NAVIGATION_GRID
#include "ui/navigation/navigation_pane.hpp"
#else
#include "ui/navigation/radial_navigator.hpp"
#endif

#include "heltec/ui/images.h"
#include "ui/screens/home_screen.hpp"
#include "ui/screens/recent_screen.hpp"
#include "ui/screens/radio_screen.hpp"
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
#include "ui/screens/compass_screen.hpp"
#include "ui/screens/find_friend_screen.hpp"
#endif
#include "ui/screens/gps_screen.hpp"
#if defined(ENV_INCLUDE_MAP) && ENV_INCLUDE_MAP
#include "ui/screens/tracker_screen.hpp"
#endif
#include "ui/screens/system_screen.hpp"

#include "ui/overlays/preview_overlay.hpp"
#include "ui/overlays/alert_overlay.hpp"
#include "ui/overlays/confirm_overlay.hpp"
#include "ui/overlays/radio_pram_sync_overlay.hpp"
#include "ui/overlays/repeat_mode_overlay.hpp"
#include "ui/overlays/send_message_overlay.hpp"
#include "ui/overlays/keyboard_overlay.hpp"
#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
#include "ui/overlays/quick_ping_overlay.hpp"
#else
#include "ui/menus/cascading_menu.hpp"
#include "ui/menus/context_menu.hpp"
#endif
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
#include "ui/overlays/calibration_overlay.hpp"
#endif

namespace heltec::meshcore::ui {

class AbstractScreen;

class UiApp final : public IUiHost, public InputHost {
 public:
  UiApp(biz::IBizFacade& biz, heltec::meshcore::power::PowerMgr& power);
  static UiApp& instance();

  void init();
  void tick();
  bool isReady() const override { return _inited; }

  bool openSendMessageOverlay();
  void closeSendMessageOverlay();
  void openRadioParamSyncOverlay();
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  bool openCalibrationOverlay();
#endif
  void openWaypointKeyboard();
  void closePreviewOverlay() override;

  void handlePowerChanged(heltec::meshcore::power::PowerChangeMask changes,
                          const heltec::meshcore::power::PowerSnapshot& snapshot);

  void setDisplayAutoOffMs(uint32_t ms);

 private:
#if defined(UI_NAVIGATION_GRID) && UI_NAVIGATION_GRID
  using AppNavigation = NavigationPane;
#else
  using AppNavigation = RadialNavigator;
#endif

  static constexpr uint8_t kNoScheduledTile = 0xFF;

  void openPreviewOverlay(uint8_t unread, uint32_t received_ms,
                          const char* origin, const char* text) override;
  void openAlertOverlay(const char* text) override;
  void closeAlertOverlay() override;

  void notifyDisplayActivity(uint32_t now_ms) override;
  bool isDisplayOn() const override;
  void toggleDisplay(uint32_t now_ms) override;
  void onBacklightTurnedOn() override;
  void reconcileInput() override;
  void ensureTileKeypadFocus() override;
  lv_obj_t* frameRoot() const override { return _frame_root; }

#if !defined(HELTEC_V4_R8_TFT) || !defined(HELTEC_HAS_TOUCH) || !HELTEC_HAS_TOUCH
  bool openContextMenu();
  void ensureContextMenusRegistered();
  bool registerRadioContextMenu();
  bool registerCompassContextMenu();
#endif
  bool initOverlay();
  bool initScreens(_lv_obj_t* content);
  bool initNavigationPane(_lv_obj_t* parent);
  bool initTimers();
  void bindFrameEvents();
  void handleFrameEvent(lv_event_t* e);
  void handleAppStateEvent(const AppStateEvent& event);
#if !defined(HELTEC_V4_R8_TFT) || !defined(HELTEC_HAS_TOUCH) || !HELTEC_HAS_TOUCH
  void dismissTopContextMenu();
  void dismissContextMenuStack();
#endif
  void closeRadioParamSyncOverlay();
  void openRepeatModeOverlay();
  void closeRepeatModeOverlay();
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  void closeCalibrationOverlay();
#endif
  void presentMessageKeyboard();
  void closeKeyboardOverlay();

  bool hitActiveTrackerViewport(lv_coord_t x, lv_coord_t y) const;
#if defined(HELTEC_TOUCH_GESTURE_INPUT) && HELTEC_TOUCH_GESTURE_INPUT
  static bool touchGestureBlockLongPress(int16_t x, int16_t y);
#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
  static bool touchGestureRawPointerPassthrough(int16_t x, int16_t y);
  static bool touchGestureBlockQuickPingDoubleTap(int16_t x, int16_t y);
#endif
#endif
  uint8_t activeTileIndex() const;
  void setTopPaneTitle(const char* title);
  bool inputOnActiveScreen() const;
  void notifyNavActivity(uint32_t now_ms);
  void openNavigationPane();
  void closeNavigationPane();
  AbstractScreen* activeScreen() const;
  AbstractScreen* screenAt(uint8_t index) const;
  AbstractScreen* resolveActiveScreen() const;
  void bindScreen(AbstractScreen* scr);
  void activateActiveScreen();
#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
  static void nativeTouchGestureEvent(lv_event_t* e);
  void handleNativeTouchGesture(lv_event_t* e);

  enum class DeferredTouchAction : uint8_t {
    None = 0,
    OpenNavigation,
    CloseNavigation,
    OpenQuickPing,
    CloseQuickPing,
  };

  void openNavigationPaneFromEdgeSwipe();
  void closeNavigationPaneFromSwipe();
  void openQuickPingFromTopSwipe();
  void closeQuickPingFromSwipe();
  void deferTouchAction(DeferredTouchAction action);
  void runDeferredTouchAction();
  static void deferredTouchActionTimerCb(lv_timer_t* timer);
#endif
  /** Touch swipe: edge gestures first, then page switching. */
  void onTouchSwipe(lv_dir_t direction, int16_t start_x, int16_t start_y);
  void restartNavigationAutoCommitTimer();
  void stopNavigationAutoCommitTimer();
  void handleNavigationAutoCommitTimeout();
  static void navigationAutoCommitTimerCb(lv_timer_t* timer);
  void syncDisplayTimeoutInhibit(uint32_t now_ms);

  void closeNavigationImmediate();
  bool selectTile(uint8_t tile_idx);
  void scheduleNavTileCommit(uint8_t tile_idx);
  void previewNavTile(uint8_t tile_idx);
  void onTileActiveChanged();
  bool switchAdjacentTile(int8_t dir);
#if defined(HELTEC_TOPBAR_TOUCH_SHELL) && HELTEC_TOPBAR_TOUCH_SHELL
  void onTopPaneShortPress();
  void onTopPaneLongPress();
#endif

  biz::IBizFacade& _biz;
  heltec::meshcore::power::PowerMgr& _power;
  AppStateUiDispatcher _app_state_dispatcher;
  SurfaceManager _surfaces;
  TopPane _top_pane;
  AppNavigation _navigation;
  uint32_t _nav_last_activity_ms = 0;
  uint8_t _scheduled_nav_tile = kNoScheduledTile;
  lv_timer_t* _nav_auto_commit_timer = nullptr;

  HomeScreen _scrHome;
  RecentScreen _scrRecent;
  RadioScreen _scrRadio;
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  CompassScreen _scrCompass;
  FindFriendScreen _scrFindFriend;
#endif
  GPSScreen _scrGPS;
#if defined(ENV_INCLUDE_MAP) && ENV_INCLUDE_MAP
  TrackerScreen _scrTracker;
#endif
  SystemScreen _scrSystem;

#if !defined(HELTEC_V4_R8_TFT) || !defined(HELTEC_HAS_TOUCH) || !HELTEC_HAS_TOUCH
  CascadingMenu _ctxRadioMenu;
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  CascadingMenu _ctxCompassMenu;
#endif
#endif
#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
  QuickPingOverlay _quickPingOverlay;
#endif

  lv_obj_t* _layerOverlay = nullptr;

  PreviewOverlay _previewOvl;
  AlertOverlay _alertOvl;
  ConfirmOverlay _confirmOvl;
  RadioParamSyncOverlay _radioParamSyncOvl;
  RepeatModeOverlay _repeatModeOvl;
  SendMessageOverlay _sendMessageOvl;
  KeyboardOverlay _keyboardOvl;
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  CalibrationOverlay _calibrationOvl;
#endif
#if !defined(HELTEC_V4_R8_TFT) || !defined(HELTEC_HAS_TOUCH) || !HELTEC_HAS_TOUCH
  ContextMenu _contextMenu;
  bool _context_menus_registered = false;
#endif
#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
  DeferredTouchAction _deferred_touch_action = DeferredTouchAction::None;
  lv_timer_t* _deferred_touch_timer = nullptr;
#endif
  bool _inited = false;
  bool _frame_events_bound = false;
  lv_obj_t* _frame_root = nullptr;
  lv_obj_t* _root = nullptr;
  lv_obj_t* _tileview = nullptr;
};

}  // namespace heltec::meshcore::ui
