/*
 * Screen navigation — swipe-left/right carousel.
 *
 * Spec: https://github.com/Temple-of-Epiphany/anchor-drag-pro/wiki/Information-Architecture
 * Section: "Sitemap" + "Gesture map".
 *
 * Owns the current set of top-level screens and dispatches swipe
 * gestures from any registered screen to the next/previous one. As
 * each new top-level screen lands (Settings #73, Diagnostics #74) it
 * registers via screen_nav_register().
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#pragma once

#include "lvgl.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SCREEN_NAV_MAX 5

/* Register a screen at the given index. Index defines swipe order:
 * 0 = leftmost (Connections), monotonic increasing rightward. Wrap-
 * around behaviour: swipe past the last screen wraps to the first. */
void screen_nav_register(int index, lv_obj_t *screen);

/* Switch to the screen at `index`. No-op if out of range. */
void screen_nav_switch_to(int index);

/* Returns the index of the currently displayed screen, or -1. */
int  screen_nav_current(void);

/* Hook the swipe handler onto a screen. Call once per screen after
 * registering. */
void screen_nav_attach_gestures(lv_obj_t *screen);

#ifdef __cplusplus
}
#endif
