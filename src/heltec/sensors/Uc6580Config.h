#pragma once

#include <Arduino.h>
#include <stdio.h>
#include <helpers/sensors/LocationProvider.h>

namespace Uc6580Config {
static constexpr uint16_t kNmeaRateHz = 1;
static constexpr uint16_t kNmeaIntervalMs = 1000;
static constexpr uint16_t kStartupWaitMs = 1200;

inline void waitAndPump(LocationProvider* location, uint16_t wait_ms) {
  if (!location || wait_ms == 0) return;
  const uint32_t started_at = millis();
  while ((uint32_t)(millis() - started_at) < wait_ms) {
    location->loop();
    delay(10);
  }
}

inline void sendCommand(LocationProvider* location, const char* sentence, uint16_t settle_ms) {
  if (!location || !sentence) return;
  location->sendSentence(sentence);
  waitAndPump(location, settle_ms);
}

inline void sendCfgMsg(LocationProvider* location, uint8_t msg_class, uint8_t msg_id,
                       uint8_t rate, uint16_t settle_ms = 100) {
  char cmd[32] = {0};
  snprintf(cmd, sizeof(cmd), "$CFGMSG,%u,%u,%u", msg_class, msg_id, rate);
  sendCommand(location, cmd, settle_ms);
}

inline void apply(LocationProvider* location) {
  if (!location || !location->isEnabled()) return;
  MESH_DEBUG_PRINTLN("[gps] configure UC6580: GGA+RMC %uHz nav_rate=%ums",
                     (unsigned)kNmeaRateHz, (unsigned)kNmeaIntervalMs);
  waitAndPump(location, kStartupWaitMs);
  char nav_cmd[40] = {0};
  snprintf(nav_cmd, sizeof(nav_cmd), "$CFGNAV,%u,%u,100", (unsigned)kNmeaIntervalMs,
           (unsigned)kNmeaIntervalMs);
  sendCommand(location, "$CFGSYS,h35155", 750);
  sendCommand(location, nav_cmd, 250);
  sendCfgMsg(location, 0, 1, 0);
  sendCfgMsg(location, 0, 2, 0);
  sendCfgMsg(location, 0, 3, 0);
  sendCfgMsg(location, 0, 5, 0);
  sendCfgMsg(location, 0, 6, 0);
  sendCfgMsg(location, 0, 7, 0);
  sendCfgMsg(location, 0, 8, 0);
  sendCfgMsg(location, 6, 0, 0);
  sendCfgMsg(location, 6, 1, 0);
  sendCfgMsg(location, 0, 0, 1);
  sendCfgMsg(location, 0, 4, 1);
  MESH_DEBUG_PRINTLN("[gps] UC6580 config sequence sent");
}
}
