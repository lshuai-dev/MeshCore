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
    AbstractScreen(Biz::IBizFacade &biz, const char *title, const lv_img_dsc_t *icon,const lv_obj_class_t &root_class);
    virtual ~AbstractScreen() = default;

    const char *title() const;
    const lv_img_dsc_t *icon() const;
    void onEnter() override;
    void onExit() override;

  protected:
    bool buildSurface() override;
    bool onKey(uint32_t key) override;
    void addNavigableObject(_lv_obj_t *object, _lv_obj_t *frame = nullptr,
                            bool propagate_focus = true);

  private:
    void focusRoot();
    void focusControls();
    static void deferredActivateControls(void *user_data);
    static void deferredActivateRoot(void *user_data);
    static void handleRootEvent(lv_event_t *event);
    static void handleFocusChangedEvent(lv_event_t *event);
    static void handleFramePressedEvent(lv_event_t *event);
#if defined(HELTEC_HAS_KEY_FRAME) && HELTEC_HAS_KEY_FRAME
    static void keyFrameFocusEvent(lv_event_t *event);
#endif

    const char *_title;
    const lv_img_dsc_t *_icon;
};

} // namespace Heltec::MeshCore::Ui
