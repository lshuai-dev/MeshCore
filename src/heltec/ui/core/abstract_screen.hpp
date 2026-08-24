/**
 * @file abstract_screen.hpp
 * @brief 全屏 Screen 基类。
 */
#pragma once

#include "screen_id.hpp"
#include "ui_surface.hpp"

#include <lvgl.h>
#include <stdint.h>

struct _lv_obj_t;

namespace heltec::meshcore::biz
{
class IBizFacade;
}

namespace heltec::meshcore::ui
{

class AbstractScreen : public UiSurface
{
  public:
    enum class FocusMode : uint8_t
    {
        Idle,
        Active,
    };

    AbstractScreen(biz::IBizFacade &biz, const char *title, const lv_img_dsc_t *icon,
                   bool root_scroll_focus = true);
    virtual ~AbstractScreen() = default;

    virtual const char *title() const;
    virtual const lv_img_dsc_t *icon() const;
    virtual eScreenId screenId() const = 0;
    lv_group_t *group() const override;
    _lv_obj_t *tile() const;
    void onEnter() override;
    void onExit() override;
    _lv_obj_t *focusedObject() const override;

  protected:
    _lv_obj_t *create(_lv_obj_t *parent) override;
    _lv_obj_t *createRoot(_lv_obj_t *parent) override;
    bool onKey(uint32_t key) override;
    void addFocusItem(_lv_obj_t *object, _lv_obj_t *frame = nullptr,
                      bool propagate_focus = true);

  private:
    static constexpr uint8_t kMaxFocusItems = 16;

    struct FocusItem
    {
        _lv_obj_t *object;
        _lv_obj_t *frame;
    };

    bool isAvailableFocusItem(const _lv_obj_t *obj) const;
    _lv_obj_t *firstAvailableFocusItem() const;
    bool cycleRootPage(bool forward);
    void activateFocusMode(FocusMode mode, _lv_obj_t *focus);
    static void activateFocusModeDeferred(void *user_data);
    static void onFocusItemChanged(lv_event_t *e);

    const char *_title;
    const lv_img_dsc_t *_icon;
    bool _root_scroll_focus;
    lv_group_t *_idle_group = nullptr;
    lv_group_t *_active_group = nullptr;
    FocusMode _focus_mode = FocusMode::Idle;
    FocusItem _focus_items[kMaxFocusItems] = {};
    uint8_t _focus_item_count = 0;
    bool _focus_activation_pending = false;
};

} // namespace heltec::meshcore::ui
