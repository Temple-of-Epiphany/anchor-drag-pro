/**
 * Screens Header
 *
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2025-12-25
 * Version: 0.1.0
 *
 * Screen creation functions for all app screens
 */

#ifndef SCREENS_H
#define SCREENS_H

#include "lvgl.h"
#include "ui_footer.h"
#include "ui_styles.h"

// Screen creation functions
// Each function returns the screen object and sets *footer_out to the footer reference
// All screens now accept ui_styles_t* for consistent styling

// Navigation screens (with footer)
lv_obj_t* create_start_screen(ui_footer_page_cb_t page_callback, lv_obj_t **footer_out, ui_styles_t* styles);
lv_obj_t* create_info_screen(ui_footer_page_cb_t page_callback, lv_obj_t **footer_out, ui_styles_t* styles);
lv_obj_t* create_pgn_screen(ui_footer_page_cb_t page_callback, lv_obj_t **footer_out, ui_styles_t* styles);
lv_obj_t* create_config_screen(ui_footer_page_cb_t page_callback, lv_obj_t **footer_out, ui_styles_t* styles);
lv_obj_t* create_update_screen(ui_footer_page_cb_t page_callback, lv_obj_t **footer_out, ui_styles_t* styles);
lv_obj_t* create_tools_screen(ui_footer_page_cb_t page_callback, lv_obj_t **footer_out, ui_styles_t* styles);

// Main anchor monitoring screen (now with footer navigation)
lv_obj_t* create_display_screen(ui_footer_page_cb_t page_callback, lv_obj_t **footer_out, ui_styles_t* styles);

// Special screens (no footer)
lv_obj_t* create_test_screen(ui_styles_t* styles);     // Hardware testing screen

#endif // SCREENS_H
