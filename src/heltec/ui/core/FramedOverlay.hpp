#pragma once
#include "AbstractOverlay.hpp"

namespace Heltec::MeshCore::Ui {
class FramedOverlay : public AbstractOverlay {
  public:
    void setStaticTitle(const char *text);
    void setStaticToolTip(const char *text);

  protected:
    FramedOverlay(Biz::IBizFacade &biz, const lv_obj_class_t &root_class,
                  const lv_obj_class_t *content_class = nullptr);

    bool buildSurface() override;
    virtual bool buildHeader();
    virtual bool buildContent();
    virtual bool buildFooter();

    _lv_obj_t *_header = nullptr;
    _lv_obj_t *_content = nullptr;
    _lv_obj_t *_footer = nullptr;
    const lv_obj_class_t *_content_class = nullptr;
    const char *_title_text = "";
    const char *_tool_tip_text = "";
};

} // namespace Heltec::MeshCore::Ui
