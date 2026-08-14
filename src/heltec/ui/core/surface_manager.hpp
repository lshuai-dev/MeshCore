#pragma once

#include <stdint.h>

#include "ui_events.h"

struct _lv_timer_t;

namespace heltec::meshcore::ui {

class UiSurface;

class SurfaceManager {
 public:
  static constexpr uint8_t kMaxModalDepth = 4;
  static constexpr uint8_t kMaxPendingOps = 4;

  enum class RootSwitchPolicy : uint8_t {
    PreserveModals,
    DismissModals,
  };

  bool createTimers();

  void setRoot(UiSurface* root, RootSwitchPolicy policy = RootSwitchPolicy::PreserveModals);

  bool present(UiSurface* surface);
  bool present(UiSurface* surface, UiSurface* owner);
  bool raise(UiSurface* surface);
  bool dismiss(UiSurface* surface);
  /** Dismisses the target modal and its explicitly owned descendants. */
  bool dismissBranch(UiSurface* surface);

  UiSurface* root() const { return _root; }
  UiSurface* active() const;

  bool isActive(const UiSurface* surface) const;
  bool contains(const UiSurface* surface) const;
  uint8_t modalDepth() const { return _modal_depth; }

  bool dispatchEventToActive(UiEventType type, const void* payload = nullptr);
  void reconcileFocus();
  void reconcileVisibility();

 private:
  UiSurface* topModal() const;
  bool dismissTop();
  bool dismissAll();

  enum class PendingOpType : uint8_t {
    SetRootPreserve,
    SetRootDismiss,
    Present,
    Raise,
    Dismiss,
    DismissBranch,
    DismissAll,
  };

  struct PendingOp {
    PendingOpType type;
    UiSurface* surface;
    UiSurface* owner;
  };

  struct ModalEntry {
    UiSurface* surface;
    UiSurface* owner;
  };

  bool canPresent(const UiSurface* surface) const;
  bool canBindInput(const UiSurface* surface) const;
  bool enqueue(PendingOpType type, UiSurface* surface = nullptr,
               UiSurface* owner = nullptr);
  void drainPending();
  bool beginMutation(PendingOpType type, UiSurface* surface = nullptr);
  void endMutation();
  bool inputRebindPending() const;
  bool createInputRebindTimer();
  void scheduleInputRebind(uint16_t delay_ms);
  void finishInputRebind();
  static void inputRebindTimerCb(_lv_timer_t* timer);

  void setRootImmediate(UiSurface* root, RootSwitchPolicy policy);
  bool presentImmediate(UiSurface* surface, UiSurface* owner);
  bool raiseImmediate(UiSurface* surface);
  bool dismissImmediate(UiSurface* surface);
  bool dismissTopImmediate();
  bool dismissAllImmediate();
  bool dismissBranchImmediate(UiSurface* surface);

  void enter(UiSurface* surface);
  void exit(UiSurface* surface);
  void bindInput(UiSurface* surface);
  int findModal(const UiSurface* surface) const;
  bool validOwner(const UiSurface* surface, const UiSurface* owner) const;
  bool validOwnershipGraph() const;
  bool hasDescendant(const UiSurface* surface) const;
  bool isDescendantOf(const UiSurface* candidate, const UiSurface* owner) const;

  UiSurface* _root = nullptr;
  UiSurface* _bound_surface = nullptr;
  ModalEntry _modals[kMaxModalDepth] = {};
  PendingOp _pending[kMaxPendingOps] = {};
  _lv_timer_t* _input_rebind_timer = nullptr;
  uint8_t _modal_depth = 0;
  uint8_t _pending_count = 0;
  bool _mutating = false;
  bool _input_rebind_pending = false;
};

}  // namespace heltec::meshcore::ui
