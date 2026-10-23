#pragma once
#include "Surface.hpp"
struct _lv_obj_t;
namespace Heltec::MeshCore::Ui {
class AbstractOverlay : public UiSurface {
  public:
    void onEnter() override;

  protected:
    explicit AbstractOverlay(Biz::IBizFacade &biz);
    AbstractOverlay(Biz::IBizFacade &biz, const lv_obj_class_t &root_class);

    bool buildSurface() override;
    void setFocusObject(_lv_obj_t *object);
    virtual _lv_obj_t *focusTarget() const;
};

} // namespace Heltec::MeshCore::Ui
