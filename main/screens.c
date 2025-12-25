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

#include <stdio.h>
#include "screens.h"
#include "esp_log.h"

static const char *TAG = "screens";

// External font declarations
LV_FONT_DECLARE(orbitron_variablefont_wght_24);
LV_FONT_DECLARE(orbitron_variablefont_wght_20);

// Common screen background color
#define SCREEN_BG_COLOR 0x001F3F  // Dark blue
#define TITLE_COLOR 0x39CCCC      // Teal

// START screen colors (per UI spec)
#define START_BG_COLOR 0x0074D9   // Marine Blue
#define BTN_OFF_COLOR 0xAAAAAA    // Gray
#define BTN_READY_COLOR 0x2ECC40  // Green
#define BTN_INFO_COLOR 0x39CCCC   // Teal
#define BTN_CONFIG_COLOR 0xBA68C8 // Purple (light)

/**
 * Button event handlers for START screen
 */
static void btn_off_clicked(lv_event_t *e) {
    ESP_LOGI(TAG, "OFF button clicked - Setting mode to OFF");
    // TODO: Set system mode to OFF
    // TODO: Change background to gray
}

static void btn_ready_clicked(lv_event_t *e) {
    ESP_LOGI(TAG, "READY button clicked - Activating anchor monitoring");
    // TODO: Set system mode to READY
    // TODO: Navigate to DISPLAY screen
}

static void btn_info_clicked(lv_event_t *e) {
    ESP_LOGI(TAG, "INFO button clicked - View GPS & Compass details");
    // TODO: Navigate to INFO screen (could use footer navigation)
}

static void btn_config_clicked(lv_event_t *e) {
    ESP_LOGI(TAG, "CONFIG button clicked - Configure system settings");
    // TODO: Navigate to CONFIG screen (could use footer navigation)
}

/**
 * Create a styled mode selection button
 */
static lv_obj_t* create_mode_button(lv_obj_t *parent, const char *icon, const char *title,
                                    const char *subtitle, uint32_t bg_color,
                                    lv_event_cb_t event_cb, int y_offset) {
    // Create button container
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 600, 70);  // Wide button
    lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, y_offset);

    // Style the button
    lv_obj_set_style_bg_color(btn, lv_color_hex(bg_color), 0);
    lv_obj_set_style_radius(btn, 15, 0);  // Rounded corners
    lv_obj_set_style_shadow_width(btn, 8, 0);
    lv_obj_set_style_shadow_ofs_y(btn, 4, 0);
    lv_obj_set_style_shadow_color(btn, lv_color_black(), 0);
    lv_obj_set_style_shadow_opa(btn, LV_OPA_30, 0);

    // Add click event
    lv_obj_add_event_cb(btn, event_cb, LV_EVENT_CLICKED, NULL);

    // Create label with text (icon optional)
    lv_obj_t *label = lv_label_create(btn);
    char text[128];
    if (icon != NULL && icon[0] != '\0') {
        snprintf(text, sizeof(text), "%s %s\n%s", icon, title, subtitle);
    } else {
        snprintf(text, sizeof(text), "%s\n%s", title, subtitle);
    }
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_set_style_text_font(label, &orbitron_variablefont_wght_20, 0);
    lv_obj_center(label);

    return btn;
}

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

    // Ensure footer is visible and in foreground
    if (footer != NULL) {
        ui_footer_show(footer);  // Explicitly show the footer
        lv_obj_move_foreground(footer);  // Bring to front (above all other objects)
    }

    // Return footer reference
    if (footer_out != NULL) {
        *footer_out = footer;
    }

    ESP_LOGI(TAG, "Created %s screen (page %d) with visible footer", title, page);

    return screen;
}

lv_obj_t* create_start_screen(ui_footer_page_cb_t page_callback, lv_obj_t **footer_out) {
    // Create screen with Marine Blue background (per UI spec)
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(START_BG_COLOR), 0);

    // Create "SELECT MODE" title
    lv_obj_t *title_label = lv_label_create(screen);
    lv_label_set_text(title_label, "SELECT MODE");
    lv_obj_set_style_text_font(title_label, &orbitron_variablefont_wght_24, 0);
    lv_obj_set_style_text_color(title_label, lv_color_white(), 0);
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 15);

    // Create subtitle
    lv_obj_t *subtitle_label = lv_label_create(screen);
    lv_label_set_text(subtitle_label, "Choose your operating mode");
    lv_obj_set_style_text_font(subtitle_label, &orbitron_variablefont_wght_20, 0);
    lv_obj_set_style_text_color(subtitle_label, lv_color_hex(0xCCCCCC), 0);
    lv_obj_align(subtitle_label, LV_ALIGN_TOP_MID, 0, 45);

    // Create 4 mode selection buttons (plain text, no icons)
    // Button positions: starting at y=80, spacing of 85px between buttons
    create_mode_button(screen, "", "OFF", "System Disabled",
                       BTN_OFF_COLOR, btn_off_clicked, 80);

    create_mode_button(screen, "", "READY", "Activate Anchor Monitoring",
                       BTN_READY_COLOR, btn_ready_clicked, 165);

    create_mode_button(screen, "", "INFO", "View GPS & Compass Details",
                       BTN_INFO_COLOR, btn_info_clicked, 250);

    create_mode_button(screen, "", "CONFIG", "Configure System Settings",
                       BTN_CONFIG_COLOR, btn_config_clicked, 335);

    // Create footer navigation bar
    lv_obj_t *footer = ui_footer_create(screen, PAGE_START, page_callback);

    // Ensure footer is visible and in foreground
    if (footer != NULL) {
        ui_footer_show(footer);  // Explicitly show the footer
        lv_obj_move_foreground(footer);  // Bring to front (above all other objects)
    }

    // Return footer reference
    if (footer_out != NULL) {
        *footer_out = footer;
    }

    ESP_LOGI(TAG, "Created START (Mode Selection) screen with 4 action buttons and visible footer");

    return screen;
}

/**
 * INFO SCREEN - Compass & GPS Details
 */
lv_obj_t* create_info_screen(ui_footer_page_cb_t page_callback, lv_obj_t **footer_out) {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(SCREEN_BG_COLOR), 0);

    // Title
    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "POSITION & NAVIGATION");
    lv_obj_set_style_text_font(title, &orbitron_variablefont_wght_24, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(TITLE_COLOR), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

    // Left side - Compass Rose placeholder
    lv_obj_t *compass_box = lv_obj_create(screen);
    lv_obj_set_size(compass_box, 180, 180);
    lv_obj_align(compass_box, LV_ALIGN_LEFT_MID, 30, -20);
    lv_obj_set_style_bg_color(compass_box, lv_color_hex(0x003366), 0);
    lv_obj_set_style_border_color(compass_box, lv_color_hex(TITLE_COLOR), 0);
    lv_obj_set_style_border_width(compass_box, 2, 0);

    lv_obj_t *compass_label = lv_label_create(compass_box);
    lv_label_set_text(compass_label, "    N\n  W + E\n    S\n\nHdg: 045 deg (NE)");
    lv_obj_set_style_text_color(compass_label, lv_color_white(), 0);
    lv_obj_center(compass_label);

    // Right side - GPS Position Data
    lv_obj_t *gps_container = lv_obj_create(screen);
    lv_obj_set_size(gps_container, 340, 320);
    lv_obj_align(gps_container, LV_ALIGN_RIGHT_MID, -30, -20);
    lv_obj_set_style_bg_color(gps_container, lv_color_hex(0x003366), 0);
    lv_obj_set_style_border_color(gps_container, lv_color_hex(TITLE_COLOR), 0);
    lv_obj_set_style_border_width(gps_container, 2, 0);

    lv_obj_t *gps_label = lv_label_create(gps_container);
    lv_label_set_text(gps_label,
        "GPS POSITION\n"
        "Lat: 30.031355 deg N\n"
        "Lon: 90.034512 deg W\n"
        "Alt: 5.2 m\n\n"
        "VELOCITY\n"
        "SOG: 0.2 kts\n"
        "COG: 045 deg\n\n"
        "QUALITY\n"
        "Sats: 8\n"
        "HDOP: 1.2\n"
        "PDOP: 2.1\n\n"
        "Last Update: 12:34:56");
    lv_obj_set_style_text_color(gps_label, lv_color_white(), 0);
    lv_obj_align(gps_label, LV_ALIGN_TOP_LEFT, 10, 10);

    // Create footer
    lv_obj_t *footer = ui_footer_create(screen, PAGE_INFO, page_callback);
    if (footer != NULL) {
        ui_footer_show(footer);
        lv_obj_move_foreground(footer);
    }
    if (footer_out != NULL) {
        *footer_out = footer;
    }

    ESP_LOGI(TAG, "Created INFO screen with GPS and compass data");
    return screen;
}

/**
 * PGN SCREEN - NMEA 2000 Monitor
 */
lv_obj_t* create_pgn_screen(ui_footer_page_cb_t page_callback, lv_obj_t **footer_out) {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(SCREEN_BG_COLOR), 0);

    // Title
    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "N2K PGN MONITOR");
    lv_obj_set_style_text_font(title, &orbitron_variablefont_wght_24, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(TITLE_COLOR), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

    // Message list container
    lv_obj_t *msg_container = lv_obj_create(screen);
    lv_obj_set_size(msg_container, 740, 340);
    lv_obj_align(msg_container, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_bg_color(msg_container, lv_color_hex(0x002040), 0);
    lv_obj_set_style_border_color(msg_container, lv_color_hex(TITLE_COLOR), 0);
    lv_obj_set_style_border_width(msg_container, 2, 0);
    lv_obj_set_scrollbar_mode(msg_container, LV_SCROLLBAR_MODE_AUTO);

    // Sample PGN messages
    lv_obj_t *msg_label = lv_label_create(msg_container);
    lv_label_set_text(msg_label,
        "PGN 129029 [GPS] Src: 0x15\n"
        "Lat: 30.031355 Lon: -90.034512 Sats: 8\n"
        "12:34:56.123\n"
        "----------------------------------------\n\n"
        "PGN 127250 [Heading] Src: 0x20\n"
        "HDG: 045.3 deg Variation: -5.2 deg\n"
        "12:34:55.891\n"
        "----------------------------------------\n\n"
        "PGN 130306 [Wind] Src: 0x30\n"
        "Speed: 12.5 kts Dir: 270 deg (Relative)\n"
        "12:34:55.456\n"
        "----------------------------------------\n\n"
        "[Auto-scroll, 20 message buffer]");
    lv_obj_set_style_text_color(msg_label, lv_color_white(), 0);
    lv_obj_align(msg_label, LV_ALIGN_TOP_LEFT, 10, 10);

    // Create footer
    lv_obj_t *footer = ui_footer_create(screen, PAGE_PGN, page_callback);
    if (footer != NULL) {
        ui_footer_show(footer);
        lv_obj_move_foreground(footer);
    }
    if (footer_out != NULL) {
        *footer_out = footer;
    }

    ESP_LOGI(TAG, "Created PGN screen with message monitor");
    return screen;
}

/**
 * Button callbacks for CONFIG screen
 */
static void config_save_clicked(lv_event_t *e) {
    ESP_LOGI(TAG, "CONFIG: Save button clicked");
}

static void config_cancel_clicked(lv_event_t *e) {
    ESP_LOGI(TAG, "CONFIG: Cancel button clicked");
}

/**
 * CONFIG SCREEN - Configuration Settings
 */
lv_obj_t* create_config_screen(ui_footer_page_cb_t page_callback, lv_obj_t **footer_out) {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(SCREEN_BG_COLOR), 0);

    // Title
    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "CONFIGURATION");
    lv_obj_set_style_text_font(title, &orbitron_variablefont_wght_24, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(TITLE_COLOR), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

    // Configuration panel
    lv_obj_t *config_panel = lv_obj_create(screen);
    lv_obj_set_size(config_panel, 740, 300);
    lv_obj_align(config_panel, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_bg_color(config_panel, lv_color_hex(0x002040), 0);
    lv_obj_set_style_border_color(config_panel, lv_color_hex(TITLE_COLOR), 0);
    lv_obj_set_style_border_width(config_panel, 2, 0);
    lv_obj_set_scrollbar_mode(config_panel, LV_SCROLLBAR_MODE_AUTO);

    lv_obj_t *config_label = lv_label_create(config_panel);
    lv_label_set_text(config_label,
        "ALARM SETTINGS\n"
        "  Distance Threshold: 50 ft     [+][-]\n"
        "  Units: Feet\n\n"
        "N2K DATA SETTINGS\n"
        "  GPS PGN: 129029               [Edit]\n"
        "  Compass PGN: 127250           [Edit]\n"
        "  External GPS: [ ] Enable\n\n"
        "DISPLAY SETTINGS\n"
        "  Background: Marine Blue       [HEX]\n"
        "  Font Color: White             [HEX]\n\n"
        "SYSTEM SETTINGS\n"
        "  Boat Name: [My Boat_______]\n"
        "  WiFi: Disabled                [Enable]\n"
        "  Bluetooth: Enabled            [Disable]\n"
        "  BT Pairing Code: [1234____]");
    lv_obj_set_style_text_color(config_label, lv_color_white(), 0);
    lv_obj_align(config_label, LV_ALIGN_TOP_LEFT, 10, 10);

    // Save and Cancel buttons
    lv_obj_t *save_btn = lv_btn_create(screen);
    lv_obj_set_size(save_btn, 180, 50);
    lv_obj_align(save_btn, LV_ALIGN_BOTTOM_LEFT, 200, -80);
    lv_obj_set_style_bg_color(save_btn, lv_color_hex(BTN_READY_COLOR), 0);
    lv_obj_add_event_cb(save_btn, config_save_clicked, LV_EVENT_CLICKED, NULL);

    lv_obj_t *save_label = lv_label_create(save_btn);
    lv_label_set_text(save_label, "SAVE");
    lv_obj_center(save_label);

    lv_obj_t *cancel_btn = lv_btn_create(screen);
    lv_obj_set_size(cancel_btn, 180, 50);
    lv_obj_align(cancel_btn, LV_ALIGN_BOTTOM_RIGHT, -200, -80);
    lv_obj_set_style_bg_color(cancel_btn, lv_color_hex(BTN_OFF_COLOR), 0);
    lv_obj_add_event_cb(cancel_btn, config_cancel_clicked, LV_EVENT_CLICKED, NULL);

    lv_obj_t *cancel_label = lv_label_create(cancel_btn);
    lv_label_set_text(cancel_label, "CANCEL");
    lv_obj_center(cancel_label);

    // Create footer
    lv_obj_t *footer = ui_footer_create(screen, PAGE_CONFIG, page_callback);
    if (footer != NULL) {
        ui_footer_show(footer);
        lv_obj_move_foreground(footer);
    }
    if (footer_out != NULL) {
        *footer_out = footer;
    }

    ESP_LOGI(TAG, "Created CONFIG screen with settings");
    return screen;
}

/**
 * Button callback for UPDATE screen
 */
static void update_start_clicked(lv_event_t *e) {
    ESP_LOGI(TAG, "UPDATE: Start update button clicked");
}

/**
 * UPDATE SCREEN - Firmware Update
 */
lv_obj_t* create_update_screen(ui_footer_page_cb_t page_callback, lv_obj_t **footer_out) {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(SCREEN_BG_COLOR), 0);

    // Title
    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "FIRMWARE UPDATE");
    lv_obj_set_style_text_font(title, &orbitron_variablefont_wght_24, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(TITLE_COLOR), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

    // Update panel
    lv_obj_t *update_panel = lv_obj_create(screen);
    lv_obj_set_size(update_panel, 600, 280);
    lv_obj_align(update_panel, LV_ALIGN_CENTER, 0, -20);
    lv_obj_set_style_bg_color(update_panel, lv_color_hex(0x002040), 0);
    lv_obj_set_style_border_color(update_panel, lv_color_hex(0xFF4136), 0);  // Red border for warning
    lv_obj_set_style_border_width(update_panel, 3, 0);

    lv_obj_t *update_label = lv_label_create(update_panel);
    lv_label_set_text(update_label,
        "   !! FIRMWARE UPDATE MODE !!\n\n"
        "Status: update.bin detected on TF card\n"
        "Size: 2.4 MB\n"
        "Current Version: 0.2.0\n"
        "New Version: 0.3.0\n\n"
        "Progress: [............] 0%\n\n"
        "   WARNING:\n"
        "   * Keep device powered during update\n"
        "   * Do not remove TF card\n"
        "   * Device will restart automatically");
    lv_obj_set_style_text_color(update_label, lv_color_white(), 0);
    lv_obj_align(update_label, LV_ALIGN_TOP_MID, 0, 10);

    // Start Update button
    lv_obj_t *update_btn = lv_btn_create(screen);
    lv_obj_set_size(update_btn, 250, 55);
    lv_obj_align(update_btn, LV_ALIGN_BOTTOM_MID, 0, -80);
    lv_obj_set_style_bg_color(update_btn, lv_color_hex(0xFF4136), 0);  // Red button
    lv_obj_add_event_cb(update_btn, update_start_clicked, LV_EVENT_CLICKED, NULL);

    lv_obj_t *update_btn_label = lv_label_create(update_btn);
    lv_label_set_text(update_btn_label, "START UPDATE");
    lv_obj_set_style_text_font(update_btn_label, &orbitron_variablefont_wght_20, 0);
    lv_obj_center(update_btn_label);

    // Create footer
    lv_obj_t *footer = ui_footer_create(screen, PAGE_UPDATE, page_callback);
    if (footer != NULL) {
        ui_footer_show(footer);
        lv_obj_move_foreground(footer);
    }
    if (footer_out != NULL) {
        *footer_out = footer;
    }

    ESP_LOGI(TAG, "Created UPDATE screen with firmware update");
    return screen;
}

/**
 * Button callbacks for TOOLS screen
 */
static void tools_format_clicked(lv_event_t *e) {
    ESP_LOGI(TAG, "TOOLS: Format TF Card clicked");
}

static void tools_logs_clicked(lv_event_t *e) {
    ESP_LOGI(TAG, "TOOLS: View Logs clicked");
}

static void tools_clear_clicked(lv_event_t *e) {
    ESP_LOGI(TAG, "TOOLS: Clear GPS Tracks clicked");
}

static void tools_save_config_clicked(lv_event_t *e) {
    ESP_LOGI(TAG, "TOOLS: Save Config clicked");
}

static void tools_sysinfo_clicked(lv_event_t *e) {
    ESP_LOGI(TAG, "TOOLS: System Info clicked");
}

static void tools_test_clicked(lv_event_t *e) {
    ESP_LOGI(TAG, "TOOLS: Test Hardware clicked");
}

static void tools_load_config_clicked(lv_event_t *e) {
    ESP_LOGI(TAG, "TOOLS: Load Config clicked");
}

static void tools_reset_clicked(lv_event_t *e) {
    ESP_LOGI(TAG, "TOOLS: Factory Reset clicked");
}

/**
 * Create a tool button for TOOLS screen
 */
static lv_obj_t* create_tool_button(lv_obj_t *parent, const char *label, int x, int y,
                                    lv_event_cb_t callback) {
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 170, 70);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x0074D9), 0);
    lv_obj_set_style_radius(btn, 10, 0);
    lv_obj_add_event_cb(btn, callback, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, label);
    lv_obj_set_style_text_color(btn_label, lv_color_white(), 0);
    lv_obj_center(btn_label);

    return btn;
}

/**
 * TOOLS SCREEN - System Utilities
 */
lv_obj_t* create_tools_screen(ui_footer_page_cb_t page_callback, lv_obj_t **footer_out) {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(SCREEN_BG_COLOR), 0);

    // Title
    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "SYSTEM TOOLS");
    lv_obj_set_style_text_font(title, &orbitron_variablefont_wght_24, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(TITLE_COLOR), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

    // 8 tool buttons in 4x2 grid
    int btn_x_start = 30;
    int btn_y_start = 55;
    int btn_spacing_x = 185;
    int btn_spacing_y = 85;

    // Row 1
    create_tool_button(screen, "Format\nTF Card", btn_x_start, btn_y_start, tools_format_clicked);
    create_tool_button(screen, "View\nLogs", btn_x_start + btn_spacing_x, btn_y_start, tools_logs_clicked);
    create_tool_button(screen, "Clear\nGPS Track", btn_x_start + btn_spacing_x * 2, btn_y_start, tools_clear_clicked);
    create_tool_button(screen, "Save\nConfig", btn_x_start + btn_spacing_x * 3, btn_y_start, tools_save_config_clicked);

    // Row 2
    create_tool_button(screen, "System\nInfo", btn_x_start, btn_y_start + btn_spacing_y, tools_sysinfo_clicked);
    create_tool_button(screen, "Test\nHardware", btn_x_start + btn_spacing_x, btn_y_start + btn_spacing_y, tools_test_clicked);
    create_tool_button(screen, "Load\nConfig", btn_x_start + btn_spacing_x * 2, btn_y_start + btn_spacing_y, tools_load_config_clicked);
    create_tool_button(screen, "Factory\nReset", btn_x_start + btn_spacing_x * 3, btn_y_start + btn_spacing_y, tools_reset_clicked);

    // System status panel
    lv_obj_t *status_panel = lv_obj_create(screen);
    lv_obj_set_size(status_panel, 740, 110);
    lv_obj_align(status_panel, LV_ALIGN_BOTTOM_MID, 0, -75);
    lv_obj_set_style_bg_color(status_panel, lv_color_hex(0x002040), 0);
    lv_obj_set_style_border_color(status_panel, lv_color_hex(TITLE_COLOR), 0);
    lv_obj_set_style_border_width(status_panel, 2, 0);

    lv_obj_t *status_label = lv_label_create(status_panel);
    lv_label_set_text(status_label,
        "Firmware: 0.2.0  Uptime: 0h 12m\n"
        "Memory: 5.7 MB free\n"
        "TF Card: 32 GB (24 GB free)\n"
        "GPS Tracks: 15 files  Logs: 2.4 MB\n"
        "Config: config.json (2.1 KB)");
    lv_obj_set_style_text_color(status_label, lv_color_white(), 0);
    lv_obj_align(status_label, LV_ALIGN_TOP_LEFT, 10, 5);

    // Create footer
    lv_obj_t *footer = ui_footer_create(screen, PAGE_TOOLS, page_callback);
    if (footer != NULL) {
        ui_footer_show(footer);
        lv_obj_move_foreground(footer);
    }
    if (footer_out != NULL) {
        *footer_out = footer;
    }

    ESP_LOGI(TAG, "Created TOOLS screen with 8 utility buttons");
    return screen;
}
