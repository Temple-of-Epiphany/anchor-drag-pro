/**
 * Screens Implementation
 *
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2025-12-25
 * Version: 0.1.0
 *
 * Screen creation functions for all app screens
 */

#include "screens.h"
#include "esp_log.h"

static const char *TAG = "screens";

// External font declarations
LV_FONT_DECLARE(orbitron_variablefont_wght_24);

// Common screen background color
#define SCREEN_BG_COLOR 0x001F3F  // Dark blue
#define TITLE_COLOR 0x39CCCC      // Teal

/**
 * Create a base screen with title and footer
 */
static lv_obj_t* create_base_screen(const char *title, ui_page_t page, ui_footer_page_cb_t page_callback, lv_obj_t **footer_out) {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(SCREEN_BG_COLOR), 0);

    // Create title label
    lv_obj_t *title_label = lv_label_create(screen);
    lv_label_set_text(title_label, title);
    lv_obj_set_style_text_font(title_label, &orbitron_variablefont_wght_24, 0);
    lv_obj_set_style_text_color(title_label, lv_color_hex(TITLE_COLOR), 0);
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 20);

    // Create footer navigation bar
    lv_obj_t *footer = ui_footer_create(screen, page, page_callback);

    // Return footer reference
    if (footer_out != NULL) {
        *footer_out = footer;
    }

    ESP_LOGI(TAG, "Created %s screen (page %d)", title, page);

    return screen;
}

lv_obj_t* create_start_screen(ui_footer_page_cb_t page_callback, lv_obj_t **footer_out) {
    return create_base_screen("START", PAGE_START, page_callback, footer_out);
}

lv_obj_t* create_info_screen(ui_footer_page_cb_t page_callback, lv_obj_t **footer_out) {
    return create_base_screen("INFO", PAGE_INFO, page_callback, footer_out);
}

lv_obj_t* create_pgn_screen(ui_footer_page_cb_t page_callback, lv_obj_t **footer_out) {
    return create_base_screen("PGN", PAGE_PGN, page_callback, footer_out);
}

lv_obj_t* create_config_screen(ui_footer_page_cb_t page_callback, lv_obj_t **footer_out) {
    return create_base_screen("CONFIG", PAGE_CONFIG, page_callback, footer_out);
}

lv_obj_t* create_update_screen(ui_footer_page_cb_t page_callback, lv_obj_t **footer_out) {
    return create_base_screen("UPDATE", PAGE_UPDATE, page_callback, footer_out);
}

lv_obj_t* create_tools_screen(ui_footer_page_cb_t page_callback, lv_obj_t **footer_out) {
    return create_base_screen("TOOLS", PAGE_TOOLS, page_callback, footer_out);
}
