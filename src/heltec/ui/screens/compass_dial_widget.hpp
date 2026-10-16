#pragma once

#include "compass_needle.hpp"
#include "ui/widgets/widget_classes.hpp"

#include <lvgl.h>

namespace heltec::meshcore::ui
{

enum class CompassDialNeedleKind
{
    BicolorHeading,
    FriendTurn
};

#ifdef COMPASS_DIAL_EDGE_MARGIN
static constexpr lv_coord_t kCompassDialEdgeMargin = COMPASS_DIAL_EDGE_MARGIN;
#else
static constexpr lv_coord_t kCompassDialEdgeMargin = 1;
#endif

/** Shared circular compass dial (hub + rotating ring + needle overlay). */
struct CompassDialWidget
{
    lv_obj_t *center_col = nullptr;
    lv_obj_t *hub = nullptr;
    lv_obj_t *needle = nullptr;
    lv_coord_t side = 0;
    uint8_t flags = 0;

    int16_t ring_heading_tenths = 0;
    int16_t needle_heading_tenths = 0;
    int16_t friend_turn_tenths = 0;
    bool friend_gps_fix = false;
    bool friend_on_target = false;
    bool friend_direction_valid = false;
    bool friend_near_target = false;
    bool center_marker_visible = false;
    CompassDialNeedleKind needle_kind = CompassDialNeedleKind::BicolorHeading;

    lv_coord_t min_side = 16;
    lv_coord_t edge_margin = kCompassDialEdgeMargin;

    static constexpr uint8_t kLayoutBusy = 0x01;
    static constexpr uint8_t kDialHidden = 0x08;

    bool create(lv_obj_t *parent_row);
    void setVisible(bool visible);
    void setRingHeading(int16_t heading_tenths);
    void setHeadingNeedle(int16_t heading_tenths);
    void setFriendNeedle(int16_t turn_tenths, bool gps_fix, bool direction_valid,
                         bool on_target, bool near_target);
    void setCenterMarkerVisible(bool visible);
    void layoutSize();
    void layoutLayers();
    void setLayersVisible(bool visible);
    void invalidateRing();
    void invalidateNeedle();

    bool dialHidden() const
    {
        return (flags & kDialHidden) != 0;
    }
    void setDialHidden(bool hidden)
    {
        if (hidden)
            flags |= kDialHidden;
        else
            flags &= ~kDialHidden;
    }
};

} // namespace heltec::meshcore::ui
