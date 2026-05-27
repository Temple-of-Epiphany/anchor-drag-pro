/*
 * WiFi STA manager.
 *
 * Spec: docs/config-schema.md § [wifi]; tracks issue #59.
 *
 * Walks cfg.wifi.sta_networks[] in priority order on each connect
 * attempt and picks the first one whose SSID appears in the latest
 * scan. Auto-reconnects on disconnect with exponential backoff.
 * Cooperates with the existing recursive LVGL mutex pattern — never
 * calls into LVGL itself; UI subscribers poll wifi_manager_get_status().
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#pragma once

#include "anchor_config.h"
#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WIFI_MGR_IDLE = 0,        /* never started */
    WIFI_MGR_SCANNING,
    WIFI_MGR_CONNECTING,
    WIFI_MGR_CONNECTED,
    WIFI_MGR_DISCONNECTED,    /* connect failed or dropped; will retry */
    WIFI_MGR_NO_NETWORKS,     /* none of the configured SSIDs visible */
    WIFI_MGR_DISABLED,        /* cfg.wifi.mode != WIFI_STA */
} wifi_mgr_state_t;

typedef struct {
    wifi_mgr_state_t state;
    char             ssid[33];        /* last attempted / current */
    int              rssi;            /* dBm; 0 if unknown */
    char             ip[16];          /* dotted-quad; empty if not connected */
    uint32_t         disconnect_count;
    uint32_t         retry_in_ms;     /* if DISCONNECTED, how long until next retry */
} wifi_mgr_status_t;

/* Initialise the underlying esp_netif + event loop + esp_wifi. Safe
 * to call once per boot. Doesn't start a connection — call _start()
 * after config is loaded. */
esp_err_t wifi_manager_init(void);

/* Kick off scan + connect using the supplied config (caller owns the
 * pointer; the manager copies what it needs). No-op if mode != STA
 * or if sta_network_count == 0. */
esp_err_t wifi_manager_start(const anchor_wifi_cfg_t *cfg);

/* Force a re-scan + reconnect using the current config — useful after
 * the user edits the network list via the UI. */
void      wifi_manager_reconnect(void);

/* ---- Scan API (for the on-device WiFi editor) ----
 *
 * Returns a snapshot of the most recent scan results. The on-device
 * WiFi screen calls wifi_manager_request_scan() (non-blocking; result
 * arrives via the event handler) and then polls
 * wifi_manager_get_scan_results() to render the list. */

typedef struct {
    char ssid[33];
    int  rssi;            /* dBm */
    int  channel;         /* 1..13 */
    bool secured;         /* true if any auth required */
} wifi_mgr_ap_t;

/* Kick off an async scan. Returns ESP_OK if started, ESP_ERR_INVALID_STATE
 * if already scanning. The worker task will update internal results on
 * completion; UI polls via _get_scan_results below. */
esp_err_t wifi_manager_request_scan(void);

/* Copy up to `max` scan results into `out`. Returns the number actually
 * written (≤ max, ≤ internal cap of 20). */
size_t    wifi_manager_get_scan_results(wifi_mgr_ap_t *out, size_t max);

/* True if a scan is currently running. */
bool      wifi_manager_scan_in_progress(void);

/* Convenience: try connecting to a specific (SSID, password) pair
 * immediately, without writing it to cfg first. Useful for "Try this
 * network" before saving. */
esp_err_t wifi_manager_try_connect(const char *ssid, const char *password);

/* Snapshot the current state. Thread-safe; safe to call from any task
 * including LVGL event callbacks. */
void      wifi_manager_get_status(wifi_mgr_status_t *out);

#ifdef __cplusplus
}
#endif
