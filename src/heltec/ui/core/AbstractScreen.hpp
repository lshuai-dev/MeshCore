#pragma once
#include "Surface.hpp"
struct _lv_obj_t;
namespace Heltec::MeshCore::Biz {
class IBizFacade;
}

namespace Heltec::MeshCore::Ui {
class AbstractScreen : public UiSurface {
  public:
    AbstractScreen(Biz::IBizFacade &biz, const char *title, const lv_img_dsc_t *icon);
    virtual ~AbstractScreen() = default;

    const char *title() const {
        return _title;
    }
    const lv_img_dsc_t *icon() const {
        return _icon;
    }

  protected:
    void onEnter() override;
    void onExit() override;
    bool build() override;
    bool onKey(uint32_t key) override;
    void addControl(_lv_obj_t *control, _lv_obj_t *row = nullptr);

  private:
    static void deferredActivateControls(void *user_data);
    static void deferredActivateRoot(void *user_data);
#if defined(HELTEC_UI_HAS_TOUCH) && HELTEC_UI_HAS_TOUCH
    static void handleRootEvent(lv_event_t *event);
    static void handleFramePressedEvent(lv_event_t *event);
#endif
#if defined(HELTEC_HAS_KEY_FRAME) && HELTEC_HAS_KEY_FRAME
    static void keyFrameFocusEvent(lv_event_t *event);
#endif

    const char *_title;
    const lv_img_dsc_t *_icon;
};

} // namespace Heltec::MeshCore::Ui
