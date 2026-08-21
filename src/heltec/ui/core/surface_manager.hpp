#pragma once

#include "ui_events.h"

#include <stdint.h>

namespace heltec::meshcore::ui
{

class UiSurface;

class SurfaceManager
{
  public:
    static constexpr uint8_t kMaxPresentedSurfaces = 4;

    void setRoot(UiSurface *surface);

    bool show(UiSurface *surface);
    bool hide(UiSurface *surface);

    UiSurface *root() const
    {
        return _root;
    }
    UiSurface *active() const;

    bool isActive(const UiSurface *surface) const;
    bool isVisible(const UiSurface *surface) const;

    bool dispatchToActive(UiEventType type, const void *payload = nullptr);
    void reconcile();

  private:
    static bool isAvailable(const UiSurface *surface);
    void enter(UiSurface *surface);
    void exit(UiSurface *surface);
    void bindInput(UiSurface *surface);
    int find(const UiSurface *surface) const;
    void remove(uint8_t index);

    UiSurface *_root = nullptr;
    UiSurface *_bound_surface = nullptr;
    UiSurface *_presented[kMaxPresentedSurfaces] = {};
    uint8_t _presented_count = 0;
};

} // namespace heltec::meshcore::ui
