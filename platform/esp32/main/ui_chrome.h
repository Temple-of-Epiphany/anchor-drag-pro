/*
 * Persistent UI chrome — header, footer, warning banner.
 *
 * Spec: https://github.com/Temple-of-Epiphany/anchor-drag-pro/wiki/Information-Architecture
 * If you're changing behaviour here, update the wiki page first.
 *
 * These are reusable LVGL widgets consumed by every screen except
 * Splash and full-screen modals. The widgets render token-driven
 * styling and expose setter functions so the parent screen can
 * update content live (state pill, time, status icons, banner copy).
 *
 * Milestone 3 of #69: skeleton API + visual scaffolding. Screens
 * (#70-#74) instantiate these and wire them to live data.
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

/* ---- Header ---- */

typedef enum {
    UI_STATE_PILL_OFF = 0,
    UI_STATE_PILL_ON,
    UI_STATE_PILL_ARMING,
    UI_STATE_PILL_ARMED,
    UI_STATE_PILL_ALARM,
    UI_STATE_PILL_MUTED,
} ui_state_pill_t;

typedef enum {
    UI_ICON_OFF = 0,    /* dim / strikethrough */
    UI_ICON_OK,         /* healthy */
    UI_ICON_WARN,       /* degraded */
    UI_ICON_ALARM,      /* failed */
} ui_icon_state_t;

/* Create the 800x64 header bar inside `parent`. Returns the container.
 * Use the setters below to update content live. */
lv_obj_t *ui_header_create(lv_obj_t *parent);

void ui_header_set_state_pill(lv_obj_t *header, ui_state_pill_t s);
void ui_header_set_boat_name (lv_obj_t *header, const char *name);
void ui_header_set_time      (lv_obj_t *header, int hour24, int minute);
void ui_header_set_icon_sd   (lv_obj_t *header, ui_icon_state_t s);
void ui_header_set_icon_wifi (lv_obj_t *header, ui_icon_state_t s);
void ui_header_set_icon_gps  (lv_obj_t *header, ui_icon_state_t s);

/* ---- Footer ---- */

/* Tab-bar style. Shows current section name + two flanking labels. */
lv_obj_t *ui_footer_create(lv_obj_t *parent);

void ui_footer_set_sections(lv_obj_t *footer,
                             const char *prev_label,
                             const char *current_label,
                             const char *next_label);

/* ---- Warning banner ---- */

typedef enum {
    UI_BANNER_INFO = 0,
    UI_BANNER_WARN,
    UI_BANNER_ALARM,
} ui_banner_level_t;

/* Create a hidden banner bar; call ui_banner_show() to surface it.
 * Returns NULL on failure. */
lv_obj_t *ui_banner_create(lv_obj_t *parent);

void ui_banner_show (lv_obj_t *banner,
                     ui_banner_level_t level,
                     const char *message,
                     const char *action_label);   /* NULL = no button */
void ui_banner_hide (lv_obj_t *banner);
bool ui_banner_visible(lv_obj_t *banner);

#ifdef __cplusplus
}
#endif
