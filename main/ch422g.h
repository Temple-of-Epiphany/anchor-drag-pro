/**
 * CH422G I/O Expander Driver
 *
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2025-12-25
 * Version: 0.1.0
 *
 * I2C I/O expander control for Waveshare ESP32-S3-Touch-LCD-4.3B
 * Controls: LCD backlight, LCD reset, Touch reset, SD CS, USB select
 */

#ifndef CH422G_H
#define CH422G_H

#include <stdbool.h>
#include "driver/i2c.h"

// CH422G I2C addresses
#define CH422G_ADDR_READ    0x24
#define CH422G_ADDR_WRITE   0x38

// EXIO pin definitions
#define CH422G_EXIO0        (1 << 0)  // Reserved
#define CH422G_EXIO1        (1 << 1)  // Touch reset
#define CH422G_EXIO2        (1 << 2)  // LCD backlight
#define CH422G_EXIO3        (1 << 3)  // LCD reset
#define CH422G_EXIO4        (1 << 4)  // SD card chip select
#define CH422G_EXIO5        (1 << 5)  // USB selection

/**
 * Initialize CH422G I/O expander
 * @param i2c_num I2C port number
 * @return ESP_OK on success
 */
esp_err_t ch422g_init(i2c_port_t i2c_num);

/**
 * Write to all EXIO pins
 * @param i2c_num I2C port number
 * @param value 8-bit value (each bit controls one EXIO pin)
 * @return ESP_OK on success
 */
esp_err_t ch422g_write(i2c_port_t i2c_num, uint8_t value);

/**
 * Set specific EXIO pin high
 * @param i2c_num I2C port number
 * @param pin EXIO pin mask (use CH422G_EXIOx defines)
 * @return ESP_OK on success
 */
esp_err_t ch422g_set_pin(i2c_port_t i2c_num, uint8_t pin);

/**
 * Clear specific EXIO pin low
 * @param i2c_num I2C port number
 * @param pin EXIO pin mask (use CH422G_EXIOx defines)
 * @return ESP_OK on success
 */
esp_err_t ch422g_clear_pin(i2c_port_t i2c_num, uint8_t pin);

/**
 * Read current EXIO state
 * @param i2c_num I2C port number
 * @param value Pointer to store current state
 * @return ESP_OK on success
 */
esp_err_t ch422g_read(i2c_port_t i2c_num, uint8_t *value);

#endif // CH422G_H
