#pragma once
#include "heltec/ui/core/BizFacade.hpp"
#include "heltec/ui/app/ScreenId.hpp"
#include "heltec/ui/core/Surface.hpp"
#include "ui/navigation/NavigationWidget.hpp"

#include <lvgl.h>

namespace Heltec::MeshCore::Ui
{
class RadialNavigator : public UiSurface
{
  public:
    explicit RadialNavigator(Biz::IBizFacade &biz);
    ~RadialNavigator();

    void configure(const UiNavigationItem *items, uint8_t count);
    void setIcon(uint8_t screen_index, const lv_img_dsc_t *image);
    void setSelectedIndex(uint8_t screen_index, bool preview = false);
    bool isTransitioning() const
    {
        return false;
    }
    uint8_t focusedIndex() const;
    _lv_obj_t *navButtonHost() const
    {
        return itemHost();
    }
    _lv_obj_t *navFocusWidget() const
    {
        return _nav ? _nav : _root;
    }
    void setTileView(_lv_obj_t *tileview);
    void setFrameRoot(_lv_obj_t *frame)
    {
        _frame_root = frame;
    }

  protected:
    void onEnter() override;
    void onExit() override;
    bool build() override;

  private:
    bool onKey(uint32_t lv_key) override;
    _lv_obj_t *itemHost() const;
    _lv_obj_t *findCellById(uint8_t id) const;
    uint8_t btnCount() const;
    uint8_t slotForId(uint8_t id) const;
    uint8_t focusedSlot() const;
    bool panelVisible() const;
    void openPanel();
    void resetPanel();
    void layoutRing(bool animate, bool snap_theta = true, bool update_emphasis = true,
                    bool update_selection = true);
    void updateGeometry();
    void sendTilePreview(uint8_t tile_idx);
    bool commitFocused();
    void clearAnimations();
    void ensureDefaultFocus();
    void invalidateSlotCache();
    void rebuildSlotCache(uint8_t count);
    _lv_obj_t *frameRoot() const;

    static constexpr uint8_t kMaxButtons = static_cast<uint8_t>(ScreenId::Count);
    static constexpr uint8_t kNoEmphasis = 0xFF;

    _lv_obj_t *_nav = nullptr;
    _lv_obj_t *_tileview = nullptr;
    _lv_obj_t *_frame_root = nullptr;
    float _slot_cos[kMaxButtons] = {};
    float _slot_sin[kMaxButtons] = {};
    uint8_t _icon_width[kMaxButtons] = {};
    uint8_t _icon_height[kMaxButtons] = {};
    lv_coord_t _max_icon_side = 0;
    float _ring_theta = 0.f;
    uint8_t _emphasis_index = kNoEmphasis;
    uint8_t _ring_layout_focus = kNoEmphasis;
    uint8_t _ring_focus_slot = 0;
    uint8_t _slot_cache_count = 0;
    bool _updating_geometry = false;
    bool _geometry_valid = false;
    bool _configuring = false;
    bool _cached_panel_visible = false;
    lv_coord_t _cached_nav_x = 0;
    lv_coord_t _cached_nav_y = 0;
    lv_coord_t _cached_nav_side = 0;
};

} // namespace Heltec::MeshCore::Ui
