/*
 * Anchor state machine.
 *
 * Spec: https://github.com/Temple-of-Epiphany/anchor-drag-pro-meta/blob/main/docs/state-spec.md
 *
 * States:
 *   OFF     — system idle; no monitoring; ARM transitions to ARMING
 *   ON      — same as OFF for v0.2; reserved for future "armed-soon" hint
 *   ARMING  — collecting GPS samples; on completion → ARMED with centroid + circle
 *   ARMED   — passive watch; when distance from centroid > threshold → ALARM
 *   ALARM   — siren + Modal-Alarm; MUTE → MUTED, DISARM → OFF
 *   MUTED   — visual still red, no audio; DISARM → OFF; if boat re-enters circle → ARMED
 *
 * Owns the centroid + circle radius + sample buffer. Receives GPS fixes
 * via anchor_state_on_fix(). Emits state-change callbacks.
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#pragma once

#include "anchor_geo.h"
#include "ui_chrome.h"     /* for ui_state_pill_t reuse */
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    AS_OFF = 0,
    AS_ON,
    AS_ARMING,
    AS_ARMED,
    AS_ALARM,
    AS_MUTED,
} anchor_state_t;

typedef struct {
    anchor_state_t state;
    geo_point_t    centroid;          /* valid in ARMED / ALARM / MUTED */
    bool           centroid_valid;
    double         alarm_distance_m;  /* from config */
    double         arming_progress;   /* 0..1 during ARMING */
    int            arming_samples;    /* count collected so far */
    int            arming_target;     /* target count (= arming_seconds * 2 Hz) */
    double         current_distance_m;/* current boat-to-centroid (post-ARMED) */
} anchor_state_snapshot_t;

typedef void (*anchor_state_change_cb)(anchor_state_t prev, anchor_state_t next,
                                        void *user_data);

/* Initialise. The state machine starts in OFF and consumes the
 * configured alarm distance + arming seconds via the supplied accessor
 * (so the same code works when the user changes the preset). */
void anchor_state_init(double alarm_distance_m, int arming_seconds);

/* Update the alarm distance + arming seconds — call when the user
 * changes the preset, even mid-ARMED (only takes effect on next
 * ARM cycle for safety). */
void anchor_state_set_config(double alarm_distance_m, int arming_seconds);

/* Subscribe to state-change events. Single subscriber for v0.2 — the
 * Monitor screen. Returns ESP_OK. */
void anchor_state_set_callback(anchor_state_change_cb cb, void *user_data);

/* User-action transitions. Each returns true if the transition fired. */
bool anchor_state_arm(void);
bool anchor_state_cancel(void);
bool anchor_state_disarm(void);
bool anchor_state_mute(void);

/* Fix ingest — called every time gps_source updates. The state machine
 * picks samples during ARMING and runs the alarm check during ARMED. */
void anchor_state_on_fix(geo_point_t pos, bool fix_valid);

/* Snapshot. */
void anchor_state_get(anchor_state_snapshot_t *out);

/* Convenience: convert internal state to the ui_chrome state-pill enum. */
ui_state_pill_t anchor_state_to_pill(anchor_state_t s);

#ifdef __cplusplus
}
#endif
