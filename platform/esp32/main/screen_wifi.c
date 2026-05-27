/*
 * WiFi screen — implementation.
 *
 * Layout:
 *   ┌─ header ────────────────────────────────────────────┐
 *   │ WIFI                                  [Back]        │
 *   │ Current: <ssid> -<rssi> dBm  IP <x.x.x.x>           │
 *   ├─────────────────────────────────────────────────────┤
 *   │ Saved networks                                      │
 *   │   [• traveler          ]                            │
 *   │   [  Unladen Swallow 2 ]                            │
 *   │                                                     │
 *   │ Nearby networks            [↻ Scan]                 │
 *   │   [traveler          -39 dBm 🔒 ]                   │
 *   │   [Comcast_4F2       -67 dBm 🔒 ]                   │
 *   │   ...                                               │
 *   └─────────────────────────────────────────────────────┘
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#include "screen_wifi.h"
#include "screen_nav.h"
#include "ui_chrome.h"
#include "ui_tokens.h"
#include "ui_wifi_password.h"
#include "lvgl_init.h"
#include "wifi_manager.h"
#include "anchor_config.h"
#include "esp_log.h"
#include "lvgl.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "scr_wifi";

extern anchor_config_t g_config;

typedef struct {
    lv_obj_t *header;
    lv_obj_t *footer;
    lv_obj_t *status_lbl;       /* "Current: <ssid> RSSI IP" */
    lv_obj_t *saved_list;       /* card containing saved network rows */
    lv_obj_t *nearby_list;      /* card containing scan rows */
    lv_obj_t *scan_btn_lbl;     /* button label, toggled "Scan" ↔ "Scanning..." */
    lv_timer_t *refresh_timer;  /* periodic status + scan-poll redraw */
} wifi_screen_state_t;

static wifi_screen_state_t *get_state(lv_obj_t *s) {
    return (wifi_screen_state_t *) lv_obj_get_user_data(s);
}

/* ---- Append helpers ---- */

static void clear_children(lv_obj_t *parent)
{
    if (!parent) return;
    lv_obj_clean(parent);
}

static void on_saved_row_clicked(lv_event_t *e); /* fwd */
static void on_nearby_row_clicked(lv_event_t *e); /* fwd */
static void on_scan_btn(lv_event_t *e);          /* fwd */

/* Build a row inside `parent` for an SSID, with right-aligned detail. */
static lv_obj_t *add_row(lv_obj_t *parent, const char *ssid, const char *detail,
                          lv_event_cb_t cb, void *user_data)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, 36);
    lv_obj_set_style_bg_opa      (row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all     (row, 0, LV_PART_MAIN);
    lv_obj_set_layout            (row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow         (row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align        (row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag            (row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag              (row, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *name = lv_label_create(row);
    lv_label_set_text(name, ssid);
    lv_obj_set_style_text_color(name, UI_COLOR(TEXT_BODY),     LV_PART_MAIN);
    lv_obj_set_style_text_font (name, &lv_font_montserrat_16,  LV_PART_MAIN);
    lv_label_set_long_mode(name, LV_LABEL_LONG_DOT);

    lv_obj_t *info = lv_label_create(row);
    lv_label_set_text(info, detail ? detail : "");
    lv_obj_set_style_text_color(info, UI_COLOR(TEXT_BODY_DIM),  LV_PART_MAIN);
    lv_obj_set_style_text_font (info, &lv_font_montserrat_14,   LV_PART_MAIN);

    if (cb) lv_obj_add_event_cb(row, cb, LV_EVENT_CLICKED, user_data);
    /* Stash the SSID directly on the row so click handlers can read it. */
    lv_obj_set_user_data(row, (void *) ssid);
    return row;
}

/* Card builder for the two sections. */
static lv_obj_t *build_card(lv_obj_t *parent)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_width(card, lv_pct(100));
    lv_obj_set_height(card, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color    (card, UI_COLOR(SURFACE_ELEVATED), LV_PART_MAIN);
    lv_obj_set_style_bg_opa      (card, LV_OPA_COVER,               LV_PART_MAIN);
    lv_obj_set_style_border_color(card, UI_COLOR(SURFACE_BORDER),   LV_PART_MAIN);
    lv_obj_set_style_border_width(card, 1, LV_PART_MAIN);
    lv_obj_set_style_radius      (card, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_all     (card, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_row     (card, 2, LV_PART_MAIN);
    lv_obj_set_layout            (card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow         (card, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag            (card, LV_OBJ_FLAG_SCROLLABLE);
    return card;
}

/* ---- Live refresh ---- */

static void render_status(wifi_screen_state_t *st)
{
    wifi_mgr_status_t s;
    wifi_manager_get_status(&s);
    char buf[96];
    const char *state_str = "?";
    switch (s.state) {
        case WIFI_MGR_IDLE:         state_str = "idle"; break;
        case WIFI_MGR_SCANNING:     state_str = "scanning"; break;
        case WIFI_MGR_CONNECTING:   state_str = "connecting"; break;
        case WIFI_MGR_CONNECTED:    state_str = "connected"; break;
        case WIFI_MGR_DISCONNECTED: state_str = "disconnected"; break;
        case WIFI_MGR_NO_NETWORKS:  state_str = "no saved networks visible"; break;
        case WIFI_MGR_DISABLED:     state_str = "disabled"; break;
    }
    if (s.state == WIFI_MGR_CONNECTED) {
        snprintf(buf, sizeof(buf), "Connected to \"%s\"  %d dBm  IP %s",
                 s.ssid, s.rssi, s.ip);
    } else if (s.ssid[0]) {
        snprintf(buf, sizeof(buf), "%s — last: \"%s\"", state_str, s.ssid);
    } else {
        snprintf(buf, sizeof(buf), "%s", state_str);
    }
    lv_label_set_text(st->status_lbl, buf);
}

static void render_saved(wifi_screen_state_t *st)
{
    clear_children(st->saved_list);
    if (g_config.wifi.sta_network_count == 0) {
        lv_obj_t *l = lv_label_create(st->saved_list);
        lv_label_set_text(l, "(none yet — tap a nearby network below)");
        lv_obj_set_style_text_color(l, UI_COLOR(TEXT_DIM),       LV_PART_MAIN);
        lv_obj_set_style_text_font (l, &lv_font_montserrat_14,   LV_PART_MAIN);
        return;
    }
    for (int i = 0; i < g_config.wifi.sta_network_count; i++) {
        char detail[24];
        snprintf(detail, sizeof(detail), "priority %d", i);
        add_row(st->saved_list, g_config.wifi.sta_networks[i].ssid,
                detail, on_saved_row_clicked, NULL);
    }
}

static void render_nearby(wifi_screen_state_t *st)
{
    clear_children(st->nearby_list);
    wifi_mgr_ap_t aps[20];
    size_t n = wifi_manager_get_scan_results(aps, 20);
    if (n == 0) {
        lv_obj_t *l = lv_label_create(st->nearby_list);
        lv_label_set_text(l, "(no scan results yet — tap Scan)");
        lv_obj_set_style_text_color(l, UI_COLOR(TEXT_DIM),       LV_PART_MAIN);
        lv_obj_set_style_text_font (l, &lv_font_montserrat_14,   LV_PART_MAIN);
        return;
    }
    for (size_t i = 0; i < n; i++) {
        char detail[40];
        snprintf(detail, sizeof(detail), "%d dBm %s ch %d",
                 aps[i].rssi, aps[i].secured ? "🔒" : "  ", aps[i].channel);
        /* Stash secured-bit + ssid via passing the index — but we need it
         * per-row. Use lv_obj_set_user_data on the row to store a small
         * heap copy of (ssid + secured). We'll free it on screen destroy. */
        wifi_mgr_ap_t *copy = lv_mem_alloc(sizeof(wifi_mgr_ap_t));
        if (!copy) continue;
        *copy = aps[i];
        lv_obj_t *row = add_row(st->nearby_list, copy->ssid, detail,
                                 on_nearby_row_clicked, NULL);
        lv_obj_set_user_data(row, copy);
    }
}

static void refresh_timer_cb(lv_timer_t *t)
{
    lv_obj_t *scr = (lv_obj_t *) t->user_data;
    wifi_screen_state_t *st = get_state(scr);
    if (!st) return;
    render_status(st);
    /* If a scan finished since last tick, redraw the nearby list. */
    static bool last_busy = false;
    bool busy = wifi_manager_scan_in_progress();
    if (last_busy && !busy) {
        render_nearby(st);
        lv_label_set_text(st->scan_btn_lbl, "↻ Scan");
    }
    last_busy = busy;
}

/* ---- Event handlers ---- */

static void on_password_entered(const char *ssid, const char *password, void *user_data)
{
    (void) user_data;
    ESP_LOGI(TAG, "trying \"%s\" with password (%d chars)", ssid, (int) strlen(password));

    /* Append/replace in g_config so it survives reboots and is picked
     * next time wifi_manager_start runs. Replace-by-SSID semantics. */
    int existing = -1;
    for (int i = 0; i < g_config.wifi.sta_network_count; i++) {
        if (strcmp(g_config.wifi.sta_networks[i].ssid, ssid) == 0) {
            existing = i; break;
        }
    }
    if (existing >= 0) {
        snprintf(g_config.wifi.sta_networks[existing].password,
                 sizeof(g_config.wifi.sta_networks[existing].password),
                 "%s", password);
    } else if (g_config.wifi.sta_network_count < ANCHOR_WIFI_MAX_NETWORKS) {
        int slot = g_config.wifi.sta_network_count++;
        snprintf(g_config.wifi.sta_networks[slot].ssid,
                 sizeof(g_config.wifi.sta_networks[slot].ssid),     "%s", ssid);
        snprintf(g_config.wifi.sta_networks[slot].password,
                 sizeof(g_config.wifi.sta_networks[slot].password), "%s", password);
    } else {
        ESP_LOGW(TAG, "saved-network slots full; rotating out oldest");
        /* Drop slot 0, shift left, append new at the end. */
        for (int i = 1; i < ANCHOR_WIFI_MAX_NETWORKS; i++) {
            g_config.wifi.sta_networks[i - 1] = g_config.wifi.sta_networks[i];
        }
        int last = ANCHOR_WIFI_MAX_NETWORKS - 1;
        snprintf(g_config.wifi.sta_networks[last].ssid,
                 sizeof(g_config.wifi.sta_networks[last].ssid),     "%s", ssid);
        snprintf(g_config.wifi.sta_networks[last].password,
                 sizeof(g_config.wifi.sta_networks[last].password), "%s", password);
    }

    /* Persist immediately + attempt connection. */
    anchor_config_save_nvs(&g_config);
    wifi_manager_try_connect(ssid, password);
}

static void on_nearby_row_clicked(lv_event_t *e)
{
    (void) e;
    lv_obj_t *row = lv_event_get_current_target(e);
    wifi_mgr_ap_t *ap = (wifi_mgr_ap_t *) lv_obj_get_user_data(row);
    if (!ap) return;
    ESP_LOGI(TAG, "nearby tapped: \"%s\" (secured=%d)", ap->ssid, ap->secured);
    ui_wifi_password_params_t p = {
        .ssid       = ap->ssid,
        .secured    = ap->secured,
        .on_connect = on_password_entered,
    };
    ui_wifi_password_show(&p);
}

static void on_saved_row_clicked(lv_event_t *e)
{
    lv_obj_t *row = lv_event_get_current_target(e);
    const char *ssid = (const char *) lv_obj_get_user_data(row);
    if (!ssid) return;
    ESP_LOGI(TAG, "saved tapped: \"%s\" — forcing reconnect", ssid);
    /* Quick path: just trigger reconnect using whatever's in g_config. */
    wifi_manager_reconnect();
}

static void on_scan_btn(lv_event_t *e)
{
    wifi_screen_state_t *st = (wifi_screen_state_t *) lv_event_get_user_data(e);
    if (!st) return;
    if (wifi_manager_request_scan() == ESP_OK) {
        lv_label_set_text(st->scan_btn_lbl, "Scanning...");
    }
}

/* Back-gesture handler — any swipe returns to Settings (index 3). */
void wifi_screen_back_gesture(lv_event_t *e)
{
    (void) e;
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    if (dir == LV_DIR_LEFT || dir == LV_DIR_RIGHT ||
        dir == LV_DIR_TOP  || dir == LV_DIR_BOTTOM) {
        screen_nav_switch_to(3);
    }
}

/* ---- Public ---- */

lv_obj_t *screen_wifi_create(void)
{
    if (!lvgl_lock(2000)) {
        ESP_LOGE(TAG, "could not acquire LVGL mutex");
        return NULL;
    }

    wifi_screen_state_t *st = lv_mem_alloc(sizeof(*st));
    if (!st) { lvgl_unlock(); return NULL; }
    memset(st, 0, sizeof(*st));

    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_size(scr, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color    (scr, UI_COLOR(BG_APP),  LV_PART_MAIN);
    lv_obj_set_style_bg_opa      (scr, LV_OPA_COVER,      LV_PART_MAIN);
    lv_obj_set_style_pad_all     (scr, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(scr, 0, LV_PART_MAIN);
    lv_obj_clear_flag            (scr, LV_OBJ_FLAG_SCROLLABLE);

    st->header = ui_header_create(scr);
    st->footer = ui_footer_create(scr);
    ui_footer_set_sections(st->footer, "← back", "WIFI", "← back");

    /* Swipe in any direction takes the user back to Settings (index 3
     * in the screen_nav carousel) — this screen sits outside the
     * top-level carousel. */
    extern void wifi_screen_back_gesture(lv_event_t *e);
    lv_obj_add_event_cb(scr, wifi_screen_back_gesture, LV_EVENT_GESTURE, NULL);

    /* Status block sits just under the header. */
    lv_obj_t *content = lv_obj_create(scr);
    lv_obj_set_size(content, LV_HOR_RES, LV_VER_RES - 128);
    lv_obj_align(content, LV_ALIGN_TOP_LEFT, 0, 64);
    lv_obj_set_style_bg_opa      (content, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(content, 0,             LV_PART_MAIN);
    lv_obj_set_style_pad_all     (content, 12,            LV_PART_MAIN);
    lv_obj_set_style_pad_row     (content, 10,            LV_PART_MAIN);
    lv_obj_set_layout            (content, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow         (content, LV_FLEX_FLOW_COLUMN);
    lv_obj_add_flag              (content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir        (content, LV_DIR_VER);

    st->status_lbl = lv_label_create(content);
    lv_label_set_text(st->status_lbl, "(checking...)");
    lv_obj_set_style_text_color(st->status_lbl, UI_COLOR(TEXT_BODY),    LV_PART_MAIN);
    lv_obj_set_style_text_font (st->status_lbl, &lv_font_montserrat_16, LV_PART_MAIN);

    /* Saved networks card */
    lv_obj_t *saved_hdr = lv_label_create(content);
    lv_label_set_text(saved_hdr, "Saved networks");
    lv_obj_set_style_text_color(saved_hdr, UI_COLOR(TEXT_LABEL),    LV_PART_MAIN);
    lv_obj_set_style_text_font (saved_hdr, &lv_font_montserrat_14,  LV_PART_MAIN);
    st->saved_list = build_card(content);

    /* Nearby row: header + scan button */
    lv_obj_t *near_row = lv_obj_create(content);
    lv_obj_set_size(near_row, lv_pct(100), 36);
    lv_obj_set_style_bg_opa      (near_row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(near_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all     (near_row, 0, LV_PART_MAIN);
    lv_obj_set_layout            (near_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow         (near_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align        (near_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag            (near_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *near_hdr = lv_label_create(near_row);
    lv_label_set_text(near_hdr, "Nearby networks");
    lv_obj_set_style_text_color(near_hdr, UI_COLOR(TEXT_LABEL),    LV_PART_MAIN);
    lv_obj_set_style_text_font (near_hdr, &lv_font_montserrat_14,  LV_PART_MAIN);

    lv_obj_t *scan_btn = lv_btn_create(near_row);
    lv_obj_set_size(scan_btn, 140, 34);
    lv_obj_set_style_bg_color    (scan_btn, UI_COLOR(ACTION_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_radius      (scan_btn, 6, LV_PART_MAIN);
    lv_obj_set_style_border_width(scan_btn, 0, LV_PART_MAIN);
    st->scan_btn_lbl = lv_label_create(scan_btn);
    lv_label_set_text(st->scan_btn_lbl, "↻ Scan");
    lv_obj_set_style_text_color(st->scan_btn_lbl, UI_COLOR(TEXT_BODY_STRONG), LV_PART_MAIN);
    lv_obj_set_style_text_font (st->scan_btn_lbl, &lv_font_montserrat_14,     LV_PART_MAIN);
    lv_obj_center(st->scan_btn_lbl);
    lv_obj_add_event_cb(scan_btn, on_scan_btn, LV_EVENT_CLICKED, st);

    st->nearby_list = build_card(content);

    lv_obj_set_user_data(scr, st);

    /* Initial render and a timer to refresh status while the screen is up. */
    render_status(st);
    render_saved(st);
    render_nearby(st);
    st->refresh_timer = lv_timer_create(refresh_timer_cb, 1500, scr);

    lvgl_unlock();
    ESP_LOGI(TAG, "wifi screen created");
    return scr;
}

void screen_wifi_refresh_status(lv_obj_t *screen)
{
    wifi_screen_state_t *st = get_state(screen);
    if (!st) return;
    if (!lvgl_lock(200)) return;
    render_status(st);
    lvgl_unlock();
}

void screen_wifi_trigger_scan(lv_obj_t *screen)
{
    wifi_screen_state_t *st = get_state(screen);
    if (!st) return;
    if (wifi_manager_request_scan() == ESP_OK) {
        if (lvgl_lock(200)) {
            lv_label_set_text(st->scan_btn_lbl, "Scanning...");
            lvgl_unlock();
        }
    }
}
