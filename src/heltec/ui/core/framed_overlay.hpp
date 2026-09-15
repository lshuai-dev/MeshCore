#pragma once
#include "abstract_overlay.hpp"

namespace heltec::meshcore::ui {
class FramedOverlay : public AbstractOverlay {
  public:
    void setTitle(const char *text);
    void setToolTip(const char *text);

  protected:
    FramedOverlay(biz::IBizFacade &biz, const lv_obj_class_t &root_class);

    bool buildSurface() override;
    virtual bool buildHeader();
    virtual bool buildContent();
    virtual bool buildFooter();

    _lv_obj_t *_header = nullptr;
    _lv_obj_t *_content = nullptr;
    _lv_obj_t *_footer = nullptr;
};

} // namespace heltec::meshcore::ui
