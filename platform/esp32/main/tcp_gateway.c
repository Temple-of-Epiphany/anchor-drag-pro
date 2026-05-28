/*
 * TCP NMEA-0183 gateway client — implementation.
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#include "tcp_gateway.h"
#include "nmea0183.h"
#include "gps_source.h"
#include "wifi_manager.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include <string.h>
#include <errno.h>

static const char *TAG = "tcp_gw";

#define RX_BUF_SZ        512
#define LINE_BUF_SZ      256
#define INITIAL_RETRY_MS 2000
#define MAX_RETRY_MS     30000

static SemaphoreHandle_t s_mtx        = NULL;
static tcp_gateway_status_t s_status  = { 0 };
static TaskHandle_t      s_task       = NULL;
static char              s_host[64]   = { 0 };
static int               s_port       = 0;

static void inc_counter(uint32_t *p)
{
    if (xSemaphoreTake(s_mtx, portMAX_DELAY) == pdTRUE) {
        (*p)++;
        xSemaphoreGive(s_mtx);
    }
}

static void set_connected(bool b)
{
    if (xSemaphoreTake(s_mtx, portMAX_DELAY) == pdTRUE) {
        s_status.connected = b;
        xSemaphoreGive(s_mtx);
    }
}

/* Process one complete line (no trailing CR/LF). */
static void on_line(const char *line)
{
    if (line[0] != '$') return;            /* skip non-NMEA noise */
    nmea_fields_t f = { 0 };
    nmea_sentence_t r = nmea0183_parse(line, &f);
    if (r == NMEA_BAD_CHECKSUM) {
        inc_counter(&s_status.bad_checksums);
        ESP_LOGW(TAG, "bad checksum: %s", line);
        return;
    }
    if (r == NMEA_UNKNOWN) {
        ESP_LOGD(TAG, "unknown sentence: %s", line);
        return;
    }
    inc_counter(&s_status.sentences_in);
    gps_source_ingest_nmea(GPS_SRC_URL, &f);

    /* Light logging — every 20th sentence so we know data is flowing
     * without flooding the serial console. */
    static uint32_t count = 0;
    if ((++count % 20) == 1) {
        gps_fix_t g;
        gps_source_get(&g);
        ESP_LOGI(TAG, "sentence #%lu  pos=%s  lat=%.6f lon=%.6f  sog=%.1f hdg=%.1f",
                 (unsigned long) count,
                 g.pos_valid ? "ok" : "—",
                 g.latitude, g.longitude,
                 g.sog_valid ? g.sog_kts : 0.0,
                 g.heading_valid ? g.heading_deg : 0.0);
    }
}

/* Connect, read until disconnect, then return. */
static void run_session(void)
{
    /* Resolve. */
    struct addrinfo hints = { 0 };
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    char port_str[8];
    snprintf(port_str, sizeof(port_str), "%d", s_port);
    struct addrinfo *res = NULL;
    int rc = getaddrinfo(s_host, port_str, &hints, &res);
    if (rc != 0 || !res) {
        ESP_LOGW(TAG, "getaddrinfo(%s:%d) failed: rc=%d", s_host, s_port, rc);
        return;
    }

    int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0) {
        ESP_LOGW(TAG, "socket() failed: errno=%d", errno);
        freeaddrinfo(res);
        return;
    }

    /* Connect with a finite timeout (lwip default is generous). */
    struct timeval tv = { .tv_sec = 5, .tv_usec = 0 };
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    rc = connect(sock, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);
    if (rc != 0) {
        ESP_LOGW(TAG, "connect(%s:%d) failed: errno=%d", s_host, s_port, errno);
        close(sock);
        return;
    }

    ESP_LOGI(TAG, "connected to %s:%d", s_host, s_port);
    set_connected(true);

    /* Receive loop — accumulate bytes into a line buffer, splitting
     * on \n. Sentences end with "\r\n"; the \r is tolerated. */
    char rx[RX_BUF_SZ];
    char line[LINE_BUF_SZ];
    size_t line_len = 0;
    while (1) {
        ssize_t n = recv(sock, rx, sizeof(rx), 0);
        if (n <= 0) {
            if (n == 0) {
                ESP_LOGW(TAG, "peer closed");
            } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                /* Timeout — peer is quiet but the link is still up.
                 * Continue (in real bench, the sim sends ~2 Hz so we
                 * never hit this; keep the path defensive). */
                continue;
            } else {
                ESP_LOGW(TAG, "recv error: errno=%d", errno);
            }
            break;
        }
        for (ssize_t i = 0; i < n; i++) {
            char c = rx[i];
            if (c == '\n') {
                /* Strip optional trailing \r. */
                if (line_len > 0 && line[line_len - 1] == '\r') line_len--;
                line[line_len] = '\0';
                if (line_len > 0) on_line(line);
                line_len = 0;
            } else if (line_len < sizeof(line) - 1) {
                line[line_len++] = c;
            } else {
                /* overflow — drop the partial line and resync at next \n */
                line_len = 0;
            }
        }
    }

    set_connected(false);
    close(sock);
}

/* Wait until WiFi has an IP. */
static void wait_for_wifi(void)
{
    while (1) {
        wifi_mgr_status_t st;
        wifi_manager_get_status(&st);
        if (st.state == WIFI_MGR_CONNECTED && st.ip[0]) return;
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

static void gateway_task(void *arg)
{
    (void) arg;
    uint32_t retry_ms = INITIAL_RETRY_MS;

    while (1) {
        wait_for_wifi();
        run_session();
        inc_counter(&s_status.reconnects);
        ESP_LOGI(TAG, "session ended; sleeping %lu ms then retrying",
                 (unsigned long) retry_ms);
        vTaskDelay(pdMS_TO_TICKS(retry_ms));
        retry_ms = (retry_ms * 2 > MAX_RETRY_MS) ? MAX_RETRY_MS : retry_ms * 2;
    }
}

esp_err_t tcp_gateway_start(const char *host, int port)
{
    if (!host || !*host || port <= 0) return ESP_ERR_INVALID_ARG;
    if (!s_mtx) s_mtx = xSemaphoreCreateMutex();
    snprintf(s_host, sizeof(s_host), "%s", host);
    s_port = port;
    if (xSemaphoreTake(s_mtx, portMAX_DELAY) == pdTRUE) {
        snprintf(s_status.host, sizeof(s_status.host), "%s", host);
        s_status.port = port;
        xSemaphoreGive(s_mtx);
    }
    if (s_task) return ESP_OK;
    /* 6 KB stack: the full ingest chain is gateway_task -> on_line ->
     * gps_source_ingest_nmea -> main_on_gps_fix -> anchor_state_on_fix,
     * plus nmea0183 parser locals and lwip recv buffers. 4 KB sat
     * uncomfortably close to the canary; 6 KB gives meaningful margin. */
    BaseType_t r = xTaskCreatePinnedToCore(gateway_task, "tcp_gw",
                                            6144, NULL, 4, &s_task, 0);
    return (r == pdPASS) ? ESP_OK : ESP_FAIL;
}

void tcp_gateway_get_status(tcp_gateway_status_t *out)
{
    if (!out) return;
    memset(out, 0, sizeof(*out));
    if (s_mtx && xSemaphoreTake(s_mtx, portMAX_DELAY) == pdTRUE) {
        *out = s_status;
        xSemaphoreGive(s_mtx);
    }
}
