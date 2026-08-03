#pragma once
#include <stdint.h>

#include "ui/core/ht_meta_data.hpp"

struct _lv_obj_t;

namespace heltec::meshcore::ui {

namespace meta_id {
constexpr MetaId SplashOverlayRoot = ht_meta_id(MetaIdScope::Overlay, 0x07);
constexpr MetaId SplashLogo = ht_meta_id(MetaIdScope::Overlay, 0xE0);
constexpr MetaId SplashVersion = ht_meta_id(MetaIdScope::Overlay, 0xE1);
constexpr MetaId SplashDate = ht_meta_id(MetaIdScope::Overlay, 0xE2);
constexpr MetaId SplashAttribution = ht_meta_id(MetaIdScope::Overlay, 0xE3);
}

class SplashOverlay {
 public:
  SplashOverlay() = default;
  ~SplashOverlay();

  bool create(_lv_obj_t* parent);
  _lv_obj_t* root() const { return _root; }

  void printInfo();

  void hide();
  bool isVisible() const { return _visible; }

 private:
  _lv_obj_t* _root = nullptr;
  _lv_obj_t* _logo = nullptr;
  _lv_obj_t* _ver = nullptr;
  _lv_obj_t* _date = nullptr;
  _lv_obj_t* _attribution = nullptr;
  char _version_text[32] = {};
  bool _visible = false;
};

}  // namespace heltec::meshcore::ui
