/*
 * Diagnostics screen — implementation (#74 m1).
 *
 * Spec: https://github.com/Temple-of-Epiphany/anchor-drag-pro/wiki/Screen-Diagnostics
 *
 * Five sections (System / Live data / Storage / Tests / Destructive)
 * with list-row pattern. Quick actions fire on tap; destructive rows
 * go through ui_confirm_show (exercising #75 on hardware).
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#include "screen_diagnostics.h"
#include "ui_chrome.h"
#include "ui_tokens.h"
#include "ui_confirm.h"
#include "ui_ota.h"
#include "ota.h"
#include "lvgl_init.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_app_format.h"
#include "esp_app_desc.h"
#include "esp_ota_ops.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "nvs_flash.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "diag";

typedef struct {
    lv_obj_t *header;
    lv_obj_t *footer;
    lv_obj_t *banner;
    lv_obj_t *uptime_value_lbl;
    lv_obj_t *heap_value_lbl;
} diag_state_t;

static diag_state_t *get_state(lv_obj_t *s) {
    return (diag_state_t *) lv_obj_get_user_data(s);
}

/* ---- Section + row helpers ---- */

static lv_obj_t *build_section(lv_obj_t *parent, const char *title, bool destructive)
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
    lv_obj_set_style_text_color(t,
        destructive ? UI_COLOR(STATE_ALARM) : UI_COLOR(TEXT_LABEL), LV_PART_MAIN);
    lv_obj_set_style_text_font (t, &lv_font_montserrat_14, LV_PART_MAIN);

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

/* Returns the value label so the caller can store it for live updates. */
static lv_obj_t *kv_row(lv_obj_t *card, const char *label, const char *value)
{
    lv_obj_t *row = lv_obj_create(card);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, 28);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *k = lv_label_create(row);
    lv_label_set_text(k, label);
    lv_obj_set_style_text_color(k, UI_COLOR(TEXT_BODY),     LV_PART_MAIN);
    lv_obj_set_style_text_font (k, &lv_font_montserrat_16,  LV_PART_MAIN);

    lv_obj_t *v = lv_label_create(row);
    lv_label_set_text(v, value);
    lv_obj_set_style_text_color(v, UI_COLOR(TEXT_BODY_DIM), LV_PART_MAIN);
    lv_obj_set_style_text_font (v, &lv_font_montserrat_14,  LV_PART_MAIN);
    lv_label_set_long_mode     (v, LV_LABEL_LONG_DOT);
    return v;
}

/* Tappable row with a click handler. */
static lv_obj_t *action_row(lv_obj_t *card, const char *label, const char *detail,
                             lv_event_cb_t cb, void *user_data,
                             bool destructive)
{
    lv_obj_t *row = lv_obj_create(card);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, 32);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag  (row, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *k = lv_label_create(row);
    lv_label_set_text(k, label);
    lv_obj_set_style_text_color(k,
        destructive ? UI_COLOR(TEXT_BODY) : UI_COLOR(TEXT_BODY), LV_PART_MAIN);
    lv_obj_set_style_text_font (k, &lv_font_montserrat_16, LV_PART_MAIN);

    lv_obj_t *v = lv_label_create(row);
    lv_label_set_text(v, detail ? detail : "");
    lv_obj_set_style_text_color(v, UI_COLOR(TEXT_BODY_DIM), LV_PART_MAIN);
    lv_obj_set_style_text_font (v, &lv_font_montserrat_14,  LV_PART_MAIN);
    lv_label_set_long_mode     (v, LV_LABEL_LONG_DOT);

    if (cb) {
        lv_obj_add_event_cb(row, cb, LV_EVENT_CLICKED, user_data);
    }
    return row;
}

/* ---- Quick actions ---- */

static void on_buzzer_test(lv_event_t *e)
{
    (void) e;
    ESP_LOGI(TAG, "buzzer test — driver not yet implemented");
}

static void on_restart_confirmed(void *user_data)
{
    (void) user_data;
    ESP_LOGW(TAG, "Restart confirmed — esp_restart() in 500 ms");
    vTaskDelay(pdMS_TO_TICKS(500));
    esp_restart();
}

static void on_restart_clicked(lv_event_t *e)
{
    (void) e;
    ui_confirm_params_t p = {
        .title         = "Restart device?",
        .body          = { "The device will reboot. Anchor watch stops while restarting.", NULL },
        .final_warning = NULL,
        .cancel_label  = "Cancel",
        .confirm_label = "Restart now",
        .hold_ms       = 0,    /* single tap */
        .severity      = UI_CONFIRM_LOW,
        .on_confirm    = on_restart_confirmed,
    };
    ui_confirm_show(&p);
}

static void on_factory_reset_confirmed(void *user_data)
{
    (void) user_data;
    ESP_LOGW(TAG, "Factory reset confirmed — erasing NVS, will restart");
    nvs_flash_erase();
    vTaskDelay(pdMS_TO_TICKS(500));
    esp_restart();
}

static void on_factory_reset_clicked(lv_event_t *e)
{
    (void) e;
    ui_confirm_params_t p = {
        .title         = "Factory reset",
        .body          = {
            "This will erase all NVS settings and restart the device.",
            "Config on the SD card is NOT deleted.",
            NULL,
        },
        .final_warning = "This cannot be undone.",
        .cancel_label  = "Cancel",
        .confirm_label = "Hold to reset",
        .hold_ms       = UI_CONFIRM_HOLD_DEFAULT,
        .severity      = UI_CONFIRM_HIGH,
        .on_confirm    = on_factory_reset_confirmed,
    };
    ui_confirm_show(&p);
}

static void on_clear_tracks_confirmed(void *user_data)
{
    (void) user_data;
    ESP_LOGW(TAG, "Clear GPS tracks confirmed — SD writes deferred to logger workstream");
}

static void on_clear_tracks_clicked(lv_event_t *e)
{
    (void) e;
    ui_confirm_params_t p = {
        .title         = "Clear GPS tracks",
        .body          = { "Delete all .csv tracks on the SD card.", NULL },
        .final_warning = "This cannot be undone.",
        .cancel_label  = "Cancel",
        .confirm_label = "Hold to delete",
        .hold_ms       = UI_CONFIRM_HOLD_DEFAULT,
        .severity      = UI_CONFIRM_HIGH,
        .on_confirm    = on_clear_tracks_confirmed,
    };
    ui_confirm_show(&p);
}

static void on_clear_logs_confirmed(void *user_data)
{
    (void) user_data;
    ESP_LOGW(TAG, "Clear logs confirmed — log file rotation TBD");
}

static void on_clear_logs_clicked(lv_event_t *e)
{
    (void) e;
    ui_confirm_params_t p = {
        .title         = "Clear logs",
        .body          = { "Delete all system logs on the SD card.", NULL },
        .final_warning = "This cannot be undone.",
        .cancel_label  = "Cancel",
        .confirm_label = "Hold to delete",
        .hold_ms       = UI_CONFIRM_HOLD_DEFAULT,
        .severity      = UI_CONFIRM_HIGH,
        .on_confirm    = on_clear_logs_confirmed,
    };
    ui_confirm_show(&p);
}

/* ---- OTA: check for update + install with progress ---- */

/* Single-instance OTA flow. The scanned candidate is held here so the
 * install worker (a separate task) can read it after the user confirms. */
static ota_update_info_t s_ota_info;

/* Runs on the worker task. Maps ota.c progress phases onto the modal.
 * ui_ota_* lock LVGL internally, so calling them off the LVGL task is
 * safe. ota_apply() reboots on success and never returns. */
static void ota_progress_trampoline(ota_phase_t phase, int pct,
                                     const char *msg, void *user)
{
    (void) user;
    switch (phase) {
        case OTA_PHASE_VERIFY: ui_ota_set_progress("Verifying", pct); break;
        case OTA_PHASE_FLASH:  ui_ota_set_progress("Flashing",  pct); break;
        case OTA_PHASE_DONE:   ui_ota_set_progress("Rebooting", 100); break;
        case OTA_PHASE_ERROR:  ui_ota_show_error(msg);                break;
    }
}

static void ota_install_task(void *arg)
{
    (void) arg;
    esp_err_t err = ota_apply(&s_ota_info, ota_progress_trampoline, NULL);
    /* Only reached on failure — success reboots inside ota_apply. The
     * error phase is already shown via the trampoline. */
    ESP_LOGE(TAG, "ota_apply returned %s (failed)", esp_err_to_name(err));
    vTaskDelete(NULL);
}

static void on_ota_install(void *user_data)
{
    (void) user_data;
    ui_ota_begin_progress();
    /* 8 KB stack: SHA256 + esp_ota_write + FATFS read path. */
    if (xTaskCreate(ota_install_task, "ota_install", 8192, NULL, 4, NULL) != pdPASS) {
        ESP_LOGE(TAG, "could not start OTA install task");
        ui_ota_show_error("Could not start the update task");
    }
}

static void show_info_dialog(const char *title, const char *line)
{
    ui_confirm_params_t p = {
        .title         = title,
        .body          = { line, NULL },
        .final_warning = NULL,
        .cancel_label  = "Close",
        .confirm_label = "OK",
        .hold_ms       = 0,
        .severity      = UI_CONFIRM_LOW,
        .on_confirm    = NULL,
    };
    ui_confirm_show(&p);
}

static void on_check_update_clicked(lv_event_t *e)
{
    (void) e;
    esp_err_t err = ota_scan_sd(&s_ota_info);
    if (err == ESP_ERR_INVALID_STATE) {
        show_info_dialog("Check for update", "SD card is not mounted.");
        return;
    }
    if (err != ESP_OK) {
        show_info_dialog("Check for update", "Could not read the SD card.");
        return;
    }
    if (!s_ota_info.available) {
        char buf[80];
        snprintf(buf, sizeof(buf), "Firmware is up to date (v%d.%d.%d).",
                 s_ota_info.running_major, s_ota_info.running_minor,
                 s_ota_info.running_patch);
        show_info_dialog("Check for update", buf);
        return;
    }
    ui_ota_params_t p = {
        .from_major = s_ota_info.running_major,
        .from_minor = s_ota_info.running_minor,
        .from_patch = s_ota_info.running_patch,
        .to_major   = s_ota_info.new_major,
        .to_minor   = s_ota_info.new_minor,
        .to_patch   = s_ota_info.new_patch,
        .notes      = "A newer firmware build was found on the SD card. "
                      "Installing takes about 30 seconds and reboots the device.",
        .on_install = on_ota_install,
        .on_skip    = NULL,
        .user_data  = NULL,
    };
    ui_ota_show(&p);
}

/* ---- Construction ---- */

lv_obj_t *screen_diagnostics_create(void)
{
    if (!lvgl_lock(2000)) {
        ESP_LOGE(TAG, "could not acquire LVGL mutex");
        return NULL;
    }

    diag_state_t *st = lv_mem_alloc(sizeof(*st));
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
    ui_footer_set_sections(st->footer, "Settings", "DIAGNOSTICS", "Connections");

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

    /* ---- System ---- */
    lv_obj_t *system = build_section(content, "System", false);
    const esp_app_desc_t *app = esp_app_get_description();
    char fw_buf[48];
    snprintf(fw_buf, sizeof(fw_buf), "v%s", app ? app->version : "?");
    kv_row(system, "Firmware",   fw_buf);
    uint32_t uptime_s = (uint32_t)(esp_timer_get_time() / 1000000ULL);
    char up_buf[24];
    snprintf(up_buf, sizeof(up_buf), "%lu s", (unsigned long) uptime_s);
    st->uptime_value_lbl = kv_row(system, "Uptime", up_buf);
    char heap_buf[32];
    snprintf(heap_buf, sizeof(heap_buf), "%lu KB",
             (unsigned long) (esp_get_free_heap_size() / 1024));
    st->heap_value_lbl = kv_row(system, "Free heap", heap_buf);
    action_row(system, "Check for update", "scan SD for newer firmware",
                on_check_update_clicked, NULL, false);

    /* ---- Live data ---- */
    lv_obj_t *live = build_section(content, "Live data", false);
    action_row(live, "PGN monitor",            "N2K driver not yet implemented", NULL, NULL, false);
    action_row(live, "Sensor scan",            "I²C scan logs on boot",           NULL, NULL, false);
    action_row(live, "Source-priority trace",  "depends on source manager",       NULL, NULL, false);

    /* ---- Storage ---- */
    lv_obj_t *storage = build_section(content, "Storage", false);
    action_row(storage, "SD card", "see Connections for mount status", NULL, NULL, false);
    action_row(storage, "Logs",    "log rotation TBD",                  NULL, NULL, false);

    /* ---- Tests ---- */
    lv_obj_t *tests = build_section(content, "Tests", false);
    action_row(tests, "Buzzer test", "tap to play 500 ms tone", on_buzzer_test, NULL, false);
    action_row(tests, "Display test pattern", "SMPTE bars (TBD)", NULL, NULL, false);
    action_row(tests, "Touch test", "live touch dot (TBD)", NULL, NULL, false);
    action_row(tests, "Backlight test", "sweep 0–100% (TBD)", NULL, NULL, false);

    /* ---- Destructive ---- */
    lv_obj_t *destructive = build_section(content, "Destructive", true);
    action_row(destructive, "Clear GPS tracks", "delete /sdcard/anchor/tracks/*.csv",
                on_clear_tracks_clicked, NULL, true);
    action_row(destructive, "Clear logs", "delete /sdcard/anchor/logs/*.log",
                on_clear_logs_clicked, NULL, true);
    action_row(destructive, "Factory reset", "erase NVS, restart",
                on_factory_reset_clicked, NULL, true);
    action_row(destructive, "Restart device", "soft reboot",
                on_restart_clicked, NULL, true);

    lv_obj_set_user_data(scr, st);
    lvgl_unlock();

    ESP_LOGI(TAG, "diagnostics screen created");
    return scr;
}

lv_obj_t *screen_diagnostics_header(lv_obj_t *s) { diag_state_t *st = get_state(s); return st ? st->header : NULL; }
lv_obj_t *screen_diagnostics_footer(lv_obj_t *s) { diag_state_t *st = get_state(s); return st ? st->footer : NULL; }
