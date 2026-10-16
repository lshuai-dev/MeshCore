#include "framed_overlay.hpp"
#include "ui/widgets/widget_classes.hpp"

namespace heltec::meshcore::ui {
FramedOverlay::FramedOverlay(biz::IBizFacade &biz, const lv_obj_class_t &root_class,
                             const lv_obj_class_t *content_class)
    : AbstractOverlay(biz, root_class),
      _content_class(content_class ? content_class : &ht_framed_overlay_content_class) {
}

bool FramedOverlay::buildSurface() {
    if (!AbstractOverlay::buildSurface()) {
        return false;
    }

    if (!buildHeader()) {
        return false;
    }
    if (!buildContent()) {
        return false;
    }
    if (!buildFooter()) {
        return false;
    }
    return true;
}

bool FramedOverlay::buildHeader() {
    _header = lv_obj_class_create_obj(&ht_overlay_header_class, _root);
    if (_header == nullptr) {
        return false;
    }
    lv_obj_class_init_obj(_header);

    lv_obj_set_width(_header, lv_pct(100));
    lv_label_set_long_mode(_header, LV_LABEL_LONG_CLIP);
    lv_label_set_text_static(_header, _title_text);
    return true;
}

bool FramedOverlay::buildContent() {
    _content = lv_obj_class_create_obj(_content_class, _root);
    if (_content == nullptr) {
        return false;
    }
    lv_obj_class_init_obj(_content);

    lv_obj_set_width(_content, lv_pct(100));
    lv_obj_set_flex_grow(_content, 1);
    lv_obj_set_flex_flow(_content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(_content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(_content, LV_OBJ_FLAG_SCROLLABLE);
    return true;
}

bool FramedOverlay::buildFooter() {
    _footer = lv_obj_class_create_obj(&ht_overlay_footer_class, _root);
    if (_footer == nullptr) {
        return false;
    }
    lv_obj_class_init_obj(_footer);

    lv_obj_set_width(_footer, lv_pct(100));
    lv_label_set_long_mode(_footer, LV_LABEL_LONG_CLIP);
    lv_label_set_text_static(_footer, _tool_tip_text);
    return true;
}

void FramedOverlay::setStaticTitle(const char *text) {
    const char *const value = text ? text : "";
    if (_title_text == value) {
        return;
    }
    _title_text = value;
    if (_header) {
        lv_label_set_text_static(_header, _title_text);
    }
}

void FramedOverlay::setStaticToolTip(const char *text) {
    const char *const value = text ? text : "";
    if (_tool_tip_text == value) {
        return;
    }
    _tool_tip_text = value;
    if (_footer) {
        lv_label_set_text_static(_footer, _tool_tip_text);
    }
}

} // namespace heltec::meshcore::ui
