/**
 * GT911 Touch Driver Header
 *
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2025-12-23
 * Date Updated: 2025-12-23
 * Version: 0.1.0
 */

#ifndef TOUCH_DRIVER_H
#define TOUCH_DRIVER_H

#include "esp_err.h"
#include "esp_lcd_touch.h"

/**
 * Initialize GT911 touch controller
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t touch_init(void);

/**
 * Get touch panel handle
 *
 * @return Touch panel handle, or NULL if not initialized
 */
esp_lcd_touch_handle_t touch_get_handle(void);

/**
 * Reset touch controller via CH422G I/O expander
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t touch_reset(void);

#endif // TOUCH_DRIVER_H
