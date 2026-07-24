#include "cascading_menu.hpp"

#if LV_USE_MENU
#include "ui/core/ui_events.h"
#include "ui/core/ht_meta_data.hpp"
#include <cstdio>
#include <cstring>
#include <lvgl.h>

namespace heltec::meshcore::ui {
namespace {

static lv_style_t s_menu_page_style;
static lv_style_t s_menu_item_style;
static bool s_menu_page_style_ready = false;
static bool s_menu_item_style_ready = false;

static constexpr lv_coord_t kMenuItemPadHorPx = 4;

static const lv_style_selector_t kMenuPageSelectors[] = {
    LV_PART_MAIN,
    LV_PART_MAIN | LV_STATE_FOCUSED,
    LV_PART_MAIN | LV_STATE_FOCUS_KEY,
    LV_PART_MAIN | LV_STATE_PRESSED,
};

static const lv_style_selector_t kMenuItemSelectors[] = {
    LV_PART_MAIN,
    LV_PART_MAIN | LV_STATE_CHECKED,
    LV_PART_MAIN | LV_STATE_FOCUSED,
    LV_PART_MAIN | LV_STATE_FOCUS_KEY,
    LV_PART_MAIN | LV_STATE_PRESSED,
};

void initMenuPageStyle() {
  if (s_menu_page_style_ready) return;
  lv_style_init(&s_menu_page_style);
  lv_style_set_width(&s_menu_page_style, lv_pct(100));
  lv_style_set_height(&s_menu_page_style, lv_pct(100));
  lv_style_set_pad_all(&s_menu_page_style, 0);
  lv_style_set_pad_row(&s_menu_page_style, 0);
  lv_style_set_border_width(&s_menu_page_style, 0);
  lv_style_set_outline_width(&s_menu_page_style, 0);
  s_menu_page_style_ready = true;
}

void applyMenuPageStyle(lv_obj_t* obj) {
  if (!obj) return;
  initMenuPageStyle();
  for (lv_style_selector_t selector : kMenuPageSelectors) {
    lv_obj_add_style(obj, &s_menu_page_style, selector);
  }
  lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

void initMenuItemStyle() {
  if (s_menu_item_style_ready) return;
  lv_style_init(&s_menu_item_style);
  lv_style_set_pad_left(&s_menu_item_style, kMenuItemPadHorPx);
  lv_style_set_pad_right(&s_menu_item_style, kMenuItemPadHorPx);
  s_menu_item_style_ready = true;
}

void applyMenuItemStyle(lv_obj_t* obj) {
  if (!obj) return;
  initMenuItemStyle();
  for (lv_style_selector_t selector : kMenuItemSelectors) {
    lv_obj_add_style(obj, &s_menu_item_style, selector);
  }
}

}  // namespace

bool CascadingMenu::actionAt(uint8_t index, MenuActionView& out) const {
  if (index >= _item_count) return false;
  const MenuItem& item = _items[index];
  out.kind = item.kind;
  out.label = item.label;
  out.cmd = (item.kind == MenuKind::Command) ? item.cmd : nullptr;
  out.submenu = (item.kind == MenuKind::Menu) ? item.submenu : nullptr;
  return true;
}

lv_group_t* CascadingMenu::group() const {
  if (_active_child && _active_child->group()) return _active_child->group();
  return _focus_group;
}

_lv_obj_t* CascadingMenu::focusedObject() const {
  if (_active_child) {
    if (lv_obj_t* focused = _active_child->focusedObject()) return focused;
  }
  if (_focus_group) {
    if (lv_obj_t* focused = lv_group_get_focused(_focus_group)) return focused;
  }
  return _roller.root();
}

bool CascadingMenu::enterSubmenuAt(uint8_t index) {
  if (index >= _item_count || !_menu_host) return false;
  MenuItem& item = _items[index];
  if (item.kind != MenuKind::Menu || !item.submenu || !item.submenu->page()) return false;

  _focused_index = index;
  _active_child = item.submenu;
  lv_menu_set_page(_menu_host, item.submenu->page());
  item.submenu->resetInputState(true);
  ui_event_send(_menu_host, UiEventType::RebindInput);
  return true;
}

void CascadingMenu::leaveSubmenu() {
  if (!_active_child || !_menu_host || !_page) return;
  _active_child->resetInputState(false);
  _active_child = nullptr;
  lv_menu_set_page(_menu_host, _page);
  resetInputState(true);
  ui_event_send(_menu_host, UiEventType::RebindInput);
}

bool CascadingMenu::create(lv_obj_t* menu, const char* title) {
  if (!menu || !title) return false;
  if (_page && _roller.root()) return true;

  _menu_host = menu;

  _page = lv_menu_page_create(menu, title && title[0] ? const_cast<char*>(title) : nullptr);
  if (!_page) return false;
  applyMenuPageStyle(_page);
  ht_set_user_data(_page, this);

  if (!_focus_group) {
    _focus_group = lv_group_create();
    if (!_focus_group) {
      lv_obj_del(_page);
      _page = nullptr;
      return false;
    }
    lv_group_set_wrap(_focus_group, true);
    lv_group_set_editing(_focus_group, false);
  }
  _root = _page;

  if (!_roller.create(_page, _focus_group)) {
    lv_obj_del(_page);
    _page = nullptr;
    return false;
  }

  lv_group_focus_freeze(_focus_group, true);
  char buf[kPageTitleLen] = {0};
  for (uint8_t i = 0; i < _item_count; ++i) {
    if(_items[i].label) {
      memset(buf, 0, sizeof(buf));
      snprintf(buf, sizeof(buf) - 1, "%s%s", _items[i].label, _items[i].kind == MenuKind::Menu ? "..." : "");
      lv_obj_t* const btn = _roller.addItem(buf);
      if (!btn) continue;
      applyMenuItemStyle(btn);
      ht_set_user_data(btn, this);
      lv_obj_add_event_cb(btn, [](lv_event_t* e) {
        const lv_event_code_t code = lv_event_get_code(e);
        if (code != LV_EVENT_FOCUSED && code != LV_EVENT_CLICKED) return;
        CascadingMenu* const menu =
            static_cast<CascadingMenu*>(ht_user_data(lv_event_get_target(e)));
        if (!menu) return;
        const uint32_t index = lv_obj_get_index(lv_event_get_target(e));
        if (index >= menu->_item_count) return;
        menu->_focused_index = static_cast<uint8_t>(index);
        if (code != LV_EVENT_FOCUSED) {
          (void)menu->_roller.focusItem(menu->_focused_index, true);
        }
      }, LV_EVENT_ALL, this);

      if (MenuKind::Menu == _items[i].kind && _items[i].submenu && menu) {
        memset(buf, 0, sizeof(buf));
        snprintf(buf, sizeof(buf) - 1, "%s > %s", title ? title : "", _items[i].label ? _items[i].label : "");
        _items[i].submenu->setTarget(_event_target);
        if (_items[i].submenu->create(menu, buf)) {
          lv_obj_add_event_cb(btn, [](lv_event_t* e) {
            if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

            CascadingMenu* const menu =
                static_cast<CascadingMenu*>(ht_user_data(lv_event_get_target(e)));
            if (!menu) return;

            const uint32_t index = lv_obj_get_index(lv_event_get_target(e));
            if (index >= menu->_item_count) return;

            if (menu->enterSubmenuAt(static_cast<uint8_t>(index))) {
              lv_event_stop_bubbling(e);
              lv_event_stop_processing(e);
            }
          }, LV_EVENT_ALL, this);

          if(_items[i].submenu->root()) {
            lv_obj_add_event_cb(_items[i].submenu->root(), [](lv_event_t* e) {
              if (lv_event_get_code(e) == LV_EVENT_KEY && LV_KEY_ESC == lv_event_get_key(e)) {
                CascadingMenu* cm = static_cast<CascadingMenu*>(lv_event_get_user_data(e));
                if(cm) {
                  cm->leaveSubmenu();
                  lv_event_stop_bubbling(e);
                  lv_event_stop_processing(e);
                }
              }
            }, LV_EVENT_KEY, this);
          }
        }
      }

      if (MenuKind::Command == _items[i].kind) {
        lv_obj_add_event_cb(btn, [](lv_event_t* e) {
          if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
          MenuItem* const item = static_cast<MenuItem*>(lv_event_get_user_data(e));
          if (!item || !item->cmd) return;
          CascadingMenu* const menu =
              static_cast<CascadingMenu*>(ht_user_data(lv_event_get_target(e)));
          if (menu) {
            item->cmd(menu->_biz);
          }
          lv_event_stop_bubbling(e);
          lv_event_stop_processing(e);
        }, LV_EVENT_ALL, &_items[i]);
      }
    }
  }
  lv_group_focus_freeze(_focus_group, false);

  return true;
}

bool CascadingMenu::addCommandHandler(const char* label, MenuHandlerFn action) {
  if (!label || !action || _item_count >= kMaxItems) return false;
  _items[_item_count].kind = MenuKind::Command;
  _items[_item_count].label = label;
  _items[_item_count].cmd = action;
  ++_item_count;
  return true;
}

void CascadingMenu::resetInputState(bool focus_first_item) {
  _active_child = nullptr;
  if (!focus_first_item) {
    _roller.resetFocus(false);
    return;
  }
  if (_item_count == 0) {
    _roller.resetFocus(true);
    return;
  }
  if (_focused_index >= _item_count) _focused_index = 0;
  if (!_roller.focusItem(_focused_index, true)) {
    _roller.resetFocus(true);
  }
}

bool CascadingMenu::addMenu(const char* label, AbstractMenu& submenu) {
  if (!label || _item_count >= kMaxItems) return false;
  _items[_item_count].kind = MenuKind::Menu;
  _items[_item_count].label = label;
  _items[_item_count].submenu = &submenu;
  ++_item_count;
  return true;
}
}  // namespace heltec::meshcore::ui

#endif  // LV_USE_MENU
