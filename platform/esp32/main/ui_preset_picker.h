/*
 * Modal: Preset Picker — alarm distance preset switch.
 *
 * Spec: https://github.com/Temple-of-Epiphany/anchor-drag-pro/wiki/Modal-Preset-Picker
 * If you're changing behaviour here, update the wiki page first.
 *
 * Centred modal with three tiles drawn from cfg.anchor.options.distances[].
 * Tap a tile → on_selected callback fires with the new index. The
 * caller persists (via anchor_config_save when that lands; #63) and
 * then closes the modal. Cancel / outside-tap dismisses without write.
 *
 * State lockout (ARMING/ARMED/ALARM/MUTED) is the caller's concern;
 * pass locked = true to render the dim variant + suppress writes.
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#pragma once

#include "ui_modals.h"
#include "anchor_config.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*ui_preset_picker_cb)(int new_selected_idx, void *user_data);

typedef struct {
    const anchor_distance_t *distances;   /* points at cfg.anchor.options.distances */
    int                      current_idx; /* 0..2 — the active preset on entry */
    bool                     locked;      /* true → tiles dim, no write fires */
    ui_preset_picker_cb      on_selected; /* fires with the new index (NOT called when locked) */
    void                    *user_data;
} ui_preset_picker_params_t;

/* Show the modal. Returns the modal handle. */
ui_modal_handle_t *ui_preset_picker_show(const ui_preset_picker_params_t *params);

#ifdef __cplusplus
}
#endif
