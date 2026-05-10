/*
 * I2C0 shared bus + mutex.
 *
 * The WaveShare 4.3B has FOUR-plus devices on one I2C bus (GPIO 8/9 @ 400kHz):
 *   0x24/0x38  CH422G I/O expander (controls SD CS, backlight, LCD/touch reset)
 *   0x4A/0x4B  BNO085 / HWT901B IMU (IMU/GPS variant only)
 *   0x51       PCF85063A RTC
 *   0x5D       GT911 touch controller
 *
 * Concurrent access from different tasks (touch + RTC + IMU polling, etc.)
 * corrupts the bus. Every transaction must hold the bus mutex.
 *
 * Usage pattern:
 *   if (i2c_bus_lock(100)) {
 *       i2c_master_transmit(device_handle, data, len, -1);
 *       i2c_bus_unlock();
 *   }
 *
 * Or via the convenience macro:
 *   WITH_I2C_MUTEX(100, {
 *       i2c_master_transmit(device_handle, data, len, -1);
 *   });
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#pragma once

#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Initialize the shared I2C0 master bus.
 * Idempotent — safe to call once at boot. Returns ESP_OK on success.
 */
esp_err_t i2c_bus_init(int sda_gpio, int scl_gpio, uint32_t clk_speed_hz);

/*
 * Bus handle for adding device handles via i2c_master_bus_add_device().
 * NULL until i2c_bus_init() has succeeded.
 */
i2c_master_bus_handle_t i2c_bus_handle(void);

/*
 * Take the bus mutex. timeout_ms = max wait. Returns true if acquired.
 */
bool i2c_bus_lock(uint32_t timeout_ms);

/*
 * Release the bus mutex. Must follow a successful i2c_bus_lock().
 */
void i2c_bus_unlock(void);

/*
 * Scan the bus for responding devices. Logs addresses found via ESP_LOGI.
 * Useful at boot for verification and for the future `i2c scan` CLI command.
 */
void i2c_bus_scan(void);

/*
 * Convenience macro for short critical sections.
 * Example:
 *   WITH_I2C_MUTEX(100, {
 *       i2c_master_transmit(handle, &cmd, 1, -1);
 *   });
 *
 * Caller is responsible for handling timeout (the body runs ONLY on
 * successful lock; if the timeout expires the body is skipped and a
 * warning is logged).
 */
#define WITH_I2C_MUTEX(timeout_ms, body) do {           \
    if (i2c_bus_lock(timeout_ms)) {                     \
        body;                                           \
        i2c_bus_unlock();                               \
    }                                                   \
} while (0)

#ifdef __cplusplus
}
#endif
