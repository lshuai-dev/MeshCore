#pragma once

#include "abstract_overlay.hpp"

namespace heltec::meshcore::ui
{

extern const lv_obj_class_t ht_framed_overlay_header_class;
extern const lv_obj_class_t ht_framed_overlay_content_class;
extern const lv_obj_class_t ht_framed_overlay_footer_class;

class FramedOverlay : public AbstractOverlay
{
  public:
    struct Options
    {
        Options(bool header = true, bool footer = true)
            : has_header(header), has_footer(footer)
        {
        }

        bool has_header = true;
        bool has_footer = true;
    };

  protected:
    explicit FramedOverlay(biz::IBizFacade &biz);
    FramedOverlay(biz::IBizFacade &biz, Options options);

    _lv_obj_t *create(_lv_obj_t *parent) override;
    _lv_obj_t *contentHost() const;
    _lv_obj_t *header() const;
    _lv_obj_t *footer() const;
    void setHeaderText(const char *text);
    void setFooterText(const char *text);

  private:
    const Options _options;
    _lv_obj_t *_header = nullptr;
    _lv_obj_t *_content = nullptr;
    _lv_obj_t *_footer = nullptr;
};

} // namespace heltec::meshcore::ui
