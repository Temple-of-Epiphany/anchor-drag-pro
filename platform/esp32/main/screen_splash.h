/*
 * Splash screen — boot screen with version + self-test progression.
 *
 * Spec: https://github.com/Temple-of-Epiphany/anchor-drag-pro/wiki/Screen-Splash
 * If you're changing behaviour here, update the wiki page first.
 *
 * Milestone 1 (#69): minimum viable splash — brand + version + a single
 * "Booting..." line. The progressive self-test row list comes next.
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Create and show the splash screen. Must be called with the LVGL mutex
 * available (acquires it internally). Safe to call once per boot. */
esp_err_t screen_splash_show(const char *firmware_version);

/* Update the live "status" line below the version (e.g., "Loading drivers...").
 * No-op if the splash has been dismissed. */
void screen_splash_set_status(const char *text);

#ifdef __cplusplus
}
#endif
