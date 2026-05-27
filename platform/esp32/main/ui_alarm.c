/*
 * Modal: Alarm — implementation.
 *
 * Spec: https://github.com/Temple-of-Epiphany/anchor-drag-pro/wiki/Modal-Alarm
 *
 * Single-instance modal (only one alarm at a time per the spec).
 * Pulsing red background driven by an lv_anim. Audio is deferred —
 * the modal logs the points where the buzzer would fire.
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#include "ui_alarm.h"
#include "ui_tokens.h"
#include "lvgl_init.h"
#include "esp_log.h"
#include "lvgl.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>

static const char *TAG = "alarm";

typedef struct {
    ui_modal_handle_t   *modal;
    ui_alarm_params_t    params;

    lv_obj_t            *bg_layer;        /* full-screen red, pulses */
    lv_obj_t            *glyph;
    lv_obj_t            *title_lbl;
    lv_obj_t            *distance_lbl;
    lv_obj_t            *threshold_lbl;
    lv_obj_t            *hdg_rot_lbl;
    lv_obj_t            *time_lbl;
    lv_obj_t            *mute_btn;
    lv_obj_t            *disarm_btn;
    lv_obj_t            *disarm_fill;
    lv_timer_t          *disarm_hold_timer;
    lv_timer_t          *pulse_timer;     /* drives background pulse */
    bool                 muted;
    bool                 pulse_dim;
} alarm_state_t;

static alarm_state_t *s_active = NULL;

/* ---- Helpers ---- */

static void format_distance(char *buf, size_t sz, double v, const char *unit)
{
    snprintf(buf, sz, "Boat is %.0f %s from anchor", v, unit ? unit : "ft");
}

static void format_threshold(char *buf, size_t sz, double v, const char *unit)
{
    snprintf(buf, sz, "Alarm threshold: %.0f %s", v, unit ? unit : "ft");
}

static void format_hdg_rot(char *buf, size_t sz, int hdg, int rot)
{
    if (hdg < 0 && rot == INT_MIN) {
        snprintf(buf, sz, " ");
        return;
    }
    if (hdg < 0) {
        snprintf(buf, sz, "ROT %+d °/s", rot);
    } else if (rot == INT_MIN) {
        snprintf(buf, sz, "Heading %d°", hdg);
    } else {
        snprintf(buf, sz, "Heading %d°    ROT %+d °/s", hdg, rot);
    }
}

/* ---- Pulse animation ----
 * Switch between full bright and 80% bright on the background every
 * ~330 ms (≈1.5 Hz). LVGL doesn't easily animate opacity on a solid
 * background, so we toggle the bg color between two shades. */
static void pulse_tick(lv_timer_t *t)
{
    alarm_state_t *st = (alarm_state_t *) t->user_data;
    if (!st || st->muted) return;
    st->pulse_dim = !st->pulse_dim;
    /* Use red with two brightness levels by mixing toward black. */
    lv_color_t base = UI_COLOR(STATE_ALARM);
    lv_color_t dim  = lv_color_darken(base, LV_OPA_30);
    lv_obj_set_style_bg_color(st->bg_layer, st->pulse_dim ? dim : base, LV_PART_MAIN);
}

/* ---- Cleanup ---- */

static void clear_disarm_hold(alarm_state_t *st)
{
    if (!st) return;
    if (st->disarm_hold_timer) {
        lv_timer_del(st->disarm_hold_timer);
        st->disarm_hold_timer = NULL;
    }
    lv_anim_del(st->disarm_fill, NULL);
    lv_obj_set_width(st->disarm_fill, 0);
}

static void destroy_state(alarm_state_t *st)
{
    if (!st) return;
    clear_disarm_hold(st);
    if (st->pulse_timer) {
        lv_timer_del(st->pulse_timer);
        st->pulse_timer = NULL;
    }
    if (st->modal) {
        ui_modal_close(st->modal);
        lv_obj_t *content = ui_modal_get_content(st->modal);
        if (content) {
            lv_obj_t *scrim = lv_obj_get_parent(content);
            if (scrim) lv_obj_del(scrim);
        }
        free(st->modal);
    }
    free(st);
}

/* ---- Event handlers ---- */

static void on_mute_clicked(lv_event_t *e)
{
    alarm_state_t *st = (alarm_state_t *) lv_event_get_user_data(e);
    if (!st) return;
    ESP_LOGI(TAG, "MUTE — siren off (buzzer driver TBD)");
    ui_alarm_action_cb cb = st->params.on_mute;
    void *ud              = st->params.user_data;
    ui_alarm_set_muted(true);
    if (cb) cb(ud);
}

static void anim_set_width_cb(void *var, int32_t v)
{
    lv_obj_set_width((lv_obj_t *) var, v);
}

static void on_disarm_hold_fired(lv_timer_t *t)
{
    alarm_state_t *st = (alarm_state_t *) t->user_data;
    if (!st) return;
    ESP_LOGI(TAG, "DISARM confirmed — dismissing alarm modal");
    ui_alarm_action_cb cb = st->params.on_disarm;
    void *ud              = st->params.user_data;
    destroy_state(st);
    if (s_active == st) s_active = NULL;
    if (cb) cb(ud);
}

static void on_disarm_pressed(lv_event_t *e)
{
    alarm_state_t *st = (alarm_state_t *) lv_event_get_user_data(e);
    if (!st) return;
    clear_disarm_hold(st);
    const uint32_t hold_ms = 1000;
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, st->disarm_fill);
    lv_anim_set_exec_cb(&a, anim_set_width_cb);
    lv_anim_set_values(&a, 0, lv_obj_get_width(st->disarm_btn));
    lv_anim_set_time(&a, hold_ms);
    lv_anim_start(&a);
    st->disarm_hold_timer = lv_timer_create(on_disarm_hold_fired, hold_ms, st);
    lv_timer_set_repeat_count(st->disarm_hold_timer, 1);
}

static void on_disarm_released(lv_event_t *e)
{
    alarm_state_t *st = (alarm_state_t *) lv_event_get_user_data(e);
    if (!st) return;
    if (st->disarm_hold_timer) {
        clear_disarm_hold(st);
        ESP_LOGI(TAG, "DISARM hold aborted");
    }
}

/* ---- Construction ---- */

ui_modal_handle_t *ui_alarm_show(const ui_alarm_params_t *params)
{
    if (!params) return NULL;

    /* If already shown, just refresh the live values. */
    if (s_active) {
        ui_alarm_update(params->distance_value, params->threshold,
                        params->heading_deg, params->rot_dps);
        return s_active->modal;
    }

    if (!lvgl_lock(2000)) {
        ESP_LOGE(TAG, "could not acquire LVGL mutex");
        return NULL;
    }

    alarm_state_t *st = calloc(1, sizeof(*st));
    if (!st) { lvgl_unlock(); return NULL; }
    st->params = *params;

    /* Alarm modal sits at the highest priority — preempts everything. */
    st->modal = ui_modal_create(LV_HOR_RES, LV_VER_RES, UI_MODAL_PRIO_ALARM);
    if (!st->modal) { free(st); lvgl_unlock(); return NULL; }

    lv_obj_t *content = ui_modal_get_content(st->modal);
    lv_obj_set_user_data(content, st);
    lv_obj_set_style_bg_opa      (content, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(content, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all     (content, 0, LV_PART_MAIN);

    /* Full-screen red layer fills the content (the scrim is also full-
     * screen at 60% black; this red overlays it). */
    st->bg_layer = lv_obj_create(content);
    lv_obj_set_size(st->bg_layer, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color    (st->bg_layer, UI_COLOR(STATE_ALARM), LV_PART_MAIN);
    lv_obj_set_style_bg_opa      (st->bg_layer, LV_OPA_COVER,          LV_PART_MAIN);
    lv_obj_set_style_border_width(st->bg_layer, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all     (st->bg_layer, 0, LV_PART_MAIN);
    lv_obj_clear_flag            (st->bg_layer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(st->bg_layer, LV_ALIGN_CENTER, 0, 0);

    /* Warning glyph centre-top. */
    st->glyph = lv_label_create(st->bg_layer);
    lv_label_set_text(st->glyph, LV_SYMBOL_WARNING);
    lv_obj_set_style_text_color(st->glyph, UI_COLOR(TEXT_BODY_STRONG), LV_PART_MAIN);
    lv_obj_set_style_text_font (st->glyph, &lv_font_montserrat_32,     LV_PART_MAIN);
    lv_obj_align(st->glyph, LV_ALIGN_TOP_MID, 0, 60);

    /* Title */
    st->title_lbl = lv_label_create(st->bg_layer);
    lv_label_set_text(st->title_lbl, "ANCHOR DRAG");
    lv_obj_set_style_text_color(st->title_lbl, UI_COLOR(TEXT_BODY_STRONG), LV_PART_MAIN);
    lv_obj_set_style_text_font (st->title_lbl, &lv_font_montserrat_32,     LV_PART_MAIN);
    lv_obj_align(st->title_lbl, LV_ALIGN_TOP_MID, 0, 110);

    /* Distance */
    char buf[96];
    format_distance(buf, sizeof(buf), st->params.distance_value, st->params.distance_unit);
    st->distance_lbl = lv_label_create(st->bg_layer);
    lv_label_set_text(st->distance_lbl, buf);
    lv_obj_set_style_text_color(st->distance_lbl, UI_COLOR(TEXT_BODY_STRONG), LV_PART_MAIN);
    lv_obj_set_style_text_font (st->distance_lbl, &lv_font_montserrat_24,     LV_PART_MAIN);
    lv_obj_align(st->distance_lbl, LV_ALIGN_TOP_MID, 0, 170);

    format_threshold(buf, sizeof(buf), st->params.threshold, st->params.distance_unit);
    st->threshold_lbl = lv_label_create(st->bg_layer);
    lv_label_set_text(st->threshold_lbl, buf);
    lv_obj_set_style_text_color(st->threshold_lbl, UI_COLOR(TEXT_BODY), LV_PART_MAIN);
    lv_obj_set_style_text_font (st->threshold_lbl, &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_align(st->threshold_lbl, LV_ALIGN_TOP_MID, 0, 210);

    format_hdg_rot(buf, sizeof(buf), st->params.heading_deg, st->params.rot_dps);
    st->hdg_rot_lbl = lv_label_create(st->bg_layer);
    lv_label_set_text(st->hdg_rot_lbl, buf);
    lv_obj_set_style_text_color(st->hdg_rot_lbl, UI_COLOR(TEXT_BODY), LV_PART_MAIN);
    lv_obj_set_style_text_font (st->hdg_rot_lbl, &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_align(st->hdg_rot_lbl, LV_ALIGN_TOP_MID, 0, 245);

    /* MUTE button — huge, single-tap. */
    st->mute_btn = lv_btn_create(st->bg_layer);
    lv_obj_set_size(st->mute_btn, 480, 110);
    lv_obj_align(st->mute_btn, LV_ALIGN_CENTER, 0, 80);
    lv_obj_set_style_bg_color(st->mute_btn, lv_color_darken(UI_COLOR(STATE_ALARM), LV_OPA_40), LV_PART_MAIN);
    lv_obj_set_style_radius  (st->mute_btn, 12, LV_PART_MAIN);
    lv_obj_set_style_border_width(st->mute_btn, 0, LV_PART_MAIN);
    lv_obj_t *mute_lbl = lv_label_create(st->mute_btn);
    lv_label_set_text(mute_lbl, "M U T E");
    lv_obj_set_style_text_color(mute_lbl, UI_COLOR(TEXT_BODY_STRONG), LV_PART_MAIN);
    lv_obj_set_style_text_font (mute_lbl, &lv_font_montserrat_32,     LV_PART_MAIN);
    lv_obj_center(mute_lbl);
    lv_obj_add_event_cb(st->mute_btn, on_mute_clicked, LV_EVENT_CLICKED, st);

    /* DISARM button — smaller, bottom-left, hold-to-confirm. */
    st->disarm_btn = lv_btn_create(st->bg_layer);
    lv_obj_set_size(st->disarm_btn, 240, 60);
    lv_obj_align(st->disarm_btn, LV_ALIGN_BOTTOM_LEFT, 20, -20);
    lv_obj_set_style_bg_color    (st->disarm_btn, UI_COLOR(ACTION_DESTRUCTIVE), LV_PART_MAIN);
    lv_obj_set_style_radius      (st->disarm_btn, 8, LV_PART_MAIN);
    lv_obj_set_style_border_width(st->disarm_btn, 0, LV_PART_MAIN);
    lv_obj_set_style_clip_corner (st->disarm_btn, true, LV_PART_MAIN);

    st->disarm_fill = lv_obj_create(st->disarm_btn);
    lv_obj_set_size(st->disarm_fill, 0, 60);
    lv_obj_align(st->disarm_fill, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_bg_color    (st->disarm_fill, UI_COLOR(TEXT_BODY_STRONG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa      (st->disarm_fill, LV_OPA_30, LV_PART_MAIN);
    lv_obj_set_style_border_width(st->disarm_fill, 0, LV_PART_MAIN);

    lv_obj_t *dlbl = lv_label_create(st->disarm_btn);
    lv_label_set_text(dlbl, "DISARM (hold 1 s)");
    lv_obj_set_style_text_color(dlbl, UI_COLOR(TEXT_BODY_STRONG), LV_PART_MAIN);
    lv_obj_set_style_text_font (dlbl, &lv_font_montserrat_16,     LV_PART_MAIN);
    lv_obj_center(dlbl);
    lv_obj_move_foreground(dlbl);
    lv_obj_add_event_cb(st->disarm_btn, on_disarm_pressed,  LV_EVENT_PRESSED,    st);
    lv_obj_add_event_cb(st->disarm_btn, on_disarm_released, LV_EVENT_RELEASED,   st);
    lv_obj_add_event_cb(st->disarm_btn, on_disarm_released, LV_EVENT_PRESS_LOST, st);

    /* Time bottom-right */
    st->time_lbl = lv_label_create(st->bg_layer);
    lv_label_set_text(st->time_lbl, "--:--:--");
    lv_obj_set_style_text_color(st->time_lbl, UI_COLOR(TEXT_BODY_STRONG), LV_PART_MAIN);
    lv_obj_set_style_text_font (st->time_lbl, &lv_font_montserrat_22,     LV_PART_MAIN);
    lv_obj_align(st->time_lbl, LV_ALIGN_BOTTOM_RIGHT, -20, -30);

    /* Start the background pulse. */
    st->pulse_timer = lv_timer_create(pulse_tick, 330, st);

    ESP_LOGW(TAG, "ALARM modal opened — siren on (buzzer driver TBD)");
    ui_modal_open(st->modal);

    s_active = st;
    lvgl_unlock();
    return st->modal;
}

void ui_alarm_update(double distance_value, double threshold,
                      int heading_deg, int rot_dps)
{
    if (!s_active) return;
    if (!lvgl_lock(500)) return;
    char buf[96];
    format_distance(buf, sizeof(buf), distance_value, s_active->params.distance_unit);
    lv_label_set_text(s_active->distance_lbl, buf);
    format_threshold(buf, sizeof(buf), threshold, s_active->params.distance_unit);
    lv_label_set_text(s_active->threshold_lbl, buf);
    format_hdg_rot(buf, sizeof(buf), heading_deg, rot_dps);
    lv_label_set_text(s_active->hdg_rot_lbl, buf);
    s_active->params.distance_value = distance_value;
    s_active->params.threshold      = threshold;
    s_active->params.heading_deg    = heading_deg;
    s_active->params.rot_dps        = rot_dps;
    lvgl_unlock();
}

void ui_alarm_set_muted(bool muted)
{
    if (!s_active) return;
    if (!lvgl_lock(500)) return;
    s_active->muted = muted;
    if (muted) {
        /* Stop the pulse; lock the background to bright red. */
        if (s_active->pulse_timer) {
            lv_timer_del(s_active->pulse_timer);
            s_active->pulse_timer = NULL;
        }
        lv_obj_set_style_bg_color(s_active->bg_layer, UI_COLOR(STATE_ALARM), LV_PART_MAIN);
    } else {
        if (!s_active->pulse_timer) {
            s_active->pulse_timer = lv_timer_create(pulse_tick, 330, s_active);
        }
    }
    lvgl_unlock();
}

void ui_alarm_dismiss(void)
{
    if (!s_active) return;
    alarm_state_t *st = s_active;
    s_active = NULL;
    destroy_state(st);
}
