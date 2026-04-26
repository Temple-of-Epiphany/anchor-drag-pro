/**
 * CH422G I/O Expander Driver
 *
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2025-12-25
 * Date Updated: 2025-12-26
 * Version: 0.2.0
 *
 * I/O expander control for Waveshare ESP32-S3-Touch-LCD-4.3B
 * Controls: LCD backlight, LCD reset, Touch reset, SD CS, USB select
 *
 * Now uses ESP_IO_Expander library instead of direct I2C
 */

#ifndef CH422G_H
#define CH422G_H

#include <stdbool.h>
#include "driver/i2c.h"
#include "esp_io_expander.h"
#include "esp_io_expander_ch422g.h"

// Global CH422G IO expander handle
extern esp_io_expander_handle_t ch422g_handle;

/**
 * Initialize CH422G I/O expander using ESP_IO_Expander library
 * @param i2c_num I2C port number
 * @return ESP_OK on success
 */
esp_err_t ch422g_init(i2c_port_t i2c_num);

/**
 * Get the global CH422G expander handle
 * @return esp_io_expander_handle_t Global handle (NULL if not initialized)
 */
esp_io_expander_handle_t ch422g_get_handle(void);

#endif // CH422G_H
