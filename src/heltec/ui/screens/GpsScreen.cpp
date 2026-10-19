#include "GpsScreen.hpp"
#include "ui/widgets/WidgetClasses.hpp"
#include "ui/theme/UiTheme.hpp"
#include "ui/widgets/HtContainer.hpp"
#include "ui/core/HtEvent.h"
#include "ui/theme/WidgetTheme.hpp"

#include <lvgl.h>

namespace heltec::meshcore::ui
{
void format_micro_degrees(char *buffer, size_t capacity, long value)
{
    int64_t signed_value = value;
    const bool negative = signed_value < 0;
    if (negative)
        signed_value = -signed_value;
    lv_snprintf(buffer, capacity, "%s%ld.%06ld", negative ? "-" : "",
                (long)(signed_value / 1000000LL), (long)(signed_value % 1000000LL));
}

void format_altitude(char *buffer, size_t capacity, double altitude_m)
{
    int64_t centi = (int64_t)(altitude_m * 100.0 + (altitude_m >= 0.0 ? 0.5 : -0.5));
    const bool negative = centi < 0;
    if (negative)
        centi = -centi;
    lv_snprintf(buffer, capacity, "alt %s%ld.%02ld m", negative ? "-" : "", (long)(centi / 100),
                (long)(centi % 100));
}

bool GPSScreen::buildSurface()
{
    if (!AbstractScreen::buildSurface())
        return false;

    _lv_obj_t *const row_power = ht_container_create(_root);
    _lv_obj_t *gps_prefix = nullptr;
    if (row_power)
    {
        lv_obj_set_flex_align(row_power, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER,
                              LV_FLEX_ALIGN_CENTER);
        gps_prefix = lv_label_create(row_power);
        if(gps_prefix) lv_label_set_text_static(gps_prefix, "GPS");
        _sw_gps = lv_switch_create(row_power);
        if (gps_prefix)
            lv_obj_set_width(gps_prefix, LV_SIZE_CONTENT);
        if (_sw_gps)
        {
            lv_obj_clear_flag(_sw_gps, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_add_event_cb(_sw_gps, onGpsSwitchKey,
                                static_cast<lv_event_code_t>(LV_EVENT_KEY | LV_EVENT_PREPROCESS),
                                this);
            lv_obj_add_event_cb(_sw_gps, onGpsSwitchValueChanged, LV_EVENT_VALUE_CHANGED, this);
        }
    }

#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
    _lv_obj_t *const row_track = ht_container_create(_root);
    _lv_obj_t *track_prefix = nullptr;
    if (row_track)
    {
        lv_obj_set_flex_align(row_track, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER,
                              LV_FLEX_ALIGN_CENTER);
        track_prefix = lv_label_create(row_track);
        if(track_prefix) lv_label_set_text_static(track_prefix, "GPS Track");
        _sw_track = lv_switch_create(row_track);
        if (track_prefix)
            lv_obj_set_width(track_prefix, LV_SIZE_CONTENT);
        if (_sw_track)
        {
            lv_obj_clear_flag(_sw_track, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_add_event_cb(_sw_track, onGpsSwitchKey,
                                static_cast<lv_event_code_t>(LV_EVENT_KEY | LV_EVENT_PREPROCESS),
                                this);
            lv_obj_add_event_cb(_sw_track, onGpsSwitchValueChanged, LV_EVENT_VALUE_CHANGED, this);
        }
    }
#endif

    _lbl_fix = ht_label_create(&ht_screen_info_label_class, _root);
    _lbl_sat = ht_label_create(&ht_screen_info_label_class, _root);
    _lbl_latlon = ht_label_create(&ht_screen_info_label_class, _root);
    _lbl_alt = ht_label_create(&ht_screen_info_label_class, _root);
    _lbl_speed = ht_label_create(&ht_screen_info_label_class, _root);
    if (!_lbl_fix || !_lbl_sat || !_lbl_latlon || !_lbl_alt || !_lbl_speed)
        return false;
    lv_label_set_text_static(_lbl_fix, "no fix");
    lv_label_set_text_static(_lbl_sat, _sat_text);
    lv_label_set_text_static(_lbl_latlon, _latlon_text);
    lv_label_set_text_static(_lbl_alt, _alt_text);
    lv_label_set_text_static(_lbl_speed, _speed_text);

    _lv_obj_t *const labels[] = {
        gps_prefix, _lbl_fix,
        _lbl_sat,        _lbl_latlon,         _lbl_alt,          _lbl_speed,
#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
        track_prefix,
#endif
    };
    for (_lv_obj_t *label : labels)
    {
        if (!label)
            continue;
        lv_obj_clear_flag(label, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ON_FOCUS);
        lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
    }

    addFocusableObject(_sw_gps, row_power);
#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
    addFocusableObject(_sw_track, row_track);
#endif

    _lv_obj_t *const info_labels[] = {
        _lbl_fix, _lbl_sat, _lbl_latlon, _lbl_alt, _lbl_speed,
    };
    for (_lv_obj_t *const info_label : info_labels)
    {
        if (!info_label)
            continue;
        lv_obj_set_width(info_label, lv_pct(100));
    }

    return true;
}

void GPSScreen::setSwitchState(_lv_obj_t *sw, bool on)
{
    if (!sw)
        return;
    const bool current = lv_obj_has_state(sw, LV_STATE_CHECKED);
    if (current == on)
        return;
    _syncing_switches = true;
    if (on)
    {
        lv_obj_add_state(sw, LV_STATE_CHECKED);
    }
    else
    {
        lv_obj_clear_state(sw, LV_STATE_CHECKED);
    }
    _syncing_switches = false;
}

bool GPSScreen::ensureGpsEnabled(_lv_obj_t *dependent_switch)
{
    biz::IBizFacade::GpsStatus gps = _biz.gpsStatus();
    if (gps.enabled)
        return true;

    _biz.setGpsEnabled(true);
    gps = _biz.gpsStatus();
    updateGps(gps);
    if (gps.enabled)
        return true;

    setSwitchState(dependent_switch, false);
    _biz.showAlert("gps unavailable", 2000);
    return false;
}






void GPSScreen::onGpsSwitchKey(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_KEY)
        return;

    auto *self = static_cast<GPSScreen *>(lv_event_get_user_data(e));
    _lv_obj_t *const sw = lv_event_get_target(e);
    if (!self || !sw)
        return;
    bool known_switch = sw == self->_sw_gps;
#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
    known_switch = known_switch || sw == self->_sw_track;
#endif
    if (!known_switch)
        return;

    const uint32_t key = lv_event_get_key(e);
    if (key == LV_KEY_ESC)
    {
        // The switch has no local ESC state. Return directly to the screen Root
        // instead of relying on widget/parent bubble flags.
        (void)self->onKey(LV_KEY_ESC);
        lv_event_stop_processing(e);
        lv_event_stop_bubbling(e);
        return;
    }
    if (key != LV_KEY_ENTER)
        return;

    // Fully consume the KEY phase so ENTER cannot reach the screen-level
    // context action. The keypad's following press/release phase is left intact
    // so the switch performs its normal single toggle.
    lv_event_stop_processing(e);
    lv_event_stop_bubbling(e);

    // The keypad driver will still emit the normal press/release sequence after
    // this KEY event. The switch toggles once on release and then emits
    // LV_EVENT_VALUE_CHANGED; toggling here as well would apply the change twice.
}

void GPSScreen::onGpsSwitchValueChanged(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED)
        return;
    auto *self = static_cast<GPSScreen *>(lv_event_get_user_data(e));
    if (!self || self->_syncing_switches)
        return;

    _lv_obj_t *const sw = lv_event_get_target(e);
    if (!sw)
        return;
    const bool requested_on = lv_obj_has_state(sw, LV_STATE_CHECKED);

    if (sw == self->_sw_gps)
    {
        self->_biz.setGpsEnabled(requested_on);
        const biz::IBizFacade::GpsStatus actual = self->_biz.gpsStatus();
        self->updateGps(actual);
        self->_biz.showAlert(actual.enabled ? "gps: on"
                                            : (requested_on ? "gps unavailable" : "gps: off"),
                             requested_on && !actual.enabled ? 2000 : 800);
        return;
    }

#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
    if (sw == self->_sw_track)
    {
        if (requested_on && !self->ensureGpsEnabled(self->_sw_track))
            return;
        if (!self->_biz.setGpsTrackRecording(requested_on))
        {
            self->setSwitchState(self->_sw_track, self->_biz.gpsTrackRecording());
            return;
        }
        self->setSwitchState(self->_sw_track, self->_biz.gpsTrackRecording());
        self->_biz.showAlert(requested_on ? "gps track on" : "gps track off", 800);
    }
#endif
}





void GPSScreen::refreshSnapshot()
{
    updateGps(_biz.gpsStatus());
}

void GPSScreen::onExit()
{
    _biz.setGpsForegroundActive(false);
    AbstractScreen::onExit();
}

void GPSScreen::onEnter()
{
    AbstractScreen::onEnter();
    _biz.setGpsForegroundActive(true);
}

void GPSScreen::onAppStateChanged(const AppStateEvent &event)
{
    if (event.type == AppStateEventType::GpsChanged)
    {
        refreshSnapshot();
        return;
    }
    if (event.type == AppStateEventType::ConfigChanged ||
        event.type == AppStateEventType::TeamChanged)
        refreshSnapshot();
}

void GPSScreen::onActivated()
{
    refreshSnapshot();
}

void GPSScreen::updateGps(const biz::IBizFacade::GpsStatus &s)
{
    const bool show_fix_data = s.available && s.enabled && s.fix_valid;

    setSwitchState(_sw_gps, s.enabled);
#if defined(ENV_INCLUDE_GPS) && ENV_INCLUDE_GPS
    setSwitchState(_sw_track, _biz.gpsTrackRecording());
#endif

    if (_lbl_fix)
    {
        if (!s.available)
        {
            lv_label_set_text_static(_lbl_fix, "can't access gps");
        }
        else if (!s.enabled)
        {
            lv_label_set_text_static(_lbl_fix, "GPS Off");
        }
        else if (!s.powered)
        {
            lv_label_set_text_static(_lbl_fix, "GPS Sleep");
        }
        else if (!s.fix_valid)
        {
            lv_label_set_text_static(_lbl_fix, "no fix");
        }
        else
        {
            lv_label_set_text_static(_lbl_fix, "fix");
        }
    }

    if (_lbl_sat)
    {
        if (!show_fix_data)
        {
            lv_snprintf(_sat_text, sizeof(_sat_text), "sat --");
        }
        else
        {
            lv_snprintf(_sat_text, sizeof(_sat_text), "sat %d", s.satellites);
        }
        lv_label_set_text_static(_lbl_sat, _sat_text);
    }

    if (_lbl_latlon)
    {
        if (!show_fix_data)
        {
            lv_snprintf(_latlon_text, sizeof(_latlon_text), "lat -- lon --");
        }
        else
        {
            char lat[20];
            char lon[20];
            format_micro_degrees(lat, sizeof(lat), s.lat_micro);
            format_micro_degrees(lon, sizeof(lon), s.lon_micro);
            lv_snprintf(_latlon_text, sizeof(_latlon_text), "lat %s lon %s", lat, lon);
        }
        lv_label_set_text_static(_lbl_latlon, _latlon_text);
    }

    if (_lbl_alt)
    {
        if (!show_fix_data)
        {
            lv_snprintf(_alt_text, sizeof(_alt_text), "alt --");
        }
        else
        {
            format_altitude(_alt_text, sizeof(_alt_text), s.alt_m);
        }
        lv_label_set_text_static(_lbl_alt, _alt_text);
    }

    if (_lbl_speed)
    {
        if (!show_fix_data || s.speed_kph < 0.0f)
        {
            lv_snprintf(_speed_text, sizeof(_speed_text), "speed -- km/h");
        }
        else
        {
            int32_t speed_deci_kph = static_cast<int32_t>(s.speed_kph * 10.0f + 0.5f);
            if (speed_deci_kph < 0)
                speed_deci_kph = 0;
            if (speed_deci_kph > 9999)
                speed_deci_kph = 9999;
            lv_snprintf(_speed_text, sizeof(_speed_text), "speed %ld.%ld km/h",
                        static_cast<long>(speed_deci_kph / 10),
                        static_cast<long>(speed_deci_kph % 10));
        }
        lv_label_set_text_static(_lbl_speed, _speed_text);
    }
}

} // namespace heltec::meshcore::ui
