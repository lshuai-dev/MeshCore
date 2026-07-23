/*******************************************************************************
 * Size: 8 px
 * Bpp: 1
 * Opts: --bpp 1 --size 8 --no-compress --stride 1 --align 1 --font dogica.ttf --range 32-127 --font FontAwesome5-Solid+Brands+Regular.woff --range 61441,61448,61452,61453,61457,61459,61461,61465,61468,61473,61478-61480,61498,61502,61507,61512,61515-61517,61521-61524,61543-61544,61550,61552-61553,61556,61559-61561,61563,61587,61589,61636-61639,61641,61664,61671 --format lvgl -o lv_font_dogica_8.c
 ******************************************************************************/

#ifdef __has_include
    #if __has_include("lvgl.h")
        #ifndef LV_LVGL_H_INCLUDE_SIMPLE
            #define LV_LVGL_H_INCLUDE_SIMPLE
        #endif
    #endif
#endif

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
    #include "lvgl.h"
#else
    #include "lvgl/lvgl.h"
#endif



#ifndef LV_FONT_DOGICA_8
#define LV_FONT_DOGICA_8 0
#endif

#if LV_FONT_DOGICA_8

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */
    0x0,

    /* U+0021 "!" */
    0xfa,

    /* U+0022 "\"" */
    0x55, 0xa0,

    /* U+0023 "#" */
    0x4b, 0xf4, 0x92, 0xfd, 0x20,

    /* U+0024 "$" */
    0x75, 0x68, 0xe2, 0xd5, 0xc0,

    /* U+0025 "%" */
    0x45, 0x52, 0xa2, 0x81, 0x45, 0x4a, 0xa2,

    /* U+0026 "&" */
    0x62, 0x49, 0x18, 0x96, 0x27, 0x40,

    /* U+0027 "'" */
    0x58,

    /* U+0028 "(" */
    0x6a, 0xa4,

    /* U+0029 ")" */
    0x95, 0x58,

    /* U+002A "*" */
    0x25, 0x55, 0x52, 0x0,

    /* U+002B "+" */
    0x21, 0x3e, 0x42, 0x0,

    /* U+002C "," */
    0x58,

    /* U+002D "-" */
    0xf0,

    /* U+002E "." */
    0x80,

    /* U+002F "/" */
    0x11, 0x22, 0x44, 0x88,

    /* U+0030 "0" */
    0x74, 0x6b, 0x5a, 0xc5, 0xc0,

    /* U+0031 "1" */
    0x27, 0x8, 0x42, 0x13, 0xe0,

    /* U+0032 "2" */
    0x74, 0x42, 0x22, 0x23, 0xe0,

    /* U+0033 "3" */
    0xf0, 0x42, 0xe0, 0x87, 0xc0,

    /* U+0034 "4" */
    0x11, 0x95, 0x2f, 0x88, 0x40,

    /* U+0035 "5" */
    0xfc, 0x3c, 0x10, 0xc5, 0xc0,

    /* U+0036 "6" */
    0x74, 0x61, 0xe8, 0xc5, 0xc0,

    /* U+0037 "7" */
    0xfc, 0x42, 0x22, 0x21, 0x0,

    /* U+0038 "8" */
    0x74, 0x62, 0xe8, 0xc5, 0xc0,

    /* U+0039 "9" */
    0x74, 0x62, 0xf0, 0x89, 0x80,

    /* U+003A ":" */
    0x90,

    /* U+003B ";" */
    0x45, 0x80,

    /* U+003C "<" */
    0x2a, 0x22,

    /* U+003D "=" */
    0xfc, 0xf, 0xc0,

    /* U+003E ">" */
    0x88, 0xa8,

    /* U+003F "?" */
    0x74, 0x42, 0x64, 0x0, 0x80,

    /* U+0040 "@" */
    0x79, 0xa, 0xd4, 0xab, 0x90, 0x5f, 0x0,

    /* U+0041 "A" */
    0x74, 0x63, 0x1f, 0xc6, 0x20,

    /* U+0042 "B" */
    0xf4, 0x63, 0xe8, 0xc7, 0xc0,

    /* U+0043 "C" */
    0x74, 0x61, 0x8, 0x45, 0xc0,

    /* U+0044 "D" */
    0xf9, 0x14, 0x51, 0x45, 0x17, 0x80,

    /* U+0045 "E" */
    0xfc, 0x21, 0xe8, 0x43, 0xe0,

    /* U+0046 "F" */
    0xfc, 0x21, 0xe8, 0x42, 0x0,

    /* U+0047 "G" */
    0x7c, 0x21, 0x38, 0xc5, 0xe0,

    /* U+0048 "H" */
    0x8c, 0x63, 0xf8, 0xc6, 0x20,

    /* U+0049 "I" */
    0xe9, 0x24, 0xb8,

    /* U+004A "J" */
    0xf2, 0x22, 0x22, 0xc0,

    /* U+004B "K" */
    0x8c, 0xa9, 0xc9, 0x4a, 0x20,

    /* U+004C "L" */
    0x88, 0x88, 0x88, 0xf0,

    /* U+004D "M" */
    0x86, 0x1c, 0xed, 0x86, 0x18, 0x40,

    /* U+004E "N" */
    0x8e, 0x6b, 0x38, 0xc6, 0x20,

    /* U+004F "O" */
    0x74, 0x63, 0x18, 0xc5, 0xc0,

    /* U+0050 "P" */
    0xf4, 0x63, 0xe8, 0x42, 0x0,

    /* U+0051 "Q" */
    0x74, 0x63, 0x1a, 0xc9, 0xa0,

    /* U+0052 "R" */
    0xf4, 0x63, 0xe9, 0x46, 0x20,

    /* U+0053 "S" */
    0x74, 0x60, 0xe0, 0xc5, 0xc0,

    /* U+0054 "T" */
    0xf9, 0x8, 0x42, 0x10, 0x80,

    /* U+0055 "U" */
    0x8c, 0x63, 0x18, 0xc5, 0xc0,

    /* U+0056 "V" */
    0x8c, 0x63, 0x15, 0x28, 0x80,

    /* U+0057 "W" */
    0x83, 0x6, 0x4c, 0x99, 0x35, 0x51, 0x0,

    /* U+0058 "X" */
    0x8c, 0x54, 0x45, 0x46, 0x20,

    /* U+0059 "Y" */
    0x8c, 0x62, 0xa2, 0x10, 0x80,

    /* U+005A "Z" */
    0xf8, 0x44, 0x44, 0x43, 0xe0,

    /* U+005B "[" */
    0xea, 0xac,

    /* U+005C "\\" */
    0x88, 0x44, 0x22, 0x11,

    /* U+005D "]" */
    0xd5, 0x5c,

    /* U+005E "^" */
    0x69,

    /* U+005F "_" */
    0xfc,

    /* U+0060 "`" */
    0x94,

    /* U+0061 "a" */
    0x70, 0x5f, 0x18, 0xbc,

    /* U+0062 "b" */
    0x84, 0x3d, 0x18, 0xc7, 0xc0,

    /* U+0063 "c" */
    0x74, 0x61, 0x8, 0x3c,

    /* U+0064 "d" */
    0x8, 0x5f, 0x18, 0xc5, 0xe0,

    /* U+0065 "e" */
    0x74, 0x7f, 0x8, 0x3c,

    /* U+0066 "f" */
    0x3a, 0x11, 0xe4, 0x21, 0x0,

    /* U+0067 "g" */
    0x7c, 0x63, 0x17, 0x85, 0xc0,

    /* U+0068 "h" */
    0x84, 0x2d, 0x98, 0xc6, 0x20,

    /* U+0069 "i" */
    0x43, 0x24, 0xb8,

    /* U+006A "j" */
    0xe9, 0x24, 0xa0,

    /* U+006B "k" */
    0x8c, 0xa9, 0xc9, 0x44,

    /* U+006C "l" */
    0x92, 0x48, 0xc0,

    /* U+006D "m" */
    0x6d, 0x26, 0x4c, 0x99, 0x32, 0x40,

    /* U+006E "n" */
    0x36, 0x63, 0x18, 0xc4,

    /* U+006F "o" */
    0x74, 0x63, 0x18, 0xb8,

    /* U+0070 "p" */
    0xf4, 0x63, 0x1f, 0x42, 0x0,

    /* U+0071 "q" */
    0x7c, 0x63, 0x17, 0x84, 0x20,

    /* U+0072 "r" */
    0xbc, 0x88, 0x88,

    /* U+0073 "s" */
    0x74, 0x1c, 0x18, 0xb8,

    /* U+0074 "t" */
    0x88, 0xf8, 0x88, 0x70,

    /* U+0075 "u" */
    0x8c, 0x63, 0x19, 0xb4,

    /* U+0076 "v" */
    0x8c, 0x62, 0xa5, 0x10,

    /* U+0077 "w" */
    0x6, 0x9a, 0x69, 0xa5, 0x60,

    /* U+0078 "x" */
    0x8a, 0x88, 0x45, 0x44,

    /* U+0079 "y" */
    0x99, 0x96, 0x2c,

    /* U+007A "z" */
    0xf1, 0x24, 0x8f,

    /* U+007B "{" */
    0x25, 0x48, 0x84, 0x52,

    /* U+007C "|" */
    0xff,

    /* U+007D "}" */
    0x4a, 0x21, 0x12, 0xa4,

    /* U+007E "~" */
    0x66, 0x60,

    /* U+007F "" */
    0x0,

    /* U+F001 "" */
    0x3, 0x3f, 0x3d, 0x21, 0x21, 0x27, 0xe7, 0xe0,

    /* U+F008 "" */
    0xff, 0xd1, 0x78, 0xff, 0xfa, 0x2f, 0xfc,

    /* U+F00C "" */
    0x3, 0x7, 0xce, 0xfc, 0x78, 0x30,

    /* U+F00D "" */
    0xcd, 0xe3, 0x1e, 0xcc,

    /* U+F011 "" */
    0x10, 0xab, 0x5c, 0x98, 0x30, 0x51, 0x1c,

    /* U+F013 "" */
    0x18, 0x5c, 0xff, 0x66, 0x66, 0xff, 0x7c, 0x18,

    /* U+F015 "" */
    0xb, 0x1b, 0x9a, 0xdf, 0xf7, 0xf3, 0xb9, 0xdc,

    /* U+F019 "" */
    0x18, 0xc, 0x6, 0xf, 0xc3, 0xc7, 0xfb, 0xfb,
    0xff,

    /* U+F01C "" */
    0x3e, 0x20, 0xb0, 0x7c, 0x7f, 0xff, 0xfc,

    /* U+F021 "" */
    0x3d, 0x43, 0xc7, 0x0, 0x0, 0xe3, 0xc6, 0xbc,

    /* U+F026 "" */
    0x3, 0xff, 0xf3, 0x0,

    /* U+F027 "" */
    0x0, 0xcf, 0x7d, 0xf0, 0xc0, 0x0,

    /* U+F028 "" */
    0x2, 0x0, 0x8c, 0xbe, 0xdf, 0x6f, 0x94, 0xc4,
    0x4,

    /* U+F03A "" */
    0xdf, 0x80, 0x37, 0xe0, 0x0, 0x6, 0xfc,

    /* U+F03E "" */
    0xff, 0x9f, 0xfb, 0xd1, 0x81, 0xff,

    /* U+F043 "" */
    0x20, 0xc7, 0x1e, 0xfa, 0xed, 0x9c,

    /* U+F048 "" */
    0x8c, 0xff, 0xff, 0xce, 0x20,

    /* U+F04B "" */
    0xc1, 0xe3, 0xe7, 0xff, 0xff, 0x38, 0x60,

    /* U+F04C "" */
    0xef, 0xdf, 0xbf, 0x7e, 0xfd, 0xfb, 0x80,

    /* U+F04D "" */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80,

    /* U+F051 "" */
    0x8e, 0x7f, 0xff, 0xe6, 0x20,

    /* U+F052 "" */
    0x10, 0x71, 0xf7, 0xf0, 0x1f, 0xff, 0x80,

    /* U+F053 "" */
    0x11, 0x99, 0x86, 0x18, 0x40,

    /* U+F054 "" */
    0x43, 0xc, 0x33, 0x31, 0x0,

    /* U+F067 "" */
    0x10, 0x20, 0x47, 0xf1, 0x2, 0x4, 0x0,

    /* U+F068 "" */
    0xfe,

    /* U+F06E "" */
    0x3e, 0x31, 0xb7, 0x7b, 0xb6, 0x31, 0xf0,

    /* U+F070 "" */
    0x40, 0xd, 0xe0, 0x76, 0x37, 0x66, 0x6c, 0x63,
    0x7, 0x30, 0x2,

    /* U+F071 "" */
    0x8, 0xe, 0x7, 0x6, 0xc7, 0x73, 0xfb, 0xdf,
    0xff,

    /* U+F074 "" */
    0x2, 0xef, 0x3a, 0x3a, 0xef, 0x2,

    /* U+F077 "" */
    0x10, 0x71, 0xb6, 0x30,

    /* U+F078 "" */
    0x82, 0xd8, 0xe0, 0x80,

    /* U+F079 "" */
    0x40, 0x77, 0xb8, 0x48, 0x24, 0x3b, 0xdc, 0x4,

    /* U+F07B "" */
    0xf0, 0xff, 0xff, 0xff, 0xff, 0xff,

    /* U+F093 "" */
    0x8, 0xe, 0xf, 0x83, 0x81, 0xc7, 0xfb, 0xfb,
    0xff,

    /* U+F095 "" */
    0x3, 0x7, 0x7, 0x2, 0x6, 0x6c, 0xf8, 0xe0,

    /* U+F0C4 "" */
    0xe7, 0x5b, 0xe1, 0x8f, 0x95, 0xb9, 0x80,

    /* U+F0C5 "" */
    0x3d, 0xf3, 0xff, 0xff, 0xff, 0xf7, 0xfc,

    /* U+F0C6 "" */
    0xc, 0x24, 0xaa, 0xaa, 0xf7, 0xa6, 0x38,

    /* U+F0C7 "" */
    0xfd, 0xe, 0x1f, 0xfe, 0xfd, 0xff, 0x80,

    /* U+F0C9 "" */
    0xff, 0xfc, 0x7, 0xf0, 0x0, 0x3f, 0x80,

    /* U+F0E0 "" */
    0xff, 0xff, 0xbc, 0xdb, 0xe7, 0xff,

    /* U+F0E7 "" */
    0xe7, 0x3f, 0xf3, 0x10, 0x80
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 128, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 128, .box_w = 1, .box_h = 7, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 2, .adv_w = 128, .box_w = 4, .box_h = 3, .ofs_x = 2, .ofs_y = 3},
    {.bitmap_index = 4, .adv_w = 128, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 9, .adv_w = 128, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 14, .adv_w = 128, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 21, .adv_w = 128, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 27, .adv_w = 128, .box_w = 2, .box_h = 3, .ofs_x = 3, .ofs_y = 3},
    {.bitmap_index = 28, .adv_w = 128, .box_w = 2, .box_h = 7, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 30, .adv_w = 128, .box_w = 2, .box_h = 7, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 32, .adv_w = 128, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 36, .adv_w = 128, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 40, .adv_w = 128, .box_w = 2, .box_h = 3, .ofs_x = 3, .ofs_y = -1},
    {.bitmap_index = 41, .adv_w = 128, .box_w = 4, .box_h = 1, .ofs_x = 2, .ofs_y = 2},
    {.bitmap_index = 42, .adv_w = 128, .box_w = 1, .box_h = 1, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 43, .adv_w = 128, .box_w = 4, .box_h = 8, .ofs_x = 2, .ofs_y = -1},
    {.bitmap_index = 47, .adv_w = 128, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 52, .adv_w = 128, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 57, .adv_w = 128, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 62, .adv_w = 128, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 67, .adv_w = 128, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 72, .adv_w = 128, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 77, .adv_w = 128, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 82, .adv_w = 128, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 87, .adv_w = 128, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 92, .adv_w = 128, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 97, .adv_w = 128, .box_w = 1, .box_h = 4, .ofs_x = 3, .ofs_y = 1},
    {.bitmap_index = 98, .adv_w = 128, .box_w = 2, .box_h = 5, .ofs_x = 3, .ofs_y = -1},
    {.bitmap_index = 100, .adv_w = 128, .box_w = 3, .box_h = 5, .ofs_x = 2, .ofs_y = 1},
    {.bitmap_index = 102, .adv_w = 128, .box_w = 6, .box_h = 3, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 105, .adv_w = 128, .box_w = 3, .box_h = 5, .ofs_x = 3, .ofs_y = 1},
    {.bitmap_index = 107, .adv_w = 128, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 112, .adv_w = 128, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 119, .adv_w = 128, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 124, .adv_w = 128, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 129, .adv_w = 128, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 134, .adv_w = 128, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 140, .adv_w = 128, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 145, .adv_w = 128, .box_w = 5, .box_h = 7, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 150, .adv_w = 128, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 155, .adv_w = 128, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 160, .adv_w = 128, .box_w = 3, .box_h = 7, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 163, .adv_w = 128, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 167, .adv_w = 128, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 172, .adv_w = 128, .box_w = 4, .box_h = 7, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 176, .adv_w = 128, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 182, .adv_w = 128, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 187, .adv_w = 128, .box_w = 5, .box_h = 7, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 192, .adv_w = 128, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 197, .adv_w = 128, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 202, .adv_w = 128, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 207, .adv_w = 128, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 212, .adv_w = 128, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 217, .adv_w = 128, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 222, .adv_w = 128, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 227, .adv_w = 128, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 234, .adv_w = 128, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 239, .adv_w = 128, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 244, .adv_w = 128, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 249, .adv_w = 128, .box_w = 2, .box_h = 7, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 251, .adv_w = 128, .box_w = 4, .box_h = 8, .ofs_x = 2, .ofs_y = -1},
    {.bitmap_index = 255, .adv_w = 128, .box_w = 2, .box_h = 7, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 257, .adv_w = 128, .box_w = 4, .box_h = 2, .ofs_x = 2, .ofs_y = 4},
    {.bitmap_index = 258, .adv_w = 128, .box_w = 6, .box_h = 1, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 259, .adv_w = 128, .box_w = 2, .box_h = 3, .ofs_x = 3, .ofs_y = 3},
    {.bitmap_index = 260, .adv_w = 128, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 264, .adv_w = 128, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 269, .adv_w = 128, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 273, .adv_w = 128, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 278, .adv_w = 128, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 282, .adv_w = 128, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 287, .adv_w = 128, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 292, .adv_w = 128, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 297, .adv_w = 128, .box_w = 3, .box_h = 7, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 300, .adv_w = 128, .box_w = 3, .box_h = 7, .ofs_x = 2, .ofs_y = -1},
    {.bitmap_index = 303, .adv_w = 128, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 307, .adv_w = 128, .box_w = 3, .box_h = 6, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 310, .adv_w = 128, .box_w = 7, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 316, .adv_w = 128, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 320, .adv_w = 128, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 324, .adv_w = 128, .box_w = 5, .box_h = 7, .ofs_x = 2, .ofs_y = -1},
    {.bitmap_index = 329, .adv_w = 128, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 334, .adv_w = 128, .box_w = 4, .box_h = 6, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 337, .adv_w = 128, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 341, .adv_w = 128, .box_w = 4, .box_h = 7, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 345, .adv_w = 128, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 349, .adv_w = 128, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 353, .adv_w = 128, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 358, .adv_w = 128, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 362, .adv_w = 128, .box_w = 4, .box_h = 6, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 365, .adv_w = 128, .box_w = 4, .box_h = 6, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 368, .adv_w = 128, .box_w = 4, .box_h = 8, .ofs_x = 2, .ofs_y = -1},
    {.bitmap_index = 372, .adv_w = 128, .box_w = 1, .box_h = 8, .ofs_x = 3, .ofs_y = -1},
    {.bitmap_index = 373, .adv_w = 128, .box_w = 4, .box_h = 8, .ofs_x = 2, .ofs_y = -1},
    {.bitmap_index = 377, .adv_w = 128, .box_w = 6, .box_h = 2, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 379, .adv_w = 128, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 380, .adv_w = 128, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 388, .adv_w = 128, .box_w = 9, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 395, .adv_w = 128, .box_w = 8, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 401, .adv_w = 88, .box_w = 6, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 405, .adv_w = 128, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 412, .adv_w = 128, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 420, .adv_w = 144, .box_w = 9, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 428, .adv_w = 128, .box_w = 9, .box_h = 8, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 437, .adv_w = 144, .box_w = 9, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 444, .adv_w = 128, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 452, .adv_w = 64, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 456, .adv_w = 96, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 462, .adv_w = 144, .box_w = 9, .box_h = 8, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 471, .adv_w = 128, .box_w = 9, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 478, .adv_w = 128, .box_w = 8, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 484, .adv_w = 88, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 490, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 495, .adv_w = 112, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 502, .adv_w = 112, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 509, .adv_w = 112, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 516, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 521, .adv_w = 112, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 528, .adv_w = 80, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 533, .adv_w = 80, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 538, .adv_w = 112, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 545, .adv_w = 112, .box_w = 7, .box_h = 1, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 546, .adv_w = 144, .box_w = 9, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 553, .adv_w = 160, .box_w = 11, .box_h = 8, .ofs_x = -1, .ofs_y = -1},
    {.bitmap_index = 564, .adv_w = 144, .box_w = 9, .box_h = 8, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 573, .adv_w = 128, .box_w = 8, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 579, .adv_w = 112, .box_w = 7, .box_h = 4, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 583, .adv_w = 112, .box_w = 7, .box_h = 4, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 587, .adv_w = 160, .box_w = 9, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 595, .adv_w = 128, .box_w = 8, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 601, .adv_w = 128, .box_w = 9, .box_h = 8, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 610, .adv_w = 128, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 618, .adv_w = 112, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 625, .adv_w = 112, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 632, .adv_w = 112, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 639, .adv_w = 112, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 646, .adv_w = 112, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 653, .adv_w = 128, .box_w = 8, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 659, .adv_w = 80, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = -1}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint16_t unicode_list_1[] = {
    0x0, 0x7, 0xb, 0xc, 0x10, 0x12, 0x14, 0x18,
    0x1b, 0x20, 0x25, 0x26, 0x27, 0x39, 0x3d, 0x42,
    0x47, 0x4a, 0x4b, 0x4c, 0x50, 0x51, 0x52, 0x53,
    0x66, 0x67, 0x6d, 0x6f, 0x70, 0x73, 0x76, 0x77,
    0x78, 0x7a, 0x92, 0x94, 0xc3, 0xc4, 0xc5, 0xc6,
    0xc8, 0xdf, 0xe6
};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 32, .range_length = 96, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 61441, .range_length = 231, .glyph_id_start = 97,
        .unicode_list = unicode_list_1, .glyph_id_ofs_list = NULL, .list_length = 43, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
    }
};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 2,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif

};

extern const lv_font_t lv_font_dogica_8;


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t lv_font_dogica_8 = {
#else
lv_font_t lv_font_dogica_8 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 8,          /*The maximum line height required by the font*/
    .base_line = 1,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = 1,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if LV_FONT_DOGICA_8*/
