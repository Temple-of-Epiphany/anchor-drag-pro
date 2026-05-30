/*
 * Modal: WiFi password entry — implementation.
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#include "ui_wifi_password.h"
#include "ui_tokens.h"
#include "lvgl_init.h"
#include "esp_log.h"
#include "lvgl.h"
#include <stdlib.h>
#include <string.h>

static const char *TAG = "wifi_pw";

typedef struct {
    ui_modal_handle_t        *modal;
    ui_wifi_password_params_t params;
    char                      ssid_buf[33];

    lv_obj_t                 *pw_ta;
    lv_obj_t                 *kb;
    lv_obj_t                 *show_chk;
} pw_state_t;

static void close_and_free(pw_state_t *st)
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

static void on_cancel(lv_event_t *e)
{
    pw_state_t *st = (pw_state_t *) lv_event_get_user_data(e);
    ESP_LOGI(TAG, "cancelled");
    close_and_free(st);
}

static void on_connect(lv_event_t *e)
{
    pw_state_t *st = (pw_state_t *) lv_event_get_user_data(e);
    if (!st) return;
    const char *pw = lv_textarea_get_text(st->pw_ta);
    ESP_LOGI(TAG, "connect requested for \"%s\" (pw len %u)",
             st->ssid_buf, (unsigned) strlen(pw));
    ui_wifi_password_cb cb = st->params.on_connect;
    void *ud               = st->params.user_data;
    /* Copy strings out before close_and_free destroys backing storage. */
    char ssid[33], pwbuf[64];
    snprintf(ssid,  sizeof(ssid),  "%s", st->ssid_buf);
    snprintf(pwbuf, sizeof(pwbuf), "%s", pw ? pw : "");
    close_and_free(st);
    if (cb) cb(ssid, pwbuf, ud);
}

static void on_show_toggled(lv_event_t *e)
{
    pw_state_t *st = (pw_state_t *) lv_event_get_user_data(e);
    if (!st) return;
    bool checked = lv_obj_has_state(st->show_chk, LV_STATE_CHECKED);
    lv_textarea_set_password_mode(st->pw_ta, !checked);
}

static void on_kb_event(lv_event_t *e)
{
    /* Keyboard's default LV_EVENT_READY = "OK" key. Treat as Connect. */
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_READY) {
        pw_state_t *st = (pw_state_t *) lv_event_get_user_data(e);
        on_connect(e);
        (void) st;
    } else if (code == LV_EVENT_CANCEL) {
        on_cancel(e);
    }
}

ui_modal_handle_t *ui_wifi_password_show(const ui_wifi_password_params_t *p)
{
    if (!p || !p->ssid || !*p->ssid) return NULL;
    if (!lvgl_lock(2000)) {
        ESP_LOGE(TAG, "could not acquire LVGL mutex");
        return NULL;
    }

    pw_state_t *st = calloc(1, sizeof(*st));
    if (!st) { lvgl_unlock(); return NULL; }
    st->params = *p;
    snprintf(st->ssid_buf, sizeof(st->ssid_buf), "%s", p->ssid);

    /* Modal takes most of the screen so the on-screen keyboard fits. */
    st->modal = ui_modal_create(760, 440, UI_MODAL_PRIO_USER);
    if (!st->modal) { free(st); lvgl_unlock(); return NULL; }

    lv_obj_t *content = ui_modal_get_content(st->modal);
    lv_obj_set_layout(content, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(content, 8, LV_PART_MAIN);

    /* Title row: "Connect to {SSID}" */
    char title_buf[64];
    snprintf(title_buf, sizeof(title_buf), "Connect to \"%.30s\"", st->ssid_buf);
    lv_obj_t *title = lv_label_create(content);
    lv_label_set_text(title, title_buf);
    lv_obj_set_style_text_color(title, UI_COLOR(TEXT_BODY_STRONG), LV_PART_MAIN);
    lv_obj_set_style_text_font (title, &lv_font_montserrat_22,     LV_PART_MAIN);

    if (!p->secured) {
        lv_obj_t *note = lv_label_create(content);
        lv_label_set_text(note, "Open network — no password required");
        lv_obj_set_style_text_color(note, UI_COLOR(TEXT_BODY_DIM),  LV_PART_MAIN);
        lv_obj_set_style_text_font (note, &lv_font_montserrat_14,    LV_PART_MAIN);
    }

    /* Password text area */
    st->pw_ta = lv_textarea_create(content);
    lv_obj_set_width(st->pw_ta, lv_pct(100));
    lv_obj_set_height(st->pw_ta, 44);
    lv_textarea_set_one_line(st->pw_ta, true);
    lv_textarea_set_password_mode(st->pw_ta, true);
    lv_textarea_set_max_length(st->pw_ta, 63);
    lv_textarea_set_placeholder_text(st->pw_ta, p->secured ? "Password" : "(open)");
    lv_obj_set_style_text_color(st->pw_ta, UI_COLOR(TEXT_BODY_STRONG), LV_PART_MAIN);
    lv_obj_set_style_text_font (st->pw_ta, &lv_font_montserrat_18,     LV_PART_MAIN);

    /* Show password toggle */
    lv_obj_t *show_row = lv_obj_create(content);
    lv_obj_set_size(show_row, lv_pct(100), 32);
    lv_obj_set_style_bg_opa      (show_row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(show_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all     (show_row, 0, LV_PART_MAIN);
    lv_obj_set_layout            (show_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow         (show_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align        (show_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag            (show_row, LV_OBJ_FLAG_SCROLLABLE);

    st->show_chk = lv_checkbox_create(show_row);
    lv_checkbox_set_text(st->show_chk, "Show password");
    lv_obj_set_style_text_color(st->show_chk, UI_COLOR(TEXT_BODY),   LV_PART_MAIN);
    lv_obj_set_style_text_font (st->show_chk, &lv_font_montserrat_16, LV_PART_MAIN);
    /* Explicitly style the indicator box — LVGL's default is white-on-
     * the-current-surface, which is white-on-white against our modal
     * background and made the checkbox impossible to read. */
    lv_obj_set_style_bg_color    (st->show_chk, UI_COLOR(BG_APP),
                                   LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa      (st->show_chk, LV_OPA_COVER,
                                   LV_PART_INDICATOR);
    lv_obj_set_style_border_color(st->show_chk, UI_COLOR(TEXT_BODY),
                                   LV_PART_INDICATOR);
    lv_obj_set_style_border_width(st->show_chk, 2,
                                   LV_PART_INDICATOR);
    lv_obj_set_style_radius      (st->show_chk, 4,
                                   LV_PART_INDICATOR);
    /* Checked state: filled with accent so the tick stands out. */
    lv_obj_set_style_bg_color    (st->show_chk, UI_COLOR(ACTION_PRIMARY),
                                   LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_border_color(st->show_chk, UI_COLOR(ACTION_PRIMARY),
                                   LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_add_event_cb(st->show_chk, on_show_toggled, LV_EVENT_VALUE_CHANGED, st);

    /* Keyboard */
    st->kb = lv_keyboard_create(content);
    lv_obj_set_width(st->kb, lv_pct(100));
    lv_obj_set_height(st->kb, 220);
    lv_keyboard_set_textarea(st->kb, st->pw_ta);
    lv_obj_add_event_cb(st->kb, on_kb_event, LV_EVENT_READY,  st);
    lv_obj_add_event_cb(st->kb, on_kb_event, LV_EVENT_CANCEL, st);

    /* Bottom button row: Cancel | Connect */
    lv_obj_t *btn_row = lv_obj_create(content);
    lv_obj_set_size(btn_row, lv_pct(100), 50);
    lv_obj_set_style_bg_opa      (btn_row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all     (btn_row, 0, LV_PART_MAIN);
    lv_obj_set_layout            (btn_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow         (btn_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align        (btn_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag            (btn_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *cancel = lv_btn_create(btn_row);
    lv_obj_set_size(cancel, 140, 44);
    lv_obj_set_style_bg_color(cancel, UI_COLOR(ACTION_SECONDARY), LV_PART_MAIN);
    lv_obj_set_style_radius  (cancel, 8, LV_PART_MAIN);
    lv_obj_set_style_border_width(cancel, 0, LV_PART_MAIN);
    lv_obj_t *cl = lv_label_create(cancel);
    lv_label_set_text(cl, "Cancel");
    lv_obj_set_style_text_color(cl, UI_COLOR(TEXT_BODY_STRONG), LV_PART_MAIN);
    lv_obj_set_style_text_font (cl, &lv_font_montserrat_16,     LV_PART_MAIN);
    lv_obj_center(cl);
    lv_obj_add_event_cb(cancel, on_cancel, LV_EVENT_CLICKED, st);

    lv_obj_t *connect = lv_btn_create(btn_row);
    lv_obj_set_size(connect, 180, 44);
    lv_obj_set_style_bg_color(connect, UI_COLOR(ACTION_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_radius  (connect, 8, LV_PART_MAIN);
    lv_obj_set_style_border_width(connect, 0, LV_PART_MAIN);
    lv_obj_t *cn = lv_label_create(connect);
    lv_label_set_text(cn, "Connect");
    lv_obj_set_style_text_color(cn, UI_COLOR(TEXT_BODY_STRONG), LV_PART_MAIN);
    lv_obj_set_style_text_font (cn, &lv_font_montserrat_16,     LV_PART_MAIN);
    lv_obj_center(cn);
    lv_obj_add_event_cb(connect, on_connect, LV_EVENT_CLICKED, st);

    ui_modal_open(st->modal);
    lvgl_unlock();
    ESP_LOGI(TAG, "shown for \"%s\" (secured=%d)", st->ssid_buf, (int) p->secured);
    return st->modal;
}
