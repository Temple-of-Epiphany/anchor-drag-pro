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
#include "anchor_geo.h"
#include "gps_source.h"
#include "wifi_manager.h"
#include "anchor_state.h"
#include "ui_alarm.h"
#include "esp_log.h"
#include "lvgl.h"
#include <math.h>
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
    lv_obj_t *plot_placeholder_label; /* shown OFF/ON/ARMING + when no fix */
    lv_obj_t *plot_alarm_circle;      /* circular outline at alarm radius */
    lv_obj_t *plot_centroid;          /* anchor position dot */
    lv_obj_t *plot_boat;              /* current boat position dot */
    lv_obj_t *plot_scale_label;       /* bottom-left "± N ft" scale */
    lv_obj_t *distance_label;
    lv_obj_t *distance_template_label;
    lv_obj_t *btn_action;
    lv_obj_t *btn_action_label;
    ui_state_pill_t current_state;
    lv_timer_t *refresh;
} monitor_state_t;

/* Plot canvas geometry — kept in sync with the lv_obj_set_size call
 * in screen_monitor_create. Centre = (PLOT_W/2, PLOT_H/2). */
#define PLOT_W           476
#define PLOT_H           (480 - 152)  /* LV_VER_RES - 152 */
#define PLOT_CX          (PLOT_W / 2)
#define PLOT_CY          (PLOT_H / 2)
/* Margin so the alarm circle never touches the canvas edge. */
#define PLOT_USABLE_R    ((PLOT_H / 2) - 18)
#define PLOT_DOT_BOAT    14
#define PLOT_DOT_ANCHOR  16

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
    (void) user_data;
    ESP_LOGI(TAG, "DISARM confirmed");
    anchor_state_disarm();
    /* UI updates via the refresh timer reading anchor_state_get. */
}

static void on_action_btn_clicked(lv_event_t *e)
{
    monitor_state_t *st = (monitor_state_t *) lv_event_get_user_data(e);
    if (!st) return;
    ESP_LOGI(TAG, "action button clicked in state=%d", (int) st->current_state);

    /* Hand to the state machine — refresh timer picks up the new
     * state on the next tick (within 1 s). */
    switch (st->current_state) {
        case UI_STATE_PILL_OFF:
        case UI_STATE_PILL_ON:
            anchor_state_arm();
            break;
        case UI_STATE_PILL_ARMING:
            anchor_state_cancel();
            break;
        case UI_STATE_PILL_ARMED:
        case UI_STATE_PILL_MUTED: {
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
        case UI_STATE_PILL_ALARM:
            anchor_state_mute();
            ui_alarm_set_muted(true);
            break;
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

/* Plot canvas tap → open Preset Picker.
 *
 * Important: the plot canvas is the largest touch target on the screen
 * and the user often swipes across it to navigate. LVGL emits CLICKED
 * at touch-release even when a swipe is in progress (the gesture-cancel
 * doesn't always beat the click). Guard against that by checking the
 * active input device — if it just produced a horizontal gesture, the
 * tap was actually a swipe and we shouldn't open the picker. */
static void on_plot_canvas_clicked(lv_event_t *e)
{
    monitor_state_t *st = (monitor_state_t *) lv_event_get_user_data(e);
    if (!st) return;

    lv_indev_t *indev = lv_indev_get_act();
    if (indev) {
        lv_dir_t g = lv_indev_get_gesture_dir(indev);
        if (g == LV_DIR_LEFT || g == LV_DIR_RIGHT ||
            g == LV_DIR_TOP  || g == LV_DIR_BOTTOM) {
            ESP_LOGD(TAG, "plot tap ignored — swipe in progress (dir=%d)", g);
            return;
        }
    }
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

    /* Alarm circle outline — circular lv_obj with border but no fill.
     * Sized + positioned each refresh from anchor_state.alarm_distance_m. */
    st->plot_alarm_circle = lv_obj_create(st->plot_canvas);
    lv_obj_set_size(st->plot_alarm_circle, 100, 100);
    lv_obj_set_style_bg_opa      (st->plot_alarm_circle, LV_OPA_TRANSP,            LV_PART_MAIN);
    lv_obj_set_style_border_color(st->plot_alarm_circle, UI_COLOR(STATE_ARMED),    LV_PART_MAIN);
    lv_obj_set_style_border_width(st->plot_alarm_circle, 2,                        LV_PART_MAIN);
    lv_obj_set_style_radius      (st->plot_alarm_circle, LV_RADIUS_CIRCLE,         LV_PART_MAIN);
    lv_obj_set_style_pad_all     (st->plot_alarm_circle, 0,                        LV_PART_MAIN);
    lv_obj_clear_flag            (st->plot_alarm_circle, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag            (st->plot_alarm_circle, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag              (st->plot_alarm_circle, LV_OBJ_FLAG_HIDDEN);

    /* Centroid (anchor) marker. */
    st->plot_centroid = lv_obj_create(st->plot_canvas);
    lv_obj_set_size(st->plot_centroid, PLOT_DOT_ANCHOR, PLOT_DOT_ANCHOR);
    lv_obj_set_style_bg_color    (st->plot_centroid, UI_COLOR(ACTION_PRIMARY),     LV_PART_MAIN);
    lv_obj_set_style_bg_opa      (st->plot_centroid, LV_OPA_COVER,                 LV_PART_MAIN);
    lv_obj_set_style_border_color(st->plot_centroid, UI_COLOR(TEXT_BODY_STRONG),   LV_PART_MAIN);
    lv_obj_set_style_border_width(st->plot_centroid, 2,                            LV_PART_MAIN);
    lv_obj_set_style_radius      (st->plot_centroid, LV_RADIUS_CIRCLE,             LV_PART_MAIN);
    lv_obj_set_style_pad_all     (st->plot_centroid, 0,                            LV_PART_MAIN);
    lv_obj_clear_flag            (st->plot_centroid, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag            (st->plot_centroid, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag              (st->plot_centroid, LV_OBJ_FLAG_HIDDEN);

    /* Boat marker — colour updated per-state in the refresh callback. */
    st->plot_boat = lv_obj_create(st->plot_canvas);
    lv_obj_set_size(st->plot_boat, PLOT_DOT_BOAT, PLOT_DOT_BOAT);
    lv_obj_set_style_bg_color    (st->plot_boat, UI_COLOR(STATE_ARMED),            LV_PART_MAIN);
    lv_obj_set_style_bg_opa      (st->plot_boat, LV_OPA_COVER,                     LV_PART_MAIN);
    lv_obj_set_style_border_color(st->plot_boat, UI_COLOR(TEXT_BODY_STRONG),       LV_PART_MAIN);
    lv_obj_set_style_border_width(st->plot_boat, 2,                                LV_PART_MAIN);
    lv_obj_set_style_radius      (st->plot_boat, LV_RADIUS_CIRCLE,                 LV_PART_MAIN);
    lv_obj_set_style_pad_all     (st->plot_boat, 0,                                LV_PART_MAIN);
    lv_obj_clear_flag            (st->plot_boat, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag            (st->plot_boat, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag              (st->plot_boat, LV_OBJ_FLAG_HIDDEN);

    /* Scale legend — bottom-left of plot. Updated each refresh. */
    st->plot_scale_label = lv_label_create(st->plot_canvas);
    lv_label_set_text(st->plot_scale_label, "");
    lv_obj_set_style_text_color(st->plot_scale_label, UI_COLOR(TEXT_DIM),          LV_PART_MAIN);
    lv_obj_set_style_text_font (st->plot_scale_label, &lv_font_montserrat_14,      LV_PART_MAIN);
    lv_obj_align(st->plot_scale_label, LV_ALIGN_BOTTOM_LEFT, 8, -8);
    lv_obj_add_flag(st->plot_scale_label, LV_OBJ_FLAG_HIDDEN);

    /* Placeholder text — shown OFF/ON/ARMING and when there's no fix. */
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

    /* Periodic refresh — header icons + plot placeholder live status.
     * Defined below; declared here just before timer creation. */
    extern void screen_monitor_refresh_cb(lv_timer_t *t);
    st->refresh = lv_timer_create(screen_monitor_refresh_cb, 1000, scr);

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

/* Periodic poll of gps_source + wifi_manager + anchor_state. */
void screen_monitor_refresh_cb(lv_timer_t *t)
{
    lv_obj_t *scr = (lv_obj_t *) t->user_data;
    monitor_state_t *st = get_state(scr);
    if (!st) return;
    /* Only do work when Monitor is the active screen. The widgets still
     * exist when the user has swiped away (lv_scr_load doesn't free
     * the previous screen) but invoking layout/style writes on a
     * non-active screen with an open modal elsewhere has reproduced a
     * LoadProhibited in the GDMA TX ISR. Cheaper to just skip. */
    if (lv_scr_act() != scr) return;

    /* Header status icons. */
    wifi_mgr_status_t w;
    wifi_manager_get_status(&w);
    ui_header_set_icon_wifi(st->header,
        (w.state == WIFI_MGR_CONNECTED) ? UI_ICON_OK : UI_ICON_OFF);

    bool gps_fresh = gps_source_is_fresh(5000);
    ui_header_set_icon_gps(st->header, gps_fresh ? UI_ICON_OK : UI_ICON_OFF);

    /* Anchor state machine snapshot. */
    anchor_state_snapshot_t as;
    anchor_state_get(&as);
    ui_state_pill_t pill = anchor_state_to_pill(as.state);
    if (pill != st->current_state) {
        st->current_state = pill;
        ui_header_set_state_pill(st->header, pill);
        action_btn_style_t a = action_for_state(pill);
        lv_obj_set_style_bg_color(st->btn_action, a.color, LV_PART_MAIN);
        lv_label_set_text(st->btn_action_label, a.label);

        /* Fire / dismiss the Alarm modal on transitions. */
        if (pill == UI_STATE_PILL_ALARM) {
            ui_alarm_params_t ap = {
                .distance_value = as.current_distance_m * 3.28084, /* metres → feet */
                .distance_unit  = "ft",
                .threshold      = as.alarm_distance_m * 3.28084,
                .heading_deg    = -1,
                .rot_dps        = -2147483647,
            };
            ui_alarm_show(&ap);
        } else if (pill == UI_STATE_PILL_OFF || pill == UI_STATE_PILL_ON) {
            ui_alarm_dismiss();
        }
    }

    /* Distance readout — state-dependent. */
    char buf[80];
    switch (as.state) {
        case AS_OFF:
        case AS_ON:
            lv_label_set_text(st->distance_label, "Tap ARM when ready");
            lv_label_set_text(st->distance_template_label, "");
            break;
        case AS_ARMING:
            snprintf(buf, sizeof(buf),
                     "Collecting samples\n%d / %d  (%.0f%%)",
                     as.arming_samples, as.arming_target,
                     as.arming_progress * 100.0);
            lv_label_set_text(st->distance_label, buf);
            lv_label_set_text(st->distance_template_label, "");
            break;
        case AS_ARMED:
        case AS_ALARM:
        case AS_MUTED:
            snprintf(buf, sizeof(buf), "%.0f ft",
                     as.current_distance_m * 3.28084);
            lv_label_set_text(st->distance_label, buf);
            snprintf(buf, sizeof(buf), "Alarm at %.0f ft",
                     as.alarm_distance_m * 3.28084);
            lv_label_set_text(st->distance_template_label, buf);
            /* Feed live distance to the Alarm modal if it's open. */
            if (as.state == AS_ALARM || as.state == AS_MUTED) {
                ui_alarm_update(as.current_distance_m * 3.28084,
                                as.alarm_distance_m * 3.28084, -1, -2147483647);
            }
            break;
    }

    /* Plot canvas — three modes:
     *  ARMED/ALARM/MUTED + centroid + fix  : draw circle, centroid, boat
     *  ARMING / no-fix / OFF / ON          : hide plot, show status text
     */
    gps_fix_t f; gps_source_get(&f);
    bool show_plot = (as.state == AS_ARMED || as.state == AS_ALARM ||
                      as.state == AS_MUTED) &&
                     as.centroid_valid && f.pos_valid;

    if (show_plot) {
        /* Hide the text labels. */
        lv_obj_add_flag(st->plot_placeholder_label, LV_OBJ_FLAG_HIDDEN);
        /* Show plot widgets. */
        lv_obj_clear_flag(st->plot_alarm_circle, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(st->plot_centroid,     LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(st->plot_boat,         LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(st->plot_scale_label,  LV_OBJ_FLAG_HIDDEN);

        /* Compute boat offset from centroid in metres (E/N). */
        geo_point_t boat = { .lat = f.latitude, .lon = f.longitude };
        double dx_m = 0, dy_m = 0;
        anchor_geo_offset_m(as.centroid, boat, &dx_m, &dy_m);

        /* Scale: alarm circle takes up ~70% of the usable radius unless
         * the boat is further out (then expand to fit the boat with 15%
         * margin). Keeps the boat dot on-screen even mid-drag. */
        double boat_m = sqrt(dx_m * dx_m + dy_m * dy_m);
        double extent_m = (boat_m > as.alarm_distance_m)
                              ? boat_m * 1.15
                              : as.alarm_distance_m / 0.70;
        if (extent_m < 1.0) extent_m = 1.0;  /* avoid divide-by-zero */
        double px_per_m = (double) PLOT_USABLE_R / extent_m;

        /* Alarm circle. */
        int r_px = (int) (as.alarm_distance_m * px_per_m);
        if (r_px < 4) r_px = 4;
        lv_obj_set_size(st->plot_alarm_circle, r_px * 2, r_px * 2);
        lv_obj_set_pos (st->plot_alarm_circle,
                        PLOT_CX - r_px, PLOT_CY - r_px);
        lv_obj_set_style_border_color(st->plot_alarm_circle,
            (as.state == AS_ALARM) ? UI_COLOR(STATE_ALARM) : UI_COLOR(STATE_ARMED),
            LV_PART_MAIN);

        /* Centroid dot centred. */
        lv_obj_set_pos(st->plot_centroid,
                        PLOT_CX - PLOT_DOT_ANCHOR / 2,
                        PLOT_CY - PLOT_DOT_ANCHOR / 2);

        /* Boat dot — y inverted (screen y grows downward, north is up). */
        int boat_x = PLOT_CX + (int) (dx_m * px_per_m) - PLOT_DOT_BOAT / 2;
        int boat_y = PLOT_CY - (int) (dy_m * px_per_m) - PLOT_DOT_BOAT / 2;
        lv_obj_set_pos(st->plot_boat, boat_x, boat_y);
        lv_obj_set_style_bg_color(st->plot_boat,
            (as.state == AS_ALARM) ? UI_COLOR(STATE_ALARM) : UI_COLOR(STATE_ARMED),
            LV_PART_MAIN);

        /* Scale legend — half-width of the canvas in feet. */
        double half_width_m = (double) PLOT_CX / px_per_m;
        snprintf(buf, sizeof(buf), "scale ±%.0f ft", half_width_m * 3.28084);
        lv_label_set_text(st->plot_scale_label, buf);
    } else {
        /* Hide plot widgets. */
        lv_obj_add_flag(st->plot_alarm_circle, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(st->plot_centroid,     LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(st->plot_boat,         LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(st->plot_scale_label,  LV_OBJ_FLAG_HIDDEN);
        /* Show text status. */
        lv_obj_clear_flag(st->plot_placeholder_label, LV_OBJ_FLAG_HIDDEN);

        if (as.state == AS_ARMING) {
            snprintf(buf, sizeof(buf), "ARMING\nsamples %d / %d",
                     as.arming_samples, as.arming_target);
            lv_label_set_text(st->plot_placeholder_label, buf);
            lv_obj_set_style_text_color(st->plot_placeholder_label,
                                         UI_COLOR(STATE_ARMING), LV_PART_MAIN);
        } else if ((as.state == AS_ARMED || as.state == AS_ALARM ||
                    as.state == AS_MUTED) && !f.pos_valid) {
            lv_label_set_text(st->plot_placeholder_label, "(waiting for fix)");
            lv_obj_set_style_text_color(st->plot_placeholder_label,
                                         UI_COLOR(STATE_WARNING), LV_PART_MAIN);
        } else if (gps_fresh && f.pos_valid) {
            snprintf(buf, sizeof(buf),
                     "GPS LIVE\n%.5f, %.5f\n%d sats   %.1f kts",
                     f.latitude, f.longitude,
                     f.satellites,
                     f.sog_valid ? f.sog_kts : 0.0);
            lv_label_set_text(st->plot_placeholder_label, buf);
            lv_obj_set_style_text_color(st->plot_placeholder_label,
                                         UI_COLOR(STATE_ARMED), LV_PART_MAIN);
        } else {
            lv_label_set_text(st->plot_placeholder_label,
                              "NO GPS\nanchor watch unavailable");
            lv_obj_set_style_text_color(st->plot_placeholder_label,
                                         UI_COLOR(TEXT_DIM), LV_PART_MAIN);
        }
    }
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
