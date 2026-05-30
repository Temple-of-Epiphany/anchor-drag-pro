/*
 * Diagnostics screen — system tools, logs, hardware tests, destructive ops.
 *
 * Spec: https://github.com/Temple-of-Epiphany/anchor-drag-pro/wiki/Screen-Diagnostics
 * If you're changing behaviour here, update the wiki page first.
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t *screen_diagnostics_create(void);

lv_obj_t *screen_diagnostics_header(lv_obj_t *screen);
lv_obj_t *screen_diagnostics_footer(lv_obj_t *screen);

#ifdef __cplusplus
}
#endif
