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

#ifndef HELTEC_TOUCH_EDGE_PX
#define HELTEC_TOUCH_EDGE_PX 24
#endif
#ifndef HELTEC_TOUCH_ACTION_EDGE_PX
#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
#define HELTEC_TOUCH_ACTION_EDGE_PX 16
#else
#define HELTEC_TOUCH_ACTION_EDGE_PX HELTEC_TOUCH_EDGE_PX
#endif
#endif
#ifndef HELTEC_TOUCH_TOP_ACTION_SIDE_MARGIN_PCT
#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
#define HELTEC_TOUCH_TOP_ACTION_SIDE_MARGIN_PCT 25
#else
#define HELTEC_TOUCH_TOP_ACTION_SIDE_MARGIN_PCT 0
#endif
#endif

namespace heltec::meshcore::ui {

namespace {

UiApp* s_ui_app_instance = nullptr;

static void cm_send_advert(biz::IBizFacade& app) {
  app.sendAdvertWithFeedback();
}

static void cm_open_send_message(biz::IBizFacade& app) {
  app.requestSendMessageOverlay();
}

static lv_coord_t current_display_width() {
  lv_disp_t* disp = lv_disp_get_default();
  return disp ? lv_disp_get_hor_res(disp) : 0;
}

static lv_coord_t current_display_height() {
  lv_disp_t* disp = lv_disp_get_default();
  return disp ? lv_disp_get_ver_res(disp) : 0;
}

static bool is_in_top_action_x_band(int16_t start_x) {
  const lv_coord_t w = current_display_width();
  if (w <= 0) return true;
  const lv_coord_t margin = (w * HELTEC_TOUCH_TOP_ACTION_SIDE_MARGIN_PCT) / 100;
  return start_x >= margin && start_x <= (w - margin);
}

static bool is_left_edge_right_swipe(uint8_t axis, int8_t dir, int16_t start_x) {
  return axis == static_cast<uint8_t>(heltec::meshcore::dal::touch_input::SwipeAxis::Horizontal) &&
         dir < 0 && start_x <= HELTEC_TOUCH_ACTION_EDGE_PX;
}

static bool is_top_edge_down_swipe(uint8_t axis, int8_t dir, int16_t start_y) {
  return axis == static_cast<uint8_t>(heltec::meshcore::dal::touch_input::SwipeAxis::Vertical) &&
         dir > 0 && start_y <= HELTEC_TOUCH_ACTION_EDGE_PX;
}

#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
static lv_coord_t displayWidth() {
  return current_display_width();
}

static lv_coord_t displayHeight() {
  return current_display_height();
}

static bool is_right_edge_left_swipe(uint8_t axis, int8_t dir, int16_t start_x) {
  const lv_coord_t w = displayWidth();
  if (w <= HELTEC_TOUCH_ACTION_EDGE_PX) return false;
  return axis == static_cast<uint8_t>(heltec::meshcore::dal::touch_input::SwipeAxis::Horizontal) &&
         dir > 0 && start_x >= (w - HELTEC_TOUCH_ACTION_EDGE_PX);
}

static bool is_bottom_edge_up_swipe(uint8_t axis, int8_t dir, int16_t start_y) {
  const lv_coord_t h = displayHeight();
  if (h <= HELTEC_TOUCH_ACTION_EDGE_PX) return false;
  return axis == static_cast<uint8_t>(heltec::meshcore::dal::touch_input::SwipeAxis::Vertical) &&
         dir < 0 && start_y >= (h - HELTEC_TOUCH_ACTION_EDGE_PX);
}
#endif

static bool is_top_action_down_swipe(uint8_t axis, int8_t dir, int16_t start_x,
                                     int16_t start_y) {
  return is_top_edge_down_swipe(axis, dir, start_y) && is_in_top_action_x_band(start_x);
}

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

#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
static void cm_calibrate_compass(biz::IBizFacade& app) {
  app.requestCompassCalibration();
}
#endif

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

void UiApp::openNavigationPane() {
  if (!_inited || _surfaces.contains(&_navigation)) return;

  reconcileInput();
  if (_surfaces.contains(&_navigation)) return;

  _navigation.setSelectedIndex(activeTileIndex(), true);
  notifyNavActivity(millis());
  (void)_surfaces.present(&_navigation);
}

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
  app_state_notifier().addObserver(&_app_state_dispatcher);

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
  touch_hooks.block_double_tap = UiApp::touchGestureBlockQuickPingKeyboard;
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
  if (!initNavigationPane(_layerOverlay)) return;
  _navigation.setFrameRoot(_frame_root);
  _navigation.setTileView(_tileview);
  bindFrameEvents();
  if (_tileview) {
    lv_obj_add_event_cb(_tileview, [](lv_event_t* e) {
      auto* app = static_cast<UiApp*>(lv_event_get_user_data(e));
      if (app) {
        const lv_event_code_t code = lv_event_get_code(e);
        if (code == LV_EVENT_VALUE_CHANGED) {
          app->onTileActiveChanged();
          return;
        }
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
            if (idx) app->scheduleNavTileCommit(*idx);
            break;
          }
          default:
            break;
        }
      }
    }, LV_EVENT_ALL, this);

  }
  _previewOvl.setTarget(_frame_root);
  _alertOvl.setTarget(_frame_root);
  _radioParamSyncOvl.setTarget(_frame_root);
  _keyboardOvl.setTarget(_frame_root);
  _sendMessageOvl.setTarget(_frame_root);
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  _calibrationOvl.setTarget(_frame_root);
#endif
#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
  _quickPingOverlay.setTarget(_frame_root);
#else
  _contextMenu.setTarget(_frame_root);
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
    lv_obj_update_layout(_tileview);
  }
  activateActiveScreen();
}

void UiApp::bindFrameEvents() {
  if (!_frame_root || _frame_events_bound) return;
  lv_obj_add_event_cb(_frame_root, [](lv_event_t* e) {
    auto* app = static_cast<UiApp*>(lv_event_get_user_data(e));
    if (app) app->handleFrameEvent(e);
  }, LV_EVENT_ALL, this);
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
#if !defined(HELTEC_V4_R8_TFT) || !defined(HELTEC_HAS_TOUCH) || !HELTEC_HAS_TOUCH
    case UiEventType::ContextOpen:
      (void)lv_async_call([](void* user_data) {
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
      (void)lv_async_call([](void* user_data) {
        auto* app = static_cast<UiApp*>(user_data);
        if (!app || !app->_surfaces.isActive(&app->_sendMessageOvl)) return;
        app->presentMessageKeyboard();
      }, this);
      break;
    }
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
      if (submit) {
        if (AbstractScreen* scr = activeScreen()) {
          scr->onWaypointKeyboardSubmit(submit->lat, submit->lon);
        }
      }
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

void UiApp::scheduleNavTileCommit(uint8_t tile_idx) {
  const bool nav_active = _surfaces.isActive(&_navigation);
  const bool transitioning = _navigation.isTransitioning();
  if (!nav_active || transitioning) return;
  if (!_tileview || tile_idx >= kScreenCnt) return;
  if (kNoScheduledTile != _scheduled_nav_tile) return;
  stopNavigationAutoCommitTimer();
  _scheduled_nav_tile = tile_idx;
  if (LV_RES_OK != lv_async_call([](void* user_data) {
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

#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH

void UiApp::toggleNavigationPaneFromEdgeSwipe() {
  const bool nav_open = _surfaces.contains(&_navigation);
  if (nav_open) {
    closeNavigationPane();
    return;
  }

  if (inputOnActiveScreen()) openNavigationPane();
}

void UiApp::toggleQuickPingFromTopSwipe() {
  if (_surfaces.contains(&_quickPingOverlay)) {
    if (!_quickPingOverlay.requestCloseAnimation()) {
      (void)_surfaces.dismissBranch(&_quickPingOverlay);
    }
    return;
  }

  if (_surfaces.contains(&_navigation)) {
    closeNavigationImmediate();
  }
  if (inputOnActiveScreen()) {
    (void)_surfaces.present(&_quickPingOverlay, _surfaces.root());
  }
}

void UiApp::deferTouchAction(DeferredTouchAction action) {
  if (action == DeferredTouchAction::None) return;
  _deferred_touch_action = action;

  if (!_deferred_touch_timer) {
    _deferred_touch_timer = lv_timer_create(deferredTouchActionTimerCb, 40U, this);
    if (!_deferred_touch_timer) {
      runDeferredTouchAction();
      return;
    }
    lv_timer_set_repeat_count(_deferred_touch_timer, -1);
    lv_timer_pause(_deferred_touch_timer);
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
    case DeferredTouchAction::ToggleNavigation:
      toggleNavigationPaneFromEdgeSwipe();
      break;
    case DeferredTouchAction::ToggleQuickPing:
      toggleQuickPingFromTopSwipe();
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

void UiApp::onTouchSwipe(uint8_t axis, int8_t dir, int16_t start_x, int16_t start_y) {
  if (!_inited || 0 == dir) return;
  if (!heltec::meshcore::dal::display_port::isBacklightOn()) {
    notifyDisplayActivity(millis());
    return;
  }

#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
  if (_surfaces.isActive(&_radioParamSyncOvl)) {
    notifyDisplayActivity(millis());
    return;
  }

  if (is_right_edge_left_swipe(axis, dir, start_x) ||
      is_bottom_edge_up_swipe(axis, dir, start_y)) {
    notifyDisplayActivity(millis());
    return;
  }
#endif

  if (is_left_edge_right_swipe(axis, dir, start_x)) {
    const lv_coord_t h = current_display_height();
    if (start_y <= HELTEC_TOUCH_ACTION_EDGE_PX ||
        (h > 0 && start_y >= (h - HELTEC_TOUCH_ACTION_EDGE_PX))) {
      return;
    }
    notifyDisplayActivity(millis());
#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
    deferTouchAction(DeferredTouchAction::ToggleNavigation);
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

  if (is_top_action_down_swipe(axis, dir, start_x, start_y)) {
    notifyDisplayActivity(millis());
#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
    deferTouchAction(DeferredTouchAction::ToggleQuickPing);
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

  if (axis != static_cast<uint8_t>(heltec::meshcore::dal::touch_input::SwipeAxis::Horizontal)) {
    return;
  }
  if (_surfaces.contains(&_navigation)) return;
#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
  if (_surfaces.contains(&_quickPingOverlay)) return;
#else
  if (_surfaces.contains(&_contextMenu)) return;
#endif
  if (_surfaces.isActive(&_radioParamSyncOvl)) {
    _radioParamSyncOvl.stepSelection(dir);
    notifyDisplayActivity(millis());
    return;
  }
  (void)switchAdjacentTile(dir);
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
  _navigation.setSelectedIndex((uint8_t)next, true);
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

void UiApp::tick() {
  if (!_inited) return;
  lv_timer_handler();
}

bool UiApp::initOverlay() {
  if (!_layerOverlay) return false;
  if (!_previewOvl.create(_layerOverlay)) return false;
  if (!_alertOvl.create(_layerOverlay)) return false;
  if (!_radioParamSyncOvl.create(_layerOverlay)) return false;
  if (!_keyboardOvl.create(_layerOverlay)) return false;
  if (!_sendMessageOvl.create(_layerOverlay)) return false;
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  if (!_calibrationOvl.create(_layerOverlay)) return false;
#endif
#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
  if (!_quickPingOverlay.create(_layerOverlay)) return false;
#else
  if (!_contextMenu.create(_layerOverlay)) return false;
  ensureContextMenusRegistered();
  if (!_context_menus_registered || !_contextMenu.canOpen()) return false;
#endif
  return true;
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
    lv_obj_t* scr_root = scr->create(tile);
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

#if !defined(HELTEC_V4_R8_TFT) || !defined(HELTEC_HAS_TOUCH) || !HELTEC_HAS_TOUCH
bool UiApp::registerRadioContextMenu() {
  (void)_ctxRadioMenu.addCommandHandler("send message", cm_open_send_message);
  (void)_ctxRadioMenu.addCommandHandler("send advert", cm_send_advert);
  return _contextMenu.registerMenu("QuickPing", _scrRadio.icon(), _ctxRadioMenu);
}

bool UiApp::registerCompassContextMenu() {
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  (void)_ctxCompassMenu.addCommandHandler("calibrate", cm_calibrate_compass);
  return _contextMenu.registerMenu(
      _scrCompass.title(), _scrCompass.icon(), _ctxCompassMenu);
#else
  return true;
#endif
}

void UiApp::ensureContextMenusRegistered() {
  if (_context_menus_registered) return;

  _contextMenu.beginRegister();

  const bool radio_ok = registerRadioContextMenu();
  const bool compass_ok = registerCompassContextMenu();

  _contextMenu.endRegister();
  _context_menus_registered = radio_ok && compass_ok;
}
#endif

void UiApp::openPreviewOverlay(uint8_t unread, uint32_t age_sec, const char* origin, const char* text) {
  if (!_inited) return;
  _previewOvl.applyContent(unread, age_sec, origin, text);
  if (_surfaces.isActive(&_previewOvl)) return;
  (void)_surfaces.present(&_previewOvl, nullptr);
}

void UiApp::closePreviewOverlay() {
  const bool was_present = _surfaces.contains(&_previewOvl);
  const bool dismissed = _surfaces.dismissBranch(&_previewOvl);
  if (!was_present || dismissed) ui_task().dismissPreview();
}

void UiApp::openAlertOverlay(const char* text) {
  if (!_inited) return;
  _alertOvl.setText(text);
  UI_ALERT_LOG("open text=%s now=%lu active=%d contains=%d depth=%u active_surface=%p",
               text ? text : "",
               (unsigned long)millis(),
               _surfaces.isActive(&_alertOvl) ? 1 : 0,
               _surfaces.contains(&_alertOvl) ? 1 : 0,
               (unsigned)_surfaces.modalDepth(),
               _surfaces.active());
  if (_surfaces.contains(&_alertOvl)) {
    const bool ok = _surfaces.raise(&_alertOvl);
    UI_ALERT_LOG("raise ok=%d depth=%u active=%p",
                 ok ? 1 : 0,
                 (unsigned)_surfaces.modalDepth(),
                 _surfaces.active());
    return;
  }
  const bool ok = _surfaces.present(&_alertOvl, nullptr);
  UI_ALERT_LOG("present ok=%d depth=%u active=%p",
               ok ? 1 : 0,
               (unsigned)_surfaces.modalDepth(),
               _surfaces.active());
}

void UiApp::closeAlertOverlay() {
  const bool was_present = _surfaces.contains(&_alertOvl);
  UI_ALERT_LOG("close begin now=%lu was_present=%d active=%d depth=%u active_surface=%p",
               (unsigned long)millis(),
               was_present ? 1 : 0,
               _surfaces.isActive(&_alertOvl) ? 1 : 0,
               (unsigned)_surfaces.modalDepth(),
               _surfaces.active());
  const bool dismissed = _surfaces.dismissBranch(&_alertOvl);
  UI_ALERT_LOG("close dismissed=%d depth=%u contains=%d active_surface=%p",
               dismissed ? 1 : 0,
               (unsigned)_surfaces.modalDepth(),
               _surfaces.contains(&_alertOvl) ? 1 : 0,
               _surfaces.active());
  if (!was_present || dismissed) ui_task().dismissAlert();
}

bool UiApp::openSendMessageOverlay() {
  if (!_inited) return false;
  if (_surfaces.isActive(&_sendMessageOvl)) return true;
  return _surfaces.present(&_sendMessageOvl);
}

void UiApp::closeSendMessageOverlay() {
  (void)_surfaces.dismissBranch(&_sendMessageOvl);
}

void UiApp::openRadioParamSyncOverlay() {
  if (!_inited) return;
  if (_surfaces.isActive(&_radioParamSyncOvl)) return;
  notifyDisplayActivity(millis());
  (void)_surfaces.present(&_radioParamSyncOvl);
}

void UiApp::closeRadioParamSyncOverlay() {
  (void)_surfaces.dismissBranch(&_radioParamSyncOvl);
}

#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
bool UiApp::openCalibrationOverlay() {
  if (!_inited) return false;
  return _surfaces.present(&_calibrationOvl);
}

void UiApp::closeCalibrationOverlay() {
  if (!_surfaces.contains(&_calibrationOvl)) return;
  _scrCompass.skipAutoCalibrationOnce();
  if (!_surfaces.dismissBranch(&_calibrationOvl)) return;
}
#endif

void UiApp::presentMessageKeyboard() {
  if (!_inited) return;
  const bool send_active = _surfaces.isActive(&_sendMessageOvl);
  if (!send_active) return;
  if (_surfaces.isActive(&_keyboardOvl)) return;
  (void)_surfaces.present(&_keyboardOvl, &_sendMessageOvl);
}

void UiApp::openWaypointKeyboard() {
  if (!_inited) return;
  UiSurface* const owner = _surfaces.active();
  if (!_keyboardOvl.prepareWaypointInput()) return;
  if (_surfaces.isActive(&_keyboardOvl)) return;
  (void)_surfaces.present(&_keyboardOvl, owner);
}

void UiApp::closeKeyboardOverlay() {
  if (!_surfaces.isActive(&_keyboardOvl)) return;
  const bool waypoint = _keyboardOvl.isWaypointCompose();
  (void)_surfaces.dismiss(&_keyboardOvl);
  if (waypoint) {
    if (AbstractScreen* scr = activeScreen()) scr->onWaypointKeyboardClosed();
  }
}

#if !defined(HELTEC_V4_R8_TFT) || !defined(HELTEC_HAS_TOUCH) || !HELTEC_HAS_TOUCH
bool UiApp::openContextMenu() {
  if (!_inited || !_frame_root || _surfaces.contains(&_contextMenu)) return false;
  return ui_event_send(_frame_root, UiEventType::ContextOpen);
}

void UiApp::dismissTopContextMenu() {
  if (!_surfaces.contains(&_contextMenu)) {
    return;
  }
  _contextMenu.leaveMenuLeaf();
  (void)_surfaces.dismiss(&_contextMenu);
}

void UiApp::dismissContextMenuStack() {
  if (!_surfaces.contains(&_contextMenu)) {
    return;
  }
  _contextMenu.leaveMenuLeaf();
  (void)_surfaces.dismissBranch(&_contextMenu);
}
#endif

bool UiApp::initNavigationPane(_lv_obj_t* parent) {
  if (!_navigation.create(parent)) return false;

  struct NavSlot {
    eScreenId id;
    AbstractScreen* scr;
  };
#if defined(UI_NAVIGATION_GRID) && UI_NAVIGATION_GRID
  const NavSlot slots[] = {
      {eScreenId::Home, &_scrHome},
      {eScreenId::Radio, &_scrRadio},
      {eScreenId::Recent, &_scrRecent},
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
      {eScreenId::Compass, &_scrCompass},
#else
      {eScreenId::GPS, &_scrGPS},
#endif
#if defined(ENV_INCLUDE_MAP) && ENV_INCLUDE_MAP
      {eScreenId::Tracker, &_scrTracker},
#endif
      {eScreenId::System, &_scrSystem},
  };
#else
  const NavSlot slots[] = {
      {eScreenId::Home, &_scrHome},
      {eScreenId::Recent, &_scrRecent},
      {eScreenId::Radio, &_scrRadio},
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
      {eScreenId::Compass, &_scrCompass},
      {eScreenId::FindFriend, &_scrFindFriend},
#endif
      {eScreenId::GPS, &_scrGPS},
#if defined(ENV_INCLUDE_MAP) && ENV_INCLUDE_MAP
      {eScreenId::Tracker, &_scrTracker},
#endif
      {eScreenId::System, &_scrSystem},
  };
#endif
  for (const NavSlot& slot : slots) {
    _navigation.setIcon(static_cast<uint8_t>(slot.id), slot.scr->icon());
#if defined(UI_NAVIGATION_GRID) && UI_NAVIGATION_GRID
    _navigation.setLabel(static_cast<uint8_t>(slot.id), slot.scr->title());
#endif
  }
#if defined(UI_NAVIGATION_GRID) && UI_NAVIGATION_GRID && \
    defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
  _navigation.setIcon(static_cast<uint8_t>(eScreenId::FindFriend), _scrFindFriend.icon());
  _navigation.setLabel(static_cast<uint8_t>(eScreenId::FindFriend), _scrFindFriend.title());
  _navigation.setFooterSlot(static_cast<uint8_t>(eScreenId::FindFriend));
#endif

  _navigation.setSelectedIndex(activeTileIndex(), true);

  return true;
}

bool UiApp::initTimers() {
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
  restartNavigationAutoCommitTimer();
  restartDisplayAutoOffTimer();
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

void UiApp::previewNavTile(uint8_t tile_idx) {
  if (AbstractScreen* scr = screenAt(tile_idx)) {
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

#if defined(HELTEC_TOUCH_GESTURE_INPUT) && HELTEC_TOUCH_GESTURE_INPUT
bool UiApp::touchGestureBlockTrackerViewport(int16_t x, int16_t y) {
  if (x <= HELTEC_TOUCH_EDGE_PX || y <= HELTEC_TOUCH_EDGE_PX) return false;
  return UiApp::instance().hitActiveTrackerViewport(x, y);
}

#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH
bool UiApp::touchGestureBlockVerticalSwipe(int16_t x, int16_t y) {
  auto& app = UiApp::instance();
#if defined(ENV_INCLUDE_MAP) && ENV_INCLUDE_MAP
  if (app.hitActiveTrackerViewport(x, y)) return true;
#endif
  return app.inputOnActiveScreen() && app.activeScreen() == &app._scrSystem &&
         app._scrSystem.hitScrollableContent(x, y);
}

bool UiApp::touchGestureRawPointerPassthrough(int16_t x, int16_t y) {
  auto& app = UiApp::instance();
  return app._surfaces.isActive(&app._radioParamSyncOvl) &&
         app._radioParamSyncOvl.hitRoller(x, y);
}

bool UiApp::touchGestureBlockQuickPingKeyboard(int16_t x, int16_t y) {
  auto& app = UiApp::instance();
  return app._surfaces.isActive(&app._quickPingOverlay) &&
         app._quickPingOverlay.hitVisibleKeyboard(x, y);
}
#endif
#endif

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

void UiApp::displayAutoOffTimerCb(lv_timer_t* timer) {
  auto* app = timer ? static_cast<UiApp*>(timer->user_data) : nullptr;
  if (app) app->handleDisplayAutoOffTimeout();
}

}  // namespace heltec::meshcore::ui
