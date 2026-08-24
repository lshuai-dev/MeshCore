#pragma once
#include "app_state_event.hpp"
#include "biz_facade.hpp"
#include "ht_meta_data.hpp"
#include "ht_event.h"

struct _lv_obj_t;
namespace heltec::meshcore::ui
{
class UiSurface
{
  public:
    explicit UiSurface(biz::IBizFacade &biz) : _biz(biz)
    {
    }
    virtual ~UiSurface() = default;

    /** Initialize the surface once during UI startup. */
    bool init(_lv_obj_t *parent);
    virtual _lv_obj_t *root() const
    {
        return _root;
    }
    /** Sets the object that receives events emitted through emit(). */
    void setEventTarget(_lv_obj_t *target)
    {
        _event_target = target;
    }
    virtual lv_group_t *group() const
    {
        return _focus_group;
    }
    virtual _lv_obj_t *focusedObject() const;
    virtual void onEnter()
    {
    }
    virtual void onExit()
    {
    }

  protected:
    /** Create the LVGL object tree. Called only by init(). */
    virtual _lv_obj_t *create(_lv_obj_t *parent);
    virtual _lv_obj_t *createRoot(_lv_obj_t *parent);
    virtual void onAppStateChanged(const AppStateEvent &event)
    {
        (void)event;
    }
    virtual void onSurfaceRefresh()
    {
    }
    virtual void onUiEvent(const UiEvent &event)
    {
        (void)event;
    }
    bool emit(EventType type, const void *payload = nullptr) const;
    _lv_obj_t *defaultFocus() const;
    virtual bool onKey(uint32_t key)
    {
        (void)key;
        return false;
    }

    void addFocusObject(_lv_obj_t *obj);
    void clearFocusObjects();

    _lv_obj_t *_root = nullptr;
    _lv_obj_t *_event_target = nullptr;
    lv_group_t *_focus_group = nullptr;
    _lv_obj_t *_default_focus = nullptr;
    biz::IBizFacade &_biz;
};

} // namespace heltec::meshcore::ui
