/**
 * Splash Screen and Self-Test Implementation with LVGL Graphics
 *
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2025-12-19
 * Date Updated: 2025-12-20
 * Version: 0.1.1
 *
 * Changelog:
 * - 0.1.1 (2025-12-20): Added LVGL graphical splash screen
 * - 0.1.1 (2025-12-19): Implemented self-test sequence with GPS source priority
 * - 0.1.0 (2025-12-19): Initial implementation
 */

#include "splash_screen.h"
#include "ui_version.h"
#include "board_config.h"
#include "lvgl_init.h"
#include "ui_header.h"
#include "splash_logo.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "sdmmc_cmd.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <sys/stat.h>
#include <string.h>

static const char *TAG = "splash_screen";

// LVGL UI objects
static lv_obj_t *splash_screen = NULL;
static lv_obj_t *status_header = NULL;
static lv_obj_t *logo_img = NULL;
static lv_obj_t *version_label = NULL;
static lv_obj_t *progress_bar = NULL;
static lv_obj_t *progress_label = NULL;
static lv_obj_t *selftest_title_label = NULL;
static lv_obj_t *tf_card_label = NULL;
static lv_obj_t *n2k_label = NULL;
static lv_obj_t *nmea_label = NULL;
static lv_obj_t *gps_label = NULL;
static lv_obj_t *status_label = NULL;

/**
 * Create LVGL splash screen UI
 */
static void create_splash_ui(void) {
    ESP_LOGI(TAG, "Creating splash screen UI...");

    if (!lvgl_lock(1000)) {
        ESP_LOGE(TAG, "Failed to lock LVGL mutex");
        return;
    }

    // Create splash screen
    splash_screen = lv_obj_create(NULL);
    if (splash_screen == NULL) {
        ESP_LOGE(TAG, "Failed to create splash screen object");
        lvgl_unlock();
        return;
    }

    // Set background color and make it opaque (critical for rendering)
    lv_obj_set_style_bg_color(splash_screen, lv_color_hex(0x001F3F), 0);  // Dark blue
    lv_obj_set_style_bg_opa(splash_screen, LV_OPA_COVER, 0);  // Fully opaque

    // Status header (full-width bar at top with title)
    status_header = ui_header_create(splash_screen);
    ui_header_set_gps_status(status_header, false);
    ui_header_set_compass_status(status_header, false);

    // Splash logo image (compiled C array) - centered below header
    logo_img = lv_img_create(splash_screen);
    lv_img_set_src(logo_img, &splash_logo);
    lv_obj_align(logo_img, LV_ALIGN_CENTER, 0, -40);  // Centered, slight offset up

    ESP_LOGI(TAG, "Splash logo loaded: %dx%d pixels", splash_logo.header.w, splash_logo.header.h);

    // Version label - BELOW logo
    version_label = lv_label_create(splash_screen);
    lv_label_set_text(version_label, "v" UI_VERSION_STRING);
    lv_obj_set_style_text_color(version_label, lv_color_hex(0xAAAAAA), 0);
    lv_obj_set_style_text_font(version_label, &lv_font_montserrat_14, 0);
    lv_obj_align_to(version_label, logo_img, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);

    // Progress bar - BELOW version label
    progress_bar = lv_bar_create(splash_screen);
    lv_obj_set_size(progress_bar, 400, 20);
    lv_obj_align_to(progress_bar, version_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
    lv_obj_set_style_bg_color(progress_bar, lv_color_hex(0x333333), 0);  // Dark gray background
    lv_obj_set_style_bg_opa(progress_bar, LV_OPA_COVER, 0);
    lv_bar_set_range(progress_bar, 0, 100);
    lv_bar_set_value(progress_bar, 0, LV_ANIM_OFF);

    // Progress label - BELOW progress bar
    progress_label = lv_label_create(splash_screen);
    lv_label_set_text(progress_label, "Initializing...");
    lv_obj_set_style_text_color(progress_label, lv_color_hex(0xFFAA00), 0);
    lv_obj_align_to(progress_label, progress_bar, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

    // Self-test title (hidden initially)
    selftest_title_label = lv_label_create(splash_screen);
    lv_label_set_text(selftest_title_label, "Hardware Self-Test");
    lv_obj_set_style_text_color(selftest_title_label, lv_color_white(), 0);
    lv_obj_align(selftest_title_label, LV_ALIGN_TOP_MID, 0, 140);
    lv_obj_add_flag(selftest_title_label, LV_OBJ_FLAG_HIDDEN);

    // TF Card check label
    tf_card_label = lv_label_create(splash_screen);
    lv_label_set_text(tf_card_label, "TF Card: Checking...");
    lv_obj_set_style_text_color(tf_card_label, lv_color_white(), 0);
    lv_obj_align(tf_card_label, LV_ALIGN_TOP_LEFT, 60, 180);
    lv_obj_add_flag(tf_card_label, LV_OBJ_FLAG_HIDDEN);

    // N2K check label
    n2k_label = lv_label_create(splash_screen);
    lv_label_set_text(n2k_label, "N2K Data: Checking...");
    lv_obj_set_style_text_color(n2k_label, lv_color_white(), 0);
    lv_obj_align(n2k_label, LV_ALIGN_TOP_LEFT, 60, 210);
    lv_obj_add_flag(n2k_label, LV_OBJ_FLAG_HIDDEN);

    // NMEA 0183 check label
    nmea_label = lv_label_create(splash_screen);
    lv_label_set_text(nmea_label, "NMEA 0183: Checking...");
    lv_obj_set_style_text_color(nmea_label, lv_color_white(), 0);
    lv_obj_align(nmea_label, LV_ALIGN_TOP_LEFT, 60, 240);
    lv_obj_add_flag(nmea_label, LV_OBJ_FLAG_HIDDEN);

    // External GPS check label
    gps_label = lv_label_create(splash_screen);
    lv_label_set_text(gps_label, "External GPS: Checking...");
    lv_obj_set_style_text_color(gps_label, lv_color_white(), 0);
    lv_obj_align(gps_label, LV_ALIGN_TOP_LEFT, 60, 270);
    lv_obj_add_flag(gps_label, LV_OBJ_FLAG_HIDDEN);

    // Status label
    status_label = lv_label_create(splash_screen);
    lv_label_set_text(status_label, "");
    lv_obj_set_style_text_color(status_label, lv_color_hex(0x00FF00), 0);
    lv_obj_set_style_text_align(status_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(status_label, LV_ALIGN_BOTTOM_MID, 0, -30);
    lv_obj_add_flag(status_label, LV_OBJ_FLAG_HIDDEN);

    // Load splash screen (LVGL 8.x)
    lv_scr_load(splash_screen);

    lvgl_unlock();

    // Force immediate redraw in direct mode (after unlocking)
    vTaskDelay(pdMS_TO_TICKS(10));  // Small delay to allow screen load to complete

    if (lvgl_lock(100)) {
        lv_obj_invalidate(splash_screen);
        lv_refr_now(lvgl_get_display());
        lvgl_unlock();
    }

    ESP_LOGI(TAG, "Splash screen created and loaded successfully");
}

/**
 * Update progress bar
 */
static void update_progress(int percent, const char *message) {
    if (!lvgl_lock(100)) return;

    if (progress_bar != NULL) {
        lv_bar_set_value(progress_bar, percent, LV_ANIM_ON);
    }

    if (progress_label != NULL && message != NULL) {
        lv_label_set_text(progress_label, message);
    }

    lvgl_unlock();
    vTaskDelay(pdMS_TO_TICKS(50));  // Small delay for visual update
}

/**
 * Show self-test UI
 */
static void show_selftest_ui(void) {
    if (!lvgl_lock(100)) return;

    // Hide progress label during self-test
    if (progress_label != NULL) {
        lv_obj_add_flag(progress_label, LV_OBJ_FLAG_HIDDEN);
    }

    lv_obj_clear_flag(selftest_title_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(tf_card_label, LV_OBJ_FLAG_HIDDEN);

    lvgl_unlock();
}

/**
 * Update self-test label
 */
static void update_test_label(lv_obj_t *label, const char *name, bool passed, bool checking) {
    if (!lvgl_lock(100)) return;

    char text[128];
    if (checking) {
        snprintf(text, sizeof(text), "%s: Checking...", name);
        lv_obj_set_style_text_color(label, lv_color_hex(0xFFAA00), 0);  // Orange
    } else if (passed) {
        snprintf(text, sizeof(text), "%s: \xE2\x9C\x93", name);  // ✓
        lv_obj_set_style_text_color(label, lv_color_hex(0x00FF00), 0);  // Green
    } else {
        snprintf(text, sizeof(text), "%s: \xE2\x9C\x97", name);  // ✗
        lv_obj_set_style_text_color(label, lv_color_hex(0xFF0000), 0);  // Red
    }
    lv_label_set_text(label, text);
    lv_obj_clear_flag(label, LV_OBJ_FLAG_HIDDEN);

    lvgl_unlock();
}

/**
 * Print splash banner (serial only)
 */
static void print_splash_banner(void) {
    printf("\033[2J\033[H");  // Clear screen
    printf("================================================================================\n");
    printf("                          ANCHOR DRAG ALARM\n");
    printf("                              Version %s\n", UI_VERSION_STRING);
    printf("                         UI Version %s\n", UI_VERSION_STRING);
    printf("                     FW Version %s\n", FW_VERSION_STRING);
    printf("                    Build: %s %s\n", UI_BUILD_DATE, UI_BUILD_TIME);
    printf("================================================================================\n");
    printf("\n");
}

/**
 * Check for TF card presence
 */
bool check_tf_card(void) {
    ESP_LOGI(TAG, "Checking for TF card...");

    // Update UI - checking
    update_test_label(tf_card_label, "TF Card", false, true);

    // TODO: Implement actual SD card detection
    vTaskDelay(pdMS_TO_TICKS(500));

    // Placeholder: Assume no TF card for now
    ESP_LOGW(TAG, "TF card support not yet implemented");

    // Update UI - failed
    update_test_label(tf_card_label, "TF Card", false, false);

    return false;
}

/**
 * Check for update.bin on TF card
 */
bool check_update_bin(void) {
    ESP_LOGI(TAG, "Checking for update.bin...");

    // TODO: Implement actual file check
    vTaskDelay(pdMS_TO_TICKS(300));

    struct stat st;
    if (stat("/sdcard/update.bin", &st) == 0) {
        ESP_LOGI(TAG, "Found update.bin (%ld bytes)", st.st_size);
        return true;
    }

    ESP_LOGD(TAG, "update.bin not found");
    return false;
}

/**
 * Check for N2K data (NMEA 2000)
 */
bool check_n2k_data(uint32_t timeout_ms) {
    ESP_LOGI(TAG, "Checking for N2K data (NMEA 2000)...");

    // Update UI - checking
    update_test_label(n2k_label, "N2K Data", false, true);

    // TODO: Implement actual CAN/TWAI check for PGN 129029
    vTaskDelay(pdMS_TO_TICKS(timeout_ms));

    #if ENABLE_CAN_BUS
    ESP_LOGD(TAG, "CAN bus enabled, checking for PGN 129029...");
    ESP_LOGW(TAG, "N2K data check not yet implemented");

    // Update UI - failed
    update_test_label(n2k_label, "N2K Data", false, false);
    return false;
    #else
    ESP_LOGD(TAG, "CAN bus disabled in configuration");
    update_test_label(n2k_label, "N2K Data", false, false);
    return false;
    #endif
}

/**
 * Check for NMEA 0183 data
 */
bool check_nmea0183_data(uint32_t timeout_ms) {
    ESP_LOGI(TAG, "Checking for NMEA 0183 data...");

    // Update UI - checking
    update_test_label(nmea_label, "NMEA 0183", false, true);

    // TODO: Implement actual RS485 UART check
    vTaskDelay(pdMS_TO_TICKS(timeout_ms));

    #if ENABLE_RS485
    ESP_LOGD(TAG, "RS485 enabled, checking for NMEA 0183...");
    ESP_LOGW(TAG, "NMEA 0183 check not yet implemented");
    update_test_label(nmea_label, "NMEA 0183", false, false);
    return false;
    #else
    ESP_LOGD(TAG, "RS485 disabled in configuration");
    update_test_label(nmea_label, "NMEA 0183", false, false);
    return false;
    #endif
}

/**
 * Check for external GPS module (I2C)
 */
bool check_external_gps(uint32_t timeout_ms) {
    ESP_LOGI(TAG, "Checking for external GPS module...");

    // Update UI - checking
    update_test_label(gps_label, "External GPS", false, true);

    // TODO: Implement actual I2C GPS check
    vTaskDelay(pdMS_TO_TICKS(timeout_ms));

    #if ENABLE_EXTERNAL_GPS
    ESP_LOGD(TAG, "External GPS enabled, checking I2C address 0x%02X...", I2C_ADDR_NEO8M_GPS);
    ESP_LOGW(TAG, "External GPS check not yet implemented");
    update_test_label(gps_label, "External GPS", false, false);
    return false;
    #else
    ESP_LOGD(TAG, "External GPS disabled in configuration");
    update_test_label(gps_label, "External GPS", false, false);
    return false;
    #endif
}

/**
 * Run hardware self-test
 */
esp_err_t run_self_test(selftest_results_t *results) {
    if (results == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // Initialize results
    memset(results, 0, sizeof(selftest_results_t));

    ESP_LOGI(TAG, "Starting hardware self-test...");
    printf("\n");
    printf("               Self-Test:\n");

    // Show self-test UI
    show_selftest_ui();

    // Progress: 0% - Starting
    update_progress(0, "Starting self-test...");
    vTaskDelay(pdMS_TO_TICKS(500));

    // Test 1: TF Card Detection (Progress: 0% -> 25%)
    printf("                         TF Card: ");
    fflush(stdout);
    update_progress(10, "Checking TF Card...");
    results->tf_card_present = check_tf_card();
    printf("%s\n", results->tf_card_present ? "✓" : "✗");
    update_progress(25, results->tf_card_present ? "TF Card: OK" : "TF Card: Not Found");

    // If TF card present, check for update.bin
    if (results->tf_card_present) {
        printf("                         update.bin: ");
        fflush(stdout);
        results->update_bin_found = check_update_bin();
        printf("%s\n", results->update_bin_found ? "FOUND" : "Not found");
    }

    // Test 2: GPS Source Detection (Priority Order) (Progress: 25% -> 100%)
    // Priority 1: N2K Data
    printf("                         N2K Data: ");
    fflush(stdout);
    update_progress(30, "Checking N2K GPS...");
    results->n2k_available = check_n2k_data(2000);
    printf("%s\n", results->n2k_available ? "✓" : "✗");
    update_progress(50, results->n2k_available ? "N2K GPS: OK" : "N2K GPS: Not Found");

    if (results->n2k_available) {
        results->gps_ready = true;
        strncpy(results->gps_source, "NMEA 2000 (N2K)", sizeof(results->gps_source) - 1);
        printf("                         GPS Ready: ✓\n");
        ESP_LOGI(TAG, "GPS source: NMEA 2000 (highest priority)");
        update_progress(100, "GPS Ready: N2K");
        // Update header GPS status
        ui_header_set_gps_status(status_header, true);
    } else {
        // Priority 2: NMEA 0183
        printf("                         NMEA 0183: ");
        fflush(stdout);
        update_progress(60, "Checking NMEA 0183...");
        results->nmea0183_available = check_nmea0183_data(2000);
        printf("%s\n", results->nmea0183_available ? "✓" : "✗");
        update_progress(70, results->nmea0183_available ? "NMEA 0183: OK" : "NMEA 0183: Not Found");

        if (results->nmea0183_available) {
            results->gps_ready = true;
            strncpy(results->gps_source, "NMEA 0183", sizeof(results->gps_source) - 1);
            printf("                         GPS Ready: ✓\n");
            ESP_LOGI(TAG, "GPS source: NMEA 0183 (secondary priority)");
            update_progress(100, "GPS Ready: NMEA 0183");
            // Update header GPS status
            ui_header_set_gps_status(status_header, true);
        } else {
            // Priority 3: External GPS
            printf("                         External GPS: ");
            fflush(stdout);
            update_progress(80, "Checking External GPS...");
            results->external_gps_available = check_external_gps(2000);
            printf("%s\n", results->external_gps_available ? "✓" : "✗");
            update_progress(90, results->external_gps_available ? "External GPS: OK" : "External GPS: Not Found");

            if (results->external_gps_available) {
                results->gps_ready = true;
                strncpy(results->gps_source, "External GPS (I2C)", sizeof(results->gps_source) - 1);
                printf("                         GPS Ready: ✓\n");
                ESP_LOGI(TAG, "GPS source: External GPS (lowest priority)");
                update_progress(100, "GPS Ready: External");
                // Update header GPS status
                ui_header_set_gps_status(status_header, true);
            } else {
                results->gps_ready = false;
                strncpy(results->gps_source, "None", sizeof(results->gps_source) - 1);
                printf("                         GPS Ready: ✗ (No GPS found)\n");
                ESP_LOGW(TAG, "No GPS source detected!");
                update_progress(100, "Warning: No GPS Found");
            }
        }
    }

    // Update status label
    if (lvgl_lock(100)) {
        char status_text[128];
        snprintf(status_text, sizeof(status_text), "GPS: %s", results->gps_source);
        lv_label_set_text(status_label, status_text);
        lv_obj_clear_flag(status_label, LV_OBJ_FLAG_HIDDEN);

        if (results->gps_ready) {
            lv_obj_set_style_text_color(status_label, lv_color_hex(0x00FF00), 0);
        } else {
            lv_obj_set_style_text_color(status_label, lv_color_hex(0xFF0000), 0);
        }

        lvgl_unlock();
    }

    printf("\n");
    ESP_LOGI(TAG, "Self-test complete");

    return ESP_OK;
}

/**
 * Display splash screen results
 */
void display_splash(const selftest_results_t *results) {
    print_splash_banner();

    if (results != NULL) {
        printf("Self-Test Results:\n");
        printf("  TF Card:        %s\n", results->tf_card_present ? "Present" : "Not Found");
        if (results->update_bin_found) {
            printf("  Update:         FOUND (will transition to UPDATE screen)\n");
        }
        printf("  GPS Source:     %s\n", results->gps_source);
        printf("  GPS Status:     %s\n", results->gps_ready ? "Ready" : "Not Available");
        printf("\n");
    }
}

/**
 * Run splash screen and self-test sequence
 */
esp_err_t splash_screen_run(uint32_t timeout_sec) {
    selftest_results_t results;
    memset(&results, 0, sizeof(results));  // Initialize to zero

    ESP_LOGI(TAG, "Splash screen starting (timeout: %lu seconds)", timeout_sec);

    // Create LVGL splash UI
    create_splash_ui();

    // Give LVGL time to render initial screen
    ESP_LOGI(TAG, "Waiting for screen to stabilize...");
    vTaskDelay(pdMS_TO_TICKS(5000));  // Hold static screen for 5 seconds

    // Display initial splash to serial
    print_splash_banner();

    // TEMPORARILY DISABLED: Show loading animation
    // for (int i = 0; i < 10; i++) {
    //     update_loading_animation();
    //     vTaskDelay(pdMS_TO_TICKS(200));
    // }
    // printf("\n\n");

    // TEMPORARILY DISABLED: Run self-test
    // ret = run_self_test(&results);
    // if (ret != ESP_OK) {
    //     ESP_LOGE(TAG, "Self-test failed: %s", esp_err_to_name(ret));
    //     return ret;
    // }

    // TEMPORARILY DISABLED: Display final results to serial
    // display_splash(&results);

    // Wait a moment to show results
    vTaskDelay(pdMS_TO_TICKS(3000));

    // If update.bin found, transition to UPDATE screen
    if (results.update_bin_found) {
        ESP_LOGI(TAG, "Firmware update detected, transitioning to UPDATE screen");
        // TODO: Transition to UPDATE screen
        // Clean up splash screen before returning
        if (lvgl_lock(1000)) {
            if (splash_screen != NULL) {
                lv_obj_del(splash_screen);
                splash_screen = NULL;
                ESP_LOGI(TAG, "Splash screen deleted");
            }
            lvgl_unlock();
        }
        return ESP_OK;
    }

    // Clean up splash screen before transitioning to next screen
    ESP_LOGI(TAG, "Splash screen complete, cleaning up...");
    if (lvgl_lock(1000)) {
        if (splash_screen != NULL) {
            lv_obj_del(splash_screen);  // LVGL 8.x API - deletes screen and all children
            splash_screen = NULL;
            status_header = NULL;
            logo_img = NULL;
            version_label = NULL;
            progress_bar = NULL;
            progress_label = NULL;
            selftest_title_label = NULL;
            tf_card_label = NULL;
            n2k_label = NULL;
            nmea_label = NULL;
            gps_label = NULL;
            status_label = NULL;
            ESP_LOGI(TAG, "Splash screen deleted");
        }
        lvgl_unlock();
    }

    ESP_LOGI(TAG, "Ready for next screen");
    return ESP_OK;
}
