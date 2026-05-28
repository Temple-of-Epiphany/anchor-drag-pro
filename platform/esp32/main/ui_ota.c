/*
 * Modal: OTA — implementation.
 *
 * Spec: https://github.com/Temple-of-Epiphany/anchor-drag-pro/wiki/Modal-OTA
 *
 * Single-instance modal. A persistent title sits above a "phase area"
 * container that is cleaned and rebuilt as the flow moves AVAILABLE →
 * PROGRESS → RESULT.
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#include "ui_ota.h"
#include "ui_tokens.h"
#include "lvgl_init.h"
#include "esp_log.h"
#include "lvgl.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static const char *TAG = "ui_ota";

typedef struct {
    ui_modal_handle_t *modal;
    ui_ota_params_t    params;
    char               notes_buf[192];

    lv_obj_t          *phase_area;   /* cleaned + rebuilt per phase */
    lv_obj_t          *progress_bar; /* valid only in PROGRESS phase */
    lv_obj_t          *progress_lbl; /* phase caption + pct */
} ota_ui_state_t;

static ota_ui_state_t *s_active = NULL;

/* ---- Cleanup ---- */

static void close_and_free(ota_ui_state_t *st)
{
    if (!st) return;
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

static void on_skip_clicked(lv_event_t *e)
{
    ota_ui_state_t *st = (ota_ui_state_t *) lv_event_get_user_data(e);
    if (!st) return;
    ESP_LOGI(TAG, "update skipped");
    ui_ota_action_cb cb = st->params.on_skip;
    void *ud            = st->params.user_data;
    if (s_active == st) s_active = NULL;
    close_and_free(st);
    if (cb) cb(ud);
}

static void on_install_clicked(lv_event_t *e)
{
    ota_ui_state_t *st = (ota_ui_state_t *) lv_event_get_user_data(e);
    if (!st) return;
    ESP_LOGW(TAG, "install confirmed");
    ui_ota_action_cb cb = st->params.on_install;
    void *ud            = st->params.user_data;
    /* The on_install handler is expected to call ui_ota_begin_progress()
     * and start the flash worker. We do NOT close the modal here. */
    if (cb) cb(ud);
}

static void on_dismiss_clicked(lv_event_t *e)
{
    ota_ui_state_t *st = (ota_ui_state_t *) lv_event_get_user_data(e);
    if (!st) return;
    if (s_active == st) s_active = NULL;
    close_and_free(st);
}

/* ---- Phase area builders (assume LVGL already locked) ---- */

static void clear_phase_area(ota_ui_state_t *st)
{
    if (st->phase_area) lv_obj_clean(st->phase_area);
    st->progress_bar = NULL;
    st->progress_lbl = NULL;
}

static lv_obj_t *make_button(lv_obj_t *parent, const char *text,
                              lv_color_t color, lv_event_cb_t cb,
                              ota_ui_state_t *st)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 220, 52);
    lv_obj_set_style_bg_color    (btn, color, LV_PART_MAIN);
    lv_obj_set_style_radius      (btn, 8, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, UI_COLOR(TEXT_BODY_STRONG), LV_PART_MAIN);
    lv_obj_set_style_text_font (lbl, &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_center(lbl);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, st);
    return btn;
}

static void build_available_phase(ota_ui_state_t *st)
{
    clear_phase_area(st);
    lv_obj_t *area = st->phase_area;

    char buf[96];
    snprintf(buf, sizeof(buf), "v%d.%d.%d   " LV_SYMBOL_RIGHT "   v%d.%d.%d",
             st->params.from_major, st->params.from_minor, st->params.from_patch,
             st->params.to_major, st->params.to_minor, st->params.to_patch);
    lv_obj_t *ver = lv_label_create(area);
    lv_label_set_text(ver, buf);
    lv_obj_set_style_text_color(ver, UI_COLOR(STATE_ON),          LV_PART_MAIN);
    lv_obj_set_style_text_font (ver, &lv_font_montserrat_24,      LV_PART_MAIN);

    if (st->params.notes && st->params.notes[0]) {
        lv_obj_t *notes = lv_label_create(area);
        lv_label_set_text(notes, st->params.notes);
        lv_obj_set_style_text_color(notes, UI_COLOR(TEXT_BODY),   LV_PART_MAIN);
        lv_obj_set_style_text_font (notes, &lv_font_montserrat_16, LV_PART_MAIN);
        lv_obj_set_width           (notes, lv_pct(100));
        lv_label_set_long_mode     (notes, LV_LABEL_LONG_WRAP);
    }

    /* Push buttons to the bottom. */
    lv_obj_t *spacer = lv_obj_create(area);
    lv_obj_set_flex_grow(spacer, 1);
    lv_obj_set_style_bg_opa(spacer, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(spacer, 0, LV_PART_MAIN);

    lv_obj_t *btn_row = lv_obj_create(area);
    lv_obj_set_size(btn_row, lv_pct(100), 60);
    lv_obj_set_style_bg_opa      (btn_row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all     (btn_row, 0, LV_PART_MAIN);
    lv_obj_set_layout            (btn_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow         (btn_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align        (btn_row, LV_FLEX_ALIGN_SPACE_BETWEEN,
                                            LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag            (btn_row, LV_OBJ_FLAG_SCROLLABLE);

    make_button(btn_row, "Skip",    UI_COLOR(ACTION_SECONDARY), on_skip_clicked,    st);
    make_button(btn_row, "Install", UI_COLOR(ACTION_PRIMARY),   on_install_clicked, st);
}

static void build_progress_phase(ota_ui_state_t *st)
{
    clear_phase_area(st);
    lv_obj_t *area = st->phase_area;

    st->progress_lbl = lv_label_create(area);
    lv_label_set_text(st->progress_lbl, "Starting...");
    lv_obj_set_style_text_color(st->progress_lbl, UI_COLOR(TEXT_BODY),       LV_PART_MAIN);
    lv_obj_set_style_text_font (st->progress_lbl, &lv_font_montserrat_18,    LV_PART_MAIN);

    lv_obj_t *spacer = lv_obj_create(area);
    lv_obj_set_flex_grow(spacer, 1);
    lv_obj_set_style_bg_opa(spacer, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(spacer, 0, LV_PART_MAIN);

    st->progress_bar = lv_bar_create(area);
    lv_obj_set_size(st->progress_bar, lv_pct(100), 26);
    lv_bar_set_range(st->progress_bar, 0, 100);
    lv_bar_set_value(st->progress_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color (st->progress_bar, UI_COLOR(SURFACE_BORDER),   LV_PART_MAIN);
    lv_obj_set_style_radius   (st->progress_bar, 6,                          LV_PART_MAIN);
    lv_obj_set_style_bg_color (st->progress_bar, UI_COLOR(ACTION_PRIMARY),   LV_PART_INDICATOR);
    lv_obj_set_style_radius   (st->progress_bar, 6,                          LV_PART_INDICATOR);

    lv_obj_t *warn = lv_label_create(area);
    lv_label_set_text(warn, "Do not power off the device.");
    lv_obj_set_style_text_color(warn, UI_COLOR(STATE_WARNING),  LV_PART_MAIN);
    lv_obj_set_style_text_font (warn, &lv_font_montserrat_14,   LV_PART_MAIN);
}

static void build_error_phase(ota_ui_state_t *st, const char *msg)
{
    clear_phase_area(st);
    lv_obj_t *area = st->phase_area;

    lv_obj_t *glyph = lv_label_create(area);
    lv_label_set_text(glyph, LV_SYMBOL_WARNING);
    lv_obj_set_style_text_color(glyph, UI_COLOR(STATE_ALARM),    LV_PART_MAIN);
    lv_obj_set_style_text_font (glyph, &lv_font_montserrat_32,   LV_PART_MAIN);

    lv_obj_t *m = lv_label_create(area);
    lv_label_set_text(m, msg ? msg : "Update failed");
    lv_obj_set_style_text_color(m, UI_COLOR(TEXT_BODY_STRONG),   LV_PART_MAIN);
    lv_obj_set_style_text_font (m, &lv_font_montserrat_18,       LV_PART_MAIN);
    lv_obj_set_width           (m, lv_pct(100));
    lv_label_set_long_mode     (m, LV_LABEL_LONG_WRAP);

    lv_obj_t *spacer = lv_obj_create(area);
    lv_obj_set_flex_grow(spacer, 1);
    lv_obj_set_style_bg_opa(spacer, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(spacer, 0, LV_PART_MAIN);

    lv_obj_t *row = lv_obj_create(area);
    lv_obj_set_size(row, lv_pct(100), 60);
    lv_obj_set_style_bg_opa      (row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all     (row, 0, LV_PART_MAIN);
    lv_obj_set_layout            (row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow         (row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align        (row, LV_FLEX_ALIGN_END,
                                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag            (row, LV_OBJ_FLAG_SCROLLABLE);
    make_button(row, "Dismiss", UI_COLOR(ACTION_SECONDARY), on_dismiss_clicked, st);
}

/* ---- Public API ---- */

ui_modal_handle_t *ui_ota_show(const ui_ota_params_t *params)
{
    if (!params) return NULL;
    if (s_active) return s_active->modal;     /* single instance */

    if (!lvgl_lock(2000)) {
        ESP_LOGE(TAG, "could not acquire LVGL mutex");
        return NULL;
    }

    ota_ui_state_t *st = calloc(1, sizeof(*st));
    if (!st) { lvgl_unlock(); return NULL; }
    st->params = *params;
    if (params->notes) {
        snprintf(st->notes_buf, sizeof(st->notes_buf), "%s", params->notes);
        st->params.notes = st->notes_buf;
    } else {
        st->params.notes = NULL;
    }

    st->modal = ui_modal_create(560, 360, UI_MODAL_PRIO_OTA);
    if (!st->modal) { free(st); lvgl_unlock(); return NULL; }

    lv_obj_t *content = ui_modal_get_content(st->modal);
    lv_obj_set_user_data(content, st);
    lv_obj_set_layout    (content, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow (content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(content, 10, LV_PART_MAIN);

    /* Persistent title. */
    lv_obj_t *title = lv_label_create(content);
    lv_label_set_text(title, LV_SYMBOL_DOWNLOAD "  Firmware Update");
    lv_obj_set_style_text_color(title, UI_COLOR(TEXT_BODY_STRONG), LV_PART_MAIN);
    lv_obj_set_style_text_font (title, &lv_font_montserrat_22,     LV_PART_MAIN);

    /* Phase area fills the rest. */
    st->phase_area = lv_obj_create(content);
    lv_obj_set_width             (st->phase_area, lv_pct(100));
    lv_obj_set_flex_grow         (st->phase_area, 1);
    lv_obj_set_style_bg_opa      (st->phase_area, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(st->phase_area, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all     (st->phase_area, 0, LV_PART_MAIN);
    lv_obj_set_layout            (st->phase_area, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow         (st->phase_area, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row     (st->phase_area, 8, LV_PART_MAIN);
    lv_obj_clear_flag            (st->phase_area, LV_OBJ_FLAG_SCROLLABLE);

    build_available_phase(st);

    ui_modal_open(st->modal);
    s_active = st;
    lvgl_unlock();
    ESP_LOGI(TAG, "shown (v%d.%d.%d -> v%d.%d.%d)",
             params->from_major, params->from_minor, params->from_patch,
             params->to_major, params->to_minor, params->to_patch);
    return st->modal;
}

void ui_ota_begin_progress(void)
{
    if (!s_active) return;
    if (!lvgl_lock(2000)) return;
    build_progress_phase(s_active);
    lvgl_unlock();
}

void ui_ota_set_progress(const char *phase, int pct)
{
    if (!s_active) return;
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    if (!lvgl_lock(500)) return;
    if (s_active->progress_bar) {
        lv_bar_set_value(s_active->progress_bar, pct, LV_ANIM_OFF);
    }
    if (s_active->progress_lbl) {
        char buf[64];
        snprintf(buf, sizeof(buf), "%s  %d%%", phase ? phase : "", pct);
        lv_label_set_text(s_active->progress_lbl, buf);
    }
    lvgl_unlock();
}

void ui_ota_show_error(const char *msg)
{
    if (!s_active) return;
    if (!lvgl_lock(2000)) return;
    build_error_phase(s_active, msg);
    lvgl_unlock();
}

void ui_ota_dismiss(void)
{
    if (!s_active) return;
    ota_ui_state_t *st = s_active;
    s_active = NULL;
    close_and_free(st);
}
