#pragma once
#include "AbstractOverlay.hpp"

namespace Heltec::MeshCore::Ui
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

        Variant variant : 2;
        bool has_title : 1;
        bool has_actions : 1;
        bool dismiss_on_backdrop : 1;
    };

  protected:
    DialogOverlay(Biz::IBizFacade &biz, Options options);

    bool build() override;
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

} // namespace Heltec::MeshCore::Ui
