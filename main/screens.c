/**
 * Screens Implementation
 *
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2025-12-25
 * Version: 0.2.0
 *
 * Screen creation functions for all app screens
 * Uses centralized ui_theme.h for colors and fonts
 */

#include <stdio.h>
#include <math.h>
#include "screens.h"
#include "ui_theme.h"
#include "ui_header.h"
#include "datetime_settings.h"
#include "power_management.h"
#include "sd_card.h"
#include "esp_log.h"

static const char *TAG = "screens";

/**
 * Button event handlers for START screen
 */
static void btn_off_clicked(lv_event_t *e) {
    ESP_LOGI(TAG, "OFF button clicked - Entering deep sleep (power off)");
    ESP_LOGI(TAG, "Device will wake on touch screen press");

    // Enter deep sleep mode
    // Device will wake when touch screen is pressed
    power_mgmt_sleep();

    // Never reaches here - device resets on wake
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
    lv_obj_set_size(btn, BUTTON_WIDTH_LARGE, BUTTON_HEIGHT_MEDIUM);
    lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, y_offset);

    // Apply theme styling
    THEME_STYLE_BUTTON(btn, bg_color);

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
    THEME_STYLE_TEXT(label, COLOR_TEXT_PRIMARY, FONT_BUTTON_SMALL);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(label);

    return btn;
}

/**
 * Create a base screen with header, title and footer
 */
static lv_obj_t* create_base_screen(const char *title, ui_page_t page, ui_footer_page_cb_t page_callback, lv_obj_t **footer_out) {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(THEME_SCREEN_BG), 0);

    // Create header bar with satellite icon
    lv_obj_t *header = ui_header_create(screen);
    ui_header_set_gps_status(header, false);  // Default to GPS not found

    // Create title label (positioned below header)
    lv_obj_t *title_label = lv_label_create(screen);
    lv_label_set_text(title_label, title);
    THEME_STYLE_TEXT(title_label, THEME_TITLE_COLOR, FONT_TITLE);
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, HEADER_HEIGHT + SPACING_MARGIN_SMALL);

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
    lv_obj_set_style_bg_color(screen, lv_color_hex(THEME_START_SCREEN_BG), 0);

    // Create header bar with satellite icon
    lv_obj_t *header = ui_header_create(screen);
    ui_header_set_gps_status(header, false);  // Default to GPS not found

    // Create "SELECT MODE" title (positioned below header)
    lv_obj_t *title_label = lv_label_create(screen);
    lv_label_set_text(title_label, "SELECT MODE");
    THEME_STYLE_TEXT(title_label, COLOR_TEXT_PRIMARY, FONT_TITLE);
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, HEADER_HEIGHT + 10);

    // Create subtitle
    lv_obj_t *subtitle_label = lv_label_create(screen);
    lv_label_set_text(subtitle_label, "Choose your operating mode");
    THEME_STYLE_TEXT(subtitle_label, THEME_SUBTITLE_COLOR, FONT_SUBTITLE);
    lv_obj_align(subtitle_label, LV_ALIGN_TOP_MID, 0, HEADER_HEIGHT + 40);

    // Create 4 mode selection buttons (plain text, no icons)
    // Improved layout: 50px tall buttons with 25px spacing between them
    create_mode_button(screen, "", "OFF", "System Disabled",
                       COLOR_BTN_OFF, btn_off_clicked, 145);

    create_mode_button(screen, "", "READY", "Activate Anchor Monitoring",
                       COLOR_BTN_READY, btn_ready_clicked, 220);

    create_mode_button(screen, "", "INFO", "View GPS & Compass Details",
                       COLOR_BTN_INFO, btn_info_clicked, 295);

    create_mode_button(screen, "", "CONFIG", "Configure System Settings",
                       COLOR_BTN_CONFIG, btn_config_clicked, 370);

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
    lv_obj_set_style_bg_color(screen, lv_color_hex(THEME_SCREEN_BG), 0);

    // Create header bar with satellite icon
    lv_obj_t *header = ui_header_create(screen);
    ui_header_set_gps_status(header, false);  // Default to GPS not found

    // Title (positioned below header)
    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "POSITION & NAVIGATION");
    THEME_STYLE_TEXT(title, THEME_TITLE_COLOR, FONT_TITLE);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, HEADER_HEIGHT + SPACING_MARGIN_SMALL);

    // Left side - Compass Rose placeholder
    lv_obj_t *compass_box = lv_obj_create(screen);
    lv_obj_set_size(compass_box, 180, 180);
    lv_obj_align(compass_box, LV_ALIGN_TOP_LEFT, 30, 130);
    THEME_STYLE_PANEL(compass_box, THEME_PANEL_BG);

    lv_obj_t *compass_label = lv_label_create(compass_box);
    lv_label_set_text(compass_label, "    N\n  W + E\n    S\n\nHdg: 045 deg (NE)");
    THEME_STYLE_TEXT(compass_label, COLOR_TEXT_PRIMARY, FONT_BODY_NORMAL);
    lv_obj_center(compass_label);

    // Right side - GPS Position Data
    lv_obj_t *gps_container = lv_obj_create(screen);
    lv_obj_set_size(gps_container, 340, 280);
    lv_obj_align(gps_container, LV_ALIGN_TOP_RIGHT, -30, 130);
    THEME_STYLE_PANEL(gps_container, THEME_PANEL_BG);

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
    THEME_STYLE_TEXT(gps_label, COLOR_TEXT_PRIMARY, FONT_BODY_NORMAL);
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
    lv_obj_set_style_bg_color(screen, lv_color_hex(THEME_SCREEN_BG), 0);

    // Create header bar with satellite icon
    lv_obj_t *header = ui_header_create(screen);
    ui_header_set_gps_status(header, false);  // Default to GPS not found

    // Title (positioned below header)
    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "N2K PGN MONITOR");
    THEME_STYLE_TEXT(title, THEME_TITLE_COLOR, FONT_TITLE);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, HEADER_HEIGHT + SPACING_MARGIN_SMALL);

    // Message list container
    lv_obj_t *msg_container = lv_obj_create(screen);
    lv_obj_set_size(msg_container, 740, 280);
    lv_obj_align(msg_container, LV_ALIGN_TOP_MID, 0, 120);
    THEME_STYLE_PANEL(msg_container, THEME_PANEL_BG_DARK);
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
    THEME_STYLE_TEXT(msg_label, COLOR_TEXT_PRIMARY, FONT_BODY_NORMAL);
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
    lv_obj_set_style_bg_color(screen, lv_color_hex(THEME_SCREEN_BG), 0);

    // Create header bar with satellite icon
    lv_obj_t *header = ui_header_create(screen);
    ui_header_set_gps_status(header, false);  // Default to GPS not found

    // Title (positioned below header)
    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "CONFIGURATION");
    THEME_STYLE_TEXT(title, THEME_TITLE_COLOR, FONT_TITLE);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, HEADER_HEIGHT + SPACING_MARGIN_SMALL);

    // Configuration panel
    lv_obj_t *config_panel = lv_obj_create(screen);
    lv_obj_set_size(config_panel, 740, 240);
    lv_obj_align(config_panel, LV_ALIGN_TOP_MID, 0, 120);
    THEME_STYLE_PANEL(config_panel, THEME_PANEL_BG_DARK);
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
    THEME_STYLE_TEXT(config_label, COLOR_TEXT_PRIMARY, FONT_BODY_NORMAL);
    lv_obj_align(config_label, LV_ALIGN_TOP_LEFT, 10, 10);

    // Save and Cancel buttons
    lv_obj_t *save_btn = lv_btn_create(screen);
    lv_obj_set_size(save_btn, 180, BUTTON_HEIGHT_MEDIUM);
    lv_obj_align(save_btn, LV_ALIGN_BOTTOM_LEFT, 200, -80);
    THEME_STYLE_BUTTON(save_btn, THEME_BTN_SUCCESS);
    lv_obj_add_event_cb(save_btn, config_save_clicked, LV_EVENT_CLICKED, NULL);

    lv_obj_t *save_label = lv_label_create(save_btn);
    lv_label_set_text(save_label, "SAVE");
    THEME_STYLE_TEXT(save_label, COLOR_TEXT_PRIMARY, FONT_BUTTON_SMALL);
    lv_obj_center(save_label);

    lv_obj_t *cancel_btn = lv_btn_create(screen);
    lv_obj_set_size(cancel_btn, 180, BUTTON_HEIGHT_MEDIUM);
    lv_obj_align(cancel_btn, LV_ALIGN_BOTTOM_RIGHT, -200, -80);
    THEME_STYLE_BUTTON(cancel_btn, THEME_BTN_CANCEL);
    lv_obj_add_event_cb(cancel_btn, config_cancel_clicked, LV_EVENT_CLICKED, NULL);

    lv_obj_t *cancel_label = lv_label_create(cancel_btn);
    lv_label_set_text(cancel_label, "CANCEL");
    THEME_STYLE_TEXT(cancel_label, COLOR_TEXT_PRIMARY, FONT_BUTTON_SMALL);
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
    lv_obj_set_style_bg_color(screen, lv_color_hex(THEME_SCREEN_BG), 0);

    // Create header bar with satellite icon
    lv_obj_t *header = ui_header_create(screen);
    ui_header_set_gps_status(header, false);  // Default to GPS not found

    // Title (positioned below header)
    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "FIRMWARE UPDATE");
    THEME_STYLE_TEXT(title, THEME_TITLE_COLOR, FONT_TITLE);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, HEADER_HEIGHT + SPACING_MARGIN_SMALL);

    // Update panel with red warning border
    lv_obj_t *update_panel = lv_obj_create(screen);
    lv_obj_set_size(update_panel, 600, 280);
    lv_obj_align(update_panel, LV_ALIGN_CENTER, 0, -20);
    lv_obj_set_style_bg_color(update_panel, lv_color_hex(THEME_PANEL_BG_DARK), 0);
    lv_obj_set_style_border_color(update_panel, lv_color_hex(COLOR_BORDER_WARNING), 0);
    lv_obj_set_style_border_width(update_panel, BORDER_WIDTH_THICK, 0);
    lv_obj_set_style_radius(update_panel, RADIUS_SMALL, 0);

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
    THEME_STYLE_TEXT(update_label, COLOR_TEXT_PRIMARY, FONT_BODY_NORMAL);
    lv_obj_align(update_label, LV_ALIGN_TOP_MID, 0, 10);

    // Start Update button (red for danger/warning)
    lv_obj_t *update_btn = lv_btn_create(screen);
    lv_obj_set_size(update_btn, 250, 55);
    lv_obj_align(update_btn, LV_ALIGN_BOTTOM_MID, 0, -80);
    THEME_STYLE_BUTTON(update_btn, THEME_BTN_DANGER);
    lv_obj_add_event_cb(update_btn, update_start_clicked, LV_EVENT_CLICKED, NULL);

    lv_obj_t *update_btn_label = lv_label_create(update_btn);
    lv_label_set_text(update_btn_label, "START UPDATE");
    THEME_STYLE_TEXT(update_btn_label, COLOR_TEXT_PRIMARY, FONT_BUTTON_LARGE);
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
 * Forward declarations for file browser
 */
static lv_obj_t* create_file_browser_screen(void);

/**
 * Format confirmation callback
 */
static void format_confirm_clicked(lv_event_t *e) {
    lv_obj_t *mbox = lv_event_get_current_target(e);
    uint32_t btn_id = lv_msgbox_get_active_btn(mbox);

    ESP_LOGI(TAG, "TF CARD: Format dialog button clicked: %ld", btn_id);

    // Close message box
    lv_obj_del(mbox);

    // Check which button was pressed (0 = Yes, 1 = No)
    if (btn_id == 0) {
        ESP_LOGI(TAG, "TF CARD: Format confirmed - formatting...");

        // Perform format
        bool success = sd_card_format();

        // Show result
        lv_obj_t *result_mbox = lv_msgbox_create(lv_scr_act(),
            success ? "Success" : "Error",
            success ? "TF Card formatted successfully!" : "Failed to format TF Card. Check card and try again.",
            NULL, true);
        lv_obj_center(result_mbox);
    } else {
        ESP_LOGI(TAG, "TF CARD: Format cancelled");
    }
}

/**
 * TF Card submenu button callbacks
 */
static void tfcard_format_clicked(lv_event_t *e) {
    ESP_LOGI(TAG, "TF CARD: Format clicked");

    // Check if SD card is mounted
    if (!sd_card_is_mounted()) {
        ESP_LOGI(TAG, "SD card not mounted, attempting to mount...");
        if (!sd_card_init()) {
            lv_obj_t *mbox = lv_msgbox_create(lv_scr_act(), "Error",
                "Failed to access TF Card.\nPlease insert card and try again.",
                NULL, true);
            lv_obj_center(mbox);
            return;
        }
    }

    // Create confirmation dialog with Yes/No buttons
    static const char *btns[] = {"Yes", "No", ""};
    lv_obj_t *mbox = lv_msgbox_create(lv_scr_act(), "Format TF Card",
        "Are you sure you want to format the TF Card?\nAll data will be lost!",
        btns, false);
    lv_obj_center(mbox);

    // Add event handler for the message box itself (captures button clicks)
    lv_obj_add_event_cb(mbox, format_confirm_clicked, LV_EVENT_VALUE_CHANGED, NULL);
}

static void tfcard_contents_clicked(lv_event_t *e) {
    ESP_LOGI(TAG, "TF CARD: Show Contents clicked");

    // Check if SD card is mounted
    if (!sd_card_is_mounted()) {
        ESP_LOGI(TAG, "SD card not mounted, attempting to mount...");
        if (!sd_card_init()) {
            lv_obj_t *mbox = lv_msgbox_create(lv_scr_act(), "Error",
                "Failed to access TF Card.\nPlease insert card and try again.",
                NULL, true);
            lv_obj_center(mbox);
            return;
        }
    }

    // Open file browser
    lv_obj_t *browser_screen = create_file_browser_screen();
    lv_scr_load(browser_screen);
}

static void tfcard_back_clicked(lv_event_t *e) {
    ESP_LOGI(TAG, "TF CARD: Back to TOOLS");
    // TODO: Navigate back to TOOLS screen
}

/**
 * TF CARD SUBMENU SCREEN
 */
static lv_obj_t* create_tfcard_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(THEME_SCREEN_BG), 0);

    // Create header
    lv_obj_t *header = ui_header_create(screen);
    ui_header_set_gps_status(header, false);

    // Title
    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "TF CARD TOOLS");
    THEME_STYLE_TEXT(title, THEME_TITLE_COLOR, FONT_TITLE);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, HEADER_HEIGHT + SPACING_MARGIN_SMALL);

    // Format button
    lv_obj_t *format_btn = lv_btn_create(screen);
    lv_obj_set_size(format_btn, 300, 80);
    lv_obj_align(format_btn, LV_ALIGN_CENTER, 0, -60);
    THEME_STYLE_BUTTON(format_btn, THEME_BTN_DANGER);
    lv_obj_add_event_cb(format_btn, tfcard_format_clicked, LV_EVENT_CLICKED, NULL);

    lv_obj_t *format_label = lv_label_create(format_btn);
    lv_label_set_text(format_label, "FORMAT CARD");
    THEME_STYLE_TEXT(format_label, COLOR_TEXT_PRIMARY, FONT_BUTTON_LARGE);
    lv_obj_center(format_label);

    // Show Contents button
    lv_obj_t *contents_btn = lv_btn_create(screen);
    lv_obj_set_size(contents_btn, 300, 80);
    lv_obj_align(contents_btn, LV_ALIGN_CENTER, 0, 40);
    THEME_STYLE_BUTTON(contents_btn, THEME_BTN_PRIMARY);
    lv_obj_add_event_cb(contents_btn, tfcard_contents_clicked, LV_EVENT_CLICKED, NULL);

    lv_obj_t *contents_label = lv_label_create(contents_btn);
    lv_label_set_text(contents_label, "SHOW CONTENTS");
    THEME_STYLE_TEXT(contents_label, COLOR_TEXT_PRIMARY, FONT_BUTTON_LARGE);
    lv_obj_center(contents_label);

    // Back button
    lv_obj_t *back_btn = lv_btn_create(screen);
    lv_obj_set_size(back_btn, 200, 60);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_MID, 0, -20);
    THEME_STYLE_BUTTON(back_btn, COLOR_BTN_CONFIG);
    lv_obj_add_event_cb(back_btn, tfcard_back_clicked, LV_EVENT_CLICKED, NULL);

    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "BACK TO TOOLS");
    THEME_STYLE_TEXT(back_label, COLOR_TEXT_PRIMARY, FONT_BUTTON_SMALL);
    lv_obj_center(back_label);

    ESP_LOGI(TAG, "Created TF CARD submenu screen");
    return screen;
}

/**
 * FILE BROWSER SCREEN
 */
static void browser_back_clicked(lv_event_t *e) {
    ESP_LOGI(TAG, "FILE BROWSER: Back clicked");
    // Return to TF Card menu
    lv_obj_t *tfcard_screen = create_tfcard_screen();
    lv_scr_load(tfcard_screen);
}

static lv_obj_t* create_file_browser_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(THEME_SCREEN_BG), 0);

    // Create header
    lv_obj_t *header = ui_header_create(screen);
    ui_header_set_gps_status(header, false);

    // Title
    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "TF CARD BROWSER");
    THEME_STYLE_TEXT(title, THEME_TITLE_COLOR, FONT_TITLE);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, HEADER_HEIGHT + SPACING_MARGIN_SMALL);

    // Get SD card space info
    uint64_t total_bytes, free_bytes;
    bool has_space_info = sd_card_get_space(&total_bytes, &free_bytes);

    // Info label (card size and free space)
    lv_obj_t *info_label = lv_label_create(screen);
    if (has_space_info) {
        lv_label_set_text_fmt(info_label, "Total: %.1f MB  Free: %.1f MB",
            total_bytes / (1024.0 * 1024.0), free_bytes / (1024.0 * 1024.0));
    } else {
        lv_label_set_text(info_label, "Card info unavailable");
    }
    THEME_STYLE_TEXT(info_label, COLOR_TEXT_SECONDARY, FONT_BODY_NORMAL);
    lv_obj_align(info_label, LV_ALIGN_TOP_MID, 0, HEADER_HEIGHT + 50);

    // Create scrollable list for files
    lv_obj_t *list = lv_list_create(screen);
    lv_obj_set_size(list, 740, 260);
    lv_obj_align(list, LV_ALIGN_TOP_MID, 0, HEADER_HEIGHT + 80);
    lv_obj_set_style_bg_color(list, lv_color_hex(THEME_PANEL_BG), 0);

    // List files from SD card
    sd_file_info_t files[50];
    int file_count = 0;

    if (sd_card_list_dir("", files, 50, &file_count)) {
        if (file_count == 0) {
            lv_obj_t *empty = lv_list_add_text(list, "No files found");
            lv_obj_set_style_text_color(empty, lv_color_hex(COLOR_TEXT_SECONDARY), 0);
        } else {
            for (int i = 0; i < file_count; i++) {
                char label[300];
                if (files[i].is_directory) {
                    snprintf(label, sizeof(label), "[DIR]  %s", files[i].name);
                } else {
                    // Show file size
                    if (files[i].size < 1024) {
                        snprintf(label, sizeof(label), "%s  (%llu B)",
                            files[i].name, files[i].size);
                    } else if (files[i].size < 1024 * 1024) {
                        snprintf(label, sizeof(label), "%s  (%.1f KB)",
                            files[i].name, files[i].size / 1024.0);
                    } else {
                        snprintf(label, sizeof(label), "%s  (%.1f MB)",
                            files[i].name, files[i].size / (1024.0 * 1024.0));
                    }
                }

                lv_obj_t *btn = lv_list_add_btn(list, NULL, label);
                lv_obj_set_style_text_font(btn, FONT_BODY_NORMAL, 0);
            }
        }
    } else {
        lv_obj_t *error = lv_list_add_text(list, "Error reading directory");
        lv_obj_set_style_text_color(error, lv_color_hex(COLOR_DANGER), 0);
    }

    // Back button
    lv_obj_t *back_btn = lv_btn_create(screen);
    lv_obj_set_size(back_btn, 200, 60);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_MID, 0, -20);
    THEME_STYLE_BUTTON(back_btn, COLOR_BTN_CONFIG);
    lv_obj_add_event_cb(back_btn, browser_back_clicked, LV_EVENT_CLICKED, NULL);

    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "BACK");
    THEME_STYLE_TEXT(back_label, COLOR_TEXT_PRIMARY, FONT_BUTTON_SMALL);
    lv_obj_center(back_label);

    ESP_LOGI(TAG, "Created file browser screen with %d files", file_count);
    return screen;
}

/**
 * Button callbacks for TOOLS screen
 */
static void tools_tfcard_clicked(lv_event_t *e) {
    ESP_LOGI(TAG, "TOOLS: TF Card clicked - opening TF Card submenu");
    lv_obj_t *tfcard_screen = create_tfcard_screen();
    lv_scr_load(tfcard_screen);
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

static void tools_datetime_clicked(lv_event_t *e) {
    ESP_LOGI(TAG, "TOOLS: Date/Time Settings clicked - opening datetime settings screen");

    // Create and load datetime settings screen
    lv_obj_t *datetime_screen = create_datetime_settings_screen(NULL, NULL);
    lv_scr_load(datetime_screen);
}

/**
 * Create a tool button for TOOLS screen
 */
static lv_obj_t* create_tool_button(lv_obj_t *parent, const char *label, int x, int y,
                                    lv_event_cb_t callback) {
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, BUTTON_WIDTH_SMALL, 70);
    lv_obj_set_pos(btn, x, y);
    THEME_STYLE_BUTTON(btn, THEME_BTN_PRIMARY);
    lv_obj_add_event_cb(btn, callback, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, label);
    THEME_STYLE_TEXT(btn_label, COLOR_TEXT_PRIMARY, FONT_BUTTON_SMALL);
    lv_obj_center(btn_label);

    return btn;
}

/**
 * Tool button definition for automatic grid layout
 */
typedef struct {
    const char *label;
    lv_event_cb_t callback;
} tool_button_t;

/**
 * TOOLS SCREEN - System Utilities
 */
lv_obj_t* create_tools_screen(ui_footer_page_cb_t page_callback, lv_obj_t **footer_out) {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(THEME_SCREEN_BG), 0);

    // Create header bar with satellite icon
    lv_obj_t *header = ui_header_create(screen);
    ui_header_set_gps_status(header, false);  // Default to GPS not found

    // Title (positioned below header)
    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "SYSTEM TOOLS");
    THEME_STYLE_TEXT(title, THEME_TITLE_COLOR, FONT_TITLE);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, HEADER_HEIGHT + SPACING_MARGIN_SMALL);

    // Define all tool buttons (add/remove buttons here and layout adjusts automatically)
    tool_button_t buttons[] = {
        {"TF Card", tools_tfcard_clicked},
        {"View\nLogs", tools_logs_clicked},
        {"Clear\nGPS Track", tools_clear_clicked},
        {"Save\nConfig", tools_save_config_clicked},
        {"System\nInfo", tools_sysinfo_clicked},
        {"Test\nHardware", tools_test_clicked},
        {"Load\nConfig", tools_load_config_clicked},
        {"Factory\nReset", tools_reset_clicked},
        {"Date/Time\nSettings", tools_datetime_clicked},
    };
    int button_count = sizeof(buttons) / sizeof(buttons[0]);

    // Automatic grid layout calculation
    // Screen dimensions: 800x480, Header: 80px, Footer: 60px, Title: ~40px
    const int screen_width = 800;
    const int available_height = 480 - 80 - 60 - 40;  // Screen - header - footer - title
    const int button_width = BUTTON_WIDTH_SMALL;
    const int button_height = 70;

    // Calculate optimal grid (prefer wider grids, e.g., 3x3 over 2x5)
    int cols = (int)ceil(sqrt(button_count));
    if (cols > 4) cols = 4;  // Max 4 columns to keep buttons readable
    int rows = (button_count + cols - 1) / cols;  // Round up division

    // Calculate spacing to spread buttons evenly across available space
    int total_button_width = cols * button_width;
    int total_button_height = rows * button_height;
    int spacing_x = (screen_width - total_button_width - 60) / (cols + 1);  // 60px margin
    int spacing_y = (available_height - total_button_height) / (rows + 1);

    // Ensure minimum spacing
    if (spacing_x < 20) spacing_x = 20;
    if (spacing_y < 20) spacing_y = 20;

    // Starting position (centered)
    int grid_width = total_button_width + (cols - 1) * spacing_x;
    int grid_height = total_button_height + (rows - 1) * spacing_y;
    int start_x = (screen_width - grid_width) / 2;
    int start_y = 140;  // Below title, moved down

    ESP_LOGI(TAG, "TOOLS grid: %d buttons in %dx%d layout, spacing: %dx%d",
             button_count, rows, cols, spacing_x, spacing_y);

    // Create buttons in automatic grid
    for (int i = 0; i < button_count; i++) {
        int row = i / cols;
        int col = i % cols;
        int x = start_x + col * (button_width + spacing_x);
        int y = start_y + row * (button_height + spacing_y);

        create_tool_button(screen, buttons[i].label, x, y, buttons[i].callback);
    }

    // Create footer
    lv_obj_t *footer = ui_footer_create(screen, PAGE_TOOLS, page_callback);
    if (footer != NULL) {
        ui_footer_show(footer);
        lv_obj_move_foreground(footer);
    }
    if (footer_out != NULL) {
        *footer_out = footer;
    }

    ESP_LOGI(TAG, "Created TOOLS screen with %d utility buttons", button_count);
    return screen;
}

/**
 * Button callbacks for DISPLAY screen
 */
static void display_mode_clicked(lv_event_t *e) {
    ESP_LOGI(TAG, "DISPLAY: Mode button clicked - return to START");
    // TODO: Return to START screen
}

static void display_info_clicked(lv_event_t *e) {
    ESP_LOGI(TAG, "DISPLAY: Info button clicked");
    // TODO: Navigate to INFO screen
}

static void display_config_clicked(lv_event_t *e) {
    ESP_LOGI(TAG, "DISPLAY: Config button clicked");
    // TODO: Navigate to CONFIG screen
}

static void display_anchor_clicked(lv_event_t *e) {
    ESP_LOGI(TAG, "DISPLAY: Anchor button clicked - start anchor tracking");
    // TODO: Start anchor location process (ARMING state)
}

/**
 * DISPLAY SCREEN - Main Anchor Monitoring
 */
lv_obj_t* create_display_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0xADD8E6), 0);  // Light Blue background

    // Create header bar with satellite icon
    lv_obj_t *header = ui_header_create(screen);
    ui_header_set_gps_status(header, false);  // Default to GPS not found

    // Status bar (below header)
    lv_obj_t *status_bar = lv_obj_create(screen);
    lv_obj_set_size(status_bar, 800, 40);
    lv_obj_align(status_bar, LV_ALIGN_TOP_MID, 0, HEADER_HEIGHT);
    lv_obj_set_style_bg_color(status_bar, lv_color_hex(THEME_PANEL_BG), 0);
    lv_obj_set_style_radius(status_bar, 0, 0);
    lv_obj_set_style_border_width(status_bar, 0, 0);

    lv_obj_t *mode_label = lv_label_create(status_bar);
    lv_label_set_text(mode_label, "MODE: READY");
    THEME_STYLE_TEXT(mode_label, COLOR_TEXT_PRIMARY, FONT_SUBTITLE);
    lv_obj_align(mode_label, LV_ALIGN_LEFT_MID, 20, 0);

    lv_obj_t *gps_status = lv_label_create(status_bar);
    lv_label_set_text(gps_status, "GPS: \xE2\x97\x8F");  // Green dot
    lv_obj_set_style_text_color(gps_status, lv_color_hex(COLOR_SUCCESS), 0);
    lv_obj_set_style_text_font(gps_status, FONT_SUBTITLE, 0);
    lv_obj_align(gps_status, LV_ALIGN_RIGHT_MID, -20, 0);

    // GPS Data panel (upper left)
    lv_obj_t *gps_panel = lv_obj_create(screen);
    lv_obj_set_size(gps_panel, 280, 140);
    lv_obj_align(gps_panel, LV_ALIGN_TOP_LEFT, 20, 130);
    THEME_STYLE_PANEL(gps_panel, THEME_PANEL_BG);

    lv_obj_t *gps_data = lv_label_create(gps_panel);
    lv_label_set_text(gps_data,
        "GPS POSITION\n"
        "Lat:  30.031355\n"
        "Lon: -90.034512\n"
        "Satellites: 8");
    THEME_STYLE_TEXT(gps_data, COLOR_TEXT_PRIMARY, FONT_BODY_NORMAL);
    lv_obj_align(gps_data, LV_ALIGN_TOP_LEFT, 10, 10);

    // Compass panel (upper right)
    lv_obj_t *compass_panel = lv_obj_create(screen);
    lv_obj_set_size(compass_panel, 140, 140);
    lv_obj_align(compass_panel, LV_ALIGN_TOP_RIGHT, -20, 130);
    THEME_STYLE_PANEL(compass_panel, THEME_PANEL_BG);

    lv_obj_t *compass_label = lv_label_create(compass_panel);
    lv_label_set_text(compass_label, "    N\n  W + E\n    S");
    THEME_STYLE_TEXT(compass_label, COLOR_TEXT_PRIMARY, FONT_BODY_LARGE);
    lv_obj_center(compass_label);

    // Large Anchor button (center)
    lv_obj_t *anchor_btn = lv_btn_create(screen);
    lv_obj_set_size(anchor_btn, 200, 200);
    lv_obj_align(anchor_btn, LV_ALIGN_CENTER, 0, -10);
    THEME_STYLE_BUTTON(anchor_btn, COLOR_PRIMARY);
    lv_obj_add_event_cb(anchor_btn, display_anchor_clicked, LV_EVENT_CLICKED, NULL);

    lv_obj_t *anchor_icon = lv_label_create(anchor_btn);
    lv_label_set_text(anchor_icon, "\xE2\x9A\x93");  // ⚓ Anchor emoji
    THEME_STYLE_TEXT(anchor_icon, COLOR_TEXT_PRIMARY, &orbitron_variablefont_wght_48);
    lv_obj_center(anchor_icon);

    lv_obj_t *anchor_text = lv_label_create(screen);
    lv_label_set_text(anchor_text, "Press to set anchor location");
    THEME_STYLE_TEXT(anchor_text, COLOR_TEXT_INVERSE, FONT_BODY_NORMAL);
    lv_obj_align(anchor_text, LV_ALIGN_CENTER, 0, 120);

    // Connection info (bottom left)
    lv_obj_t *connection_label = lv_label_create(screen);
    lv_label_set_text(connection_label, "N2K Connected: 127.0.0.1:10110");
    THEME_STYLE_TEXT(connection_label, COLOR_TEXT_INVERSE, FONT_LABEL);
    lv_obj_align(connection_label, LV_ALIGN_BOTTOM_LEFT, 20, -70);

    // Time display (bottom right)
    lv_obj_t *time_label = lv_label_create(screen);
    lv_label_set_text(time_label, "12:34:56");
    THEME_STYLE_TEXT(time_label, COLOR_TEXT_INVERSE, FONT_BODY_LARGE);
    lv_obj_align(time_label, LV_ALIGN_BOTTOM_RIGHT, -20, -70);

    // Bottom navigation buttons (no footer bar, just buttons)
    lv_obj_t *info_btn = lv_btn_create(screen);
    lv_obj_set_size(info_btn, 120, 50);
    lv_obj_align(info_btn, LV_ALIGN_BOTTOM_LEFT, 20, -10);
    THEME_STYLE_BUTTON(info_btn, COLOR_BTN_INFO);
    lv_obj_add_event_cb(info_btn, display_info_clicked, LV_EVENT_CLICKED, NULL);

    lv_obj_t *info_btn_label = lv_label_create(info_btn);
    lv_label_set_text(info_btn_label, "INFO");
    THEME_STYLE_TEXT(info_btn_label, COLOR_TEXT_PRIMARY, FONT_BUTTON_SMALL);
    lv_obj_center(info_btn_label);

    lv_obj_t *config_btn = lv_btn_create(screen);
    lv_obj_set_size(config_btn, 120, 50);
    lv_obj_align(config_btn, LV_ALIGN_BOTTOM_MID, 0, -10);
    THEME_STYLE_BUTTON(config_btn, COLOR_BTN_CONFIG);
    lv_obj_add_event_cb(config_btn, display_config_clicked, LV_EVENT_CLICKED, NULL);

    lv_obj_t *config_btn_label = lv_label_create(config_btn);
    lv_label_set_text(config_btn_label, "CONFIG");
    THEME_STYLE_TEXT(config_btn_label, COLOR_TEXT_PRIMARY, FONT_BUTTON_SMALL);
    lv_obj_center(config_btn_label);

    lv_obj_t *mode_btn = lv_btn_create(screen);
    lv_obj_set_size(mode_btn, 120, 50);
    lv_obj_align(mode_btn, LV_ALIGN_BOTTOM_RIGHT, -20, -10);
    THEME_STYLE_BUTTON(mode_btn, COLOR_BTN_OFF);
    lv_obj_add_event_cb(mode_btn, display_mode_clicked, LV_EVENT_CLICKED, NULL);

    lv_obj_t *mode_btn_label = lv_label_create(mode_btn);
    lv_label_set_text(mode_btn_label, "MODE");
    THEME_STYLE_TEXT(mode_btn_label, COLOR_TEXT_PRIMARY, FONT_BUTTON_SMALL);
    lv_obj_center(mode_btn_label);

    ESP_LOGI(TAG, "Created DISPLAY screen (Ready to Anchor)");
    return screen;
}

/**
 * Button callbacks for TEST screen
 */
static void test_buzzer_clicked(lv_event_t *e) {
    ESP_LOGI(TAG, "TEST: Buzzer toggle clicked");
    // TODO: Toggle buzzer
}

static void test_relay_clicked(lv_event_t *e) {
    ESP_LOGI(TAG, "TEST: Relay toggle clicked");
    // TODO: Toggle relay
}

static void test_alarm_clicked(lv_event_t *e) {
    ESP_LOGI(TAG, "TEST: Alarm toggle clicked");
    // TODO: Toggle alarm state
}

static void test_led_clicked(lv_event_t *e) {
    ESP_LOGI(TAG, "TEST: LED cycle clicked");
    // TODO: Cycle LED states
}

static void test_back_clicked(lv_event_t *e) {
    ESP_LOGI(TAG, "TEST: Back to TOOLS clicked");
    // TODO: Return to TOOLS screen
}

/**
 * Create a hardware test toggle button
 */
static lv_obj_t* create_test_toggle(lv_obj_t *parent, const char *label, const char *state,
                                     uint32_t color, int x, int y, lv_event_cb_t callback) {
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 140, 80);
    lv_obj_set_pos(btn, x, y);
    THEME_STYLE_BUTTON(btn, color);
    lv_obj_add_event_cb(btn, callback, LV_EVENT_CLICKED, NULL);

    lv_obj_t *title = lv_label_create(btn);
    lv_label_set_text(title, label);
    THEME_STYLE_TEXT(title, COLOR_TEXT_PRIMARY, FONT_LABEL);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t *state_label = lv_label_create(btn);
    lv_label_set_text(state_label, state);
    THEME_STYLE_TEXT(state_label, COLOR_TEXT_PRIMARY, FONT_BUTTON_LARGE);
    lv_obj_align(state_label, LV_ALIGN_BOTTOM_MID, 0, -10);

    return btn;
}

/**
 * TEST SCREEN - Hardware Testing
 */
lv_obj_t* create_test_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(THEME_SCREEN_BG), 0);

    // Create header bar with satellite icon
    lv_obj_t *header = ui_header_create(screen);
    ui_header_set_gps_status(header, false);  // Default to GPS not found

    // Title (positioned below header)
    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "HARDWARE TEST");
    THEME_STYLE_TEXT(title, THEME_TITLE_COLOR, FONT_TITLE);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, HEADER_HEIGHT + SPACING_MARGIN_SMALL);

    // Left column - Hardware controls
    create_test_toggle(screen, "BUZZER:", "OFF", COLOR_BTN_OFF, 30, 120, test_buzzer_clicked);
    create_test_toggle(screen, "RELAY:", "OFF", COLOR_BTN_OFF, 30, 215, test_relay_clicked);
    create_test_toggle(screen, "ALARM:", "OFF", COLOR_BTN_OFF, 30, 310, test_alarm_clicked);

    // Right column - Data sources panel
    lv_obj_t *data_panel = lv_obj_create(screen);
    lv_obj_set_size(data_panel, 590, 285);
    lv_obj_align(data_panel, LV_ALIGN_TOP_RIGHT, -30, 120);
    THEME_STYLE_PANEL(data_panel, THEME_PANEL_BG_DARK);

    lv_obj_t *data_title = lv_label_create(data_panel);
    lv_label_set_text(data_title, "DATA SOURCES");
    THEME_STYLE_TEXT(data_title, THEME_TITLE_COLOR, FONT_SUBTITLE);
    lv_obj_align(data_title, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t *data_status = lv_label_create(data_panel);
    lv_label_set_text(data_status,
        "GPS: ACTIVE (N2K)\n"
        "Sats: 8 | Lat: 30.031355\n\n"
        "COMPASS: NO DATA\n\n"
        "WIND: NO DATA\n\n"
        "WATER SPEED: NO DATA\n\n\n"
        "[Updates every 1 second]");
    THEME_STYLE_TEXT(data_status, COLOR_TEXT_PRIMARY, FONT_BODY_NORMAL);
    lv_obj_align(data_status, LV_ALIGN_TOP_LEFT, 20, 50);

    // Back button
    lv_obj_t *back_btn = lv_btn_create(screen);
    lv_obj_set_size(back_btn, 200, 50);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_LEFT, 30, -10);
    THEME_STYLE_BUTTON(back_btn, THEME_BTN_PRIMARY);
    lv_obj_add_event_cb(back_btn, test_back_clicked, LV_EVENT_CLICKED, NULL);

    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "\xE2\x97\x80 BACK TO TOOLS");  // ◀ arrow
    THEME_STYLE_TEXT(back_label, COLOR_TEXT_PRIMARY, FONT_BUTTON_SMALL);
    lv_obj_center(back_label);

    ESP_LOGI(TAG, "Created TEST screen with hardware controls");
    return screen;
}
