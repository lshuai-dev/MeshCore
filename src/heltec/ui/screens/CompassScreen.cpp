#if defined(ENV_INCLUDE_COMPASS) && ENV_INCLUDE_COMPASS
#include "CompassScreen.hpp"
#include "CompassCommon.hpp"
#include "ui/theme/UiTheme.hpp"
#include "ui/core/HtEvent.h"

#include <Arduino.h>
#include <lvgl.h>
#include <string.h>

#ifndef COMPASS_HEADING_OFFSET_DEG
#define COMPASS_HEADING_OFFSET_DEG 0
#endif

static constexpr lv_coord_t kCompassInfoColumnWidthPct = 40;
static constexpr lv_coord_t kCompassInfoColumnMarginRight = 4;
static constexpr lv_coord_t kCompassInfoColumnPadLeft = 0;

namespace heltec::meshcore::ui
{

namespace
{

constexpr int16_t kHeadingInvalid = -10000;
constexpr int16_t kMagInvalid = 10000;
constexpr uint8_t kFLastHw = 0x02;
constexpr uint8_t kFLastValid = 0x04;

int mag_centi(float gauss)
{
    return (int)(gauss * 100.f + (gauss >= 0.f ? 0.5f : -0.5f));
}

int clamp_q(int q)
{
    if (q < 0)
        return 0;
    if (q > 3)
        return 3;
    return q;
}

void set_static_text(lv_obj_t *label, char *buffer, size_t capacity, const char *text)
{
    if (!label || !buffer || capacity == 0)
        return;
    const char *value = text ? text : "";
    if (strncmp(buffer, value, capacity) == 0)
        return;
    lv_snprintf(buffer, capacity, "%s", value);
    lv_label_set_text_static(label, buffer);
}

void set_q_label(lv_obj_t *label, char *buffer, size_t capacity, int quality, bool active)
{
    if (!label || !buffer || capacity == 0)
        return;
    if (!active)
    {
        lv_snprintf(buffer, capacity, "Q:--");
        lv_label_set_text_static(label, buffer);
        return;
    }
    const int q = clamp_q(quality);
    const uint32_t color = lv_color_to32(q >= 3 ? ui_color_success() : ui_color_error());
    lv_snprintf(buffer, capacity, "Q:#%06lx %d#", (unsigned long)(color & 0xFFFFFFUL), q);
    lv_label_set_text_static(label, buffer);
}

} // namespace

void CompassScreen::clearMagLabels()
{
    for (int i = 0; i < 3; ++i)
    {
        _last_mag_centi[i] = kMagInvalid;
    }
    lv_snprintf(_mag_text, sizeof(_mag_text), "X:--\nY:--\nZ:--");
    if (_lbl_mag)
        lv_label_set_text_static(_lbl_mag, _mag_text);
}

void CompassScreen::showUnavailable(const biz::CompassUi &c, const char *hdg_text)
{
    _dial.setVisible(false);
    _dial.invalidateNeedle();
    set_static_text(_lbl_hdg, _hdg_text, sizeof(_hdg_text), hdg_text);
    set_q_label(_lbl_q, _q_text, sizeof(_q_text), 0, false);
    clearMagLabels();
    _heading_dial_track_tenths = kHeadingInvalid;
    _heading_label_tenths = kHeadingInvalid;
    _last_quality = c.quality;
}

void CompassScreen::updateDialHeading(float heading_deg)
{
    const float hdg = compass_screen_heading(heading_deg, COMPASS_HEADING_OFFSET_DEG);
    const int16_t hdg_t = (int16_t)compass_heading_tenths(hdg);
    const int16_t dial_t = compass_dial_redraw_tenths(hdg_t);
    _dial.setRingHeading(dial_t);
    _dial.setHeadingNeedle(dial_t);
    if (_dial.side > 0)
    {
        _dial.setRingHeading(dial_t);
        _dial.setHeadingNeedle(dial_t);
    }
}

bool CompassScreen::buildSurface()
{
    if (!AbstractScreen::buildSurface())
        return false;

    compass_init_dial_row(_root, kCompassInfoColumnMarginRight);

    _dial.min_side = 20;
    _dial.needle_kind = CompassDialNeedleKind::BicolorHeading;
    if (!_dial.create(_root))
        return false;

    lv_obj_t *info = compass_create_info_column(_root, kCompassInfoColumnWidthPct,
                                                kCompassInfoColumnPadLeft, 0);
    if (info)
    {
        _lbl_hdg = ht_label_create(&ht_compass_label_class, info);
        compass_style_info_label(_lbl_hdg, "HDG:--", LV_LABEL_LONG_CLIP);
        if (_lbl_hdg)
            lv_label_set_text_static(_lbl_hdg, _hdg_text);

        _lbl_q = ht_label_create(&ht_compass_label_class, info);
        if (_lbl_q)
        {
            compass_style_info_label(_lbl_q, _q_text, LV_LABEL_LONG_CLIP);
            lv_label_set_recolor(_lbl_q, true);
        }

        _lbl_mag = ht_label_create(&ht_compass_label_class, info);
        if (_lbl_mag)
        {
            lv_obj_set_width(_lbl_mag, lv_pct(100));
            compass_style_info_label(_lbl_mag, _mag_text, LV_LABEL_LONG_CLIP);
        }
        lv_obj_move_foreground(info);
    }

    lv_obj_update_layout(_root);
    _dial.layoutSize();
    updateDialHeading(0.f);
    return true;
}

void CompassScreen::refresh_ui(const biz::CompassUi &c)
{
    const uint8_t prev = _state_flags & (kFLastHw | kFLastValid);
    uint8_t next = 0;
    if (c.has_hardware)
        next |= kFLastHw;
    if (c.heading_valid)
        next |= kFLastValid;
    const bool state_changed = (prev != next) || (c.quality != _last_quality);

    if (!c.has_hardware)
    {
        if (!state_changed)
            return;
        _state_flags = 0;
        showUnavailable(c, "No compass");
        return;
    }

    if (!c.heading_valid)
    {
        if (!state_changed)
            return;
        _state_flags = kFLastHw;
        showUnavailable(c, "Starting...");
        return;
    }

    _state_flags = kFLastHw | kFLastValid;
    _dial.setVisible(true);

    const float hdg = compass_screen_heading(c.heading_deg, COMPASS_HEADING_OFFSET_DEG);
    const int16_t hdg_t = (int16_t)compass_heading_tenths(hdg);
    const int16_t dial_t = compass_dial_redraw_tenths(hdg_t);
    const bool dial_moved =
        (_heading_dial_track_tenths == kHeadingInvalid) || dial_t != _heading_dial_track_tenths;
    const bool label_moved =
        (_heading_label_tenths == kHeadingInvalid) || hdg_t != _heading_label_tenths;

    if (state_changed)
    {
        _dial.invalidateRing();
        _dial.invalidateNeedle();
    }

    if (dial_moved)
    {
        _heading_dial_track_tenths = dial_t;
        updateDialHeading(c.heading_deg);
    }

    if (state_changed || label_moved)
    {
        _heading_label_tenths = hdg_t;
        if (_lbl_hdg)
        {
            lv_snprintf(_hdg_text, sizeof(_hdg_text), "HDG:%3d.%d", hdg_t / 10, hdg_t % 10);
            lv_label_set_text_static(_lbl_hdg, _hdg_text);
        }
    }

    if (state_changed)
    {
        set_q_label(_lbl_q, _q_text, sizeof(_q_text), c.quality, true);
        _last_quality = c.quality;
    }

    bool mag_changed = state_changed || label_moved;
    if (!mag_changed)
    {
        for (int i = 0; i < 3; ++i)
        {
            if (mag_centi(c.mag_xyz[i]) != _last_mag_centi[i])
            {
                mag_changed = true;
                break;
            }
        }
    }
    if (mag_changed)
    {
        int centi[3] = {};
        for (int i = 0; i < 3; ++i)
        {
            centi[i] = mag_centi(c.mag_xyz[i]);
            _last_mag_centi[i] = (int16_t)centi[i];
        }
        lv_snprintf(_mag_text, sizeof(_mag_text), "X:%3d.%02d\nY:%3d.%02d\nZ:%3d.%02d",
                    centi[0] / 100, (centi[0] < 0 ? -centi[0] : centi[0]) % 100,
                    centi[1] / 100, (centi[1] < 0 ? -centi[1] : centi[1]) % 100,
                    centi[2] / 100, (centi[2] < 0 ? -centi[2] : centi[2]) % 100);
        if (_lbl_mag)
            lv_label_set_text_static(_lbl_mag, _mag_text);
    }
}

void CompassScreen::maybeScheduleAutoCal()
{
    if (_skip_auto_cal_once)
    {
        _skip_auto_cal_once = false;
        return;
    }

    const biz::CompassUi &c = _biz.compassUi();
    if (!c.has_hardware || !c.heading_valid)
        return;
    if (_biz.compassHasStoredCalibration() && c.quality >= 3)
        return;
    _pending_auto_cal = true;
    _auto_cal_due_ms = millis() + 200U;
}

void CompassScreen::skipAutoCalibrationOnce()
{
    _skip_auto_cal_once = true;
}

void CompassScreen::onEnter()
{
    AbstractScreen::onEnter();
    _pending_auto_cal = false;
    _auto_cal_due_ms = 0;

    if (_root)
        lv_obj_update_layout(_root);
    _dial.layoutSize();
}

void CompassScreen::onExit()
{
    _pending_auto_cal = false;
    _auto_cal_due_ms = 0;
    _dial.setVisible(false);
    AbstractScreen::onExit();
}

void CompassScreen::refresh()
{
    if (_dial.side == 0)
    {
        if (_root)
            lv_obj_update_layout(_root);
        _dial.layoutSize();
    }

    const uint32_t now = millis();
    if (_pending_auto_cal && (int32_t)(now - _auto_cal_due_ms) >= 0)
    {
        _pending_auto_cal = false;
        _auto_cal_due_ms = 0;
        (void)emitEvent(EventType::CalibrationOpen);
        return;
    }

    refresh_ui(_biz.compassUi());
}

void CompassScreen::onAppStateChanged(const AppStateEvent &event)
{
    if (event.type == AppStateEventType::CompassChanged ||
        event.type == AppStateEventType::ConfigChanged)
    {
        refresh();
    }
}

void CompassScreen::onActivated()
{
    refresh();
    maybeScheduleAutoCal();
}

} // namespace heltec::meshcore::ui

#endif // ENV_INCLUDE_COMPASS
