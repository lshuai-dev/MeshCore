/**
 * @file    team_log.hpp
 * @brief   Compile-time Team feature diagnostics.
 * @version 1.0
 * @date    2026-09-04
 */
#ifndef HELTEC_TEAM_LOG_HPP
#define HELTEC_TEAM_LOG_HPP

#include "app/team_config.hpp"

#if HELTEC_TEAM_FEATURE && HELTEC_TEAM_LOG >= 1
#include <Arduino.h>
#define TEAM_INFO(format, ...) \
    Serial.printf("[TEAM] " format "\n", ##__VA_ARGS__)
#else
#define TEAM_INFO(...) do { } while (0)
#endif

/* Verbose frame/location diagnostics are absent from normal firmware. */
#if HELTEC_TEAM_FEATURE && HELTEC_TEAM_LOG >= 2
#define TEAM_LOG(format, ...) Serial.printf("[TEAM] " format "\n", ##__VA_ARGS__)
#else
#define TEAM_LOG(...) do { } while (0)
#endif

#endif
