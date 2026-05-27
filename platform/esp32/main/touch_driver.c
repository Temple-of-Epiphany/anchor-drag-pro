/*
 * GT911 touch driver — STUB for milestone 1 of #69.
 *
 * The v0.1 archive driver used the legacy driver/i2c.h API which conflicts
 * with our shared i2c_bus.{c,h} (new driver/i2c_master.h). The real port
 * lands as a follow-up commit within #69 using the new bus handle pattern
 * (esp_lcd_new_panel_io_i2c_v2 with i2c_bus_handle()) and the existing
 * ch422g_touch_reset_pulse() helper.
 *
 * For now: touch_init() performs the hardware reset via CH422G but does
 * not bring up the touch controller. touch_get_handle() returns NULL so
 * lvgl_init.c skips touch registration cleanly (display continues to work).
 *
 * Tracking: same issue (#69). Wiki contract honoured — Splash and Info
 * screens never required touch in the spec.
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#include "touch_driver.h"
#include "board_config.h"
#include "ch422g.h"
#include "esp_log.h"

static const char *TAG = "touch_driver";

esp_err_t touch_init(void)
{
    ESP_LOGW(TAG, "touch_init: STUB — hardware reset only, no GT911 bring-up "
                  "(real driver in follow-up commit, #69)");

    /* Hardware reset via CH422G EXIO1 — proves the IO expander wiring is
     * sound and resets the touch chip into a known state. */
    esp_err_t err = ch422g_touch_reset_pulse(100);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ch422g_touch_reset_pulse failed: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

esp_lcd_touch_handle_t touch_get_handle(void)
{
    /* Returning NULL is intentional — see file header. lvgl_init.c logs
     * a warning and skips input device registration when this is NULL. */
    return NULL;
}

esp_err_t touch_reset(void)
{
    return ch422g_touch_reset_pulse(100);
}
