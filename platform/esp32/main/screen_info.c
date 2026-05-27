/*
 * Info screen — implementation (#72 m1).
 *
 * Spec: https://github.com/Temple-of-Epiphany/anchor-drag-pro/wiki/Screen-Info
 *
 * Two-column layout: Position card + Heading & attitude card. Live
 * values come from GPS / IMU source managers when those workstreams
 * land — until then everything shows "––" per spec empty-state rules.
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#include "screen_info.h"
#include "ui_chrome.h"
#include "ui_tokens.h"
#include "lvgl_init.h"
#include "esp_log.h"
#include "lvgl.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "info";

typedef struct {
    lv_obj_t *header;
    lv_obj_t *footer;
    lv_obj_t *banner;
    /* Position card */
    lv_obj_t *lat_lbl;
    lv_obj_t *lon_lbl;
    lv_obj_t *fix_lbl;
    lv_obj_t *hdop_lbl;
    lv_obj_t *alt_lbl;
    lv_obj_t *sog_lbl;
    lv_obj_t *cog_lbl;
    lv_obj_t *pos_source_lbl;
    /* Heading & attitude card */
    lv_obj_t *hdg_lbl;
    lv_obj_t *heel_lbl;
    lv_obj_t *pitch_lbl;
    lv_obj_t *rot_lbl;
    lv_obj_t *hdg_source_lbl;
} info_state_t;

static info_state_t *get_state(lv_obj_t *s) {
    return (info_state_t *) lv_obj_get_user_data(s);
}

/* Helper: one row inside a card — label on the left, value on the right. */
static lv_obj_t *kv_row(lv_obj_t *parent, const char *key)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, 24);
    lv_obj_set_style_bg_opa      (row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all     (row, 0, LV_PART_MAIN);
    lv_obj_set_layout            (row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow         (row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align        (row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag            (row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *k = lv_label_create(row);
    lv_label_set_text(k, key);
    lv_obj_set_style_text_color(k, UI_COLOR(TEXT_LABEL),     LV_PART_MAIN);
    lv_obj_set_style_text_font (k, &lv_font_montserrat_14,   LV_PART_MAIN);

    lv_obj_t *v = lv_label_create(row);
    lv_label_set_text(v, "––");
    lv_obj_set_style_text_color(v, UI_COLOR(TEXT_BODY),      LV_PART_MAIN);
    lv_obj_set_style_text_font (v, &lv_font_montserrat_18,   LV_PART_MAIN);
    return v;   /* caller saves the value label so it can update */
}

/* Build one card with title + child rows (caller adds rows after). */
static lv_obj_t *build_card(lv_obj_t *parent, const char *title)
{
    lv_obj_t *wrap = lv_obj_create(parent);
    lv_obj_set_width(wrap, 380);
    lv_obj_set_height(wrap, LV_VER_RES - 152);
    lv_obj_set_style_bg_color    (wrap, UI_COLOR(SURFACE_ELEVATED), LV_PART_MAIN);
    lv_obj_set_style_bg_opa      (wrap, LV_OPA_COVER,               LV_PART_MAIN);
    lv_obj_set_style_border_color(wrap, UI_COLOR(SURFACE_BORDER),   LV_PART_MAIN);
    lv_obj_set_style_border_width(wrap, 1, LV_PART_MAIN);
    lv_obj_set_style_radius      (wrap, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_all     (wrap, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_row     (wrap, 4, LV_PART_MAIN);
    lv_obj_set_layout            (wrap, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow         (wrap, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag            (wrap, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *t = lv_label_create(wrap);
    lv_label_set_text(t, title);
    lv_obj_set_style_text_color(t, UI_COLOR(TEXT_LABEL),    LV_PART_MAIN);
    lv_obj_set_style_text_font (t, &lv_font_montserrat_14,  LV_PART_MAIN);
    return wrap;
}

lv_obj_t *screen_info_create(void)
{
    if (!lvgl_lock(2000)) {
        ESP_LOGE(TAG, "could not acquire LVGL mutex");
        return NULL;
    }

    info_state_t *st = lv_mem_alloc(sizeof(*st));
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
    ui_footer_set_sections(st->footer, "Connections", "INFO", "Monitor");

    /* Two-column content area. */
    lv_obj_t *content = lv_obj_create(scr);
    lv_obj_set_size(content, LV_HOR_RES, LV_VER_RES - 128);
    lv_obj_align(content, LV_ALIGN_TOP_LEFT, 0, 64);
    lv_obj_set_style_bg_opa      (content, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(content, 0,             LV_PART_MAIN);
    lv_obj_set_style_pad_all     (content, 12,            LV_PART_MAIN);
    lv_obj_set_style_pad_column  (content, 12,            LV_PART_MAIN);
    lv_obj_set_layout            (content, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow         (content, LV_FLEX_FLOW_ROW);
    lv_obj_clear_flag            (content, LV_OBJ_FLAG_SCROLLABLE);

    /* ---- Position card ---- */
    lv_obj_t *pos = build_card(content, "Position");

    st->lat_lbl  = lv_label_create(pos);
    lv_label_set_text(st->lat_lbl, "––° ––.–––' –");
    lv_obj_set_style_text_color(st->lat_lbl, UI_COLOR(TEXT_BODY_STRONG), LV_PART_MAIN);
    lv_obj_set_style_text_font (st->lat_lbl, &lv_font_montserrat_24,     LV_PART_MAIN);

    st->lon_lbl  = lv_label_create(pos);
    lv_label_set_text(st->lon_lbl, "––° ––.–––' –");
    lv_obj_set_style_text_color(st->lon_lbl, UI_COLOR(TEXT_BODY_STRONG), LV_PART_MAIN);
    lv_obj_set_style_text_font (st->lon_lbl, &lv_font_montserrat_24,     LV_PART_MAIN);

    /* Spacer */
    lv_obj_t *sp = lv_obj_create(pos);
    lv_obj_set_size(sp, 1, 10);
    lv_obj_set_style_bg_opa(sp, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(sp, 0, LV_PART_MAIN);

    st->fix_lbl  = kv_row(pos, "Fix");
    st->hdop_lbl = kv_row(pos, "HDOP");
    st->alt_lbl  = kv_row(pos, "Alt");
    st->sog_lbl  = kv_row(pos, "SOG");
    st->cog_lbl  = kv_row(pos, "COG");

    /* Source line */
    lv_obj_t *src_sp = lv_obj_create(pos);
    lv_obj_set_flex_grow(src_sp, 1);
    lv_obj_set_style_bg_opa(src_sp, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(src_sp, 0, LV_PART_MAIN);

    st->pos_source_lbl = lv_label_create(pos);
    lv_label_set_text(st->pos_source_lbl, "Source: none");
    lv_obj_set_style_text_color(st->pos_source_lbl, UI_COLOR(TEXT_DIM),     LV_PART_MAIN);
    lv_obj_set_style_text_font (st->pos_source_lbl, &lv_font_montserrat_14, LV_PART_MAIN);

    /* ---- Heading & attitude card ---- */
    lv_obj_t *hdg = build_card(content, "Heading & attitude");

    /* Centred big "NO HEADING" placeholder where the compass rose will live. */
    lv_obj_t *rose_slot = lv_obj_create(hdg);
    lv_obj_set_size(rose_slot, 200, 160);
    lv_obj_set_style_bg_opa      (rose_slot, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(rose_slot, 0,             LV_PART_MAIN);
    lv_obj_clear_flag            (rose_slot, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all     (rose_slot, 0,             LV_PART_MAIN);

    lv_obj_t *no_hdg = lv_label_create(rose_slot);
    lv_label_set_text(no_hdg, "NO HEADING");
    lv_obj_set_style_text_color(no_hdg, UI_COLOR(TEXT_DIM),       LV_PART_MAIN);
    lv_obj_set_style_text_font (no_hdg, &lv_font_montserrat_18,   LV_PART_MAIN);
    lv_obj_center(no_hdg);

    st->hdg_lbl   = kv_row(hdg, "HDG");
    st->heel_lbl  = kv_row(hdg, "HEEL");
    st->pitch_lbl = kv_row(hdg, "PITCH");
    st->rot_lbl   = kv_row(hdg, "ROT");

    lv_obj_t *hdg_sp = lv_obj_create(hdg);
    lv_obj_set_flex_grow(hdg_sp, 1);
    lv_obj_set_style_bg_opa(hdg_sp, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(hdg_sp, 0, LV_PART_MAIN);

    st->hdg_source_lbl = lv_label_create(hdg);
    lv_label_set_text(st->hdg_source_lbl, "Source: none");
    lv_obj_set_style_text_color(st->hdg_source_lbl, UI_COLOR(TEXT_DIM),     LV_PART_MAIN);
    lv_obj_set_style_text_font (st->hdg_source_lbl, &lv_font_montserrat_14, LV_PART_MAIN);

    lv_obj_set_user_data(scr, st);
    lvgl_unlock();

    ESP_LOGI(TAG, "info screen created");
    return scr;
}

lv_obj_t *screen_info_header(lv_obj_t *s) { info_state_t *st = get_state(s); return st ? st->header : NULL; }
lv_obj_t *screen_info_footer(lv_obj_t *s) { info_state_t *st = get_state(s); return st ? st->footer : NULL; }
