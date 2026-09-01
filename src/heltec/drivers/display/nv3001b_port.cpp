#include "nv3001b_port.hpp"

#include <Arduino.h>
#include <SPI.h>
#include <lvgl.h>
#if !RADIOCORE_HAS_NV3001B_DISPLAY
#error "NV3001B display support is not enabled for this Radio Core target."
#endif

namespace heltec::meshcore::dal::nv3001b
{
namespace
{

#ifndef DISPLAY_ROTATION
#define DISPLAY_ROTATION 3
#endif

constexpr uint8_t kDisplayRotation = DISPLAY_ROTATION;
constexpr uint16_t kNativePanelWidth = 128;
constexpr uint16_t kNativePanelHeight = 220;
constexpr bool kIsLandscape = (kDisplayRotation & 1U) != 0U;
constexpr uint16_t kDisplayWidth = kIsLandscape ? kNativePanelHeight : kNativePanelWidth;
constexpr uint16_t kDisplayHeight = kIsLandscape ? kNativePanelWidth : kNativePanelHeight;
constexpr uint32_t kSpiFrequency = RADIOCORE_NV3001B_SPI_FREQUENCY;
constexpr uint16_t kDrawBufferLines = 12;
constexpr size_t kDrawBufferPixels = kDisplayWidth * kDrawBufferLines;
constexpr uint8_t kCommandColumnAddress = 0x2A;
constexpr uint8_t kCommandRowAddress = 0x2B;
constexpr uint8_t kCommandMemoryWrite = 0x2C;
constexpr uint8_t kCommandAddressMode = 0x36;
constexpr uint8_t kMadctlMy = 0x80;
constexpr uint8_t kMadctlMx = 0x40;
constexpr uint8_t kMadctlMv = 0x20;
constexpr uint8_t kMadctlBgr = 0x08;

struct InitCommand
{
    uint8_t command;
    uint8_t data[2];
    uint8_t data_length;
};

#define NV_CMD(command) {command, {0x00, 0x00}, 0}
#define NV_CMD1(command, data0) {command, {data0, 0x00}, 1}
#define NV_CMD2(command, data0, data1) {command, {data0, data1}, 2}

constexpr InitCommand kInitCommands[] = {
    NV_CMD1(0xFF, 0xA5), NV_CMD1(0x41, 0x00), NV_CMD1(0x50, 0x02),
    NV_CMD1(0x52, 0x6E), NV_CMD1(0x57, 0x02), NV_CMD1(0x46, 0x11),
    NV_CMD2(0x47, 0x00, 0x01), NV_CMD2(0x8F, 0x22, 0x03),
    NV_CMD1(0x9A, 0x78), NV_CMD1(0x9B, 0x78), NV_CMD1(0x9C, 0xA0),
    NV_CMD1(0x9D, 0x17), NV_CMD1(0x9E, 0xC1), NV_CMD1(0x83, 0x5A),
    NV_CMD1(0x84, 0xB6), NV_CMD1(0xFF, 0xA5), NV_CMD1(0x85, 0x5F),
    NV_CMD1(0x6E, 0x0F), NV_CMD1(0x7E, 0x0F), NV_CMD1(0x60, 0x00),
    NV_CMD1(0x70, 0x00), NV_CMD1(0x6D, 0x33), NV_CMD1(0x7D, 0x37),
    NV_CMD1(0x61, 0x09), NV_CMD1(0x71, 0x0A), NV_CMD1(0x6C, 0x2A),
    NV_CMD1(0x7C, 0x36), NV_CMD1(0x62, 0x11), NV_CMD1(0x72, 0x10),
    NV_CMD1(0x68, 0x4E), NV_CMD1(0x78, 0x4E), NV_CMD1(0x66, 0x36),
    NV_CMD1(0x76, 0x3C), NV_CMD1(0x1A, 0x1C), NV_CMD1(0x7B, 0x14),
    NV_CMD1(0x63, 0x0D), NV_CMD1(0x73, 0x0A), NV_CMD1(0x6A, 0x16),
    NV_CMD1(0x7A, 0x12), NV_CMD1(0x64, 0x0B), NV_CMD1(0x74, 0x0A),
    NV_CMD1(0x69, 0x08), NV_CMD1(0x79, 0x0A), NV_CMD1(0x65, 0x06),
    NV_CMD1(0x75, 0x07), NV_CMD1(0x67, 0x23), NV_CMD1(0x77, 0x44),
    NV_CMD1(0xE0, 0x00), NV_CMD1(0xE9, 0x30), NV_CMD1(0xEB, 0xB7),
    NV_CMD1(0xEC, 0x00), NV_CMD1(0xED, 0x11), NV_CMD1(0xF0, 0xB7),
    NV_CMD1(0x53, 0x04), NV_CMD1(0x54, 0x04), NV_CMD1(0x55, 0x40),
    NV_CMD1(0x56, 0x40), NV_CMD2(0xA0, 0x60, 0x01), NV_CMD1(0xA1, 0x84),
    NV_CMD1(0xA2, 0x85), NV_CMD2(0xAB, 0x00, 0x02),
    NV_CMD2(0xAC, 0x00, 0x06), NV_CMD2(0xAD, 0x00, 0x03),
    NV_CMD2(0xAE, 0x00, 0x07), NV_CMD1(0xC7, 0x01), NV_CMD1(0xB9, 0x82),
    NV_CMD1(0xBA, 0x83), NV_CMD1(0xBB, 0x00), NV_CMD1(0xBC, 0x81),
    NV_CMD1(0xBD, 0x02), NV_CMD1(0xBE, 0x01), NV_CMD1(0xBF, 0x04),
    NV_CMD1(0xC0, 0x03), NV_CMD1(0xC8, 0x55), NV_CMD1(0xC9, 0xC9),
    NV_CMD1(0xCA, 0xC8), NV_CMD1(0xCB, 0xCB), NV_CMD1(0xCC, 0xCA),
    NV_CMD1(0xCD, 0x55), NV_CMD1(0xCE, 0xCE), NV_CMD1(0xCF, 0xCD),
    NV_CMD1(0xD0, 0xD0), NV_CMD1(0xD1, 0xCF), NV_CMD1(0xF2, 0x46),
    NV_CMD1(0xA8, 0x04), NV_CMD1(0xA9, 0xB0), NV_CMD1(0xAA, 0xA3),
    NV_CMD1(0xB6, 0x00), NV_CMD1(0xB7, 0xB0), NV_CMD1(0xB8, 0xA3),
    NV_CMD1(0xC4, 0x03), NV_CMD1(0xC5, 0xB0), NV_CMD1(0xC6, 0xA3),
    NV_CMD1(0x80, 0x10), NV_CMD1(0xFF, 0x00), NV_CMD1(0x35, 0x00),
    NV_CMD(0x11),
};

#undef NV_CMD
#undef NV_CMD1
#undef NV_CMD2

// ---------- 硬件 SPI 总线对象 ----------
#if defined(RADIOCORE_BOARD_RC32)
SPIClass s_display_spi(FSPI);
#elif defined(RADIOCORE_BOARD_RC52)
/*
 * Adafruit nRF52 SPIClass maps every constructor pin through
 * g_ADigitalPinMap and cannot accept -1. The NV3001B is write-only, so this
 * valid, otherwise unused Arduino pin is only a framework compatibility
 * placeholder; the configured display MISO remains disabled.
 */
constexpr uint8_t kNrf52SpiUnusedMisoPlaceholder = 12U;
static_assert(RADIOCORE_NV3001B_MISO < 0,
              "The RC52 NV3001B display must not configure a MISO pin");
SPIClass s_display_spi(NRF_SPIM3, kNrf52SpiUnusedMisoPlaceholder,
                       RADIOCORE_NV3001B_SCK, RADIOCORE_NV3001B_MOSI);
#elif defined(ARDUINO_ARCH_ESP32) && defined(RADIOCORE_NV3001B_SPI_HOST)
SPIClass s_display_spi(RADIOCORE_NV3001B_SPI_HOST);
#else
SPIClass s_display_spi;
#endif
LV_ATTRIBUTE_FAST_MEM LV_ATTRIBUTE_MEM_ALIGN static lv_color_t
    s_draw_buffer[kDrawBufferPixels];
static lv_disp_draw_buf_t s_draw_buf;
static lv_disp_drv_t s_disp_drv;
static bool s_hw_inited = false;
static bool s_disp_registered = false;
static bool s_backlight_on = false;

// ---------- 硬件 SPI 传输函数 ----------
inline __attribute__((always_inline)) void write_transport_byte(uint8_t value)
{
    s_display_spi.transfer(value);
}

bool write_transport_bytes(const uint8_t *data, size_t data_length)
{
    if (data == nullptr || data_length == 0U)
    {
        return data_length == 0U;
    }
    for (size_t i = 0; i < data_length; ++i)
    {
        write_transport_byte(data[i]);
    }
    return true;
}

void write_rgb565_row(const lv_color_t *pixels, int32_t width)
{
    for (int32_t column = 0; column < width; ++column)
    {
        uint16_t color = pixels[column].full;
#if defined(LV_COLOR_16_SWAP) && (LV_COLOR_16_SWAP != 0)
        color = static_cast<uint16_t>((color << 8) | (color >> 8));
#endif
        write_transport_byte(static_cast<uint8_t>(color >> 8));
        write_transport_byte(static_cast<uint8_t>(color));
    }
}

void begin_write()
{
    s_display_spi.beginTransaction(SPISettings(kSpiFrequency, MSBFIRST, SPI_MODE0));
    digitalWrite(RADIOCORE_NV3001B_CS, LOW);
}

void end_write()
{
    digitalWrite(RADIOCORE_NV3001B_CS, HIGH);
    s_display_spi.endTransaction();
}

void write_command(uint8_t command)
{
    digitalWrite(RADIOCORE_NV3001B_DC, LOW);
    write_transport_byte(command);
    digitalWrite(RADIOCORE_NV3001B_DC, HIGH);
}

void write_command_data(uint8_t command, const uint8_t *data, size_t data_length)
{
    begin_write();
    write_command(command);
    (void)write_transport_bytes(data, data_length);
    end_write();
}

void set_panel_power(bool enabled)
{
    pinMode(RADIOCORE_NV3001B_ENABLE, OUTPUT);
    digitalWrite(RADIOCORE_NV3001B_ENABLE,
                 enabled ? RADIOCORE_NV3001B_ENABLE_ACTIVE
                         : !RADIOCORE_NV3001B_ENABLE_ACTIVE);
}

void set_panel_backlight(bool enabled)
{
    pinMode(RADIOCORE_NV3001B_BACKLIGHT, OUTPUT);
    digitalWrite(RADIOCORE_NV3001B_BACKLIGHT,
                 enabled ? RADIOCORE_NV3001B_BACKLIGHT_ACTIVE
                         : !RADIOCORE_NV3001B_BACKLIGHT_ACTIVE);
    s_backlight_on = enabled;
}

void begin_transport()
{
    pinMode(RADIOCORE_NV3001B_DC, OUTPUT);
    pinMode(RADIOCORE_NV3001B_CS, OUTPUT);
    digitalWrite(RADIOCORE_NV3001B_DC, HIGH);
    digitalWrite(RADIOCORE_NV3001B_CS, HIGH);
#if defined(ARDUINO_ARCH_ESP32)
    s_display_spi.begin(RADIOCORE_NV3001B_SCK, RADIOCORE_NV3001B_MISO,
                        RADIOCORE_NV3001B_MOSI, RADIOCORE_NV3001B_CS);
#elif defined(RADIOCORE_BOARD_RC52)
    s_display_spi.begin();
#else
    s_display_spi.setPins(RADIOCORE_NV3001B_MISO, RADIOCORE_NV3001B_SCK,
                          RADIOCORE_NV3001B_MOSI);
    s_display_spi.begin();
#endif
}

void reset_panel()
{
#if RADIOCORE_NV3001B_RST >= 0
    pinMode(RADIOCORE_NV3001B_RST, OUTPUT);
    digitalWrite(RADIOCORE_NV3001B_RST, HIGH);
    delay(100);
    digitalWrite(RADIOCORE_NV3001B_RST, LOW);
    delay(120);
    digitalWrite(RADIOCORE_NV3001B_RST, HIGH);
    delay(120);
#endif
}

void initialize_panel()
{
    reset_panel();

    begin_write();
    for (const InitCommand &entry : kInitCommands)
    {
        write_command(entry.command);
        (void)write_transport_bytes(entry.data, entry.data_length);
    }
    end_write();

    delay(120);
    const uint8_t color_mode = 0x05;

    begin_write();
    write_command(0x3A);
    write_transport_byte(color_mode);
    write_command(0x29);
    end_write();

    delay(10);

    write_command_data(0x20, nullptr, 0);
}

void set_panel_rotation_and_color_order()
{
    uint8_t madctl = kMadctlBgr;
    switch (kDisplayRotation & 3U)
    {
    case 1:
        madctl |= kMadctlMx | kMadctlMv;
        break;
    case 2:
        madctl |= kMadctlMy | kMadctlMx;
        break;
    case 3:
        madctl |= kMadctlMy | kMadctlMv;
        break;
    default:
        break;
    }
    write_command_data(kCommandAddressMode, &madctl, 1);
}

void write_address_window(const lv_area_t *area)
{
    const uint8_t columns[] = {
        static_cast<uint8_t>(area->x1 >> 8), static_cast<uint8_t>(area->x1),
        static_cast<uint8_t>(area->x2 >> 8), static_cast<uint8_t>(area->x2)};
    const uint8_t rows[] = {
        static_cast<uint8_t>(area->y1 >> 8), static_cast<uint8_t>(area->y1),
        static_cast<uint8_t>(area->y2 >> 8), static_cast<uint8_t>(area->y2)};

    write_command(kCommandColumnAddress);
    (void)write_transport_bytes(columns, sizeof(columns));
    write_command(kCommandRowAddress);
    (void)write_transport_bytes(rows, sizeof(rows));
    write_command(kCommandMemoryWrite);
}

void flush_area(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
{
    const int32_t width = lv_area_get_width(area);
    const int32_t height = lv_area_get_height(area);
    if (width <= 0 || height <= 0 || color_p == nullptr)
    {
        lv_disp_flush_ready(drv);
        return;
    }

    begin_write();
    write_address_window(area);
    for (int32_t row = 0; row < height; ++row)
    {
        write_rgb565_row(&color_p[row * width], width);
    }
    end_write();
    lv_disp_flush_ready(drv);
}

} // namespace

bool init()
{
    if (s_disp_registered)
    {
        return lv_disp_get_default() != nullptr;
    }

    if (!s_hw_inited)
    {
        set_panel_power(true);
        set_panel_backlight(false);
        delay(20);
        begin_transport();
        initialize_panel();
        set_panel_rotation_and_color_order();
        set_panel_backlight(true);
        s_hw_inited = true;
    }

    lv_disp_draw_buf_init(&s_draw_buf, s_draw_buffer, nullptr,
                          kDrawBufferPixels);
    lv_disp_drv_init(&s_disp_drv);
    s_disp_drv.hor_res = kDisplayWidth;
    s_disp_drv.ver_res = kDisplayHeight;
    s_disp_drv.flush_cb = flush_area;
    s_disp_drv.draw_buf = &s_draw_buf;
    s_disp_registered = lv_disp_drv_register(&s_disp_drv) != nullptr;
    return s_disp_registered;
}

void deinit()
{
    lv_disp_t *const display = lv_disp_get_default();
    if (display != nullptr)
    {
        lv_disp_remove(display);
    }
    s_disp_registered = false;
}

void setBacklightOn(bool on)
{
    if (s_hw_inited)
    {
        set_panel_backlight(on);
    }
}

bool isBacklightOn()
{
    return s_backlight_on;
}

} // namespace heltec::meshcore::dal::nv3001b
