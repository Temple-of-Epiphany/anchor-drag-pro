/**
 * Simple Test Screen - Minimal UI for testing LVGL 8.4.0
 *
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2025-12-24
 * Version: 0.1.0
 */

#ifndef SIMPLE_TEST_SCREEN_H
#define SIMPLE_TEST_SCREEN_H

#include "esp_err.h"

/**
 * Create simple test screen with title and OK button
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t simple_test_screen_create(void);

/**
 * Delete simple test screen
 */
void simple_test_screen_delete(void);

#endif // SIMPLE_TEST_SCREEN_H
