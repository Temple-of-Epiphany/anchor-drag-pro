/*
 * N2K / TWAI listener — implementation (issue #83 minimum slice).
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#include "n2k_listener.h"
#include "board_config.h"
#include "ch422g.h"
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

        window_count++;
        if (xSemaphoreTake(s_mtx, portMAX_DELAY) == pdTRUE) {
            s_status.frames_total++;
            s_status.last_frame_us  = now_us;
            s_status.last_frame_id  = msg.identifier;
            s_status.last_frame_dlc = msg.data_length_code;
            memcpy(s_status.last_frame_data, msg.data,
                   msg.data_length_code <= 8 ? msg.data_length_code : 8);
            if ((now_us - window_start_us) >= 1000000ULL) {
                s_status.frames_per_sec = window_count;
                window_start_us = now_us;
                window_count    = 0;
            }
            xSemaphoreGive(s_mtx);
        }

        /* Throttled log so a 200 fps bus doesn't drown the console.
         * The screen_diagnostics PGN monitor (future) will surface the
         * full stream instead. */
        if (++log_n % LOG_THROTTLE_N == 0) {
            char hex[3 * 8 + 1] = { 0 };
            for (int i = 0; i < msg.data_length_code && i < 8; i++) {
                snprintf(hex + i * 3, sizeof(hex) - i * 3, "%02X ", msg.data[i]);
            }
            ESP_LOGI(TAG, "rx id=0x%08lX %s dlc=%u  data=%s",
                     (unsigned long) msg.identifier,
                     (msg.flags & TWAI_MSG_FLAG_EXTD) ? "EXT" : "STD",
                     (unsigned) msg.data_length_code, hex);
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
