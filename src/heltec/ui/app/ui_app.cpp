#include "ui/app/ui_app.hpp"
#include "ui/app/ui_app_ids.hpp"
#include "heltec/ui/images.h"
#include "ui/app/ui_theme.hpp"
#include "ui/core/ui_task.hpp"
#include "ui/menus/cascading_menu.hpp"
#include "ui/menus/context_menu.hpp"
#include "ui/overlays/splash_overlay.hpp"
#include "ui/core/ht_meta_data.hpp"
#include "ui/theme/ui_widget_theme.hpp"
#include "ui/core/backlight_policy.hpp"
#include "ui/core/input_pipeline.hpp"
#include "ui/core/screen_id.hpp"
#include "ui/core/ui_events.h"
#include "ui/core/ui_surface.hpp"
#include "ui/core/app_state_notifier.hpp"
#include "ui/core/ui_deferred_queue.hpp"
#include "ui/core/ui_motion_scheduler.hpp"
#include "ui/navigation/ui_navigator.hpp"
#include "ui/theme/ui_theme_metrics.hpp"
#include "heltec/drivers/display/display_port.hpp"
#include "heltec/drivers/input/touch_input.hpp"
#include "heltec/drivers/input/touch_port.hpp"
#include <Arduino.h>
#include <cstring>
#include <lvgl.h>

#if defined(MESH_DEBUG) && MESH_DEBUG
#define UI_ALERT_LOG(fmt, ...) \
  do { \
    Serial.printf("[alert] " fmt "\n", ##__VA_ARGS__); \
    Serial.flush(); \
  } while (0)
#else
#define UI_ALERT_LOG(fmt, ...) ((void)0)
#endif

namespace heltec::meshcore::ui {

namespace {

UiApp* s_ui_app_instance = nullptr;

static void relayout_tileview_tiles(lv_obj_t* tileview) {
  if (!tileview) return;

  // lv_tileview positions tiles at creation time; boot-time flex layout may still be 0x0.
  lv_obj_update_layout(tileview);
  lv_coord_t tile_w = lv_obj_get_content_width(tileview);
  lv_coord_t tile_h = lv_obj_get_content_height(tileview);
  if (tile_w <= 0) tile_w = lv_obj_get_width(tileview);
  if (tile_h <= 0) tile_h = lv_obj_get_height(tileview);
  if (tile_w <= 0 || tile_h <= 0) return;

  const uint32_t child_count = lv_obj_get_child_cnt(tileview);
  for (uint32_t i = 0; i < child_count; ++i) {
    lv_obj_t* tile = lv_obj_get_child(tileview, i);
    if (!tile) continue;
    lv_obj_set_size(tile, tile_w, tile_h);
    lv_obj_set_pos(tile, static_cast<lv_coord_t>(i) * tile_w, 0);
  }

  if (lv_obj_t* active_tile = lv_tileview_get_tile_act(tileview)) {
    lv_obj_set_tile(tileview, active_tile, LV_ANIM_OFF);
  }
}

static void tileview_size_changed_cb(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_SIZE_CHANGED) return;
  relayout_tileview_tiles(lv_event_get_target(e));
}

}  // namespace

UiApp& UiApp::instance() {
  LV_ASSERT(s_ui_app_instance != nullptr);
  return *s_ui_app_instance;
}

UiApp::UiApp(biz::IBizFacade& biz)
    : _biz(biz),
      _top_pane(),
      _navigation(_biz),
      _scrHome(_biz, "Home", &icon_home_img),
      _scrRecent(_biz, "Recent", &icon_recent_img),
      _scrRadio(_biz, "Radio", &icon_radio_img),
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
      _scrCompass(_biz, "Compass", &icon_compass_img),
      _scrFindFriend(_biz, "Find Friend", &icon_findfriend_img),
#endif
      _scrGPS(_biz, "GPS", &icon_gps_img),
#if defined(ENV_INCLUDE_MAP) && ENV_INCLUDE_MAP
      _scrTracker(_biz, "Tracker", &icon_map_img),
#endif
      _scrSystem(_biz, "System", &icon_system_img)
#if !defined(HELTEC_V4_R8_TFT) || !defined(HELTEC_HAS_TOUCH) || !HELTEC_HAS_TOUCH
      ,
      _ctxRadioMenu(_biz)
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
      ,
      _ctxCompassMenu(_biz)
#endif
#endif
#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
      ,
      _quickPingOverlay(_biz)
#endif
      ,
      _previewOvl(_biz),
      _alertOvl(_biz),
      _radioParamSyncOvl(_biz),
      _choicePickerOvl(_biz),
      _sendMessageOvl(_biz),
      _keyboardOvl(_biz)
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
      ,
      _calibrationOvl(_biz)
#endif
#if !defined(HELTEC_V4_R8_TFT) || !defined(HELTEC_HAS_TOUCH) || !HELTEC_HAS_TOUCH
      ,
      _contextMenu(_biz)
#endif
      {
  LV_ASSERT(s_ui_app_instance == nullptr);
  s_ui_app_instance = this;
}

AbstractScreen* UiApp::activeScreen() const {
  lv_obj_t* tile = _tileview ? lv_tileview_get_tile_act(_tileview) : nullptr;
  return tile ? static_cast<AbstractScreen*>(ht_user_data(tile)) : nullptr;
}

AbstractScreen* UiApp::screenAt(uint8_t index) const {
  if (!_tileview) return nullptr;
  lv_obj_t* tile = lv_obj_get_child(_tileview, index);
  return tile ? static_cast<AbstractScreen*>(ht_user_data(tile)) : nullptr;
}

AbstractScreen* UiApp::resolveActiveScreen() const {
  if (AbstractScreen* scr = activeScreen()) return scr;
  if (AbstractScreen* scr = screenAt(activeTileIndex())) return scr;
  return screenAt(0);
}

void UiApp::activateActiveScreen() {
  bindScreen(resolveActiveScreen());
}

void UiApp::bindScreen(AbstractScreen* scr) {
  if (!scr) {
    return;
  }
  _surfaces.setRoot(scr);
}

bool UiApp::inputOnActiveScreen() const {
  AbstractScreen* scr = resolveActiveScreen();
  return scr && _surfaces.isActive(static_cast<UiSurface*>(scr));
}

void UiApp::reconcileInput() {
  _surfaces.reconcileVisibility();
  if (_surfaces.contains(&_previewOvl) && !ui_task().isPreviewActive()) {
    (void)_surfaces.dismissBranch(&_previewOvl);
    return;
  }
  if (_surfaces.contains(&_alertOvl) && !ui_task().isAlertActive()) {
    UI_ALERT_LOG("reconcile alert inactive depth=%u active=%p alert=%p",
                 (unsigned)_surfaces.modalDepth(),
                 _surfaces.active(),
                 &_alertOvl);
    const bool dismissed = _surfaces.dismissBranch(&_alertOvl);
    UI_ALERT_LOG("reconcile alert dismiss=%d depth=%u contains=%d",
                 dismissed ? 1 : 0,
                 (unsigned)_surfaces.modalDepth(),
                 _surfaces.contains(&_alertOvl) ? 1 : 0);
    return;
  }
  _surfaces.reconcileFocus();
}

void UiApp::setTopPaneTitle(const char* title) {
  if (title) _top_pane.setTitle(title);
}

#if defined(HELTEC_TOPBAR_TOUCH_SHELL) && HELTEC_TOPBAR_TOUCH_SHELL

void UiApp::onTopPaneShortPress() {
  if (!_inited) return;
#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
  // Once a swipe crosses the gesture threshold the pointer path is hidden from
  // LVGL as a synthetic release, while the finger is still physically down.
  // Do not interpret that release as a top-pane tap; the swipe handler will
  // consume the gesture when the finger is actually released.
  if (heltec::meshcore::dal::touch_port::isPressed()) return;
#endif
  notifyDisplayActivity(millis());
  if (_surfaces.contains(&_navigation)) {
    closeNavigationPane();
    return;
  }
  if (!inputOnActiveScreen()) return;
  openNavigationPane();
}

void UiApp::onTopPaneLongPress() {
  if (!_inited) return;
  notifyDisplayActivity(millis());
}

#endif

void UiApp::init() {
  _inited = false;
  ui_events_init();
  ui_task().attachHost(this);
  _app_state_dispatcher.bindSurfaceManager(_surfaces);
  _app_state_dispatcher.bindGlobalHandler(
      [](void* user_data, const AppStateEvent& event) {
        auto* app = static_cast<UiApp*>(user_data);
        if (app) app->handleAppStateEvent(event);
      },
      this);
  _app_state_dispatcher.bindNotifier(app_state_notifier());

  InputPipeline::init();
#if HELTEC_TOUCH_INPUT
  heltec::meshcore::dal::touch_input::UiHooks touch_hooks{};
  touch_hooks.wake = +[]() {
    auto& app = UiApp::instance();
    if (BacklightPolicy::mode() == BacklightMode::AutoTimeout) {
      const bool was_off = !heltec::meshcore::dal::display_port::isBacklightOn();
      app.notifyDisplayActivity(millis());
      return was_off;
    }
    return !heltec::meshcore::dal::display_port::isBacklightOn();
  };
#if defined(HELTEC_TOUCH_GESTURE_INPUT) && HELTEC_TOUCH_GESTURE_INPUT
#if defined(ENV_INCLUDE_MAP) && ENV_INCLUDE_MAP
  touch_hooks.block_horizontal_swipe = UiApp::touchGestureBlockTrackerViewport;
#if !defined(HELTEC_V4_R8_TFT) || !defined(HELTEC_HAS_TOUCH) || !HELTEC_HAS_TOUCH
  touch_hooks.block_vertical_swipe = UiApp::touchGestureBlockTrackerViewport;
#endif
  touch_hooks.block_long_enter = UiApp::touchGestureBlockTrackerViewport;
#endif
#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
  touch_hooks.block_vertical_swipe = UiApp::touchGestureBlockVerticalSwipe;
  touch_hooks.raw_pointer_passthrough = UiApp::touchGestureRawPointerPassthrough;
  touch_hooks.block_double_tap = UiApp::touchGestureBlockQuickPingDoubleTap;
#endif
  touch_hooks.on_swipe = +[](heltec::meshcore::dal::touch_input::SwipeAxis axis, int8_t dir,
                             int16_t start_x, int16_t start_y) {
    UiApp::instance().onTouchSwipe(static_cast<uint8_t>(axis), dir, start_x, start_y);
  };
#endif
  heltec::meshcore::dal::touch_input::bindUi(touch_hooks);
#endif

  lv_obj_t* scr = lv_scr_act();
  if (!scr) return;

  lv_disp_t* disp = lv_disp_get_default();
  if (!disp) return;
  if (!heltec::meshcore::ui::ui_theme_init(disp)) return;
  if (!_surfaces.createTimers()) return;
  if (!initTimers()) return;

  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(scr, 0, LV_PART_MAIN);
  ui_app_active_screen_apply_theme(scr);
#if defined(HELTEC_V4_R8_TFT)
  lv_obj_t* bg = ht_img_create(scr, meta_id::AppBackgroundImage);
  if (bg) {
    lv_obj_set_pos(bg, 0, 0);
    lv_obj_clear_flag(bg, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_move_background(bg);
    lv_img_set_src(bg, &ui_background_img);
  }
#endif

  _layerOverlay = ht_obj_create(lv_layer_top(), meta_id::AppOverlayLayer);
  if (!_layerOverlay) return;
  lv_obj_set_size(_layerOverlay, lv_pct(100), lv_pct(100));
  lv_obj_set_style_pad_all(_layerOverlay, 0, LV_PART_MAIN);
  lv_obj_clear_flag(_layerOverlay, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

  SplashOverlay splash;
  if (!splash.create(_layerOverlay)) return;
  if (!initOverlay()) return;

  lv_obj_t* layout = ht_obj_create(scr, meta_id::AppFrameLayout);
  if (!layout) return;
  const UiAppFrameMetrics& frame_metrics = ui_app_frame_metrics(layout);
  lv_obj_set_size(layout, lv_pct(100), lv_pct(100));
  lv_obj_set_flex_flow(layout, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(layout, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_top(layout, frame_metrics.frame_margin_top, LV_PART_MAIN);
  lv_obj_set_style_pad_row(layout, 0, LV_PART_MAIN);
  lv_obj_clear_flag(layout, LV_OBJ_FLAG_SCROLLABLE);

  _frame_root = layout;

  if (!_top_pane.create(layout)) return;
  _top_pane.setBatteryMilliVolts(ui_task().batteryMilliVolts());
  lv_obj_set_flex_grow(_top_pane.root(), 0);
#if defined(HELTEC_TOPBAR_TOUCH_SHELL) && HELTEC_TOPBAR_TOUCH_SHELL
  _top_pane.enableTouchShell(
      [](lv_event_t* e) {
        if (lv_event_get_code(e) != LV_EVENT_SHORT_CLICKED) return;
        UiApp::instance().onTopPaneShortPress();
      },
      [](lv_event_t* e) {
        if (lv_event_get_code(e) != LV_EVENT_LONG_PRESSED) return;
        heltec::meshcore::dal::touch_port::requestReleaseBarrier();
        UiApp::instance().onTopPaneLongPress();
      },
      this);
#endif

  lv_obj_t* content = ht_obj_create(layout, meta_id::AppContent);
  if (!content) return;
  lv_obj_set_width(content, lv_pct(100));
  lv_obj_set_flex_grow(content, 1);
  lv_obj_set_style_pad_all(content, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_left(content, frame_metrics.frame_margin_left, LV_PART_MAIN);
  lv_obj_set_style_pad_right(content, frame_metrics.frame_margin_right, LV_PART_MAIN);
  lv_obj_set_style_pad_bottom(content, frame_metrics.frame_margin_bottom, LV_PART_MAIN);
  lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(content, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
  if (!initScreens(content)) return;
  if (!initNavigationPane(_layerOverlay)) {
    return;
  }
  _navigation.setFrameRoot(_frame_root);
  _navigation.setTileView(_tileview);
  bindFrameEvents();
  if (_tileview) {
    lv_obj_add_event_cb(_tileview, [](lv_event_t* e) {
      auto* app = static_cast<UiApp*>(lv_event_get_user_data(e));
      if (app) app->onTileActiveChanged();
    }, LV_EVENT_VALUE_CHANGED, this);
    lv_obj_add_event_cb(_tileview, [](lv_event_t* e) {
      auto* app = static_cast<UiApp*>(lv_event_get_user_data(e));
      if (!app) return;
      const UiEvent* event = ui_event_get(e);
      if (!event) return;
      switch (event->type) {
        case UiEventType::TilePreview: {
          const auto* idx = static_cast<const uint8_t*>(event->payload);
          if (idx) app->previewNavTile(*idx);
          break;
        }
        case UiEventType::TileCommit: {
          const auto* idx = static_cast<const uint8_t*>(event->payload);
          if (idx) app->scheduleNavTileCommit(*idx, true);
          break;
        }
        default:
          break;
      }
    }, ui_event_code(), this);
  }
  _previewOvl.setTarget(_frame_root);
  _alertOvl.setTarget(_frame_root);
  _radioParamSyncOvl.setTarget(_frame_root);
  _choicePickerOvl.setTarget(_frame_root);
  _keyboardOvl.setTarget(_frame_root);
  _sendMessageOvl.setTarget(_frame_root);
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  _calibrationOvl.setTarget(_frame_root);
#endif
#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
  _quickPingOverlay.setTarget(_frame_root);
#else
  _contextMenu.setTarget(_frame_root);
  _ctxRadioMenu.setTarget(_frame_root);
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  _ctxCompassMenu.setTarget(_frame_root);
#endif
#endif

  _display_auto_off_ms = 0;
  _display_last_activity_ms = millis();
  _inited = true;
  if (_tileview) {
    lv_obj_t* tile = lv_tileview_get_tile_act(_tileview);
    if (!tile) {
      tile = lv_obj_get_child(_tileview, 0);
      if (tile) lv_obj_set_tile(_tileview, tile, LV_ANIM_OFF);
    }
  }
  activateActiveScreen();
}

void UiApp::bindFrameEvents() {
  if (!_frame_root || _frame_events_bound) return;
  lv_obj_add_event_cb(_frame_root, [](lv_event_t* e) {
    auto* app = static_cast<UiApp*>(lv_event_get_user_data(e));
    if (app) app->handleFrameEvent(e);
  }, ui_event_code(), this);
  _frame_events_bound = true;
}

void UiApp::handleFrameEvent(lv_event_t* e) {
  const UiEvent* event = ui_event_get(e);
  if (!event) return;

  switch (event->type) {
    case UiEventType::NavOpen:
      openNavigationPane();
      break;
    case UiEventType::NavClose:
      closeNavigationPane();
      break;
    case UiEventType::NavActivity:
      notifyNavActivity(millis());
      break;
    case UiEventType::ActionOpen:
#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
      (void)ui_event_send(_frame_root, UiEventType::QuickPingOpen);
#else
      (void)ui_event_send(_frame_root, UiEventType::ContextOpen);
#endif
      break;
#if !defined(HELTEC_V4_R8_TFT) || !defined(HELTEC_HAS_TOUCH) || !HELTEC_HAS_TOUCH
    case UiEventType::ContextOpen:
      (void)ui_defer(+[](void* user_data) {
        auto* app = static_cast<UiApp*>(user_data);
        if (!app) return;
        if (!app->_inited || app->_surfaces.contains(&app->_contextMenu)) return;
        app->reconcileInput();
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
        if (app->_surfaces.isActive(&app->_calibrationOvl)) {
          app->closeCalibrationOverlay();
        }
#endif
        app->ensureContextMenusRegistered();
        if (!app->_contextMenu.canOpen()) return;
        if (!app->inputOnActiveScreen()) return;
        (void)app->_surfaces.present(&app->_contextMenu);
      }, this);
      break;
    case UiEventType::ContextClose:
      dismissTopContextMenu();
      break;
#endif
    case UiEventType::SendMessageOpen:
      _sendMessageOvl.prepareTarget(static_cast<const UiSendMessageTarget*>(event->payload));
      (void)openSendMessageOverlay();
      break;
    case UiEventType::CalibrationOpen:
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
      (void)openCalibrationOverlay();
#endif
      break;
#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
    case UiEventType::QuickPingOpen:
      if (!_surfaces.contains(&_quickPingOverlay) && inputOnActiveScreen()) {
        (void)_surfaces.present(&_quickPingOverlay, _surfaces.root());
      }
      break;
    case UiEventType::QuickPingClose:
      if (!_quickPingOverlay.requestCloseAnimation()) {
        (void)_surfaces.dismissBranch(&_quickPingOverlay);
      }
      break;
#endif
    case UiEventType::PreviewClose:
      closePreviewOverlay();
      break;
    case UiEventType::AlertClose:
      UI_ALERT_LOG("event close depth=%u active=%p contains=%d",
                   (unsigned)_surfaces.modalDepth(),
                   _surfaces.active(),
                   _surfaces.contains(&_alertOvl) ? 1 : 0);
      closeAlertOverlay();
      break;
    case UiEventType::RadioSyncClose:
      closeRadioParamSyncOverlay();
      break;
    case UiEventType::ChoicePickerOpen: {
      auto* source = static_cast<IChoicePickerSource*>(const_cast<void*>(event->payload));
      if (!source || _surfaces.contains(&_choicePickerOvl) || !inputOnActiveScreen()) break;
      if (_choicePickerOvl.prepare(source)) {
        (void)_surfaces.present(&_choicePickerOvl, _surfaces.active());
      }
      break;
    }
    case UiEventType::ChoicePickerClose:
      closeChoicePickerOverlay();
      break;
    case UiEventType::CalibrationClose:
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
      closeCalibrationOverlay();
#endif
      break;
    case UiEventType::KeyboardClose:
      closeKeyboardOverlay();
      break;
    case UiEventType::SendMessageClose:
      closeSendMessageOverlay();
      break;
    case UiEventType::MessageKeyboardOpen: {
      auto* req = static_cast<const UiMessageKeyboardRequest*>(event->payload);
      if (!req || !_surfaces.isActive(&_sendMessageOvl)) break;
      if (!_keyboardOvl.prepareMessageInput(req->title ? req->title : "broadcast")) break;
      (void)ui_defer(+[](void* user_data) {
        auto* app = static_cast<UiApp*>(user_data);
        if (!app || !app->_surfaces.isActive(&app->_sendMessageOvl)) return;
        app->presentMessageKeyboard();
      }, this);
      break;
    }
    case UiEventType::WaypointKeyboardOpen:
      openWaypointKeyboard();
      break;
    case UiEventType::MessageKeyboardSubmit: {
      auto* submit = static_cast<const UiMessageKeyboardSubmit*>(event->payload);
      if (submit && submit->text) {
        if (_surfaces.contains(&_sendMessageOvl)) {
          _sendMessageOvl.submitCustomMessage(submit->text);
        }
      }
      break;
    }
    case UiEventType::WaypointKeyboardSubmit: {
      auto* submit = static_cast<const UiWaypointKeyboardSubmit*>(event->payload);
      if (submit) _surfaces.dispatchEventToActive(UiEventType::WaypointKeyboardSubmit, submit);
      break;
    }
    case UiEventType::RebindInput:
      _surfaces.reconcileFocus();
      break;
    default:
      break;
  }
}

void UiApp::handleAppStateEvent(const AppStateEvent& event) {
  if (event.type == AppStateEventType::BatteryChanged) {
    _top_pane.setBatteryMilliVolts(event.battery.millivolts);
  }
}

void UiApp::tick() {
  if (!_inited) return;
  lv_timer_handler();
}

bool UiApp::initScreens(_lv_obj_t* content) {
  if (!content) return false;

  AbstractScreen* screens[static_cast<uint8_t>(eScreenId::kScreenCnt)];
  uint8_t screen_count = 0;
  screens[screen_count++] = &_scrHome;
  screens[screen_count++] = &_scrRecent;
  screens[screen_count++] = &_scrRadio;
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  screens[screen_count++] = &_scrCompass;
  screens[screen_count++] = &_scrFindFriend;
#endif
  screens[screen_count++] = &_scrGPS;
#if defined(ENV_INCLUDE_MAP) && ENV_INCLUDE_MAP
  screens[screen_count++] = &_scrTracker;
#endif
  screens[screen_count++] = &_scrSystem;

  _root = ht_obj_create(content, meta_id::AppScreenRoot);
  if (!_root) return false;
  lv_obj_set_size(_root, lv_pct(100), lv_pct(100));
  lv_obj_set_style_pad_all(_root, 0, LV_PART_MAIN);
  lv_obj_clear_flag(_root, LV_OBJ_FLAG_SCROLLABLE);

  _tileview = lv_tileview_create(_root);
  if (!_tileview) return false;
  ht_set_meta_id(_tileview, meta_id::AppTileView);
  ui_widget_theme_apply(_tileview);
  lv_obj_set_size(_tileview, lv_pct(100), lv_pct(100));
  lv_obj_set_style_pad_all(_tileview, 0, LV_PART_MAIN);
  lv_obj_set_scrollbar_mode(_tileview, LV_SCROLLBAR_MODE_OFF);
  lv_obj_add_event_cb(_tileview, tileview_size_changed_cb, LV_EVENT_SIZE_CHANGED, nullptr);
  if (_frame_root) lv_obj_update_layout(_frame_root);
  lv_obj_update_layout(_tileview);

  for (uint8_t i = 0; i < screen_count; ++i) {
#if defined(UI_NAVIGATION_GRID) && UI_NAVIGATION_GRID
    lv_obj_t* tile = lv_tileview_add_tile(_tileview, i, 0, LV_DIR_NONE);
#else
    lv_obj_t* tile = lv_tileview_add_tile(_tileview, i, 0, LV_DIR_HOR);
#endif
    if (!tile) return false;

    AbstractScreen* scr = screens[i];
    if (!scr) continue;

    ht_set_meta_id(tile, meta_id::AppTile);
    ht_set_user_data(tile, scr);
    ui_widget_theme_apply(tile);
    lv_obj_set_style_pad_all(tile, ui_app_frame_metrics(_root).screen_pad, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(tile, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(tile, LV_OBJ_FLAG_SCROLLABLE);
    scr->setTarget(_frame_root);
    if (!scr->init(tile)) return false;
    lv_obj_t* scr_root = scr->root();
    if (!scr_root) return false;
  }

  relayout_tileview_tiles(_tileview);

  lv_obj_t* first_tile = lv_obj_get_child(_tileview, 0);
  if (first_tile) lv_obj_set_tile(_tileview, first_tile, LV_ANIM_OFF);
#if defined(UI_NAVIGATION_GRID) && UI_NAVIGATION_GRID
  lv_obj_clear_flag(_tileview, LV_OBJ_FLAG_SCROLLABLE);
#else
  lv_obj_set_scroll_dir(_tileview, LV_DIR_HOR);
#endif

  if (AbstractScreen* home = screenAt(0)) _top_pane.setTitle(home->title());
  return true;
}

bool UiApp::initTimers() {
  if (!ui_deferred_init()) return false;
  if (!ui_motion_init()) return false;
  if (!_app_state_dispatcher.createTimer()) return false;
  if (!_nav_auto_commit_timer) {
    _nav_auto_commit_timer = lv_timer_create(navigationAutoCommitTimerCb, 1U, this);
    if (!_nav_auto_commit_timer) return false;
    lv_timer_set_repeat_count(_nav_auto_commit_timer, -1);
    lv_timer_pause(_nav_auto_commit_timer);
  }
  if (!_display_auto_off_timer) {
    _display_auto_off_timer = lv_timer_create(displayAutoOffTimerCb, 1U, this);
    if (!_display_auto_off_timer) return false;
    lv_timer_set_repeat_count(_display_auto_off_timer, -1);
    lv_timer_pause(_display_auto_off_timer);
  }
#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
  if (!_deferred_touch_timer) {
    _deferred_touch_timer = lv_timer_create(deferredTouchActionTimerCb, 40U, this);
    if (!_deferred_touch_timer) return false;
    lv_timer_set_repeat_count(_deferred_touch_timer, -1);
    lv_timer_pause(_deferred_touch_timer);
  }
#endif
  restartNavigationAutoCommitTimer();
  restartDisplayAutoOffTimer();
  return true;
}

void UiApp::notifyDisplayActivity(uint32_t now_ms) {
  if (!_display_auto_off_ms) return;
  _display_last_activity_ms = now_ms;
  restartDisplayAutoOffTimer();
  if (!heltec::meshcore::dal::display_port::isBacklightOn()) {
    onBacklightTurnedOn();
  }
}

void UiApp::onBacklightTurnedOn() {
  heltec::meshcore::dal::display_port::setBacklightOn(true);
  if (_display_auto_off_ms) {
    _display_last_activity_ms = millis();
    restartDisplayAutoOffTimer();
  }
  ensureTileKeypadFocus();
  reconcileInput();
  lv_obj_t* scr = lv_scr_act();
  if (scr) lv_obj_invalidate(scr);
}

void UiApp::setDisplayAutoOffMs(uint32_t ms) {
  if (BacklightPolicy::mode() == BacklightMode::ManualToggle) {
    ms = 0;
  }
  _display_auto_off_ms = ms;
  if (ms) {
    _display_last_activity_ms = millis();
    restartDisplayAutoOffTimer();
  } else {
    _display_last_activity_ms = 0;
    stopDisplayAutoOffTimer();
  }
}

void UiApp::restartDisplayAutoOffTimer() {
  if (!_display_auto_off_timer) return;
  if (BacklightPolicy::mode() == BacklightMode::ManualToggle ||
      !_display_auto_off_ms || !_display_last_activity_ms ||
      !heltec::meshcore::dal::display_port::isBacklightOn()) {
    stopDisplayAutoOffTimer();
    return;
  }

  const uint32_t elapsed = millis() - _display_last_activity_ms;
  const uint32_t delay_ms = elapsed >= _display_auto_off_ms
                                ? 1U
                                : _display_auto_off_ms - elapsed;
  lv_timer_set_period(_display_auto_off_timer, delay_ms);
  lv_timer_set_repeat_count(_display_auto_off_timer, -1);
  lv_timer_reset(_display_auto_off_timer);
  lv_timer_resume(_display_auto_off_timer);
}

void UiApp::stopDisplayAutoOffTimer() {
  if (_display_auto_off_timer) lv_timer_pause(_display_auto_off_timer);
}

void UiApp::handleDisplayAutoOffTimeout() {
  stopDisplayAutoOffTimer();
  if (BacklightPolicy::mode() == BacklightMode::ManualToggle) return;
  if (!_display_auto_off_ms || !_display_last_activity_ms) return;
  if (!heltec::meshcore::dal::display_port::isBacklightOn()) return;
  if (_surfaces.isActive(&_radioParamSyncOvl)) {
    _display_last_activity_ms = millis();
    restartDisplayAutoOffTimer();
    return;
  }
  if ((millis() - _display_last_activity_ms) < _display_auto_off_ms) {
    restartDisplayAutoOffTimer();
    return;
  }

  ensureTileKeypadFocus();
  heltec::meshcore::dal::display_port::setBacklightOn(false);
  _display_last_activity_ms = 0;
}

void UiApp::displayAutoOffTimerCb(lv_timer_t* timer) {
  auto* app = timer ? static_cast<UiApp*>(timer->user_data) : nullptr;
  if (app) app->handleDisplayAutoOffTimeout();
}

}  // namespace heltec::meshcore::ui
