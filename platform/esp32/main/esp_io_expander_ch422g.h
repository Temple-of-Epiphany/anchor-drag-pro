/**
 * ESP IO Expander - CH422G Device Driver
 *
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2025-12-26
 * Version: 0.1.0
 *
 * CH422G-specific implementation for ESP_IO_Expander library
 */

#ifndef ESP_IO_EXPANDER_CH422G_H
#define ESP_IO_EXPANDER_CH422G_H

#include "esp_io_expander.h"
#include "driver/i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create a new CH422G IO expander device
 *
 * @param i2c_num I2C port number
 * @param i2c_address I2C device address (read address, typically 0x24)
 * @param handle Pointer to store the IO expander handle
 * @return esp_err_t ESP_OK on success
 */
esp_err_t esp_io_expander_new_i2c_ch422g(i2c_port_t i2c_num, uint8_t i2c_address, esp_io_expander_handle_t *handle);

#ifdef __cplusplus
}
#endif

#endif // ESP_IO_EXPANDER_CH422G_H
