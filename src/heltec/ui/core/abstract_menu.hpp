#pragma once

#include "biz_facade.hpp"

#include <lvgl.h>
#include <stdint.h>

namespace heltec::meshcore::ui
{

using MenuHandlerFn = void (*)(biz::IBizFacade &, _lv_obj_t *event_target);
enum MenuKind : uint8_t
{
    Command,
    Menu
};

class AbstractMenu;

class AbstractMenu
{
  public:
    explicit AbstractMenu(biz::IBizFacade &biz) : _biz(biz)
    {
    }
    AbstractMenu(const AbstractMenu &) = delete;
    AbstractMenu &operator=(const AbstractMenu &) = delete;
    AbstractMenu(AbstractMenu &&) = delete;
    AbstractMenu &operator=(AbstractMenu &&) = delete;
    virtual ~AbstractMenu() = default;

    virtual lv_obj_t *page() const = 0;
    virtual lv_obj_t *root() const = 0;
    virtual void resetInputState(bool focus_first_item = true)
    {
    }

    virtual bool create(lv_obj_t *menu, lv_group_t *group, const char *parent_title,
                        lv_obj_t *event_target = nullptr) = 0;

  protected:
    biz::IBizFacade &_biz;
    lv_obj_t *_event_target = nullptr;
};

} // namespace heltec::meshcore::ui
