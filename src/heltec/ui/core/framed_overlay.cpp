#include "framed_overlay.hpp"

#include "ht_meta_data_internal.hpp"
#include "ui_semantic_classes.hpp"

#include <lvgl.h>

namespace heltec::meshcore::ui
{

const lv_obj_class_t ht_framed_overlay_header_class = {
    &ht_overlay_bar_class,
    nullptr,
    nullptr,
#if LV_USE_USER_DATA
    nullptr,
#endif
    nullptr,
    LV_SIZE_CONTENT,
    LV_SIZE_CONTENT,
    LV_OBJ_CLASS_EDITABLE_INHERIT,
    LV_OBJ_CLASS_GROUP_DEF_INHERIT,
    sizeof(lv_label_t),
};

const lv_obj_class_t ht_framed_overlay_content_class = {
    &lv_obj_class,
    nullptr,
    nullptr,
#if LV_USE_USER_DATA
    nullptr,
#endif
    nullptr,
    LV_SIZE_CONTENT,
    LV_SIZE_CONTENT,
    LV_OBJ_CLASS_EDITABLE_INHERIT,
    LV_OBJ_CLASS_GROUP_DEF_INHERIT,
    sizeof(lv_obj_t),
};

const lv_obj_class_t ht_framed_overlay_footer_class = {
    &ht_overlay_bar_class,
    nullptr,
    nullptr,
#if LV_USE_USER_DATA
    nullptr,
#endif
    nullptr,
    LV_SIZE_CONTENT,
    LV_SIZE_CONTENT,
    LV_OBJ_CLASS_EDITABLE_INHERIT,
    LV_OBJ_CLASS_GROUP_DEF_INHERIT,
    sizeof(lv_label_t),
};

FramedOverlay::FramedOverlay(biz::IBizFacade &biz)
    : FramedOverlay(biz, Options{})
{
}

FramedOverlay::FramedOverlay(biz::IBizFacade &biz, Options options)
    : AbstractOverlay(biz), _options(options)
{
}

_lv_obj_t *FramedOverlay::create(_lv_obj_t *parent)
{
    if (!AbstractOverlay::create(parent))
    {
        return nullptr;
    }
    if (_content)
    {
        return _root;
    }

    if (_options.has_header)
    {
        _header = detail::create_object(&ht_framed_overlay_header_class, _root);
        if (!_header)
        {
            return nullptr;
        }
        lv_obj_set_width(_header, lv_pct(100));
        lv_label_set_long_mode(_header, LV_LABEL_LONG_CLIP);
    }

    _content = detail::create_object(&ht_framed_overlay_content_class, _root);
    if (!_content)
    {
        return nullptr;
    }
    lv_obj_set_width(_content, lv_pct(100));
    lv_obj_set_flex_grow(_content, 1);
    lv_obj_set_flex_flow(_content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(_content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(_content, LV_OBJ_FLAG_SCROLLABLE);

    if (_options.has_footer)
    {
        _footer = detail::create_object(&ht_framed_overlay_footer_class, _root);
        if (!_footer)
        {
            return nullptr;
        }
        lv_obj_set_width(_footer, lv_pct(100));
        lv_label_set_long_mode(_footer, LV_LABEL_LONG_CLIP);
    }
    return _root;
}

_lv_obj_t *FramedOverlay::contentHost() const
{
    return _content;
}

_lv_obj_t *FramedOverlay::header() const
{
    return _header;
}

_lv_obj_t *FramedOverlay::footer() const
{
    return _footer;
}

void FramedOverlay::setHeaderText(const char *text)
{
    if (_header)
    {
        lv_label_set_text(_header, text ? text : "");
    }
}

void FramedOverlay::setFooterText(const char *text)
{
    if (_footer)
    {
        lv_label_set_text(_footer, text ? text : "");
    }
}

} // namespace heltec::meshcore::ui
