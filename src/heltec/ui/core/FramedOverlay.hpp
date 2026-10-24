#pragma once
#include "AbstractOverlay.hpp"

namespace Heltec::MeshCore::Ui {
class FramedOverlay : public AbstractOverlay {
  public:
    void setStaticTitle(const char *text);
    void setStaticToolTip(const char *text);

  protected:
    explicit FramedOverlay(Biz::IBizFacade &biz,
                           const lv_obj_class_t *content_class = nullptr);

    bool build() override;
    virtual bool buildHeader();

    _lv_obj_t *_header = nullptr;
    _lv_obj_t *_content = nullptr;
    _lv_obj_t *_footer = nullptr;

  private:
    bool buildContent();
    bool buildFooter();

    const lv_obj_class_t *_content_class = nullptr;
};

} // namespace Heltec::MeshCore::Ui
