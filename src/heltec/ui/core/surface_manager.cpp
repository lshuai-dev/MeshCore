#include "surface_manager.hpp"

#include "group_manager.hpp"
#include "ui_surface.hpp"

#include <lvgl.h>

namespace heltec::meshcore::ui
{

bool SurfaceManager::isAvailable(const UiSurface *surface)
{
    return surface && surface->canPresent();
}

UiSurface *SurfaceManager::active() const
{
    return _presented_count > 0 ? _presented[_presented_count - 1] : _root;
}

void SurfaceManager::enter(UiSurface *surface)
{
    if (!isAvailable(surface))
    {
        return;
    }

    lv_obj_t *const object = surface->root();
    lv_obj_clear_flag(object, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(object);
    surface->onEnter();
}

void SurfaceManager::exit(UiSurface *surface)
{
    if (!isAvailable(surface))
    {
        return;
    }

    surface->onExit();
    if (surface != _root)
    {
        lv_obj_add_flag(surface->root(), LV_OBJ_FLAG_HIDDEN);
    }
}

void SurfaceManager::bindInput(UiSurface *surface)
{
    if (!surface || !surface->canBindInput())
    {
        GroupManager::instance().clear();
        _bound_surface = nullptr;
        return;
    }

    const bool changed = _bound_surface != surface;
    GroupManager::instance().bind(surface->group(), surface->focusedObject());
    _bound_surface = surface;
    if (changed)
    {
        (void)ui_event_send(surface->root(), UiEventType::SurfaceRefresh);
    }
}

int SurfaceManager::find(const UiSurface *surface) const
{
    if (!surface)
    {
        return -1;
    }
    for (uint8_t index = 0; index < _presented_count; ++index)
    {
        if (_presented[index] == surface)
        {
            return static_cast<int>(index);
        }
    }
    return -1;
}

void SurfaceManager::remove(uint8_t index)
{
    if (index >= _presented_count)
    {
        return;
    }
    for (uint8_t current = index; current + 1U < _presented_count; ++current)
    {
        _presented[current] = _presented[current + 1U];
    }
    _presented[--_presented_count] = nullptr;
}

void SurfaceManager::setRoot(UiSurface *surface)
{
    if (!isAvailable(surface))
    {
        return;
    }
    if (_root == surface)
    {
        reconcile();
        return;
    }

    if (_root)
    {
        exit(_root);
    }
    _root = surface;
    enter(_root);
    bindInput(active());
}

bool SurfaceManager::show(UiSurface *surface)
{
    if (!isAvailable(surface) || surface == _root)
    {
        return false;
    }

    const int existing = find(surface);
    if (existing >= 0)
    {
        remove(static_cast<uint8_t>(existing));
    }
    else if (_presented_count >= kMaxPresentedSurfaces)
    {
        return false;
    }

    _presented[_presented_count++] = surface;
    enter(surface);
    if (!isAvailable(surface))
    {
        remove(static_cast<uint8_t>(_presented_count - 1U));
        reconcile();
        return false;
    }
    bindInput(surface);
    return true;
}

bool SurfaceManager::hide(UiSurface *surface)
{
    const int index = find(surface);
    if (index < 0)
    {
        return false;
    }

    const bool was_active = isActive(surface);
    if (was_active)
    {
        GroupManager::instance().clear();
        _bound_surface = nullptr;
    }
    exit(surface);
    remove(static_cast<uint8_t>(index));
    if (was_active)
    {
        bindInput(active());
    }
    return true;
}

bool SurfaceManager::isActive(const UiSurface *surface) const
{
    return surface && active() == surface;
}

bool SurfaceManager::isVisible(const UiSurface *surface) const
{
    if (!isAvailable(surface))
    {
        return false;
    }
    if (surface == _root)
    {
        return true;
    }
    return find(surface) >= 0 &&
           !lv_obj_has_flag(surface->root(), LV_OBJ_FLAG_HIDDEN);
}

bool SurfaceManager::dispatchToActive(UiEventType type, const void *payload)
{
    UiSurface *const surface = active();
    return isAvailable(surface) && ui_event_send(surface->root(), type, payload);
}

void SurfaceManager::reconcile()
{
    uint8_t index = 0;
    while (index < _presented_count)
    {
        UiSurface *const surface = _presented[index];
        if (!isAvailable(surface) ||
            lv_obj_has_flag(surface->root(), LV_OBJ_FLAG_HIDDEN))
        {
            exit(surface);
            remove(index);
            continue;
        }
        ++index;
    }
    bindInput(active());
}

} // namespace heltec::meshcore::ui
