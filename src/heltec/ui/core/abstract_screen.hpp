#pragma once
#include "surface.hpp"
struct _lv_obj_t;
namespace heltec::meshcore::biz {
class IBizFacade;
}

namespace heltec::meshcore::ui {
class AbstractScreen : public UiSurface {
  public:
    AbstractScreen(biz::IBizFacade &biz, const char *title, const lv_img_dsc_t *icon);
    AbstractScreen(biz::IBizFacade &biz, const char *title, const lv_img_dsc_t *icon,const lv_obj_class_t &root_class);
    virtual ~AbstractScreen() = default;

    const char *title() const;
    const lv_img_dsc_t *icon() const;
    void onEnter() override;
    void onExit() override;

  protected:
    bool buildSurface() override;
    bool onKey(uint32_t key) override;
    void addFocusableObject(_lv_obj_t *object, _lv_obj_t *frame = nullptr, bool propagate_focus = true);

  private:
    void focusRoot();
    void focusControls();
    static lv_obj_tree_walk_res_t addFocusObjectFromTree(_lv_obj_t *object, void *user_data);
    static void deferredActivateControls(void *user_data);
    static void deferredActivateRoot(void *user_data);
    static void handleRootEvent(lv_event_t *event);
    static void handleFocusChangedEvent(lv_event_t *event);
    static void handleFramePressedEvent(lv_event_t *event);
#if defined(HELTEC_DISPLAY_E213) && HELTEC_DISPLAY_E213
    static void e213FocusEvent(lv_event_t *event);
#endif

    const char *_title;
    const lv_img_dsc_t *_icon;
};

} // namespace heltec::meshcore::ui
