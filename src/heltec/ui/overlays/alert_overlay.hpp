#pragma once

#include "../core/dialog_overlay.hpp"

#include <stdint.h>

struct _lv_obj_t;

namespace heltec::meshcore::biz
{
class IBizFacade;
}

namespace heltec::meshcore::ui
{

/** @brief 短时提示 overlay；由 UiApp::activate 驱动显隐。 */
class AlertOverlay : public DialogOverlay
{
  public:
    explicit AlertOverlay(biz::IBizFacade &biz)
        : DialogOverlay(biz, Options{Variant::Alert, false, false, false})
    {
    }

    void onEnter() override;
    void onExit() override;
    void setText(const char *text);

  protected:
    _lv_obj_t *create(_lv_obj_t *parent) override;

  private:
    _lv_obj_t *focusTarget() const override
    {
        return _root;
    }
    bool onKey(uint32_t key) override;

    _lv_obj_t *_label = nullptr;
    char _text_buffer[80] = {};
    uint32_t _entered_ms = 0;
};

} // namespace heltec::meshcore::ui
