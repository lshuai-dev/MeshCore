#pragma once
#include "Surface.hpp"
namespace Heltec::MeshCore::Ui {
class AbstractOverlay : public UiSurface {
  protected:
    explicit AbstractOverlay(Biz::IBizFacade &biz);

    bool build() override;
};

} // namespace Heltec::MeshCore::Ui
