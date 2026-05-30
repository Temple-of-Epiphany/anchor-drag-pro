/*
 * Modal stack manager — precedence rules per Information-Architecture.
 *
 * Spec: https://github.com/Temple-of-Epiphany/anchor-drag-pro/wiki/Information-Architecture
 * Section: "Modal precedence".
 *
 * Precedence (highest first):
 *   ALARM > OTA > user-initiated (Preset Picker, Confirm)
 *
 * Modal types are registered with a priority. When a higher-priority
 * modal opens, lower-priority modals close (preserving their state for
 * later reopening would be a future enhancement; v0.2 just closes).
 *
 * Implementation skeleton — milestone 3 of #69. Subsequent modal
 * issues (#75-#78) instantiate against this API.
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#pragma once

#include "lvgl.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    UI_MODAL_PRIO_USER  = 0,   /* Preset picker, Confirm */
    UI_MODAL_PRIO_OTA   = 5,   /* OTA confirmation */
    UI_MODAL_PRIO_ALARM = 10,  /* Alarm — preempts everything */
} ui_modal_priority_t;

typedef struct ui_modal_handle ui_modal_handle_t;

/* Create a centred modal container at the given priority. Returns a
 * handle. The caller populates the modal contents inside the returned
 * lv_obj_t* via ui_modal_get_content().
 *
 * Calling this function does NOT yet show the modal — call ui_modal_open(). */
ui_modal_handle_t *ui_modal_create(int width, int height,
                                    ui_modal_priority_t prio);

/* Get the content container so the caller can add widgets into it. */
lv_obj_t *ui_modal_get_content(ui_modal_handle_t *m);

/* Push to the visible stack. Lower-priority modals are closed first. */
void ui_modal_open (ui_modal_handle_t *m);

/* Hide and free. */
void ui_modal_close(ui_modal_handle_t *m);

bool ui_modal_is_open(ui_modal_handle_t *m);

/* Anything currently visible at >= prio? Used to suppress lower-priority
 * triggers (e.g., don't open Preset Picker while OTA modal is up). */
bool ui_modal_any_at_or_above(ui_modal_priority_t prio);

#ifdef __cplusplus
}
#endif
