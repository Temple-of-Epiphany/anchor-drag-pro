/*
 * Modal: Confirm — generic destructive-action confirmation.
 *
 * Spec: https://github.com/Temple-of-Epiphany/anchor-drag-pro/wiki/Modal-Confirm
 * If you're changing behaviour here, update the wiki page first.
 *
 * Single reusable widget called by every destructive code path
 * (factory reset, clear logs, clear tracks, DISARM from ALARM, etc.).
 * Caller passes a params struct; widget handles layout + hold-to-confirm
 * + dismissal semantics. State-machine lockout is the caller's job.
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#pragma once

#include "ui_modals.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    UI_CONFIRM_LOW    = 0,   /* info — primary button colour, single tap default */
    UI_CONFIRM_MEDIUM = 1,   /* warn — warning button, 1.5 s default hold */
    UI_CONFIRM_HIGH   = 2,   /* destructive — alarm-red button, 3 s default hold */
} ui_confirm_severity_t;

typedef void (*ui_confirm_action_cb)(void *user_data);

typedef struct {
    const char           *title;            /* required, ≤ 32 chars */
    const char           *body[4];          /* up to 4 lines, NULL-terminate */
    const char           *final_warning;    /* optional dim italic line */
    const char           *cancel_label;     /* default "Cancel" if NULL */
    const char           *confirm_label;    /* required */
    uint32_t              hold_ms;          /* 0 = single tap; otherwise hold duration. If 0xFFFFFFFF, use severity default. */
    ui_confirm_severity_t severity;
    ui_confirm_action_cb  on_confirm;       /* fires on completed confirm */
    void                 *user_data;        /* passed to on_confirm */
} ui_confirm_params_t;

/* Show the modal. Returns the handle so the caller can force-close it
 * (e.g., state changed during display). Modal closes itself on Cancel /
 * outside-tap / completed confirm. Returns NULL on failure. */
ui_modal_handle_t *ui_confirm_show(const ui_confirm_params_t *params);

#define UI_CONFIRM_HOLD_DEFAULT 0xFFFFFFFF

#ifdef __cplusplus
}
#endif
