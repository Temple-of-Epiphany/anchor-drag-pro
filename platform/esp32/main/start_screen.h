/**
 * START Screen - Main Application Screen
 *
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2025-12-22
 * Date Updated: 2025-12-22
 * Version: 0.1.0
 *
 * Main application screen with header, footer navigation,
 * and status display. First screen shown after boot sequence.
 */

#ifndef START_SCREEN_H
#define START_SCREEN_H

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * Create and display the START screen
 * @return ESP_OK on success, error code on failure
 */
esp_err_t start_screen_create(void);

/**
 * Update GPS status on START screen
 * @param gps_ready true if GPS is ready, false otherwise
 * @param gps_source GPS source string (e.g., "NMEA 2000")
 */
void start_screen_update_gps(bool gps_ready, const char *gps_source);

#endif // START_SCREEN_H
