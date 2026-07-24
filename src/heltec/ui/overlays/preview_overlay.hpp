#pragma once
#include <stdint.h>
#include "../core/abstract_overlay.hpp"

namespace heltec::meshcore::biz {
class IBizFacade;
}
struct _lv_obj_t;
namespace heltec::meshcore::ui {

namespace meta_id {
constexpr MetaId PreviewOverlayRoot = ht_meta_id(MetaIdScope::Overlay, 0x01);
constexpr MetaId PreviewHeader = ht_meta_id(MetaIdScope::Overlay, 0x20);
constexpr MetaId PreviewTitle = ht_meta_id(MetaIdScope::Overlay, 0x21);
constexpr MetaId PreviewAge = ht_meta_id(MetaIdScope::Overlay, 0x22);
constexpr MetaId PreviewOrigin = ht_meta_id(MetaIdScope::Overlay, 0x23);
constexpr MetaId PreviewText = ht_meta_id(MetaIdScope::Overlay, 0x24);
constexpr MetaId PreviewFooter = ht_meta_id(MetaIdScope::Overlay, 0x25);
}

class PreviewOverlay : public AbstractOverlay {
 public:
  explicit PreviewOverlay(biz::IBizFacade& biz) : AbstractOverlay(biz) {}

  _lv_obj_t* create(_lv_obj_t* parent) override;
  void applyContent(uint8_t unread, uint32_t age_sec, const char* origin, const char* text);
  void dismissByUser();

 private:
  _lv_obj_t* createRoot(_lv_obj_t* parent) override;
  _lv_obj_t* focusTarget() const override;
  bool onKey(uint32_t key) override;

  _lv_obj_t* _title = nullptr;
  _lv_obj_t* _age = nullptr;
  _lv_obj_t* _origin = nullptr;
  _lv_obj_t* _text = nullptr;
  _lv_obj_t* _footer = nullptr;
  char _title_text[16] = {};
  char _age_text[12] = {};
  char _origin_text[48] = {};
  char _message_text[80] = {};
};

}  // namespace heltec::meshcore::ui
