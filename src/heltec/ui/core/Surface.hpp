#pragma once
#include "AppStateEvent.hpp"
#include "BizFacade.hpp"
#include "HtGridNav.hpp"
#include "HtEvent.h"

namespace Heltec::MeshCore::Ui {
class SurfaceManager;
class UiSurface {
  public:
    explicit UiSurface(Biz::IBizFacade &biz);
    UiSurface(const UiSurface &) = delete;
    UiSurface &operator=(const UiSurface &) = delete;
    UiSurface(UiSurface &&) = delete;
    UiSurface &operator=(UiSurface &&) = delete;
    virtual ~UiSurface() = default;

    bool init(lv_obj_t *parent, lv_obj_t *app_target = nullptr,
              const lv_obj_class_t &root_class = lv_obj_class);
    lv_obj_t *root() const {
        return _root;
    }

  protected:
    virtual bool build() = 0;
    virtual void onEnter() { }
    virtual void onExit() { }
    virtual void onAppStateChanged(const AppStateEvent &event) {
        (void)event;
    }

    virtual void onActivated() {
    }

    virtual void onAppEvent(const UiEvent &event) {
        (void)event;
    }
    virtual bool acceptsAppEvents() const {
        return false;
    }
    virtual bool onKey(uint32_t key) {
        (void)key;
        return false;
    }

    bool notifyApp(EventType type, const void *payload = nullptr) const;
    bool showAlert(const char *text, uint16_t duration_ms) const;

    lv_obj_t *_root = nullptr;
    Biz::IBizFacade &_biz;
    HtGridNav _gridNav;

  private:
    friend class SurfaceManager;

    static void handleKeyEvent(lv_event_t *event);
    static void handleFocusedEvent(lv_event_t *event);
    static void handleAppEvent(lv_event_t *event);

    lv_obj_t *_app_target = nullptr;
};

} // namespace Heltec::MeshCore::Ui
