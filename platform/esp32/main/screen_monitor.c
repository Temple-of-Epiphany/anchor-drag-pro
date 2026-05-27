/*
 * Monitor screen — implementation (milestone 1 of #70).
 *
 * Spec: https://github.com/Temple-of-Epiphany/anchor-drag-pro/wiki/Screen-Monitor
 *
 * Builds the persistent chrome (header + footer + banner) around a
 * two-column content area. Left column: plot canvas placeholder (real
 * anchor circle plot lands with the GPS source manager). Right
 * column: distance readout + primary action button (ARM / DISARM / MUTE
 * / CANCEL depending on state).
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#include "screen_monitor.h"
#include "ui_chrome.h"
#include "ui_tokens.h"
#include "ui_preset_picker.h"
#include "ui_confirm.h"
#include "lvgl_init.h"
#include "anchor_config.h"
#include "esp_log.h"
#include "lvgl.h"
#include <stdio.h>
#include <string.h>

/* Provided by main.c — the active config struct held for the lifetime
 * of the device. Monitor reads cfg.anchor.options.distances[] to
 * populate the Preset Picker modal. */
extern anchor_config_t g_config;

static const char *TAG = "monitor";

/* User-data struct attached to the screen lv_obj_t so set_*() functions
 * can find the widgets they update. */
typedef struct {
    lv_obj_t *header;
    lv_obj_t *footer;
    lv_obj_t *banner;
    lv_obj_t *plot_canvas;
    lv_obj_t *plot_placeholder_label;
    lv_obj_t *distance_label;
    lv_obj_t *distance_template_label;
    lv_obj_t *btn_action;
    lv_obj_t *btn_action_label;
    ui_state_pill_t current_state;
} monitor_state_t;

static monitor_state_t *get_state(lv_obj_t *m)
{
    return (monitor_state_t *) lv_obj_get_user_data(m);
}

/* ---- Action button styling per state (spec) ---- */

typedef struct {
    const char *label;
    lv_color_t  color;
    bool        enabled;
} action_btn_style_t;

static action_btn_style_t action_for_state(ui_state_pill_t s)
{
    switch (s) {
        case UI_STATE_PILL_OFF:
            return (action_btn_style_t){ "ARM",    UI_COLOR(ACTION_PRIMARY),     true };
        case UI_STATE_PILL_ON:
            return (action_btn_style_t){ "ARM",    UI_COLOR(ACTION_PRIMARY),     true };
        case UI_STATE_PILL_ARMING:
            return (action_btn_style_t){ "CANCEL", UI_COLOR(ACTION_SECONDARY),   true };
        case UI_STATE_PILL_ARMED:
            return (action_btn_style_t){ "DISARM", UI_COLOR(ACTION_DESTRUCTIVE), true };
        case UI_STATE_PILL_ALARM:
            return (action_btn_style_t){ "MUTE",   UI_COLOR(ACTION_WARNING),     true };
        case UI_STATE_PILL_MUTED:
            return (action_btn_style_t){ "DISARM", UI_COLOR(ACTION_DESTRUCTIVE), true };
        default:
            return (action_btn_style_t){ "ARM",    UI_COLOR(ACTION_PRIMARY),     true };
    }
}

/* ---- Button event handler (placeholder for state-machine wiring) ---- */

/* Confirm callback for DISARM-while-armed. */
static void on_disarm_confirmed(void *user_data)
{
    monitor_state_t *st = (monitor_state_t *) user_data;
    if (!st) return;
    ESP_LOGI(TAG, "DISARM confirmed → state OFF (state-machine wiring TODO)");
    /* Real implementation will transition the state machine. For now,
     * just reflect OFF in the UI so the user sees their action. */
    if (!lvgl_lock(500)) return;
    st->current_state = UI_STATE_PILL_OFF;
    ui_header_set_state_pill(st->header, UI_STATE_PILL_OFF);
    action_btn_style_t a = action_for_state(UI_STATE_PILL_OFF);
    lv_obj_set_style_bg_color(st->btn_action, a.color, LV_PART_MAIN);
    lv_label_set_text(st->btn_action_label, a.label);
    lv_label_set_text(st->distance_label, "Tap ARM when ready");
    lv_label_set_text(st->distance_template_label, "");
    lvgl_unlock();
}

static void on_action_btn_clicked(lv_event_t *e)
{
    monitor_state_t *st = (monitor_state_t *) lv_event_get_user_data(e);
    if (!st) return;
    ESP_LOGI(TAG, "action button clicked in state=%d", (int) st->current_state);

    switch (st->current_state) {
        case UI_STATE_PILL_OFF:
        case UI_STATE_PILL_ON: {
            /* ARM — transition to ARMING (state machine TODO; just reflect
             * the visual transition for now). */
            ESP_LOGI(TAG, "ARM tapped — transitioning to ARMING (visual only)");
            st->current_state = UI_STATE_PILL_ARMING;
            ui_header_set_state_pill(st->header, UI_STATE_PILL_ARMING);
            action_btn_style_t a = action_for_state(UI_STATE_PILL_ARMING);
            lv_obj_set_style_bg_color(st->btn_action, a.color, LV_PART_MAIN);
            lv_label_set_text(st->btn_action_label, a.label);
            lv_label_set_text(st->distance_label, "Collecting samples...");
            break;
        }
        case UI_STATE_PILL_ARMING: {
            /* CANCEL — back to OFF. */
            ESP_LOGI(TAG, "CANCEL tapped — back to OFF");
            st->current_state = UI_STATE_PILL_OFF;
            ui_header_set_state_pill(st->header, UI_STATE_PILL_OFF);
            action_btn_style_t a = action_for_state(UI_STATE_PILL_OFF);
            lv_obj_set_style_bg_color(st->btn_action, a.color, LV_PART_MAIN);
            lv_label_set_text(st->btn_action_label, a.label);
            lv_label_set_text(st->distance_label, "Tap ARM when ready");
            break;
        }
        case UI_STATE_PILL_ARMED:
        case UI_STATE_PILL_MUTED: {
            /* DISARM — hold to confirm (1 s, low-severity per spec). */
            ui_confirm_params_t params = {
                .title         = "DISARM anchor watch?",
                .body          = { "The alarm will no longer trigger if the boat drifts.", NULL },
                .final_warning = NULL,
                .cancel_label  = "Cancel",
                .confirm_label = "Hold to disarm",
                .hold_ms       = 1000,
                .severity      = UI_CONFIRM_HIGH,
                .on_confirm    = on_disarm_confirmed,
                .user_data     = st,
            };
            ui_confirm_show(&params);
            break;
        }
        case UI_STATE_PILL_ALARM: {
            /* MUTE — single tap, no confirm. State machine wiring TODO. */
            ESP_LOGI(TAG, "MUTE tapped — silencing (visual only)");
            st->current_state = UI_STATE_PILL_MUTED;
            ui_header_set_state_pill(st->header, UI_STATE_PILL_MUTED);
            action_btn_style_t a = action_for_state(UI_STATE_PILL_MUTED);
            lv_obj_set_style_bg_color(st->btn_action, a.color, LV_PART_MAIN);
            lv_label_set_text(st->btn_action_label, a.label);
            break;
        }
        default: break;
    }
}

/* Preset Picker callback — caller-side persistence. */
static void on_preset_selected(int new_idx, void *user_data)
{
    monitor_state_t *st = (monitor_state_t *) user_data;
    (void) st;
    ESP_LOGI(TAG, "preset %d selected — persistence wiring lands with #63", new_idx);
    /* When #63 (config writer) lands:
     *   g_config.anchor.selected_idx = new_idx;
     *   anchor_config_save(&g_config, NULL, 0);
     */
}

/* Plot canvas tap → open Preset Picker. */
static void on_plot_canvas_clicked(lv_event_t *e)
{
    monitor_state_t *st = (monitor_state_t *) lv_event_get_user_data(e);
    if (!st) return;
    bool locked = (st->current_state == UI_STATE_PILL_ARMING ||
                   st->current_state == UI_STATE_PILL_ARMED  ||
                   st->current_state == UI_STATE_PILL_ALARM  ||
                   st->current_state == UI_STATE_PILL_MUTED);
    ui_preset_picker_params_t params = {
        .distances   = g_config.anchor.options,
        .current_idx = g_config.anchor.selected_idx,
        .locked      = locked,
        .on_selected = on_preset_selected,
        .user_data   = st,
    };
    ESP_LOGI(TAG, "plot tap → preset picker (locked=%d)", (int) locked);
    ui_preset_picker_show(&params);
}

/* ---- Construction ---- */

lv_obj_t *screen_monitor_create(void)
{
    if (!lvgl_lock(2000)) {
        ESP_LOGE(TAG, "could not acquire LVGL mutex");
        return NULL;
    }

    monitor_state_t *st = lv_mem_alloc(sizeof(*st));
    if (!st) { lvgl_unlock(); return NULL; }
    memset(st, 0, sizeof(*st));
    st->current_state = UI_STATE_PILL_OFF;

    /* Root screen. */
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_size(scr, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color    (scr, UI_COLOR(BG_APP),  LV_PART_MAIN);
    lv_obj_set_style_bg_opa      (scr, LV_OPA_COVER,      LV_PART_MAIN);
    lv_obj_set_style_pad_all     (scr, 0,                  LV_PART_MAIN);
    lv_obj_set_style_border_width(scr, 0,                  LV_PART_MAIN);
    lv_obj_clear_flag            (scr, LV_OBJ_FLAG_SCROLLABLE);

    /* Persistent chrome. */
    st->header = ui_header_create(scr);
    st->footer = ui_footer_create(scr);
    st->banner = ui_banner_create(scr);

    ui_footer_set_sections(st->footer, "Info", "MONITOR", "Settings");

    /* Content area between header (top 64) and footer (bottom 64). */
    lv_obj_t *content = lv_obj_create(scr);
    lv_obj_set_size(content, LV_HOR_RES, LV_VER_RES - 128);
    lv_obj_align(content, LV_ALIGN_TOP_LEFT, 0, 64);
    lv_obj_set_style_bg_opa      (content, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(content, 0,             LV_PART_MAIN);
    lv_obj_set_style_pad_all     (content, 12,            LV_PART_MAIN);
    lv_obj_set_layout            (content, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow         (content, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column  (content, 12,            LV_PART_MAIN);
    lv_obj_clear_flag            (content, LV_OBJ_FLAG_SCROLLABLE);

    /* ---- Left column: plot canvas (placeholder for now) ---- */
    st->plot_canvas = lv_obj_create(content);
    lv_obj_set_size(st->plot_canvas, 476, LV_VER_RES - 152);
    lv_obj_set_style_bg_color    (st->plot_canvas, UI_COLOR(BG_CANVAS),     LV_PART_MAIN);
    lv_obj_set_style_bg_opa      (st->plot_canvas, LV_OPA_COVER,            LV_PART_MAIN);
    lv_obj_set_style_border_color(st->plot_canvas, UI_COLOR(SURFACE_BORDER),LV_PART_MAIN);
    lv_obj_set_style_border_width(st->plot_canvas, 1,                        LV_PART_MAIN);
    lv_obj_set_style_radius      (st->plot_canvas, 8,                        LV_PART_MAIN);
    lv_obj_set_style_pad_all     (st->plot_canvas, 0,                        LV_PART_MAIN);
    lv_obj_clear_flag            (st->plot_canvas, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag              (st->plot_canvas, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb          (st->plot_canvas, on_plot_canvas_clicked, LV_EVENT_CLICKED, st);

    st->plot_placeholder_label = lv_label_create(st->plot_canvas);
    lv_label_set_text(st->plot_placeholder_label, "NO GPS\nanchor watch unavailable");
    lv_obj_set_style_text_color(st->plot_placeholder_label, UI_COLOR(TEXT_DIM),          LV_PART_MAIN);
    lv_obj_set_style_text_font (st->plot_placeholder_label, &lv_font_montserrat_22,      LV_PART_MAIN);
    lv_obj_set_style_text_align(st->plot_placeholder_label, LV_TEXT_ALIGN_CENTER,        LV_PART_MAIN);
    lv_obj_center(st->plot_placeholder_label);
    /* The placeholder text covers part of the clickable canvas. Make
     * the label itself clickable AND register the same handler so
     * tapping over the text also opens the picker (the parent's
     * handler doesn't fire when the click hits a non-clickable child
     * because the click is consumed silently by the hit-test). */
    lv_obj_add_flag    (st->plot_placeholder_label, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(st->plot_placeholder_label, on_plot_canvas_clicked, LV_EVENT_CLICKED, st);

    /* ---- Right column: readouts + action button ---- */
    lv_obj_t *right = lv_obj_create(content);
    lv_obj_set_size(right, 296, LV_VER_RES - 152);
    lv_obj_set_style_bg_opa      (right, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(right, 0,             LV_PART_MAIN);
    lv_obj_set_style_pad_all     (right, 0,             LV_PART_MAIN);
    lv_obj_set_layout            (right, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow         (right, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align        (right, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row     (right, 8,             LV_PART_MAIN);
    lv_obj_clear_flag            (right, LV_OBJ_FLAG_SCROLLABLE);

    /* "Distance from anchor" caption */
    lv_obj_t *caption = lv_label_create(right);
    lv_label_set_text(caption, "Distance from anchor");
    lv_obj_set_style_text_color(caption, UI_COLOR(TEXT_LABEL),       LV_PART_MAIN);
    lv_obj_set_style_text_font (caption, &lv_font_montserrat_14,     LV_PART_MAIN);

    /* Main distance readout (big number) */
    st->distance_label = lv_label_create(right);
    lv_label_set_text(st->distance_label, "Tap ARM when ready");
    lv_obj_set_style_text_color(st->distance_label, UI_COLOR(TEXT_BODY_STRONG), LV_PART_MAIN);
    lv_obj_set_style_text_font (st->distance_label, &lv_font_montserrat_22,     LV_PART_MAIN);
    lv_obj_set_style_text_align(st->distance_label, LV_TEXT_ALIGN_CENTER,       LV_PART_MAIN);

    /* "Alarm at X ft" template */
    st->distance_template_label = lv_label_create(right);
    lv_label_set_text(st->distance_template_label, "");
    lv_obj_set_style_text_color(st->distance_template_label, UI_COLOR(TEXT_DIM),       LV_PART_MAIN);
    lv_obj_set_style_text_font (st->distance_template_label, &lv_font_montserrat_14,   LV_PART_MAIN);

    /* Spacer */
    lv_obj_t *spacer = lv_obj_create(right);
    lv_obj_set_size(spacer, 1, 30);
    lv_obj_set_style_bg_opa      (spacer, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(spacer, 0,             LV_PART_MAIN);

    /* Primary action button — full width, big */
    st->btn_action = lv_btn_create(right);
    lv_obj_set_size(st->btn_action, 280, 64);
    lv_obj_set_style_radius      (st->btn_action, 10, LV_PART_MAIN);
    lv_obj_set_style_border_width(st->btn_action, 0,  LV_PART_MAIN);
    lv_obj_add_event_cb(st->btn_action, on_action_btn_clicked, LV_EVENT_CLICKED, st);

    st->btn_action_label = lv_label_create(st->btn_action);
    lv_label_set_text(st->btn_action_label, "ARM");
    lv_obj_set_style_text_color(st->btn_action_label, UI_COLOR(TEXT_BODY_STRONG), LV_PART_MAIN);
    lv_obj_set_style_text_font (st->btn_action_label, &lv_font_montserrat_24,     LV_PART_MAIN);
    lv_obj_center(st->btn_action_label);

    lv_obj_set_user_data(scr, st);

    /* Apply initial state (OFF). */
    action_btn_style_t a = action_for_state(UI_STATE_PILL_OFF);
    lv_obj_set_style_bg_color(st->btn_action, a.color, LV_PART_MAIN);
    lv_label_set_text(st->btn_action_label, a.label);
    ui_header_set_state_pill(st->header, UI_STATE_PILL_OFF);

    lvgl_unlock();

    ESP_LOGI(TAG, "monitor screen created");
    return scr;
}

/* ---- Setters ---- */

void screen_monitor_set_state(lv_obj_t *monitor, ui_state_pill_t state)
{
    monitor_state_t *st = get_state(monitor);
    if (!st) return;
    if (!lvgl_lock(500)) return;

    st->current_state = state;
    ui_header_set_state_pill(st->header, state);

    action_btn_style_t a = action_for_state(state);
    lv_obj_set_style_bg_color(st->btn_action, a.color, LV_PART_MAIN);
    lv_label_set_text(st->btn_action_label, a.label);
    if (a.enabled) {
        lv_obj_clear_state(st->btn_action, LV_STATE_DISABLED);
    } else {
        lv_obj_add_state(st->btn_action, LV_STATE_DISABLED);
    }

    /* Update distance label content depending on state (OFF/ON show
     * the prompt; ARMING shows progress; ARMED+ show the live number). */
    switch (state) {
        case UI_STATE_PILL_OFF:
        case UI_STATE_PILL_ON:
            lv_label_set_text(st->distance_label, "Tap ARM when ready");
            lv_label_set_text(st->distance_template_label, "");
            break;
        case UI_STATE_PILL_ARMING:
            lv_label_set_text(st->distance_label, "Collecting samples...");
            break;
        case UI_STATE_PILL_ARMED:
        case UI_STATE_PILL_ALARM:
        case UI_STATE_PILL_MUTED:
            /* Distance value populated by screen_monitor_set_distance(). */
            break;
        default: break;
    }

    lvgl_unlock();
}

void screen_monitor_set_distance(lv_obj_t *monitor,
                                  double value, const char *unit,
                                  double alarm_at)
{
    monitor_state_t *st = get_state(monitor);
    if (!st) return;
    if (!lvgl_lock(500)) return;

    char val_buf[32], tmpl_buf[64];
    if (unit) {
        snprintf(val_buf,  sizeof(val_buf),  "%.0f %s", value, unit);
        snprintf(tmpl_buf, sizeof(tmpl_buf), "Alarm at %.0f %s", alarm_at, unit);
        lv_label_set_text(st->distance_label, val_buf);
        lv_label_set_text(st->distance_template_label, tmpl_buf);
    }

    lvgl_unlock();
}

void screen_monitor_set_boat_name(lv_obj_t *monitor, const char *name)
{
    monitor_state_t *st = get_state(monitor);
    if (!st) return;
    if (!lvgl_lock(500)) return;
    ui_header_set_boat_name(st->header, name);
    lvgl_unlock();
}

lv_obj_t *screen_monitor_header(lv_obj_t *monitor)
{
    monitor_state_t *st = get_state(monitor);
    return st ? st->header : NULL;
}

lv_obj_t *screen_monitor_footer(lv_obj_t *monitor)
{
    monitor_state_t *st = get_state(monitor);
    return st ? st->footer : NULL;
}

lv_obj_t *screen_monitor_banner(lv_obj_t *monitor)
{
    monitor_state_t *st = get_state(monitor);
    return st ? st->banner : NULL;
}
