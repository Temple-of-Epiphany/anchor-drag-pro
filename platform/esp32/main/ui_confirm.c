/*
 * Modal: Confirm — implementation.
 *
 * Spec: https://github.com/Temple-of-Epiphany/anchor-drag-pro/wiki/Modal-Confirm
 *
 * Hold-to-confirm uses an lv_timer set on PRESSED and cancelled on
 * RELEASED. The visual fill bar inside the confirm button is driven
 * by an lv_anim that runs in parallel; both end at the same time.
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#include "ui_confirm.h"
#include "ui_tokens.h"
#include "lvgl_init.h"
#include "esp_log.h"
#include "lvgl.h"
#include <stdlib.h>
#include <string.h>

static const char *TAG = "confirm";

/* ---- Per-instance state ---- */

typedef struct {
    ui_modal_handle_t   *modal;
    ui_confirm_params_t  params;     /* deep-copied */
    char                 title_buf[64];
    char                 body_bufs[4][96];
    char                 final_buf[96];
    char                 cancel_buf[32];
    char                 confirm_buf[48];

    lv_obj_t            *btn_confirm;
    lv_obj_t            *btn_confirm_fill;   /* progress overlay */
    lv_timer_t          *hold_timer;
    lv_anim_t            fill_anim;
} confirm_state_t;

/* ---- Severity → defaults ---- */

static uint32_t default_hold_ms(ui_confirm_severity_t s)
{
    switch (s) {
        case UI_CONFIRM_LOW:    return 0;
        case UI_CONFIRM_MEDIUM: return 1500;
        case UI_CONFIRM_HIGH:   return 3000;
        default:                return 3000;
    }
}

static lv_color_t severity_button_color(ui_confirm_severity_t s)
{
    switch (s) {
        case UI_CONFIRM_LOW:    return UI_COLOR(ACTION_PRIMARY);
        case UI_CONFIRM_MEDIUM: return UI_COLOR(ACTION_WARNING);
        case UI_CONFIRM_HIGH:   return UI_COLOR(ACTION_DESTRUCTIVE);
        default:                return UI_COLOR(ACTION_DESTRUCTIVE);
    }
}

static lv_color_t severity_glyph_color(ui_confirm_severity_t s)
{
    switch (s) {
        case UI_CONFIRM_LOW:    return UI_COLOR(STATE_ON);
        case UI_CONFIRM_MEDIUM: return UI_COLOR(STATE_WARNING);
        case UI_CONFIRM_HIGH:   return UI_COLOR(STATE_ALARM);
        default:                return UI_COLOR(STATE_ALARM);
    }
}

static const char *severity_glyph(ui_confirm_severity_t s)
{
    switch (s) {
        case UI_CONFIRM_LOW:    return LV_SYMBOL_OK;        /* placeholder for ℹ */
        case UI_CONFIRM_MEDIUM: return LV_SYMBOL_WARNING;
        case UI_CONFIRM_HIGH:   return LV_SYMBOL_CLOSE;
        default:                return LV_SYMBOL_WARNING;
    }
}

/* ---- Cleanup ---- */

static void cleanup_state(confirm_state_t *st)
{
    if (!st) return;
    if (st->hold_timer) {
        lv_timer_del(st->hold_timer);
        st->hold_timer = NULL;
    }
    lv_anim_del(st->btn_confirm_fill, NULL);
}

static void close_and_free(confirm_state_t *st)
{
    if (!st) return;
    cleanup_state(st);
    if (st->modal) {
        ui_modal_close(st->modal);
        /* ui_modal_close hides but doesn't free; explicitly delete the
         * scrim so LVGL releases the widgets. */
        lv_obj_t *content = ui_modal_get_content(st->modal);
        if (content) {
            lv_obj_t *scrim = lv_obj_get_parent(content);
            if (scrim) lv_obj_del(scrim);
        }
        /* And free the handle backing memory. ui_modal_create allocs;
         * we own that. */
        free(st->modal);
    }
    free(st);
}

/* ---- Event handlers ---- */

static void on_cancel_clicked(lv_event_t *e)
{
    confirm_state_t *st = (confirm_state_t *) lv_event_get_user_data(e);
    ESP_LOGI(TAG, "cancelled");
    close_and_free(st);
}

static void on_scrim_clicked(lv_event_t *e)
{
    /* Tap outside the modal (on the scrim) = cancel. */
    lv_obj_t *target = lv_event_get_target(e);
    lv_obj_t *content = (lv_obj_t *) lv_event_get_user_data(e);
    /* Only treat clicks on the scrim itself, not bubbled clicks from
     * the modal content. */
    if (target == content) return;
    confirm_state_t *st = (confirm_state_t *) lv_obj_get_user_data(content);
    ESP_LOGI(TAG, "cancelled via outside tap");
    close_and_free(st);
}

static void anim_set_width_cb(void *var, int32_t v)
{
    lv_obj_set_width((lv_obj_t *) var, v);
}

static void hold_timer_fired(lv_timer_t *t)
{
    confirm_state_t *st = (confirm_state_t *) t->user_data;
    if (!st) return;
    /* Run the callback then dismiss. Callback gets the user_data, not
     * the state — caller never sees this widget's internals. */
    ESP_LOGI(TAG, "confirmed (hold completed)");
    ui_confirm_action_cb cb = st->params.on_confirm;
    void *ud               = st->params.user_data;
    close_and_free(st);
    if (cb) cb(ud);
}

static void on_confirm_pressed(lv_event_t *e)
{
    confirm_state_t *st = (confirm_state_t *) lv_event_get_user_data(e);
    if (!st) return;

    uint32_t ms = st->params.hold_ms;
    if (ms == UI_CONFIRM_HOLD_DEFAULT) ms = default_hold_ms(st->params.severity);

    if (ms == 0) {
        /* Single-tap path — fire immediately on PRESSED.
         * (Could also wait for CLICKED, but PRESSED feels snappier and
         * the user is committed once they touch.) */
        ESP_LOGI(TAG, "confirmed (single tap)");
        ui_confirm_action_cb cb = st->params.on_confirm;
        void *ud               = st->params.user_data;
        close_and_free(st);
        if (cb) cb(ud);
        return;
    }

    /* Hold-to-confirm path: start fill animation + arm one-shot timer. */
    lv_obj_set_width(st->btn_confirm_fill, 0);
    lv_obj_clear_flag(st->btn_confirm_fill, LV_OBJ_FLAG_HIDDEN);

    lv_coord_t target_w = lv_obj_get_width(st->btn_confirm);
    lv_anim_init(&st->fill_anim);
    lv_anim_set_var(&st->fill_anim, st->btn_confirm_fill);
    lv_anim_set_exec_cb(&st->fill_anim, anim_set_width_cb);
    lv_anim_set_values(&st->fill_anim, 0, target_w);
    lv_anim_set_time(&st->fill_anim, ms);
    lv_anim_start(&st->fill_anim);

    st->hold_timer = lv_timer_create(hold_timer_fired, ms, st);
    lv_timer_set_repeat_count(st->hold_timer, 1);
}

static void on_confirm_released(lv_event_t *e)
{
    confirm_state_t *st = (confirm_state_t *) lv_event_get_user_data(e);
    if (!st) return;

    /* If the timer is still alive, the user released early — abort. */
    if (st->hold_timer) {
        lv_timer_del(st->hold_timer);
        st->hold_timer = NULL;
        lv_anim_del(st->btn_confirm_fill, NULL);
        lv_obj_set_width(st->btn_confirm_fill, 0);
        lv_obj_add_flag(st->btn_confirm_fill, LV_OBJ_FLAG_HIDDEN);
        ESP_LOGI(TAG, "hold aborted (released early)");
    }
}

/* ---- Build + show ---- */

static void deep_copy_params(confirm_state_t *st, const ui_confirm_params_t *src)
{
    st->params = *src;
    if (src->title) {
        snprintf(st->title_buf, sizeof(st->title_buf), "%s", src->title);
        st->params.title = st->title_buf;
    }
    for (int i = 0; i < 4; i++) {
        if (src->body[i]) {
            snprintf(st->body_bufs[i], sizeof(st->body_bufs[i]), "%s", src->body[i]);
            st->params.body[i] = st->body_bufs[i];
        } else {
            st->params.body[i] = NULL;
        }
    }
    if (src->final_warning) {
        snprintf(st->final_buf, sizeof(st->final_buf), "%s", src->final_warning);
        st->params.final_warning = st->final_buf;
    }
    snprintf(st->cancel_buf, sizeof(st->cancel_buf), "%s",
             src->cancel_label ? src->cancel_label : "Cancel");
    st->params.cancel_label = st->cancel_buf;
    if (src->confirm_label) {
        snprintf(st->confirm_buf, sizeof(st->confirm_buf), "%s", src->confirm_label);
        st->params.confirm_label = st->confirm_buf;
    }
}

ui_modal_handle_t *ui_confirm_show(const ui_confirm_params_t *params)
{
    if (!params || !params->title || !params->confirm_label) {
        ESP_LOGE(TAG, "title and confirm_label required");
        return NULL;
    }

    if (!lvgl_lock(2000)) {
        ESP_LOGE(TAG, "could not acquire LVGL mutex");
        return NULL;
    }

    confirm_state_t *st = calloc(1, sizeof(*st));
    if (!st) { lvgl_unlock(); return NULL; }
    deep_copy_params(st, params);

    /* Create the modal at user priority. */
    st->modal = ui_modal_create(520, 320, UI_MODAL_PRIO_USER);
    if (!st->modal) { free(st); lvgl_unlock(); return NULL; }

    lv_obj_t *content = ui_modal_get_content(st->modal);
    lv_obj_set_user_data(content, st);     /* outside-tap dismissal lookup */
    lv_obj_set_layout    (content, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow (content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(content, 8, LV_PART_MAIN);

    /* Title row: glyph + title text. */
    lv_obj_t *title_row = lv_obj_create(content);
    lv_obj_set_size(title_row, lv_pct(100), 36);
    lv_obj_set_style_bg_opa      (title_row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(title_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all     (title_row, 0, LV_PART_MAIN);
    lv_obj_set_layout            (title_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow         (title_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align        (title_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column  (title_row, 12, LV_PART_MAIN);
    lv_obj_clear_flag            (title_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *glyph = lv_label_create(title_row);
    lv_label_set_text(glyph, severity_glyph(st->params.severity));
    lv_obj_set_style_text_color(glyph, severity_glyph_color(st->params.severity), LV_PART_MAIN);
    lv_obj_set_style_text_font (glyph, &lv_font_montserrat_24, LV_PART_MAIN);

    lv_obj_t *title = lv_label_create(title_row);
    lv_label_set_text(title, st->params.title);
    lv_obj_set_style_text_color(title, UI_COLOR(TEXT_BODY_STRONG), LV_PART_MAIN);
    lv_obj_set_style_text_font (title, &lv_font_montserrat_22, LV_PART_MAIN);

    /* Body lines. */
    for (int i = 0; i < 4 && st->params.body[i]; i++) {
        lv_obj_t *line = lv_label_create(content);
        lv_label_set_text(line, st->params.body[i]);
        lv_obj_set_style_text_color(line, UI_COLOR(TEXT_BODY), LV_PART_MAIN);
        lv_obj_set_style_text_font (line, &lv_font_montserrat_16, LV_PART_MAIN);
        lv_obj_set_width           (line, lv_pct(100));
        lv_label_set_long_mode     (line, LV_LABEL_LONG_WRAP);
    }

    /* Final warning. */
    if (st->params.final_warning) {
        lv_obj_t *spacer = lv_obj_create(content);
        lv_obj_set_size(spacer, 1, 4);
        lv_obj_set_style_bg_opa(spacer, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(spacer, 0, LV_PART_MAIN);

        lv_obj_t *fw = lv_label_create(content);
        lv_label_set_text(fw, st->params.final_warning);
        lv_obj_set_style_text_color(fw, UI_COLOR(STATE_WARNING), LV_PART_MAIN);
        lv_obj_set_style_text_font (fw, &lv_font_montserrat_16, LV_PART_MAIN);
        lv_obj_set_width           (fw, lv_pct(100));
        lv_label_set_long_mode     (fw, LV_LABEL_LONG_WRAP);
    }

    /* Spacer to push buttons to the bottom. */
    lv_obj_t *flex_spacer = lv_obj_create(content);
    lv_obj_set_flex_grow(flex_spacer, 1);
    lv_obj_set_style_bg_opa(flex_spacer, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(flex_spacer, 0, LV_PART_MAIN);

    /* Button row. */
    lv_obj_t *btn_row = lv_obj_create(content);
    lv_obj_set_size(btn_row, lv_pct(100), 56);
    lv_obj_set_style_bg_opa      (btn_row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all     (btn_row, 0, LV_PART_MAIN);
    lv_obj_set_layout            (btn_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow         (btn_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align        (btn_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag            (btn_row, LV_OBJ_FLAG_SCROLLABLE);

    /* Cancel button. */
    lv_obj_t *cancel = lv_btn_create(btn_row);
    lv_obj_set_size(cancel, 160, 48);
    lv_obj_set_style_bg_color    (cancel, UI_COLOR(ACTION_SECONDARY), LV_PART_MAIN);
    lv_obj_set_style_radius      (cancel, 8, LV_PART_MAIN);
    lv_obj_set_style_border_width(cancel, 0, LV_PART_MAIN);
    lv_obj_t *cancel_lbl = lv_label_create(cancel);
    lv_label_set_text(cancel_lbl, st->params.cancel_label);
    lv_obj_set_style_text_color(cancel_lbl, UI_COLOR(TEXT_BODY_STRONG), LV_PART_MAIN);
    lv_obj_set_style_text_font (cancel_lbl, &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_center(cancel_lbl);
    lv_obj_add_event_cb(cancel, on_cancel_clicked, LV_EVENT_CLICKED, st);

    /* Confirm button (with hold-fill overlay). */
    st->btn_confirm = lv_btn_create(btn_row);
    lv_obj_set_size(st->btn_confirm, 280, 48);
    lv_obj_set_style_bg_color    (st->btn_confirm, severity_button_color(st->params.severity), LV_PART_MAIN);
    lv_obj_set_style_radius      (st->btn_confirm, 8, LV_PART_MAIN);
    lv_obj_set_style_border_width(st->btn_confirm, 0, LV_PART_MAIN);
    lv_obj_set_style_clip_corner (st->btn_confirm, true, LV_PART_MAIN);
    lv_obj_clear_flag            (st->btn_confirm, LV_OBJ_FLAG_SCROLLABLE);

    /* Fill overlay (sits behind label, grows left-to-right). */
    st->btn_confirm_fill = lv_obj_create(st->btn_confirm);
    lv_obj_set_size(st->btn_confirm_fill, 0, 48);
    lv_obj_align(st->btn_confirm_fill, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_bg_color(st->btn_confirm_fill, UI_COLOR(TEXT_BODY_STRONG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa  (st->btn_confirm_fill, LV_OPA_30, LV_PART_MAIN);
    lv_obj_set_style_border_width(st->btn_confirm_fill, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all     (st->btn_confirm_fill, 0, LV_PART_MAIN);
    lv_obj_add_flag(st->btn_confirm_fill, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *confirm_lbl = lv_label_create(st->btn_confirm);
    lv_label_set_text(confirm_lbl, st->params.confirm_label);
    lv_obj_set_style_text_color(confirm_lbl, UI_COLOR(TEXT_BODY_STRONG), LV_PART_MAIN);
    lv_obj_set_style_text_font (confirm_lbl, &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_center(confirm_lbl);
    lv_obj_move_foreground(confirm_lbl);   /* keep above fill */

    lv_obj_add_event_cb(st->btn_confirm, on_confirm_pressed,  LV_EVENT_PRESSED,        st);
    lv_obj_add_event_cb(st->btn_confirm, on_confirm_released, LV_EVENT_RELEASED,       st);
    lv_obj_add_event_cb(st->btn_confirm, on_confirm_released, LV_EVENT_PRESS_LOST,     st);

    /* Outside-tap dismissal: attach a click handler on the scrim. */
    lv_obj_t *scrim = lv_obj_get_parent(content);
    if (scrim) {
        lv_obj_add_flag(scrim, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(scrim, on_scrim_clicked, LV_EVENT_CLICKED, content);
    }

    /* Open via the modal stack manager (handles precedence). */
    ui_modal_open(st->modal);

    lvgl_unlock();

    ESP_LOGI(TAG, "shown (severity=%d, hold_ms=%lu)",
             (int) st->params.severity,
             (unsigned long) (st->params.hold_ms == UI_CONFIRM_HOLD_DEFAULT
                              ? default_hold_ms(st->params.severity)
                              : st->params.hold_ms));
    return st->modal;
}
