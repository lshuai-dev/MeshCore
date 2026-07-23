/*******************************************************************************
 * Size: 12 px
 * Bpp: 1
 * Opts: --bpp 1 --size 12 --no-compress --stride 1 --align 1 --font arial.ttf --range 32-127 --font FontAwesome5-Solid+Brands+Regular.woff --range 61441,61448,61451,61452,61452,61453,61457,61459,61461,61465,61468,61473,61478,61479,61480,61502,61507,61512,61515,61516,61517,61521,61522,61523,61524,61543,61544,61550,61552,61553,61556,61559,61560,61561,61563,61587,61589,61636,61637,61639,61641,61664,61671,61674,61683,61724,61732,61784,61931,62016,62017,62018,62019,62020,62087,62099,62212,62189,62810,63426,63650 --format lvgl -o lv_font_arial_12.c
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



#ifndef LV_FONT_ARIAL_12
#define LV_FONT_ARIAL_12 0
#endif

#if LV_FONT_ARIAL_12

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */
    0x0,

    /* U+0021 "!" */
    0xfd,

    /* U+0022 "\"" */
    0xb6, 0x80,

    /* U+0023 "#" */
    0x24, 0x4b, 0xf9, 0x44, 0x9f, 0xd2, 0x28,

    /* U+0024 "$" */
    0x27, 0xeb, 0x47, 0x16, 0xb5, 0x71, 0x0,

    /* U+0025 "%" */
    0x62, 0x49, 0x25, 0xc, 0x80, 0xb8, 0x94, 0x4a,
    0x47,

    /* U+0026 "&" */
    0x30, 0x91, 0x21, 0x8d, 0x51, 0xa3, 0x39,

    /* U+0027 "'" */
    0xe0,

    /* U+0028 "(" */
    0x4a, 0x49, 0x24, 0x48,

    /* U+0029 ")" */
    0x88, 0x92, 0x49, 0x50,

    /* U+002A "*" */
    0x2f, 0x60,

    /* U+002B "+" */
    0x21, 0x3e, 0x42, 0x0,

    /* U+002C "," */
    0xc0,

    /* U+002D "-" */
    0xe0,

    /* U+002E "." */
    0x80,

    /* U+002F "/" */
    0x24, 0xa4, 0xa4,

    /* U+0030 "0" */
    0x7b, 0x38, 0x61, 0x86, 0x1c, 0xde,

    /* U+0031 "1" */
    0x2e, 0x92, 0x49,

    /* U+0032 "2" */
    0x79, 0x14, 0x43, 0x18, 0x84, 0x3f,

    /* U+0033 "3" */
    0x74, 0x42, 0x60, 0x86, 0x2e,

    /* U+0034 "4" */
    0x8, 0x62, 0x8a, 0x4b, 0xf0, 0x82,

    /* U+0035 "5" */
    0x7d, 0x7, 0x93, 0x7, 0x14, 0xde,

    /* U+0036 "6" */
    0x79, 0x38, 0x2e, 0xc6, 0x1c, 0xde,

    /* U+0037 "7" */
    0xfc, 0x21, 0x8, 0x21, 0x4, 0x10,

    /* U+0038 "8" */
    0x72, 0x28, 0x9c, 0xce, 0x18, 0x5e,

    /* U+0039 "9" */
    0x7b, 0x18, 0x61, 0x7c, 0x18, 0x9e,

    /* U+003A ":" */
    0x84,

    /* U+003B ";" */
    0x86,

    /* U+003C "<" */
    0x4, 0xec, 0x30, 0x38, 0x10,

    /* U+003D "=" */
    0xf8, 0x3e,

    /* U+003E ">" */
    0x81, 0x81, 0x86, 0x62, 0x0,

    /* U+003F "?" */
    0x74, 0x62, 0x33, 0x10, 0x4,

    /* U+0040 "@" */
    0x1f, 0x6, 0x19, 0x35, 0xc9, 0x9a, 0x13, 0x44,
    0x68, 0x92, 0xfc, 0x60, 0x63, 0xf0,

    /* U+0041 "A" */
    0x18, 0x18, 0x2c, 0x24, 0x64, 0x7e, 0x42, 0x83,

    /* U+0042 "B" */
    0xfa, 0x18, 0x7e, 0x86, 0x18, 0x7e,

    /* U+0043 "C" */
    0x3c, 0x8e, 0x4, 0x8, 0x10, 0x51, 0x9e,

    /* U+0044 "D" */
    0xf9, 0xa, 0xc, 0x18, 0x30, 0x61, 0x7c,

    /* U+0045 "E" */
    0xfe, 0x8, 0x3f, 0x82, 0x8, 0x3f,

    /* U+0046 "F" */
    0xfe, 0x8, 0x3e, 0x82, 0x8, 0x20,

    /* U+0047 "G" */
    0x3c, 0x43, 0x81, 0x80, 0x8f, 0x81, 0x41, 0x3e,

    /* U+0048 "H" */
    0x83, 0x6, 0xf, 0xf8, 0x30, 0x60, 0xc1,

    /* U+0049 "I" */
    0xff,

    /* U+004A "J" */
    0x8, 0x42, 0x10, 0x86, 0x2e,

    /* U+004B "K" */
    0x85, 0x12, 0x47, 0xd, 0x11, 0x23, 0x43,

    /* U+004C "L" */
    0x84, 0x21, 0x8, 0x42, 0x1f,

    /* U+004D "M" */
    0xc3, 0xc3, 0xc3, 0xa5, 0xa5, 0x99, 0x99, 0x99,

    /* U+004E "N" */
    0x83, 0x86, 0x8d, 0x99, 0xb1, 0x61, 0xc1,

    /* U+004F "O" */
    0x3c, 0x42, 0x81, 0x81, 0x81, 0x81, 0x42, 0x3c,

    /* U+0050 "P" */
    0xfa, 0x18, 0x61, 0xfa, 0x8, 0x20,

    /* U+0051 "Q" */
    0x3c, 0x42, 0x81, 0x81, 0x81, 0x81, 0x4e, 0x3f,

    /* U+0052 "R" */
    0xfc, 0x82, 0x82, 0xfc, 0x88, 0x84, 0x86, 0x82,

    /* U+0053 "S" */
    0x7a, 0x38, 0x18, 0x1e, 0x18, 0x5e,

    /* U+0054 "T" */
    0xfe, 0x20, 0x40, 0x81, 0x2, 0x4, 0x8,

    /* U+0055 "U" */
    0x83, 0x6, 0xc, 0x18, 0x30, 0x71, 0xbe,

    /* U+0056 "V" */
    0x81, 0x42, 0x42, 0x64, 0x24, 0x24, 0x18, 0x18,

    /* U+0057 "W" */
    0x86, 0x28, 0xc5, 0x29, 0x25, 0x24, 0x94, 0x62,
    0x8c, 0x61, 0x84,

    /* U+0058 "X" */
    0x42, 0x24, 0x3c, 0x18, 0x18, 0x24, 0x66, 0xc3,

    /* U+0059 "Y" */
    0x82, 0x89, 0xb1, 0x41, 0x2, 0x4, 0x8,

    /* U+005A "Z" */
    0xfe, 0x8, 0x20, 0x82, 0xc, 0x30, 0x7f,

    /* U+005B "[" */
    0xea, 0xaa, 0xb0,

    /* U+005C "\\" */
    0x91, 0x24, 0x49,

    /* U+005D "]" */
    0xd5, 0x55, 0x70,

    /* U+005E "^" */
    0x21, 0x94, 0x94, 0x80,

    /* U+005F "_" */
    0xfe,

    /* U+0060 "`" */
    0x80,

    /* U+0061 "a" */
    0x74, 0x47, 0xd8, 0xfc,

    /* U+0062 "b" */
    0x84, 0x3d, 0x18, 0xc6, 0x3e,

    /* U+0063 "c" */
    0x76, 0x61, 0x8, 0xb8,

    /* U+0064 "d" */
    0x8, 0x5f, 0x18, 0xc6, 0x2f,

    /* U+0065 "e" */
    0x72, 0x2f, 0xa0, 0xc9, 0xe0,

    /* U+0066 "f" */
    0x74, 0xe4, 0x44, 0x44,

    /* U+0067 "g" */
    0x7c, 0x63, 0x18, 0xbe, 0x2e,

    /* U+0068 "h" */
    0x84, 0x3d, 0x18, 0xc6, 0x31,

    /* U+0069 "i" */
    0xbf,

    /* U+006A "j" */
    0x45, 0x55, 0x70,

    /* U+006B "k" */
    0x84, 0x25, 0x4e, 0x52, 0x53,

    /* U+006C "l" */
    0xff,

    /* U+006D "m" */
    0xf7, 0x44, 0x62, 0x31, 0x18, 0x8c, 0x44,

    /* U+006E "n" */
    0xf4, 0x63, 0x18, 0xc4,

    /* U+006F "o" */
    0x7b, 0x38, 0x61, 0xcd, 0xe0,

    /* U+0070 "p" */
    0xf6, 0x63, 0x18, 0xfa, 0x10,

    /* U+0071 "q" */
    0x7c, 0xe3, 0x18, 0xbc, 0x21,

    /* U+0072 "r" */
    0xf2, 0x49, 0x0,

    /* U+0073 "s" */
    0x74, 0x78, 0x38, 0xb8,

    /* U+0074 "t" */
    0x4b, 0xa4, 0x93,

    /* U+0075 "u" */
    0x8c, 0x63, 0x18, 0xbc,

    /* U+0076 "v" */
    0x85, 0x24, 0x8c, 0x30, 0xc0,

    /* U+0077 "w" */
    0x89, 0x2c, 0x95, 0x4a, 0xc6, 0x61, 0x30,

    /* U+0078 "x" */
    0x49, 0xe3, 0xc, 0x4b, 0x30,

    /* U+0079 "y" */
    0xc5, 0x24, 0x8c, 0x30, 0xc2, 0x18,

    /* U+007A "z" */
    0xfc, 0x21, 0x8, 0x43, 0xf0,

    /* U+007B "{" */
    0x69, 0x28, 0x92, 0x4c,

    /* U+007C "|" */
    0xff, 0xc0,

    /* U+007D "}" */
    0xc9, 0x22, 0x92, 0x58,

    /* U+007E "~" */
    0xe0, 0x70,

    /* U+F001 "" */
    0x0, 0x70, 0x3f, 0x1f, 0xf1, 0xfb, 0x1c, 0x31,
    0x83, 0x18, 0x31, 0x83, 0x19, 0xf7, 0x9f, 0xf8,
    0x47, 0x0,

    /* U+F008 "" */
    0xbf, 0xde, 0x7, 0xa0, 0x5e, 0x7, 0xbf, 0xde,
    0x7, 0xa0, 0x5e, 0x7, 0xbf, 0xd0,

    /* U+F00B "" */
    0xf7, 0xf7, 0xbf, 0xfd, 0xfe, 0x0, 0xf, 0x7f,
    0x7b, 0xff, 0xdf, 0xc0, 0x0, 0xf7, 0xf7, 0xbf,
    0xfd, 0xfc,

    /* U+F00C "" */
    0x0, 0x20, 0x7, 0x0, 0xe4, 0x1c, 0xe3, 0x87,
    0x70, 0x3e, 0x1, 0xc0, 0x8, 0x0,

    /* U+F00D "" */
    0xc3, 0xe7, 0x7e, 0x3c, 0x3c, 0x7e, 0xe7, 0xc3,

    /* U+F011 "" */
    0x6, 0x2, 0x64, 0x76, 0xe6, 0x66, 0xc6, 0x3c,
    0x63, 0xc6, 0x3c, 0x3, 0x60, 0x67, 0xe, 0x3f,
    0xc0, 0xf0,

    /* U+F013 "" */
    0xe, 0x4, 0xf0, 0x7f, 0xef, 0xfe, 0x71, 0xe7,
    0xc, 0x71, 0xef, 0xfe, 0x7f, 0xe4, 0xf0, 0xe,
    0x0,

    /* U+F015 "" */
    0x3, 0x30, 0x1e, 0xc1, 0xcf, 0xc, 0xcc, 0x6f,
    0xdb, 0x7f, 0xb3, 0xff, 0xf, 0x3c, 0x3c, 0xf0,
    0xf3, 0xc0,

    /* U+F019 "" */
    0xe, 0x0, 0xe0, 0xe, 0x0, 0xe0, 0x3f, 0xc3,
    0xf8, 0x1f, 0x0, 0xe0, 0xf5, 0xff, 0xff, 0xff,
    0x5f, 0xff,

    /* U+F01C "" */
    0x1f, 0xe0, 0xc0, 0xc6, 0x1, 0x90, 0x2, 0xf8,
    0x7f, 0xe1, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc,

    /* U+F021 "" */
    0x0, 0x31, 0xf3, 0x71, 0xfc, 0x7, 0xc3, 0xf0,
    0x0, 0x0, 0x0, 0x0, 0xfc, 0x3e, 0x3, 0xf8,
    0xec, 0xf8, 0xc0, 0x0,

    /* U+F026 "" */
    0xc, 0x7f, 0xff, 0xff, 0xf1, 0xc3,

    /* U+F027 "" */
    0xc, 0xe, 0x3f, 0x7f, 0x9f, 0xdf, 0xe0, 0x70,
    0x18,

    /* U+F028 "" */
    0x0, 0x60, 0x1, 0x83, 0x34, 0x38, 0xdf, 0xda,
    0xfe, 0x57, 0xf6, 0xbf, 0x8d, 0x1c, 0xd0, 0x61,
    0x80, 0x18,

    /* U+F03E "" */
    0xff, 0xf9, 0xff, 0x9f, 0xf9, 0xef, 0xfc, 0x7d,
    0x83, 0xc0, 0x38, 0x3, 0xff, 0xf0,

    /* U+F043 "" */
    0x0, 0xc, 0x7, 0x7, 0x83, 0xe3, 0xfb, 0xfd,
    0xff, 0xdf, 0xef, 0x99, 0xc7, 0x80,

    /* U+F048 "" */
    0xc3, 0xc7, 0xcf, 0xdf, 0xff, 0xff, 0xdf, 0xcf,
    0xc7, 0xc3,

    /* U+F04B "" */
    0x0, 0x1c, 0x3, 0xe0, 0x7f, 0xf, 0xf9, 0xff,
    0xbf, 0xff, 0xfe, 0xff, 0x9f, 0xc3, 0xe0, 0x70,
    0x0, 0x0,

    /* U+F04C "" */
    0xfb, 0xff, 0x7f, 0xef, 0xfd, 0xff, 0xbf, 0xf7,
    0xfe, 0xff, 0xdf, 0xfb, 0xff, 0x7c,

    /* U+F04D "" */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xfc,

    /* U+F051 "" */
    0xc3, 0xe3, 0xf3, 0xfb, 0xff, 0xff, 0xfb, 0xf3,
    0xe3, 0xc3,

    /* U+F052 "" */
    0xc, 0x3, 0xc0, 0x7c, 0x1f, 0xc7, 0xfd, 0xff,
    0xbf, 0xf0, 0x0, 0xff, 0xff, 0xff, 0xff, 0x80,

    /* U+F053 "" */
    0xc, 0x73, 0x9c, 0xe3, 0x87, 0xe, 0x1c, 0x30,

    /* U+F054 "" */
    0x83, 0x87, 0xe, 0x1c, 0x73, 0x9c, 0xe2, 0x0,

    /* U+F067 "" */
    0xe, 0x1, 0xc0, 0x38, 0x7, 0xf, 0xff, 0xff,
    0xc3, 0x80, 0x70, 0xe, 0x1, 0xc0,

    /* U+F068 "" */
    0xff, 0xff, 0xfc,

    /* U+F06E "" */
    0xf, 0x81, 0xc7, 0x1c, 0x1d, 0xc6, 0x7e, 0xfb,
    0xf7, 0xdd, 0xdd, 0xc7, 0x1c, 0xf, 0x80,

    /* U+F070 "" */
    0x0, 0x1, 0xc0, 0x1, 0xdf, 0x0, 0xe3, 0x80,
    0xdb, 0x84, 0xfb, 0x9c, 0x77, 0x3c, 0x6e, 0x38,
    0x78, 0x38, 0x70, 0x1e, 0x30, 0x0, 0x30, 0x0,
    0x0,

    /* U+F071 "" */
    0x3, 0x0, 0x1c, 0x0, 0xf8, 0x3, 0xf0, 0x1c,
    0xc0, 0x73, 0x83, 0xcf, 0x1f, 0xfc, 0x7c, 0xfb,
    0xf3, 0xef, 0xff, 0x80,

    /* U+F074 "" */
    0x0, 0x0, 0x6, 0xe1, 0xff, 0x3f, 0x17, 0x60,
    0xe4, 0x1f, 0x6f, 0xbf, 0xf1, 0xf0, 0x6, 0x0,
    0x40,

    /* U+F077 "" */
    0x0, 0x3, 0x1, 0xe0, 0xcc, 0x61, 0xb0, 0x30,
    0x0,

    /* U+F078 "" */
    0x0, 0x30, 0x36, 0x18, 0xcc, 0x1e, 0x3, 0x0,
    0x0,

    /* U+F079 "" */
    0x30, 0x0, 0xf7, 0xf3, 0xf0, 0x65, 0xa0, 0xc3,
    0x1, 0x86, 0xb, 0x4c, 0x1f, 0x9f, 0xde, 0x0,
    0x18,

    /* U+F07B "" */
    0x78, 0xf, 0xc0, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xf0,

    /* U+F093 "" */
    0x6, 0x0, 0xf0, 0x1f, 0x83, 0xfc, 0x7, 0x0,
    0x70, 0x7, 0x0, 0x70, 0xf7, 0xff, 0xff, 0xff,
    0x5f, 0xff,

    /* U+F095 "" */
    0x0, 0x0, 0xf, 0x0, 0xf0, 0x1f, 0x0, 0xf0,
    0x6, 0x0, 0xe0, 0x1c, 0x73, 0xcf, 0xf8, 0xfe,
    0xf, 0xc0, 0x40, 0x0,

    /* U+F0C4 "" */
    0x70, 0x5b, 0x3f, 0x6f, 0x3f, 0xc1, 0xf0, 0x3e,
    0x1f, 0xe6, 0xde, 0xd9, 0xee, 0x8,

    /* U+F0C5 "" */
    0x1f, 0x43, 0xef, 0x7f, 0xef, 0xfd, 0xff, 0xbf,
    0xf7, 0xfe, 0xff, 0xdf, 0xf8, 0x3, 0xfc, 0x0,

    /* U+F0C7 "" */
    0xff, 0x98, 0x1b, 0x3, 0xe0, 0x7c, 0xf, 0xff,
    0xfe, 0x7f, 0x8f, 0xf9, 0xff, 0xfc,

    /* U+F0C9 "" */
    0xff, 0xe0, 0x0, 0x0, 0x0, 0xf, 0xfe, 0x0,
    0x0, 0x0, 0x0, 0xff, 0xe0,

    /* U+F0E0 "" */
    0xff, 0xff, 0xff, 0x7f, 0xeb, 0xfd, 0xcf, 0x3e,
    0x67, 0xf9, 0xff, 0xff, 0xff, 0xf0,

    /* U+F0E7 "" */
    0x78, 0x78, 0xf8, 0xf0, 0xff, 0xfe, 0xfc, 0x1c,
    0x18, 0x18, 0x10, 0x30,

    /* U+F0EA "" */
    0x18, 0x3b, 0x8e, 0xe3, 0xf8, 0xe0, 0x3b, 0xae,
    0xe7, 0xbf, 0xef, 0xfb, 0xf0, 0xfc, 0x3f,

    /* U+F0F3 "" */
    0x4, 0x0, 0x80, 0x7c, 0x1f, 0xc3, 0xf8, 0x7f,
    0x1f, 0xf3, 0xfe, 0x7f, 0xdf, 0xfc, 0x0, 0x7,
    0x0,

    /* U+F11C "" */
    0xff, 0xff, 0x52, 0xbd, 0x4a, 0xff, 0xff, 0xeb,
    0x5f, 0xff, 0xfd, 0x2, 0xf4, 0xb, 0xff, 0xfc,

    /* U+F124 "" */
    0x0, 0x0, 0xf, 0x3, 0xf0, 0xfe, 0x3f, 0xef,
    0xfc, 0xff, 0xc0, 0x78, 0x7, 0x80, 0x78, 0x7,
    0x0, 0x70, 0x2, 0x0,

    /* U+F158 "" */
    0x3f, 0xc, 0x63, 0x18, 0xc6, 0x31, 0xbf, 0xc3,
    0x3, 0xfc, 0x30, 0xc, 0x0,

    /* U+F1EB "" */
    0x7, 0xc0, 0x7f, 0xf1, 0xe0, 0xf7, 0x0, 0x70,
    0x7c, 0x3, 0xfe, 0x6, 0xc, 0x0, 0x0, 0x3,
    0x80, 0x7, 0x0, 0xe, 0x0,

    /* U+F240 "" */
    0xff, 0xff, 0x80, 0x1f, 0x7f, 0xfe, 0xff, 0xbd,
    0xff, 0xf8, 0x1, 0xff, 0xff, 0x80,

    /* U+F241 "" */
    0xff, 0xff, 0x80, 0x1f, 0x7f, 0x3e, 0xfe, 0x3d,
    0xfc, 0xf8, 0x1, 0xff, 0xff, 0x80,

    /* U+F242 "" */
    0xff, 0xff, 0x80, 0x1f, 0x78, 0x3e, 0xf0, 0x3d,
    0xe0, 0xf8, 0x1, 0xff, 0xff, 0x80,

    /* U+F243 "" */
    0xff, 0xff, 0x80, 0x1f, 0x60, 0x3e, 0xc0, 0x3d,
    0x80, 0xf8, 0x1, 0xff, 0xff, 0x80,

    /* U+F244 "" */
    0xff, 0xff, 0x80, 0x1f, 0x0, 0x3e, 0x0, 0x3c,
    0x0, 0xf8, 0x1, 0xff, 0xff, 0x80,

    /* U+F287 "" */
    0x0, 0xc0, 0x7, 0x80, 0x10, 0x7, 0x20, 0x6f,
    0xff, 0xfc, 0x41, 0x80, 0x40, 0x0, 0xb8, 0x0,
    0xf0,

    /* U+F293 "" */
    0x3e, 0x3b, 0x9c, 0xdb, 0x7c, 0xbf, 0x1f, 0x9f,
    0x87, 0xd5, 0xf9, 0x9d, 0xc7, 0xc0,

    /* U+F2ED "" */
    0xe, 0x1f, 0xfc, 0x0, 0x0, 0x7, 0xfc, 0xd5,
    0x9a, 0xb3, 0x56, 0x6a, 0xcd, 0x59, 0xab, 0x3f,
    0xe0,

    /* U+F304 "" */
    0x0, 0x40, 0xe, 0x0, 0xf0, 0x37, 0x7, 0xa0,
    0xfc, 0x1f, 0x83, 0xf0, 0x7e, 0xf, 0xc0, 0xf8,
    0xf, 0x0, 0x80, 0x0,

    /* U+F55A "" */
    0xf, 0xfe, 0x3f, 0xfc, 0xfb, 0x3b, 0xf0, 0xff,
    0xf3, 0xef, 0xc3, 0xcf, 0xb7, 0x8f, 0xff, 0xf,
    0xfe,

    /* U+F7C2 "" */
    0x1f, 0x9a, 0xbe, 0xaf, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xf8,

    /* U+F8A2 "" */
    0x0, 0x0, 0x3, 0x30, 0x37, 0x3, 0xff, 0xff,
    0xff, 0x70, 0x3, 0x0
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 53, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 53, .box_w = 1, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 2, .adv_w = 68, .box_w = 3, .box_h = 3, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 4, .adv_w = 107, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 11, .adv_w = 107, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 18, .adv_w = 171, .box_w = 9, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 27, .adv_w = 128, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 34, .adv_w = 37, .box_w = 1, .box_h = 3, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 35, .adv_w = 64, .box_w = 3, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 39, .adv_w = 64, .box_w = 3, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 43, .adv_w = 75, .box_w = 4, .box_h = 3, .ofs_x = 0, .ofs_y = 5},
    {.bitmap_index = 45, .adv_w = 112, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 49, .adv_w = 53, .box_w = 1, .box_h = 3, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 50, .adv_w = 64, .box_w = 3, .box_h = 1, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 51, .adv_w = 53, .box_w = 1, .box_h = 1, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 52, .adv_w = 53, .box_w = 3, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 55, .adv_w = 107, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 61, .adv_w = 107, .box_w = 3, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 64, .adv_w = 107, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 70, .adv_w = 107, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 75, .adv_w = 107, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 81, .adv_w = 107, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 87, .adv_w = 107, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 93, .adv_w = 107, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 99, .adv_w = 107, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 105, .adv_w = 107, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 111, .adv_w = 53, .box_w = 1, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 112, .adv_w = 53, .box_w = 1, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 113, .adv_w = 112, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 118, .adv_w = 112, .box_w = 5, .box_h = 3, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 120, .adv_w = 112, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 125, .adv_w = 107, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 130, .adv_w = 195, .box_w = 11, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 144, .adv_w = 128, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 152, .adv_w = 128, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 158, .adv_w = 139, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 165, .adv_w = 139, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 172, .adv_w = 128, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 178, .adv_w = 117, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 184, .adv_w = 149, .box_w = 8, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 192, .adv_w = 139, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 199, .adv_w = 53, .box_w = 1, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 200, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 205, .adv_w = 128, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 212, .adv_w = 107, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 217, .adv_w = 160, .box_w = 8, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 225, .adv_w = 139, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 232, .adv_w = 149, .box_w = 8, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 240, .adv_w = 128, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 246, .adv_w = 149, .box_w = 8, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 254, .adv_w = 139, .box_w = 8, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 262, .adv_w = 128, .box_w = 6, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 268, .adv_w = 117, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 275, .adv_w = 139, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 282, .adv_w = 128, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 290, .adv_w = 181, .box_w = 11, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 301, .adv_w = 128, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 309, .adv_w = 128, .box_w = 7, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 316, .adv_w = 117, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 323, .adv_w = 53, .box_w = 2, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 326, .adv_w = 53, .box_w = 3, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 329, .adv_w = 53, .box_w = 2, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 332, .adv_w = 90, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 4},
    {.bitmap_index = 336, .adv_w = 107, .box_w = 7, .box_h = 1, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 337, .adv_w = 64, .box_w = 2, .box_h = 1, .ofs_x = 1, .ofs_y = 7},
    {.bitmap_index = 338, .adv_w = 107, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 342, .adv_w = 107, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 347, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 351, .adv_w = 107, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 356, .adv_w = 107, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 361, .adv_w = 53, .box_w = 4, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 365, .adv_w = 107, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 370, .adv_w = 107, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 375, .adv_w = 43, .box_w = 1, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 376, .adv_w = 43, .box_w = 2, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 379, .adv_w = 96, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 384, .adv_w = 43, .box_w = 1, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 385, .adv_w = 160, .box_w = 9, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 392, .adv_w = 107, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 396, .adv_w = 107, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 401, .adv_w = 107, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 406, .adv_w = 107, .box_w = 5, .box_h = 8, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 411, .adv_w = 64, .box_w = 3, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 414, .adv_w = 96, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 418, .adv_w = 53, .box_w = 3, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 421, .adv_w = 107, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 425, .adv_w = 96, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 430, .adv_w = 139, .box_w = 9, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 437, .adv_w = 96, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 442, .adv_w = 96, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 448, .adv_w = 96, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 453, .adv_w = 64, .box_w = 3, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 457, .adv_w = 50, .box_w = 1, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 459, .adv_w = 64, .box_w = 3, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 463, .adv_w = 112, .box_w = 6, .box_h = 2, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 465, .adv_w = 192, .box_w = 12, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 483, .adv_w = 192, .box_w = 12, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 497, .adv_w = 192, .box_w = 13, .box_h = 11, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 515, .adv_w = 192, .box_w = 12, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 529, .adv_w = 132, .box_w = 8, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 537, .adv_w = 192, .box_w = 12, .box_h = 12, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 555, .adv_w = 192, .box_w = 12, .box_h = 11, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 572, .adv_w = 216, .box_w = 14, .box_h = 10, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 590, .adv_w = 192, .box_w = 12, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 608, .adv_w = 216, .box_w = 14, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 624, .adv_w = 192, .box_w = 12, .box_h = 13, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 644, .adv_w = 96, .box_w = 6, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 650, .adv_w = 144, .box_w = 9, .box_h = 8, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 659, .adv_w = 216, .box_w = 13, .box_h = 11, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 677, .adv_w = 192, .box_w = 12, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 691, .adv_w = 132, .box_w = 9, .box_h = 12, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 705, .adv_w = 168, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 715, .adv_w = 168, .box_w = 11, .box_h = 13, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 733, .adv_w = 168, .box_w = 11, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 747, .adv_w = 168, .box_w = 11, .box_h = 10, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 761, .adv_w = 168, .box_w = 8, .box_h = 10, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 771, .adv_w = 168, .box_w = 11, .box_h = 11, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 787, .adv_w = 120, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 795, .adv_w = 120, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 803, .adv_w = 168, .box_w = 11, .box_h = 10, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 817, .adv_w = 168, .box_w = 11, .box_h = 2, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 820, .adv_w = 216, .box_w = 13, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 835, .adv_w = 240, .box_w = 15, .box_h = 13, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 860, .adv_w = 216, .box_w = 14, .box_h = 11, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 880, .adv_w = 192, .box_w = 12, .box_h = 11, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 897, .adv_w = 168, .box_w = 10, .box_h = 7, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 906, .adv_w = 168, .box_w = 10, .box_h = 7, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 915, .adv_w = 240, .box_w = 15, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 932, .adv_w = 192, .box_w = 12, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 946, .adv_w = 192, .box_w = 12, .box_h = 12, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 964, .adv_w = 192, .box_w = 12, .box_h = 13, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 984, .adv_w = 168, .box_w = 11, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 998, .adv_w = 168, .box_w = 11, .box_h = 11, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1014, .adv_w = 168, .box_w = 11, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1028, .adv_w = 168, .box_w = 11, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1041, .adv_w = 192, .box_w = 12, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1055, .adv_w = 120, .box_w = 8, .box_h = 12, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1067, .adv_w = 168, .box_w = 10, .box_h = 12, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1082, .adv_w = 168, .box_w = 11, .box_h = 12, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1099, .adv_w = 216, .box_w = 14, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1115, .adv_w = 192, .box_w = 12, .box_h = 13, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1135, .adv_w = 144, .box_w = 10, .box_h = 10, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1148, .adv_w = 240, .box_w = 15, .box_h = 11, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1169, .adv_w = 240, .box_w = 15, .box_h = 7, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 1183, .adv_w = 240, .box_w = 15, .box_h = 7, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 1197, .adv_w = 240, .box_w = 15, .box_h = 7, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 1211, .adv_w = 240, .box_w = 15, .box_h = 7, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 1225, .adv_w = 240, .box_w = 15, .box_h = 7, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 1239, .adv_w = 240, .box_w = 15, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1256, .adv_w = 168, .box_w = 9, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 1270, .adv_w = 168, .box_w = 11, .box_h = 12, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1287, .adv_w = 192, .box_w = 12, .box_h = 13, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1307, .adv_w = 240, .box_w = 15, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1324, .adv_w = 144, .box_w = 10, .box_h = 11, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1338, .adv_w = 193, .box_w = 12, .box_h = 8, .ofs_x = 0, .ofs_y = 1}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint16_t unicode_list_1[] = {
    0x0, 0x7, 0xa, 0xb, 0xc, 0x10, 0x12, 0x14,
    0x18, 0x1b, 0x20, 0x25, 0x26, 0x27, 0x3d, 0x42,
    0x47, 0x4a, 0x4b, 0x4c, 0x50, 0x51, 0x52, 0x53,
    0x66, 0x67, 0x6d, 0x6f, 0x70, 0x73, 0x76, 0x77,
    0x78, 0x7a, 0x92, 0x94, 0xc3, 0xc4, 0xc6, 0xc8,
    0xdf, 0xe6, 0xe9, 0xf2, 0x11b, 0x123, 0x157, 0x1ea,
    0x23f, 0x240, 0x241, 0x242, 0x243, 0x286, 0x292, 0x2ec,
    0x303, 0x559, 0x7c1, 0x8a1
};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 32, .range_length = 95, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 61441, .range_length = 2210, .glyph_id_start = 96,
        .unicode_list = unicode_list_1, .glyph_id_ofs_list = NULL, .list_length = 60, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
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

extern const lv_font_t lv_font_arial_12;


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t lv_font_arial_12 = {
#else
lv_font_t lv_font_arial_12 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 13,          /*The maximum line height required by the font*/
    .base_line = 2,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -1,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = NULL,
#endif
    .user_data = NULL,
};



#endif /*#if LV_FONT_ARIAL_12*/
