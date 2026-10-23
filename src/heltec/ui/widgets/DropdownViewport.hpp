/**
 * @file DropdownViewport.hpp
 * @brief Viewport placement helpers for native LVGL dropdowns.
 */
#ifndef HELTEC_UI_WIDGETS_DROPDOWN_VIEWPORT_HPP
#define HELTEC_UI_WIDGETS_DROPDOWN_VIEWPORT_HPP

#include <lvgl.h>

namespace Heltec::MeshCore::Ui
{

void dropdown_viewport_realign(lv_obj_t *dropdown, lv_obj_t *viewport);

} // namespace Heltec::MeshCore::Ui

#endif // HELTEC_UI_WIDGETS_DROPDOWN_VIEWPORT_HPP
