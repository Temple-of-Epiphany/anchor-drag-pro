/**
 * START Screen Implementation
 *
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2025-12-22
 * Date Updated: 2025-12-22
 * Version: 0.1.0
 */

#include "start_screen.h"
#include "ui_header.h"
#include "ui_footer.h"
#include "ui_version.h"
#include "lvgl_init.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "start_screen";

// LVGL UI objects
static lv_obj_t *start_screen = NULL;
static lv_obj_t *header = NULL;
static lv_obj_t *footer = NULL;
static lv_obj_t *status_label = NULL;
static lv_obj_t *gps_status_label = NULL;
static lv_obj_t *system_status_label = NULL;
static lv_obj_t *info_label = NULL;
static lv_obj_t *test_button = NULL;
static int button_press_count = 0;

/**
 * Test button event handler
 */
static void test_button_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) {
        button_press_count++;
        ESP_LOGI(TAG, "========================================");
        ESP_LOGI(TAG, "TEST BUTTON CLICKED! Count: %d", button_press_count);
        ESP_LOGI(TAG, "Touch input is working correctly!");
        ESP_LOGI(TAG, "========================================");

        // Update button label to show press count
        lv_obj_t *label = lv_obj_get_child(test_button, 0);
        if (label != NULL) {
            char btn_text[32];
            snprintf(btn_text, sizeof(btn_text), "PRESSED %d TIMES", button_press_count);
            lv_label_set_text(label, btn_text);
        }
    }
}

/**
 * Footer page navigation callback
 */
static void footer_page_callback(ui_page_t page) {
    ESP_LOGI(TAG, "Footer navigation: switching to page %d", page);
    // TODO: Implement actual page navigation when other screens are created
    // For now, just log the navigation request
}

/**
 * Create and display the START screen
 */
esp_err_t start_screen_create(void) {
    ESP_LOGI(TAG, "=== CREATING START SCREEN ===");

    if (!lvgl_lock(1000)) {
        ESP_LOGE(TAG, "ERROR: Failed to lock LVGL mutex for START screen creation");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "LVGL mutex locked successfully");

    // Create screen
    ESP_LOGI(TAG, "Creating screen object...");
    start_screen = lv_obj_create(NULL);
    if (start_screen == NULL) {
        ESP_LOGE(TAG, "ERROR: Failed to create START screen object");
        lvgl_unlock();
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Screen object created successfully");

    // Set background color
    lv_obj_set_style_bg_color(start_screen, lv_color_hex(0x001F3F), 0);  // Dark blue
    lv_obj_set_style_bg_opa(start_screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(start_screen, LV_OBJ_FLAG_SCROLLABLE);
    ESP_LOGI(TAG, "Background style configured");

    // Create header bar
    ESP_LOGI(TAG, "Creating header bar...");
    header = ui_header_create(start_screen);
    if (header == NULL) {
        ESP_LOGE(TAG, "ERROR: Failed to create header");
        lvgl_unlock();
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Header created successfully");

    // Set initial GPS/Compass status (will be updated later)
    ui_header_set_gps_status(header, false);
    ui_header_set_compass_status(header, false);
    ESP_LOGI(TAG, "Header status icons initialized");

    // Create footer navigation bar (START = page 0) with callback
    ESP_LOGI(TAG, "Creating footer navigation bar...");
    footer = ui_footer_create(start_screen, PAGE_START, footer_page_callback);
    if (footer == NULL) {
        ESP_LOGE(TAG, "ERROR: Failed to create footer");
        lvgl_unlock();
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Footer created successfully");

    // Calculate content area (between header and footer)
    // Header: 80px from top
    // Footer: 60px from bottom
    // Content area: 480 - 80 - 60 = 340px
    int content_y = 80;
    int content_height = 340;

    // Main status label (centered in content area)
    status_label = lv_label_create(start_screen);
    lv_label_set_text(status_label, "ANCHOR DRAG ALARM");
    lv_obj_set_style_text_color(status_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_14, 0);
    lv_obj_align(status_label, LV_ALIGN_TOP_MID, 0, content_y + 20);

    // GPS status label
    gps_status_label = lv_label_create(start_screen);
    lv_label_set_text(gps_status_label, "GPS: Initializing...");
    lv_obj_set_style_text_color(gps_status_label, lv_color_hex(0xFFAA00), 0);
    lv_obj_align(gps_status_label, LV_ALIGN_TOP_MID, 0, content_y + 60);

    // System status label
    system_status_label = lv_label_create(start_screen);
    lv_label_set_text(system_status_label, "System: Ready");
    lv_obj_set_style_text_color(system_status_label, lv_color_hex(0x00FF00), 0);
    lv_obj_align(system_status_label, LV_ALIGN_TOP_MID, 0, content_y + 90);

    // Info label at bottom of content area
    info_label = lv_label_create(start_screen);
    lv_label_set_text(info_label, "Swipe or use navigation buttons to change screens\n"
                                   "Touch screen to interact");
    lv_obj_set_style_text_color(info_label, lv_color_hex(0xAAAAAA), 0);
    lv_obj_set_style_text_align(info_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(info_label, 600);
    lv_obj_align(info_label, LV_ALIGN_TOP_MID, 0, content_y + content_height - 80);

    // Version label in content area
    lv_obj_t *version_label = lv_label_create(start_screen);
    lv_label_set_text(version_label, "v" UI_VERSION_STRING);
    lv_obj_set_style_text_color(version_label, lv_color_hex(0x666666), 0);
    lv_obj_align(version_label, LV_ALIGN_TOP_MID, 0, content_y + 150);

    // Create test button for touch input verification
    ESP_LOGI(TAG, "Creating test button...");
    test_button = lv_btn_create(start_screen);
    lv_obj_set_size(test_button, 200, 60);
    lv_obj_align(test_button, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(test_button, test_button_event_cb, LV_EVENT_CLICKED, NULL);

    // Button label
    lv_obj_t *btn_label = lv_label_create(test_button);
    lv_label_set_text(btn_label, "TOUCH ME!");
    lv_obj_center(btn_label);

    // Style the button
    lv_obj_set_style_bg_color(test_button, lv_color_hex(0x00AA00), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(test_button, lv_color_hex(0x00FF00), LV_STATE_PRESSED);
    lv_obj_set_style_text_color(btn_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(btn_label, &lv_font_montserrat_14, 0);
    ESP_LOGI(TAG, "Test button created successfully");

    // Load the screen
    ESP_LOGI(TAG, "Loading START screen...");
    lv_scr_load(start_screen);
    ESP_LOGI(TAG, "Screen loaded");

    lvgl_unlock();
    ESP_LOGI(TAG, "LVGL mutex unlocked");

    // Force immediate redraw
    vTaskDelay(pdMS_TO_TICKS(10));
    if (lvgl_lock(100)) {
        lv_obj_invalidate(start_screen);
        lv_refr_now(lvgl_get_display());
        lvgl_unlock();
        ESP_LOGI(TAG, "Screen refresh forced");
    }

    ESP_LOGI(TAG, "=== START SCREEN CREATED AND LOADED SUCCESSFULLY ===");
    return ESP_OK;
}

/**
 * Update GPS status on START screen
 */
void start_screen_update_gps(bool gps_ready, const char *gps_source) {
    if (!lvgl_lock(100)) return;

    if (gps_status_label != NULL) {
        char gps_text[64];
        if (gps_ready) {
            snprintf(gps_text, sizeof(gps_text), "GPS: Ready (%s)", gps_source);
            lv_obj_set_style_text_color(gps_status_label, lv_color_hex(0x00FF00), 0);
        } else {
            snprintf(gps_text, sizeof(gps_text), "GPS: Not Available");
            lv_obj_set_style_text_color(gps_status_label, lv_color_hex(0xFF0000), 0);
        }
        lv_label_set_text(gps_status_label, gps_text);
    }

    // Update header GPS icon
    if (header != NULL) {
        ui_header_set_gps_status(header, gps_ready);
    }

    lvgl_unlock();
}
