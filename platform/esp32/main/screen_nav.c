/*
 * Screen navigation — implementation.
 *
 * Spec: https://github.com/Temple-of-Epiphany/anchor-drag-pro/wiki/Information-Architecture
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#include "screen_nav.h"
#include "lvgl_init.h"
#include "esp_log.h"
#include "lvgl.h"

static const char *TAG = "nav";

static lv_obj_t *s_screens[SCREEN_NAV_MAX];
static int       s_count   = 0;
static int       s_current = -1;

void screen_nav_register(int index, lv_obj_t *screen)
{
    if (index < 0 || index >= SCREEN_NAV_MAX) return;
    s_screens[index] = screen;
    if (index + 1 > s_count) s_count = index + 1;
}

void screen_nav_switch_to(int index)
{
    if (index < 0 || index >= s_count) return;
    if (!s_screens[index]) return;
    if (!lvgl_lock(500)) return;
    lv_scr_load(s_screens[index]);
    lvgl_unlock();
    s_current = index;
    ESP_LOGI(TAG, "switched to screen %d", index);
}

int screen_nav_current(void) { return s_current; }

static int find_screen_index(lv_obj_t *scr)
{
    for (int i = 0; i < s_count; i++) {
        if (s_screens[i] == scr) return i;
    }
    return -1;
}

/* Gesture callback — fires on swipe events anywhere on the screen. */
static void on_gesture(lv_event_t *e)
{
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    lv_obj_t *origin = lv_event_get_current_target(e);
    int idx = find_screen_index(origin);
    if (idx < 0 || s_count == 0) return;

    /* Swipe LEFT → move toward higher index (next screen on the right).
     * Swipe RIGHT → previous (lower index). Wrap around. */
    int next = idx;
    if (dir == LV_DIR_LEFT) {
        next = (idx + 1) % s_count;
    } else if (dir == LV_DIR_RIGHT) {
        next = (idx - 1 + s_count) % s_count;
    } else {
        return;
    }
    screen_nav_switch_to(next);
}

void screen_nav_attach_gestures(lv_obj_t *screen)
{
    if (!screen) return;
    lv_obj_add_event_cb(screen, on_gesture, LV_EVENT_GESTURE, NULL);
}
