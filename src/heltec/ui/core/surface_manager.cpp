#include "surface_manager.hpp"

#include "group_manager.hpp"
#include "ui_surface.hpp"

#include <lvgl.h>

namespace heltec::meshcore::ui {

UiSurface* SurfaceManager::active() const {
  return _modal_depth ? _modals[_modal_depth - 1].surface : _root;
}

UiSurface* SurfaceManager::topModal() const {
  return _modal_depth ? _modals[_modal_depth - 1].surface : nullptr;
}

void SurfaceManager::enter(UiSurface* surface) {
  if (!surface) return;
  lv_obj_t* root = surface->root();
  if (root && lv_obj_is_valid(root)) {
    lv_obj_clear_flag(root, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(root);
  }
  surface->onEnter();
}

void SurfaceManager::exit(UiSurface* surface) {
  if (!surface) return;
  lv_obj_t* root = surface->root();
  if (!root || !lv_obj_is_valid(root)) return;
  surface->onExit();
  if (surface != _root) lv_obj_add_flag(root, LV_OBJ_FLAG_HIDDEN);
}

void SurfaceManager::bindInput(UiSurface* surface) {
  if (inputRebindPending()) {
    GroupManager::instance().clear();
    _bound_surface = nullptr;
    return;
  }
  if (!canBindInput(surface)) {
    GroupManager::instance().clear();
    _bound_surface = nullptr;
    return;
  }
  const bool binding_changed = _bound_surface != surface;
  GroupManager::instance().bind(surface->group(), surface->focusedObject());
  _bound_surface = surface;
  if (binding_changed) {
    ui_event_send(surface->root(), UiEventType::SurfaceRefresh);
  }
}

bool SurfaceManager::canPresent(const UiSurface* surface) const {
  return surface && surface->canPresent();
}

bool SurfaceManager::canBindInput(const UiSurface* surface) const {
  return surface && surface->canBindInput();
}

bool SurfaceManager::enqueue(PendingOpType type, UiSurface* surface, UiSurface* owner) {
  if (_pending_count >= kMaxPendingOps) return false;
  _pending[_pending_count++] = PendingOp{type, surface, owner};
  return true;
}

bool SurfaceManager::beginMutation(PendingOpType type, UiSurface* surface) {
  (void)type;
  (void)surface;
  if (!_mutating) {
    _mutating = true;
    return true;
  }
  return false;
}

void SurfaceManager::endMutation() {
  _mutating = false;
  drainPending();
}

bool SurfaceManager::inputRebindPending() const {
  return _input_rebind_pending;
}

bool SurfaceManager::createTimers() {
  return createInputRebindTimer();
}

bool SurfaceManager::createInputRebindTimer() {
  if (_input_rebind_timer) return true;
  _input_rebind_timer = lv_timer_create(inputRebindTimerCb, 1U, this);
  if (!_input_rebind_timer) return false;
  lv_timer_set_repeat_count(_input_rebind_timer, -1);
  lv_timer_pause(_input_rebind_timer);
  return true;
}

void SurfaceManager::scheduleInputRebind(uint16_t delay_ms) {
  if (delay_ms == 0) {
    if (_input_rebind_timer) lv_timer_pause(_input_rebind_timer);
    _input_rebind_pending = false;
    bindInput(active());
    return;
  }

  _input_rebind_pending = true;
  GroupManager::instance().clear();
  _bound_surface = nullptr;
  if (!createInputRebindTimer()) {
    finishInputRebind();
    return;
  }
  lv_timer_set_period(_input_rebind_timer, delay_ms);
  lv_timer_set_repeat_count(_input_rebind_timer, -1);
  lv_timer_reset(_input_rebind_timer);
  lv_timer_resume(_input_rebind_timer);
}

void SurfaceManager::finishInputRebind() {
  _input_rebind_pending = false;
  bindInput(active());
}

void SurfaceManager::inputRebindTimerCb(lv_timer_t* timer) {
  auto* self = timer ? static_cast<SurfaceManager*>(timer->user_data) : nullptr;
  if (!self) return;
  if (timer) lv_timer_pause(timer);
  self->finishInputRebind();
}

void SurfaceManager::drainPending() {
  while (!_mutating && _pending_count > 0) {
    const PendingOp op = _pending[0];
    for (uint8_t i = 1; i < _pending_count; ++i) {
      _pending[i - 1] = _pending[i];
    }
    --_pending_count;

    switch (op.type) {
      case PendingOpType::SetRootPreserve:
        setRoot(op.surface, RootSwitchPolicy::PreserveModals);
        break;
      case PendingOpType::SetRootDismiss:
        setRoot(op.surface, RootSwitchPolicy::DismissModals);
        break;
      case PendingOpType::Present:
        (void)present(op.surface, op.owner);
        break;
      case PendingOpType::Raise:
        (void)raise(op.surface);
        break;
      case PendingOpType::Dismiss:
        (void)dismiss(op.surface);
        break;
      case PendingOpType::DismissBranch:
        (void)dismissBranch(op.surface);
        break;
      case PendingOpType::DismissAll:
        (void)dismissAll();
        break;
    }
  }
}

void SurfaceManager::setRoot(UiSurface* root, RootSwitchPolicy policy) {
  if (!beginMutation(policy == RootSwitchPolicy::DismissModals ? PendingOpType::SetRootDismiss
                                                               : PendingOpType::SetRootPreserve,
                     root)) {
    (void)enqueue(policy == RootSwitchPolicy::DismissModals ? PendingOpType::SetRootDismiss
                                                            : PendingOpType::SetRootPreserve,
                  root);
    return;
  }
  setRootImmediate(root, policy);
  endMutation();
}

void SurfaceManager::setRootImmediate(UiSurface* root, RootSwitchPolicy policy) {
  if (!canPresent(root)) return;

  if (_root == root && _modal_depth == 0) {
    bindInput(_root);
    return;
  }

  if (policy == RootSwitchPolicy::DismissModals) {
    (void)dismissAllImmediate();
  }

  if (_root == root) {
    bindInput(active());
    return;
  }

  UiSurface* const previous_root = _root;
  if (previous_root && previous_root != root) {
    exit(previous_root);
  }

  _root = root;
  if (policy == RootSwitchPolicy::PreserveModals && previous_root) {
    for (uint8_t i = 0; i < _modal_depth; ++i) {
      if (_modals[i].owner == previous_root) _modals[i].owner = root;
    }
  }
  enter(_root);
  bindInput(active());
}

bool SurfaceManager::present(UiSurface* surface) {
  return present(surface, active());
}

bool SurfaceManager::present(UiSurface* surface, UiSurface* owner) {
  if (!beginMutation(PendingOpType::Present, surface)) {
    return enqueue(PendingOpType::Present, surface, owner);
  }
  const bool ok = presentImmediate(surface, owner);
  endMutation();
  return ok;
}

bool SurfaceManager::presentImmediate(UiSurface* surface, UiSurface* owner) {
  if (isActive(surface)) return true;
  if (!canPresent(surface) || !validOwner(surface, owner)) return false;
  if (contains(surface)) return false;
  if (_modal_depth >= kMaxModalDepth) return false;

  _modals[_modal_depth++] = ModalEntry{surface, owner};
  enter(surface);
  if (!canPresent(surface)) {
    exit(surface);
    _modals[--_modal_depth] = {};
    bindInput(active());
    return false;
  }
  bindInput(surface);
  return true;
}

bool SurfaceManager::raise(UiSurface* surface) {
  if (!beginMutation(PendingOpType::Raise, surface)) {
    return enqueue(PendingOpType::Raise, surface);
  }
  const bool ok = raiseImmediate(surface);
  endMutation();
  return ok;
}

bool SurfaceManager::raiseImmediate(UiSurface* surface) {
  if (!canPresent(surface)) return false;
  const int idx = findModal(surface);
  if (idx < 0) return presentImmediate(surface, active());
  if (idx == static_cast<int>(_modal_depth) - 1) {
    enter(surface);
    bindInput(surface);
    return true;
  }

  if (hasDescendant(surface)) return false;
  ModalEntry raised = _modals[idx];
  for (uint8_t i = static_cast<uint8_t>(idx); i + 1 < _modal_depth; ++i) {
    _modals[i] = _modals[i + 1];
  }
  _modals[_modal_depth - 1] = raised;
  enter(raised.surface);
  bindInput(raised.surface);
  return true;
}

bool SurfaceManager::dismiss(UiSurface* surface) {
  if (!beginMutation(PendingOpType::Dismiss, surface)) {
    return enqueue(PendingOpType::Dismiss, surface);
  }
  const bool ok = dismissImmediate(surface);
  endMutation();
  return ok;
}

bool SurfaceManager::dismissImmediate(UiSurface* surface) {
  if (!validOwnershipGraph()) return false;
  const int idx = findModal(surface);
  if (idx < 0 || hasDescendant(surface)) return false;

  const bool was_bound = _bound_surface == surface;
  const uint16_t rebind_delay_ms = was_bound ? surface->inputRebindDelayMs() : 0;
  if (was_bound) {
    GroupManager::instance().clear();
    _bound_surface = nullptr;
  }
  exit(surface);
  for (uint8_t i = static_cast<uint8_t>(idx); i + 1 < _modal_depth; ++i) {
    _modals[i] = _modals[i + 1];
  }
  _modals[--_modal_depth] = {};
  if (was_bound) {
    scheduleInputRebind(rebind_delay_ms);
  } else {
    bindInput(active());
  }
  return true;
}

bool SurfaceManager::dismissTop() {
  UiSurface* top = topModal();
  if (!beginMutation(PendingOpType::Dismiss, top)) {
    return enqueue(PendingOpType::Dismiss, top);
  }
  const bool ok = dismissTopImmediate();
  endMutation();
  return ok;
}

bool SurfaceManager::dismissTopImmediate() {
  return _modal_depth > 0 && dismissImmediate(topModal());
}

bool SurfaceManager::dismissBranch(UiSurface* surface) {
  if (!beginMutation(PendingOpType::DismissBranch, surface)) {
    return enqueue(PendingOpType::DismissBranch, surface);
  }
  const bool ok = dismissBranchImmediate(surface);
  endMutation();
  return ok;
}

bool SurfaceManager::dismissBranchImmediate(UiSurface* surface) {
  if (!validOwnershipGraph()) return false;
  const int idx = findModal(surface);
  if (idx < 0) return false;

  bool selected[kMaxModalDepth]{};
  for (uint8_t i = 0; i < _modal_depth; ++i) {
    selected[i] = _modals[i].surface == surface ||
                  isDescendantOf(_modals[i].surface, surface);
  }

  const bool active_removed = selected[_modal_depth - 1];
  UiSurface* const removed_active = active_removed ? active() : nullptr;
  const uint16_t rebind_delay_ms =
      removed_active ? removed_active->inputRebindDelayMs() : 0;
  if (active_removed) {
    GroupManager::instance().clear();
    _bound_surface = nullptr;
  }

  for (int i = static_cast<int>(_modal_depth) - 1; i >= 0; --i) {
    if (selected[i]) exit(_modals[i].surface);
  }

  uint8_t write = 0;
  for (uint8_t i = 0; i < _modal_depth; ++i) {
    if (!selected[i]) _modals[write++] = _modals[i];
  }
  while (write < _modal_depth) _modals[write++] = {};
  uint8_t removed = 0;
  for (uint8_t i = 0; i < _modal_depth; ++i) {
    if (selected[i]) ++removed;
  }
  _modal_depth = static_cast<uint8_t>(_modal_depth - removed);

  if (active_removed) {
    scheduleInputRebind(rebind_delay_ms);
  } else {
    bindInput(active());
  }
  return true;
}

bool SurfaceManager::dismissAll() {
  if (!beginMutation(PendingOpType::DismissAll)) return enqueue(PendingOpType::DismissAll);
  const bool ok = dismissAllImmediate();
  endMutation();
  return ok;
}

bool SurfaceManager::dismissAllImmediate() {
  if (_modal_depth == 0) return false;
  UiSurface* const removed_active = active();
  const uint16_t rebind_delay_ms =
      removed_active ? removed_active->inputRebindDelayMs() : 0;
  GroupManager::instance().clear();
  _bound_surface = nullptr;
  for (int i = static_cast<int>(_modal_depth) - 1; i >= 0; --i) {
    exit(_modals[i].surface);
    _modals[i] = {};
  }
  _modal_depth = 0;
  scheduleInputRebind(rebind_delay_ms);
  return true;
}

bool SurfaceManager::isActive(const UiSurface* surface) const {
  return surface && active() == surface;
}

bool SurfaceManager::contains(const UiSurface* surface) const {
  if (!surface) return false;
  if (_root == surface) return true;
  return findModal(surface) >= 0;
}

bool SurfaceManager::dispatchEventToActive(UiEventType type, const void* payload) {
  UiSurface* surface = active();
  if (!canPresent(surface)) return false;
  return ui_event_send(surface->root(), type, payload);
}

void SurfaceManager::reconcileFocus() {
  bindInput(active());
}

void SurfaceManager::reconcileVisibility() {
  while (_modal_depth > 0) {
    UiSurface* top = topModal();
    if (!canPresent(top) || lv_obj_has_flag(top->root(), LV_OBJ_FLAG_HIDDEN)) {
      (void)dismissTop();
      continue;
    }
    break;
  }
  if (UiSurface* current = active()) {
    bindInput(current);
  }
}

int SurfaceManager::findModal(const UiSurface* surface) const {
  if (!surface) return -1;
  for (uint8_t i = 0; i < _modal_depth; ++i) {
    if (_modals[i].surface == surface) return static_cast<int>(i);
  }
  return -1;
}

bool SurfaceManager::validOwner(const UiSurface* surface, const UiSurface* owner) const {
  if (!surface || surface == owner) return false;
  if (!owner) return true;
  if (owner != _root && findModal(owner) < 0) return false;

  const UiSurface* current = owner;
  for (uint8_t depth = 0; current && depth <= kMaxModalDepth; ++depth) {
    if (current == surface) return false;
    if (current == _root) return true;
    const int idx = findModal(current);
    if (idx < 0) return false;
    current = _modals[idx].owner;
  }
  return current == nullptr;
}

bool SurfaceManager::validOwnershipGraph() const {
  for (uint8_t i = 0; i < _modal_depth; ++i) {
    const ModalEntry& entry = _modals[i];
    if (!entry.surface || entry.surface == entry.owner) return false;
    for (uint8_t j = 0; j < i; ++j) {
      if (_modals[j].surface == entry.surface) return false;
    }
    if (entry.owner && entry.owner != _root) {
      const int owner_idx = findModal(entry.owner);
      if (owner_idx < 0 || owner_idx >= static_cast<int>(i)) return false;
    }

    const UiSurface* current = entry.owner;
    for (uint8_t depth = 0; current && current != _root; ++depth) {
      if (depth >= _modal_depth) return false;
      const int owner_idx = findModal(current);
      if (owner_idx < 0) return false;
      current = _modals[owner_idx].owner;
      if (current == entry.surface) return false;
    }
  }
  return true;
}

bool SurfaceManager::isDescendantOf(const UiSurface* candidate,
                                    const UiSurface* owner) const {
  if (!candidate || !owner || candidate == owner) return false;
  const UiSurface* current = candidate;
  for (uint8_t depth = 0; depth < kMaxModalDepth; ++depth) {
    const int idx = findModal(current);
    if (idx < 0) return false;
    current = _modals[idx].owner;
    if (current == owner) return true;
    if (!current || current == _root) return false;
  }
  return false;
}

bool SurfaceManager::hasDescendant(const UiSurface* surface) const {
  if (!surface) return false;
  for (uint8_t i = 0; i < _modal_depth; ++i) {
    if (isDescendantOf(_modals[i].surface, surface)) return true;
  }
  return false;
}

}  // namespace heltec::meshcore::ui
