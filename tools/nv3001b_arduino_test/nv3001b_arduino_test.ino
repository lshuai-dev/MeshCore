/**
 * @file    nv3001b_arduino_test.ino
 * @brief   Standalone Arduino hardware test for a 128 x 220 NV3001B SPI panel.
 * @note    No LVGL or display library is required. Defaults target Radio Core RC32.
 */

#include <Arduino.h>
#include <SPI.h>

/* Radio Core RC32 defaults. Change these values for another board. */
static constexpr int kPinSck = 17;
static constexpr int kPinMosi = 38;
static constexpr int kPinCs = 39;
static constexpr int kPinDc = 16;
static constexpr int kPinReset = 4;
static constexpr int kPinPanelPower = 6;
static constexpr int kPinBacklight = 5;
static constexpr uint8_t kPanelPowerOnLevel = LOW;
static constexpr uint8_t kBacklightOnLevel = HIGH;

static constexpr uint32_t kSpiFrequency = 40000000U;
static constexpr uint16_t kNativeWidth = 128;
static constexpr uint16_t kNativeHeight = 220;
static constexpr uint8_t kRotation = 3;
static constexpr bool kLandscape = (kRotation & 1U) != 0U;
static constexpr uint16_t kWidth = kLandscape ? kNativeHeight : kNativeWidth;
static constexpr uint16_t kHeight = kLandscape ? kNativeWidth : kNativeHeight;
static constexpr size_t kPixelChunkSize = 128;

struct InitCommand
{
    uint8_t command;
    uint8_t data[2];
    uint8_t data_length;
};

#define NV_CMD(command) {command, {0x00, 0x00}, 0}
#define NV_CMD1(command, data0) {command, {data0, 0x00}, 1}
#define NV_CMD2(command, data0, data1) {command, {data0, data1}, 2}

static constexpr InitCommand kInitCommands[] = {
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

static SPIClass &s_spi = SPI;
static uint8_t s_transfer_buffer[kPixelChunkSize * 2U] = {};
static uint8_t s_test = 0;
static uint8_t s_solid_index = 0;
static uint8_t s_auto_test = 1;
static bool s_auto_enabled = true;
static uint32_t s_test_started_ms = 0;
static uint32_t s_last_frame_ms = 0;
static uint32_t s_frame_count = 0;
static int16_t s_marquee_x = 0;

static constexpr uint16_t kBlack = 0x0000;
static constexpr uint16_t kWhite = 0xFFFF;
static constexpr uint16_t kRed = 0xF800;
static constexpr uint16_t kGreen = 0x07E0;
static constexpr uint16_t kBlue = 0x001F;
static constexpr uint16_t kCyan = 0x07FF;
static constexpr uint16_t kMagenta = 0xF81F;
static constexpr uint16_t kYellow = 0xFFE0;

void beginWrite()
{
    s_spi.beginTransaction(SPISettings(kSpiFrequency, MSBFIRST, SPI_MODE0));
    digitalWrite(kPinCs, LOW);
}

void endWrite()
{
    digitalWrite(kPinCs, HIGH);
    s_spi.endTransaction();
}

void writeCommand(uint8_t command)
{
    digitalWrite(kPinDc, LOW);
    s_spi.transfer(command);
    digitalWrite(kPinDc, HIGH);
}

void writeCommandData(uint8_t command, const uint8_t *data, size_t length)
{
    beginWrite();
    writeCommand(command);
    for (size_t index = 0; index < length; ++index)
    {
        s_spi.transfer(data[index]);
    }
    endWrite();
}

void writeBytes(const uint8_t *data, size_t length)
{
    if (data == nullptr)
    {
        return;
    }
    for (size_t index = 0; index < length; ++index)
    {
        s_spi.transfer(data[index]);
    }
}

void setAddressWindow(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    const uint16_t x2 = static_cast<uint16_t>(x + width - 1U);
    const uint16_t y2 = static_cast<uint16_t>(y + height - 1U);
    const uint8_t columns[] = {
        static_cast<uint8_t>(x >> 8), static_cast<uint8_t>(x),
        static_cast<uint8_t>(x2 >> 8), static_cast<uint8_t>(x2)};
    const uint8_t rows[] = {
        static_cast<uint8_t>(y >> 8), static_cast<uint8_t>(y),
        static_cast<uint8_t>(y2 >> 8), static_cast<uint8_t>(y2)};

    writeCommand(0x2A);
    writeBytes(columns, sizeof(columns));
    writeCommand(0x2B);
    writeBytes(rows, sizeof(rows));
    writeCommand(0x2C);
}

void writeRepeatedColor(uint16_t color, size_t pixel_count)
{
    for (size_t index = 0; index < kPixelChunkSize; ++index)
    {
        s_transfer_buffer[index * 2U] = static_cast<uint8_t>(color >> 8);
        s_transfer_buffer[index * 2U + 1U] = static_cast<uint8_t>(color);
    }
    while (pixel_count > 0U)
    {
        const size_t count = pixel_count < kPixelChunkSize ? pixel_count : kPixelChunkSize;
        writeBytes(s_transfer_buffer, count * 2U);
        pixel_count -= count;
    }
}

void fillRect(int16_t x, int16_t y, int16_t width, int16_t height, uint16_t color)
{
    if (width <= 0 || height <= 0 || x >= static_cast<int16_t>(kWidth) ||
        y >= static_cast<int16_t>(kHeight))
    {
        return;
    }
    if (x < 0)
    {
        width = static_cast<int16_t>(width + x);
        x = 0;
    }
    if (y < 0)
    {
        height = static_cast<int16_t>(height + y);
        y = 0;
    }
    if (x + width > static_cast<int16_t>(kWidth))
    {
        width = static_cast<int16_t>(kWidth - x);
    }
    if (y + height > static_cast<int16_t>(kHeight))
    {
        height = static_cast<int16_t>(kHeight - y);
    }
    if (width <= 0 || height <= 0)
    {
        return;
    }

    beginWrite();
    setAddressWindow(static_cast<uint16_t>(x), static_cast<uint16_t>(y),
                     static_cast<uint16_t>(width), static_cast<uint16_t>(height));
    writeRepeatedColor(color, static_cast<size_t>(width) * static_cast<size_t>(height));
    endWrite();
}

void fillScreen(uint16_t color)
{
    fillRect(0, 0, kWidth, kHeight, color);
}

uint16_t rgb565(uint8_t red, uint8_t green, uint8_t blue)
{
    return static_cast<uint16_t>(((red & 0xF8U) << 8) |
                                 ((green & 0xFCU) << 3) | (blue >> 3));
}

void initializePanel()
{
    pinMode(kPinPanelPower, OUTPUT);
    digitalWrite(kPinPanelPower, kPanelPowerOnLevel);
    pinMode(kPinBacklight, OUTPUT);
    digitalWrite(kPinBacklight, !kBacklightOnLevel);
    pinMode(kPinCs, OUTPUT);
    pinMode(kPinDc, OUTPUT);
    pinMode(kPinReset, OUTPUT);
    digitalWrite(kPinCs, HIGH);
    digitalWrite(kPinDc, HIGH);

#if defined(ARDUINO_ARCH_ESP32)
    s_spi.begin(kPinSck, -1, kPinMosi, kPinCs);
#else
    s_spi.begin();
#endif

    digitalWrite(kPinReset, HIGH);
    delay(100);
    digitalWrite(kPinReset, LOW);
    delay(120);
    digitalWrite(kPinReset, HIGH);
    delay(120);

    beginWrite();
    for (const InitCommand &entry : kInitCommands)
    {
        writeCommand(entry.command);
        for (uint8_t index = 0; index < entry.data_length; ++index)
        {
            s_spi.transfer(entry.data[index]);
        }
    }
    endWrite();
    delay(120);

    const uint8_t color_mode = 0x05;
    writeCommandData(0x3A, &color_mode, 1);

    uint8_t madctl = 0x08;
    switch (kRotation & 3U)
    {
    case 1:
        madctl |= 0x40U | 0x20U;
        break;
    case 2:
        madctl |= 0x80U | 0x40U;
        break;
    case 3:
        madctl |= 0x80U | 0x20U;
        break;
    default:
        break;
    }
    writeCommandData(0x36, &madctl, 1);
    writeCommandData(0x29, nullptr, 0);
    writeCommandData(0x20, nullptr, 0);
    delay(10);
    digitalWrite(kPinBacklight, kBacklightOnLevel);
}

void drawColorBars()
{
    static constexpr uint16_t colors[] = {
        kWhite, kYellow, kCyan, kGreen, kMagenta, kRed, kBlue, kBlack};
    const int16_t bar_width = static_cast<int16_t>(kWidth / 8U);
    for (size_t index = 0; index < 8U; ++index)
    {
        const int16_t x = static_cast<int16_t>(index * bar_width);
        const int16_t width = index == 7U ? static_cast<int16_t>(kWidth - x) : bar_width;
        fillRect(x, 0, width, kHeight, colors[index]);
    }
}

void drawCheckerboard()
{
    static constexpr int16_t kCell = 16;
    for (int16_t y = 0; y < static_cast<int16_t>(kHeight); y += kCell)
    {
        for (int16_t x = 0; x < static_cast<int16_t>(kWidth); x += kCell)
        {
            const uint16_t color = ((x / kCell + y / kCell) & 1) != 0 ? kWhite : kBlack;
            fillRect(x, y, kCell, kCell, color);
        }
    }
}

void drawGeometryTest()
{
    fillScreen(kBlack);
    fillRect(0, 0, kWidth, 2, kWhite);
    fillRect(0, kHeight - 2, kWidth, 2, kWhite);
    fillRect(0, 0, 2, kHeight, kWhite);
    fillRect(kWidth - 2, 0, 2, kHeight, kWhite);
    fillRect(2, 2, 14, 14, kRed);
    fillRect(kWidth - 16, 2, 14, 14, kGreen);
    fillRect(2, kHeight - 16, 14, 14, kBlue);
    fillRect(kWidth - 16, kHeight - 16, 14, 14, kYellow);
    fillRect(kWidth / 2 - 1, 0, 3, kHeight, kCyan);
    fillRect(0, kHeight / 2 - 1, kWidth, 3, kMagenta);
}

void drawGradient()
{
    for (uint16_t x = 0; x < kWidth; ++x)
    {
        const uint8_t red = static_cast<uint8_t>((static_cast<uint32_t>(x) * 255U) /
                                                 (kWidth - 1U));
        const uint8_t blue = static_cast<uint8_t>(255U - red);
        fillRect(x, 0, 1, kHeight / 2, rgb565(red, 0, blue));
        fillRect(x, kHeight / 2, 1, kHeight - kHeight / 2,
                 rgb565(red, red, red));
    }
}

void drawMarqueeFrame()
{
    static constexpr uint16_t colors[] = {kRed, kYellow, kGreen, kCyan, kBlue, kMagenta};
    static constexpr int16_t kBlockWidth = 26;
    static constexpr int16_t kGap = 10;
    static constexpr int16_t kTrackHeight = 24;
    static constexpr int16_t kPitch = kBlockWidth + kGap;
    static int16_t previous_x = -kBlockWidth;

    for (int16_t y = 8; y < static_cast<int16_t>(kHeight); y += kTrackHeight + 8)
    {
        fillRect(previous_x, y, kBlockWidth, kTrackHeight, kBlack);
        fillRect(s_marquee_x, y, kBlockWidth, kTrackHeight,
                 colors[(y / (kTrackHeight + 8)) % 6]);
    }
    previous_x = s_marquee_x;
    s_marquee_x = static_cast<int16_t>(s_marquee_x + 3);
    if (s_marquee_x >= static_cast<int16_t>(kWidth + kGap))
    {
        s_marquee_x = -kPitch;
    }
}

void drawStressFrame()
{
    static constexpr uint16_t colors[] = {
        kRed, kGreen, kBlue, kCyan, kMagenta, kYellow, kWhite};
    const int16_t width = static_cast<int16_t>(4U + random(28));
    const int16_t height = static_cast<int16_t>(4U + random(28));
    const int16_t x = static_cast<int16_t>(random(kWidth - width + 1));
    const int16_t y = static_cast<int16_t>(random(kHeight - height + 1));
    fillRect(x, y, width, height, colors[random(7)]);
    ++s_frame_count;
    if ((s_frame_count % 250U) == 0U)
    {
        Serial.printf("Stress rectangles: %lu\n", static_cast<unsigned long>(s_frame_count));
    }
}

void selectTest(uint8_t test)
{
    s_test = test;
    s_test_started_ms = millis();
    s_last_frame_ms = s_test_started_ms;
    Serial.printf("Test %u\n", static_cast<unsigned>(test));
    switch (test)
    {
    case 1:
        s_solid_index = 0;
        fillScreen(kRed);
        break;
    case 2:
        drawColorBars();
        break;
    case 3:
        drawCheckerboard();
        break;
    case 4:
        drawGeometryTest();
        break;
    case 5:
        drawGradient();
        break;
    case 6:
        fillScreen(kBlack);
        s_marquee_x = -26;
        break;
    case 7:
        fillScreen(kBlack);
        s_frame_count = 0;
        break;
    default:
        s_test = 2;
        drawColorBars();
        break;
    }
}

void printMenu()
{
    Serial.println("NV3001B test: 0=auto 1=solid 2=bars 3=checker 4=geometry");
    Serial.println("               5=gradient 6=marquee 7=stress ?=menu");
}

void setup()
{
    Serial.begin(115200);
    delay(200);
    Serial.printf("NV3001B %ux%u, SPI=%lu Hz\n", kWidth, kHeight,
                  static_cast<unsigned long>(kSpiFrequency));
    initializePanel();
    printMenu();
    selectTest(s_auto_test);
}

void loop()
{
    if (Serial.available() > 0)
    {
        const char input = static_cast<char>(Serial.read());
        if (input >= '0' && input <= '7')
        {
            const uint8_t requested = static_cast<uint8_t>(input - '0');
            if (requested == 0U)
            {
                s_auto_enabled = true;
                s_auto_test = 1;
                selectTest(s_auto_test);
            }
            else
            {
                s_auto_enabled = false;
                selectTest(requested);
            }
        }
        else if (input == '?')
        {
            printMenu();
        }
    }

    const uint32_t now = millis();
    if (s_auto_enabled && now - s_test_started_ms >= 2500U)
    {
        s_auto_test = s_auto_test >= 6U ? 1U : static_cast<uint8_t>(s_auto_test + 1U);
        selectTest(s_auto_test);
    }
    if (s_test == 1 && now - s_last_frame_ms >= 1000U)
    {
        static constexpr uint16_t colors[] = {
            kRed, kGreen, kBlue, kWhite, kBlack, kCyan, kMagenta, kYellow};
        s_solid_index = static_cast<uint8_t>((s_solid_index + 1U) % 8U);
        fillScreen(colors[s_solid_index]);
        s_last_frame_ms = now;
    }
    else if (s_test == 6 && now - s_last_frame_ms >= 25U)
    {
        drawMarqueeFrame();
        s_last_frame_ms = now;
    }
    else if (s_test == 7)
    {
        drawStressFrame();
    }
}
