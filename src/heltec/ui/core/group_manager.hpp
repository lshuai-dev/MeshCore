#pragma once

#include <stdint.h>
#include <lvgl.h>

struct _lv_obj_t;
struct _lv_group_t;

namespace heltec::meshcore::ui {

struct GroupBinding {
  lv_group_t* group;
  lv_obj_t* default_focus;
};

/** Owns keypad indev to lv_group_t binding. SurfaceManager owns surface stack order. */
class GroupManager {
 public:
  static GroupManager& instance();

  void bind(lv_group_t* group, lv_obj_t* default_focus = nullptr);
  void clear();

  GroupBinding current() const { return _current; }
  lv_group_t* currentGroup() const { return _current.group; }
  void reconcile();

 private:
  void leaveGroup(lv_group_t* group);
  void enterBinding(const GroupBinding& binding);
  lv_obj_t* resolveFocus(lv_group_t* group, lv_obj_t* preferred) const;

  GroupBinding _current{};
};

}  // namespace heltec::meshcore::ui
