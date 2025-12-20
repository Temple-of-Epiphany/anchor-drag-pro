/**
 * Splash Screen and Self-Test
 *
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2025-12-19
 * Date Updated: 2025-12-19
 * Version: 0.1.1
 *
 * Implements SPLASH SCREEN (Screen 0) per docs/UI_Screens.md
 */

#ifndef SPLASH_SCREEN_H
#define SPLASH_SCREEN_H

#include <stdbool.h>
#include "esp_err.h"

// Self-test results structure
typedef struct {
    bool tf_card_present;
    bool update_bin_found;
    bool n2k_available;
    bool nmea0183_available;
    bool external_gps_available;
    bool gps_ready;
    char gps_source[32];
} selftest_results_t;

/**
 * Run splash screen and self-test sequence
 *
 * @param timeout_sec Maximum time to display splash (default: 30 seconds)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t splash_screen_run(uint32_t timeout_sec);

/**
 * Run hardware self-test
 *
 * @param results Pointer to structure to store results
 * @return ESP_OK if self-test completed (even if some tests failed)
 */
esp_err_t run_self_test(selftest_results_t *results);

/**
 * Check for TF card presence
 *
 * @return true if TF card mounted, false otherwise
 */
bool check_tf_card(void);

/**
 * Check for update.bin on TF card
 *
 * @return true if update.bin exists, false otherwise
 */
bool check_update_bin(void);

/**
 * Check for N2K data (NMEA 2000)
 * Priority GPS source
 *
 * @param timeout_ms Timeout in milliseconds
 * @return true if N2K data detected, false otherwise
 */
bool check_n2k_data(uint32_t timeout_ms);

/**
 * Check for NMEA 0183 data
 * Secondary GPS source
 *
 * @param timeout_ms Timeout in milliseconds
 * @return true if NMEA 0183 data detected, false otherwise
 */
bool check_nmea0183_data(uint32_t timeout_ms);

/**
 * Check for external GPS module (I2C)
 * Tertiary GPS source
 *
 * @param timeout_ms Timeout in milliseconds
 * @return true if external GPS detected, false otherwise
 */
bool check_external_gps(uint32_t timeout_ms);

/**
 * Display splash screen (placeholder until LVGL integrated)
 * Currently outputs to serial
 *
 * @param results Self-test results to display
 */
void display_splash(const selftest_results_t *results);

#endif // SPLASH_SCREEN_H
