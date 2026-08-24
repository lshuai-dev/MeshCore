#include "surface_manager.hpp"

#include "ui_surface.hpp"

#include "heltec/drivers/input/key_input.hpp"

#if defined(MESH_DEBUG) && MESH_DEBUG
#include <Arduino.h>
#define SURFACE_MANAGER_LOG(fmt, ...) \
    do \
    { \
        Serial.printf("[surface-manager] " fmt "\n", ##__VA_ARGS__); \
        Serial.flush(); \
    } while (0)
#else
#define SURFACE_MANAGER_LOG(fmt, ...) ((void)0)
#endif

#include <lvgl.h>

namespace heltec::meshcore::ui
{

bool SurfaceManager::isAvailable(const UiSurface *surface)
{
    lv_obj_t *const root = surface ? surface->root() : nullptr;
    return root && lv_obj_is_valid(root);
}

UiSurface *SurfaceManager::active() const
{
    for (uint8_t index = _presented_count; index > 0; --index)
    {
        UiSurface *const surface = _presented[index - 1U];
        if (isAvailable(surface) && !lv_obj_has_flag(surface->root(), LV_OBJ_FLAG_HIDDEN))
        {
            return surface;
        }
    }
    return _root;
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
    if (!surface || !isAvailable(surface) || !surface->group())
    {
        clearInput();
        _bound_surface = nullptr;
        return;
    }

    const bool changed = _bound_surface != surface;
    lv_group_t *const group = surface->group();
    SURFACE_MANAGER_LOG("bind surface=%p root=%p group=%p count=%u changed=%u",
                        (void *)surface, (void *)surface->root(), (void *)group,
                        (unsigned)lv_group_get_obj_count(group), changed ? 1U : 0U);
    const bool preserve_editing = lv_group_get_editing(group);
    lv_group_set_default(group);
    heltec::meshcore::dal::key_input::set_group(group);
    lv_obj_t *const preferred = surface->focusedObject();
    lv_obj_t *focused = nullptr;
    if (preferred && lv_obj_is_valid(preferred) && lv_obj_get_group(preferred) == group)
    {
        focused = preferred;
    }
    else
    {
        focused = lv_group_get_focused(group);
        if (!focused || !lv_obj_is_valid(focused))
        {
            focused = nullptr;
        }
    }
    if (focused)
    {
        lv_group_focus_obj(focused);
    }
    else if (lv_group_get_obj_count(group) > 0)
    {
        lv_group_focus_next(group);
    }
    SURFACE_MANAGER_LOG("bound surface=%p focused=%p default=%p",
                        (void *)surface, (void *)lv_group_get_focused(group),
                        (void *)lv_group_get_default());
    if (preserve_editing)
    {
        lv_group_set_editing(group, true);
    }
    _bound_surface = surface;
    if (changed)
    {
        (void)ht_event_send(surface->root(), EventType::SURFACE_REFRESH);
    }
}

void SurfaceManager::clearInput()
{
    heltec::meshcore::dal::key_input::set_group(nullptr);
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

    /* The manager owns one base slot. Screen roots remain visible because
       the application TileView controls their clipping and visibility. */
    if (_root)
    {
        exit(_root);
    }
    _root = surface;
    enter(_root);
    bindInput(active());
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

    if (!isAvailable(surface) || surface == _root)
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

bool SurfaceManager::dispatchToActive(EventType type, const void *payload)
{
    UiSurface *const surface = active();
    return isAvailable(surface) && ht_event_send(surface->root(), type, payload);
}

void SurfaceManager::reconcile()
{
    uint8_t index = 0;
    while (index < _presented_count)
    {
        UiSurface *const surface = _presented[index];
        if (!isAvailable(surface))
        {
            remove(index);
            continue;
        }
        ++index;
    }
    bindInput(active());
}

} // namespace heltec::meshcore::ui
