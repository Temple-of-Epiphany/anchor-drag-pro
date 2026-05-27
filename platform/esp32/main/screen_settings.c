/*
 * Settings screen — implementation (#73 m1).
 *
 * Spec: https://github.com/Temple-of-Epiphany/anchor-drag-pro/wiki/Screen-Settings
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#include "screen_settings.h"
#include "ui_chrome.h"
#include "ui_tokens.h"
#include "ui_preset_picker.h"
#include "lvgl_init.h"
#include "anchor_config.h"
#include "esp_log.h"
#include "lvgl.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "settings";

extern anchor_config_t g_config;

typedef struct {
    lv_obj_t *header;
    lv_obj_t *footer;
    lv_obj_t *banner;
    /* Live-updateable widgets */
    lv_obj_t *distance_value_lbl;
    lv_obj_t *brightness_value_lbl;
    lv_obj_t *units_value_lbl;
} settings_state_t;

static settings_state_t *get_state(lv_obj_t *s) {
    return (settings_state_t *) lv_obj_get_user_data(s);
}

/* ---- "Edit in web UI" toast ---- */

static void show_web_only_toast(lv_obj_t *screen, const char *field)
{
    /* Tiny lazy-shown toast at the bottom of the content area. For
     * v0.2 this is just a transient label; in a future commit it can
     * use lv_timer to auto-dismiss after a few seconds. */
    (void) screen;
    ESP_LOGI(TAG, "%s is web-only — edit at http://anchor.local "
                  "(or the AP IP when not joined to STA)", field);
}

/* ---- Row builders ---- */

/* A row with a label on the left and an arbitrary widget on the right. */
static lv_obj_t *row_create(lv_obj_t *card, const char *label_text,
                             bool web_only)
{
    lv_obj_t *row = lv_obj_create(card);
    lv_obj_set_width (row, lv_pct(100));
    lv_obj_set_height(row, 40);
    lv_obj_set_style_bg_opa      (row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all     (row, 0, LV_PART_MAIN);
    lv_obj_set_layout            (row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow         (row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align        (row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag            (row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *label = lv_label_create(row);
    lv_label_set_text(label, label_text);
    lv_obj_set_style_text_color(label, UI_COLOR(TEXT_BODY),     LV_PART_MAIN);
    lv_obj_set_style_text_font (label, &lv_font_montserrat_16,  LV_PART_MAIN);

    if (web_only) {
        /* A subtle marker so the user knows where to edit. */
        lv_obj_t *tag = lv_label_create(label);
        (void) tag; /* not used — comment kept for design intent */
    }
    return row;
}

/* Build a section card with header. */
static lv_obj_t *build_section(lv_obj_t *parent, const char *title)
{
    lv_obj_t *wrap = lv_obj_create(parent);
    lv_obj_set_width(wrap, lv_pct(100));
    lv_obj_set_height(wrap, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa      (wrap, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(wrap, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all     (wrap, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_row     (wrap, 4, LV_PART_MAIN);
    lv_obj_set_layout            (wrap, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow         (wrap, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag            (wrap, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *t = lv_label_create(wrap);
    lv_label_set_text(t, title);
    lv_obj_set_style_text_color(t, UI_COLOR(TEXT_LABEL),    LV_PART_MAIN);
    lv_obj_set_style_text_font (t, &lv_font_montserrat_14,  LV_PART_MAIN);

    lv_obj_t *card = lv_obj_create(wrap);
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

/* ---- Event handlers ---- */

static void on_preset_picked(int new_idx, void *user_data)
{
    settings_state_t *st = (settings_state_t *) user_data;
    ESP_LOGI(TAG, "preset selected: %d (persistence wiring lands with #63)", new_idx);
    /* Reflect the visual change immediately. Persistence comes with
     * #63 — at which point we also re-render after the write. */
    if (st && st->distance_value_lbl) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%.0f %s",
                 g_config.anchor.options[new_idx].value,
                 g_config.anchor.options[new_idx].unit == UNIT_M ? "m" : "ft");
        if (lvgl_lock(500)) {
            lv_label_set_text(st->distance_value_lbl, buf);
            lvgl_unlock();
        }
    }
}

static void on_distance_row_clicked(lv_event_t *e)
{
    settings_state_t *st = (settings_state_t *) lv_event_get_user_data(e);
    if (!st) return;
    ui_preset_picker_params_t p = {
        .distances   = g_config.anchor.options,
        .current_idx = g_config.anchor.selected_idx,
        .locked      = false,
        .on_selected = on_preset_picked,
        .user_data   = st,
    };
    ui_preset_picker_show(&p);
}

static void on_web_only_row_clicked(lv_event_t *e)
{
    const char *field = (const char *) lv_event_get_user_data(e);
    show_web_only_toast(NULL, field ? field : "Setting");
}

/* ---- Construction ---- */

lv_obj_t *screen_settings_create(void)
{
    if (!lvgl_lock(2000)) {
        ESP_LOGE(TAG, "could not acquire LVGL mutex");
        return NULL;
    }

    settings_state_t *st = lv_mem_alloc(sizeof(*st));
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
    st->banner = ui_banner_create(scr);
    ui_footer_set_sections(st->footer, "Monitor", "SETTINGS", "Diagnostics");

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

    /* ---- Anchor section ---- */
    lv_obj_t *anchor = build_section(content, "Anchor");

    /* Alarm distance — tap opens preset picker. */
    lv_obj_t *dist_row = row_create(anchor, "Alarm distance", false);
    lv_obj_add_flag    (dist_row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(dist_row, on_distance_row_clicked, LV_EVENT_CLICKED, st);
    char dbuf[16];
    snprintf(dbuf, sizeof(dbuf), "%.0f %s",
             g_config.anchor.options[g_config.anchor.selected_idx].value,
             g_config.anchor.options[g_config.anchor.selected_idx].unit == UNIT_M ? "m" : "ft");
    st->distance_value_lbl = lv_label_create(dist_row);
    lv_label_set_text(st->distance_value_lbl, dbuf);
    lv_obj_set_style_text_color(st->distance_value_lbl, UI_COLOR(TEXT_BODY_STRONG), LV_PART_MAIN);
    lv_obj_set_style_text_font (st->distance_value_lbl, &lv_font_montserrat_18,     LV_PART_MAIN);

    /* Arming seconds — web-only */
    lv_obj_t *arm_row = row_create(anchor, "Arming seconds", true);
    lv_obj_add_flag    (arm_row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(arm_row, on_web_only_row_clicked, LV_EVENT_CLICKED, "Arming seconds");
    char abuf[24];
    snprintf(abuf, sizeof(abuf), "%d  (web only)", g_config.anchor.arming_seconds);
    lv_obj_t *arm_v = lv_label_create(arm_row);
    lv_label_set_text(arm_v, abuf);
    lv_obj_set_style_text_color(arm_v, UI_COLOR(TEXT_BODY_DIM),   LV_PART_MAIN);
    lv_obj_set_style_text_font (arm_v, &lv_font_montserrat_16,    LV_PART_MAIN);

    /* Sound volume — placeholder (low/med/high segment lands later) */
    lv_obj_t *vol_row = row_create(anchor, "Sound volume", false);
    const char *vol_str = (g_config.anchor.sound_volume == SOUND_LOW) ? "low"
                          : (g_config.anchor.sound_volume == SOUND_HIGH) ? "high" : "medium";
    lv_obj_t *vol_v = lv_label_create(vol_row);
    lv_label_set_text(vol_v, vol_str);
    lv_obj_set_style_text_color(vol_v, UI_COLOR(TEXT_BODY_STRONG), LV_PART_MAIN);
    lv_obj_set_style_text_font (vol_v, &lv_font_montserrat_18,     LV_PART_MAIN);

    /* ---- Display section ---- */
    lv_obj_t *display_card = build_section(content, "Display");

    lv_obj_t *bri_row = row_create(display_card, "Brightness", false);
    char bbuf[12];
    snprintf(bbuf, sizeof(bbuf), "%d%%", g_config.display.brightness);
    st->brightness_value_lbl = lv_label_create(bri_row);
    lv_label_set_text(st->brightness_value_lbl, bbuf);
    lv_obj_set_style_text_color(st->brightness_value_lbl, UI_COLOR(TEXT_BODY_STRONG), LV_PART_MAIN);
    lv_obj_set_style_text_font (st->brightness_value_lbl, &lv_font_montserrat_18,     LV_PART_MAIN);

    lv_obj_t *rot_row = row_create(display_card, "Rotation", false);
    char rbuf[8];
    snprintf(rbuf, sizeof(rbuf), "%d°", g_config.display.rotation);
    lv_obj_t *rot_v = lv_label_create(rot_row);
    lv_label_set_text(rot_v, rbuf);
    lv_obj_set_style_text_color(rot_v, UI_COLOR(TEXT_BODY_STRONG), LV_PART_MAIN);
    lv_obj_set_style_text_font (rot_v, &lv_font_montserrat_18,     LV_PART_MAIN);

    /* ---- Device section ---- */
    lv_obj_t *device = build_section(content, "Device");

    lv_obj_t *unit_row = row_create(device, "Units", false);
    st->units_value_lbl = lv_label_create(unit_row);
    lv_label_set_text(st->units_value_lbl, g_config.device.metric ? "metres" : "feet");
    lv_obj_set_style_text_color(st->units_value_lbl, UI_COLOR(TEXT_BODY_STRONG), LV_PART_MAIN);
    lv_obj_set_style_text_font (st->units_value_lbl, &lv_font_montserrat_18,     LV_PART_MAIN);

    lv_obj_t *boat_row = row_create(device, "Boat name", true);
    lv_obj_add_flag    (boat_row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(boat_row, on_web_only_row_clicked, LV_EVENT_CLICKED, "Boat name");
    /* Boat name is up to 64 chars from anchor_config_t. Source buffer
     * must be large enough that "\"…\"  (web only)" fits without the
     * compiler's format-truncation warning escalating to an error. */
    char nbuf[96];
    snprintf(nbuf, sizeof(nbuf), "\"%.40s\"  (web only)",
             g_config.device.name[0] ? g_config.device.name : "(unset)");
    lv_obj_t *boat_v = lv_label_create(boat_row);
    lv_label_set_text(boat_v, nbuf);
    lv_obj_set_style_text_color(boat_v, UI_COLOR(TEXT_BODY_DIM),   LV_PART_MAIN);
    lv_obj_set_style_text_font (boat_v, &lv_font_montserrat_16,    LV_PART_MAIN);
    lv_label_set_long_mode     (boat_v, LV_LABEL_LONG_DOT);

    /* ---- Network section ---- */
    lv_obj_t *network = build_section(content, "Network");

    lv_obj_t *wifi_row = row_create(network, "WiFi mode", true);
    lv_obj_add_flag    (wifi_row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(wifi_row, on_web_only_row_clicked, LV_EVENT_CLICKED, "WiFi mode");
    const char *wmode = (g_config.wifi.mode == WIFI_OFF) ? "off"
                       : (g_config.wifi.mode == WIFI_AP) ? "AP" : "STA";
    char wbuf[64];
    snprintf(wbuf, sizeof(wbuf), "%s  (web only)", wmode);
    lv_obj_t *wifi_v = lv_label_create(wifi_row);
    lv_label_set_text(wifi_v, wbuf);
    lv_obj_set_style_text_color(wifi_v, UI_COLOR(TEXT_BODY_DIM),   LV_PART_MAIN);
    lv_obj_set_style_text_font (wifi_v, &lv_font_montserrat_16,    LV_PART_MAIN);

    lv_obj_t *nets_row = row_create(network, "STA networks", true);
    lv_obj_add_flag    (nets_row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(nets_row, on_web_only_row_clicked, LV_EVENT_CLICKED, "STA networks");
    char nnbuf[40];
    snprintf(nnbuf, sizeof(nnbuf), "%d configured  (web only)",
             g_config.wifi.sta_network_count);
    lv_obj_t *nets_v = lv_label_create(nets_row);
    lv_label_set_text(nets_v, nnbuf);
    lv_obj_set_style_text_color(nets_v, UI_COLOR(TEXT_BODY_DIM),   LV_PART_MAIN);
    lv_obj_set_style_text_font (nets_v, &lv_font_montserrat_16,    LV_PART_MAIN);

    lv_obj_set_user_data(scr, st);
    lvgl_unlock();

    ESP_LOGI(TAG, "settings screen created");
    return scr;
}

lv_obj_t *screen_settings_header(lv_obj_t *s) { settings_state_t *st = get_state(s); return st ? st->header : NULL; }
lv_obj_t *screen_settings_footer(lv_obj_t *s) { settings_state_t *st = get_state(s); return st ? st->footer : NULL; }
