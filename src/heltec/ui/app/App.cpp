#include "ui/app/App.hpp"

#include "heltec/drivers/input/TouchInput.hpp"
#include "heltec/ui/Images.h"
#include "ui/theme/UiTheme.hpp"
#include "ui/core/AppStateNotifier.hpp"
#include "ui/core/BacklightPolicy.hpp"
#include "ui/core/UiMessage.hpp"
#include "ui/core/InputRouter.hpp"
#include "ui/core/Scheduler.hpp"
#include "ui/app/ScreenId.hpp"
#include "ui/core/Surface.hpp"
#include "ui/core/UiTask.hpp"
#include "ui/widgets/WidgetClasses.hpp"
#include "ui/menus/ContextMenu.hpp"
#include "ui/navigation/NavigationWidget.hpp"
#include "ui/overlays/SplashOverlay.hpp"
#include "ui/theme/WidgetTheme.hpp"
#if defined(HELTEC_HAS_KEY_FRAME) && HELTEC_HAS_KEY_FRAME
#include "heltec/drivers/display/DisplayPort.hpp"
#endif

#include <Arduino.h>
#include <cstring>
#include <lvgl.h>

namespace Heltec::MeshCore::Ui {

namespace {

UiApp *s_ui_app_instance = nullptr;

static void relayout_tileview_tiles(lv_obj_t *tileview) {
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
        lv_obj_t *tile = lv_obj_get_child(tileview, i);
        if (!tile) continue;
        lv_obj_set_size(tile, tile_w, tile_h);
        lv_obj_set_pos(tile, static_cast<lv_coord_t>(i) * tile_w, 0);
    }

    if (lv_obj_t *active_tile = lv_tileview_get_tile_act(tileview)) {
        lv_obj_set_tile(tileview, active_tile, LV_ANIM_OFF);
    }
}

} // namespace

UiApp &UiApp::instance() {
    LV_ASSERT(s_ui_app_instance != nullptr);
    return *s_ui_app_instance;
}

UiApp::UiApp(Biz::IBizFacade &biz, Heltec::MeshCore::Power::PowerMgr &power)
    : _biz(biz), _power(power), _top_pane(), _navigation(_biz),
      _scrHome(_biz, "Home", &icon_home_img), _scrRecent(_biz, "Recent", &icon_recent_img),
      _scrRadio(_biz, "Radio", &icon_radio_img),
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
      _scrCompass(_biz, "Compass", &icon_compass_img),
#endif
#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
      _scrFindFriend(_biz, "Find Friend", &icon_findfriend_img),
#endif
#if ENV_INCLUDE_GPS
      _scrGPS(_biz, "GPS", &icon_gps_img),
#endif
#if defined(ENV_INCLUDE_MAP) && ENV_INCLUDE_MAP
      _scrTracker(_biz, "Tracker", &icon_map_img),
#endif
      _scrSystem(_biz, "System", &icon_system_img)
#if HELTEC_UI_HAS_TOUCH
      ,
      _quickPingOverlay(_biz)
#endif
      ,
      _previewOvl(_biz), _alertOvl(_biz), _confirmOvl(_biz), _radioParamSyncOvl(_biz),
      _repeatModeOvl(_biz), _sendMessageOvl(_biz)
#if HELTEC_UI_ENABLE_KEYBOARD
      ,
      _keyboardOvl(_biz)
#endif
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
      ,
      _calibrationOvl(_biz)
#endif
#if !defined(HELTEC_UI_HAS_TOUCH) || !HELTEC_UI_HAS_TOUCH
      ,
      _contextMenu(_biz)
#endif
{
    LV_ASSERT(s_ui_app_instance == nullptr);
    s_ui_app_instance = this;
}

AbstractScreen *UiApp::activeScreen() const {
    lv_obj_t *tile = _tileview ? lv_tileview_get_tile_act(_tileview) : nullptr;
    return tile ? static_cast<AbstractScreen *>(lv_obj_get_user_data(tile)) : nullptr;
}

AbstractScreen *UiApp::screenAt(uint8_t index) const {
    if (!_tileview) return nullptr;
    lv_obj_t *tile = lv_obj_get_child(_tileview, index);
    return tile ? static_cast<AbstractScreen *>(lv_obj_get_user_data(tile)) : nullptr;
}

AbstractScreen *UiApp::resolveActiveScreen() const {
    if (AbstractScreen *scr = activeScreen()) return scr;
    if (AbstractScreen *scr = screenAt(activeTileIndex())) return scr;
    return screenAt(0);
}

void UiApp::activateActiveScreen() {
    bindScreen(resolveActiveScreen());
}

void UiApp::bindScreen(AbstractScreen *scr) {
    if (!scr) {
        return;
    }
    _surfaces.setRoot(scr);
}

bool UiApp::inputOnActiveScreen() const {
    AbstractScreen *scr = resolveActiveScreen();
    return scr && _surfaces.isActive(static_cast<UiSurface *>(scr));
}

void UiApp::setTopPaneTitle(const char *title) {
    if (title) _top_pane.setTitle(title);
}

void UiApp::refreshTopPaneStatus(bool force) {
    const uint32_t epoch_seconds = _biz.currentTimeSeconds();
    const uint32_t minute = epoch_seconds / 60U;
    if (force || minute != _top_pane_minute) {
        _top_pane_minute = minute;
        _top_pane.setTime(epoch_seconds);
    }
}

bool UiApp::init() {
    _inited = false;
    initUiMessages();
    _app_state_dispatcher.bindSurfaceManager(_surfaces);
    _app_state_dispatcher.bindGlobalHandler(
        [](void *user_data, const AppStateEvent &event) {
            auto *app = static_cast<UiApp *>(user_data);
            if (app) app->handleAppStateEvent(event);
        },
        this);
    _app_state_dispatcher.bindNotifier(app_state_notifier());

    InputRouter::init();
#if HELTEC_TOUCH_INPUT
    Heltec::MeshCore::Dal::TouchInput::UiHooks touch_hooks{};
    touch_hooks.wake = +[]() {
        auto &app = UiApp::instance();
        if (BacklightPolicy::mode() == BacklightMode::AutoTimeout) {
            const bool was_off = !app.isDisplayOn();
            app.notifyDisplayActivity(millis());
            return was_off;
        }
        return !app.isDisplayOn();
    };
#if HELTEC_UI_HAS_TOUCH && defined(HELTEC_TOUCH_GESTURE) && HELTEC_TOUCH_GESTURE
    touch_hooks.block_long_enter = UiApp::touchGestureBlockLongPress;
    touch_hooks.raw_pointer_passthrough = UiApp::touchGestureRawPointerPassthrough;
    touch_hooks.block_double_tap = UiApp::touchGestureBlockQuickPingDoubleTap;
#endif
    Heltec::MeshCore::Dal::TouchInput::bindUi(touch_hooks);
#endif

    lv_obj_t *scr = lv_scr_act();
    if (!scr) {
        return false;
    }

    lv_disp_t *disp = lv_disp_get_default();
    if (!disp) {
        return false;
    }
    if (!Heltec::MeshCore::Ui::ui_theme_init(disp)) {
        return false;
    }
    if (!initTimers()) {
        return false;
    }

    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    ui_app_active_screen_apply_theme(scr);
#if HELTEC_UI_HAS_TOUCH
    lv_obj_t *bg = ht_class_create(&ht_app_background_class, scr);
    if (bg) {
        lv_obj_set_pos(bg, 0, 0);
        lv_obj_clear_flag(bg, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
        lv_obj_move_background(bg);
        lv_img_set_src(bg, &ui_background_img);
    }
#endif
    lv_obj_t *const overlay_layer =
        ht_class_create(&ht_app_overlay_layer_class, lv_layer_top());
    if (!overlay_layer) {
        return false;
    }
    lv_obj_set_size(overlay_layer, lv_pct(100), lv_pct(100));
    lv_obj_clear_flag(overlay_layer, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    SplashOverlay splash;
    if (!splash.create(overlay_layer)) {
        return false;
    }
    lv_obj_t *layout = ht_class_create(&ht_app_frame_class, scr);
    if (!layout) {
        return false;
    }
    lv_obj_set_size(layout, lv_pct(100), lv_pct(100));
    lv_obj_set_flex_flow(layout, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(layout, LV_OBJ_FLAG_SCROLLABLE);

    _frame_root = layout;
    if (!initOverlay(overlay_layer)) {
        return false;
    }
    if (!_top_pane.create(layout)) {
        return false;
    }
    _top_pane.setBatteryStatus(_power.snapshot().battery_mv, _power.snapshot().battery_percent);
    refreshTopPaneStatus(true);
    lv_obj_set_flex_grow(_top_pane.root(), 0);
    lv_obj_t *content = ht_class_create(&ht_app_content_class, layout);
    if (!content) {
        return false;
    }
    lv_obj_set_width(content, lv_pct(100));
    lv_obj_set_flex_grow(content, 1);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(content, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    if (!initScreens(content)) {
        return false;
    }
    if (!initNavigationPane(overlay_layer)) {
        return false;
    }
    _navigation.setFrameRoot(_frame_root);
    _navigation.setTileView(_tileview);
    bindFrameEvents();
    if (_tileview) {
        lv_obj_add_event_cb(
            _tileview,
            [](lv_event_t *e) {
                auto *app = static_cast<UiApp *>(lv_event_get_user_data(e));
                if (app) app->onTileActiveChanged();
            },
            LV_EVENT_VALUE_CHANGED, this);
        lv_obj_add_event_cb(
            _tileview,
            [](lv_event_t *e) {
                auto *app = static_cast<UiApp *>(lv_event_get_user_data(e));
                if (!app) return;
                const UiMessage *event = getUiMessage(e);
                if (!event) return;
                switch (event->type) {
                case UiMessageType::TilePreview: {
                    const auto *idx = static_cast<const uint8_t *>(event->payload);
                    if (idx) app->previewNavTile(*idx);
                    break;
                }
                case UiMessageType::TileCommit: {
                    const auto *idx = static_cast<const uint8_t *>(event->payload);
                    if (idx) app->scheduleNavTileCommit(*idx);
                    break;
                }
                default:
                    break;
                }
            },
            UI_MESSAGE_EVENT, this);
    }
#if HELTEC_UI_HAS_TOUCH && \
    defined(HELTEC_TOUCH_GESTURE) && HELTEC_TOUCH_GESTURE
    lv_obj_add_event_cb(scr, nativeTouchGestureEvent, LV_EVENT_GESTURE, this);
    if (lv_obj_t *const top_layer = lv_layer_top()) {
        lv_obj_add_event_cb(top_layer, nativeTouchGestureEvent, LV_EVENT_GESTURE, this);
    }
#endif
    _inited = true;
#if defined(HELTEC_HAS_KEY_FRAME) && HELTEC_HAS_KEY_FRAME
    splash.hide();
    Heltec::MeshCore::Dal::DisplayPort::requestRefresh();
#endif
    if (_tileview) {
        lv_obj_t *tile = lv_tileview_get_tile_act(_tileview);
        if (!tile) {
            tile = lv_obj_get_child(_tileview, 0);
            if (tile) lv_obj_set_tile(_tileview, tile, LV_ANIM_OFF);
        }
    }
    activateActiveScreen();
    return true;
}

void UiApp::bindFrameEvents() {
    if (!_frame_root || _frame_events_bound) return;
    lv_obj_add_event_cb(
        _frame_root,
        [](lv_event_t *e) {
            auto *app = static_cast<UiApp *>(lv_event_get_user_data(e));
            if (app) app->handleFrameEvent(e);
        },
        UI_MESSAGE_EVENT, this);
    _frame_events_bound = true;
}

void UiApp::handleFrameEvent(lv_event_t *e) {
    const UiMessage *event = getUiMessage(e);
    if (!event) return;

    switch (event->type) {
    // Navigation and action surfaces.
    case UiMessageType::NavOpen:
        openNavigationPane();
        break;
    case UiMessageType::NavClose:
        closeNavigationPane();
        break;
    case UiMessageType::NavActivity:
        notifyNavActivity(millis());
        break;

    // Confirmation requests and replies.
    case UiMessageType::ConfirmOpen: {
        const auto *request = static_cast<const UiConfirmAction *>(event->payload);
        if (!request || !inputOnActiveScreen() || _surfaces.isVisible(&_confirmOvl)) break;
        _confirm_reply = event->source;
        _confirmOvl.setRequest(*request);
        if (!_surfaces.isVisible(&_confirmOvl) && !_surfaces.show(&_confirmOvl)) {
            _confirm_reply = nullptr;
            ui_task().showAlert("Unable to open confirmation", 2000);
        }
        break;
    }
    case UiMessageType::ConfirmAccepted: {
        const auto *action = static_cast<const UiConfirmAction *>(event->payload);
        if (!action || !_surfaces.isVisible(&_confirmOvl)) break;
        const UiConfirmAction accepted_action = *action;
        const uint64_t team_session = _confirmOvl.teamSession();
        lv_obj_t *const reply = _confirm_reply;
        _confirm_reply = nullptr;
        if (_surfaces.show(&_confirmOvl, false) && !_surfaces.isVisible(&_confirmOvl)) {
            if (accepted_action == UiConfirmAction::TeamJoin) {
                ui_task().showAlert(_biz.teamDecidePending(team_session, true)
                                   ? "Member approved"
                                   : "Request expired or send failed",
                               2000);
                break;
            }
            if (accepted_action == UiConfirmAction::TeamLeave) {
                const bool ok = _biz.teamLeave();
                ui_task().showAlert(ok ? "Team updated" : "Team update failed", 2000);
                break;
            }
            if (reply && lv_obj_is_valid(reply)) {
                (void)sendUiMessage(reply, UiMessageType::ConfirmAccepted, &accepted_action);
            }
        }
        break;
    }
    case UiMessageType::ConfirmCancelled: {
        const UiConfirmAction cancelled_action = _confirmOvl.action();
        const uint64_t team_session = _confirmOvl.teamSession();
        lv_obj_t *const reply = _confirm_reply;
        _confirm_reply = nullptr;
        if (_surfaces.show(&_confirmOvl, false) && !_surfaces.isVisible(&_confirmOvl)) {
            if (cancelled_action == UiConfirmAction::TeamJoin) {
                (void)_biz.teamDecidePending(team_session, false);
                break;
            }
            if (reply && lv_obj_is_valid(reply)) {
                (void)sendUiMessage(reply, UiMessageType::ConfirmCancelled, &cancelled_action);
            }
        }
        break;
    }
#if !defined(HELTEC_UI_HAS_TOUCH) || !HELTEC_UI_HAS_TOUCH
    // Keypad context menu.
    case UiMessageType::ContextOpen:
        (void)lv_async_call(
            +[](void *user_data) {
                auto *app = static_cast<UiApp *>(user_data);
                if (!app) return;
                if (!app->_inited || app->_surfaces.isVisible(&app->_contextMenu)) return;
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
                if (app->_surfaces.isActive(&app->_calibrationOvl)) {
                    app->closeCalibrationOverlay();
                }
#endif
                app->ensureContextMenusRegistered();
                if (!app->_contextMenu.canOpen()) return;
                if (!app->inputOnActiveScreen()) return;
                (void)app->_surfaces.show(&app->_contextMenu, true);
            },
            this);
        break;
    case UiMessageType::ContextClose:
        closeContextMenu();
        break;
#endif
    // Transient overlays and notifications.
    case UiMessageType::SendMessageOpen:
#if HELTEC_UI_HAS_TOUCH
        _quickPingOverlay.prepareTarget(static_cast<const UiSendMessageTarget *>(event->payload));
        if (lv_async_call(
                +[](void *user_data) {
                    auto *app = static_cast<UiApp *>(user_data);
                    if (app) (void)app->openActionPane();
                },
                this) != LV_RES_OK) {
            (void)openActionPane();
        }
#else
        _sendMessageOvl.prepareTarget(static_cast<const UiSendMessageTarget *>(event->payload));
        if (lv_async_call(
                +[](void *user_data) {
                    auto *app = static_cast<UiApp *>(user_data);
                    if (app) (void)app->openSendMessageOverlay();
                },
                this) != LV_RES_OK) {
            (void)openSendMessageOverlay();
        }
#endif
        break;
    case UiMessageType::CalibrationOpen:
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
        (void)openCalibrationOverlay();
#endif
        break;
    case UiMessageType::RepeatModeOpen:
        openRepeatModeOverlay();
        break;
#if HELTEC_UI_HAS_TOUCH
    case UiMessageType::QuickPingClose:
        if (!_quickPingOverlay.requestCloseAnimation()) {
            (void)_surfaces.show(&_quickPingOverlay, false);
        }
        break;
#endif
    case UiMessageType::PreviewNext:
        (void)ui_task().advancePreview();
        break;
    case UiMessageType::PreviewClose:
        ui_task().dismissPreview();
        break;
    case UiMessageType::AlertOpen: {
        const auto *request = static_cast<const UiAlertRequest *>(event->payload);
        if (request && request->text) {
            ui_task().showAlert(request->text, request->duration_ms);
        }
        break;
    }
    case UiMessageType::AlertClose:
        ui_task().dismissAlert();
        break;
    case UiMessageType::RadioSyncClose:
        closeRadioParamSyncOverlay();
        break;
    case UiMessageType::RepeatModeClose:
        closeRepeatModeOverlay();
        break;
    case UiMessageType::CalibrationClose:
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
        closeCalibrationOverlay();
#endif
        break;
    case UiMessageType::KeyboardClose:
        closeKeyboardOverlay();
        break;
    case UiMessageType::SendMessageClose:
        closeSendMessageOverlay();
        break;

    // Keyboard requests and replies.
    case UiMessageType::MessageKeyboardOpen: {
#if HELTEC_UI_HAS_TOUCH
        auto *preset_req = static_cast<const UiMessageKeyboardRequest *>(event->payload);
        if (preset_req && preset_req->purpose == UiMessageKeyboardPurpose::EditPreset &&
            activeScreen() == &_scrSystem &&
            _quickPingOverlay.preparePresetEdit(preset_req->preset_index,
                                                preset_req->initial_text)) {
            if (lv_async_call(
                    +[](void *user_data) {
                        auto *app = static_cast<UiApp *>(user_data);
                        if (app && app->activeScreen() == &app->_scrSystem)
                            (void)app->openActionPane();
                    },
                    this) != LV_RES_OK) {
                (void)openActionPane();
            }
            break;
        }
#endif
#if HELTEC_UI_ENABLE_KEYBOARD
        auto *req = static_cast<const UiMessageKeyboardRequest *>(event->payload);
        const bool message_active = _surfaces.isActive(&_sendMessageOvl);
        const bool preset_active = activeScreen() == &_scrSystem;
        if (!req || (!message_active && !preset_active)) break;
        if (!_keyboardOvl.prepareMessageInput(*req)) break;
        _keyboard_reply = event->source;
        if (lv_async_call(
            +[](void *user_data) {
                auto *app = static_cast<UiApp *>(user_data);
                if (!app) return;
                const bool message_active = app->_surfaces.isActive(&app->_sendMessageOvl);
                const bool preset_active = app->activeScreen() == &app->_scrSystem;
                if (!message_active && !preset_active) {
                    app->_keyboard_reply = nullptr;
                    return;
                }
                app->presentMessageKeyboard();
                if (!app->_surfaces.isVisible(&app->_keyboardOvl)) {
                    app->_keyboard_reply = nullptr;
                }
            },
            this) != LV_RES_OK) {
            presentMessageKeyboard();
            if (!_surfaces.isVisible(&_keyboardOvl)) _keyboard_reply = nullptr;
        }
#endif
        break;
    }
    case UiMessageType::WaypointKeyboardOpen:
#if HELTEC_UI_ENABLE_KEYBOARD
        _keyboard_reply = event->source;
        openWaypointKeyboard();
        if (!_surfaces.isVisible(&_keyboardOvl)) _keyboard_reply = nullptr;
#endif
        break;
    case UiMessageType::MessageKeyboardSubmit: {
        auto *submit = static_cast<const UiMessageKeyboardSubmit *>(event->payload);
        if (submit && submit->text) {
            if (submit->purpose == UiMessageKeyboardPurpose::EditPreset) {
#if HELTEC_UI_ENABLE_KEYBOARD
                if (_keyboard_reply && lv_obj_is_valid(_keyboard_reply)) {
                    (void)sendUiMessage(_keyboard_reply, UiMessageType::MessageKeyboardSubmit,
                                        submit);
                }
                _keyboard_reply = nullptr;
#endif
            } else if (_surfaces.isVisible(&_sendMessageOvl)) {
                _sendMessageOvl.submitCustomMessage(submit->text);
            }
        }
        break;
    }
    case UiMessageType::WaypointKeyboardSubmit: {
        auto *submit = static_cast<const UiWaypointKeyboardSubmit *>(event->payload);
#if HELTEC_UI_ENABLE_KEYBOARD
        if (submit && _keyboard_reply && lv_obj_is_valid(_keyboard_reply)) {
            (void)sendUiMessage(_keyboard_reply, UiMessageType::WaypointKeyboardSubmit, submit);
        }
        _keyboard_reply = nullptr;
#else
        (void)submit;
#endif
        break;
    }
    default:
        break;
    }
#if defined(HELTEC_HAS_KEY_FRAME) && HELTEC_HAS_KEY_FRAME
    switch (event->type) {
    case UiMessageType::NavOpen:
    case UiMessageType::NavClose:
    case UiMessageType::NavActivity:
        // Navigation owns a refresh session and coalesces rapid selection changes.
        break;
    default:
        Heltec::MeshCore::Dal::DisplayPort::requestRefresh();
        break;
    }
#endif
}

void UiApp::handleAppStateEvent(const AppStateEvent &event) {
    if (event.type == AppStateEventType::ConfigChanged) {
        refreshTopPaneStatus(true);
    }
    if (event.type == AppStateEventType::PreviewChanged) {
        if (ui_task().isPreviewActive()) {
            openPreviewOverlay(ui_task().previewUnread(), ui_task().previewReceivedMillis(),
                               ui_task().previewOrigin(), ui_task().previewText());
        } else {
            (void)_surfaces.show(&_previewOvl, false);
        }
    }
    if (event.type == AppStateEventType::AlertChanged) {
        if (ui_task().isAlertActive()) {
            openAlertOverlay(ui_task().getAlertText());
        } else {
            (void)_surfaces.show(&_alertOvl, false);
        }
    }
#if defined(HELTEC_HAS_KEY_FRAME) && HELTEC_HAS_KEY_FRAME
    Heltec::MeshCore::Dal::DisplayPort::requestDeferredRefresh();
#endif
}

void UiApp::handlePowerChanged(Heltec::MeshCore::Power::PowerChangeMask changes,
                               const Heltec::MeshCore::Power::PowerSnapshot &snapshot) {
    using Heltec::MeshCore::Power::PowerChange;
    if (Heltec::MeshCore::Power::hasChange(changes, PowerChange::Battery) ||
        Heltec::MeshCore::Power::hasChange(changes, PowerChange::Source)) {
        _top_pane.setBatteryStatus(snapshot.battery_mv, snapshot.battery_percent);
    }
    if (!Heltec::MeshCore::Power::hasChange(changes, PowerChange::Display)) return;
    if (snapshot.display_on) {
        onBacklightTurnedOn();
    } else {
        ensureTileKeypadFocus();
    }
}

void UiApp::tick()
{
    if (!_inited)
    {
        return;
    }
    if (_power.snapshot().display_on)
    {
        lv_timer_handler();
    }
    refreshTopPaneStatus();
    // Shared approval UI also serves leaders without a Tracker screen.
    // A join request is a device-level Team event. Do not gate it on the
    // currently focused screen; Tracker and Find Friend must both receive it.
    const Biz::IBizFacade::TeamStatus team = _biz.teamStatus();
    if (team.is_leader && !_surfaces.isVisible(&_confirmOvl) && team.pending_session != 0) {
        _confirmOvl.setRequest(UiConfirmAction::TeamJoin);
        (void)_surfaces.show(&_confirmOvl);
    }
    syncDisplayTimeoutInhibit(millis());
}

bool UiApp::initScreens(_lv_obj_t *content) {
    if (!content) return false;

    AbstractScreen *screens[static_cast<uint8_t>(ScreenId::Count)];
    uint8_t screen_count = 0;
    screens[screen_count++] = &_scrHome;
    screens[screen_count++] = &_scrRecent;
    screens[screen_count++] = &_scrRadio;
#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
    screens[screen_count++] = &_scrCompass;
#endif
#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
    screens[screen_count++] = &_scrFindFriend;
#endif
#if ENV_INCLUDE_GPS
    screens[screen_count++] = &_scrGPS;
#endif
#if defined(ENV_INCLUDE_MAP) && ENV_INCLUDE_MAP
    screens[screen_count++] = &_scrTracker;
#endif
    screens[screen_count++] = &_scrSystem;

    lv_obj_t *const screen_host = ht_class_create(&ht_app_screen_host_class, content);
    if (!screen_host) return false;
    lv_obj_set_size(screen_host, lv_pct(100), lv_pct(100));
    lv_obj_clear_flag(screen_host, LV_OBJ_FLAG_SCROLLABLE);

    _tileview = lv_tileview_create(screen_host);
    if (!_tileview) return false;
    lv_obj_set_size(_tileview, lv_pct(100), lv_pct(100));
    lv_obj_set_scrollbar_mode(_tileview, LV_SCROLLBAR_MODE_OFF);
    if (_frame_root) lv_obj_update_layout(_frame_root);
    lv_obj_update_layout(_tileview);

    for (uint8_t i = 0; i < screen_count; ++i) {
        AbstractScreen *const scr = screens[i];
        if (!scr) continue;

#if HELTEC_UI_HAS_TOUCH
        lv_obj_t *tile = lv_tileview_add_tile(_tileview, i, 0, LV_DIR_NONE);
#else
        lv_obj_t *tile = lv_tileview_add_tile(_tileview, i, 0, LV_DIR_HOR);
#endif
        if (!tile) {
            return false;
        }
        lv_obj_set_user_data(tile, scr);

        lv_obj_set_scrollbar_mode(tile, LV_SCROLLBAR_MODE_OFF);
        lv_obj_clear_flag(tile, LV_OBJ_FLAG_SCROLLABLE);
        const lv_obj_class_t *root_class = &lv_obj_class;
        if (scr == &_scrSystem) root_class = &ht_system_screen_root_class;
#if defined(ENV_INCLUDE_MAP) && ENV_INCLUDE_MAP
        if (scr == &_scrTracker) root_class = &ht_tracker_screen_root_class;
#endif
        if (!scr->init(tile, _frame_root, *root_class)) {
            return false;
        }
        if (!_surfaces.registerSurface(scr)) {
            return false;
        }
        lv_obj_t *scr_root = scr->root();
        if (!scr_root) {
            return false;
        }
    }

    relayout_tileview_tiles(_tileview);

    lv_obj_t *first_tile = lv_obj_get_child(_tileview, 0);
    if (first_tile) lv_obj_set_tile(_tileview, first_tile, LV_ANIM_OFF);
#if HELTEC_UI_HAS_TOUCH
    lv_obj_clear_flag(_tileview, LV_OBJ_FLAG_SCROLLABLE);
#else
    lv_obj_set_scroll_dir(_tileview, LV_DIR_HOR);
#endif

    if (AbstractScreen *home = screenAt(0)) _top_pane.setTitle(home->title());
    return true;
}

bool UiApp::initTimers() {
    if (!ui_motion_init()) return false;
    if (!_app_state_dispatcher.createTimer()) return false;
    if (!_nav_auto_commit_timer) {
        _nav_auto_commit_timer = lv_timer_create(navigationAutoCommitTimerCb, 1U, this);
        if (!_nav_auto_commit_timer) return false;
        lv_timer_set_repeat_count(_nav_auto_commit_timer, -1);
        lv_timer_pause(_nav_auto_commit_timer);
    }
#if HELTEC_UI_HAS_TOUCH
    if (!_deferred_touch_timer) {
        _deferred_touch_timer = lv_timer_create(deferredTouchActionTimerCb, 40U, this);
        if (!_deferred_touch_timer) return false;
        lv_timer_set_repeat_count(_deferred_touch_timer, -1);
        lv_timer_pause(_deferred_touch_timer);
    }
#endif
    restartNavigationAutoCommitTimer();
    return true;
}

void UiApp::notifyDisplayActivity(uint32_t now_ms) {
    _power.notifyUserActivity(now_ms);
}

bool UiApp::isDisplayOn() const {
    return _power.snapshot().display_on;
}

void UiApp::onBacklightTurnedOn() {
    ensureTileKeypadFocus();
    lv_obj_t *scr = lv_scr_act();
    if (scr) lv_obj_invalidate(scr);
}

void UiApp::setDisplayAutoOffMs(uint32_t ms) {
    if (BacklightPolicy::mode() == BacklightMode::ManualToggle) {
        ms = 0;
    }
    _power.setDisplayAutoOffMs(ms, millis());
}

void UiApp::syncDisplayTimeoutInhibit(uint32_t now_ms) {
    constexpr uint16_t kCriticalOverlay = 1U << 0;
    const bool inhibited =
        _surfaces.isActive(&_radioParamSyncOvl) || _surfaces.isActive(&_repeatModeOvl);
    _power.setDisplayTimeoutInhibited(kCriticalOverlay, inhibited, now_ms);
}

} // namespace Heltec::MeshCore::Ui
