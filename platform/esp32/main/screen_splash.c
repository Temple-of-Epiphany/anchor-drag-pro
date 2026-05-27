/*
 * Splash screen — implementation (milestone 1 of #69).
 *
 * Spec: https://github.com/Temple-of-Epiphany/anchor-drag-pro/wiki/Screen-Splash
 *
 * This is the minimum viable splash: brand title + version + a single
 * live status line. The full self-test row list (per spec) lands in a
 * follow-up commit within #69; this checkpoint proves the LVGL chain
 * (display + tokens + render task) works end-to-end on hardware.
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

static lv_obj_t *s_screen        = NULL;
static lv_obj_t *s_status_label  = NULL;

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
    lv_obj_clear_flag        (s_screen, LV_OBJ_FLAG_SCROLLABLE);

    /* Product title — centred upper third. */
    lv_obj_t *title = lv_label_create(s_screen);
    lv_label_set_text(title, "Anchor Drag Pro");
    lv_obj_set_style_text_color(title, UI_COLOR(TEXT_BODY_STRONG), LV_PART_MAIN);
    lv_obj_set_style_text_font (title, &lv_font_montserrat_32,     LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, -60);

    /* Version line — under the title. */
    char ver_buf[64];
    snprintf(ver_buf, sizeof(ver_buf), "Firmware v%s", firmware_version);

    lv_obj_t *version = lv_label_create(s_screen);
    lv_label_set_text(version, ver_buf);
    lv_obj_set_style_text_color(version, UI_COLOR(TEXT_BODY_DIM), LV_PART_MAIN);
    lv_obj_set_style_text_font (version, &lv_font_montserrat_16,  LV_PART_MAIN);
    lv_obj_align(version, LV_ALIGN_CENTER, 0, -20);

    /* Live status line — lower third. */
    s_status_label = lv_label_create(s_screen);
    lv_label_set_text(s_status_label, "Booting...");
    lv_obj_set_style_text_color(s_status_label, UI_COLOR(STATE_ON),       LV_PART_MAIN);
    lv_obj_set_style_text_font (s_status_label, &lv_font_montserrat_18,   LV_PART_MAIN);
    lv_obj_align(s_status_label, LV_ALIGN_CENTER, 0, 60);

    lv_scr_load(s_screen);

    lvgl_unlock();

    ESP_LOGI(TAG, "splash shown (firmware v%s)", firmware_version);
    return ESP_OK;
}

void screen_splash_set_status(const char *text)
{
    if (!text || !s_status_label) return;
    if (!lvgl_lock(500)) return;
    lv_label_set_text(s_status_label, text);
    lvgl_unlock();
}
