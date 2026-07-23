/**
 * ST7789 240x320 (Heltec LoRa V4 TFT) for LVGL 8.x.
 * Draw buffer in PSRAM (full double-buffer preferred); SPI via internal SRAM chunk.
 */
#include "st7789_port.hpp"

#include <Arduino.h>
#include <SPI.h>
#include <lvgl.h>

#if !defined(SPI_INTERFACES_COUNT) || (SPI_INTERFACES_COUNT < 2) || !defined(PIN_SPI1_SCK) || \
    (PIN_SPI1_SCK < 0)
#error "ST7789 port requires SPI1 (SPI_INTERFACES_COUNT>=2 and PIN_SPI1_SCK)"
#endif
#include "target.h"
#include "spi1_bus_lock.hpp"

#if defined(ESP_PLATFORM)
#include <esp_heap_caps.h>
#endif

#ifndef PIN_TFT_CS
#error "PIN_TFT_CS must be defined for ST7789 port"
#endif
#ifndef PIN_TFT_DC
#error "PIN_TFT_DC must be defined for ST7789 port"
#endif

#ifndef HELTEC_TFT_HOR_RES
#define HELTEC_TFT_HOR_RES 240
#endif
#ifndef HELTEC_TFT_VER_RES
#define HELTEC_TFT_VER_RES 320
#endif
#ifndef DISPLAY_ROTATION
#define DISPLAY_ROTATION 0
#endif

namespace heltec::meshcore::dal::st7789 {
namespace {

static constexpr uint32_t kTftSpiHz = 40000000;
static constexpr int kScrWidth = HELTEC_TFT_HOR_RES;
static constexpr int kScrHeight = HELTEC_TFT_VER_RES;
#ifndef ST7789_DRAW_PART_LINES
#define ST7789_DRAW_PART_LINES 160
#endif
#ifndef ST7789_FALLBACK_LINES
#if defined(ESP_PLATFORM)
#define ST7789_FALLBACK_LINES 40
#else
#define ST7789_FALLBACK_LINES 8
#endif
#endif
#ifndef ST7789_SPI_CHUNK_BYTES
#if defined(ESP_PLATFORM)
#define ST7789_SPI_CHUNK_BYTES 8192
#else
#define ST7789_SPI_CHUNK_BYTES 1024
#endif
#endif
static constexpr int kFallbackLines = ST7789_FALLBACK_LINES;
static constexpr size_t kSpiChunkBytes = ST7789_SPI_CHUNK_BYTES;

static bool s_hw_inited = false;
static lv_color_t* s_buf1 = nullptr;
static lv_color_t* s_buf2 = nullptr;
static bool s_buf1_psram = false;
static bool s_buf2_psram = false;
LV_ATTRIBUTE_MEM_ALIGN static lv_color_t s_fallback_buf[(uint32_t)kScrWidth * (uint32_t)kFallbackLines];
static uint8_t s_spi_chunk[kSpiChunkBytes];
static lv_disp_draw_buf_t s_draw_buf;
static lv_disp_drv_t s_disp_drv;

constexpr uint8_t CMD_SOFT_RESET = 0x01;
constexpr uint8_t CMD_SLEEP_IN = 0x10;
constexpr uint8_t CMD_SLEEP_OUT = 0x11;
constexpr uint8_t CMD_NORMAL = 0x13;
constexpr uint8_t CMD_INVERT_OFF = 0x20;
constexpr uint8_t CMD_INVERT_ON = 0x21;
constexpr uint8_t CMD_GAMMA = 0x26;
constexpr uint8_t CMD_DISP_ON = 0x29;
constexpr uint8_t CMD_CASET = 0x2A;
constexpr uint8_t CMD_RASET = 0x2B;
constexpr uint8_t CMD_RAMWR = 0x2C;
constexpr uint8_t CMD_MADCTL = 0x36;
constexpr uint8_t CMD_COLMOD = 0x3A;
constexpr uint8_t CMD_DELAY = 0xFF;
constexpr uint8_t CMD_EOF = 0xFF;

constexpr uint8_t MADCTL_MX = 0x40;
constexpr uint8_t MADCTL_MY = 0x80;
constexpr uint8_t MADCTL_MV = 0x20;
#if defined(ST7789_LVGL_MADCTL_BGR)
constexpr uint8_t kMadColor = 0x08;
#else
constexpr uint8_t kMadColor = 0x00;
#endif
constexpr uint8_t COLMOD_RGB565 = 0x55;

constexpr uint8_t CMD_RAMCTRL = 0xB0;
constexpr uint8_t CMD_GCTRL = 0xB7;
constexpr uint8_t CMD_VCOMS = 0xBB;
constexpr uint8_t CMD_VRHS = 0xC3;
constexpr uint8_t CMD_PWCTRL1 = 0xD0;
constexpr uint8_t CMD_PVGAMCTRL = 0xE0;
constexpr uint8_t CMD_NVGAMCTRL = 0xE1;

static const uint8_t kInitCmdList[] = {
    CMD_GCTRL, 1, 0x44, CMD_VCOMS, 1, 0x24, CMD_VRHS, 1, 0x13, CMD_PWCTRL1, 2, 0xA4, 0xA1,
    CMD_RAMCTRL, 2, 0x00, 0xC0, CMD_PVGAMCTRL, 14, 0xD0, 0x00, 0x02, 0x07, 0x0A, 0x28, 0x32, 0x44,
    0x42, 0x06, 0x0E, 0x12, 0x14, 0x17, CMD_NVGAMCTRL, 14, 0xD0, 0x00, 0x02, 0x07, 0x0A, 0x28, 0x31,
    0x54, 0x47, 0x0E, 0x1C, 0x17, 0x1B, 0x1E, CMD_GAMMA, 1, 0x01, CMD_DELAY, CMD_EOF};

#ifndef PIN_TFT_LEDA_CTL_ACTIVE
#define HELTEC_TFT_LEDA_ON_LEVEL HIGH
#else
#define HELTEC_TFT_LEDA_ON_LEVEL PIN_TFT_LEDA_CTL_ACTIVE
#endif

#ifndef PIN_TFT_VDD_CTL_ACTIVE
#define HELTEC_TFT_VDD_ON_LEVEL HIGH
#else
#define HELTEC_TFT_VDD_ON_LEVEL PIN_TFT_VDD_CTL_ACTIVE
#endif

#ifndef HELTEC_TFT_X_GAP
#define HELTEC_TFT_X_GAP 0
#endif
#ifndef HELTEC_TFT_Y_GAP
#define HELTEC_TFT_Y_GAP 0
#endif

static uint8_t madctlForRotation(uint8_t rot) {
  uint8_t mad = kMadColor;
  switch (rot & 3) {
    case 0:
      mad |= MADCTL_MX | MADCTL_MY;
      break;
    case 1:
      mad |= MADCTL_MY | MADCTL_MV;
      break;
    case 2:
      break;
    case 3:
    default:
      mad |= MADCTL_MX | MADCTL_MV;
      break;
  }
  return mad;
}

static void freeDrawBufs() {
#if defined(ESP_PLATFORM)
  if (s_buf1_psram && s_buf1) heap_caps_free(s_buf1);
  if (s_buf2_psram && s_buf2) heap_caps_free(s_buf2);
#endif
  s_buf1 = nullptr;
  s_buf2 = nullptr;
  s_buf1_psram = false;
  s_buf2_psram = false;
}

#if defined(ESP_PLATFORM)
static lv_color_t* allocPsram(size_t bytes) {
  return (lv_color_t*)heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
}
#endif

static bool initDrawBuf() {
  freeDrawBufs();
  const uint32_t full_px = (uint32_t)kScrWidth * (uint32_t)kScrHeight;

#if defined(ESP_PLATFORM)
  const size_t full_bytes = full_px * sizeof(lv_color_t);
  s_buf1 = allocPsram(full_bytes);
  if (s_buf1) {
    s_buf1_psram = true;
    s_buf2 = allocPsram(full_bytes);
    if (s_buf2) {
      s_buf2_psram = true;
      lv_disp_draw_buf_init(&s_draw_buf, s_buf1, s_buf2, full_px);
      return true;
    }
    freeDrawBufs();
  }

  const uint32_t part_lines = (uint32_t)ST7789_DRAW_PART_LINES;
  const uint32_t part_px = (uint32_t)kScrWidth * part_lines;
  s_buf1 = allocPsram(part_px * sizeof(lv_color_t));
  if (s_buf1) {
    s_buf1_psram = true;
    lv_disp_draw_buf_init(&s_draw_buf, s_buf1, nullptr, part_px);
    return true;
  }
  freeDrawBufs();
#endif

  const uint32_t fb_px = (uint32_t)kScrWidth * (uint32_t)kFallbackLines;
  lv_disp_draw_buf_init(&s_draw_buf, s_fallback_buf, nullptr, fb_px);
  return true;
}

class St7789Panel {
 public:
  St7789Panel(SPIClass& spi, int8_t cs, int8_t dc, int8_t rst)
      : _spi(&spi),
        _cs(cs),
        _dc(dc),
        _rst(rst),
        _spi_hz(kTftSpiHz),
        _x_gap(HELTEC_TFT_X_GAP),
        _y_gap(HELTEC_TFT_Y_GAP) {}

  void boardPowerOn() {
    if(_rst >= 0) pinMode(_rst, OUTPUT);
    pinMode(_cs, OUTPUT);
    pinMode(_dc, OUTPUT);
    digitalWrite(_cs, HIGH);
    digitalWrite(_rst, HIGH);
#if defined(PIN_TFT_VDD_CTL)
    if (PIN_TFT_VDD_CTL >= 0) {
      pinMode(PIN_TFT_VDD_CTL, OUTPUT);
      digitalWrite(PIN_TFT_VDD_CTL, HELTEC_TFT_VDD_ON_LEVEL);
    }
#endif
  }

  void hardReset() {
    digitalWrite(_rst, LOW);
    delay(10);
    digitalWrite(_rst, HIGH);
    delay(120);
  }

  void initPanel(uint8_t rotation) {
    sendCmd(CMD_SLEEP_IN, nullptr, 0);
    delay(10);
    sendCmd(CMD_SOFT_RESET, nullptr, 0);
    delay(200);
    sendCmd(CMD_SLEEP_OUT, nullptr, 0);
    delay(300);
    sendCmd(CMD_NORMAL, nullptr, 0);

    const uint8_t mad = madctlForRotation(rotation);
    sendCmd(CMD_MADCTL, &mad, 1);
    const uint8_t colmod = COLMOD_RGB565;
    sendCmd(CMD_COLMOD, &colmod, 1);
    runCmdList(kInitCmdList);
#if defined(ST7789_LVGL_INVERT) && ST7789_LVGL_INVERT
    sendCmd(CMD_INVERT_ON, nullptr, 0);
#else
    sendCmd(CMD_INVERT_OFF, nullptr, 0);
#endif
    sendCmd(CMD_DISP_ON, nullptr, 0);
  }

  void flushRgb565(const lv_area_t* area, const lv_color_t* color_p) {
    int32_t x0 = area->x1 + _x_gap;
    int32_t x1 = area->x2 + _x_gap;
    int32_t y0 = area->y1 + _y_gap;
    int32_t y1 = area->y2 + _y_gap;
    uint8_t caset[4] = {(uint8_t)((x0 >> 8) & 0xFF), (uint8_t)(x0 & 0xFF), (uint8_t)((x1 >> 8) & 0xFF),
                        (uint8_t)(x1 & 0xFF)};
    uint8_t raset[4] = {(uint8_t)((y0 >> 8) & 0xFF), (uint8_t)(y0 & 0xFF), (uint8_t)((y1 >> 8) & 0xFF),
                        (uint8_t)(y1 & 0xFF)};
    sendCmd(CMD_CASET, caset, 4);
    sendCmd(CMD_RASET, raset, 4);
    const size_t len = (size_t)(x1 - x0 + 1) * (size_t)(y1 - y0 + 1) * 2;
    sendPixels(reinterpret_cast<const uint8_t*>(color_p), len);
  }

 private:
  void sendCmd(uint8_t cmd, const uint8_t* param, size_t param_len) {
    spi1::prepareDisplayBus(*_spi);
    const SPISettings settings(_spi_hz, MSBFIRST, SPI_MODE0);
    digitalWrite(_dc, LOW);
    digitalWrite(_cs, LOW);
    _spi->beginTransaction(settings);
    _spi->transfer(cmd);
    _spi->endTransaction();
    digitalWrite(_cs, HIGH);
    if (param_len == 0 || !param) return;
    digitalWrite(_dc, HIGH);
    digitalWrite(_cs, LOW);
    _spi->beginTransaction(settings);
    for (size_t i = 0; i < param_len; i++) {
      _spi->transfer(param[i]);
    }
    _spi->endTransaction();
    digitalWrite(_cs, HIGH);
  }

  void sendPixels(const uint8_t* px, size_t num_bytes) {
    spi1::prepareDisplayBus(*_spi);
    const SPISettings settings(_spi_hz, MSBFIRST, SPI_MODE0);
    digitalWrite(_dc, LOW);
    digitalWrite(_cs, LOW);
    _spi->beginTransaction(settings);
    _spi->transfer(CMD_RAMWR);
    _spi->endTransaction();
    digitalWrite(_dc, HIGH);
    digitalWrite(_cs, LOW);
    _spi->beginTransaction(settings);
#if !defined(LV_COLOR_16_SWAP) || (LV_COLOR_16_SWAP == 0)
#error "ST7789 LVGL port expects LV_COLOR_16_SWAP=1"
#endif
    size_t off = 0;
    while (off < num_bytes) {
      size_t n = num_bytes - off;
      if (n > kSpiChunkBytes) n = kSpiChunkBytes;
      memcpy(s_spi_chunk, px + off, n);
#if defined(ESP_PLATFORM)
      _spi->transferBytes(s_spi_chunk, nullptr, n);
#else
      _spi->transfer(s_spi_chunk, nullptr, n);
#endif
      off += n;
    }
    _spi->endTransaction();
    digitalWrite(_cs, HIGH);
  }

  void runCmdList(const uint8_t* cmd_list) {
    while (true) {
      const uint8_t cmd = *cmd_list++;
      const uint8_t num = *cmd_list++;
      if (cmd == CMD_DELAY) {
        if (num == CMD_EOF) break;
        delay((uint32_t)num * 10);
      } else {
        sendCmd(cmd, cmd_list, num);
        cmd_list += num;
      }
    }
  }

  SPIClass* _spi;
  int8_t _cs;
  int8_t _dc;
  int8_t _rst;
  uint32_t _spi_hz;
  uint16_t _x_gap;
  uint16_t _y_gap;
};

static St7789Panel s_panel(SPI1, PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_RST);
static bool s_backlight_on = true;

static void apply_backlight(bool on) {
#if defined(PIN_TFT_LEDA_CTL)
  if (PIN_TFT_LEDA_CTL >= 0) {
    pinMode(PIN_TFT_LEDA_CTL, OUTPUT);
    const uint8_t level =
        on ? HELTEC_TFT_LEDA_ON_LEVEL
           : (uint8_t)(HELTEC_TFT_LEDA_ON_LEVEL == HIGH ? LOW : HIGH);
    digitalWrite(PIN_TFT_LEDA_CTL, level);
  }
#else
  (void)on;
#endif
  s_backlight_on = on;
}

static void hw_begin() {
  if (s_hw_inited) return;
  s_panel.boardPowerOn();
  delay(20);
#if defined(ARDUINO_ARCH_NRF52)
  SPI1.setPins(-1, PIN_TFT_SCL, PIN_TFT_SDA);
  SPI1.begin();
#elif defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_ARCH_STM32)
  SPI1.begin(PIN_TFT_SCL, -1, PIN_TFT_SDA, PIN_TFT_CS);
#else
  SPI1.begin();
#endif
  SPI1.setBitOrder(MSBFIRST);
  SPI1.setDataMode(SPI_MODE0);
#if defined(ESP_PLATFORM)
  SPI1.setFrequency(kTftSpiHz);
#endif
  s_panel.hardReset();
  s_panel.initPanel((uint8_t)(DISPLAY_ROTATION & 3));
  apply_backlight(false);
  s_hw_inited = true;
#if defined(MESH_DEBUG) && MESH_DEBUG
  Serial.printf("[spi] 3c ST7789 panel init done t=%lu\n", (unsigned long)millis());
#endif
}

}  // namespace

bool init() {
  hw_begin();
  if (!initDrawBuf()) return false;

  lv_disp_drv_init(&s_disp_drv);
  s_disp_drv.hor_res = kScrWidth;
  s_disp_drv.ver_res = kScrHeight;
  s_disp_drv.flush_cb = [](lv_disp_drv_t* drv, const lv_area_t* area, lv_color_t* color_p) {
    s_panel.flushRgb565(area, color_p);
    lv_disp_flush_ready(drv);
  };
  s_disp_drv.draw_buf = &s_draw_buf;

  lv_disp_t* disp = lv_disp_drv_register(&s_disp_drv);
  return disp != nullptr;
}

void deinit() {
  lv_disp_t* d = lv_disp_get_default();
  if (d) lv_disp_remove(d);
  freeDrawBufs();
}

void setBacklightOn(bool on) {
  if (!s_hw_inited) return;
  apply_backlight(on);
}

bool isBacklightOn() { return s_backlight_on; }

void reinitPanelAfterSharedReset() {
  if (!s_hw_inited) return;
  s_panel.hardReset();
  s_panel.initPanel((uint8_t)(DISPLAY_ROTATION & 3));
}

}  // namespace heltec::meshcore::dal::st7789
