#include "home_screen.hpp"

#include "MeshCore.h"
#include "ui/core/ht_container.hpp"
#include "ui/core/ht_meta_data.hpp"
#include "ui/core/ht_event.h"
#include "ui/screens/screen_widget_classes.hpp"

#include <lvgl.h>

namespace heltec::meshcore::ui
{
void HomeScreen::refreshSnapshot()
{
    _biz.formatNodeIdLine(_id_line, sizeof(_id_line));
    _message_count = _biz.messageCount();
    _companion_connected = _biz.hasCompanionConnection();
    _pairing_pin = _biz.companionPairingPin();
    refreshLabels();
}

void HomeScreen::onAppStateChanged(const AppStateEvent &event)
{
    switch (event.type)
    {
    case AppStateEventType::UnreadMessageCountChanged:
        _message_count = event.unread.count;
        refreshLabels();
        break;
    case AppStateEventType::CompanionChanged:
        _companion_connected = event.companion.connected;
        _pairing_pin = event.companion.pairing_pin;
        refreshLabels();
        break;
    case AppStateEventType::ConfigChanged:
        refreshSnapshot();
        break;
    default:
        break;
    }
}

void HomeScreen::onSurfaceRefresh()
{
    refreshSnapshot();
}

void HomeScreen::refreshLabels()
{
    if (_lblId)
    {
        lv_label_set_text_static(_lblId, _id_line);
    }
    if (_lblMsg)
    {
        lv_snprintf(_message_line, sizeof(_message_line), "MSG: %d", _message_count);
        lv_label_set_text_static(_lblMsg, _message_line);
    }
    if (_lblStatus)
    {
        if (_companion_connected)
        {
            lv_snprintf(_status_line, sizeof(_status_line), "< Connected >");
        }
        else if (_pairing_pin != 0)
        {
            lv_snprintf(_status_line, sizeof(_status_line), "Pin:%lu", (unsigned long)_pairing_pin);
        }
        else
        {
            lv_snprintf(_status_line, sizeof(_status_line), "COMP: idle");
        }
        lv_label_set_text_static(_lblStatus, _status_line);
    }
}

_lv_obj_t *HomeScreen::create(_lv_obj_t *parent)
{
    if (!AbstractScreen::create(parent))
        return nullptr;
    lv_obj_set_flex_align(_root, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    _lv_obj_t *const id_row = ht_container_create(_root);
    _lv_obj_t *const message_row = ht_container_create(_root);
    _lv_obj_t *const status_row = ht_container_create(_root);
    _lblId = id_row ? ht_screen_centered_label_create(id_row, "ID: --------") : nullptr;
    _lblMsg = message_row ? ht_screen_centered_label_create(message_row, "MSG: 0") : nullptr;
    _lblStatus = status_row ? ht_screen_centered_label_create(status_row, "") : nullptr;

    lv_snprintf(_id_line, sizeof(_id_line), "ID: --------");
    lv_snprintf(_message_line, sizeof(_message_line), "MSG: 0");
    _status_line[0] = '\0';
    if (_lblId)
        lv_label_set_text_static(_lblId, _id_line);
    if (_lblMsg)
        lv_label_set_text_static(_lblMsg, _message_line);
    if (_lblStatus)
        lv_label_set_text_static(_lblStatus, _status_line);

    struct HomeItem
    {
        _lv_obj_t *row;
        _lv_obj_t *label;
    };
    const HomeItem items[] = {
        { id_row, _lblId },
        { message_row, _lblMsg },
        { status_row, _lblStatus },
    };
    for (const HomeItem &item : items)
    {
        if (!item.row || !item.label)
            continue;
#if defined(HELTEC_T114_WITH_DISPLAY)
        lv_obj_set_size(item.label, lv_pct(100), LV_SIZE_CONTENT);
#else
        lv_obj_set_size(item.label, lv_pct(100), 12);
#endif
        lv_obj_clear_flag(item.label, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ON_FOCUS);
        lv_label_set_long_mode(item.label, LV_LABEL_LONG_CLIP);
        addFocusItem(item.label, item.row);
    }
    if (_lblStatus)
        lv_label_set_long_mode(_lblStatus, LV_LABEL_LONG_WRAP);

    return _root;
}

} // namespace heltec::meshcore::ui
