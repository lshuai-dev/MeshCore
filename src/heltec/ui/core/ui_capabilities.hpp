#pragma once

// Device builds may turn individual UI capabilities off. Defaults preserve
// the feature set of every existing Heltec Mesh UI target.
#ifndef UI_INCLUDE_GPS_SCREEN
#define UI_INCLUDE_GPS_SCREEN 1
#endif

#ifndef UI_INCLUDE_KEYBOARD_OVERLAY
#define UI_INCLUDE_KEYBOARD_OVERLAY 1
#endif

#ifndef UI_DISPLAY_BACKLIGHT
#define UI_DISPLAY_BACKLIGHT 1
#endif
