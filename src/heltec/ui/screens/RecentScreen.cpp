#include "RecentScreen.hpp"
#include "ui/widgets/HtContainer.hpp"
#include <lvgl.h>

namespace Heltec::MeshCore::Ui
{
namespace
{

void formatAge(int32_t secs, char *buf, size_t buf_size)
{
    if (!buf || buf_size == 0)
        return;
    if (secs < 0)
        secs = 0;
    if (secs < 60)
    {
        lv_snprintf(buf, buf_size, "%ds", static_cast<int>(secs));
    }
    else if (secs < 3600)
    {
        lv_snprintf(buf, buf_size, "%dm", static_cast<int>(secs / 60));
    }
    else
    {
        lv_snprintf(buf, buf_size, "%dh", static_cast<int>(secs / 3600));
    }
}

} // namespace

bool RecentScreen::build()
{
    if (!AbstractScreen::build())
        return false;
    lv_obj_add_flag(_root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(_root, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(_root, LV_SCROLLBAR_MODE_AUTO);

    // Measure one fully styled row first. The viewport and font metrics differ
    // between variants, so the number of reusable controls must be derived from
    // the actual LVGL layout instead of a device-specific constant.
    if (!createRowControl(0))
        return false;
    lv_snprintf(_age_text[0], sizeof(_age_text[0]), "00h");
    lv_label_set_text_static(_names[0], "Ag");
    lv_label_set_text_static(_ages[0], _age_text[0]);
    lv_obj_update_layout(_root);

    _row_control_count = calculateRowControlCount();
    for (int i = 1; i < _row_control_count; ++i)
    {
        if (!createRowControl(i))
        {
            // Keep the screen usable with the controls already allocated. A Recent
            // row is a reusable window slot, so reduced capacity only means smaller
            // pages; it must not abort the complete UI initialization at Splash.
            _row_control_count = i;
            break;
        }
    }

    _refresh_timer = lv_timer_create(refreshTimerCallback, 1000U, this);
    if (_refresh_timer)
    {
        lv_timer_pause(_refresh_timer);
    }
    return true;
}

bool RecentScreen::createRowControl(int index)
{
    if (index < 0 || index >= kMaxRowControls)
        return false;

    _rows[index] = ht_container_create(_root);
    if (!_rows[index])
        return false;
    _names[index] = lv_label_create(_rows[index]);
    _ages[index] = lv_label_create(_rows[index]);
    if (!_names[index] || !_ages[index])
    {
        lv_obj_del(_rows[index]);
        _rows[index] = nullptr;
        _names[index] = nullptr;
        _ages[index] = nullptr;
        return false;
    }
    lv_label_set_text_static(_names[index], "");
    lv_label_set_text_static(_ages[index], _age_text[index]);
    lv_obj_set_width(_names[index], 1);
    lv_obj_set_flex_grow(_names[index], 1);
    lv_label_set_long_mode(_names[index], LV_LABEL_LONG_DOT);
    lv_obj_set_width(_ages[index], LV_SIZE_CONTENT);
    return true;
}

int RecentScreen::calculateRowControlCount()
{
    lv_coord_t available_height = lv_obj_get_content_height(_root);
    if (available_height <= 0)
        available_height = lv_obj_get_height(_root);

    // During boot some parent layouts can still be resolving percentages. Walk
    // upward to the first usable viewport instead of falling back to a fixed
    // device-specific row count.
    for (lv_obj_t *obj = lv_obj_get_parent(_root); available_height <= 0 && obj;
         obj = lv_obj_get_parent(obj))
    {
        lv_obj_update_layout(obj);
        available_height = lv_obj_get_content_height(obj);
        if (available_height <= 0)
            available_height = lv_obj_get_height(obj);
    }

    lv_coord_t row_height = lv_obj_get_height(_rows[0]);
    if (row_height <= 0)
        row_height = lv_obj_get_height(_names[0]);
    if (available_height <= 0 || row_height <= 0)
        return 1;

    lv_coord_t gap = lv_obj_get_style_pad_row(_root, LV_PART_MAIN);
    if (gap < 0)
        gap = 0;
    const int count = static_cast<int>((available_height + gap) / (row_height + gap));
    if (count < 1)
        return 1;
    return count < kMaxRowControls ? count : kMaxRowControls;
}

void RecentScreen::onEnter()
{
    if (_refresh_timer)
    {
        lv_timer_reset(_refresh_timer);
        lv_timer_resume(_refresh_timer);
    }
    AbstractScreen::onEnter();
}

void RecentScreen::onExit()
{
    if (_refresh_timer)
        lv_timer_pause(_refresh_timer);
    AbstractScreen::onExit();
}

void RecentScreen::refreshRows()
{
    int item_count = _biz.fillRecentlyHeard(_items, kAdvertCapacity);
    if (item_count < 0)
        item_count = 0;
    if (item_count > kAdvertCapacity)
        item_count = kAdvertCapacity;
    _item_count = static_cast<uint8_t>(item_count);
    _age_tick_ms = lv_tick_get();

    if (_item_count == 0 || _row_control_count == 0)
    {
        _window_start = 0;
    }
    else if (_window_start >= _item_count)
    {
        const int last_page = (_item_count - 1) / _row_control_count;
        _window_start = static_cast<uint8_t>(last_page * _row_control_count);
    }
    renderWindow();
}

void RecentScreen::renderWindow()
{
    for (int row = 0; row < _row_control_count; ++row)
    {
        const int item = _window_start + row;
        if (item < _item_count)
        {
            formatAge(_items[item].age_seconds, _age_text[row], sizeof(_age_text[row]));
            lv_obj_clear_state(_rows[row], LV_STATE_DISABLED);
            lv_obj_clear_flag(_rows[row], LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text_static(_names[row], _items[item].name);
        }
        else if (row == 0 && _item_count == 0)
        {
            _age_text[row][0] = '\0';
            lv_obj_clear_state(_rows[row], LV_STATE_DISABLED);
            lv_obj_clear_flag(_rows[row], LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text_static(_names[row], "(no recent adverts)");
        }
        else
        {
            _age_text[row][0] = '\0';
            lv_obj_add_state(_rows[row], LV_STATE_DISABLED);
            lv_obj_add_flag(_rows[row], LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text_static(_names[row], "");
        }
        lv_label_set_text_static(_ages[row], _age_text[row]);
    }
}

void RecentScreen::movePage(int delta)
{
    if (_item_count == 0 || _row_control_count == 0 || delta == 0)
        return;

    const int page_count = (_item_count + _row_control_count - 1) / _row_control_count;
    if (page_count <= 1)
        return;

    int page = _window_start / _row_control_count;
    page += delta > 0 ? 1 : -1;
    if (page >= page_count)
        page = 0;
    else if (page < 0)
        page = page_count - 1;

    _window_start = static_cast<uint8_t>(page * _row_control_count);
    renderWindow();
}

bool RecentScreen::onKey(uint32_t key)
{
    if (key == LV_KEY_DOWN || key == LV_KEY_RIGHT || key == LV_KEY_NEXT)
    {
        movePage(1);
        return true;
    }
    if (key == LV_KEY_UP || key == LV_KEY_LEFT || key == LV_KEY_PREV)
    {
        movePage(-1);
        return true;
    }
    return AbstractScreen::onKey(key);
}

void RecentScreen::onAppStateChanged(const AppStateEvent &event)
{
    if (event.type == AppStateEventType::RecentHeardChanged)
        refreshRows();
}

void RecentScreen::onActivated()
{
    refreshRows();
}

void RecentScreen::refreshTimerCallback(lv_timer_t *timer)
{
    auto *self = timer ? static_cast<RecentScreen *>(timer->user_data) : nullptr;
    if (!self)
        return;

    const uint32_t now = lv_tick_get();
    const uint32_t elapsed_seconds = (now - self->_age_tick_ms) / 1000U;
    if (elapsed_seconds == 0)
        return;
    self->_age_tick_ms += elapsed_seconds * 1000U;
    for (uint8_t index = 0; index < self->_item_count; ++index)
    {
        const int32_t age = self->_items[index].age_seconds;
        self->_items[index].age_seconds =
            age > INT32_MAX - static_cast<int32_t>(elapsed_seconds)
                ? INT32_MAX
                : age + static_cast<int32_t>(elapsed_seconds);
    }
    self->renderWindow();
}

} // namespace Heltec::MeshCore::Ui
