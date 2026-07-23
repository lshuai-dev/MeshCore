#pragma once

#if defined(MESH_DEBUG) && MESH_DEBUG
#include <Arduino.h>
#define BTN_UI_LOG(fmt, ...) \
  do { \
    Serial.printf("[btn] " fmt "\n", ##__VA_ARGS__); \
    Serial.flush(); \
  } while (0)
#else
#define BTN_UI_LOG(fmt, ...) ((void)0)
#endif
