/*
 * CH422G I/O expander driver — WaveShare ESP32-S3-Touch-LCD-4.3B.
 *
 * The CH422G has a quirky addressing scheme: instead of one slave address
 * with internal registers, it exposes its registers AS DIFFERENT I2C ADDRESSES:
 *   0x24 — mode register (output-enable, scan, open-drain, sleep)
 *   0x23 — open-drain output (4 pins, OC0..OC3)
 *   0x38 — push-pull I/O output and read-back (8 pins, IO0..IO7)
 *
 * On the WaveShare 4.3B, EXIO1..EXIO5 are wired to specific functions
 * (touch reset, LCD backlight/reset, SD CS, USB/CAN switch). We expose
 * named accessors below; raw pin access is also available for testing.
 *
 * Threading: every transaction takes the shared I2C0 mutex
 * (per Workstream 0 prereq #37). Safe to call from any task.
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Initialize the CH422G.
 * Requires i2c_bus_init() to have succeeded first.
 *
 * Sets the chip into IO output-enable mode and writes the initial
 * pin state from board_config.h CH422G_EXIO_INIT_STATE.
 */
esp_err_t ch422g_init(void);

/*
 * Set the entire IO output register at once. Bits 0..7 map to EXIO0..EXIO7.
 * (Reserved bits should not be driven; safest to AND with allowed mask.)
 */
esp_err_t ch422g_set_all(uint8_t io_mask);

/*
 * Set a single EXIO pin. pin = 0..7. level = true (high) / false (low).
 * Updates the cached IO state and pushes the whole register.
 */
esp_err_t ch422g_set_pin(uint8_t pin, bool level);

/*
 * Read a single EXIO pin. Returns ESP_OK and writes *out_level on success.
 * Note: CH422G must be in input mode for this to be meaningful for pins
 * that are wired as inputs. On the WaveShare 4.3B all CH422G pins are
 * outputs, so reads return the cached output state.
 */
esp_err_t ch422g_get_pin(uint8_t pin, bool *out_level);

/*
 * Get the cached IO register state without performing a bus transaction.
 * Useful for diagnostics.
 */
uint8_t ch422g_get_state_cached(void);


/* ---- Named accessors for the WaveShare 4.3B pin functions ---- */

/* LCD backlight: true = on, false = off (active high). */
esp_err_t ch422g_lcd_backlight(bool on);

/*
 * LCD reset pulse — drive LCD_RST low for hold_ms then release.
 * The display panel needs this at init time.
 */
esp_err_t ch422g_lcd_reset_pulse(uint32_t hold_ms);

/*
 * Touch reset pulse — drive TP_RST low for hold_ms then release.
 * GT911 needs this at init time, with timing relative to its IRQ pin.
 */
esp_err_t ch422g_touch_reset_pulse(uint32_t hold_ms);

/* SD card chip select. assert = true pulls CS low (selected). */
esp_err_t ch422g_sd_cs(bool assert);

/*
 * USB/CAN multiplexer.
 *   true  = CAN routed to GPIO 15/16 (TWAI / NMEA 2000 active)
 *   false = USB routed (default at boot for serial console)
 */
esp_err_t ch422g_usb_can_select(bool can_mode);

#ifdef __cplusplus
}
#endif
