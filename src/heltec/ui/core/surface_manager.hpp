#pragma once

#include "ht_event.h"

#include <stdint.h>

namespace heltec::meshcore::ui
{

class UiSurface;

class SurfaceManager
{
  public:
    static constexpr uint8_t kMaxPresentedSurfaces = 4;

    /* Replaces the single base surface; TileView owns old screen visibility. */
    void setRoot(UiSurface *surface);

    bool show(UiSurface *surface, bool visible = true);

    UiSurface *active() const;

    bool isActive(const UiSurface *surface) const;
    bool isVisible(const UiSurface *surface) const;

    bool dispatchToActive(EventType type, const void *payload = nullptr);
    void reconcile();

  private:
    static bool isAvailable(const UiSurface *surface);
    void enter(UiSurface *surface);
    void exit(UiSurface *surface);
    void bindInput(UiSurface *surface);
    void clearInput();
    int find(const UiSurface *surface) const;
    void remove(uint8_t index);

    UiSurface *_root = nullptr;
    UiSurface *_bound_surface = nullptr;
    UiSurface *_presented[kMaxPresentedSurfaces] = {};
    uint8_t _presented_count = 0;
};

} // namespace heltec::meshcore::ui
