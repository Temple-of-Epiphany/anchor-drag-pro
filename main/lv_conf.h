/**
 * LVGL Configuration for Anchor Drag Pro
 *
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2025-12-20
 * Version: 0.1.1
 *
 * LVGL v9.2.0 configuration for ESP32-S3 with 800x480 RGB LCD
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*====================
   COLOR SETTINGS
 *====================*/

/* Color depth: 16 (RGB565), 24, or 32 */
#define LV_COLOR_DEPTH 16

/*=========================
   MEMORY SETTINGS
 *=========================*/

/* Size of the memory available for `lv_malloc()` in bytes (>= 2kB) */
#define LV_MEM_SIZE (128 * 1024U)  /* 128 KB */

/* Use the standard `malloc` and `free` */
#define LV_MEM_CUSTOM 1
#if LV_MEM_CUSTOM == 1
    #define LV_MEM_CUSTOM_INCLUDE <stdlib.h>
    #define LV_MEM_CUSTOM_ALLOC   malloc
    #define LV_MEM_CUSTOM_FREE    free
    #define LV_MEM_CUSTOM_REALLOC realloc
#endif

/*====================
   HAL SETTINGS
 *====================*/

/* Default display refresh period in milliseconds */
#define LV_DEF_REFR_PERIOD 10

/* Default DPI (dots per inch) */
#define LV_DPI_DEF 130

/*=================
   FONT USAGE
 *=================*/

/* Montserrat fonts with different sizes */
#define LV_FONT_MONTSERRAT_8  0
#define LV_FONT_MONTSERRAT_10 0
#define LV_FONT_MONTSERRAT_12 0
#define LV_FONT_MONTSERRAT_14 0
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_18 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_22 0
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_26 0
#define LV_FONT_MONTSERRAT_28 0
#define LV_FONT_MONTSERRAT_30 0
#define LV_FONT_MONTSERRAT_32 1
#define LV_FONT_MONTSERRAT_34 0
#define LV_FONT_MONTSERRAT_36 0
#define LV_FONT_MONTSERRAT_38 0
#define LV_FONT_MONTSERRAT_40 0
#define LV_FONT_MONTSERRAT_42 0
#define LV_FONT_MONTSERRAT_44 0
#define LV_FONT_MONTSERRAT_46 0
#define LV_FONT_MONTSERRAT_48 0

/* Default font */
#define LV_FONT_DEFAULT &lv_font_montserrat_16

/*================
   TEXT SETTINGS
 *================*/

/* Enable UTF-8 support */
#define LV_TXT_ENC LV_TXT_ENC_UTF8

/*===================
   FEATURE USAGE
 *===================*/

/* Enable animations */
#define LV_USE_ANIMATION 1
#if LV_USE_ANIMATION
    /* Default animation time in ms */
    #define LV_ANIM_TIME 300
#endif

/* Enable PNG decoder */
#define LV_USE_PNG 1

/* Enable BMP decoder */
#define LV_USE_BMP 1

/* Enable GIF decoder */
#define LV_USE_GIF 0

/* Enable SJPG decoder */
#define LV_USE_SJPG 0

/* Enable freetype library */
#define LV_USE_FREETYPE 0

/* Enable file system */
#define LV_USE_FS_POSIX 0

/*==================
   THEME USAGE
 *==================*/

/* Default theme */
#define LV_USE_THEME_DEFAULT 1
#if LV_USE_THEME_DEFAULT
    /* Dark mode */
    #define LV_THEME_DEFAULT_DARK 1
    /* Primary color */
    #define LV_THEME_DEFAULT_PRIMARY_COLOR lv_color_hex(0x01A2B1)
    /* Secondary color */
    #define LV_THEME_DEFAULT_SECONDARY_COLOR lv_color_hex(0x44D62C)
    /* Font */
    #define LV_THEME_DEFAULT_FONT &lv_font_montserrat_16
#endif

/*===================
   LAYOUTS
 *===================*/

/* Enable flex layout */
#define LV_USE_FLEX 1

/* Enable grid layout */
#define LV_USE_GRID 1

/*==================
   WIDGETS
 *==================*/

/* Enable all widgets by default */
#define LV_USE_ANIMIMG    1
#define LV_USE_ARC        1
#define LV_USE_BAR        1
#define LV_USE_BUTTON     1
#define LV_USE_BUTTONMATRIX 1
#define LV_USE_CALENDAR   0
#define LV_USE_CANVAS     1
#define LV_USE_CHECKBOX   1
#define LV_USE_DROPDOWN   1
#define LV_USE_IMAGE      1
#define LV_USE_IMAGEBUTTON 1
#define LV_USE_KEYBOARD   1
#define LV_USE_LABEL      1
#define LV_USE_LED        1
#define LV_USE_LINE       1
#define LV_USE_LIST       1
#define LV_USE_MENU       1
#define LV_USE_METER      1
#define LV_USE_MSGBOX     1
#define LV_USE_ROLLER     1
#define LV_USE_SCALE      1
#define LV_USE_SLIDER     1
#define LV_USE_SPAN       1
#define LV_USE_SPINBOX    1
#define LV_USE_SPINNER    1
#define LV_USE_SWITCH     1
#define LV_USE_TEXTAREA   1
#define LV_USE_TABLE      1
#define LV_USE_TABVIEW    1
#define LV_USE_TILEVIEW   1
#define LV_USE_WIN        1

/*==================
   LOGGING
 *==================*/

/* Enable logging */
#define LV_USE_LOG 1
#if LV_USE_LOG
    /* Log level: TRACE, INFO, WARN, ERROR, USER, NONE */
    #define LV_LOG_LEVEL LV_LOG_LEVEL_INFO

    /* Print log via ESP_LOG */
    #define LV_LOG_PRINTF 0
    #if LV_LOG_PRINTF
        #include <stdio.h>
        #define LV_LOG_PRINTF_FUNC printf
    #endif
#endif

/*=================
   ASSERTS
 *=================*/

/* Enable asserts */
#define LV_USE_ASSERT_NULL          1  /* Check if parameter is NULL */
#define LV_USE_ASSERT_MALLOC        1  /* Check if malloc succeeded */
#define LV_USE_ASSERT_STYLE         0  /* Check style consistency */
#define LV_USE_ASSERT_MEM_INTEGRITY 0  /* Check memory integrity */
#define LV_USE_ASSERT_OBJ           0  /* Check object validity */

/*==================
   OTHERS
 *==================*/

/* Enable snapshot feature */
#define LV_USE_SNAPSHOT 0

/* Enable monkey test */
#define LV_USE_MONKEY 0

/* Enable grid navigation */
#define LV_USE_GRIDNAV 0

/* Enable fragment */
#define LV_USE_FRAGMENT 0

/* Enable IME pinyin */
#define LV_USE_IME_PINYIN 0

/* Enable file explorer */
#define LV_USE_FILE_EXPLORER 0

/*==================
   DEMOS
 *==================*/

/* Disable all demos */
#define LV_USE_DEMO_WIDGETS 0
#define LV_USE_DEMO_BENCHMARK 0
#define LV_USE_DEMO_STRESS 0
#define LV_USE_DEMO_MUSIC 0

/*==================
   EXAMPLES
 *==================*/

/* Disable examples */
#define LV_BUILD_EXAMPLES 0

#endif /* LV_CONF_H */
