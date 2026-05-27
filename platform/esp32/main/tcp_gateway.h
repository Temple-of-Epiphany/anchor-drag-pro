/*
 * TCP NMEA-0183 gateway client.
 *
 * Opens a TCP socket to a configured host:port, reads CRLF-delimited
 * NMEA 0183 sentences, parses each via nmea0183.h, and pushes the
 * result into gps_source. Reconnects on disconnect with backoff.
 *
 * v0.2 hardcodes the host:port for the bench-test path; once #63 +
 * the [gps.url] config is fully wired, host:port comes from
 * cfg.gps.url.url.
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Start the gateway client task. The task waits for WiFi to come up
 * (polls wifi_manager_get_status) before attempting the first connect.
 * Idempotent: safe to call once at boot. */
esp_err_t tcp_gateway_start(const char *host, int port);

/* Snapshot status (for the UI). */
typedef struct {
    bool     connected;
    char     host[64];
    int      port;
    uint32_t sentences_in;
    uint32_t bad_checksums;
    uint32_t reconnects;
} tcp_gateway_status_t;

void     tcp_gateway_get_status(tcp_gateway_status_t *out);

#ifdef __cplusplus
}
#endif

#include <stdbool.h>
#include <stdint.h>
