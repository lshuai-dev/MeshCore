#include "group_manager.hpp"

#include "heltec/drivers/input/key_input.hpp"

#include <lvgl.h>

namespace heltec::meshcore::ui {

namespace {

GroupBinding makeBinding(lv_group_t* group, lv_obj_t* default_focus) {
  GroupBinding binding;
  binding.group = group;
  binding.default_focus = default_focus;
  return binding;
}

}  // namespace

GroupManager& GroupManager::instance() {
  static GroupManager manager;
  return manager;
}

lv_obj_t* GroupManager::resolveFocus(lv_group_t* group, lv_obj_t* preferred) const {
  if (preferred && lv_obj_is_valid(preferred) && lv_obj_get_group(preferred) == group) {
    return preferred;
  }
  if (lv_obj_t* focused = lv_group_get_focused(group)) {
    if (lv_obj_is_valid(focused)) return focused;
  }
  return nullptr;
}

void GroupManager::leaveGroup(lv_group_t* group) {
  if (!group) return;
  // Do not synthesize LV_EVENT_DEFOCUSED when switching between independent
  // surface groups. LVGL sends focus/defocus events within a group; across
  // modal groups the old group is simply detached from the input device and
  // restored unchanged on pop.
}

void GroupManager::enterBinding(const GroupBinding& binding) {
  if (!binding.group) return;
  const bool preserve_editing = lv_group_get_editing(binding.group);
  lv_group_set_default(binding.group);
  heltec::meshcore::dal::key_input::set_group(binding.group);
  if (lv_obj_t* target = resolveFocus(binding.group, binding.default_focus)) {
    lv_group_focus_obj(target);
  } else if (lv_group_get_obj_count(binding.group) > 0) {
    lv_group_focus_next(binding.group);
  }
  if (preserve_editing) lv_group_set_editing(binding.group, true);
  _current = binding;
}

void GroupManager::bind(lv_group_t* group, lv_obj_t* default_focus) {
  if (!group) return;
  if (group == _current.group && default_focus == _current.default_focus) {
    reconcile();
    return;
  }
  leaveGroup(_current.group);
  enterBinding(makeBinding(group, default_focus));
}

void GroupManager::clear() {
  leaveGroup(_current.group);
  heltec::meshcore::dal::key_input::set_group(nullptr);
  _current = {};
}

void GroupManager::reconcile() {
  if (!_current.group) return;
  if (lv_group_get_obj_count(_current.group) == 0) return;
  if (lv_obj_t* focused = lv_group_get_focused(_current.group)) {
    if (lv_obj_is_valid(focused)) return;
  }
  if (lv_obj_t* target = resolveFocus(_current.group, _current.default_focus)) {
    lv_group_focus_obj(target);
  } else if (lv_group_get_obj_count(_current.group) > 0) {
    lv_group_focus_next(_current.group);
  }
}

}  // namespace heltec::meshcore::ui
