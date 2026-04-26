/**
 * SMPTE Color Bar Test Screen
 *
 * Displays SMPTE color bars for screen self-test on boot
 *
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2025-12-22
 * Date Updated: 2025-12-22
 * Version: 0.1.0
 */

#ifndef SMPTE_TEST_SCREEN_H
#define SMPTE_TEST_SCREEN_H

#include "esp_err.h"
#include <stdint.h>

/**
 * Display SMPTE color bars for screen self-test
 *
 * @param duration_sec Duration to display test pattern in seconds
 * @return ESP_OK on success, error code on failure
 */
esp_err_t smpte_test_screen_run(uint32_t duration_sec);

#endif // SMPTE_TEST_SCREEN_H