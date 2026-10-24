#pragma once

#include "app/PowerMgr.hpp"
#include "ui/theme/UiTheme.hpp"
#include "ui/core/AppStateDispatcher.hpp"
#include "ui/core/BizFacade.hpp"
#include "ui/core/Capabilities.hpp"
#include "ui/core/HtEvent.h"
#include "ui/core/Surface.hpp"
#include "ui/core/SurfaceManager.hpp"
#include "ui/widgets/TopPane.hpp"
#if HELTEC_UI_HAS_TOUCH
#include "ui/navigation/NavigationPane.hpp"
#else
#include "ui/navigation/RadialNavigator.hpp"
#endif
#include "heltec/ui/Images.h"
#include "ui/screens/HomeScreen.hpp"
#include "ui/screens/RadioScreen.hpp"
#include "ui/screens/RecentScreen.hpp"
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
#include "ui/screens/CompassScreen.hpp"
#endif
#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
#include "ui/screens/FindFriendScreen.hpp"
#endif
#if ENV_INCLUDE_GPS
#include "ui/screens/GpsScreen.hpp"
#endif
#if defined(ENV_INCLUDE_MAP) && ENV_INCLUDE_MAP
#include "ui/screens/TrackerScreen.hpp"
#endif
#include "ui/overlays/AlertOverlay.hpp"
#include "ui/overlays/ConfirmOverlay.hpp"
#if HELTEC_UI_ENABLE_KEYBOARD
#include "ui/overlays/KeyboardOverlay.hpp"
#endif
#include "ui/overlays/PreviewOverlay.hpp"
#include "ui/overlays/RadioParamSyncOverlay.hpp"
#include "ui/overlays/RepeatModeOverlay.hpp"
#include "ui/overlays/SendMessageOverlay.hpp"
#include "ui/screens/SystemScreen.hpp"
#if HELTEC_UI_HAS_TOUCH
#include "ui/overlays/QuickPingOverlay.hpp"
#else
#include "ui/menus/ContextMenu.hpp"
#endif
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
#include "ui/overlays/CalibrationOverlay.hpp"
#endif

namespace Heltec::MeshCore::Ui {

class AbstractScreen;

class UiApp final {
  public:
    UiApp(Biz::IBizFacade &biz, Heltec::MeshCore::Power::PowerMgr &power);
    static UiApp &instance();

    void init();
    void tick();
    bool isReady() const {
        return _inited;
    }

    bool acceptsKeyboardCycle() const
    {
#if HELTEC_UI_ENABLE_KEYBOARD
        if (_surfaces.isActive(&_keyboardOvl))
        {
            return true;
        }
#endif
#if HELTEC_UI_HAS_TOUCH
        return _surfaces.isActive(&_quickPingOverlay) &&
               _quickPingOverlay.acceptsKeyboardCycle();
#else
        return false;
#endif
    }

    bool openSendMessageOverlay();
    void closeSendMessageOverlay();
    void openRadioParamSyncOverlay();
    bool openActionPane();
    bool closeActionPane();
    bool isActionPaneVisible() const;
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
    bool openCalibrationOverlay();
#endif
    void openWaypointKeyboard();

    void handlePowerChanged(Heltec::MeshCore::Power::PowerChangeMask changes,
                            const Heltec::MeshCore::Power::PowerSnapshot &snapshot);

    void setDisplayAutoOffMs(uint32_t ms);

  private:
    friend class BacklightPolicy;
#if HELTEC_UI_HAS_TOUCH
    using AppNavigation = NavigationPane;
#else
    using AppNavigation = RadialNavigator;
#endif

    static constexpr uint8_t kNoScheduledTile = 0xFF;

    void openPreviewOverlay(uint8_t unread, uint32_t received_ms, const char *origin,
                            const char *text);
    void openAlertOverlay(const char *text);

    void notifyDisplayActivity(uint32_t now_ms);
    bool isDisplayOn() const;
    void onBacklightTurnedOn();
    void ensureTileKeypadFocus();

#if !defined(HELTEC_UI_HAS_TOUCH) || !HELTEC_UI_HAS_TOUCH
    bool openContextMenu();
    void closeContextMenu();
    void ensureContextMenusRegistered();
    bool registerRadioContextMenu();
    bool registerCompassContextMenu();
#endif
    bool initOverlay();
    bool initScreens(_lv_obj_t *content);
    bool initNavigationPane(_lv_obj_t *parent);
    bool initTimers();
    void bindFrameEvents();
    void handleFrameEvent(lv_event_t *e);
    void handleAppStateEvent(const AppStateEvent &event);
    void closeRadioParamSyncOverlay();
    void openRepeatModeOverlay();
    void closeRepeatModeOverlay();
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
    void closeCalibrationOverlay();
#endif
    void presentMessageKeyboard();
    void closeKeyboardOverlay();

#if HELTEC_UI_HAS_TOUCH
    bool hitActiveTrackerViewport(lv_coord_t x, lv_coord_t y) const;
#if defined(HELTEC_TOUCH_GESTURE) && HELTEC_TOUCH_GESTURE
    static bool touchGestureBlockLongPress(int16_t x, int16_t y);
    static bool touchGestureRawPointerPassthrough(int16_t x, int16_t y);
    static bool touchGestureBlockQuickPingDoubleTap(int16_t x, int16_t y);
#endif
#endif
    uint8_t activeTileIndex() const;
    void setTopPaneTitle(const char *title);
    void refreshTopPaneStatus(bool force = false);
    bool inputOnActiveScreen() const;
    void notifyNavActivity(uint32_t now_ms);
    void openNavigationPane();
    void closeNavigationPane();
    AbstractScreen *activeScreen() const;
    AbstractScreen *screenAt(uint8_t index) const;
    AbstractScreen *resolveActiveScreen() const;
    void bindScreen(AbstractScreen *scr);
    void activateActiveScreen();
#if HELTEC_UI_HAS_TOUCH
    static void nativeTouchGestureEvent(lv_event_t *e);
    void handleNativeTouchGesture(lv_event_t *e);

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
    static void deferredTouchActionTimerCb(lv_timer_t *timer);
    /** Touch swipe: edge gestures first, then page switching. */
    void onTouchSwipe(lv_dir_t direction, int16_t start_x, int16_t start_y);
#endif
    void restartNavigationAutoCommitTimer();
    void stopNavigationAutoCommitTimer();
    void handleNavigationAutoCommitTimeout();
    static void navigationAutoCommitTimerCb(lv_timer_t *timer);
    void syncDisplayTimeoutInhibit(uint32_t now_ms);

    void closeNavigationImmediate();
    bool selectTile(uint8_t tile_idx);
    void scheduleNavTileCommit(uint8_t tile_idx);
    void previewNavTile(uint8_t tile_idx);
    void onTileActiveChanged();
    bool switchAdjacentTile(int8_t dir);
    Biz::IBizFacade &_biz;
    Heltec::MeshCore::Power::PowerMgr &_power;
    AppStateUiDispatcher _app_state_dispatcher;
    SurfaceManager _surfaces;
    TopPane _top_pane;
    uint32_t _top_pane_minute = UINT32_MAX;
    AppNavigation _navigation;
    uint32_t _nav_last_activity_ms = 0;
    uint8_t _scheduled_nav_tile = kNoScheduledTile;
    lv_timer_t *_nav_auto_commit_timer = nullptr;

    HomeScreen _scrHome;
    RecentScreen _scrRecent;
    RadioScreen _scrRadio;
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
    CompassScreen _scrCompass;
#endif
#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
    FindFriendScreen _scrFindFriend;
#endif
#if ENV_INCLUDE_GPS
    GPSScreen _scrGPS;
#endif
#if defined(ENV_INCLUDE_MAP) && ENV_INCLUDE_MAP
    TrackerScreen _scrTracker;
#endif
    SystemScreen _scrSystem;

#if HELTEC_UI_HAS_TOUCH
    QuickPingOverlay _quickPingOverlay;
#endif

    lv_obj_t *_layerOverlay = nullptr;

    PreviewOverlay _previewOvl;
    AlertOverlay _alertOvl;
    ConfirmOverlay _confirmOvl;
    lv_obj_t *_confirm_reply = nullptr;
    RadioParamSyncOverlay _radioParamSyncOvl;
    RepeatModeOverlay _repeatModeOvl;
    SendMessageOverlay _sendMessageOvl;
#if HELTEC_UI_ENABLE_KEYBOARD
    KeyboardOverlay _keyboardOvl;
    lv_obj_t *_keyboard_reply = nullptr;
#endif
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
    CalibrationOverlay _calibrationOvl;
#endif
#if !defined(HELTEC_UI_HAS_TOUCH) || !HELTEC_UI_HAS_TOUCH
    ContextMenu _contextMenu;
    bool _context_menus_registered = false;
#endif
#if HELTEC_UI_HAS_TOUCH
    DeferredTouchAction _deferred_touch_action = DeferredTouchAction::None;
    lv_timer_t *_deferred_touch_timer = nullptr;
#endif
    bool _inited = false;
    bool _frame_events_bound = false;
    lv_obj_t *_frame_root = nullptr;
    lv_obj_t *_root = nullptr;
    lv_obj_t *_tileview = nullptr;
};

} // namespace Heltec::MeshCore::Ui
