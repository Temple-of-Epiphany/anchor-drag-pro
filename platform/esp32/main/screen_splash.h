/*
 * Splash screen — boot screen with version + progressive self-test rows.
 *
 * Spec: https://github.com/Temple-of-Epiphany/anchor-drag-pro/wiki/Screen-Splash
 * If you're changing behaviour here, update the wiki page first.
 *
 * Boot orchestration model:
 *   1. main.c calls screen_splash_show() once the display + LVGL are up
 *   2. As each boot phase completes (I²C, CH422G, RTC, SD, config, OTA,
 *      WiFi, GPS, IMU, Display), main.c calls screen_splash_set_row()
 *      with the result. Rows appear visually as they're set — empty
 *      until first set, then their final status.
 *   3. After the last row is reported, main.c calls screen_splash_done()
 *      to allow the splash to hand off to Monitor (#70 lands that
 *      transition; until then the splash remains visible).
 *
 * Hard cap: spec requires a 10 s ceiling. main.c is responsible for
 * calling screen_splash_done() within that window; if a probe is still
 * running, main.c can mark its row "continuing in background" and
 * proceed anyway.
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#pragma once

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Canonical row order — matches the wiki spec table. */
typedef enum {
    SPLASH_ROW_I2C = 0,
    SPLASH_ROW_CH422G,
    SPLASH_ROW_RTC,
    SPLASH_ROW_SD,
    SPLASH_ROW_CONFIG,
    SPLASH_ROW_OTA,
    SPLASH_ROW_WIFI,
    SPLASH_ROW_GPS,
    SPLASH_ROW_IMU,
    SPLASH_ROW_DISPLAY,
    SPLASH_ROW_COUNT,
} splash_row_id_t;

typedef enum {
    SPLASH_STATUS_RUNNING = 0,   /* ⟳ */
    SPLASH_STATUS_PASS,          /* ✓ */
    SPLASH_STATUS_WARN,          /* ⚠ */
    SPLASH_STATUS_FAIL,          /* ✗ */
    SPLASH_STATUS_SKIP,          /* – */
    SPLASH_STATUS_CONTINUING,    /* ⟳ (continuing in background) */
} splash_row_status_t;

/* Create + show the splash. Safe to call once per boot. Returns ESP_OK
 * if the splash is on-screen. */
esp_err_t screen_splash_show(const char *firmware_version);

/* Set/update a row by canonical id. Re-callable to reflect status
 * changes (RUNNING → PASS, etc.). Detail may be NULL or empty.
 * Thread-safe; takes the LVGL mutex internally. */
void screen_splash_set_row(splash_row_id_t row,
                            splash_row_status_t status,
                            const char *detail);

/* Mark the boot sequence complete — clears any RUNNING glyphs on
 * unreported rows and stops the LVGL re-flow. Currently a no-op for
 * dismissal; #70 (Monitor) will trigger the actual screen transition. */
void screen_splash_done(void);

#ifdef __cplusplus
}
#endif
