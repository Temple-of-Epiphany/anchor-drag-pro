/*
 * GPS source manager — implementation.
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#include "gps_source.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <string.h>

static gps_fix_t          s_fix       = { 0 };
static SemaphoreHandle_t  s_mtx       = NULL;
static bool               s_initted   = false;
static gps_source_fix_cb  s_sub_cb    = NULL;
static void              *s_sub_user  = NULL;

void gps_source_init(void)
{
    if (s_initted) return;
    s_mtx = xSemaphoreCreateMutex();
    s_initted = true;
}

void gps_source_ingest_nmea(gps_source_t source, const nmea_fields_t *fields)
{
    if (!s_initted) gps_source_init();
    if (!fields) return;

    if (xSemaphoreTake(s_mtx, portMAX_DELAY) != pdTRUE) return;

    if (fields->pos_valid) {
        s_fix.pos_valid   = true;
        s_fix.latitude    = fields->latitude;
        s_fix.longitude   = fields->longitude;
    }
    if (fields->fix_valid) {
        s_fix.fix_quality = fields->fix_quality;
        if (fields->satellites) s_fix.satellites = fields->satellites;
        if (fields->hdop > 0)   s_fix.hdop       = fields->hdop;
    }
    if (fields->type == NMEA_GGA && fields->altitude_m != 0) {
        s_fix.altitude_m = fields->altitude_m;
    }
    if (fields->sog_valid) {
        s_fix.sog_valid = true;
        s_fix.sog_kts   = fields->sog_kts;
    }
    if (fields->cog_valid) {
        s_fix.cog_valid = true;
        s_fix.cog_deg   = fields->cog_deg;
    }
    if (fields->heading_valid) {
        s_fix.heading_valid   = true;
        s_fix.heading_deg     = fields->heading_deg;
        s_fix.heading_is_true = fields->heading_is_true;
    }
    s_fix.source         = source;
    s_fix.last_update_us = (uint64_t) esp_timer_get_time();

    /* Snapshot for the subscriber so the callback doesn't hold the
     * mutex (it may take its own lock). */
    gps_fix_t snap = s_fix;
    gps_source_fix_cb cb = s_sub_cb;
    void *user = s_sub_user;

    xSemaphoreGive(s_mtx);

    /* Fire the subscriber only on sentences that updated position. */
    if (fields->pos_valid && cb) cb(&snap, user);
}

void gps_source_subscribe(gps_source_fix_cb cb, void *user_data)
{
    if (!s_initted) gps_source_init();
    if (xSemaphoreTake(s_mtx, portMAX_DELAY) == pdTRUE) {
        s_sub_cb   = cb;
        s_sub_user = user_data;
        xSemaphoreGive(s_mtx);
    }
}

void gps_source_get(gps_fix_t *out)
{
    if (!out) return;
    if (!s_initted) { memset(out, 0, sizeof(*out)); return; }
    if (xSemaphoreTake(s_mtx, portMAX_DELAY) == pdTRUE) {
        *out = s_fix;
        xSemaphoreGive(s_mtx);
    } else {
        memset(out, 0, sizeof(*out));
    }
}

bool gps_source_is_fresh(uint32_t max_age_ms)
{
    if (!s_initted || s_fix.last_update_us == 0) return false;
    uint64_t now = (uint64_t) esp_timer_get_time();
    uint64_t age_us = now - s_fix.last_update_us;
    return (age_us / 1000) <= max_age_ms;
}
