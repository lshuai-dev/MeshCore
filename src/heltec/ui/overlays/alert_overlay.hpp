#pragma once

#include <stdint.h>

#include "../core/abstract_overlay.hpp"

struct _lv_obj_t;

namespace heltec::meshcore::biz {
class IBizFacade;
}

namespace heltec::meshcore::ui {

namespace meta_id {
constexpr MetaId AlertOverlayRoot = ht_meta_id(MetaIdScope::Overlay, 0x02);
constexpr MetaId AlertBox = ht_meta_id(MetaIdScope::Overlay, 0x40);
constexpr MetaId AlertLabel = ht_meta_id(MetaIdScope::Overlay, 0x41);
}

/** @brief 短时提示 overlay；由 UiApp::activate 驱动显隐。 */
class AlertOverlay : public AbstractOverlay {
 public:
  explicit AlertOverlay(biz::IBizFacade& biz) : AbstractOverlay(biz) {}

  void onEnter() override;
  void onExit() override;
  void setText(const char* text);

 protected:
  _lv_obj_t* create(_lv_obj_t* parent) override;

 private:
  _lv_obj_t* createRoot(_lv_obj_t* parent) override;
  _lv_obj_t* focusTarget() const override { return _root; }
  bool onKey(uint32_t key) override;

  _lv_obj_t* _box = nullptr;
  _lv_obj_t* _label = nullptr;
  char _text_buffer[80] = {};
  uint32_t _entered_ms = 0;
};

}  // namespace heltec::meshcore::ui
