#include "mesh_app_ui.hpp"

#include "HeltecMesh.h"
#include "config/NodePrefs.h"

#include "target.h"

#include <Arduino.h>

#ifndef HELTEC_UI_RADIO_STATUS_POLL_MS
#define HELTEC_UI_RADIO_STATUS_POLL_MS 1000U
#endif

namespace heltec::meshcore::biz {

namespace {

bool radio_status_changed(const IBizFacade::RadioStatus& a, const IBizFacade::RadioStatus& b) {
  return a.freq_mhz != b.freq_mhz || a.bw_khz != b.bw_khz || a.cr != b.cr || a.sf != b.sf ||
         a.tx_power_dbm != b.tx_power_dbm || a.noise_floor_dbm != b.noise_floor_dbm ||
         a.rx_valid != b.rx_valid || a.last_rssi_dbm != b.last_rssi_dbm ||
         a.last_snr_db != b.last_snr_db || a.last_rx_at_ms != b.last_rx_at_ms ||
         a.forwarding_enabled != b.forwarding_enabled;
}

}  // namespace

void MeshAppUi::notifyRadioChanged() {
  _radio_status_cache = radioStatus();
  _radio_status_cache_valid = true;
  _radio_status_poll_ms = millis();
  notifyAppState(heltec::meshcore::ui::AppStateEventType::RadioChanged);
}

void MeshAppUi::pollRadioStatus() {
  const uint32_t now = millis();
  if (_radio_status_cache_valid &&
      (uint32_t)(now - _radio_status_poll_ms) < HELTEC_UI_RADIO_STATUS_POLL_MS) {
    return;
  }
  _radio_status_poll_ms = now;

  const RadioStatus cur = radioStatus();
  if (_radio_status_cache_valid && !radio_status_changed(cur, _radio_status_cache)) return;
  _radio_status_cache = cur;
  _radio_status_cache_valid = true;
  notifyAppState(heltec::meshcore::ui::AppStateEventType::RadioChanged);
}

void MeshAppUi::adjustTxPowerDbm(int delta_db) {
  NodePrefs* p = the_mesh.getNodePrefs();
  if (!p) return;
  int next = (int)p->tx_power_dbm + delta_db;
  if (next > MAX_LORA_TX_POWER) next = MAX_LORA_TX_POWER;
  if (next < -9) next = -9;
  p->tx_power_dbm = next;
  radio_set_tx_power(p->tx_power_dbm);
  the_mesh.savePrefs();
  notifyRadioChanged();
}

void MeshAppUi::adjustSpreadingFactor(int delta) {
  NodePrefs* p = the_mesh.getNodePrefs();
  if (!p) return;
  int next = (int)p->sf + delta;
  if (next > 12) next = 12;
  if (next < 7) next = 7;
  p->sf = next;
  the_mesh.savePrefs();
  notifyRadioChanged();
}

IBizFacade::RadioStatus MeshAppUi::radioStatus() const {
  RadioStatus s;
  NodePrefs* p = the_mesh.getNodePrefs();
  if (!p) return s;
  s.freq_mhz = p->freq;
  s.bw_khz = p->bw;
  s.cr = (int)p->cr;
  s.sf = (int)p->sf;
  s.tx_power_dbm = (int)p->tx_power_dbm;
  s.noise_floor_dbm = (int)radio_driver.getNoiseFloor();
  const HeltecMesh::LastRxMetrics rx = the_mesh.lastRxMetrics();
  s.rx_valid = rx.valid;
  s.last_rssi_dbm = rx.rssi_dbm;
  s.last_snr_db = rx.snr_db;
  s.last_rx_at_ms = rx.received_ms;
  s.forwarding_enabled = p->client_repeat != 0;
  return s;
}

}  // namespace heltec::meshcore::biz

