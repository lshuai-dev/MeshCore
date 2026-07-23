#include "st7735_port.hpp"
#include <Arduino.h>
#include <SPI.h>
#include <lvgl.h>
#ifndef PIN_TFT_CS
#error "PIN_TFT_CS must be defined for ST7735 port"
#endif

namespace heltec::meshcore::dal::st7735 {
namespace {

static constexpr uint32_t kTftSpiHz = 40000000;
static bool s_hw_inited = false;
static bool s_disp_registered = false;
static constexpr int kPartLines = 8;
static constexpr int kScrWidth = 160;
static constexpr int kScrHeight = 80;
#ifndef HELTEC_ST7735_EXTRA_CLEAR_RIGHT
#define HELTEC_ST7735_EXTRA_CLEAR_RIGHT 0
#endif
#ifndef HELTEC_ST7735_EXTRA_CLEAR_BOTTOM
#define HELTEC_ST7735_EXTRA_CLEAR_BOTTOM 0
#endif
static constexpr int kBootClearWidth = kScrWidth + HELTEC_ST7735_EXTRA_CLEAR_RIGHT;
static constexpr int kBootClearHeight = kScrHeight + HELTEC_ST7735_EXTRA_CLEAR_BOTTOM;
static lv_color_t s_boot_clear_row[kBootClearWidth];
#if defined(ARDUINO_ARCH_NRF52)
SPIClass& tft_spi = SPI1;
#else
SPIClass tft_spi;
#endif

#if defined(LV_COLOR_16_SWAP) && (LV_COLOR_16_SWAP != 0)
#define ST7735_LV_TO_RGB565(u16_) (uint16_t)((uint16_t)((u16_) << 8) | (uint16_t)((u16_) >> 8))
#else
#define ST7735_LV_TO_RGB565(u16_) (uint16_t)(u16_)
#endif

static const uint16_t s_buf_size_px = kScrWidth * kScrHeight / kPartLines;
LV_ATTRIBUTE_FAST_MEM LV_ATTRIBUTE_MEM_ALIGN static lv_color_t s_buf[s_buf_size_px * (LV_COLOR_DEPTH / 8)];
static lv_disp_draw_buf_t s_draw_buf;
static lv_disp_drv_t s_disp_drv;

// ---- ST77xx + ST7735R (Adafruit-compatible) ----
constexpr uint8_t kStCmdDelay = 0x80;

constexpr uint8_t ST77XX_SWRESET = 0x01;
constexpr uint8_t ST77XX_SLPOUT = 0x11;
constexpr uint8_t ST77XX_INVOFF = 0x20;
constexpr uint8_t ST77XX_INVON = 0x21;
constexpr uint8_t ST77XX_NORON = 0x13;
constexpr uint8_t ST77XX_DISPON = 0x29;
constexpr uint8_t ST77XX_MADCTL = 0x36;
constexpr uint8_t ST77XX_COLMOD = 0x3A;

constexpr uint8_t ST7735_FRMCTR1 = 0xB1;
constexpr uint8_t ST7735_FRMCTR2 = 0xB2;
constexpr uint8_t ST7735_FRMCTR3 = 0xB3;
constexpr uint8_t ST7735_INVCTR = 0xB4;
constexpr uint8_t ST7735_PWCTR1 = 0xC0;
constexpr uint8_t ST7735_PWCTR2 = 0xC1;
constexpr uint8_t ST7735_PWCTR3 = 0xC2;
constexpr uint8_t ST7735_PWCTR4 = 0xC3;
constexpr uint8_t ST7735_PWCTR5 = 0xC4;
constexpr uint8_t ST7735_VMCTR1 = 0xC5;
constexpr uint8_t ST7735_GMCTRP1 = 0xE0;
constexpr uint8_t ST7735_GMCTRN1 = 0xE1;

constexpr uint8_t ST77XX_MADCTL_MX = 0x40;
constexpr uint8_t ST77XX_MADCTL_MY = 0x80;
constexpr uint8_t ST77XX_MADCTL_MV = 0x20;

constexpr uint8_t LV_LCD_CMD_SET_COLUMN_ADDRESS = 0x2A;
constexpr uint8_t LV_LCD_CMD_SET_PAGE_ADDRESS = 0x2B;
constexpr uint8_t LV_LCD_CMD_WRITE_MEMORY_START = 0x2C;

constexpr uint16_t kColStart = 24;
constexpr uint16_t kRowStart = 0;
#ifndef HELTEC_ST7735_X_GAP_ADJUST
#define HELTEC_ST7735_X_GAP_ADJUST 0
#endif
#ifndef HELTEC_ST7735_Y_GAP_ADJUST
#define HELTEC_ST7735_Y_GAP_ADJUST 0
#endif
constexpr uint8_t kMadColor = 0x08;
static const uint8_t kRcmd1[] = {
    15,        ST77XX_SWRESET,        kStCmdDelay, 150, ST77XX_SLPOUT,         kStCmdDelay, 255,
    ST7735_FRMCTR1,       3,         0x01,          0x2C, 0x2D,
    ST7735_FRMCTR2,       3,         0x01,          0x2C, 0x2D,
    ST7735_FRMCTR3,       6,         0x01,          0x2C, 0x2D, 0x01, 0x2C, 0x2D,
    ST7735_INVCTR,        1,         0x07,
    ST7735_PWCTR1,       3,         0xA2,          0x02, 0x84,
    ST7735_PWCTR2,       1,         0xC5,
    ST7735_PWCTR3,       2,         0x0A,          0x00,
    ST7735_PWCTR4,       2,         0x8A,          0x2A,
    ST7735_PWCTR5,       2,         0x8A,          0xEE,
    ST7735_VMCTR1,       1,         0x0E,
#if defined(ST7735_LVGL_INVERT) && ST7735_LVGL_INVERT
    ST77XX_INVON,        0,
#else
    ST77XX_INVOFF,       0,
#endif
    /* 0xC8 = MX|MY|BGR (Adafruit INITR_MINI160x80); applyRotation() reapplies kMadColor. */
    ST77XX_MADCTL,       1,         0xC8,
    ST77XX_COLMOD,       1,         0x05,
};

static const uint8_t kRcmd2Mini160x80[] = {
    2,
    LV_LCD_CMD_SET_COLUMN_ADDRESS,
    4,
    0x00,
    0x00,
    0x00,
    0x4F,
    LV_LCD_CMD_SET_PAGE_ADDRESS,
    4,
    0x00,
    0x00,
    0x00,
    0x9F,
};

static const uint8_t kRcmd3[] = {
    4,
    ST7735_GMCTRP1,
    16,
    0x02,
    0x1c,
    0x07,
    0x12,
    0x37,
    0x32,
    0x29,
    0x2d,
    0x29,
    0x25,
    0x2B,
    0x39,
    0x00,
    0x01,
    0x03,
    0x10,
    ST7735_GMCTRN1,
    16,
    0x03,
    0x1d,
    0x07,
    0x06,
    0x2E,
    0x2C,
    0x29,
    0x2d,
    0x2E,
    0x2E,
    0x37,
    0x3F,
    0x00,
    0x00,
    0x02,
    0x10,
    ST77XX_NORON,
    kStCmdDelay,
    10,
    ST77XX_DISPON,
    kStCmdDelay,
    100,
};

class MiniMipiSt7735 {
 public:
  MiniMipiSt7735(SPIClass& spi, int8_t cs, int8_t dc, int8_t rst)
      : _spi(&spi), _cs(cs), _dc(dc), _rst(rst), _spi_hz(kTftSpiHz), _x_gap(0), _y_gap(0), _madctl(0) {}

  void boardPowerOn() {
    pinMode(_rst, OUTPUT);
    pinMode(_cs, OUTPUT);
    pinMode(_dc, OUTPUT);
    digitalWrite(_cs, HIGH);
    digitalWrite(_rst, HIGH);
#if defined(PIN_TFT_VDD_CTL)
    if (PIN_TFT_VDD_CTL >= 0) {
      pinMode(PIN_TFT_VDD_CTL, OUTPUT);
      digitalWrite(PIN_TFT_VDD_CTL, HIGH);
      digitalWrite(PIN_TFT_VDD_CTL, LOW);
      delay(10);
    }
#endif
#if defined(PIN_TFT_LEDA_CTL)
    if (PIN_TFT_LEDA_CTL >= 0) {
      pinMode(PIN_TFT_LEDA_CTL, OUTPUT);
      digitalWrite(PIN_TFT_LEDA_CTL, HIGH);
    }
#endif
  }

  void hardReset() {
    digitalWrite(_rst, LOW);
    delay(10);
    digitalWrite(_rst, HIGH);
    delay(100);
  }

  void setSpiHz(uint32_t hz) { _spi_hz = hz; }

  void setGap(uint16_t x, uint16_t y) {
    _x_gap = x + HELTEC_ST7735_X_GAP_ADJUST;
    _y_gap = y + HELTEC_ST7735_Y_GAP_ADJUST;
  }

  void initPanel(uint8_t rotation) {
    runDisplayInitTable(kRcmd1);
    runDisplayInitTable(kRcmd2Mini160x80);
    runDisplayInitTable(kRcmd3);
    applyRotation(rotation);
  }

  void flushRgb565(const lv_area_t* area, const lv_color_t* color_p) {
    int32_t x_start = area->x1;
    int32_t x_end = area->x2 + 1;
    int32_t y_start = area->y1;
    int32_t y_end = area->y2 + 1;
    x_start += _x_gap;
    x_end += _x_gap;
    y_start += _y_gap;
    y_end += _y_gap;
    uint8_t caset[4] = {(uint8_t)((x_start >> 8) & 0xFF), (uint8_t)(x_start & 0xFF), (uint8_t)(((x_end - 1) >> 8) & 0xFF),
                        (uint8_t)((x_end - 1) & 0xFF)};
    uint8_t raset[4] = {(uint8_t)((y_start >> 8) & 0xFF), (uint8_t)(y_start & 0xFF), (uint8_t)(((y_end - 1) >> 8) & 0xFF),
                        (uint8_t)((y_end - 1) & 0xFF)};
    sendCmd(LV_LCD_CMD_SET_COLUMN_ADDRESS, caset, 4);
    sendCmd(LV_LCD_CMD_SET_PAGE_ADDRESS, raset, 4);
    const size_t len = (size_t)(x_end - x_start) * (size_t)(y_end - y_start) * (LV_COLOR_DEPTH / 8);
    sendPixels(reinterpret_cast<const uint8_t*>(color_p), len);
  }

 private:
  void sendCmd(uint8_t cmd, const uint8_t* param, size_t param_len) {
    SPISettings settings(_spi_hz, MSBFIRST, SPI_MODE0);
    digitalWrite(_dc, LOW);
    digitalWrite(_cs, LOW);
    _spi->beginTransaction(settings);
    _spi->transfer(cmd);
    _spi->endTransaction();
    digitalWrite(_cs, HIGH);
    if (param_len > 0 && param) {
      digitalWrite(_dc, HIGH);
      digitalWrite(_cs, LOW);
      _spi->beginTransaction(settings);
      for (size_t i = 0; i < param_len; i++) {
        _spi->transfer(param[i]);
      }
      _spi->endTransaction();
      digitalWrite(_cs, HIGH);
    }
  }

  void sendPixels(const uint8_t* rgb565, size_t num_bytes) {
    SPISettings settings(_spi_hz, MSBFIRST, SPI_MODE0);
    digitalWrite(_dc, LOW);
    digitalWrite(_cs, LOW);
    _spi->beginTransaction(settings);
    _spi->transfer(LV_LCD_CMD_WRITE_MEMORY_START);
    _spi->endTransaction();
    digitalWrite(_dc, HIGH);
    digitalWrite(_cs, LOW);
    _spi->beginTransaction(settings);
    const size_t npx = num_bytes / 2;
    const lv_color_t* px = reinterpret_cast<const lv_color_t*>(rgb565);
#if (LV_COLOR_DEPTH == 16) && (!defined(LV_COLOR_16_SWAP) || (LV_COLOR_16_SWAP == 0))
    static uint8_t s_tx_chunk[512];
    size_t i = 0;
    while (i < npx) {
      const size_t chunk = (npx - i) > 256 ? 256 : (npx - i);
      uint8_t* dst = s_tx_chunk;
      for (size_t j = 0; j < chunk; ++j) {
        const uint16_t v = ST7735_LV_TO_RGB565(px[i + j].full);
        *dst++ = (uint8_t)(v >> 8);
        *dst++ = (uint8_t)(v & 0xFF);
      }
      _spi->transfer(s_tx_chunk, chunk * 2);
      i += chunk;
    }
#else
    for (size_t i = 0; i < npx; i++) {
      const uint16_t v = ST7735_LV_TO_RGB565(px[i].full);
      _spi->transfer((uint8_t)(v >> 8));
      _spi->transfer((uint8_t)(v & 0xFF));
    }
#endif
    _spi->endTransaction();
    digitalWrite(_cs, HIGH);
  }

  void runDisplayInitTable(const uint8_t* addr) {
    uint8_t numCommands = *addr++;
    while (numCommands--) {
      uint8_t cmd = *addr++;
      uint8_t numArgs = *addr++;
      const uint8_t delay_flag = (uint8_t)(numArgs & kStCmdDelay);
      numArgs &= (uint8_t)~kStCmdDelay;
      sendCmd(cmd, addr, numArgs);
      addr += numArgs;
      if (delay_flag) {
        uint8_t d = *addr++;
        if (d == 255) {
          delay(500);
        } else {
          delay(d);
        }
      }
    }
  }

  void sendMadctl(uint8_t mad) {
    _madctl = mad;
    sendCmd(ST77XX_MADCTL, &mad, 1);
  }

  void applyRotation(uint8_t m) {
    m = (uint8_t)(m & 3);
    uint8_t mad = 0;
    switch (m) {
      case 0:
        mad = (uint8_t)(ST77XX_MADCTL_MX | ST77XX_MADCTL_MY | kMadColor);
        setGap(kColStart, kRowStart);
        break;
      case 1:
        mad = (uint8_t)(ST77XX_MADCTL_MY | ST77XX_MADCTL_MV | kMadColor);
        setGap(kRowStart, kColStart);
        break;
      case 2:
        mad = kMadColor;
        setGap(kColStart, kRowStart);
        break;
      case 3:
      default:
        mad = (uint8_t)(ST77XX_MADCTL_MX | ST77XX_MADCTL_MV | kMadColor);
        setGap(kRowStart, kColStart);
        break;
    }
    sendMadctl(mad);
  }

  SPIClass* _spi;
  int8_t _cs;
  int8_t _dc;
  int8_t _rst;
  uint32_t _spi_hz;
  uint16_t _x_gap;
  uint16_t _y_gap;
  uint8_t _madctl;
};

static MiniMipiSt7735 s_panel(tft_spi, PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_RST);
static bool s_backlight_on = true;

#ifndef PIN_TFT_LEDA_CTL_ACTIVE
/** T114/Badge mini panel: init leaves LEDA low when visible. */
#define HELTEC_TFT_LEDA_ON_LEVEL LOW
#else
#define HELTEC_TFT_LEDA_ON_LEVEL PIN_TFT_LEDA_CTL_ACTIVE
#endif

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

#if defined(ARDUINO_ARCH_NRF52)
  tft_spi.setPins(-1, PIN_TFT_SCL, PIN_TFT_SDA);
  tft_spi.begin();
#elif defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_ARCH_STM32)
  tft_spi.begin(PIN_TFT_SCL, -1, PIN_TFT_SDA, PIN_TFT_CS);
#else
  tft_spi.begin();
#endif
  tft_spi.setBitOrder(MSBFIRST);
  tft_spi.setDataMode(SPI_MODE0);

  s_panel.hardReset();
  s_panel.setSpiHz(kTftSpiHz);
#ifdef DISPLAY_ROTATION
  s_panel.initPanel((uint8_t)(DISPLAY_ROTATION & 3));
#else
  s_panel.initPanel(0);
#endif

  for (int x = 0; x < kBootClearWidth; x++) {
    s_boot_clear_row[x] = lv_color_black();
  }
  for (int y = 0; y < kBootClearHeight; y++) {
    lv_area_t ln;
    ln.x1 = 0;
    ln.y1 = (lv_coord_t)y;
    ln.x2 = (lv_coord_t)(kBootClearWidth - 1);
    ln.y2 = (lv_coord_t)y;
    s_panel.flushRgb565(&ln, s_boot_clear_row);
  }

  apply_backlight(true);

  s_hw_inited = true;
}

}  // namespace

bool init() {
  if (s_disp_registered) return lv_disp_get_default() != nullptr;

  hw_begin();

  lv_disp_draw_buf_init(&s_draw_buf, s_buf, nullptr, s_buf_size_px);

  lv_disp_drv_init(&s_disp_drv);
  s_disp_drv.hor_res = kScrWidth;
  s_disp_drv.ver_res = kScrHeight;
  s_disp_drv.flush_cb = [](lv_disp_drv_t* drv, const lv_area_t* area, lv_color_t* color_p) {
    const uint32_t w = (uint32_t)lv_area_get_width(area);
    const uint32_t h = (uint32_t)lv_area_get_height(area);

#if (LV_COLOR_DEPTH == 16) && (!defined(LV_COLOR_16_SWAP) || (LV_COLOR_16_SWAP == 0))
    s_panel.flushRgb565(area, color_p);
#else
    static constexpr uint32_t kChunkPx = 256;
    static lv_color_t tmp_area_buf[kChunkPx];
    for (uint32_t row = 0; row < h; row++) {
      lv_color_t* src = color_p + row * w;
      uint32_t col = 0;
      while (col < w) {
        const uint32_t n = (w - col) < kChunkPx ? (w - col) : kChunkPx;
        for (uint32_t i = 0; i < n; i++) {
          tmp_area_buf[i].full = ST7735_LV_TO_RGB565((uint16_t)src[col + i].full);
        }
        lv_area_t slice;
        slice.x1 = (lv_coord_t)(area->x1 + (lv_coord_t)col);
        slice.y1 = (lv_coord_t)(area->y1 + (lv_coord_t)row);
        slice.x2 = (lv_coord_t)(slice.x1 + (lv_coord_t)n - 1);
        slice.y2 = (lv_coord_t)(area->y1 + (lv_coord_t)row);
        s_panel.flushRgb565(&slice, tmp_area_buf);
        col += n;
      }
    }
#endif
    lv_disp_flush_ready(drv);
  };
  s_disp_drv.draw_buf = &s_draw_buf;

  lv_disp_t* disp = lv_disp_drv_register(&s_disp_drv);
  s_disp_registered = disp != nullptr;
  return s_disp_registered;
}

void deinit() {
  lv_disp_t* d = lv_disp_get_default();
  if (d) lv_disp_remove(d);
}

void setBacklightOn(bool on) {
  if (!s_hw_inited) return;
  apply_backlight(on);
}

bool isBacklightOn() { return s_backlight_on; }

}  // namespace heltec::meshcore::dal::st7735
