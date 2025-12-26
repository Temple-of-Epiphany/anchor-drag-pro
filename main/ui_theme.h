/**
 * UI Theme - Centralized Colors and Fonts
 *
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2025-12-25
 * Version: 1.0.0
 *
 * Centralized color palette and font definitions for consistent theming
 * across all UI screens. Modify values here to change the entire app theme.
 */

#ifndef UI_THEME_H
#define UI_THEME_H

#include "lvgl.h"

// ============================================================================
// FONT DECLARATIONS
// ============================================================================

// Orbitron - Futuristic/Technical font (variable weight)
LV_FONT_DECLARE(orbitron_variablefont_wght_16);
LV_FONT_DECLARE(orbitron_variablefont_wght_20);
LV_FONT_DECLARE(orbitron_variablefont_wght_24);
LV_FONT_DECLARE(orbitron_variablefont_wght_32);
LV_FONT_DECLARE(orbitron_variablefont_wght_48);

// Poppins - Clean, readable font
LV_FONT_DECLARE(poppins_regular_16);
LV_FONT_DECLARE(poppins_regular_20);
LV_FONT_DECLARE(poppins_regular_24);
LV_FONT_DECLARE(poppins_regular_32);
LV_FONT_DECLARE(poppins_regular_48);

// GolosText - Geometric, excellent readability
LV_FONT_DECLARE(golostext_regular_16);
LV_FONT_DECLARE(golostext_regular_20);
LV_FONT_DECLARE(golostext_regular_24);
LV_FONT_DECLARE(golostext_regular_32);
LV_FONT_DECLARE(golostext_regular_48);

// SF Pro Rounded - Apple's rounded system font
LV_FONT_DECLARE(sfnsrounded_16);
LV_FONT_DECLARE(sfnsrounded_20);
LV_FONT_DECLARE(sfnsrounded_24);
LV_FONT_DECLARE(sfnsrounded_32);
LV_FONT_DECLARE(sfnsrounded_48);

// Apple Symbols - Icons
LV_FONT_DECLARE(apple_symbols_16);
LV_FONT_DECLARE(apple_symbols_24);
LV_FONT_DECLARE(apple_symbols_32);

// ============================================================================
// FONT ASSIGNMENTS - Change these to switch fonts globally
// ============================================================================

#define FONT_TITLE          &orbitron_variablefont_wght_24   // Screen titles
#define FONT_SUBTITLE       &orbitron_variablefont_wght_20   // Subtitles
#define FONT_BUTTON_LARGE   &orbitron_variablefont_wght_20   // Large buttons
#define FONT_BUTTON_SMALL   &orbitron_variablefont_wght_16   // Small buttons
#define FONT_BODY_LARGE     &orbitron_variablefont_wght_20   // Body text (large)
#define FONT_BODY_NORMAL    &orbitron_variablefont_wght_16   // Body text (normal)
#define FONT_LABEL          &orbitron_variablefont_wght_16   // Labels
#define FONT_FOOTER         &orbitron_variablefont_wght_16   // Footer buttons

// ============================================================================
// COLOR PALETTE - Marine/Nautical Theme
// ============================================================================

// Primary Colors
#define COLOR_PRIMARY_DARK      0x001F3F    // Dark Navy Blue - Main background
#define COLOR_PRIMARY           0x0074D9    // Marine Blue - Accent/highlights
#define COLOR_PRIMARY_LIGHT     0x39CCCC    // Teal - Titles/important text

// Semantic Colors (Status)
#define COLOR_SUCCESS           0x2ECC40    // Green - Success, ready, go
#define COLOR_WARNING           0xFFDC00    // Yellow - Warning, caution
#define COLOR_DANGER            0xFF4136    // Red - Error, alarm, stop
#define COLOR_INFO              0x39CCCC    // Teal - Information
#define COLOR_DISABLED          0xAAAAAA    // Gray - Disabled, off

// Background Colors
#define COLOR_BG_SCREEN         COLOR_PRIMARY_DARK      // Main screen background
#define COLOR_BG_PANEL          0x003366                // Panel background (lighter than screen)
#define COLOR_BG_PANEL_DARK     0x002040                // Dark panel background
#define COLOR_BG_MODAL          0x001020                // Modal/overlay background

// Text Colors
#define COLOR_TEXT_PRIMARY      0xFFFFFF    // White - Primary text
#define COLOR_TEXT_SECONDARY    0xCCCCCC    // Light gray - Secondary text
#define COLOR_TEXT_DISABLED     0x666666    // Dark gray - Disabled text
#define COLOR_TEXT_INVERSE      0x000000    // Black - Text on light backgrounds

// Button Colors (Mode Selection)
#define COLOR_BTN_OFF           COLOR_DISABLED          // Gray - System off
#define COLOR_BTN_READY         COLOR_SUCCESS           // Green - Ready/active
#define COLOR_BTN_INFO          COLOR_INFO              // Teal - Information
#define COLOR_BTN_CONFIG        0xBA68C8                // Purple - Configuration

// Border/Accent Colors
#define COLOR_BORDER            COLOR_PRIMARY_LIGHT     // Teal border
#define COLOR_BORDER_WARNING    COLOR_DANGER            // Red border (warnings)
#define COLOR_BORDER_SUBTLE     0x004466                // Subtle border

// Footer/Navigation Colors
#define COLOR_FOOTER_BG         0x002244                // Footer background
#define COLOR_FOOTER_BTN        0x0074D9                // Footer button color
#define COLOR_FOOTER_BTN_ACTIVE 0x39CCCC                // Active footer button

// Shadow Colors
#define COLOR_SHADOW            0x000000                // Black shadow

// ============================================================================
// NAMED THEME COLORS - Semantic names for specific UI elements
// ============================================================================

// Screen backgrounds
#define THEME_SCREEN_BG         COLOR_BG_SCREEN
#define THEME_START_SCREEN_BG   COLOR_PRIMARY           // START screen has lighter blue

// Title colors
#define THEME_TITLE_COLOR       COLOR_PRIMARY_LIGHT     // Teal titles
#define THEME_SUBTITLE_COLOR    COLOR_TEXT_SECONDARY    // Gray subtitles

// Panel colors
#define THEME_PANEL_BG          COLOR_BG_PANEL
#define THEME_PANEL_BG_DARK     COLOR_BG_PANEL_DARK
#define THEME_PANEL_BORDER      COLOR_BORDER

// Button colors (by function)
#define THEME_BTN_PRIMARY       COLOR_PRIMARY
#define THEME_BTN_SUCCESS       COLOR_SUCCESS
#define THEME_BTN_DANGER        COLOR_DANGER
#define THEME_BTN_CANCEL        COLOR_DISABLED

// ============================================================================
// SPACING AND SIZING CONSTANTS
// ============================================================================

// Screen layout
#define LAYOUT_SCREEN_WIDTH     800
#define LAYOUT_SCREEN_HEIGHT    480
#define LAYOUT_FOOTER_HEIGHT    60
#define LAYOUT_CONTENT_HEIGHT   (LAYOUT_SCREEN_HEIGHT - LAYOUT_FOOTER_HEIGHT)

// Margins and padding
#define SPACING_MARGIN_LARGE    30
#define SPACING_MARGIN_MEDIUM   20
#define SPACING_MARGIN_SMALL    10
#define SPACING_PADDING_LARGE   20
#define SPACING_PADDING_MEDIUM  10
#define SPACING_PADDING_SMALL   5

// Button sizes
#define BUTTON_HEIGHT_LARGE     70
#define BUTTON_HEIGHT_MEDIUM    50
#define BUTTON_HEIGHT_SMALL     40
#define BUTTON_WIDTH_LARGE      600
#define BUTTON_WIDTH_MEDIUM     250
#define BUTTON_WIDTH_SMALL      170

// Border radius
#define RADIUS_LARGE            15
#define RADIUS_MEDIUM           10
#define RADIUS_SMALL            5

// Border widths
#define BORDER_WIDTH_THICK      3
#define BORDER_WIDTH_MEDIUM     2
#define BORDER_WIDTH_THIN       1

// Shadow
#define SHADOW_WIDTH            8
#define SHADOW_OFFSET_Y         4
#define SHADOW_OPACITY          LV_OPA_30

// ============================================================================
// OPACITY LEVELS
// ============================================================================

#define OPACITY_FULL            LV_OPA_COVER
#define OPACITY_HIGH            LV_OPA_80
#define OPACITY_MEDIUM          LV_OPA_50
#define OPACITY_LOW             LV_OPA_30
#define OPACITY_TRANSPARENT     LV_OPA_TRANSP

// ============================================================================
// HELPER MACROS
// ============================================================================

// Apply standard button style
#define THEME_STYLE_BUTTON(obj, bg_color) \
    do { \
        lv_obj_set_style_bg_color(obj, lv_color_hex(bg_color), 0); \
        lv_obj_set_style_radius(obj, RADIUS_LARGE, 0); \
        lv_obj_set_style_shadow_width(obj, SHADOW_WIDTH, 0); \
        lv_obj_set_style_shadow_ofs_y(obj, SHADOW_OFFSET_Y, 0); \
        lv_obj_set_style_shadow_color(obj, lv_color_hex(COLOR_SHADOW), 0); \
        lv_obj_set_style_shadow_opa(obj, SHADOW_OPACITY, 0); \
    } while(0)

// Apply standard panel style
#define THEME_STYLE_PANEL(obj, bg_color) \
    do { \
        lv_obj_set_style_bg_color(obj, lv_color_hex(bg_color), 0); \
        lv_obj_set_style_border_color(obj, lv_color_hex(COLOR_BORDER), 0); \
        lv_obj_set_style_border_width(obj, BORDER_WIDTH_MEDIUM, 0); \
        lv_obj_set_style_radius(obj, RADIUS_SMALL, 0); \
    } while(0)

// Apply standard text style
#define THEME_STYLE_TEXT(obj, color, font) \
    do { \
        lv_obj_set_style_text_color(obj, lv_color_hex(color), 0); \
        lv_obj_set_style_text_font(obj, font, 0); \
    } while(0)

#endif // UI_THEME_H
