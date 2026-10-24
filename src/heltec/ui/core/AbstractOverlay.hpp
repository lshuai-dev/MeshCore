#pragma once
#include "Surface.hpp"
struct _lv_obj_t;
namespace Heltec::MeshCore::Ui {
class AbstractOverlay : public UiSurface {
  public:

  protected:
    void onEnter() override;
    explicit AbstractOverlay(Biz::IBizFacade &biz);

    bool build() override;
    void setFocusObject(_lv_obj_t *object);
    virtual _lv_obj_t *focusTarget() const;
};

} // namespace Heltec::MeshCore::Ui
