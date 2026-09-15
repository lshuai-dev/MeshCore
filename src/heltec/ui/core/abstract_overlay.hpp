#pragma once
#include "surface.hpp"
struct _lv_obj_t;
namespace heltec::meshcore::ui {
class AbstractOverlay : public UiSurface {
  public:
    void onEnter() override;

  protected:
    explicit AbstractOverlay(biz::IBizFacade &biz);
    AbstractOverlay(biz::IBizFacade &biz, const lv_obj_class_t &root_class);

    bool buildSurface() override;
    void setFocusObject(_lv_obj_t *object);
    virtual _lv_obj_t *focusTarget() const;
};

} // namespace heltec::meshcore::ui
