#pragma once

#if !defined(HELTEC_UI_HAS_TOUCH) || !HELTEC_UI_HAS_TOUCH

#include "heltec/ui/core/Surface.hpp"
#include "ui/widgets/ButtonRoller.hpp"

namespace Heltec::MeshCore::Ui
{

using ContextMenuCommand = void (*)(Biz::IBizFacade &, lv_obj_t *event_target);

struct ContextMenuPage;

struct ContextMenuItem
{
    constexpr ContextMenuItem(const char *item_label = nullptr,
                              ContextMenuCommand item_command = nullptr,
                              const ContextMenuPage *item_submenu = nullptr)
        : label(item_label), command(item_command), submenu(item_submenu)
    {
    }

    const char *label;
    ContextMenuCommand command;
    const ContextMenuPage *submenu;
};

struct ContextMenuPage
{
    constexpr ContextMenuPage(const char *page_title = nullptr,
                              const ContextMenuItem *page_items = nullptr,
                              uint8_t page_item_count = 0)
        : title(page_title), items(page_items), item_count(page_item_count)
    {
    }

    const char *title;
    const ContextMenuItem *items;
    uint8_t item_count;
};

class ContextMenu : public UiSurface
{
  public:
    explicit ContextMenu(Biz::IBizFacade &biz);

    bool registerMenu(const lv_img_dsc_t *icon, const ContextMenuPage &page);
    bool isEmpty() const;
    bool canOpen() const;

    void onEnter() override;
    void onExit() override;

  protected:
    bool buildSurface() override;
    bool onKey(uint32_t key) override;

  private:
    enum class Stage : uint8_t
    {
        MainMenu,
        Submenu,
    };

    struct HistoryEntry
    {
        const ContextMenuPage *page = nullptr;
        uint8_t focused_index = 0;
    };

    static constexpr uint8_t kMaxPanes = 8;
    static constexpr uint8_t kMaxMenuDepth = 4;

    void selectPane(uint8_t index);
    bool enterSelectedPane();
    void enterMainMenu(uint8_t focus_index);
    bool showPage(const ContextMenuPage *page, uint8_t focus_index = 0);
    bool activateItem(uint8_t index);
    bool navigateBack();
    void updateIconSelection();
    void resetHistory();
    bool ensureRollerCapacity(const ContextMenuPage &page, uint8_t depth = 0);

    static void handleRollerClicked(lv_event_t *event);

    lv_obj_t *_header_icon_row = nullptr;
    lv_obj_t *_menu = nullptr;
    lv_obj_t *_title_label = nullptr;

    ButtonRoller _roller;
    const ContextMenuPage *_active_page = nullptr;
    HistoryEntry _history[kMaxMenuDepth]{};
    uint8_t _history_depth = 0;
    uint8_t _active_menu = 0;
    lv_coord_t _icon_row_height = 0;
    Stage _stage = Stage::MainMenu;
};

} // namespace Heltec::MeshCore::Ui

#endif
