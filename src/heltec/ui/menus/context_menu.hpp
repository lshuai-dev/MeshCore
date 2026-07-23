#pragma once

#include "heltec/ui/core/ht_meta_data.hpp"

namespace heltec::meshcore::ui::meta_id {
constexpr MetaId ContextMenuRoot = ht_meta_id(MetaIdScope::ContextMenu, 0x00);
constexpr MetaId ContextMenuMenu = ht_meta_id(MetaIdScope::ContextMenu, 0x01);
constexpr MetaId ContextMenuHeader = ht_meta_id(MetaIdScope::ContextMenu, 0x02);
constexpr MetaId ContextMenuHeaderIconRow = ht_meta_id(MetaIdScope::ContextMenu, 0x03);
constexpr MetaId ContextMenuHeaderNavRow = ht_meta_id(MetaIdScope::ContextMenu, 0x04);
constexpr MetaId ContextMenuBackButton = ht_meta_id(MetaIdScope::ContextMenu, 0x05);
constexpr MetaId ContextMenuTitle = ht_meta_id(MetaIdScope::ContextMenu, 0x06);
constexpr MetaId ContextMenuIconButton = ht_meta_id(MetaIdScope::ContextMenu, 0x07);
constexpr MetaId ContextMenuIcon = ht_meta_id(MetaIdScope::ContextMenu, 0x08);
}

#if !defined(HELTEC_V4_R8_TFT) || !defined(HELTEC_HAS_TOUCH) || !HELTEC_HAS_TOUCH

#include "heltec/ui/core/ui_surface.hpp"

namespace heltec::meshcore::biz {
class IBizFacade;
}

namespace heltec::meshcore::ui {

class AbstractMenu;

class ContextMenu : public UiSurface {
 public:
  explicit ContextMenu(biz::IBizFacade& biz) : UiSurface(biz) {}

  _lv_obj_t* create(_lv_obj_t* parent) override;
  void beginRegister();
  void endRegister();
  bool registerMenu(const char* title, const lv_img_dsc_t* icon, AbstractMenu& menu);
  bool isEmpty() const;
  bool canOpen() const;
  void leaveMenuLeaf();

  lv_group_t* group() const override;
  lv_obj_t* focusedObject() const override;

  void onEnter() override;
  void onExit() override;

 private:
  _lv_obj_t* createRoot(_lv_obj_t* parent) override;

  void selectPane(uint8_t index);
  void enterMenuLeaf(AbstractMenu* menu);
  bool activatePaneButton(_lv_obj_t* btn, bool enter_leaf);
  void updateIconSelection();
  void handleLeafEsc();
  void bindPendingMenu();
  void bindPendingLeaf();
  void schedulePendingBind(uint32_t delay_ms = 0);
  void cancelPendingBind();

  static void on_menu_leaf_esc(lv_event_t* e);
  static void pendingBindTimerCb(lv_timer_t* timer);

  static constexpr uint8_t kMaxPanes = 8;

  _lv_obj_t* _header_icon_row = nullptr;
  _lv_obj_t* _menu = nullptr;

  uint8_t _active_menu = 0;
  bool _in_leaf = false;
  bool _registering_panes = false;
  bool _pending_menu_bind = false;
  bool _pending_leaf_bind = false;
  AbstractMenu* _leaf_menu = nullptr;
  lv_timer_t* _pending_bind_timer = nullptr;
};

}  // namespace heltec::meshcore::ui

#endif
