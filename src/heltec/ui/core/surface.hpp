#pragma once
#include "AppStateEvent.hpp"
#include "BizFacade.hpp"
#include "HtGridNav.hpp"
#include "HtEvent.h"

namespace Heltec::MeshCore::Ui {
class SurfaceManager;

class UiSurface {
  public:
    explicit UiSurface(Biz::IBizFacade &biz, const lv_obj_class_t &root_class = lv_obj_class);
    UiSurface(const UiSurface &) = delete;
    UiSurface &operator=(const UiSurface &) = delete;
    UiSurface(UiSurface &&) = delete;
    UiSurface &operator=(UiSurface &&) = delete;
    virtual ~UiSurface() = default;

    bool init(lv_obj_t *parent, lv_obj_t *event_target = nullptr);
    lv_obj_t *root() const {
        return _root;
    }

    virtual void onEnter() { }
    virtual void onExit() { }

  protected:
    virtual bool buildSurface() = 0;
    virtual void onAppStateChanged(const AppStateEvent &event) {
        (void)event;
    }

    virtual void onActivated() {
    }

    virtual void onUIEvent(const UiEvent &event) {
        (void)event;
    }
    virtual bool onKey(uint32_t key) {
        (void)key;
        return false;
    }

    bool emitEvent(EventType type, const void *payload = nullptr) const;
    bool showAlert(const char *text, uint16_t duration_ms) const;
    lv_obj_t *eventTarget() const {
        return _event_target;
    }

    lv_obj_t *_root = nullptr;
    Biz::IBizFacade &_biz;
    HtGridNav _gridNav;

  private:
    friend class SurfaceManager;

    bool installEventHandlers();
    static void handleKeyEvent(lv_event_t *event);
    static void handleFocusedEvent(lv_event_t *event);
    static void handleUIEvent(lv_event_t *event);

    const lv_obj_class_t &_root_class;
    lv_obj_t *_event_target = nullptr;
    bool _inited = false;
};

} // namespace Heltec::MeshCore::Ui
