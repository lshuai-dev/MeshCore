#pragma once

#include "ui_surface.hpp"
#include "ht_meta_data.hpp"

struct _lv_obj_t;

namespace heltec::meshcore::ui {

namespace meta_id {
constexpr MetaId OverlayRoot = ht_meta_id(MetaIdScope::Overlay, 0x00);
}

class AbstractOverlay : public UiSurface {
 public:
  explicit AbstractOverlay(biz::IBizFacade& biz) : UiSurface(biz) {}

  _lv_obj_t* create(_lv_obj_t* parent) override;
  _lv_obj_t* focusedObject() const override;
  void onEnter() override;

 protected:
  _lv_obj_t* createRoot(_lv_obj_t* parent) override;
  void setFocusObject(_lv_obj_t* obj);
  virtual _lv_obj_t* focusTarget() const;
  bool onKey(uint32_t key) override;
};

}  // namespace heltec::meshcore::ui
