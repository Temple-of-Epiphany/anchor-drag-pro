/*
 * Monitor screen — anchor state machine UI + circle plot.
 *
 * Spec: https://github.com/Temple-of-Epiphany/anchor-drag-pro/wiki/Screen-Monitor
 * If you're changing behaviour here, update the wiki page first.
 *
 * Layout per spec:
 *   - Persistent header (state pill / boat name / time / status icons)
 *   - Left column 480px: plot canvas (anchor circle + boat icon + trail)
 *   - Right column 320px: distance readout + secondary readouts + primary action button
 *   - Persistent footer (◄ Info — Monitor — Settings ►)
 *
 * Milestone 1 of #70: layout shell + state pill + distance label +
 * primary action button. Plot canvas is a placeholder until GPS source
 * manager lands (separate workstream). Real anchor state machine
 * integration is also pending — for now the screen accepts a state via
 * screen_monitor_set_state() and main.c calls it with OFF on boot.
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#pragma once

#include "ui_chrome.h"
#include "esp_err.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Create the Monitor screen object (does NOT load it). Returns NULL
 * on failure. The screen is parented to NULL (top-level), ready to be
 * passed to lv_scr_load(). */
lv_obj_t *screen_monitor_create(void);

/* Drive the screen's state-dependent rendering. Pass the state pill
 * enum from ui_chrome.h. Updates the action button label/colour, the
 * distance readout, and the screen tint. Re-callable; takes the LVGL
 * mutex internally. */
void screen_monitor_set_state(lv_obj_t *monitor, ui_state_pill_t state);

/* Update the live distance readout. Pass the magnitude + unit string
 * (e.g. 35.2, "ft"). Use NULL/0 to show the alarm-distance template.
 * Re-callable; safe at any state. */
void screen_monitor_set_distance(lv_obj_t *monitor,
                                  double value, const char *unit,
                                  double alarm_at);

/* Update the boat-name displayed in the header. */
void screen_monitor_set_boat_name(lv_obj_t *monitor, const char *name);

/* Direct access to the persistent header/footer/banner widgets so
 * other code (clock task, source-health pub/sub) can update them. */
lv_obj_t *screen_monitor_header(lv_obj_t *monitor);
lv_obj_t *screen_monitor_footer(lv_obj_t *monitor);
lv_obj_t *screen_monitor_banner(lv_obj_t *monitor);

#ifdef __cplusplus
}
#endif
