#pragma once

#if !defined(HELTEC_V4_R8_TFT) || !defined(HELTEC_HAS_TOUCH) || !HELTEC_HAS_TOUCH

#include "heltec/ui/core/ui_surface.hpp"

namespace heltec::meshcore::biz
{
class IBizFacade;
}

namespace heltec::meshcore::ui
{

class AbstractMenu;

class ContextMenu : public UiSurface
{
  public:
    explicit ContextMenu(biz::IBizFacade &biz) : UiSurface(biz)
    {
    }

    void beginRegister();
    void endRegister();
    bool registerMenu(const char *title, const lv_img_dsc_t *icon, AbstractMenu &menu);
    bool isEmpty() const;
    bool canOpen() const;
    void leaveMenuLeaf();

    lv_group_t *group() const override;
    lv_obj_t *focusedObject() const override;

    void onEnter() override;
    void onExit() override;

  protected:
    _lv_obj_t *create(_lv_obj_t *parent) override;

  private:
    _lv_obj_t *createRoot(_lv_obj_t *parent) override;

    void selectPane(uint8_t index);
    void enterMenuLeaf(AbstractMenu *menu);
    bool activatePaneButton(_lv_obj_t *btn, bool enter_leaf);
    void updateIconSelection();
    void handleLeafEsc();

    static void on_menu_leaf_esc(lv_event_t *e);

    static constexpr uint8_t kMaxPanes = 8;

    _lv_obj_t *_header_icon_row = nullptr;
    _lv_obj_t *_menu = nullptr;

    uint8_t _active_menu = 0;
    lv_coord_t _icon_row_height = 0;
    bool _in_leaf = false;
    bool _registering_panes = false;
    AbstractMenu *_leaf_menu = nullptr;
};

} // namespace heltec::meshcore::ui

#endif
