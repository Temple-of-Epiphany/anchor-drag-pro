/*
 * N2K / TWAI listener — minimum-viable receive path for issue #83.
 *
 * LISTEN_ONLY mode at NMEA-2000's 250 kbps. The receive task pulls
 * frames in a loop, logs each one at INFO during dev, and updates
 * a small status snapshot (frame count, last-frame timestamp, last
 * 11/29-bit ID) that screen_connections + screen_diagnostics can
 * surface.
 *
 * Out of scope for this slice (tracked in #83 / #34 / #54):
 *   - fast-packet reassembly
 *   - PGN decoding (no PGN 129029 -> gps_source yet)
 *   - transmit path (LISTEN_ONLY is enforced — never drives the bus)
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

typedef struct {
    bool      started;            /* twai_start succeeded */
    uint32_t  frames_total;       /* total frames received since start */
    uint32_t  frames_per_sec;     /* rolling fps, updated ~1 Hz */
    uint64_t  last_frame_us;      /* esp_timer_get_time() of latest rx */
    uint32_t  last_frame_id;      /* 29-bit extended ID of latest rx */
    uint32_t  last_frame_pgn;     /* decoded PGN of latest rx */
    uint8_t   last_frame_src;     /* source address of latest rx */
    uint8_t   last_frame_prio;    /* priority 0..7 (0 = highest) */
    uint8_t   last_frame_dlc;     /* data length, 0..8 */
    uint8_t   last_frame_data[8]; /* raw payload of latest rx */
    uint16_t  distinct_pgns_seen; /* count of unique PGNs since start */
    uint16_t  distinct_srcs_seen; /* count of unique source addresses */
    uint32_t  bus_off_count;      /* TWAI bus-off recoveries since start */
    uint32_t  rx_error_count;     /* twai_receive non-OK returns */
} n2k_listener_status_t;

/* Bring up the TWAI driver in LISTEN_ONLY mode and start the rx task.
 * Idempotent. Toggles the CH422G USB/CAN mux to CAN mode. Returns:
 *   ESP_OK                — running (or already running)
 *   ESP_ERR_INVALID_STATE — already started
 *   anything else         — propagate from twai_driver_install/start */
esp_err_t n2k_listener_init(void);

/* Snapshot current state. Thread-safe; caller-owned `out`. */
void n2k_listener_get_status(n2k_listener_status_t *out);

/* True if at least one frame has been received in the last `max_age_ms`. */
bool n2k_listener_is_fresh(uint32_t max_age_ms);

#ifdef __cplusplus
}
#endif
