/*
 * Modal: Alarm — full-screen takeover on state → ALARM.
 *
 * Spec: https://github.com/Temple-of-Epiphany/anchor-drag-pro/wiki/Modal-Alarm
 * If you're changing behaviour here, update the wiki page first.
 *
 * Highest priority modal — interrupts everything. Pulsing red, large
 * MUTE button (single-tap), smaller DISARM button (1 s hold). Audio
 * (buzzer) is deferred until the buzzer driver lands; the modal logs
 * "ALARM: siren on/off" at the points it would have triggered audio.
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#pragma once

#include "ui_modals.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*ui_alarm_action_cb)(void *user_data);

typedef struct {
    /* Live info shown in the modal body. Caller updates by re-calling
     * ui_alarm_update() — the modal stays mounted. */
    double      distance_value;
    const char *distance_unit;     /* "ft" | "m" */
    double      threshold;
    int         heading_deg;       /* -1 = unknown */
    int         rot_dps;           /* INT_MIN = unknown */
    ui_alarm_action_cb on_mute;     /* single tap */
    ui_alarm_action_cb on_disarm;   /* after 1 s hold */
    void              *user_data;
} ui_alarm_params_t;

/* Show the modal. If already showing, updates the live info instead.
 * Returns the modal handle. */
ui_modal_handle_t *ui_alarm_show(const ui_alarm_params_t *params);

/* Update live values on the open modal. Safe no-op if not open. */
void ui_alarm_update(double distance_value, double threshold,
                      int heading_deg, int rot_dps);

/* Transition visual to MUTED (background stays red but stops pulsing).
 * Caller is responsible for the state machine; modal stays mounted. */
void ui_alarm_set_muted(bool muted);

/* Dismiss the modal entirely — call when state returns to OFF/ON via
 * a confirmed DISARM. */
void ui_alarm_dismiss(void);

#ifdef __cplusplus
}
#endif
