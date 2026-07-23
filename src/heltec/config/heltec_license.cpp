#include "config/heltec_license.h"

#include <cstring>

#if defined(NRF52_PLATFORM)
#include <nrf.h>
#elif defined(ESP32) || defined(ESP_PLATFORM)
#if defined(CONFIG_IDF_TARGET_ESP32C6) && defined(CONFIG_SOC_IEEE802154_SUPPORTED)
#include <esp_mac.h>
#endif
#endif

#if defined(HELTEC_MESH_UI) && HELTEC_MESH_UI && HELTEC_LICENSE_BOOT_GATE
#include <lvgl.h>
#include "heltec/drivers/display/display_port.hpp"
#include "heltec/drivers/input/momentary_button.hpp"
#endif

#if defined(MESH_DEBUG) && MESH_DEBUG
#define LICENSE_VERBOSE(...) Serial.printf("[license] " __VA_ARGS__)
#define LICENSE_VERBOSELN(msg) Serial.println("[license] " msg)
#else
#define LICENSE_VERBOSE(...) ((void)0)
#define LICENSE_VERBOSELN(msg) ((void)0)
#endif

#define LICENSE_LOG(...) Serial.printf("[license] " __VA_ARGS__)
#define LICENSE_LOGLN(msg) Serial.println("[license] " msg)

#if defined(NRF52_PLATFORM) || defined(ESP32) || defined(ESP_PLATFORM)

static FILESYSTEM* s_license_fs = nullptr;

static constexpr const char kLicenseFilePath[] = "/heltec_lic";

void heltecLicenseSetFilesystem(FILESYSTEM* fs) { s_license_fs = fs; }

static bool licenseWordsNonEmpty(const uint32_t license[HT_LICENSE_WORDS]) {
  for (uint32_t i = 0; i < HT_LICENSE_WORDS; ++i) {
    if (license[i] != 0xFFFFFFFFU) return true;
  }
  return false;
}

static bool licenseReadWords(uint32_t license[HT_LICENSE_WORDS]) {
  if (!license || !s_license_fs) return false;
  if (!s_license_fs->exists(kLicenseFilePath)) return false;

#if defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
  File f = s_license_fs->open(kLicenseFilePath);
#else
  File f = s_license_fs->open(kLicenseFilePath, "r");
#endif
  if (!f) return false;

  const int n = f.read((uint8_t*)license, (int)(HT_LICENSE_WORDS * sizeof(uint32_t)));
  f.close();
  if (n != (int)(HT_LICENSE_WORDS * sizeof(uint32_t))) return false;
  return licenseWordsNonEmpty(license);
}

static bool licenseWriteWords(const uint32_t license[HT_LICENSE_WORDS]) {
  if (!license || !s_license_fs) return false;

  s_license_fs->remove(kLicenseFilePath);
#if defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
  File f = s_license_fs->open(kLicenseFilePath, FILE_O_WRITE);
#else
  File f = s_license_fs->open(kLicenseFilePath, "w", true);
#endif
  if (!f) return false;

  const size_t n = f.write((const uint8_t*)license, HT_LICENSE_WORDS * sizeof(uint32_t));
  f.close();
  return n == HT_LICENSE_WORDS * sizeof(uint32_t);
}

bool heltecLicenseClearStored() {
  if (!s_license_fs) return false;
  if (!s_license_fs->exists(kLicenseFilePath)) return false;
  return s_license_fs->remove(kLicenseFilePath);
}

#endif

static uint8_t hexNibble(char c) {
  if (c >= '0' && c <= '9') return (uint8_t)(c - '0');
  if (c >= 'a' && c <= 'f') return (uint8_t)(c - 'a' + 10);
  if (c >= 'A' && c <= 'F') return (uint8_t)(c - 'A' + 10);
  return 0xFF;
}

static bool parseHexU32(const char* hex8, uint32_t& out) {
  if (!hex8) return false;
  uint32_t v = 0;
  for (int i = 0; i < 8; ++i) {
    const uint8_t n = hexNibble(hex8[i]);
    if (n > 0x0F) return false;
    v = (v << 4) | n;
  }
  out = v;
  return true;
}

/** Collect 32 hex digits after '=' (or from start if no '='); ignores non-hex. */
static bool extractCdkeyHex32(const char* ascii, char hex32[33]) {
  if (!ascii || !hex32) return false;

  const char* p = ascii;
  while (*p == ' ' || *p == '\t') ++p;

  const char* eq = strchr(p, '=');
  if (eq) {
    p = eq + 1;
  }
  while (*p == ' ' || *p == '\t') ++p;

  size_t n = 0;
  for (; *p != '\0' && n < 32; ++p) {
    const char ch = *p;
    if (ch == '\r' || ch == '\n') continue;
    if (hexNibble(ch) > 0x0F) continue;
    hex32[n++] = ch;
  }
  hex32[n] = '\0';
  return n == 32;
}

uint64_t heltecLicenseGetChipId64() {
#if defined(NRF52_PLATFORM)
  const uint64_t id =
      ((uint64_t)NRF_FICR->DEVICEID[1] << 32) | (uint64_t)NRF_FICR->DEVICEID[0];
  return id & 0x0000FFFFFFFFFFFFULL;
#elif defined(ESP32) || defined(ESP_PLATFORM)
#if defined(CONFIG_IDF_TARGET_ESP32C6) && defined(CONFIG_SOC_IEEE802154_SUPPORTED)
  uint8_t mac[6] = {0};
  esp_base_mac_addr_get(mac);
  return ((uint64_t)mac[0] << 40) | ((uint64_t)mac[1] << 32) | ((uint64_t)mac[2] << 24) |
         ((uint64_t)mac[3] << 16) | ((uint64_t)mac[4] << 8) | ((uint64_t)mac[5]);
#else
  return (uint64_t)ESP.getEfuseMac();
#endif
#else
  return 0;
#endif
}

void heltecLicenseFormatChipIdHex(char out13[13]) {
  if (!out13) return;
  uint64_t id = heltecLicenseGetChipId64();
  static const char kHex[] = "0123456789ABCDEF";
  for (int i = 11; i >= 0; --i) {
    out13[i] = kHex[id & 0xFULL];
    id >>= 4;
  }
  out13[12] = '\0';
}

void heltecLicenseFormatSerialLine(char* out, size_t out_len) {
  if (!out || out_len == 0) return;
  char chip[13];
  heltecLicenseFormatChipIdHex(chip);
#if defined(NRF52_PLATFORM)
  snprintf(out, out_len, "NRFChipID=%s", chip);
#elif defined(ESP32) || defined(ESP_PLATFORM)
  snprintf(out, out_len, "ESP32ChipID=%s", chip);
#else
  snprintf(out, out_len, "ChipID=%s", chip);
#endif
}

int heltecLicenseCalRtc(const uint32_t license[HT_LICENSE_WORDS]) {
  if (!license) return 0;

  uint32_t correctlicense[HT_LICENSE_WORDS];
  uint64_t temp0 = heltecLicenseGetChipId64();
  temp0 = temp0 + ((temp0 >> 16) & 0x00000000FFFF0000ULL) - ((temp0 >> 32) & 0x000000000000FFFFULL);
  uint32_t temp = (uint32_t)(temp0 >> 16);
  temp += 0x7F94C959U;
  temp = ((temp >> 4) & 0x0F0F0F0FU) | ((temp << 4) & 0xF0F0F0F0U);
  temp -= 0xF5B1C7B2U;
  temp = ((temp >> 2) & 0x33333333U) | ((temp << 2) & 0xCCCCCCCCU);
  temp += 0x572384DCU;
  temp = ((temp >> 1) & 0x55555555U) | ((temp << 1) & 0xAAAAAAAAU);
  temp -= 0x572384DCU;
  temp = ((temp >> 8) & 0x00FF00FFU) | ((temp << 8) & 0xFF00FF00U);
  correctlicense[0] = temp;

  temp = (uint32_t)(temp0 >> 16);
  temp = (temp & 0xFFF0FFFFU) | ((uint32_t)(temp0 << 4) & 0x000F0000U);
  temp += 0x394BBD41U;
  temp = ((temp >> 8) & 0x00FF00FFU) | ((temp << 8) & 0xFF00FF00U);
  temp -= 0xC05309BAU;
  temp = ((temp >> 2) & 0x33333333U) | ((~temp << 2) & 0xCCCCCCCCU);
  temp += 0x414DE9DAU;
  temp = ((~temp >> 4) & 0x0F0F0F0FU) | ((temp << 4) & 0xF0F0F0F0U);
  temp -= 0x818666BEU;
  temp = ((temp >> 1) & 0x55555555U) | ((temp << 1) & 0xAAAAAAAAU);
  correctlicense[1] = temp;

  temp = (uint32_t)(temp0 >> 16);
  temp = (temp & 0xF0FFFF0FU) | ((uint32_t)(temp0 << 16) & 0x0F000000U) | ((uint32_t)(temp0)&0x000000F0U);
  temp += 0xB2F6B12DU;
  temp = ((~temp >> 8) & 0x00FF00FFU) | ((temp << 8) & 0xFF00FF00U);
  temp -= 0xE4123C4EU;
  temp = ((temp >> 4) & 0x0F0F0F0FU) | ((~temp << 4) & 0xF0F0F0F0U);
  temp += 0xF5941F73U;
  temp = ((~temp >> 2) & 0x33333333U) | ((~temp << 2) & 0xCCCCCCCCU);
  temp -= 0x8C165125U;
  temp = ((temp >> 1) & 0x55555555U) | ((temp << 1) & 0xAAAAAAAAU);
  correctlicense[2] = temp;

  temp = (uint32_t)(temp0 >> 16);
  temp = (temp & 0xFFFFF0FFU) | ((uint32_t)(temp0 << 8) & 0x00000F00U);
  temp = ((~temp >> 1) & 0x55555555U) | ((temp << 1) & 0xAAAAAAAAU);
  temp += 0x237CE438U;
  temp = ((temp >> 4) & 0x0F0F0F0FU) | ((~temp << 4) & 0xF0F0F0F0U);
  temp -= 0x263821C7U;
  temp = ((~temp >> 8) & 0x00FF00FFU) | ((temp << 8) & 0xFF00FF00U);
  temp += 0x41EC0F9AU;
  temp = ((~temp >> 2) & 0x33333333U) | ((~temp << 2) & 0xCCCCCCCCU);
  temp -= 0xD7393F00U;
  correctlicense[3] = temp;

  return (license[0] == correctlicense[0] && license[1] == correctlicense[1] &&
          license[2] == correctlicense[2] && license[3] == correctlicense[3])
             ? 1
             : 0;
}

bool heltecLicenseReadStored(uint32_t license[HT_LICENSE_WORDS]) {
  if (!license) return false;

#if defined(NRF52_PLATFORM) || defined(ESP32) || defined(ESP_PLATFORM)
  const bool ok = licenseReadWords(license);
  if (ok) {
    LICENSE_VERBOSE("file read %s: %08lX %08lX %08lX %08lX\n", kLicenseFilePath, (unsigned long)license[0],
                    (unsigned long)license[1], (unsigned long)license[2], (unsigned long)license[3]);
  }
  return ok;
#else
  memset(license, 0, HT_LICENSE_WORDS * sizeof(uint32_t));
  return false;
#endif
}

bool heltecLicenseWriteStored(const uint32_t license[HT_LICENSE_WORDS]) {
  if (!license) return false;

#if defined(NRF52_PLATFORM) || defined(ESP32) || defined(ESP_PLATFORM)
  if (!s_license_fs) {
    LICENSE_LOGLN("file write: filesystem not ready");
    return false;
  }
  if (!licenseWriteWords(license)) {
    LICENSE_LOGLN("file write: failed");
    return false;
  }
  LICENSE_LOG("file write OK %s\n", kLicenseFilePath);
  return true;
#else
  (void)license;
  LICENSE_LOGLN("file write: unsupported platform");
  return false;
#endif
}

bool heltecLicenseIsAuthorized() {
  uint32_t license[HT_LICENSE_WORDS];
  if (!heltecLicenseReadStored(license)) {
    LICENSE_LOGLN("check: no stored license");
    return false;
  }
  const int ok = heltecLicenseCalRtc(license);
  if (ok) {
    LICENSE_LOGLN("check: stored license valid");
  } else {
    LICENSE_LOGLN("check: stored license invalid (chip mismatch)");
  }
  return ok != 0;
}

bool heltecLicenseParseCdkeyHex(const char* hex32, uint32_t license[HT_LICENSE_WORDS]) {
  if (!hex32 || !license) return false;
  for (int w = 0; w < (int)HT_LICENSE_WORDS; ++w) {
    if (!parseHexU32(hex32 + w * 8, license[w])) return false;
  }
  return true;
}

bool heltecLicenseTryCdkeyCommand(const char* line) {
  if (!line) {
    LICENSE_LOGLN("cdkey: empty input");
    return false;
  }

  char hex32[33] = {0};
  if (!extractCdkeyHex32(line, hex32)) {
    const char* eq = strchr(line, '=');
    LICENSE_LOG("cdkey: need 32 hex digits after '=' (input len=%u, payload len=%u)\n",
                (unsigned)strlen(line), eq ? (unsigned)strlen(eq + 1) : 0u);
    return false;
  }

  LICENSE_LOG("cdkey: extracted hex32=%s\n", hex32);

  uint32_t license[HT_LICENSE_WORDS];
  if (!heltecLicenseParseCdkeyHex(hex32, license)) {
    LICENSE_LOGLN("cdkey: hex parse to words failed");
    return false;
  }

  LICENSE_VERBOSE("cdkey: words %08lX %08lX %08lX %08lX\n", (unsigned long)license[0],
                  (unsigned long)license[1], (unsigned long)license[2], (unsigned long)license[3]);

  if (!heltecLicenseCalRtc(license)) {
    LICENSE_LOGLN("cdkey: validation failed (chip mismatch)");
    return false;
  }

  if (!heltecLicenseWriteStored(license)) {
    LICENSE_LOGLN("cdkey: file store failed");
    return false;
  }

  LICENSE_LOGLN("cdkey: authorized and stored");
  return true;
}

#if defined(HELTEC_MESH_UI) && HELTEC_MESH_UI && HELTEC_LICENSE_BOOT_GATE

#ifndef HELTEC_LICENSE_GATE_AUTO_OFF_MS
#define HELTEC_LICENSE_GATE_AUTO_OFF_MS 30000U
#endif

#ifndef HELTEC_LICENSE_GATE_RX_GAP_MS
#define HELTEC_LICENSE_GATE_RX_GAP_MS 5U
#endif

static lv_obj_t* s_gate_root = nullptr;
static lv_obj_t* s_gate_title = nullptr;
static lv_obj_t* s_gate_chip = nullptr;
static uint32_t s_gate_display_last_activity_ms = 0;
static lv_style_t s_gate_root_style;
static lv_style_t s_gate_label_style;
static lv_style_t s_gate_chip_style;
static bool s_gate_styles_ready = false;

static void heltecLicenseGateInitStyles() {
  if (s_gate_styles_ready) return;

  lv_style_init(&s_gate_root_style);
  lv_style_set_width(&s_gate_root_style, lv_pct(100));
  lv_style_set_height(&s_gate_root_style, lv_pct(100));
  lv_style_set_radius(&s_gate_root_style, 0);
#if defined(HELTEC_V4_R8_TFT)
  lv_style_set_pad_all(&s_gate_root_style, 0);
  lv_style_set_border_width(&s_gate_root_style, 0);
  lv_style_set_outline_width(&s_gate_root_style, 0);
#else
  lv_style_set_pad_all(&s_gate_root_style, 8);
#endif
  lv_style_set_bg_color(&s_gate_root_style, lv_color_black());
  lv_style_set_bg_opa(&s_gate_root_style, LV_OPA_COVER);
  lv_style_set_layout(&s_gate_root_style, LV_LAYOUT_FLEX);
  lv_style_set_flex_flow(&s_gate_root_style, LV_FLEX_FLOW_COLUMN);
  lv_style_set_flex_main_place(&s_gate_root_style, LV_FLEX_ALIGN_CENTER);
  lv_style_set_flex_cross_place(&s_gate_root_style, LV_FLEX_ALIGN_CENTER);
  lv_style_set_flex_track_place(&s_gate_root_style, LV_FLEX_ALIGN_CENTER);
#if defined(HELTEC_V4_R8_TFT)
  lv_style_set_pad_row(&s_gate_root_style, 0);
  lv_style_set_pad_column(&s_gate_root_style, 0);
#else
  lv_style_set_pad_row(&s_gate_root_style, 12);
#endif

  lv_style_init(&s_gate_label_style);
  lv_style_set_text_color(&s_gate_label_style, lv_color_white());
  lv_style_set_text_align(&s_gate_label_style, LV_TEXT_ALIGN_CENTER);
#if defined(HELTEC_V4_R8_TFT)
  lv_style_set_pad_all(&s_gate_label_style, 0);
  lv_style_set_border_width(&s_gate_label_style, 0);
  lv_style_set_outline_width(&s_gate_label_style, 0);
#endif

  lv_style_init(&s_gate_chip_style);
  lv_style_set_width(&s_gate_chip_style, lv_pct(100));
#if defined(HELTEC_V4_R8_TFT)
  lv_style_set_pad_all(&s_gate_chip_style, 0);
  lv_style_set_border_width(&s_gate_chip_style, 0);
  lv_style_set_outline_width(&s_gate_chip_style, 0);
#endif

  s_gate_styles_ready = true;
}

static void heltecLicenseGateNotifyDisplayActivity(uint32_t now) {
  s_gate_display_last_activity_ms = now;
  if (!heltec::meshcore::dal::display_port::isBacklightOn()) {
    heltec::meshcore::dal::display_port::setBacklightOn(true);
    if (lv_disp_t* d = lv_disp_get_default()) {
      lv_refr_now(d);
    }
  }
}

static void heltecLicenseGateTickDisplayAutoOff(uint32_t now) {
  if (!s_gate_display_last_activity_ms) return;
  if (!heltec::meshcore::dal::display_port::isBacklightOn()) return;
  if ((now - s_gate_display_last_activity_ms) >= HELTEC_LICENSE_GATE_AUTO_OFF_MS) {
    heltec::meshcore::dal::display_port::setBacklightOn(false);
  }
}

static void heltecLicenseGatePollInputWake(uint32_t now) {
  if (heltec::meshcore::dal::momentary_button::anyPhysicalPressed()) {
    heltecLicenseGateNotifyDisplayActivity(now);
  }
}

static void heltecLicenseShowUnauthorizedUi(const char* display_id) {
  if (s_gate_root) {
    if (s_gate_chip && display_id) lv_label_set_text(s_gate_chip, display_id);
    return;
  }

  s_gate_root = lv_obj_create(lv_layer_top());
  if (!s_gate_root) return;

  heltecLicenseGateInitStyles();
  lv_obj_add_style(s_gate_root, &s_gate_root_style, LV_PART_MAIN);
  lv_obj_clear_flag(s_gate_root, LV_OBJ_FLAG_SCROLLABLE);

  s_gate_title = lv_label_create(s_gate_root);
  if (s_gate_title) {
    lv_label_set_text(s_gate_title, "Not Authorized");
    lv_obj_add_style(s_gate_title, &s_gate_label_style, LV_PART_MAIN);
  }

  s_gate_chip = lv_label_create(s_gate_root);
  if (s_gate_chip) {
    lv_label_set_text(s_gate_chip, display_id ? display_id : "ID:");
    lv_obj_add_style(s_gate_chip, &s_gate_label_style, LV_PART_MAIN);
    lv_obj_add_style(s_gate_chip, &s_gate_chip_style, LV_PART_MAIN);
    lv_label_set_long_mode(s_gate_chip, LV_LABEL_LONG_WRAP);
  }

  heltecLicenseGateNotifyDisplayActivity(millis());
}

static void heltecLicenseHideUnauthorizedUi() {
  if (!s_gate_root) return;
  lv_obj_del(s_gate_root);
  s_gate_root = nullptr;
  s_gate_title = nullptr;
  s_gate_chip = nullptr;
}

void heltecLicenseUiTick() {
  lv_timer_handler();
  if (lv_disp_t* d = lv_disp_get_default()) {
    lv_refr_now(d);
  }
}

bool heltecLicenseBootGate(bool has_display) {
#if defined(NRF52_PLATFORM) || defined(ESP32) || defined(ESP_PLATFORM)
  if (!s_license_fs) {
    LICENSE_LOGLN("boot gate: filesystem not bound");
  }
#endif
  if (heltecLicenseIsAuthorized()) {
    LICENSE_LOGLN("boot gate: already authorized");
    return true;
  }

  char chip_hex[13] = {0};
  heltecLicenseFormatChipIdHex(chip_hex);
  char display_id[20] = {0};
  snprintf(display_id, sizeof(display_id), "ID:%s", chip_hex);
  char serial_line[40] = {0};
  heltecLicenseFormatSerialLine(serial_line, sizeof(serial_line));

  LICENSE_LOG("boot gate: waiting for CDKEY (chip=%s, ui=%d, gap=%ums)\n", chip_hex,
              has_display ? 1 : 0, (unsigned)HELTEC_LICENSE_GATE_RX_GAP_MS);

  if (has_display) {
    heltecLicenseShowUnauthorizedUi(display_id);
    heltecLicenseUiTick();
    LICENSE_LOGLN("boot gate: unauthorized UI shown");
  }

  Serial.println(serial_line);

  char rx_buf[96];
  size_t rx_len = 0;
  uint32_t last_serial_ms = millis();
  uint32_t last_rx_char_ms = 0;
  bool rx_active = false;

  for (;;) {
    while (Serial.available() > 0) {
      const int c = Serial.read();
      if (c < 0) break;
      const char ch = (char)c;
      const uint32_t char_ms = millis();
      if (has_display) {
        heltecLicenseGateNotifyDisplayActivity(char_ms);
      }
#if defined(MESH_DEBUG) && MESH_DEBUG
      Serial.printf("%c", ch);
#endif
      if (rx_len + 1 < sizeof(rx_buf)) {
        rx_buf[rx_len++] = ch;
        last_rx_char_ms = char_ms;
        rx_active = true;
      } else {
        LICENSE_LOGLN("boot gate: rx buffer full, dropping frame");
        rx_len = 0;
        rx_active = false;
        last_rx_char_ms = 0;
      }
    }

    if (rx_active && rx_len > 0 &&
        (uint32_t)(millis() - last_rx_char_ms) >= HELTEC_LICENSE_GATE_RX_GAP_MS) {
      rx_buf[rx_len] = '\0';
      if (heltecLicenseTryCdkeyCommand(rx_buf)) {
        LICENSE_LOGLN("boot gate: authorization OK, continuing boot");
        heltecLicenseHideUnauthorizedUi();
        return true;
      }
      LICENSE_LOGLN("boot gate: CDKEY rejected, waiting for next frame");
      rx_len = 0;
      rx_active = false;
    }

    const uint32_t now = millis();
    if ((uint32_t)(now - last_serial_ms) >= 5000U) {
      Serial.println(serial_line);
      last_serial_ms = now;
    }

    if (has_display) {
      heltecLicenseGatePollInputWake(now);
      heltecLicenseGateTickDisplayAutoOff(now);
      heltecLicenseUiTick();
    }
    delay(1);
  }
}

#elif defined(HELTEC_MESH_UI) && HELTEC_MESH_UI

bool heltecLicenseBootGate(bool) {
  return true;
}

void heltecLicenseUiTick() {}

#else

bool heltecLicenseBootGate(bool) {
  return heltecLicenseIsAuthorized();
}

void heltecLicenseUiTick() {}

#endif
