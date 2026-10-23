#pragma once

#include "../core/DialogOverlay.hpp"
#include "../core/HtEvent.h"

#include <stdint.h>

struct _lv_obj_t;

namespace Heltec::MeshCore::Biz
{
class IBizFacade;
}

namespace Heltec::MeshCore::Ui
{

class ConfirmOverlay final : public DialogOverlay
{
  public:
    explicit ConfirmOverlay(Biz::IBizFacade &biz)
        : DialogOverlay(biz, Options{Variant::Modal, true, true, true})
    {
    }

    void setRequest(UiConfirmAction action);
    void onEnter() override;
    UiConfirmAction action() const { return _action; }
    uint64_t teamSession() const { return _team_session; }

  protected:
    bool buildSurface() override;

  private:
    bool onKey(uint32_t key) override;
    void onBackdropPressed() override;

    static void onButtonClicked(lv_event_t *e);

    _lv_obj_t *_body = nullptr;
    _lv_obj_t *_cancel = nullptr;
    _lv_obj_t *_accept = nullptr;
    UiConfirmAction _action = UiConfirmAction::FactoryReset;
    char _body_text[80] = {};
    uint64_t _team_session = 0;
    uint32_t _entered_ms = 0;
};

} // namespace Heltec::MeshCore::Ui
