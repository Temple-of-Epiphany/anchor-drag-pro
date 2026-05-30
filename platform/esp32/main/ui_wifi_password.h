/*
 * Modal: WiFi password entry — on-screen keyboard.
 *
 * Spec: https://github.com/Temple-of-Epiphany/anchor-drag-pro/wiki/Information-Architecture
 *
 * Centred modal with the SSID (read-only) at the top, a password
 * text area, a Show / Hide toggle, an on-screen lv_keyboard, and
 * Cancel / Connect buttons. On Connect, fires the callback with the
 * SSID + entered password. Caller is responsible for persisting and
 * triggering wifi_manager_try_connect.
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#pragma once

#include "ui_modals.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*ui_wifi_password_cb)(const char *ssid, const char *password,
                                     void *user_data);

typedef struct {
    const char          *ssid;           /* required, read-only display */
    bool                 secured;        /* if false, no password is required */
    ui_wifi_password_cb  on_connect;
    void                *user_data;
} ui_wifi_password_params_t;

ui_modal_handle_t *ui_wifi_password_show(const ui_wifi_password_params_t *p);

#ifdef __cplusplus
}
#endif
