/*
 * Connections screen — implementation (#71 m1).
 *
 * Spec: https://github.com/Temple-of-Epiphany/anchor-drag-pro/wiki/Screen-Connections
 *
 * Three sectioned cards (Inputs / Network / Storage & time) with the
 * 10 canonical rows. Static defaults until source-health pub/sub
 * lands and the screen subscribes for live updates.
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#include "screen_connections.h"
#include "ui_chrome.h"
#include "ui_tokens.h"
#include "lvgl_init.h"
#include "wifi_manager.h"
#include "tcp_gateway.h"
#include "gps_source.h"
#include "esp_log.h"
#include "lvgl.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "conn";

typedef struct {
    lv_obj_t *glyph;
    lv_obj_t *label;
    lv_obj_t *detail;
} row_widgets_t;

typedef struct {
    lv_obj_t       *header;
    lv_obj_t       *footer;
    lv_obj_t       *banner;
    row_widgets_t   rows[CONN_ROW_COUNT];
    lv_timer_t     *refresh;
} conn_state_t;

static conn_state_t *get_state(lv_obj_t *s) {
    return (conn_state_t *) lv_obj_get_user_data(s);
}

static void refresh_cb(lv_timer_t *t);   /* defined below; referenced by create */

static const char *row_label(conn_row_id_t r)
{
    switch (r) {
        case CONN_ROW_N2K:        return "N2K bus";
        case CONN_ROW_GPS:        return "GPS source";
        case CONN_ROW_IMU:        return "IMU source";
        case CONN_ROW_SERIAL_GPS: return "Serial GPS";
        case CONN_ROW_SERIAL_IMU: return "Serial IMU";
        case CONN_ROW_URL:        return "URL gateway";
        case CONN_ROW_WIFI_STA:   return "WiFi STA";
        case CONN_ROW_WIFI_AP:    return "WiFi AP";
        case CONN_ROW_SD:         return "SD card";
        case CONN_ROW_RTC:        return "RTC";
        default:                  return "?";
    }
}

static const char *status_glyph(conn_row_status_t s)
{
    switch (s) {
        case CONN_STATUS_OK:       return LV_SYMBOL_OK;
        case CONN_STATUS_WARN:     return LV_SYMBOL_WARNING;
        case CONN_STATUS_FAIL:     return LV_SYMBOL_CLOSE;
        case CONN_STATUS_SCANNING: return LV_SYMBOL_REFRESH;
        case CONN_STATUS_DISABLED: return LV_SYMBOL_MINUS;
        default:                   return " ";
    }
}

static lv_color_t status_color(conn_row_status_t s)
{
    switch (s) {
        case CONN_STATUS_OK:       return UI_COLOR(STATE_ARMED);
        case CONN_STATUS_WARN:     return UI_COLOR(STATE_WARNING);
        case CONN_STATUS_FAIL:     return UI_COLOR(STATE_ALARM);
        case CONN_STATUS_SCANNING:
        case CONN_STATUS_DISABLED:
        default:                   return UI_COLOR(TEXT_DIM);
    }
}

/* Build one row inside the given section card. */
static void build_row(lv_obj_t *card, row_widgets_t *out, const char *label)
{
    lv_obj_t *row = lv_obj_create(card);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, 22);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row, 12, LV_PART_MAIN);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    out->glyph = lv_label_create(row);
    lv_obj_set_width(out->glyph, 24);
    lv_label_set_text(out->glyph, " ");
    lv_obj_set_style_text_color(out->glyph, UI_COLOR(TEXT_DIM),       LV_PART_MAIN);
    lv_obj_set_style_text_font (out->glyph, &lv_font_montserrat_18,   LV_PART_MAIN);

    out->label = lv_label_create(row);
    lv_obj_set_width(out->label, 160);
    lv_label_set_text(out->label, label);
    lv_obj_set_style_text_color(out->label, UI_COLOR(TEXT_BODY),      LV_PART_MAIN);
    lv_obj_set_style_text_font (out->label, &lv_font_montserrat_16,   LV_PART_MAIN);

    out->detail = lv_label_create(row);
    lv_obj_set_flex_grow(out->detail, 1);
    lv_label_set_text(out->detail, "");
    lv_obj_set_style_text_color(out->detail, UI_COLOR(TEXT_BODY_DIM), LV_PART_MAIN);
    lv_obj_set_style_text_font (out->detail, &lv_font_montserrat_14,  LV_PART_MAIN);
    lv_label_set_long_mode(out->detail, LV_LABEL_LONG_DOT);
}

/* Build a section card with a header label and a list of rows. */
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

    lv_obj_t *title_lbl = lv_label_create(wrap);
    lv_label_set_text(title_lbl, title);
    lv_obj_set_style_text_color(title_lbl, UI_COLOR(TEXT_LABEL),    LV_PART_MAIN);
    lv_obj_set_style_text_font (title_lbl, &lv_font_montserrat_14,  LV_PART_MAIN);

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

lv_obj_t *screen_connections_create(void)
{
    if (!lvgl_lock(2000)) {
        ESP_LOGE(TAG, "could not acquire LVGL mutex");
        return NULL;
    }

    conn_state_t *st = lv_mem_alloc(sizeof(*st));
    if (!st) { lvgl_unlock(); return NULL; }
    memset(st, 0, sizeof(*st));

    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_size(scr, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color    (scr, UI_COLOR(BG_APP),  LV_PART_MAIN);
    lv_obj_set_style_bg_opa      (scr, LV_OPA_COVER,      LV_PART_MAIN);
    lv_obj_set_style_pad_all     (scr, 0,                  LV_PART_MAIN);
    lv_obj_set_style_border_width(scr, 0,                  LV_PART_MAIN);
    lv_obj_clear_flag            (scr, LV_OBJ_FLAG_SCROLLABLE);

    st->header = ui_header_create(scr);
    st->footer = ui_footer_create(scr);
    st->banner = ui_banner_create(scr);
    ui_footer_set_sections(st->footer, "Diagnostics", "CONNECTIONS", "Info");

    /* Content area between header (64) and footer (64). Vertically
     * scrollable since the section list exceeds 352 px easily. */
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

    /* ---- Inputs ---- */
    lv_obj_t *inputs = build_section(content, "Inputs");
    build_row(inputs, &st->rows[CONN_ROW_N2K],        row_label(CONN_ROW_N2K));
    build_row(inputs, &st->rows[CONN_ROW_GPS],        row_label(CONN_ROW_GPS));
    build_row(inputs, &st->rows[CONN_ROW_IMU],        row_label(CONN_ROW_IMU));
    build_row(inputs, &st->rows[CONN_ROW_SERIAL_GPS], row_label(CONN_ROW_SERIAL_GPS));
    build_row(inputs, &st->rows[CONN_ROW_SERIAL_IMU], row_label(CONN_ROW_SERIAL_IMU));
    build_row(inputs, &st->rows[CONN_ROW_URL],        row_label(CONN_ROW_URL));

    /* ---- Network ---- */
    lv_obj_t *network = build_section(content, "Network");
    build_row(network, &st->rows[CONN_ROW_WIFI_STA], row_label(CONN_ROW_WIFI_STA));
    build_row(network, &st->rows[CONN_ROW_WIFI_AP],  row_label(CONN_ROW_WIFI_AP));

    /* ---- Storage & time ---- */
    lv_obj_t *storage = build_section(content, "Storage & time");
    build_row(storage, &st->rows[CONN_ROW_SD],  row_label(CONN_ROW_SD));
    build_row(storage, &st->rows[CONN_ROW_RTC], row_label(CONN_ROW_RTC));

    /* Apply sensible defaults — every row gets a status so the screen
     * isn't blank on first render. These mirror the current Splash
     * results until source managers replace them with live data. */
    static const struct {
        conn_row_id_t      id;
        conn_row_status_t  status;
        const char        *detail;
    } defaults[] = {
        { CONN_ROW_N2K,        CONN_STATUS_DISABLED, "TWAI driver not yet implemented" },
        { CONN_ROW_GPS,        CONN_STATUS_DISABLED, "no source configured" },
        { CONN_ROW_IMU,        CONN_STATUS_DISABLED, "no source configured" },
        { CONN_ROW_SERIAL_GPS, CONN_STATUS_DISABLED, "disabled" },
        { CONN_ROW_SERIAL_IMU, CONN_STATUS_DISABLED, "disabled" },
        { CONN_ROW_URL,        CONN_STATUS_DISABLED, "disabled" },
        { CONN_ROW_WIFI_STA,   CONN_STATUS_DISABLED, "WiFi not yet implemented" },
        { CONN_ROW_WIFI_AP,    CONN_STATUS_DISABLED, "WiFi not yet implemented" },
        { CONN_ROW_SD,         CONN_STATUS_WARN,     "not mounted at boot" },
        { CONN_ROW_RTC,        CONN_STATUS_OK,       "PCF85063A at 0x51" },
    };

    for (size_t i = 0; i < sizeof(defaults) / sizeof(defaults[0]); i++) {
        lv_label_set_text(st->rows[defaults[i].id].glyph, status_glyph(defaults[i].status));
        lv_obj_set_style_text_color(st->rows[defaults[i].id].glyph,
                                    status_color(defaults[i].status), LV_PART_MAIN);
        lv_label_set_text(st->rows[defaults[i].id].detail, defaults[i].detail);
    }

    lv_obj_set_user_data(scr, st);

    /* Periodic refresh — wifi + gateway + gps live status. */
    st->refresh = lv_timer_create(refresh_cb, 1000, scr);

    lvgl_unlock();

    ESP_LOGI(TAG, "connections screen created");
    return scr;
}

/* Periodic refresh — polls wifi_manager + tcp_gateway + gps_source and
 * updates the WiFi STA, URL gateway, and GPS source rows. */
static void refresh_cb(lv_timer_t *t)
{
    lv_obj_t *scr = (lv_obj_t *) t->user_data;
    conn_state_t *st = get_state(scr);
    if (!st) return;

    /* WiFi STA */
    wifi_mgr_status_t w;
    wifi_manager_get_status(&w);
    char buf[64];
    if (w.state == WIFI_MGR_CONNECTED) {
        snprintf(buf, sizeof(buf), "%s  %d dBm  %s", w.ssid, w.rssi, w.ip);
        screen_connections_set_row(scr, CONN_ROW_WIFI_STA, CONN_STATUS_OK, buf);
    } else if (w.state == WIFI_MGR_CONNECTING || w.state == WIFI_MGR_SCANNING) {
        screen_connections_set_row(scr, CONN_ROW_WIFI_STA, CONN_STATUS_SCANNING,
                                    w.state == WIFI_MGR_SCANNING ? "scanning..." : "connecting...");
    } else if (w.state == WIFI_MGR_DISABLED) {
        screen_connections_set_row(scr, CONN_ROW_WIFI_STA, CONN_STATUS_DISABLED, "disabled in config");
    } else {
        screen_connections_set_row(scr, CONN_ROW_WIFI_STA, CONN_STATUS_FAIL, "not connected");
    }

    /* URL gateway — host buffer is 64 chars; pre-truncate so the
     * "%s:%d ..." formatted detail fits in `buf` without
     * -Wformat-truncation tripping. */
    tcp_gateway_status_t g;
    tcp_gateway_get_status(&g);
    char hbuf[40];
    snprintf(hbuf, sizeof(hbuf), "%.*s", (int) sizeof(hbuf) - 1, g.host);
    if (g.connected) {
        snprintf(buf, sizeof(buf), "%s:%d  %lu sent", hbuf, g.port,
                 (unsigned long) g.sentences_in);
        screen_connections_set_row(scr, CONN_ROW_URL, CONN_STATUS_OK, buf);
    } else if (g.host[0]) {
        snprintf(buf, sizeof(buf), "%s:%d  retrying...", hbuf, g.port);
        screen_connections_set_row(scr, CONN_ROW_URL, CONN_STATUS_SCANNING, buf);
    }

    /* GPS source (URL ingest) */
    if (gps_source_is_fresh(5000)) {
        gps_fix_t f;
        gps_source_get(&f);
        if (f.pos_valid) {
            snprintf(buf, sizeof(buf), "URL  %.4f, %.4f  sats %d",
                     f.latitude, f.longitude, f.satellites);
        } else {
            snprintf(buf, sizeof(buf), "URL  (no fix yet)");
        }
        screen_connections_set_row(scr, CONN_ROW_GPS, CONN_STATUS_OK, buf);
    } else {
        screen_connections_set_row(scr, CONN_ROW_GPS, CONN_STATUS_DISABLED, "no source visible");
    }
}

void screen_connections_set_row(lv_obj_t *screen,
                                 conn_row_id_t row,
                                 conn_row_status_t status,
                                 const char *detail)
{
    if (row >= CONN_ROW_COUNT) return;
    conn_state_t *st = get_state(screen);
    if (!st || !st->rows[row].glyph) return;
    if (!lvgl_lock(500)) return;
    lv_label_set_text(st->rows[row].glyph, status_glyph(status));
    lv_obj_set_style_text_color(st->rows[row].glyph, status_color(status), LV_PART_MAIN);
    lv_label_set_text(st->rows[row].detail, detail ? detail : "");
    lvgl_unlock();
}

lv_obj_t *screen_connections_header(lv_obj_t *s) { conn_state_t *st = get_state(s); return st ? st->header : NULL; }
lv_obj_t *screen_connections_footer(lv_obj_t *s) { conn_state_t *st = get_state(s); return st ? st->footer : NULL; }
