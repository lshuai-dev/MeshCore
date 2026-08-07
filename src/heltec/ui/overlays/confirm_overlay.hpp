#pragma once

#include <stdint.h>

#include "../core/abstract_overlay.hpp"
#include "../core/ui_events.h"

struct _lv_obj_t;

namespace heltec::meshcore::biz {
class IBizFacade;
}

namespace heltec::meshcore::ui {

namespace meta_id {
constexpr MetaId ConfirmOverlayRoot = ht_meta_id(MetaIdScope::Overlay, 0x0A);
constexpr MetaId ConfirmBox = ht_meta_id(MetaIdScope::Overlay, 0x70);
constexpr MetaId ConfirmTitle = ht_meta_id(MetaIdScope::Overlay, 0x71);
constexpr MetaId ConfirmBody = ht_meta_id(MetaIdScope::Overlay, 0x72);
constexpr MetaId ConfirmButtonRow = ht_meta_id(MetaIdScope::Overlay, 0x73);
constexpr MetaId ConfirmButton = ht_meta_id(MetaIdScope::Overlay, 0x74);
constexpr MetaId ConfirmButtonLabel = ht_meta_id(MetaIdScope::Overlay, 0x75);
constexpr MetaId ConfirmKeyHint = ht_meta_id(MetaIdScope::Overlay, 0x76);
}

class ConfirmOverlay final : public AbstractOverlay {
 public:
  explicit ConfirmOverlay(biz::IBizFacade& biz) : AbstractOverlay(biz) {}

  void setRequest(UiConfirmAction action);

 protected:
  _lv_obj_t* create(_lv_obj_t* parent) override;

 private:
  _lv_obj_t* createRoot(_lv_obj_t* parent) override;
  _lv_obj_t* focusTarget() const override;
  bool onKey(uint32_t key) override;

  static void onBackdropClicked(lv_event_t* e);
  static void onButtonClicked(lv_event_t* e);

  _lv_obj_t* _box = nullptr;
  _lv_obj_t* _body = nullptr;
  _lv_obj_t* _cancel = nullptr;
  _lv_obj_t* _accept = nullptr;
  UiConfirmAction _action = UiConfirmAction::FactoryReset;
};

}  // namespace heltec::meshcore::ui
