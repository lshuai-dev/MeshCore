#pragma once
#include <stdint.h>
#include "../core/abstract_overlay.hpp"

namespace heltec::meshcore::biz {
class IBizFacade;
}
struct _lv_obj_t;
namespace heltec::meshcore::ui {

namespace meta_id {
constexpr MetaId RadioParamSyncOverlayRoot = ht_meta_id(MetaIdScope::Overlay, 0x05);
constexpr MetaId RadioParamSyncTitle = ht_meta_id(MetaIdScope::Overlay, 0xA0);
constexpr MetaId RadioParamSyncList = ht_meta_id(MetaIdScope::Overlay, 0xA1);
constexpr MetaId RadioParamSyncRow = ht_meta_id(MetaIdScope::Overlay, 0xA2);
constexpr MetaId RadioParamSyncFooter = ht_meta_id(MetaIdScope::Overlay, 0xA3);
}

class RadioParamSyncOverlay : public AbstractOverlay {
 public:
  explicit RadioParamSyncOverlay(biz::IBizFacade& biz) : AbstractOverlay(biz) {}

  _lv_obj_t* create(_lv_obj_t* parent) override;

  void onEnter() override;
  void onExit() override;

  void stepSelection(int8_t dir);
#if defined(HELTEC_V4_R8_TFT)
  bool hitRoller(int16_t x, int16_t y) const;
#endif

 private:
  _lv_obj_t* createRoot(_lv_obj_t* parent) override;
  _lv_obj_t* focusTarget() const override;
  bool onKey(uint32_t key) override;
  void syncFromPrefs();
  void configureListLayout();
  void renderRows();
  void applySelection();
#if defined(HELTEC_V4_R8_TFT)
  void rebuildRollerOptions();
#endif

  _lv_obj_t* _title = nullptr;
  _lv_obj_t* _list = nullptr;
  _lv_obj_t* _footer = nullptr;
#if defined(HELTEC_V4_R8_TFT)
  _lv_obj_t* _roller = nullptr;
  uint8_t _roller_press_selected = 0;
#endif
  int8_t _select = 0;
  uint8_t _count = 0;
  uint8_t _visible_rows = 1;
  char _current_row_text[32] = {};
};

}  // namespace heltec::meshcore::ui
