#pragma once

#include "compass_needle.hpp"
#include "ui/core/ui_semantic_classes.hpp"

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
    lv_obj_t *ring = nullptr;
    lv_obj_t *needle = nullptr;
    lv_coord_t side = 0;
    uint8_t flags = 0;

    int16_t ring_heading_tenths = 0;
    int16_t needle_heading_tenths = 0;
    float friend_turn_deg = 0.f;
    bool friend_gps_fix = false;
    bool friend_on_target = false;
    bool friend_direction_valid = false;
    bool friend_near_target = false;
    CompassDialNeedleKind needle_kind = CompassDialNeedleKind::BicolorHeading;

    lv_coord_t min_side = 16;
    lv_coord_t edge_margin = kCompassDialEdgeMargin;

    static constexpr uint8_t kLayoutBusy = 0x01;
    static constexpr uint8_t kDialHidden = 0x08;

    bool create(lv_obj_t *parent_row);
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
