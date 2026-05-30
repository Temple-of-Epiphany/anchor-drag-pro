/*
 * N2K / TWAI listener — implementation (issue #83 minimum slice).
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#include "n2k_listener.h"
#include "board_config.h"
#include "ch422g.h"
#include "gps_source.h"
#include "nmea0183.h"
#include "driver/twai.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <string.h>

static const char *TAG = "n2k_lis";

#define RX_TASK_STACK   4096
#define RX_TASK_PRIO    8
#define RX_TASK_CORE    0
#define LOG_THROTTLE_N  20   /* INFO-log every Nth frame to avoid flooding */

static n2k_listener_status_t  s_status   = { 0 };
static SemaphoreHandle_t      s_mtx      = NULL;
static TaskHandle_t           s_rx_task  = NULL;
static volatile bool          s_running  = false;

/* Bitmaps for distinct-talker / distinct-PGN counters. 256 source
 * addresses fit in 32 bytes; PGNs are 18-bit but we only track the
 * low 16 (top bits are reserved/data-page and rarely vary for the
 * PGNs a marine app sees). */
static uint8_t  s_src_seen[32];        /* 256 bits */
static uint8_t  s_pgn_seen_low[8192];  /* 65536 bits */

static inline void bitmap_set(uint8_t *bm, uint16_t bit) {
    bm[bit >> 3] |= (uint8_t)(1u << (bit & 7));
}
static inline bool bitmap_test(const uint8_t *bm, uint16_t bit) {
    return (bm[bit >> 3] & (uint8_t)(1u << (bit & 7))) != 0;
}

/* Decode a 29-bit J1939 / N2K identifier into priority + PGN + src.
 * PF < 240 -> PDU1 (PGN = DP|PF|00, destination = PS)
 * PF >= 240 -> PDU2 (PGN = DP|PF|PS, broadcast)
 * See ISO 11783-3 / NMEA-2000 §2.4. */
static void decode_id(uint32_t id, uint8_t *prio, uint32_t *pgn, uint8_t *src)
{
    *prio = (uint8_t)((id >> 26) & 0x7);
    *src  = (uint8_t)(id & 0xFF);
    uint8_t dp = (uint8_t)((id >> 24) & 0x1);
    uint8_t pf = (uint8_t)((id >> 16) & 0xFF);
    uint8_t ps = (uint8_t)((id >> 8)  & 0xFF);
    if (pf < 240) {
        *pgn = ((uint32_t) dp << 16) | ((uint32_t) pf << 8);
    } else {
        *pgn = ((uint32_t) dp << 16) | ((uint32_t) pf << 8) | ps;
    }
}

/* ---- PGN decoders (single-frame only for now; fast-packet lands in
 * #54 alongside PGN 129029).
 *
 * PGN 129025 — Position, Rapid Update  (8 bytes, ~10 Hz)
 *   bytes 0..3 : latitude  (int32 LE, units 1e-7 deg, signed)
 *   bytes 4..7 : longitude (int32 LE, units 1e-7 deg, signed)
 *
 * PGN 129026 — COG/SOG, Rapid Update   (8 bytes, ~4 Hz)
 *   byte   0    : SID
 *   byte   1    : COG reference (lower 2 bits: 0=true, 1=magnetic)
 *   bytes 2..3  : COG (uint16 LE, units 1e-4 rad)
 *   bytes 4..5  : SOG (uint16 LE, units 0.01 m/s)
 *   bytes 6..7  : reserved
 *
 * Per NMEA-2000 § Table 6, "not available" sentinels are 0x7FFFFFFF
 * for signed int32 and 0xFFFF for unsigned int16. */

#define N2K_INT32_NA    (int32_t)0x7FFFFFFF
#define N2K_UINT16_NA   (uint16_t)0xFFFF

#define N2K_RAD_TO_DEG  (180.0 / 3.14159265358979323846)
#define N2K_MS_TO_KTS   1.943844

static int32_t  rd_le_i32(const uint8_t *p) {
    return (int32_t)((uint32_t) p[0]        |
                     ((uint32_t) p[1] << 8) |
                     ((uint32_t) p[2] << 16)|
                     ((uint32_t) p[3] << 24));
}
static uint16_t rd_le_u16(const uint8_t *p) {
    return (uint16_t)((uint16_t) p[0] | ((uint16_t) p[1] << 8));
}

static void handle_pgn_129025(const uint8_t *d, uint8_t dlc)
{
    if (dlc < 8) return;
    int32_t lat_i = rd_le_i32(d + 0);
    int32_t lon_i = rd_le_i32(d + 4);
    if (lat_i == N2K_INT32_NA || lon_i == N2K_INT32_NA) return;

    nmea_fields_t f = { 0 };
    f.type      = NMEA_RMC;       /* synthetic — only pos_valid matters */
    f.pos_valid = true;
    f.latitude  = (double) lat_i * 1e-7;
    f.longitude = (double) lon_i * 1e-7;
    gps_source_ingest_nmea(GPS_SRC_N2K, &f);
}

static void handle_pgn_129026(const uint8_t *d, uint8_t dlc)
{
    if (dlc < 6) return;
    uint16_t cog_raw = rd_le_u16(d + 2);
    uint16_t sog_raw = rd_le_u16(d + 4);
    bool cog_is_true = ((d[1] & 0x03) == 0);

    nmea_fields_t f = { 0 };
    f.type = NMEA_VTG;
    if (sog_raw != N2K_UINT16_NA) {
        f.sog_valid = true;
        f.sog_kts   = (double) sog_raw * 0.01 * N2K_MS_TO_KTS;
    }
    if (cog_raw != N2K_UINT16_NA) {
        double cog_deg = (double) cog_raw * 1e-4 * N2K_RAD_TO_DEG;
        if (cog_deg < 0)   cog_deg += 360.0;
        if (cog_deg > 360) cog_deg -= 360.0;
        f.cog_valid = true;
        f.cog_deg   = cog_deg;
        (void) cog_is_true;  /* gps_source carries COG as a single value */
    }
    if (f.sog_valid || f.cog_valid) {
        gps_source_ingest_nmea(GPS_SRC_N2K, &f);
    }
}

static void rx_task(void *arg)
{
    (void) arg;
    uint64_t window_start_us = (uint64_t) esp_timer_get_time();
    uint32_t window_count    = 0;
    uint32_t log_n           = 0;

    while (s_running) {
        twai_message_t msg;
        esp_err_t err = twai_receive(&msg, pdMS_TO_TICKS(1000));
        uint64_t now_us = (uint64_t) esp_timer_get_time();

        if (err == ESP_ERR_TIMEOUT) {
            /* No frames in the last second — still update fps window so
             * a dead bus reads 0 fps instead of a stale number. */
            if (xSemaphoreTake(s_mtx, portMAX_DELAY) == pdTRUE) {
                s_status.frames_per_sec = 0;
                xSemaphoreGive(s_mtx);
            }
            window_start_us = now_us;
            window_count    = 0;
            continue;
        }
        if (err != ESP_OK) {
            if (xSemaphoreTake(s_mtx, portMAX_DELAY) == pdTRUE) {
                s_status.rx_error_count++;
                xSemaphoreGive(s_mtx);
            }
            ESP_LOGW(TAG, "twai_receive: %s", esp_err_to_name(err));
            continue;
        }

        uint8_t  prio = 0, src = 0;
        uint32_t pgn  = 0;
        if (msg.flags & TWAI_MSG_FLAG_EXTD) {
            decode_id(msg.identifier, &prio, &pgn, &src);

            /* Decode the PGNs we know — single-frame only for now.
             * Fast-packet (PGN 129029 etc.) lands with #54. */
            switch (pgn) {
                case 129025: handle_pgn_129025(msg.data, msg.data_length_code); break;
                case 129026: handle_pgn_129026(msg.data, msg.data_length_code); break;
                default: break;
            }
        }

        window_count++;
        bool first_pgn = false, first_src = false;
        if (xSemaphoreTake(s_mtx, portMAX_DELAY) == pdTRUE) {
            s_status.frames_total++;
            s_status.last_frame_us   = now_us;
            s_status.last_frame_id   = msg.identifier;
            s_status.last_frame_pgn  = pgn;
            s_status.last_frame_src  = src;
            s_status.last_frame_prio = prio;
            s_status.last_frame_dlc  = msg.data_length_code;
            memcpy(s_status.last_frame_data, msg.data,
                   msg.data_length_code <= 8 ? msg.data_length_code : 8);
            if (msg.flags & TWAI_MSG_FLAG_EXTD) {
                uint16_t pgn_lo = (uint16_t)(pgn & 0xFFFF);
                if (!bitmap_test(s_pgn_seen_low, pgn_lo)) {
                    bitmap_set(s_pgn_seen_low, pgn_lo);
                    s_status.distinct_pgns_seen++;
                    first_pgn = true;
                }
                if (!bitmap_test(s_src_seen, src)) {
                    bitmap_set(s_src_seen, src);
                    s_status.distinct_srcs_seen++;
                    first_src = true;
                }
            }
            if ((now_us - window_start_us) >= 1000000ULL) {
                s_status.frames_per_sec = window_count;
                window_start_us = now_us;
                window_count    = 0;
            }
            xSemaphoreGive(s_mtx);
        }

        /* First-sighting events always log — useful for understanding
         * what's on the bus during bring-up. Subsequent frames are
         * throttled (~1 in N) so a 200 fps bus doesn't drown the
         * console. The screen_diagnostics PGN monitor (future) will
         * surface the full stream instead. */
        bool should_log = first_pgn || first_src ||
                          ((++log_n % LOG_THROTTLE_N) == 0);
        if (should_log) {
            char hex[3 * 8 + 1] = { 0 };
            for (int i = 0; i < msg.data_length_code && i < 8; i++) {
                snprintf(hex + i * 3, sizeof(hex) - i * 3, "%02X ", msg.data[i]);
            }
            if (msg.flags & TWAI_MSG_FLAG_EXTD) {
                ESP_LOGI(TAG, "rx PGN %lu  src=%u  prio=%u  dlc=%u  data=%s%s%s",
                         (unsigned long) pgn, (unsigned) src, (unsigned) prio,
                         (unsigned) msg.data_length_code, hex,
                         first_pgn ? " [new PGN]" : "",
                         first_src ? " [new src]" : "");
            } else {
                ESP_LOGI(TAG, "rx STD id=0x%03lX dlc=%u  data=%s",
                         (unsigned long) msg.identifier,
                         (unsigned) msg.data_length_code, hex);
            }
        }

        /* Bus-off recovery: poll status occasionally and kick recovery
         * if needed. The driver doesn't auto-recover in LISTEN_ONLY
         * but bus-off is rare on a healthy 250 kbps bus. */
        if ((s_status.frames_total & 0xFF) == 0) {
            twai_status_info_t info;
            if (twai_get_status_info(&info) == ESP_OK &&
                info.state == TWAI_STATE_BUS_OFF) {
                ESP_LOGW(TAG, "bus-off — initiating recovery");
                if (xSemaphoreTake(s_mtx, portMAX_DELAY) == pdTRUE) {
                    s_status.bus_off_count++;
                    xSemaphoreGive(s_mtx);
                }
                twai_initiate_recovery();
            }
        }
    }
    vTaskDelete(NULL);
}

esp_err_t n2k_listener_init(void)
{
    if (s_running) return ESP_ERR_INVALID_STATE;

    if (!s_mtx) s_mtx = xSemaphoreCreateMutex();
    if (!s_mtx) return ESP_ERR_NO_MEM;

    /* Switch the CH422G USB/CAN mux to CAN before driver install — the
     * RX pin is meaningless otherwise. */
    esp_err_t err = ch422g_usb_can_select(true);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ch422g_usb_can_select(can): %s", esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(TAG, "CH422G state after CAN-mux flip = 0x%02X (EXIO5=CAN expected)",
             ch422g_get_state_cached());

    twai_general_config_t g = TWAI_GENERAL_CONFIG_DEFAULT(
        TWAI_TX_GPIO, TWAI_RX_GPIO, TWAI_MODE_LISTEN_ONLY);
    /* LISTEN_ONLY is required for #40 (never disturb a customer bus
     * while we're in receive-only dev). */
    g.rx_queue_len = 32;

    twai_timing_config_t t = TWAI_TIMING_CONFIG_250KBITS();
    twai_filter_config_t f = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    err = twai_driver_install(&g, &t, &f);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "twai_driver_install: %s", esp_err_to_name(err));
        return err;
    }

    err = twai_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "twai_start: %s", esp_err_to_name(err));
        twai_driver_uninstall();
        return err;
    }

    s_running         = true;
    s_status.started  = true;

    BaseType_t r = xTaskCreatePinnedToCore(rx_task, "n2k_rx",
                                            RX_TASK_STACK, NULL,
                                            RX_TASK_PRIO, &s_rx_task,
                                            RX_TASK_CORE);
    if (r != pdPASS) {
        s_running = false;
        s_status.started = false;
        twai_stop();
        twai_driver_uninstall();
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "TWAI LISTEN_ONLY @ 250 kbps on TX=%d RX=%d (CH422G EXIO5=CAN)",
             TWAI_TX_GPIO, TWAI_RX_GPIO);
    return ESP_OK;
}

void n2k_listener_get_status(n2k_listener_status_t *out)
{
    if (!out) return;
    if (!s_mtx) { memset(out, 0, sizeof(*out)); return; }
    if (xSemaphoreTake(s_mtx, portMAX_DELAY) == pdTRUE) {
        *out = s_status;
        xSemaphoreGive(s_mtx);
    } else {
        memset(out, 0, sizeof(*out));
    }
}

bool n2k_listener_is_fresh(uint32_t max_age_ms)
{
    if (!s_running || s_status.last_frame_us == 0) return false;
    uint64_t now = (uint64_t) esp_timer_get_time();
    return ((now - s_status.last_frame_us) / 1000ULL) <= max_age_ms;
}
