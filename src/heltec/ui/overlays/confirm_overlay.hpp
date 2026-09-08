#pragma once

#include "../core/dialog_overlay.hpp"
#include "../core/ht_event.h"

#include <stdint.h>

struct _lv_obj_t;

namespace heltec::meshcore::biz
{
class IBizFacade;
}

namespace heltec::meshcore::ui
{

class ConfirmOverlay final : public DialogOverlay
{
  public:
    explicit ConfirmOverlay(biz::IBizFacade &biz)
        : DialogOverlay(biz, Options{Variant::Modal, true, true, true})
    {
    }

    void setRequest(UiConfirmAction action);
    UiConfirmAction action() const { return _action; }
    uint64_t teamSession() const { return _team_session; }

  protected:
    _lv_obj_t *create(_lv_obj_t *parent) override;

  private:
    _lv_obj_t *focusTarget() const override;
    bool onKey(uint32_t key) override;
    void onBackdropPressed() override;

    static void onButtonClicked(lv_event_t *e);

    _lv_obj_t *_body = nullptr;
    _lv_obj_t *_cancel = nullptr;
    _lv_obj_t *_accept = nullptr;
    UiConfirmAction _action = UiConfirmAction::FactoryReset;
    char _body_text[80] = {};
    uint64_t _team_session = 0;
};

} // namespace heltec::meshcore::ui
