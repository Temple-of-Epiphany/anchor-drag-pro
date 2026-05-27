/*
 * WiFi STA manager — implementation.
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#include "wifi_manager.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_mac.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "wifi_mgr";

#define MAX_SCAN_RESULTS  20
#define MIN_RETRY_MS      2000
#define MAX_RETRY_MS      60000

static SemaphoreHandle_t s_state_mtx = NULL;
static wifi_mgr_status_t s_status     = { .state = WIFI_MGR_IDLE };
static anchor_wifi_cfg_t s_cfg_copy;       /* deep copy from caller */
static bool              s_have_cfg   = false;
static bool              s_initted    = false;
static esp_netif_t      *s_netif_sta  = NULL;
static TaskHandle_t      s_worker     = NULL;
static uint32_t          s_retry_ms   = MIN_RETRY_MS;

/* ---- helpers ---- */

static void set_state(wifi_mgr_state_t st)
{
    if (xSemaphoreTakeRecursive(s_state_mtx, portMAX_DELAY) == pdTRUE) {
        s_status.state = st;
        xSemaphoreGiveRecursive(s_state_mtx);
    }
}

static void set_ssid(const char *ssid)
{
    if (xSemaphoreTakeRecursive(s_state_mtx, portMAX_DELAY) == pdTRUE) {
        snprintf(s_status.ssid, sizeof(s_status.ssid), "%s", ssid ? ssid : "");
        xSemaphoreGiveRecursive(s_state_mtx);
    }
}

static void set_rssi(int rssi)
{
    if (xSemaphoreTakeRecursive(s_state_mtx, portMAX_DELAY) == pdTRUE) {
        s_status.rssi = rssi;
        xSemaphoreGiveRecursive(s_state_mtx);
    }
}

static void set_ip(const esp_ip4_addr_t *ip)
{
    if (xSemaphoreTakeRecursive(s_state_mtx, portMAX_DELAY) == pdTRUE) {
        if (ip) {
            snprintf(s_status.ip, sizeof(s_status.ip), IPSTR, IP2STR(ip));
        } else {
            s_status.ip[0] = '\0';
        }
        xSemaphoreGiveRecursive(s_state_mtx);
    }
}

static void inc_disconnect(void)
{
    if (xSemaphoreTakeRecursive(s_state_mtx, portMAX_DELAY) == pdTRUE) {
        s_status.disconnect_count++;
        xSemaphoreGiveRecursive(s_state_mtx);
    }
}

/* ---- WiFi event handler ---- */

static void on_wifi_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void) arg;
    if (base == WIFI_EVENT) {
        switch (id) {
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "STA started; waiting for scan trigger");
                break;
            case WIFI_EVENT_STA_CONNECTED: {
                wifi_event_sta_connected_t *e = (wifi_event_sta_connected_t *) data;
                ESP_LOGI(TAG, "associated with \"%.*s\" on channel %d",
                         e->ssid_len, e->ssid, e->channel);
                /* Don't flip to CONNECTED until we have an IP (IP_EVENT_STA_GOT_IP). */
                break;
            }
            case WIFI_EVENT_STA_DISCONNECTED: {
                wifi_event_sta_disconnected_t *e = (wifi_event_sta_disconnected_t *) data;
                ESP_LOGW(TAG, "disconnected (reason=%d)", e->reason);
                inc_disconnect();
                set_state(WIFI_MGR_DISCONNECTED);
                set_ip(NULL);
                /* Worker task watches state and retries. */
                if (s_worker) xTaskNotifyGive(s_worker);
                break;
            }
            default:
                ESP_LOGD(TAG, "wifi event %ld", id);
                break;
        }
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *e = (ip_event_got_ip_t *) data;
        ESP_LOGI(TAG, "got IP " IPSTR, IP2STR(&e->ip_info.ip));
        set_ip(&e->ip_info.ip);
        set_state(WIFI_MGR_CONNECTED);
        s_retry_ms = MIN_RETRY_MS;  /* reset backoff on success */
    }
}

/* ---- Connect attempt ----
 * Scans, picks the first sta_network whose SSID is visible (per spec
 * priority order), configures + connects. Updates status as it goes. */
static esp_err_t try_connect(void)
{
    if (!s_have_cfg) return ESP_ERR_INVALID_STATE;
    if (s_cfg_copy.sta_network_count == 0) {
        ESP_LOGW(TAG, "no STA networks configured");
        set_state(WIFI_MGR_NO_NETWORKS);
        return ESP_ERR_NOT_FOUND;
    }

    set_state(WIFI_MGR_SCANNING);

    wifi_scan_config_t scan_cfg = { 0 };  /* all channels, passive=false */
    esp_err_t err = esp_wifi_scan_start(&scan_cfg, true);   /* blocking */
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "scan_start failed: %s", esp_err_to_name(err));
        set_state(WIFI_MGR_DISCONNECTED);
        return err;
    }

    uint16_t n = 0;
    esp_wifi_scan_get_ap_num(&n);
    if (n > MAX_SCAN_RESULTS) n = MAX_SCAN_RESULTS;

    wifi_ap_record_t records[MAX_SCAN_RESULTS];
    if (n) {
        esp_wifi_scan_get_ap_records(&n, records);
    }
    ESP_LOGI(TAG, "scan found %u AP(s)", n);

    /* Pick the first sta_network whose SSID is in the scan. */
    int chosen   = -1;
    int chosen_rssi = -127;
    for (int p = 0; p < s_cfg_copy.sta_network_count; p++) {
        for (int s = 0; s < n; s++) {
            if (strncmp((char *) records[s].ssid,
                        s_cfg_copy.sta_networks[p].ssid, 33) == 0) {
                chosen      = p;
                chosen_rssi = records[s].rssi;
                break;
            }
        }
        if (chosen >= 0) break;
    }
    if (chosen < 0) {
        ESP_LOGW(TAG, "none of %d configured SSIDs are visible",
                 s_cfg_copy.sta_network_count);
        set_state(WIFI_MGR_NO_NETWORKS);
        return ESP_ERR_NOT_FOUND;
    }

    const anchor_wifi_network_t *pick = &s_cfg_copy.sta_networks[chosen];
    ESP_LOGI(TAG, "picked \"%s\" (priority %d, rssi %d dBm)",
             pick->ssid, chosen, chosen_rssi);
    set_ssid(pick->ssid);
    set_rssi(chosen_rssi);

    wifi_config_t wifi_cfg = { 0 };
    strncpy((char *) wifi_cfg.sta.ssid,     pick->ssid,     sizeof(wifi_cfg.sta.ssid));
    strncpy((char *) wifi_cfg.sta.password, pick->password, sizeof(wifi_cfg.sta.password));
    wifi_cfg.sta.threshold.authmode = WIFI_AUTH_OPEN; /* allow open; STA picks strongest cipher */

    err = esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "set_config failed: %s", esp_err_to_name(err));
        set_state(WIFI_MGR_DISCONNECTED);
        return err;
    }

    set_state(WIFI_MGR_CONNECTING);
    err = esp_wifi_connect();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "connect kickoff failed: %s", esp_err_to_name(err));
        set_state(WIFI_MGR_DISCONNECTED);
        return err;
    }
    return ESP_OK;
}

/* ---- Worker task ----
 * Waits on a notification (sent by disconnect events or by start()).
 * Each wake, if disconnected, retries with exponential backoff. */
static void worker_task(void *arg)
{
    (void) arg;
    /* Initial attempt — fire immediately on entry. */
    try_connect();

    while (1) {
        /* Wait until something changes (disconnect / reconnect request). */
        uint32_t notified = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        (void) notified;

        /* Snapshot state. */
        wifi_mgr_status_t st = { 0 };
        wifi_manager_get_status(&st);

        if (st.state == WIFI_MGR_CONNECTED) {
            /* Spurious wake — nothing to do. */
            continue;
        }

        /* Sleep retry backoff then try again. */
        if (xSemaphoreTakeRecursive(s_state_mtx, portMAX_DELAY) == pdTRUE) {
            s_status.retry_in_ms = s_retry_ms;
            xSemaphoreGiveRecursive(s_state_mtx);
        }
        ESP_LOGI(TAG, "retrying in %lu ms", (unsigned long) s_retry_ms);
        vTaskDelay(pdMS_TO_TICKS(s_retry_ms));

        /* Exponential backoff capped at MAX_RETRY_MS. */
        s_retry_ms = s_retry_ms * 2;
        if (s_retry_ms > MAX_RETRY_MS) s_retry_ms = MAX_RETRY_MS;

        try_connect();
    }
}

/* ---- Public API ---- */

esp_err_t wifi_manager_init(void)
{
    if (s_initted) return ESP_OK;

    if (!s_state_mtx) {
        s_state_mtx = xSemaphoreCreateRecursiveMutex();
        if (!s_state_mtx) return ESP_ERR_NO_MEM;
    }

    /* NVS is required by esp_wifi for AP scan-list caching. */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        err = nvs_flash_init();
    }
    /* ignore — main.c already does this */

    err = esp_netif_init();
    if (err != ESP_OK) return err;

    err = esp_event_loop_create_default();
    if (err == ESP_ERR_INVALID_STATE) err = ESP_OK;     /* main may have done it */
    if (err != ESP_OK) return err;

    s_netif_sta = esp_netif_create_default_wifi_sta();

    wifi_init_config_t wcfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&wcfg);
    if (err != ESP_OK) return err;

    err = esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                on_wifi_event, NULL, NULL);
    if (err != ESP_OK) return err;
    err = esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                                on_wifi_event, NULL, NULL);
    if (err != ESP_OK) return err;

    err = esp_wifi_set_storage(WIFI_STORAGE_RAM);
    if (err != ESP_OK) return err;
    err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err != ESP_OK) return err;
    err = esp_wifi_start();
    if (err != ESP_OK) return err;

    s_initted = true;
    ESP_LOGI(TAG, "init OK (STA mode, no connection yet)");
    return ESP_OK;
}

esp_err_t wifi_manager_start(const anchor_wifi_cfg_t *cfg)
{
    if (!cfg) return ESP_ERR_INVALID_ARG;
    if (!s_initted) {
        esp_err_t e = wifi_manager_init();
        if (e != ESP_OK) return e;
    }

    if (xSemaphoreTakeRecursive(s_state_mtx, portMAX_DELAY) == pdTRUE) {
        s_cfg_copy = *cfg;
        s_have_cfg = true;
        xSemaphoreGiveRecursive(s_state_mtx);
    }

    if (cfg->mode != WIFI_STA) {
        ESP_LOGI(TAG, "config mode is not STA (%d); not connecting", (int) cfg->mode);
        set_state(WIFI_MGR_DISABLED);
        return ESP_OK;
    }

    if (!s_worker) {
        BaseType_t r = xTaskCreatePinnedToCore(worker_task, "wifi_mgr",
                                                4096, NULL, 4, &s_worker, 0);
        if (r != pdPASS) {
            ESP_LOGE(TAG, "failed to spawn worker");
            return ESP_FAIL;
        }
    } else {
        xTaskNotifyGive(s_worker);
    }
    return ESP_OK;
}

void wifi_manager_reconnect(void)
{
    if (s_worker) {
        esp_wifi_disconnect();   /* triggers DISCONNECTED event → worker wake */
        xTaskNotifyGive(s_worker);
    }
}

void wifi_manager_get_status(wifi_mgr_status_t *out)
{
    if (!out) return;
    memset(out, 0, sizeof(*out));
    if (s_state_mtx && xSemaphoreTakeRecursive(s_state_mtx, portMAX_DELAY) == pdTRUE) {
        *out = s_status;
        xSemaphoreGiveRecursive(s_state_mtx);
    }
}

/* ---- Scan API (on-device WiFi editor support) ----
 *
 * We piggy-back on esp_wifi's scan facility but avoid blocking the
 * caller: request_scan() kicks the worker, which performs the scan
 * synchronously on its own task; results stay in s_scan_results until
 * the next scan.
 */

#define MAX_UI_SCAN_RESULTS  20

static wifi_mgr_ap_t s_scan_results[MAX_UI_SCAN_RESULTS];
static size_t        s_scan_result_count = 0;
static bool          s_scan_busy         = false;

/* Called from worker_task and on-demand scans. */
static void do_scan_into_results(void)
{
    if (xSemaphoreTakeRecursive(s_state_mtx, portMAX_DELAY) == pdTRUE) {
        s_scan_busy = true;
        xSemaphoreGiveRecursive(s_state_mtx);
    }

    wifi_scan_config_t scan_cfg = { 0 };
    esp_err_t err = esp_wifi_scan_start(&scan_cfg, true);  /* blocking */
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ui scan failed: %s", esp_err_to_name(err));
        if (xSemaphoreTakeRecursive(s_state_mtx, portMAX_DELAY) == pdTRUE) {
            s_scan_busy = false;
            xSemaphoreGiveRecursive(s_state_mtx);
        }
        return;
    }

    uint16_t n = 0;
    esp_wifi_scan_get_ap_num(&n);
    if (n > MAX_UI_SCAN_RESULTS) n = MAX_UI_SCAN_RESULTS;

    wifi_ap_record_t recs[MAX_UI_SCAN_RESULTS];
    if (n) esp_wifi_scan_get_ap_records(&n, recs);

    if (xSemaphoreTakeRecursive(s_state_mtx, portMAX_DELAY) == pdTRUE) {
        s_scan_result_count = 0;
        for (uint16_t i = 0; i < n && i < MAX_UI_SCAN_RESULTS; i++) {
            wifi_mgr_ap_t *out = &s_scan_results[s_scan_result_count++];
            snprintf(out->ssid, sizeof(out->ssid), "%s", recs[i].ssid);
            out->rssi    = recs[i].rssi;
            out->channel = recs[i].primary;
            out->secured = (recs[i].authmode != WIFI_AUTH_OPEN);
        }
        s_scan_busy = false;
        xSemaphoreGiveRecursive(s_state_mtx);
    }
    ESP_LOGI(TAG, "UI scan complete: %u APs", n);
}

/* Tiny one-shot task so request_scan() returns immediately. */
static void ui_scan_task(void *arg)
{
    (void) arg;
    do_scan_into_results();
    vTaskDelete(NULL);
}

esp_err_t wifi_manager_request_scan(void)
{
    if (!s_initted) return ESP_ERR_INVALID_STATE;
    bool busy = false;
    if (xSemaphoreTakeRecursive(s_state_mtx, portMAX_DELAY) == pdTRUE) {
        busy = s_scan_busy;
        xSemaphoreGiveRecursive(s_state_mtx);
    }
    if (busy) return ESP_ERR_INVALID_STATE;
    if (xTaskCreatePinnedToCore(ui_scan_task, "wifi_ui_scan",
                                  3072, NULL, 4, NULL, 0) != pdPASS) {
        return ESP_FAIL;
    }
    return ESP_OK;
}

size_t wifi_manager_get_scan_results(wifi_mgr_ap_t *out, size_t max)
{
    if (!out || max == 0) return 0;
    size_t copied = 0;
    if (xSemaphoreTakeRecursive(s_state_mtx, portMAX_DELAY) == pdTRUE) {
        copied = (s_scan_result_count < max) ? s_scan_result_count : max;
        memcpy(out, s_scan_results, copied * sizeof(wifi_mgr_ap_t));
        xSemaphoreGiveRecursive(s_state_mtx);
    }
    return copied;
}

bool wifi_manager_scan_in_progress(void)
{
    bool busy = false;
    if (xSemaphoreTakeRecursive(s_state_mtx, portMAX_DELAY) == pdTRUE) {
        busy = s_scan_busy;
        xSemaphoreGiveRecursive(s_state_mtx);
    }
    return busy;
}

esp_err_t wifi_manager_try_connect(const char *ssid, const char *password)
{
    if (!ssid || !*ssid) return ESP_ERR_INVALID_ARG;
    if (!s_initted)      return ESP_ERR_INVALID_STATE;

    ESP_LOGI(TAG, "try_connect: \"%s\"", ssid);
    set_ssid(ssid);
    set_state(WIFI_MGR_CONNECTING);

    wifi_config_t cfg = { 0 };
    strncpy((char *) cfg.sta.ssid,     ssid,     sizeof(cfg.sta.ssid));
    if (password) strncpy((char *) cfg.sta.password, password, sizeof(cfg.sta.password));
    cfg.sta.threshold.authmode = WIFI_AUTH_OPEN;
    esp_err_t err = esp_wifi_set_config(WIFI_IF_STA, &cfg);
    if (err != ESP_OK) return err;

    esp_wifi_disconnect();   /* drop any existing assoc */
    return esp_wifi_connect();
}
