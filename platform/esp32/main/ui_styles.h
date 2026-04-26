/**
 * UI Style Manager - Encapsulated Pattern
 *
 * Provides reusable LVGL style objects without global variables.
 * Designed for LVGL 8.4.0
 *
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2026-01-01
 * Version: 0.1.0
 */

#ifndef UI_STYLES_H
#define UI_STYLES_H

#include "lvgl.h"
#include <stdbool.h>

/**
 * Opaque style manager handle
 * Encapsulates all style objects - implementation hidden
 */
typedef struct ui_styles_t ui_styles_t;

/**
 * Create and initialize the style manager
 *
 * @return Style manager instance, or NULL on failure
 * @note Caller is responsible for calling ui_styles_destroy()
 */
ui_styles_t* ui_styles_create(void);

/**
 * Destroy style manager and free resources
 *
 * @param styles Style manager instance (can be NULL)
 */
void ui_styles_destroy(ui_styles_t* styles);

/**
 * Get screen background style
 * Color: THEME_SCREEN_BG
 */
lv_style_t* ui_styles_get_screen_bg(ui_styles_t* styles);

/**
 * Get title text style
 * Font: FONT_TITLE, Color: THEME_TITLE_COLOR
 */
lv_style_t* ui_styles_get_title(ui_styles_t* styles);

/**
 * Get subtitle text style
 * Font: FONT_SUBTITLE, Color: COLOR_TEXT_SECONDARY
 */
lv_style_t* ui_styles_get_subtitle(ui_styles_t* styles);

/**
 * Get body text style
 * Font: FONT_BODY, Color: COLOR_TEXT_PRIMARY
 */
lv_style_t* ui_styles_get_body_text(ui_styles_t* styles);

/**
 * Get 3D button base style (shadow effects, rounded corners)
 * Apply this first, then add a color variant style
 */
lv_style_t* ui_styles_get_button_3d(ui_styles_t* styles);

/**
 * Get 3D button pressed state style
 */
lv_style_t* ui_styles_get_button_3d_pressed(ui_styles_t* styles);

/**
 * Button color variant styles (apply AFTER button_3d)
 */
lv_style_t* ui_styles_get_button_green(ui_styles_t* styles);
lv_style_t* ui_styles_get_button_red(ui_styles_t* styles);
lv_style_t* ui_styles_get_button_yellow(ui_styles_t* styles);
lv_style_t* ui_styles_get_button_blue(ui_styles_t* styles);
lv_style_t* ui_styles_get_button_gray(ui_styles_t* styles);

/**
 * Panel/container styles
 */
lv_style_t* ui_styles_get_panel(ui_styles_t* styles);
lv_style_t* ui_styles_get_panel_border(ui_styles_t* styles);

/**
 * Helper: Apply 3D button style with color variant
 *
 * @param styles Style manager instance
 * @param btn Button object
 * @param color_style One of: button_green, button_red, button_yellow, button_blue
 *
 * Example:
 *   ui_styles_apply_button(styles, btn, ui_styles_get_button_green(styles));
 */
void ui_styles_apply_button(ui_styles_t* styles, lv_obj_t* btn, lv_style_t* color_style);

#endif // UI_STYLES_H
