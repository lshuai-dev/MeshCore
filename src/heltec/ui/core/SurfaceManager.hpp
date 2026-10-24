#pragma once

#include "UiMessage.hpp"

#include <stdint.h>

namespace Heltec::MeshCore::Ui
{

class UiSurface;
struct AppStateEvent;

class SurfaceManager
{
  public:
    static constexpr uint8_t kMaxPresentedSurfaces = 4;

    bool registerSurface(UiSurface *surface);

    /* Replaces the single base surface; TileView owns old screen visibility. */
    void setRoot(UiSurface *surface);

    bool show(UiSurface *surface, bool visible = true);

    UiSurface *active() const;

    bool isActive(const UiSurface *surface) const;
    bool isVisible(const UiSurface *surface) const;

    void dispatchAppStateToActive(const AppStateEvent &event);

  private:
    bool init();
    static bool isAvailable(const UiSurface *surface);
    bool isRegistered(const UiSurface *surface) const;
    void enter(UiSurface *surface);
    void exit(UiSurface *surface);
    void bindInput(UiSurface *surface);
    void clearInput();
    int find(const UiSurface *surface) const;
    void remove(uint8_t index);

    lv_group_t *_input_group = nullptr;
    UiSurface *_base_surface = nullptr;
    UiSurface *_surface_stack[kMaxPresentedSurfaces] = {};
    uint8_t _surface_count = 0;
};

} // namespace Heltec::MeshCore::Ui
