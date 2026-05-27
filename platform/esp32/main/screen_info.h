/*
 * Info screen — GPS + compass + heading + heel + pitch + ROT.
 *
 * Spec: https://github.com/Temple-of-Epiphany/anchor-drag-pro/wiki/Screen-Info
 * If you're changing behaviour here, update the wiki page first.
 *
 * Read-only. Subscribes (eventually) to the GPS source manager + IMU
 * source manager for live updates. Until those land, screen displays
 * "––" placeholders per the spec's empty-state rules.
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t *screen_info_create(void);

lv_obj_t *screen_info_header(lv_obj_t *screen);
lv_obj_t *screen_info_footer(lv_obj_t *screen);

#ifdef __cplusplus
}
#endif
