/**
 * @file    theme_profile.hpp
 * @brief   Compile-time display and input profile for the UI theme.
 */
#ifndef HELTEC_UI_APP_UI_THEME_PROFILE_HPP
#define HELTEC_UI_APP_UI_THEME_PROFILE_HPP

/* Legacy UI_THEME_* macros remain accepted while variants migrate. */
#ifndef HELTEC_UI_DISPLAY_MONO
#if defined(UI_THEME_MONO) && UI_THEME_MONO
#define HELTEC_UI_DISPLAY_MONO 1
#else
#define HELTEC_UI_DISPLAY_MONO 0
#endif
#endif

#endif // HELTEC_UI_APP_UI_THEME_PROFILE_HPP
