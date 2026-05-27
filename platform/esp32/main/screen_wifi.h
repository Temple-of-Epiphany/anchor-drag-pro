/*
 * WiFi screen — on-device WiFi configuration.
 *
 * Reached from Settings → "STA networks" row (formerly web-only).
 * Shows current connection status, saved networks (cfg.wifi.sta_networks),
 * a "Scan for networks" button, and a list of nearby APs.
 * Tap a network → ui_wifi_password_show → wifi_manager_try_connect →
 * (on success) persist via anchor_config_save_nvs.
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

lv_obj_t *screen_wifi_create(void);

/* Refresh the live status block (current SSID / RSSI / IP). Cheap to
 * call from a timer; usually triggered when the screen is visible. */
void screen_wifi_refresh_status(lv_obj_t *screen);

/* Trigger an async scan + redraw the AP list when complete. */
void screen_wifi_trigger_scan(lv_obj_t *screen);

#ifdef __cplusplus
}
#endif
