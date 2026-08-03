/*******************************************************************************
 * Size: 7 px
 * Bpp: 1
 * Opts: --bpp 1 --size 7 --no-compress --stride 1 --align 1 --font 4.ttf --range 32-127 --font FontAwesome5-Solid+Brands+Regular.woff --range 61441,61448,61451,61452,61453,61457,61459,61461,61465,61468,61473,61478,61479,61480,61502,61507,61512,61515,61516,61517,61521,61522,61523,61524,61543,61544,61550,61552,61553,61556,61559,61560,61561,61563,61587,61589,61636,61637,61639,61641,61664,61671,61674,61683,61724,61732,61787,61931,62016,62017,62018,62019,62020,62087,62099,62189,62212,62810,63426,63650 --font MontserratMedium.ttf --range 95 --format lvgl -o lv_font_heltec_7.c
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



#ifndef LV_FONT_HELTEC_7
#define LV_FONT_HELTEC_7 0
#endif

#if LV_FONT_HELTEC_7

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */
    0x0,

    /* U+0021 "!" */
    0xe8,

    /* U+0022 "\"" */
    0xb4,

    /* U+0023 "#" */
    0x57, 0xd5, 0xf5, 0x0,

    /* U+0024 "$" */
    0x27, 0xe9, 0xf2, 0xfc, 0x80,

    /* U+0025 "%" */
    0xce, 0x88, 0xb9, 0x80,

    /* U+0026 "&" */
    0x64, 0x22, 0xe9, 0x3c,

    /* U+0027 "'" */
    0xc0,

    /* U+0028 "(" */
    0x6a, 0x40,

    /* U+0029 ")" */
    0x95, 0x80,

    /* U+002A "*" */
    0x25, 0x6a, 0xea, 0xd4, 0x80,

    /* U+002B "+" */
    0x21, 0x3e, 0x42, 0x0,

    /* U+002C "," */
    0x80,

    /* U+002D "-" */
    0xe0,

    /* U+002E "." */
    0x80,

    /* U+002F "/" */
    0x8, 0x88, 0x88, 0x0,

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

    /* U+003A ":" */
    0xa0,

    /* U+003B ";" */
    0x46,

    /* U+003C "<" */
    0x2a, 0x22,

    /* U+003D "=" */
    0xe3, 0x80,

    /* U+003E ">" */
    0x88, 0xa8,

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

    /* U+005B "[" */
    0xea, 0xc0,

    /* U+005C "\\" */
    0x82, 0x8, 0x20, 0x80,

    /* U+005D "]" */
    0xd5, 0xc0,

    /* U+005E "^" */
    0xfc, 0x63, 0x1f, 0x80,

    /* U+005F "_" */
    0xf0,

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

    /* U+F00B "" */
    0xff, 0xff, 0x7f, 0xfd, 0xff, 0xff, 0x80,

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

    /* U+F0C7 "" */
    0xfa, 0x38, 0xff, 0xcf, 0xf0,

    /* U+F0C9 "" */
    0xfc, 0xf, 0xc0, 0xfc,

    /* U+F0E0 "" */
    0xff, 0xfe, 0xee, 0xbf, 0xe0,

    /* U+F0E7 "" */
    0xee, 0xff, 0x22, 0x40,

    /* U+F0EA "" */
    0xfb, 0xed, 0xbf, 0xff, 0xf3, 0xc0,

    /* U+F0F3 "" */
    0x21, 0xe7, 0x9e, 0xff, 0xf3, 0x0,

    /* U+F11C "" */
    0xff, 0xed, 0xff, 0xf7, 0xcd, 0xff,

    /* U+F124 "" */
    0x6, 0x3f, 0xf7, 0xe1, 0x83, 0x4, 0x0,

    /* U+F15B "" */
    0xf7, 0xff, 0xff, 0xff, 0xe0,

    /* U+F1EB "" */
    0x3e, 0x60, 0xcf, 0x4, 0x40, 0x80, 0x40,

    /* U+F240 "" */
    0xff, 0x7f, 0x7f, 0xb0, 0x1f, 0xf0,

    /* U+F241 "" */
    0xff, 0x7e, 0x7f, 0x30, 0x1f, 0xf0,

    /* U+F242 "" */
    0xff, 0x7c, 0x7e, 0x30, 0x1f, 0xf0,

    /* U+F243 "" */
    0xff, 0x70, 0x78, 0x30, 0x1f, 0xf0,

    /* U+F244 "" */
    0xff, 0x40, 0x60, 0x30, 0x1f, 0xf0,

    /* U+F287 "" */
    0x1c, 0x30, 0xbf, 0xea, 0x20, 0xe0,

    /* U+F293 "" */
    0x76, 0xeb, 0xba, 0xed, 0xc0,

    /* U+F2ED "" */
    0xfc, 0xf, 0xff, 0xff, 0xff, 0xc0,

    /* U+F304 "" */
    0x4, 0xc, 0x61, 0xc7, 0x1c, 0x30, 0x0,

    /* U+F55A "" */
    0x3f, 0xba, 0xfe, 0xee, 0xb3, 0xf8,

    /* U+F7C2 "" */
    0x7b, 0xdf, 0xff, 0xff, 0xff, 0x80,

    /* U+F8A2 "" */
    0x2, 0x87, 0xfa, 0x0
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 40, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 48, .box_w = 1, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2, .adv_w = 64, .box_w = 3, .box_h = 2, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 3, .adv_w = 112, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 7, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 12, .adv_w = 112, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 16, .adv_w = 112, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 20, .adv_w = 48, .box_w = 1, .box_h = 2, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 21, .adv_w = 64, .box_w = 2, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 23, .adv_w = 64, .box_w = 2, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 25, .adv_w = 112, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 30, .adv_w = 112, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 34, .adv_w = 48, .box_w = 1, .box_h = 1, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 35, .adv_w = 80, .box_w = 3, .box_h = 1, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 36, .adv_w = 48, .box_w = 1, .box_h = 1, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 37, .adv_w = 112, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 41, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 45, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 49, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 53, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 57, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 61, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 65, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 69, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 73, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 77, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 81, .adv_w = 48, .box_w = 1, .box_h = 3, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 82, .adv_w = 64, .box_w = 2, .box_h = 4, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 83, .adv_w = 80, .box_w = 3, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 85, .adv_w = 80, .box_w = 3, .box_h = 3, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 87, .adv_w = 80, .box_w = 3, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 89, .adv_w = 96, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 92, .adv_w = 112, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 97, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 101, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 105, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 109, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 113, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 117, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 121, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 125, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 129, .adv_w = 64, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 132, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 136, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 140, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 144, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 148, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 152, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 156, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 160, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 164, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 168, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 172, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 176, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 180, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 184, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 188, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 192, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 196, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 200, .adv_w = 64, .box_w = 2, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 202, .adv_w = 112, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 206, .adv_w = 64, .box_w = 2, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 208, .adv_w = 112, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 212, .adv_w = 56, .box_w = 4, .box_h = 1, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 213, .adv_w = 96, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 217, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 221, .adv_w = 96, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 225, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 229, .adv_w = 96, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 233, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 237, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 241, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 245, .adv_w = 48, .box_w = 1, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 246, .adv_w = 48, .box_w = 2, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 248, .adv_w = 80, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 251, .adv_w = 32, .box_w = 1, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 252, .adv_w = 96, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 256, .adv_w = 96, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 260, .adv_w = 96, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 264, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 268, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 272, .adv_w = 96, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 276, .adv_w = 96, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 280, .adv_w = 64, .box_w = 3, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 283, .adv_w = 96, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 287, .adv_w = 96, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 291, .adv_w = 96, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 295, .adv_w = 96, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 299, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 303, .adv_w = 96, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 307, .adv_w = 112, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 314, .adv_w = 112, .box_w = 7, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 319, .adv_w = 112, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 326, .adv_w = 112, .box_w = 7, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 331, .adv_w = 77, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 335, .adv_w = 112, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 342, .adv_w = 112, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 349, .adv_w = 126, .box_w = 8, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 355, .adv_w = 112, .box_w = 8, .box_h = 7, .ofs_x = -1, .ofs_y = -1},
    {.bitmap_index = 362, .adv_w = 126, .box_w = 8, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 367, .adv_w = 112, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 374, .adv_w = 56, .box_w = 4, .box_h = 4, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 376, .adv_w = 84, .box_w = 6, .box_h = 4, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 379, .adv_w = 126, .box_w = 8, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 386, .adv_w = 112, .box_w = 7, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 391, .adv_w = 77, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 396, .adv_w = 98, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 399, .adv_w = 98, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 405, .adv_w = 98, .box_w = 7, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 411, .adv_w = 98, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 416, .adv_w = 98, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 419, .adv_w = 98, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 424, .adv_w = 70, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 427, .adv_w = 70, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 430, .adv_w = 98, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 435, .adv_w = 98, .box_w = 6, .box_h = 1, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 436, .adv_w = 126, .box_w = 8, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 441, .adv_w = 140, .box_w = 8, .box_h = 7, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 448, .adv_w = 126, .box_w = 8, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 455, .adv_w = 112, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 462, .adv_w = 98, .box_w = 6, .box_h = 3, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 465, .adv_w = 98, .box_w = 6, .box_h = 3, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 468, .adv_w = 140, .box_w = 8, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 473, .adv_w = 112, .box_w = 7, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 478, .adv_w = 112, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 485, .adv_w = 112, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 492, .adv_w = 98, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 497, .adv_w = 98, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 503, .adv_w = 98, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 508, .adv_w = 98, .box_w = 6, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 512, .adv_w = 112, .box_w = 7, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 517, .adv_w = 70, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 521, .adv_w = 98, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 527, .adv_w = 98, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 533, .adv_w = 126, .box_w = 8, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 539, .adv_w = 112, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 546, .adv_w = 84, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 551, .adv_w = 140, .box_w = 9, .box_h = 6, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 558, .adv_w = 140, .box_w = 9, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 564, .adv_w = 140, .box_w = 9, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 570, .adv_w = 140, .box_w = 9, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 576, .adv_w = 140, .box_w = 9, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 582, .adv_w = 140, .box_w = 9, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 588, .adv_w = 140, .box_w = 9, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 594, .adv_w = 98, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 599, .adv_w = 98, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 605, .adv_w = 112, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 612, .adv_w = 140, .box_w = 9, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 618, .adv_w = 84, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 624, .adv_w = 113, .box_w = 7, .box_h = 4, .ofs_x = 0, .ofs_y = 1}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint16_t unicode_list_2[] = {
    0x0, 0x7, 0xa, 0xb, 0xc, 0x10, 0x12, 0x14,
    0x18, 0x1b, 0x20, 0x25, 0x26, 0x27, 0x3d, 0x42,
    0x47, 0x4a, 0x4b, 0x4c, 0x50, 0x51, 0x52, 0x53,
    0x66, 0x67, 0x6d, 0x6f, 0x70, 0x73, 0x76, 0x77,
    0x78, 0x7a, 0x92, 0x94, 0xc3, 0xc4, 0xc6, 0xc8,
    0xdf, 0xe6, 0xe9, 0xf2, 0x11b, 0x123, 0x15a, 0x1ea,
    0x23f, 0x240, 0x241, 0x242, 0x243, 0x286, 0x292, 0x2ec,
    0x303, 0x559, 0x7c1, 0x8a1
};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 32, .range_length = 64, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 97, .range_length = 26, .glyph_id_start = 65,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 61441, .range_length = 2210, .glyph_id_start = 91,
        .unicode_list = unicode_list_2, .glyph_id_ofs_list = NULL, .list_length = 60, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
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
    .cmap_num = 3,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif

};

extern const lv_font_t lv_font_heltec_7;


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t lv_font_heltec_7 = {
#else
lv_font_t lv_font_heltec_7 = {
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
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if LV_FONT_HELTEC_7*/
