#pragma once
#include "heltec/ui/core/abstract_menu.hpp"
#include "ui/widgets/button_roller.hpp"

namespace heltec::meshcore::biz
{
class IBizFacade;
}

namespace heltec::meshcore::ui
{

class CascadingMenu final : public AbstractMenu
{
  public:
    static constexpr size_t kPageTitleLen = 48;

    explicit CascadingMenu(biz::IBizFacade &biz) : AbstractMenu(biz)
    {
    }
    lv_obj_t *page() const override
    {
        return _page;
    }
    lv_obj_t *root() const override
    {
        return _roller.root();
    }
    bool create(lv_obj_t *menu, lv_group_t *group, const char *parent_title,
                lv_obj_t *event_target = nullptr) override;
    void resetInputState(bool focus_first_item = true) override;
    bool addCommandHandler(const char *label, MenuHandlerFn action);
    bool addMenu(const char *label, AbstractMenu &submenu);

  private:
    struct MenuItem
    {
        MenuKind kind = MenuKind::Command;
        const char *label = nullptr;
        union
        {
            MenuHandlerFn cmd;
            AbstractMenu *submenu;
        };
    };

    static constexpr uint8_t kMaxItems = 12;
    MenuItem _items[kMaxItems]{};
    uint8_t _item_count = 0;

    bool enterSubmenuAt(uint8_t index);
    void leaveSubmenu();

    lv_obj_t *_menu_host = nullptr;
    lv_obj_t *_page = nullptr;
    lv_group_t *_focus_group = nullptr;
    AbstractMenu *_active_child = nullptr;
    ButtonRoller _roller;
    uint8_t _focused_index = 0;
};

} // namespace heltec::meshcore::ui
