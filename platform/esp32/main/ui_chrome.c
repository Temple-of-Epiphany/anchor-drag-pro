/*
 * Persistent UI chrome — header, footer, warning banner. Skeleton.
 *
 * Spec: https://github.com/Temple-of-Epiphany/anchor-drag-pro/wiki/Information-Architecture
 *
 * Milestone 3 of #69. Visual scaffolding + setter wiring; subsequent
 * issues (#70 Monitor, #71 Connections, ...) instantiate these on
 * their screens.
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#include "ui_chrome.h"
#include "ui_tokens.h"
#include "lvgl.h"
#include <stdio.h>

/* ---- Helpers ---- */

static lv_color_t state_pill_color(ui_state_pill_t s)
{
    switch (s) {
        case UI_STATE_PILL_OFF:    return UI_COLOR(STATE_OFF);
        case UI_STATE_PILL_ON:     return UI_COLOR(STATE_ON);
        case UI_STATE_PILL_ARMING: return UI_COLOR(STATE_ARMING);
        case UI_STATE_PILL_ARMED:  return UI_COLOR(STATE_ARMED);
        case UI_STATE_PILL_ALARM:  return UI_COLOR(STATE_ALARM);
        case UI_STATE_PILL_MUTED:  return UI_COLOR(STATE_ALARM);
        default:                   return UI_COLOR(STATE_OFF);
    }
}

static const char *state_pill_text(ui_state_pill_t s)
{
    switch (s) {
        case UI_STATE_PILL_OFF:    return "OFF";
        case UI_STATE_PILL_ON:     return "ON";
        case UI_STATE_PILL_ARMING: return "ARMING";
        case UI_STATE_PILL_ARMED:  return "ARMED";
        case UI_STATE_PILL_ALARM:  return "ALARM";
        case UI_STATE_PILL_MUTED:  return "MUTED";
        default:                   return "?";
    }
}

static lv_color_t icon_color(ui_icon_state_t s)
{
    switch (s) {
        case UI_ICON_OK:    return UI_COLOR(STATE_ARMED);
        case UI_ICON_WARN:  return UI_COLOR(STATE_WARNING);
        case UI_ICON_ALARM: return UI_COLOR(STATE_ALARM);
        case UI_ICON_OFF:
        default:            return UI_COLOR(TEXT_DIM);
    }
}

/* ---- Header ----
 * Layout from Information-Architecture.md:
 *   [STATE pill] Boat Name                     14:23  SD WIFI GPS
 */

typedef struct {
    lv_obj_t *pill_bg;
    lv_obj_t *pill_text;
    lv_obj_t *boat;
    lv_obj_t *time;
    lv_obj_t *icon_sd;
    lv_obj_t *icon_wifi;
    lv_obj_t *icon_gps;
} header_state_t;

static header_state_t *header_state(lv_obj_t *h)
{
    return (header_state_t *) lv_obj_get_user_data(h);
}

lv_obj_t *ui_header_create(lv_obj_t *parent)
{
    lv_obj_t *h = lv_obj_create(parent);
    lv_obj_set_size(h, LV_HOR_RES, 64);
    lv_obj_align(h, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color    (h, UI_COLOR(SURFACE_ELEVATED), LV_PART_MAIN);
    lv_obj_set_style_bg_opa      (h, LV_OPA_COVER,               LV_PART_MAIN);
    lv_obj_set_style_border_color(h, UI_COLOR(SURFACE_BORDER),   LV_PART_MAIN);
    lv_obj_set_style_border_width(h, 0, LV_PART_MAIN);
    lv_obj_set_style_border_side (h, LV_BORDER_SIDE_BOTTOM,      LV_PART_MAIN);
    lv_obj_set_style_pad_hor     (h, 16,                          LV_PART_MAIN);
    lv_obj_set_style_pad_ver     (h, 0,                           LV_PART_MAIN);
    lv_obj_clear_flag            (h, LV_OBJ_FLAG_SCROLLABLE);

    header_state_t *st = lv_mem_alloc(sizeof(*st));
    if (!st) return h;
    /* zero-init */
    *st = (header_state_t){0};

    /* State pill — left edge. */
    st->pill_bg = lv_obj_create(h);
    lv_obj_set_size(st->pill_bg, 96, 36);
    lv_obj_align(st->pill_bg, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_radius      (st->pill_bg, 18, LV_PART_MAIN);
    lv_obj_set_style_bg_color    (st->pill_bg, state_pill_color(UI_STATE_PILL_OFF), LV_PART_MAIN);
    lv_obj_set_style_bg_opa      (st->pill_bg, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(st->pill_bg, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all     (st->pill_bg, 0, LV_PART_MAIN);
    lv_obj_clear_flag            (st->pill_bg, LV_OBJ_FLAG_SCROLLABLE);

    st->pill_text = lv_label_create(st->pill_bg);
    lv_label_set_text(st->pill_text, state_pill_text(UI_STATE_PILL_OFF));
    lv_obj_set_style_text_color(st->pill_text, UI_COLOR(TEXT_BODY_STRONG), LV_PART_MAIN);
    lv_obj_set_style_text_font (st->pill_text, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_center(st->pill_text);

    /* Boat name — to the right of the pill. */
    st->boat = lv_label_create(h);
    lv_label_set_text(st->boat, "");
    lv_obj_set_style_text_color(st->boat, UI_COLOR(TEXT_BODY), LV_PART_MAIN);
    lv_obj_set_style_text_font (st->boat, &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_align(st->boat, LV_ALIGN_LEFT_MID, 112, 0);

    /* Time — right side. */
    st->time = lv_label_create(h);
    lv_label_set_text(st->time, "--:--");
    lv_obj_set_style_text_color(st->time, UI_COLOR(TEXT_BODY), LV_PART_MAIN);
    lv_obj_set_style_text_font (st->time, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_align(st->time, LV_ALIGN_RIGHT_MID, -120, 0);

    /* Status icons (right edge). LV_SYMBOL_* glyphs as placeholders. */
    st->icon_gps = lv_label_create(h);
    lv_label_set_text(st->icon_gps, LV_SYMBOL_GPS);
    lv_obj_set_style_text_color(st->icon_gps, icon_color(UI_ICON_OFF), LV_PART_MAIN);
    lv_obj_set_style_text_font (st->icon_gps, &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_align(st->icon_gps, LV_ALIGN_RIGHT_MID, 0, 0);

    st->icon_wifi = lv_label_create(h);
    lv_label_set_text(st->icon_wifi, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(st->icon_wifi, icon_color(UI_ICON_OFF), LV_PART_MAIN);
    lv_obj_set_style_text_font (st->icon_wifi, &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_align(st->icon_wifi, LV_ALIGN_RIGHT_MID, -36, 0);

    st->icon_sd = lv_label_create(h);
    lv_label_set_text(st->icon_sd, LV_SYMBOL_SD_CARD);
    lv_obj_set_style_text_color(st->icon_sd, icon_color(UI_ICON_OFF), LV_PART_MAIN);
    lv_obj_set_style_text_font (st->icon_sd, &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_align(st->icon_sd, LV_ALIGN_RIGHT_MID, -72, 0);

    lv_obj_set_user_data(h, st);
    return h;
}

void ui_header_set_state_pill(lv_obj_t *header, ui_state_pill_t s)
{
    header_state_t *st = header_state(header);
    if (!st) return;
    lv_obj_set_style_bg_color(st->pill_bg, state_pill_color(s), LV_PART_MAIN);
    lv_label_set_text(st->pill_text, state_pill_text(s));
}

void ui_header_set_boat_name(lv_obj_t *header, const char *name)
{
    header_state_t *st = header_state(header);
    if (!st) return;
    lv_label_set_text(st->boat, name ? name : "");
}

void ui_header_set_time(lv_obj_t *header, int hour24, int minute)
{
    header_state_t *st = header_state(header);
    if (!st) return;
    char buf[8];
    snprintf(buf, sizeof(buf), "%02d:%02d", hour24, minute);
    lv_label_set_text(st->time, buf);
}

void ui_header_set_icon_sd(lv_obj_t *header, ui_icon_state_t s)
{
    header_state_t *st = header_state(header);
    if (!st) return;
    lv_obj_set_style_text_color(st->icon_sd, icon_color(s), LV_PART_MAIN);
}

void ui_header_set_icon_wifi(lv_obj_t *header, ui_icon_state_t s)
{
    header_state_t *st = header_state(header);
    if (!st) return;
    lv_obj_set_style_text_color(st->icon_wifi, icon_color(s), LV_PART_MAIN);
}

void ui_header_set_icon_gps(lv_obj_t *header, ui_icon_state_t s)
{
    header_state_t *st = header_state(header);
    if (!st) return;
    lv_obj_set_style_text_color(st->icon_gps, icon_color(s), LV_PART_MAIN);
}

/* ---- Footer ---- */

typedef struct {
    lv_obj_t *prev;
    lv_obj_t *current;
    lv_obj_t *next;
} footer_state_t;

static footer_state_t *footer_state(lv_obj_t *f)
{
    return (footer_state_t *) lv_obj_get_user_data(f);
}

lv_obj_t *ui_footer_create(lv_obj_t *parent)
{
    lv_obj_t *f = lv_obj_create(parent);
    lv_obj_set_size(f, LV_HOR_RES, 64);
    lv_obj_align(f, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color    (f, UI_COLOR(SURFACE_ELEVATED), LV_PART_MAIN);
    lv_obj_set_style_bg_opa      (f, LV_OPA_COVER,               LV_PART_MAIN);
    lv_obj_set_style_border_color(f, UI_COLOR(SURFACE_BORDER),   LV_PART_MAIN);
    lv_obj_set_style_border_side (f, LV_BORDER_SIDE_TOP,         LV_PART_MAIN);
    lv_obj_set_style_border_width(f, 1, LV_PART_MAIN);
    lv_obj_set_style_pad_hor     (f, 24, LV_PART_MAIN);
    lv_obj_set_style_pad_ver     (f, 0,  LV_PART_MAIN);
    lv_obj_clear_flag            (f, LV_OBJ_FLAG_SCROLLABLE);

    footer_state_t *st = lv_mem_alloc(sizeof(*st));
    if (!st) return f;
    *st = (footer_state_t){0};

    st->prev = lv_label_create(f);
    lv_label_set_text(st->prev, LV_SYMBOL_LEFT "  Prev");
    lv_obj_set_style_text_color(st->prev, UI_COLOR(TEXT_DIM), LV_PART_MAIN);
    lv_obj_set_style_text_font (st->prev, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(st->prev, LV_ALIGN_LEFT_MID, 0, 0);

    st->current = lv_label_create(f);
    lv_label_set_text(st->current, "");
    lv_obj_set_style_text_color(st->current, UI_COLOR(ACTION_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_text_font (st->current, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_align(st->current, LV_ALIGN_CENTER, 0, 0);

    st->next = lv_label_create(f);
    lv_label_set_text(st->next, "Next  " LV_SYMBOL_RIGHT);
    lv_obj_set_style_text_color(st->next, UI_COLOR(TEXT_DIM), LV_PART_MAIN);
    lv_obj_set_style_text_font (st->next, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(st->next, LV_ALIGN_RIGHT_MID, 0, 0);

    lv_obj_set_user_data(f, st);
    return f;
}

void ui_footer_set_sections(lv_obj_t *footer,
                             const char *prev_label,
                             const char *current_label,
                             const char *next_label)
{
    footer_state_t *st = footer_state(footer);
    if (!st) return;

    char buf[48];
    snprintf(buf, sizeof(buf), "%s  %s", LV_SYMBOL_LEFT, prev_label ? prev_label : "");
    lv_label_set_text(st->prev, buf);

    lv_label_set_text(st->current, current_label ? current_label : "");

    snprintf(buf, sizeof(buf), "%s  %s", next_label ? next_label : "", LV_SYMBOL_RIGHT);
    lv_label_set_text(st->next, buf);
}

/* ---- Warning banner ---- */

typedef struct {
    lv_obj_t *message;
    lv_obj_t *btn;
    lv_obj_t *btn_label;
    bool visible;
} banner_state_t;

static banner_state_t *banner_state(lv_obj_t *b)
{
    return (banner_state_t *) lv_obj_get_user_data(b);
}

static lv_color_t banner_color(ui_banner_level_t level)
{
    switch (level) {
        case UI_BANNER_WARN:  return UI_COLOR(STATE_WARNING);
        case UI_BANNER_ALARM: return UI_COLOR(STATE_ALARM);
        case UI_BANNER_INFO:
        default:              return UI_COLOR(STATE_ON);
    }
}

lv_obj_t *ui_banner_create(lv_obj_t *parent)
{
    lv_obj_t *b = lv_obj_create(parent);
    lv_obj_set_size(b, LV_HOR_RES, 40);
    lv_obj_align(b, LV_ALIGN_TOP_MID, 0, 64);   /* directly under the header */
    lv_obj_set_style_bg_color    (b, UI_COLOR(STATE_WARNING),  LV_PART_MAIN);
    lv_obj_set_style_bg_opa      (b, LV_OPA_COVER,             LV_PART_MAIN);
    lv_obj_set_style_border_width(b, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_hor     (b, 16, LV_PART_MAIN);
    lv_obj_set_style_pad_ver     (b, 0,  LV_PART_MAIN);
    lv_obj_clear_flag            (b, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag              (b, LV_OBJ_FLAG_HIDDEN);

    banner_state_t *st = lv_mem_alloc(sizeof(*st));
    if (!st) return b;
    *st = (banner_state_t){0};

    st->message = lv_label_create(b);
    lv_label_set_text(st->message, "");
    lv_obj_set_style_text_color(st->message, UI_COLOR(TEXT_BODY_STRONG), LV_PART_MAIN);
    lv_obj_set_style_text_font (st->message, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(st->message, LV_ALIGN_LEFT_MID, 8, 0);

    st->btn = lv_btn_create(b);
    lv_obj_set_size(st->btn, 120, 28);
    lv_obj_align(st->btn, LV_ALIGN_RIGHT_MID, -8, 0);
    lv_obj_set_style_bg_color(st->btn, UI_COLOR(SURFACE_ELEVATED), LV_PART_MAIN);
    lv_obj_add_flag(st->btn, LV_OBJ_FLAG_HIDDEN);

    st->btn_label = lv_label_create(st->btn);
    lv_label_set_text(st->btn_label, "");
    lv_obj_set_style_text_color(st->btn_label, UI_COLOR(TEXT_BODY_STRONG), LV_PART_MAIN);
    lv_obj_set_style_text_font (st->btn_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_center(st->btn_label);

    lv_obj_set_user_data(b, st);
    return b;
}

void ui_banner_show(lv_obj_t *banner,
                     ui_banner_level_t level,
                     const char *message,
                     const char *action_label)
{
    banner_state_t *st = banner_state(banner);
    if (!st) return;

    lv_obj_set_style_bg_color(banner, banner_color(level), LV_PART_MAIN);
    lv_label_set_text(st->message, message ? message : "");

    if (action_label && *action_label) {
        lv_label_set_text(st->btn_label, action_label);
        lv_obj_clear_flag(st->btn, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(st->btn, LV_OBJ_FLAG_HIDDEN);
    }

    lv_obj_clear_flag(banner, LV_OBJ_FLAG_HIDDEN);
    st->visible = true;
}

void ui_banner_hide(lv_obj_t *banner)
{
    banner_state_t *st = banner_state(banner);
    if (!st) return;
    lv_obj_add_flag(banner, LV_OBJ_FLAG_HIDDEN);
    st->visible = false;
}

bool ui_banner_visible(lv_obj_t *banner)
{
    banner_state_t *st = banner_state(banner);
    return st && st->visible;
}
