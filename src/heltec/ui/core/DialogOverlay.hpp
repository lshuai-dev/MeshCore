#pragma once
#include "AbstractOverlay.hpp"

namespace heltec::meshcore::ui
{
class DialogOverlay : public AbstractOverlay
{
  public:
    enum class Variant : uint8_t
    {
        Modal,
        Alert,
        Fullscreen
    };

    struct Options
    {
        Options(Variant dialog_variant = Variant::Modal, bool title = false,
                bool actions = false, bool backdrop_dismiss = false)
            : variant(dialog_variant), has_title(title), has_actions(actions),
              dismiss_on_backdrop(backdrop_dismiss)
        {
        }

        Variant variant = Variant::Modal;
        bool has_title = false;
        bool has_actions = false;
        bool dismiss_on_backdrop = false;
    };

  protected:
    DialogOverlay(biz::IBizFacade &biz, Options options);

    bool buildSurface() override;
    _lv_obj_t *panel() const;
    _lv_obj_t *contentHost() const;
    _lv_obj_t *actionsHost() const;
    void setTitle(const char *text);
    virtual void onBackdropPressed();

  private:
    static void backdropEventCb(lv_event_t *event);

    const Options _options;
    _lv_obj_t *_panel = nullptr;
    _lv_obj_t *_title = nullptr;
    _lv_obj_t *_content = nullptr;
    _lv_obj_t *_actions = nullptr;
};

} // namespace heltec::meshcore::ui
