#pragma once

#include "ui_surface.hpp"

#include <lvgl.h>
#include <stdint.h>

namespace heltec::meshcore::ui {

using MenuHandlerFn = void (*)(biz::IBizFacade&);
enum MenuKind : uint8_t { Command, Menu };

class AbstractMenu;

struct MenuActionView {
  MenuKind kind = MenuKind::Command;
  const char* label = nullptr;
  MenuHandlerFn cmd = nullptr;
  AbstractMenu* submenu = nullptr;
};

class AbstractMenu : public UiSurface {
 public:
  explicit AbstractMenu(biz::IBizFacade& biz) : UiSurface(biz) {}
  AbstractMenu(const AbstractMenu&) = delete;
  AbstractMenu& operator=(const AbstractMenu&) = delete;
  AbstractMenu(AbstractMenu&&) = delete;
  AbstractMenu& operator=(AbstractMenu&&) = delete;
  virtual ~AbstractMenu() = default;

  virtual lv_obj_t* page() const = 0;
  virtual lv_obj_t* root() const override = 0;

  virtual void resetInputState(bool focus_first_item = true) {}
  virtual uint8_t actionCount() const { return 0; }
  virtual bool actionAt(uint8_t index, MenuActionView& out) const {
    (void)index;
    (void)out;
    return false;
  }

  virtual bool create(lv_obj_t* menu, const char* parent_title) = 0;
};

}  // namespace heltec::meshcore::ui
