/**
 * Splash Screen and Self-Test Implementation
 *
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2025-12-19
 * Date Updated: 2025-12-19
 * Version: 0.1.1
 *
 * Changelog:
 * - 0.1.1 (2025-12-19): Implemented self-test sequence with GPS source priority
 * - 0.1.0 (2025-12-19): Initial implementation
 */

#include "splash_screen.h"
#include "ui_version.h"
#include "board_config.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "sdmmc_cmd.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <sys/stat.h>
#include <string.h>

static const char *TAG = "splash_screen";

// Animation characters for loading
static const char animation_chars[] = {'●', '○', '○', '○', '○'};
static uint8_t animation_frame = 0;

/**
 * Print splash banner
 */
static void print_splash_banner(void) {
    printf("\033[2J\033[H");  // Clear screen
    printf("================================================================================\n");
    printf("                          🏴‍☠️ ANCHOR DRAG ALARM\n");
    printf("                              Version %s\n", UI_VERSION_STRING);
    printf("                         UI Version %s\n", UI_VERSION_STRING);
    printf("                     FW Version %s\n", FW_VERSION_STRING);
    printf("                    Build: %s %s\n", UI_BUILD_DATE, UI_BUILD_TIME);
    printf("================================================================================\n");
    printf("\n");
}

/**
 * Display loading animation
 */
static void display_loading_animation(void) {
    printf("\r                        [Loading");
    for (int i = 0; i < 5; i++) {
        if (i == animation_frame) {
            printf("●");
        } else {
            printf("○");
        }
    }
    printf("]");
    fflush(stdout);

    animation_frame = (animation_frame + 1) % 5;
}

/**
 * Check for TF card presence
 */
bool check_tf_card(void) {
    ESP_LOGI(TAG, "Checking for TF card...");

    // TODO: Implement actual SD card detection
    // For now, simulate check
    vTaskDelay(pdMS_TO_TICKS(500));

    // Placeholder: Assume no TF card for now
    ESP_LOGW(TAG, "TF card support not yet implemented");
    return false;
}

/**
 * Check for update.bin on TF card
 */
bool check_update_bin(void) {
    ESP_LOGI(TAG, "Checking for update.bin...");

    // TODO: Implement actual file check
    // For now, simulate check
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
 * Priority GPS source
 */
bool check_n2k_data(uint32_t timeout_ms) {
    ESP_LOGI(TAG, "Checking for N2K data (NMEA 2000)...");

    // TODO: Implement actual CAN/TWAI check for PGN 129029
    // For now, simulate check
    vTaskDelay(pdMS_TO_TICKS(timeout_ms));

    #if ENABLE_CAN_BUS
    // Placeholder: Check if CAN bus is configured
    ESP_LOGD(TAG, "CAN bus enabled, checking for PGN 129029...");

    // In real implementation:
    // - Initialize TWAI driver
    // - Listen for PGN 129029 messages
    // - Return true if GPS data received within timeout

    ESP_LOGW(TAG, "N2K data check not yet implemented");
    return false;
    #else
    ESP_LOGD(TAG, "CAN bus disabled in configuration");
    return false;
    #endif
}

/**
 * Check for NMEA 0183 data
 * Secondary GPS source
 */
bool check_nmea0183_data(uint32_t timeout_ms) {
    ESP_LOGI(TAG, "Checking for NMEA 0183 data...");

    // TODO: Implement actual RS485 UART check
    // For now, simulate check
    vTaskDelay(pdMS_TO_TICKS(timeout_ms));

    #if ENABLE_RS485
    // Placeholder: Check for NMEA 0183 sentences
    ESP_LOGD(TAG, "RS485 enabled, checking for NMEA 0183...");

    // In real implementation:
    // - Initialize RS485 UART
    // - Listen for GPGGA or GPRMC sentences
    // - Return true if valid GPS data received within timeout

    ESP_LOGW(TAG, "NMEA 0183 check not yet implemented");
    return false;
    #else
    ESP_LOGD(TAG, "RS485 disabled in configuration");
    return false;
    #endif
}

/**
 * Check for external GPS module (I2C)
 * Tertiary GPS source
 */
bool check_external_gps(uint32_t timeout_ms) {
    ESP_LOGI(TAG, "Checking for external GPS module...");

    // TODO: Implement actual I2C GPS check
    // For now, simulate check
    vTaskDelay(pdMS_TO_TICKS(timeout_ms));

    #if ENABLE_EXTERNAL_GPS
    // Placeholder: Check for NEO-8M at address 0x42
    ESP_LOGD(TAG, "External GPS enabled, checking I2C address 0x%02X...", I2C_ADDR_NEO8M_GPS);

    // In real implementation:
    // - Initialize I2C0 bus
    // - Probe for GPS module at I2C_ADDR_NEO8M_GPS
    // - Check for valid GPS data
    // - Return true if GPS module responding

    ESP_LOGW(TAG, "External GPS check not yet implemented");
    return false;
    #else
    ESP_LOGD(TAG, "External GPS disabled in configuration");
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

    // Test 1: TF Card Detection
    printf("                         TF Card: ");
    fflush(stdout);
    results->tf_card_present = check_tf_card();
    printf("%s\n", results->tf_card_present ? "✓" : "✗");

    // If TF card present, check for update.bin
    if (results->tf_card_present) {
        printf("                         update.bin: ");
        fflush(stdout);
        results->update_bin_found = check_update_bin();
        printf("%s\n", results->update_bin_found ? "FOUND" : "Not found");
    }

    // Test 2: GPS Source Detection (Priority Order)
    // Priority 1: N2K Data
    printf("                         N2K Data: ");
    fflush(stdout);
    results->n2k_available = check_n2k_data(2000);  // 2 second timeout
    printf("%s\n", results->n2k_available ? "✓" : "✗");

    if (results->n2k_available) {
        results->gps_ready = true;
        strncpy(results->gps_source, "NMEA 2000 (N2K)", sizeof(results->gps_source) - 1);
        printf("                         GPS Ready: ✓\n");
        ESP_LOGI(TAG, "GPS source: NMEA 2000 (highest priority)");
    } else {
        // Priority 2: NMEA 0183
        printf("                         NMEA 0183: ");
        fflush(stdout);
        results->nmea0183_available = check_nmea0183_data(2000);  // 2 second timeout
        printf("%s\n", results->nmea0183_available ? "✓" : "✗");

        if (results->nmea0183_available) {
            results->gps_ready = true;
            strncpy(results->gps_source, "NMEA 0183", sizeof(results->gps_source) - 1);
            printf("                         GPS Ready: ✓\n");
            ESP_LOGI(TAG, "GPS source: NMEA 0183 (secondary priority)");
        } else {
            // Priority 3: External GPS
            printf("                         External GPS: ");
            fflush(stdout);
            results->external_gps_available = check_external_gps(2000);  // 2 second timeout
            printf("%s\n", results->external_gps_available ? "✓" : "✗");

            if (results->external_gps_available) {
                results->gps_ready = true;
                strncpy(results->gps_source, "External GPS (I2C)", sizeof(results->gps_source) - 1);
                printf("                         GPS Ready: ✓\n");
                ESP_LOGI(TAG, "GPS source: External GPS (lowest priority)");
            } else {
                results->gps_ready = false;
                strncpy(results->gps_source, "None", sizeof(results->gps_source) - 1);
                printf("                         GPS Ready: ✗ (No GPS found)\n");
                ESP_LOGW(TAG, "No GPS source detected!");
            }
        }
    }

    printf("\n");
    ESP_LOGI(TAG, "Self-test complete");

    return ESP_OK;
}

/**
 * Display splash screen
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
    esp_err_t ret;
    selftest_results_t results;

    ESP_LOGI(TAG, "Splash screen starting (timeout: %lu seconds)", timeout_sec);

    // Display initial splash
    print_splash_banner();

    // Show loading animation for a moment
    for (int i = 0; i < 10; i++) {
        display_loading_animation();
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    printf("\n\n");

    // Run self-test
    ret = run_self_test(&results);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Self-test failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Display final results
    display_splash(&results);

    // If update.bin found, transition to UPDATE screen
    if (results.update_bin_found) {
        ESP_LOGI(TAG, "Firmware update detected, transitioning to UPDATE screen");
        // TODO: Transition to UPDATE screen
        return ESP_OK;
    }

    // Otherwise, wait for remaining time and transition to START screen
    ESP_LOGI(TAG, "Splash screen complete, transitioning to START screen...");

    return ESP_OK;
}
