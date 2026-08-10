#include "power_mgr.hpp"

#include <Arduino.h>

namespace heltec::meshcore::power {

PowerConfig defaultPowerConfig() {
  PowerConfig config{};
#if defined(BATT_MIN_MILLIVOLTS)
  config.battery_min_mv = BATT_MIN_MILLIVOLTS;
#elif defined(BAT_MIN_MV)
  config.battery_min_mv = BAT_MIN_MV;
#endif
#if defined(BATT_MAX_MILLIVOLTS)
  config.battery_max_mv = BATT_MAX_MILLIVOLTS;
#elif defined(BAT_MAX_MV)
  config.battery_max_mv = BAT_MAX_MV;
#endif
  return config;
}

bool PowerMgr::deadlineReached(uint32_t now_ms, uint32_t deadline_ms) {
  return static_cast<int32_t>(now_ms - deadline_ms) >= 0;
}

uint8_t PowerMgr::batteryPercent(uint16_t mv) const {
  if (mv == 0 || _config.battery_max_mv <= _config.battery_min_mv) return 0;
  if (mv <= _config.battery_min_mv) return 0;
  if (mv >= _config.battery_max_mv) return 100;
  return static_cast<uint8_t>(
      (static_cast<uint32_t>(mv - _config.battery_min_mv) * 100U) /
      (_config.battery_max_mv - _config.battery_min_mv));
}

void PowerMgr::begin(uint32_t now_ms, const PowerConfig& config) {
  _config = config;
  _snapshot = PowerSnapshot{};
  _snapshot.display_on = _config.display_available && _port.isDisplayOn();
  _snapshot.gps_available = _port.isGpsAvailable();
  _snapshot.gps_powered = _snapshot.gps_available && _port.isGpsPowered();
  _display_last_activity_ms = _snapshot.display_on ? now_ms : 0;
  _display_activity_valid = _snapshot.display_on;
  _display_off_since_ms = _snapshot.display_on ? 0 : now_ms;
  _gps_retry_due_ms = now_ms;
  _begun = true;
  pollBattery(now_ms, true);
  pollPowerSource(now_ms, true);
  (void)reconcileGpsPower(now_ms);
}

void PowerMgr::notify(PowerChangeMask changes) {
  if (changes != 0 && _listener) _listener->onPowerChanged(changes, _snapshot);
}

void PowerMgr::setFault(PowerFault fault) {
  if (_snapshot.fault == fault) return;
  _snapshot.fault = fault;
  notify(mask(PowerChange::Fault));
}

void PowerMgr::pollBattery(uint32_t now_ms, bool force) {
  if (!force && _battery_polled &&
      static_cast<uint32_t>(now_ms - _battery_last_poll_ms) < _config.battery_poll_ms) {
    return;
  }
  _battery_polled = true;
  _battery_last_poll_ms = now_ms;
  const uint16_t mv = _board.getBattMilliVolts();
  const uint8_t percent = batteryPercent(mv);
  if (_snapshot.battery_mv == mv && _snapshot.battery_percent == percent) return;
  _snapshot.battery_mv = mv;
  _snapshot.battery_percent = percent;
  notify(mask(PowerChange::Battery));
}

void PowerMgr::pollPowerSource(uint32_t now_ms, bool force) {
  if (!force && _source_polled &&
      static_cast<uint32_t>(now_ms - _source_last_poll_ms) < _config.source_poll_ms) {
    return;
  }
  _source_polled = true;
  _source_last_poll_ms = now_ms;
  const mesh::PowerSource source = _port.getPowerSource();
  if (_snapshot.source == source) return;
  _snapshot.source = source;
  notify(mask(PowerChange::Source));
  (void)reconcileGpsPower(now_ms);
}

void PowerMgr::notifyUserActivity(uint32_t now_ms) {
  if (!_begun || !_config.display_available || _display_auto_off_ms == 0) return;
  _display_last_activity_ms = now_ms;
  _display_activity_valid = true;
  if (!_snapshot.display_on) (void)setDisplayOn(true, now_ms);
}

void PowerMgr::setDisplayAutoOffMs(uint32_t timeout_ms, uint32_t now_ms) {
  _display_auto_off_ms = timeout_ms;
  _display_last_activity_ms = timeout_ms && _snapshot.display_on ? now_ms : 0;
  _display_activity_valid = timeout_ms != 0 && _snapshot.display_on;
}

void PowerMgr::setDisplayTimeoutInhibited(uint16_t reason, bool inhibited,
                                          uint32_t now_ms) {
  const uint16_t before = _display_inhibit_mask;
  if (inhibited) {
    _display_inhibit_mask |= reason;
  } else {
    _display_inhibit_mask &= static_cast<uint16_t>(~reason);
  }
  if (before != 0 && _display_inhibit_mask == 0 && _snapshot.display_on) {
    _display_last_activity_ms = now_ms;
    _display_activity_valid = true;
  }
}

bool PowerMgr::setDisplayOn(bool on, uint32_t now_ms) {
  if (!_config.display_available) return !on;
  if (_snapshot.display_on == on) {
    if (on && _display_auto_off_ms != 0) {
      _display_last_activity_ms = now_ms;
      _display_activity_valid = true;
    }
    return true;
  }

  _port.setDisplayOn(on);
  const bool actual = _port.isDisplayOn();
  if (actual != on) {
    setFault(PowerFault::DisplayApplyFailed);
    return false;
  }

  _snapshot.display_on = actual;
  if (actual) {
    _display_last_activity_ms = _display_auto_off_ms ? now_ms : 0;
    _display_activity_valid = _display_auto_off_ms != 0;
    _display_off_since_ms = 0;
  } else {
    _display_last_activity_ms = 0;
    _display_activity_valid = false;
    _display_off_since_ms = now_ms;
  }
  if (_snapshot.fault == PowerFault::DisplayApplyFailed) setFault(PowerFault::None);
  notify(mask(PowerChange::Display));
  (void)reconcileGpsPower(now_ms);
  return true;
}

void PowerMgr::processDisplayTimeout(uint32_t now_ms) {
  if (!_config.display_available || !_snapshot.display_on ||
      _display_auto_off_ms == 0 || !_display_activity_valid ||
      _display_inhibit_mask != 0) {
    return;
  }
  if (static_cast<uint32_t>(now_ms - _display_last_activity_ms) <
      _display_auto_off_ms) {
    return;
  }
  (void)setDisplayOn(false, now_ms);
}

void PowerMgr::setGpsAllowed(bool allowed) {
  if (_snapshot.gps_allowed == allowed) return;
  _snapshot.gps_allowed = allowed;
  _gps_policy_changed = true;
}

void PowerMgr::setGpsDemand(GpsDemand demand, bool active) {
  const uint16_t bit = mask(demand);
  if (active) {
    _requested_gps_demands |= bit;
  } else {
    _requested_gps_demands &= static_cast<uint16_t>(~bit);
  }
}

uint16_t PowerMgr::effectiveGpsDemands(uint32_t now_ms) const {
  uint16_t demands = _requested_gps_demands;
  demands &= static_cast<uint16_t>(~mask(GpsDemand::ExternalPower));
  if (_snapshot.source == mesh::PowerSource::External) {
    demands |= mask(GpsDemand::ExternalPower);
  }

  if (!_snapshot.display_on &&
      static_cast<uint32_t>(now_ms - _display_off_since_ms) >=
          _config.gps_screen_off_grace_ms) {
    demands &= static_cast<uint16_t>(~(mask(GpsDemand::GpsScreen) |
                                       mask(GpsDemand::MapScreen)));
  }
  return demands;
}

bool PowerMgr::reconcileGpsPower(uint32_t now_ms) {
  if (!_begun) return true;
  const bool available = _port.isGpsAvailable();
  const bool available_changed = _snapshot.gps_available != available;
  _snapshot.gps_available = available;
  const bool policy_changed = _gps_policy_changed;
  _gps_policy_changed = false;
  const uint16_t demands = effectiveGpsDemands(now_ms);
  const bool demand_changed = _snapshot.gps_demands != demands;
  _snapshot.gps_demands = demands;

  const bool actual = _snapshot.gps_available && _port.isGpsPowered();
  const bool desired = _snapshot.gps_available && _snapshot.gps_allowed && demands != 0;
  if (actual == desired) {
    const bool powered_changed = _snapshot.gps_powered != actual;
    _snapshot.gps_powered = actual;
    if (_snapshot.fault == PowerFault::GpsApplyFailed) setFault(PowerFault::None);
    if (available_changed || policy_changed || demand_changed || powered_changed) {
      notify(mask(PowerChange::Gps));
    }
    return true;
  }

  if (!deadlineReached(now_ms, _gps_retry_due_ms)) {
    if (available_changed || policy_changed || demand_changed) {
      notify(mask(PowerChange::Gps));
    }
    return false;
  }
  _gps_retry_due_ms = now_ms + _config.gps_retry_ms;
  if (!_port.setGpsPowered(desired)) {
    setFault(PowerFault::GpsApplyFailed);
    if (available_changed || policy_changed || demand_changed) {
      notify(mask(PowerChange::Gps));
    }
    return false;
  }

  const bool applied = _snapshot.gps_available && _port.isGpsPowered();
  _snapshot.gps_powered = applied;
  if (applied != desired) {
    setFault(PowerFault::GpsApplyFailed);
    if (available_changed || policy_changed || demand_changed) {
      notify(mask(PowerChange::Gps));
    }
    return false;
  }
  if (_snapshot.fault == PowerFault::GpsApplyFailed) setFault(PowerFault::None);
  notify(mask(PowerChange::Gps));
  MESH_DEBUG_PRINTLN("[power] gps=%s demands=0x%04x source=%u display=%u",
                     desired ? "on" : "off", demands,
                     static_cast<unsigned>(_snapshot.source),
                     _snapshot.display_on ? 1U : 0U);
  return true;
}

void PowerMgr::requestPowerOff(uint32_t now_ms) {
  if (!_begun || _snapshot.system_state != SystemPowerState::Running) return;
  _snapshot.system_state = SystemPowerState::ShuttingDown;
  _system_transition_due_ms = now_ms + _config.shutdown_feedback_timeout_ms;
  _port.startShutdownFeedback();
  notify(mask(PowerChange::System));
}

void PowerMgr::requestReboot(uint32_t delay_ms, uint32_t now_ms) {
  if (!_begun || _snapshot.system_state != SystemPowerState::Running) return;
  _snapshot.system_state = SystemPowerState::Rebooting;
  _system_transition_due_ms = now_ms + delay_ms;
  notify(mask(PowerChange::System));
}

void PowerMgr::finishPowerOff() {
  if (_snapshot.gps_powered && _port.setGpsPowered(false)) {
    _snapshot.gps_powered = false;
    notify(mask(PowerChange::Gps));
  }
  if (_config.display_available && _snapshot.display_on) {
    _port.setDisplayOn(false);
    _snapshot.display_on = _port.isDisplayOn();
    notify(mask(PowerChange::Display));
  }
  _port.powerOffRadio();
  _board.powerOff();
  _snapshot.system_state = SystemPowerState::Fault;
  setFault(PowerFault::PowerOffReturned);
  notify(mask(PowerChange::System));
}

void PowerMgr::finishReboot() {
  if (_snapshot.gps_powered && _port.setGpsPowered(false)) {
    _snapshot.gps_powered = false;
    notify(mask(PowerChange::Gps));
  }
  _port.powerOffRadio();
  _board.reboot();
  _snapshot.system_state = SystemPowerState::Fault;
  setFault(PowerFault::RebootReturned);
  notify(mask(PowerChange::System));
}

void PowerMgr::processSystemTransition(uint32_t now_ms) {
  if (_snapshot.system_state == SystemPowerState::ShuttingDown) {
    if (_port.shutdownFeedbackDone() ||
        deadlineReached(now_ms, _system_transition_due_ms)) {
      finishPowerOff();
    }
  } else if (_snapshot.system_state == SystemPowerState::Rebooting &&
             deadlineReached(now_ms, _system_transition_due_ms)) {
    finishReboot();
  }
}

void PowerMgr::loop(uint32_t now_ms) {
  if (!_begun) return;
  pollBattery(now_ms);
  pollPowerSource(now_ms);
  processDisplayTimeout(now_ms);
  (void)reconcileGpsPower(now_ms);
  processSystemTransition(now_ms);
}

}  // namespace heltec::meshcore::power
