#include "SurfaceManager.hpp"

#include "Surface.hpp"

#include "heltec/drivers/input/KeyInput.hpp"

#include <lvgl.h>

namespace Heltec::MeshCore::Ui
{

bool SurfaceManager::isAvailable(const UiSurface *surface)
{
    lv_obj_t *const root = surface ? surface->root() : nullptr;
    return root && lv_obj_is_valid(root);
}

bool SurfaceManager::init()
{
    if (_input_group != nullptr)
    {
        return true;
    }
    _input_group = lv_group_create();
    if (_input_group == nullptr)
    {
        return false;
    }
    lv_group_set_wrap(_input_group, false);
    lv_group_set_default(_input_group);
    Heltec::MeshCore::Dal::KeyInput::setGroup(_input_group);
    return true;
}

bool SurfaceManager::registerSurface(UiSurface *surface)
{
    if (!init() || !isAvailable(surface))
    {
        return false;
    }
    if (isRegistered(surface))
    {
        return true;
    }
    lv_group_add_obj(_input_group, surface->root());
    return lv_obj_get_group(surface->root()) == _input_group;
}

bool SurfaceManager::isRegistered(const UiSurface *surface) const
{
    return isAvailable(surface) && _input_group != nullptr &&
           lv_obj_get_group(surface->root()) == _input_group;
}

UiSurface *SurfaceManager::active() const
{
    if (_input_group == nullptr)
    {
        return nullptr;
    }
    lv_obj_t *const root = lv_group_get_focused(_input_group);
    return root != nullptr ? static_cast<UiSurface *>(lv_obj_get_user_data(root)) : nullptr;
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
    if (surface != _base_surface)
    {
        lv_obj_add_flag(surface->root(), LV_OBJ_FLAG_HIDDEN);
    }
}

void SurfaceManager::bindInput(UiSurface *surface)
{
    if (!isAvailable(surface) || !registerSurface(surface))
    {
        clearInput();
        return;
    }
    Heltec::MeshCore::Dal::KeyInput::setGroup(_input_group);
    UiSurface *const previous = active();
    lv_group_focus_obj(surface->root());
    if (previous != surface)
    {
        surface->onActivated();
    }
    surface->_gridNav.restoreFocus();
}

void SurfaceManager::clearInput()
{
    Heltec::MeshCore::Dal::KeyInput::setGroup(nullptr);
}

int SurfaceManager::find(const UiSurface *surface) const
{
    if (!surface)
    {
        return -1;
    }
    for (uint8_t index = 0; index < _surface_count; ++index)
    {
        if (_surface_stack[index] == surface)
        {
            return static_cast<int>(index);
        }
    }
    return -1;
}

void SurfaceManager::remove(uint8_t index)
{
    if (index >= _surface_count)
    {
        return;
    }
    for (uint8_t current = index; current + 1U < _surface_count; ++current)
    {
        _surface_stack[current] = _surface_stack[current + 1U];
    }
    _surface_stack[--_surface_count] = nullptr;
}

void SurfaceManager::setRoot(UiSurface *surface)
{
    if (!isAvailable(surface))
    {
        return;
    }
    if (_base_surface == surface)
    {
        return;
    }

    /* The manager owns one base slot. Screen roots remain visible because
       the application TileView controls their clipping and visibility. */
    if (_base_surface)
    {
        exit(_base_surface);
    }
    _base_surface = surface;
    (void)registerSurface(_base_surface);
    enter(_base_surface);
    bindInput(_base_surface);
}

bool SurfaceManager::show(UiSurface *surface, bool visible)
{
    if (!visible)
    {
        const int index = find(surface);
        if (index < 0)
        {
            return false;
        }

        const bool was_active = isActive(surface);
        if (was_active)
        {
            clearInput();
        }
        exit(surface);
        remove(static_cast<uint8_t>(index));
        if (was_active)
        {
            UiSurface *const next =
                _surface_count > 0 ? _surface_stack[_surface_count - 1U] : _base_surface;
            bindInput(next);
        }
        return true;
    }

    if (!isAvailable(surface) || surface == _base_surface)
    {
        return false;
    }

    const int existing = find(surface);
    if (existing >= 0)
    {
        if (isActive(surface) && isVisible(surface))
        {
            bindInput(surface);
            return true;
        }
        remove(static_cast<uint8_t>(existing));
    }
    else if (_surface_count >= kMaxPresentedSurfaces)
    {
        return false;
    }

    (void)registerSurface(surface);
    _surface_stack[_surface_count++] = surface;
    enter(surface);
    if (!isAvailable(surface))
    {
        remove(static_cast<uint8_t>(_surface_count - 1U));
        UiSurface *const next =
            _surface_count > 0 ? _surface_stack[_surface_count - 1U] : _base_surface;
        bindInput(next);
        return false;
    }
    bindInput(surface);
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
    const bool managed = surface == _base_surface || find(surface) >= 0;
    return managed && !lv_obj_has_flag(surface->root(), LV_OBJ_FLAG_HIDDEN);
}

void SurfaceManager::dispatchAppStateToActive(const AppStateEvent &event)
{
    UiSurface *const surface = active();
    if (isAvailable(surface))
    {
        surface->onAppStateChanged(event);
    }
}

} // namespace Heltec::MeshCore::Ui
