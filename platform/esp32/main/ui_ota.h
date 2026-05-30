/*
 * Modal: OTA — firmware update confirmation + progress.
 *
 * Spec: https://github.com/Temple-of-Epiphany/anchor-drag-pro/wiki/Modal-OTA
 * If you're changing behaviour here, update the wiki page first.
 *
 * Priority UI_MODAL_PRIO_OTA — above user modals, below the alarm.
 * Three phases share one mounted modal:
 *   AVAILABLE : current → new version, optional notes, [Skip] [Install]
 *   PROGRESS  : phase text + progress bar, no buttons (flash can't be
 *               safely cancelled mid-write)
 *   RESULT    : success auto-reboots; failure shows a message + [Dismiss]
 *
 * The install itself runs on a caller-supplied worker task (the flash
 * blocks for the whole write and ends in esp_restart). Progress is
 * pushed back via ui_ota_set_progress(), which locks LVGL internally
 * so it is safe to call from that worker task.
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#pragma once

#include "ui_modals.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*ui_ota_action_cb)(void *user_data);

typedef struct {
    int from_major, from_minor, from_patch;
    int to_major, to_minor, to_patch;
    const char       *notes;        /* optional; may be NULL */
    ui_ota_action_cb  on_install;   /* user tapped Install */
    ui_ota_action_cb  on_skip;      /* Skip button or outside tap */
    void             *user_data;
} ui_ota_params_t;

/* Show the AVAILABLE phase. Returns the modal handle (single instance —
 * a second call while one is open is a no-op returning the existing one). */
ui_modal_handle_t *ui_ota_show(const ui_ota_params_t *params);

/* Switch the open modal to the PROGRESS phase (call from on_install
 * before kicking off the flash worker). Buttons are removed. */
void ui_ota_begin_progress(void);

/* Update the progress bar + phase caption. pct is 0..100. Thread-safe. */
void ui_ota_set_progress(const char *phase, int pct);

/* Switch to the RESULT/error phase: shows msg + a Dismiss button. */
void ui_ota_show_error(const char *msg);

/* Dismiss the modal entirely. */
void ui_ota_dismiss(void);

#ifdef __cplusplus
}
#endif
