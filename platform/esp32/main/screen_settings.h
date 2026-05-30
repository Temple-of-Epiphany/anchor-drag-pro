/*
 * Settings screen — read-mostly config viewer with edit-on-touch
 * for a small subset (distance preset, sound, brightness, rotation,
 * units, idle dim).
 *
 * Spec: https://github.com/Temple-of-Epiphany/anchor-drag-pro/wiki/Screen-Settings
 * If you're changing behaviour here, update the wiki page first.
 *
 * Heavy edits (boat name, WiFi networks, coordinate format) route to
 * a "Edit in the web UI" toast — see spec for the rationale.
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t *screen_settings_create(void);

lv_obj_t *screen_settings_header(lv_obj_t *screen);
lv_obj_t *screen_settings_footer(lv_obj_t *screen);

#ifdef __cplusplus
}
#endif
