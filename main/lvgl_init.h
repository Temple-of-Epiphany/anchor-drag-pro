/**
 * LVGL Initialization and Integration
 *
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2025-12-20
 * Date Updated: 2025-12-20
 * Version: 0.1.1
 *
 * Initializes LVGL graphics library and integrates with RGB LCD display driver.
 */

#ifndef LVGL_INIT_H
#define LVGL_INIT_H

#include "esp_err.h"
#include "lvgl.h"

/**
 * Initialize LVGL graphics library
 *
 * This function:
 * - Initializes LVGL core
 * - Creates display buffers in PSRAM
 * - Registers RGB LCD panel with LVGL
 * - Starts LVGL tick timer
 * - Creates LVGL task for handling UI updates
 *
 * Must be called after display_init()
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t lvgl_init(void);

/**
 * Get LVGL display object
 *
 * @return Pointer to LVGL display object
 */
lv_display_t* lvgl_get_display(void);

/**
 * Lock LVGL mutex (for thread-safe LVGL API calls)
 *
 * Call this before any LVGL API calls from non-LVGL threads.
 * Must be paired with lvgl_unlock().
 *
 * @param timeout_ms Timeout in milliseconds
 * @return true if lock acquired, false on timeout
 */
bool lvgl_lock(uint32_t timeout_ms);

/**
 * Unlock LVGL mutex
 *
 * Call this after LVGL API calls from non-LVGL threads.
 */
void lvgl_unlock(void);

#endif // LVGL_INIT_H
