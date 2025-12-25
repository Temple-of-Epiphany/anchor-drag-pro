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

// Screen creation functions
// Each function returns the screen object and sets *footer_out to the footer reference
lv_obj_t* create_start_screen(ui_footer_page_cb_t page_callback, lv_obj_t **footer_out);
lv_obj_t* create_info_screen(ui_footer_page_cb_t page_callback, lv_obj_t **footer_out);
lv_obj_t* create_pgn_screen(ui_footer_page_cb_t page_callback, lv_obj_t **footer_out);
lv_obj_t* create_config_screen(ui_footer_page_cb_t page_callback, lv_obj_t **footer_out);
lv_obj_t* create_update_screen(ui_footer_page_cb_t page_callback, lv_obj_t **footer_out);
lv_obj_t* create_tools_screen(ui_footer_page_cb_t page_callback, lv_obj_t **footer_out);

#endif // SCREENS_H
