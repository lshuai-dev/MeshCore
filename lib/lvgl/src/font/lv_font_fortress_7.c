/*******************************************************************************
 * Size: 7 px
 * Bpp: 1
 * Opts: --bpp 1 --size 7 --no-compress --stride 1 --align 1 --font 8-bit-fortress.ttf --range 32-127 --font FontAwesome5-Solid+Brands+Regular.woff --range 61441,61448,61452,61453,61457,61459,61461,61465,61468,61473,61478-61480,61498,61502,61507,61512,61515-61517,61521-61524,61543-61544,61550,61552-61553,61556,61559-61561,61563,61587,61589,61636-61639,61641,61664,61671 --format lvgl -o lv_font_fortress_7.c
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



#ifndef LV_FONT_FORTRESS_7
#define LV_FONT_FORTRESS_7 0
#endif

#if LV_FONT_FORTRESS_7

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */
    0x0,

    /* U+0021 "!" */
    0xf4,

    /* U+0022 "\"" */
    0xb6, 0x80,

    /* U+0023 "#" */
    0x57, 0xd5, 0xf5, 0x28,

    /* U+0024 "$" */
    0x23, 0xe8, 0xe2, 0x97, 0xc4,

    /* U+0025 "%" */
    0xc7, 0x21, 0x8, 0x4e, 0x30,

    /* U+0026 "&" */
    0xd2, 0xdb, 0xd2,

    /* U+0027 "'" */
    0xe0,

    /* U+0028 "(" */
    0x6a, 0xa4,

    /* U+0029 ")" */
    0x95, 0x58,

    /* U+002A "*" */
    0xaa, 0x80,

    /* U+002B "+" */
    0x5d, 0x0,

    /* U+002C "," */
    0x60,

    /* U+002E "." */
    0x80,

    /* U+0030 "0" */
    0x74, 0x63, 0x18, 0xb8,

    /* U+0031 "1" */
    0x23, 0x28, 0x42, 0x7c,

    /* U+0032 "2" */
    0x74, 0x42, 0x64, 0x7c,

    /* U+0033 "3" */
    0x74, 0x4c, 0x18, 0xb8,

    /* U+0034 "4" */
    0x11, 0x95, 0xf1, 0x8,

    /* U+0035 "5" */
    0xfc, 0x3c, 0x18, 0xb8,

    /* U+0036 "6" */
    0x7c, 0x3d, 0x18, 0xb8,

    /* U+0037 "7" */
    0xf8, 0x44, 0x42, 0x10,

    /* U+0038 "8" */
    0x74, 0x5d, 0x18, 0xb8,

    /* U+0039 "9" */
    0x74, 0x62, 0xf0, 0xb8,

    /* U+003F "?" */
    0xe1, 0x16, 0x4,

    /* U+0040 "@" */
    0xf8, 0x1e, 0x45, 0x75, 0xe0,

    /* U+0041 "A" */
    0x74, 0x63, 0xf8, 0xc4,

    /* U+0042 "B" */
    0xf4, 0x7d, 0x18, 0xf8,

    /* U+0043 "C" */
    0x7c, 0x21, 0x8, 0x3c,

    /* U+0044 "D" */
    0xf4, 0x63, 0x18, 0xf8,

    /* U+0045 "E" */
    0xfc, 0x3d, 0x8, 0x7c,

    /* U+0046 "F" */
    0xfc, 0x21, 0xe8, 0x40,

    /* U+0047 "G" */
    0x7c, 0x2f, 0x18, 0xb8,

    /* U+0048 "H" */
    0x8c, 0x7f, 0x18, 0xc4,

    /* U+0049 "I" */
    0xe9, 0x25, 0xc0,

    /* U+004A "J" */
    0x8, 0x42, 0x18, 0xb8,

    /* U+004B "K" */
    0x8c, 0xb9, 0x28, 0xc4,

    /* U+004C "L" */
    0x84, 0x21, 0x8, 0x7c,

    /* U+004D "M" */
    0x8e, 0xeb, 0x18, 0xc4,

    /* U+004E "N" */
    0x8e, 0x6b, 0x38, 0xc4,

    /* U+004F "O" */
    0x74, 0x63, 0x18, 0xb8,

    /* U+0050 "P" */
    0xf4, 0x63, 0xe8, 0x40,

    /* U+0051 "Q" */
    0x74, 0x63, 0x57, 0x4,

    /* U+0052 "R" */
    0xf4, 0x7d, 0x18, 0xc4,

    /* U+0053 "S" */
    0x7c, 0x1c, 0x10, 0xf8,

    /* U+0054 "T" */
    0xf9, 0x8, 0x42, 0x10,

    /* U+0055 "U" */
    0x8c, 0x63, 0x18, 0xb8,

    /* U+0056 "V" */
    0x8c, 0x63, 0x15, 0x10,

    /* U+0057 "W" */
    0x8c, 0x6b, 0x5a, 0xa8,

    /* U+0058 "X" */
    0x8a, 0x88, 0xa8, 0xc4,

    /* U+0059 "Y" */
    0x8a, 0x88, 0x42, 0x10,

    /* U+005A "Z" */
    0xf8, 0x44, 0xc8, 0x7c,

    /* U+005F "_" */
    0xf8,

    /* U+0061 "a" */
    0xf0, 0x5f, 0x17, 0x80,

    /* U+0062 "b" */
    0x84, 0x3d, 0x18, 0xf8,

    /* U+0063 "c" */
    0x7c, 0x21, 0x7, 0x80,

    /* U+0064 "d" */
    0x8, 0x5f, 0x18, 0xbc,

    /* U+0065 "e" */
    0x74, 0x7f, 0x7, 0x0,

    /* U+0066 "f" */
    0x3a, 0x11, 0xe4, 0x20,

    /* U+0067 "g" */
    0x7c, 0x62, 0xf0, 0xb8,

    /* U+0068 "h" */
    0x84, 0x3d, 0x18, 0xc4,

    /* U+0069 "i" */
    0xbc,

    /* U+006A "j" */
    0x45, 0x58,

    /* U+006B "k" */
    0x89, 0xac, 0xa9,

    /* U+006C "l" */
    0xfc,

    /* U+006D "m" */
    0xd5, 0x6b, 0x5a, 0x80,

    /* U+006E "n" */
    0xf4, 0x63, 0x18, 0x80,

    /* U+006F "o" */
    0x74, 0x63, 0x17, 0x0,

    /* U+0070 "p" */
    0xf4, 0x63, 0xe8, 0x40,

    /* U+0071 "q" */
    0x7c, 0x62, 0xf0, 0x84,

    /* U+0072 "r" */
    0xbe, 0x21, 0x8, 0x0,

    /* U+0073 "s" */
    0x74, 0x1c, 0x1f, 0x0,

    /* U+0074 "t" */
    0x9e, 0x48, 0xc0,

    /* U+0075 "u" */
    0x8c, 0x63, 0x17, 0x0,

    /* U+0076 "v" */
    0x8c, 0x62, 0xa2, 0x0,

    /* U+0077 "w" */
    0xad, 0x6b, 0x5d, 0x0,

    /* U+0078 "x" */
    0x8a, 0x88, 0xa8, 0x80,

    /* U+0079 "y" */
    0x8c, 0x62, 0xf0, 0xb8,

    /* U+007A "z" */
    0xf8, 0x88, 0x8f, 0x80,

    /* U+F001 "" */
    0xe, 0x7c, 0xc9, 0x12, 0x7d, 0xf8, 0x0,

    /* U+F008 "" */
    0xff, 0x8e, 0xfe, 0x3f, 0xe0,

    /* U+F00C "" */
    0x6, 0x1b, 0x63, 0x82, 0x0,

    /* U+F00D "" */
    0x8a, 0x8c, 0xa8, 0x80,

    /* U+F011 "" */
    0x10, 0xaa, 0x4c, 0x98, 0x28, 0x8e, 0x0,

    /* U+F013 "" */
    0x18, 0xff, 0xdb, 0x3f, 0xef, 0xc6, 0x0,

    /* U+F015 "" */
    0x1a, 0x26, 0x5a, 0xff, 0x66, 0x66,

    /* U+F019 "" */
    0x18, 0x18, 0x3c, 0x3c, 0xe7, 0xfd, 0xff,

    /* U+F01C "" */
    0x7e, 0x42, 0xe7, 0xff, 0xff,

    /* U+F021 "" */
    0x3a, 0x8e, 0x38, 0xe, 0x38, 0xae, 0x0,

    /* U+F026 "" */
    0x1f, 0xf1,

    /* U+F027 "" */
    0x13, 0xef, 0x44,

    /* U+F028 "" */
    0x2, 0x15, 0xfb, 0xfb, 0x13, 0x5, 0x2,

    /* U+F03A "" */
    0xbe, 0x2, 0xf8, 0xb, 0xe0,

    /* U+F03E "" */
    0xff, 0x6f, 0x8c, 0x1f, 0xe0,

    /* U+F043 "" */
    0x23, 0x1d, 0xff, 0xdd, 0xc0,

    /* U+F048 "" */
    0x9b, 0xff, 0xb9,

    /* U+F04B "" */
    0xc3, 0xcf, 0xbf, 0xfb, 0x88, 0x0,

    /* U+F04C "" */
    0xed, 0xdf, 0xbf, 0x7e, 0xfd, 0x80,

    /* U+F04D "" */
    0xff, 0xff, 0xff, 0xff, 0xf0,

    /* U+F051 "" */
    0x9d, 0xff, 0xd9,

    /* U+F052 "" */
    0x31, 0xef, 0xdf, 0xff, 0xf0,

    /* U+F053 "" */
    0x12, 0x4c, 0x21,

    /* U+F054 "" */
    0xc6, 0x33, 0x6c,

    /* U+F067 "" */
    0x30, 0xcf, 0xff, 0x30, 0xc0,

    /* U+F068 "" */
    0xfc,

    /* U+F06E "" */
    0x3c, 0x6e, 0xdb, 0x66, 0x3c,

    /* U+F070 "" */
    0x0, 0xfc, 0xbe, 0xdb, 0x6e, 0x3a, 0x1,

    /* U+F071 "" */
    0x18, 0x18, 0x2c, 0x2c, 0x7e, 0xe6, 0xff,

    /* U+F074 "" */
    0x5, 0x9d, 0x51, 0x83, 0x59, 0xc1, 0x0,

    /* U+F077 "" */
    0x31, 0x28, 0x40,

    /* U+F078 "" */
    0xcd, 0xe3, 0x0,

    /* U+F079 "" */
    0x5e, 0xe2, 0x42, 0x47, 0x7a,

    /* U+F07B "" */
    0xe1, 0xff, 0xff, 0xff, 0xe0,

    /* U+F093 "" */
    0x10, 0x71, 0xf1, 0x83, 0x1a, 0xff, 0xff,

    /* U+F095 "" */
    0x6, 0x1c, 0x18, 0x24, 0xdf, 0x38, 0x0,

    /* U+F0C4 "" */
    0x42, 0xbf, 0x8c, 0xfb, 0x10,

    /* U+F0C5 "" */
    0x7f, 0xef, 0xff, 0xfe, 0xff, 0x80,

    /* U+F0C6 "" */
    0x1c, 0xd7, 0xfb, 0xda, 0xc6, 0x0,

    /* U+F0C7 "" */
    0xfa, 0x38, 0xff, 0xcf, 0xf0,

    /* U+F0C9 "" */
    0xfc, 0xf, 0xc0, 0xfc,

    /* U+F0E0 "" */
    0xff, 0xfe, 0xee, 0xbf, 0xe0,

    /* U+F0E7 "" */
    0xee, 0xff, 0x22, 0x40
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 40, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 32, .box_w = 1, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 2, .adv_w = 64, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 4, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 8, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 13, .adv_w = 112, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 18, .adv_w = 160, .box_w = 8, .box_h = 3, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 21, .adv_w = 32, .box_w = 1, .box_h = 3, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 22, .adv_w = 64, .box_w = 2, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 24, .adv_w = 64, .box_w = 2, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 26, .adv_w = 80, .box_w = 3, .box_h = 3, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 28, .adv_w = 80, .box_w = 3, .box_h = 3, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 30, .adv_w = 64, .box_w = 2, .box_h = 2, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 31, .adv_w = 32, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 32, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 36, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 40, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 44, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 48, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 52, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 56, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 60, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 64, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 68, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 72, .adv_w = 80, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 75, .adv_w = 112, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 80, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 84, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 88, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 92, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 96, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 100, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 104, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 108, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 112, .adv_w = 64, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 115, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 119, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 123, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 127, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 131, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 135, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 139, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 143, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 147, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 151, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 155, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 159, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 163, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 167, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 171, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 175, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 179, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 183, .adv_w = 96, .box_w = 5, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 184, .adv_w = 96, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 188, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 192, .adv_w = 96, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 196, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 200, .adv_w = 96, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 204, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 208, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 212, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 216, .adv_w = 48, .box_w = 1, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 217, .adv_w = 48, .box_w = 2, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 219, .adv_w = 80, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 222, .adv_w = 32, .box_w = 1, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 223, .adv_w = 96, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 227, .adv_w = 96, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 231, .adv_w = 96, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 235, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 239, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 243, .adv_w = 96, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 247, .adv_w = 96, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 251, .adv_w = 64, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 254, .adv_w = 96, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 258, .adv_w = 96, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 262, .adv_w = 96, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 266, .adv_w = 96, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 270, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 274, .adv_w = 96, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 278, .adv_w = 112, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 285, .adv_w = 112, .box_w = 7, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 290, .adv_w = 112, .box_w = 7, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 295, .adv_w = 77, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 299, .adv_w = 112, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 306, .adv_w = 112, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 313, .adv_w = 126, .box_w = 8, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 319, .adv_w = 112, .box_w = 8, .box_h = 7, .ofs_x = -1, .ofs_y = -1},
    {.bitmap_index = 326, .adv_w = 126, .box_w = 8, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 331, .adv_w = 112, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 338, .adv_w = 56, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 340, .adv_w = 84, .box_w = 6, .box_h = 4, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 343, .adv_w = 126, .box_w = 8, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 350, .adv_w = 112, .box_w = 7, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 355, .adv_w = 112, .box_w = 7, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 360, .adv_w = 77, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 365, .adv_w = 98, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 368, .adv_w = 98, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 374, .adv_w = 98, .box_w = 7, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 380, .adv_w = 98, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 385, .adv_w = 98, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 388, .adv_w = 98, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 393, .adv_w = 70, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 396, .adv_w = 70, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 399, .adv_w = 98, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 404, .adv_w = 98, .box_w = 6, .box_h = 1, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 405, .adv_w = 126, .box_w = 8, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 410, .adv_w = 140, .box_w = 8, .box_h = 7, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 417, .adv_w = 126, .box_w = 8, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 424, .adv_w = 112, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 431, .adv_w = 98, .box_w = 6, .box_h = 3, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 434, .adv_w = 98, .box_w = 6, .box_h = 3, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 437, .adv_w = 140, .box_w = 8, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 442, .adv_w = 112, .box_w = 7, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 447, .adv_w = 112, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 454, .adv_w = 112, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 461, .adv_w = 98, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 466, .adv_w = 98, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 472, .adv_w = 98, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 478, .adv_w = 98, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 483, .adv_w = 98, .box_w = 6, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 487, .adv_w = 112, .box_w = 7, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 492, .adv_w = 70, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = -1}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint8_t glyph_id_ofs_list_0[] = {
    0, 1, 2, 3, 4, 5, 6, 7,
    8, 9, 10, 11, 12, 0, 13, 0,
    14, 15, 16, 17, 18, 19, 20, 21,
    22, 23
};

static const uint16_t unicode_list_4[] = {
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
        .range_start = 32, .range_length = 26, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = glyph_id_ofs_list_0, .list_length = 26, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_FULL
    },
    {
        .range_start = 63, .range_length = 28, .glyph_id_start = 25,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 95, .range_length = 1, .glyph_id_start = 53,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 97, .range_length = 26, .glyph_id_start = 54,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 61441, .range_length = 231, .glyph_id_start = 80,
        .unicode_list = unicode_list_4, .glyph_id_ofs_list = NULL, .list_length = 43, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
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
    .cmap_num = 5,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif

};

extern const lv_font_t lv_font_fortress_7;
#if LV_FONT_UNSCII_8
LV_FONT_DECLARE(lv_font_unscii_8)
#endif


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t lv_font_fortress_7 = {
#else
lv_font_t lv_font_fortress_7 = {
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
    .underline_thickness = 0,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback =
#if LV_FONT_UNSCII_8
    &lv_font_unscii_8,
#else
    NULL,
#endif
#endif
};



#endif /*#if LV_FONT_FORTRESS_7*/
