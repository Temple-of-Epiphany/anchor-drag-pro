/**
 * LVGL Initialization and Integration
 * Waveshare Mode 3: Direct-Mode with hardware-managed synchronization
 *
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2025-12-20
 * Date Updated: 2025-12-24
 * Version: 0.2.1
 *
 * Initializes LVGL graphics library and integrates with RGB LCD display driver.
 * Uses full-screen frame buffers from RGB panel driver with direct-mode rendering.
 * Hardware double-buffering and bounce buffer provide automatic synchronization.
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
lv_disp_t* lvgl_get_display(void);

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

/**
 * Notify LVGL task from VSYNC ISR
 *
 * Called from display driver when RGB frame transmission completes.
 * This function is safe to call from ISR context.
 *
 * @return true if context switch is needed, false otherwise
 */
bool lvgl_notify_vsync_isr(void);

#endif // LVGL_INIT_H
