#pragma once
#include "../core/framed_overlay.hpp"

#include <stdint.h>

namespace heltec::meshcore::biz
{
class IBizFacade;
}
struct _lv_obj_t;
namespace heltec::meshcore::ui
{

class RadioParamSyncOverlay : public FramedOverlay
{
  public:
    explicit RadioParamSyncOverlay(biz::IBizFacade &biz);

    void onEnter() override;
    void onExit() override;

    void stepSelection(int8_t dir);
#if HELTEC_HAS_TOUCH
    bool hitRoller(int16_t x, int16_t y) const;
#endif

  protected:
    bool buildSurface() override;

  private:
    bool onKey(uint32_t key) override;
    void syncFromPrefs();
    void configureListLayout();
    void renderRows();
    void applySelection();
#if HELTEC_HAS_TOUCH
    void rebuildRollerOptions();
#endif

    _lv_obj_t *_list = nullptr;
#if HELTEC_HAS_TOUCH
    _lv_obj_t *_roller = nullptr;
    uint8_t _roller_press_selected = 0;
    int8_t _rendered_current_index = -1;
#endif
    int8_t _select = 0;
    uint8_t _count = 0;
    uint8_t _visible_rows = 1;
    bool _applying = false;
    char _current_row_text[32] = {};
};

} // namespace heltec::meshcore::ui
