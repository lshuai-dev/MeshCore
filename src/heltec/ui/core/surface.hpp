#pragma once
#include "app_state_event.hpp"
#include "biz_facade.hpp"
#include "ht_event.h"

namespace heltec::meshcore::ui {
class SurfaceManager;

class UiSurface {
  public:
    explicit UiSurface(biz::IBizFacade &biz, const lv_obj_class_t &root_class = lv_obj_class);
    UiSurface(const UiSurface &) = delete;
    UiSurface &operator=(const UiSurface &) = delete;
    UiSurface(UiSurface &&) = delete;
    UiSurface &operator=(UiSurface &&) = delete;
    virtual ~UiSurface() = default;

    bool init(lv_obj_t *parent, lv_obj_t *event_target = nullptr);
    lv_obj_t *root() const {
        return _root;
    }
    lv_group_t *group() const {
        return _focus_group;
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
    lv_group_t *focusGroup() const {
        return _focus_group;
    }
    lv_obj_t *eventTarget() const {
        return _event_target;
    }

    lv_obj_t *_root = nullptr;
    biz::IBizFacade &_biz;

  private:
    friend class SurfaceManager;

    bool createFocusGroup();
    bool installEventHandlers();
    static void handleKeyEvent(lv_event_t *event);
    static void handleUIEvent(lv_event_t *event);

    const lv_obj_class_t &_root_class;
    lv_obj_t *_event_target = nullptr;
    lv_group_t *_focus_group = nullptr;
    bool _inited = false;
};

} // namespace heltec::meshcore::ui
