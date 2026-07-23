#pragma once
#include <lvgl.h>
#include <stdint.h>
#include "app_state_event.hpp"
#include "biz_facade.hpp"
#include "ui_events.h"

struct _lv_obj_t;
namespace heltec::meshcore::ui {
class UiSurface {
 public:
  explicit UiSurface(biz::IBizFacade& biz) : _biz(biz) {}
  virtual ~UiSurface() = default;

  virtual _lv_obj_t* create(_lv_obj_t* parent);
  virtual _lv_obj_t* root() const { return _root; }
  void setTarget(_lv_obj_t* target) { _event_target = target; }
  virtual lv_group_t* group() const { return _focus_group; }
  virtual _lv_obj_t* focusedObject() const;
  virtual bool canPresent() const;
  virtual bool canBindInput() const;
  virtual uint16_t inputRebindDelayMs() const { return 0; }
  virtual void onEnter() {}
  virtual void onExit() {}
 protected:
  virtual _lv_obj_t* createRoot(_lv_obj_t* parent);
  virtual void onAppStateChanged(const AppStateEvent& event) { (void)event; }
  virtual void onRefreshRequested() {}
  bool emitEvent(UiEventType type, const void* payload = nullptr) const;
  virtual _lv_obj_t* defaultFocus() const;
  virtual bool onKey(uint32_t key) { (void)key; return false; }

  void addFocusObject(_lv_obj_t* obj);
  void clearFocusObjects();

  _lv_obj_t* _root = nullptr;
  _lv_obj_t* _event_target = nullptr;
  lv_group_t* _focus_group = nullptr;
  _lv_obj_t* _default_focus = nullptr;
  biz::IBizFacade& _biz;
};

}  // namespace heltec::meshcore::ui
