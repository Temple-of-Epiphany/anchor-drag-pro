/*
 * Splash screen — implementation (milestone 2 of #69).
 *
 * Spec: https://github.com/Temple-of-Epiphany/anchor-drag-pro/wiki/Screen-Splash
 *
 * Renders the brand block + a 10-row self-test list. Rows are populated
 * progressively as main.c reports phase completions via
 * screen_splash_set_row(). All visual styling sourced from
 * anchor_design_tokens.h via UI_COLOR(); no inline literals.
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#include "screen_splash.h"
#include "ui_tokens.h"
#include "lvgl_init.h"
#include "esp_log.h"
#include "lvgl.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "splash";

/* Per-row state — the LVGL widgets and the cached label so we can
 * re-render on status change. */
typedef struct {
    lv_obj_t *glyph;     /* status glyph label */
    lv_obj_t *name;      /* row name */
    lv_obj_t *detail;    /* detail text (right of name) */
} splash_row_widgets_t;

static lv_obj_t *s_screen = NULL;
static splash_row_widgets_t s_rows[SPLASH_ROW_COUNT];

/* Canonical row names — keyed for future i18n via the strings table. */
static const char *row_label(splash_row_id_t row)
{
    switch (row) {
        case SPLASH_ROW_I2C:      return "I2C bus";
        case SPLASH_ROW_CH422G:   return "CH422G I/O expander";
        case SPLASH_ROW_RTC:      return "RTC";
        case SPLASH_ROW_SD:       return "SD card";
        case SPLASH_ROW_CONFIG:   return "Configuration loaded";
        case SPLASH_ROW_OTA:      return "OTA partition status";
        case SPLASH_ROW_WIFI:     return "WiFi";
        case SPLASH_ROW_GPS:      return "GPS source";
        case SPLASH_ROW_IMU:      return "IMU source";
        case SPLASH_ROW_DISPLAY:  return "Display";
        default:                  return "?";
    }
}

static const char *status_glyph(splash_row_status_t s)
{
    switch (s) {
        case SPLASH_STATUS_RUNNING:    return LV_SYMBOL_REFRESH;
        case SPLASH_STATUS_PASS:       return LV_SYMBOL_OK;
        case SPLASH_STATUS_WARN:       return LV_SYMBOL_WARNING;
        case SPLASH_STATUS_FAIL:       return LV_SYMBOL_CLOSE;
        case SPLASH_STATUS_SKIP:       return LV_SYMBOL_MINUS;
        case SPLASH_STATUS_CONTINUING: return LV_SYMBOL_REFRESH;
        default:                       return " ";
    }
}

static lv_color_t status_color(splash_row_status_t s)
{
    switch (s) {
        case SPLASH_STATUS_PASS:       return UI_COLOR(STATE_ARMED);
        case SPLASH_STATUS_WARN:       return UI_COLOR(STATE_WARNING);
        case SPLASH_STATUS_FAIL:       return UI_COLOR(STATE_ALARM);
        case SPLASH_STATUS_RUNNING:
        case SPLASH_STATUS_CONTINUING:
        case SPLASH_STATUS_SKIP:
        default:                       return UI_COLOR(TEXT_DIM);
    }
}

esp_err_t screen_splash_show(const char *firmware_version)
{
    if (!firmware_version) firmware_version = "?.?.?";

    if (!lvgl_lock(2000)) {
        ESP_LOGE(TAG, "could not acquire LVGL mutex");
        return ESP_ERR_TIMEOUT;
    }

    /* Whole-screen container — uses the token background. */
    s_screen = lv_obj_create(NULL);
    lv_obj_set_size(s_screen, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_color(s_screen, UI_COLOR(BG_APP),       LV_PART_MAIN);
    lv_obj_set_style_bg_opa  (s_screen, LV_OPA_COVER,           LV_PART_MAIN);
    lv_obj_set_style_pad_all (s_screen, 0,                       LV_PART_MAIN);
    lv_obj_set_style_border_width(s_screen, 0,                   LV_PART_MAIN);
    lv_obj_clear_flag        (s_screen, LV_OBJ_FLAG_SCROLLABLE);

    /* Brand block — title + version, top centre. */
    lv_obj_t *title = lv_label_create(s_screen);
    lv_label_set_text(title, "Anchor Drag Pro");
    lv_obj_set_style_text_color(title, UI_COLOR(TEXT_BODY_STRONG), LV_PART_MAIN);
    lv_obj_set_style_text_font (title, &lv_font_montserrat_32,     LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 48);

    char ver_buf[64];
    snprintf(ver_buf, sizeof(ver_buf), "Firmware v%s", firmware_version);

    lv_obj_t *version = lv_label_create(s_screen);
    lv_label_set_text(version, ver_buf);
    lv_obj_set_style_text_color(version, UI_COLOR(TEXT_BODY_DIM), LV_PART_MAIN);
    lv_obj_set_style_text_font (version, &lv_font_montserrat_16,  LV_PART_MAIN);
    lv_obj_align(version, LV_ALIGN_TOP_MID, 0, 96);

    /* Self-test panel — sits below the brand. */
    lv_obj_t *panel = lv_obj_create(s_screen);
    lv_obj_set_size(panel, 600, 280);
    lv_obj_align(panel, LV_ALIGN_TOP_MID, 0, 160);
    lv_obj_set_style_bg_color    (panel, UI_COLOR(SURFACE_ELEVATED), LV_PART_MAIN);
    lv_obj_set_style_bg_opa      (panel, LV_OPA_COVER,               LV_PART_MAIN);
    lv_obj_set_style_border_color(panel, UI_COLOR(SURFACE_BORDER),   LV_PART_MAIN);
    lv_obj_set_style_border_width(panel, 1,                          LV_PART_MAIN);
    lv_obj_set_style_radius      (panel, 8,                          LV_PART_MAIN);
    lv_obj_set_style_pad_all     (panel, 16,                         LV_PART_MAIN);
    lv_obj_set_layout            (panel, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow         (panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align        (panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row     (panel, 4,                          LV_PART_MAIN);
    lv_obj_clear_flag            (panel, LV_OBJ_FLAG_SCROLLABLE);

    /* Pre-build all 10 rows with empty glyphs. Rows fill in as the boot
     * sequence calls screen_splash_set_row(). */
    for (int i = 0; i < SPLASH_ROW_COUNT; i++) {
        lv_obj_t *row = lv_obj_create(panel);
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

        /* Glyph (fixed 24 px). */
        s_rows[i].glyph = lv_label_create(row);
        lv_obj_set_width(s_rows[i].glyph, 24);
        lv_label_set_text(s_rows[i].glyph, " ");
        lv_obj_set_style_text_color(s_rows[i].glyph, UI_COLOR(TEXT_DIM),         LV_PART_MAIN);
        lv_obj_set_style_text_font (s_rows[i].glyph, &lv_font_montserrat_18,     LV_PART_MAIN);

        /* Name. */
        s_rows[i].name = lv_label_create(row);
        lv_obj_set_width(s_rows[i].name, 220);
        lv_label_set_text(s_rows[i].name, row_label((splash_row_id_t) i));
        lv_obj_set_style_text_color(s_rows[i].name, UI_COLOR(TEXT_BODY),        LV_PART_MAIN);
        lv_obj_set_style_text_font (s_rows[i].name, &lv_font_montserrat_16,    LV_PART_MAIN);

        /* Detail (flex remainder). */
        s_rows[i].detail = lv_label_create(row);
        lv_obj_set_flex_grow(s_rows[i].detail, 1);
        lv_label_set_text(s_rows[i].detail, "");
        lv_obj_set_style_text_color(s_rows[i].detail, UI_COLOR(TEXT_BODY_DIM), LV_PART_MAIN);
        lv_obj_set_style_text_font (s_rows[i].detail, &lv_font_montserrat_14,  LV_PART_MAIN);
    }

    lv_scr_load(s_screen);

    lvgl_unlock();

    ESP_LOGI(TAG, "splash shown (firmware v%s)", firmware_version);
    return ESP_OK;
}

void screen_splash_set_row(splash_row_id_t row,
                            splash_row_status_t status,
                            const char *detail)
{
    if (row >= SPLASH_ROW_COUNT) return;
    if (!s_rows[row].glyph) return;     /* show not called yet */

    if (!lvgl_lock(500)) return;

    lv_label_set_text         (s_rows[row].glyph, status_glyph(status));
    lv_obj_set_style_text_color(s_rows[row].glyph, status_color(status), LV_PART_MAIN);

    if (detail && *detail) {
        lv_label_set_text(s_rows[row].detail, detail);
    } else {
        lv_label_set_text(s_rows[row].detail, "");
    }

    lvgl_unlock();
}

void screen_splash_done(void)
{
    /* For #69 milestone 2: nothing to do — Monitor (#70) introduces the
     * actual hand-off. Rows that were never set retain their empty
     * glyph; that's an indicator main.c skipped reporting them, which
     * is itself a useful diagnostic. */
}
