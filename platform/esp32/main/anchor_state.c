/*
 * Anchor state machine — implementation.
 *
 * Sample collection rate assumption: gps_source updates at the source
 * rate (~2 Hz from the simulator's NMEA 0183 stream). ARMING duration
 * × 2 Hz gives the target sample count; the actual rate varies in
 * production (N2K is typically 10 Hz), so we collect until both
 * (time-elapsed ≥ arming_seconds) and (≥ 30 samples) are satisfied.
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#include "anchor_state.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <string.h>

static const char *TAG = "anchor_state";

#define MIN_ARMING_SAMPLES 30

typedef struct {
    anchor_state_t            state;
    anchor_geo_buf_t          buf;
    geo_point_t               centroid;
    bool                      centroid_valid;
    double                    alarm_distance_m;
    int                       arming_seconds;
    uint64_t                  arming_started_us;
    double                    current_distance_m;
    anchor_state_change_cb    cb;
    void                     *cb_user;
} machine_t;

static machine_t        s_m       = { 0 };
static SemaphoreHandle_t s_mtx    = NULL;

static const char *state_name(anchor_state_t s)
{
    switch (s) {
        case AS_OFF:    return "OFF";
        case AS_ON:     return "ON";
        case AS_ARMING: return "ARMING";
        case AS_ARMED:  return "ARMED";
        case AS_ALARM:  return "ALARM";
        case AS_MUTED:  return "MUTED";
        default:        return "?";
    }
}

static void transition_to(anchor_state_t next)
{
    anchor_state_t prev = s_m.state;
    if (prev == next) return;
    s_m.state = next;
    ESP_LOGI(TAG, "%s -> %s", state_name(prev), state_name(next));
    if (s_m.cb) s_m.cb(prev, next, s_m.cb_user);
}

/* ---- Public API ---- */

void anchor_state_init(double alarm_distance_m, int arming_seconds)
{
    if (!s_mtx) s_mtx = xSemaphoreCreateMutex();
    if (xSemaphoreTake(s_mtx, portMAX_DELAY) != pdTRUE) return;
    memset(&s_m, 0, sizeof(s_m));
    s_m.state            = AS_OFF;
    s_m.alarm_distance_m = alarm_distance_m;
    s_m.arming_seconds   = (arming_seconds > 0) ? arming_seconds : 60;
    xSemaphoreGive(s_mtx);
}

void anchor_state_set_config(double alarm_distance_m, int arming_seconds)
{
    if (xSemaphoreTake(s_mtx, portMAX_DELAY) != pdTRUE) return;
    s_m.alarm_distance_m = alarm_distance_m;
    s_m.arming_seconds   = (arming_seconds > 0) ? arming_seconds : 60;
    xSemaphoreGive(s_mtx);
}

void anchor_state_set_callback(anchor_state_change_cb cb, void *user_data)
{
    if (!s_mtx) s_mtx = xSemaphoreCreateMutex();
    if (xSemaphoreTake(s_mtx, portMAX_DELAY) != pdTRUE) return;
    s_m.cb      = cb;
    s_m.cb_user = user_data;
    xSemaphoreGive(s_mtx);
}

bool anchor_state_arm(void)
{
    if (xSemaphoreTake(s_mtx, portMAX_DELAY) != pdTRUE) return false;
    bool ok = false;
    if (s_m.state == AS_OFF || s_m.state == AS_ON) {
        anchor_geo_buf_reset(&s_m.buf);
        s_m.centroid_valid    = false;
        s_m.arming_started_us = (uint64_t) esp_timer_get_time();
        transition_to(AS_ARMING);
        ok = true;
    }
    xSemaphoreGive(s_mtx);
    return ok;
}

bool anchor_state_cancel(void)
{
    if (xSemaphoreTake(s_mtx, portMAX_DELAY) != pdTRUE) return false;
    bool ok = false;
    if (s_m.state == AS_ARMING) {
        transition_to(AS_OFF);
        ok = true;
    }
    xSemaphoreGive(s_mtx);
    return ok;
}

bool anchor_state_disarm(void)
{
    if (xSemaphoreTake(s_mtx, portMAX_DELAY) != pdTRUE) return false;
    bool ok = false;
    if (s_m.state == AS_ARMED || s_m.state == AS_ALARM || s_m.state == AS_MUTED) {
        transition_to(AS_OFF);
        s_m.centroid_valid = false;
        anchor_geo_buf_reset(&s_m.buf);
        ok = true;
    }
    xSemaphoreGive(s_mtx);
    return ok;
}

bool anchor_state_mute(void)
{
    if (xSemaphoreTake(s_mtx, portMAX_DELAY) != pdTRUE) return false;
    bool ok = false;
    if (s_m.state == AS_ALARM) {
        transition_to(AS_MUTED);
        ok = true;
    }
    xSemaphoreGive(s_mtx);
    return ok;
}

void anchor_state_on_fix(geo_point_t pos, bool fix_valid)
{
    if (!fix_valid) return;
    if (xSemaphoreTake(s_mtx, portMAX_DELAY) != pdTRUE) return;

    if (s_m.state == AS_ARMING) {
        anchor_geo_buf_push(&s_m.buf, pos);
        uint64_t elapsed_us  = (uint64_t) esp_timer_get_time() - s_m.arming_started_us;
        bool time_done       = (elapsed_us / 1000000ULL) >= (uint64_t) s_m.arming_seconds;
        bool samples_done    = anchor_geo_buf_size(&s_m.buf) >= MIN_ARMING_SAMPLES;
        if (time_done && samples_done) {
            /* Compute centroid + transition. */
            geo_point_t centre;
            geo_point_t snap[ANCHOR_GEO_BUF_MAX];
            size_t n = anchor_geo_buf_snapshot(&s_m.buf, snap, ANCHOR_GEO_BUF_MAX);
            if (anchor_geo_centroid(snap, n, &centre)) {
                s_m.centroid       = centre;
                s_m.centroid_valid = true;
                double r = anchor_geo_max_radius_m(snap, n, centre);
                ESP_LOGI(TAG, "centroid lat=%.6f lon=%.6f  collected %u samples  observed radius=%.1f m",
                         centre.lat, centre.lon, (unsigned) n, r);
                transition_to(AS_ARMED);
            } else {
                /* Shouldn't happen unless GPS dropped during ARMING. */
                transition_to(AS_OFF);
            }
        }
    } else if (s_m.state == AS_ARMED || s_m.state == AS_ALARM || s_m.state == AS_MUTED) {
        if (s_m.centroid_valid) {
            s_m.current_distance_m = anchor_geo_distance_m(s_m.centroid, pos);
            bool exceeded = s_m.current_distance_m > s_m.alarm_distance_m;
            if (s_m.state == AS_ARMED && exceeded) {
                transition_to(AS_ALARM);
            } else if (s_m.state == AS_MUTED && !exceeded) {
                /* Boat re-entered the circle while muted → re-arm. */
                transition_to(AS_ARMED);
            }
            /* Note: if state is ALARM and boat re-enters, we stay in
             * ALARM until the user explicitly DISARMs. Spec rationale:
             * an alarm that fired indicates something real happened. */
        }
    }
    xSemaphoreGive(s_mtx);
}

void anchor_state_get(anchor_state_snapshot_t *out)
{
    if (!out) return;
    memset(out, 0, sizeof(*out));
    if (!s_mtx) return;
    if (xSemaphoreTake(s_mtx, portMAX_DELAY) != pdTRUE) return;
    out->state             = s_m.state;
    out->centroid          = s_m.centroid;
    out->centroid_valid    = s_m.centroid_valid;
    out->alarm_distance_m  = s_m.alarm_distance_m;
    out->arming_samples    = (int) anchor_geo_buf_size(&s_m.buf);
    out->arming_target     = MIN_ARMING_SAMPLES > s_m.arming_seconds * 2
                                 ? MIN_ARMING_SAMPLES
                                 : s_m.arming_seconds * 2;
    out->arming_progress   = (out->arming_target > 0)
                                 ? (double) out->arming_samples / (double) out->arming_target
                                 : 0.0;
    if (out->arming_progress > 1.0) out->arming_progress = 1.0;
    out->current_distance_m = s_m.current_distance_m;
    xSemaphoreGive(s_mtx);
}

ui_state_pill_t anchor_state_to_pill(anchor_state_t s)
{
    switch (s) {
        case AS_OFF:    return UI_STATE_PILL_OFF;
        case AS_ON:     return UI_STATE_PILL_ON;
        case AS_ARMING: return UI_STATE_PILL_ARMING;
        case AS_ARMED:  return UI_STATE_PILL_ARMED;
        case AS_ALARM:  return UI_STATE_PILL_ALARM;
        case AS_MUTED:  return UI_STATE_PILL_MUTED;
        default:        return UI_STATE_PILL_OFF;
    }
}
