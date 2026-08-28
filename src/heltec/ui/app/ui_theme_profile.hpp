/**
 * @file    ui_theme_profile.hpp
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

#ifndef HELTEC_UI_INPUT_TOUCH
#if (defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH) || \
    (defined(HAS_TOUCH) && HAS_TOUCH)
#define HELTEC_UI_INPUT_TOUCH 1
#else
#define HELTEC_UI_INPUT_TOUCH 0
#endif
#endif

#endif // HELTEC_UI_APP_UI_THEME_PROFILE_HPP
