/*
 * Connections screen — live status of every data source.
 *
 * Spec: https://github.com/Temple-of-Epiphany/anchor-drag-pro/wiki/Screen-Connections
 * If you're changing behaviour here, update the wiki page first.
 *
 * Milestone 1 of #71: layout shell + static rows. Live updates from
 * source-health pub/sub land when source managers (GPS / IMU / WiFi)
 * land. Each row exposes a setter so the eventual pub/sub layer
 * just calls screen_connections_set_row().
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#pragma once

#include "lvgl.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Canonical row identifiers — match the wiki's three-section layout. */
typedef enum {
    CONN_ROW_N2K = 0,
    CONN_ROW_GPS,
    CONN_ROW_IMU,
    CONN_ROW_SERIAL_GPS,
    CONN_ROW_SERIAL_IMU,
    CONN_ROW_URL,
    CONN_ROW_WIFI_STA,
    CONN_ROW_WIFI_AP,
    CONN_ROW_SD,
    CONN_ROW_RTC,
    CONN_ROW_COUNT,
} conn_row_id_t;

typedef enum {
    CONN_STATUS_OK = 0,        /* ✓ green */
    CONN_STATUS_WARN,          /* ⚠ amber — degraded */
    CONN_STATUS_FAIL,          /* ✗ red — should be present, isn't */
    CONN_STATUS_SCANNING,      /* ⟳ dim — probing */
    CONN_STATUS_DISABLED,      /* – dim — config has it off */
} conn_row_status_t;

/* Create the Connections screen (returns the root lv_obj_t,
 * not loaded yet — pass to lv_scr_load). */
lv_obj_t *screen_connections_create(void);

/* Update a row's status + detail string. Re-callable; takes the
 * LVGL mutex internally. */
void screen_connections_set_row(lv_obj_t *screen,
                                 conn_row_id_t row,
                                 conn_row_status_t status,
                                 const char *detail);

/* Accessors so screen_nav can drive the persistent chrome. */
lv_obj_t *screen_connections_header(lv_obj_t *screen);
lv_obj_t *screen_connections_footer(lv_obj_t *screen);

#ifdef __cplusplus
}
#endif
