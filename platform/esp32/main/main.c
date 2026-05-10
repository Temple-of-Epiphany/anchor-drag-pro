/*
 * Anchor Drag Pro — ESP32 firmware
 * v0.2 clean rebuild starting point.
 *
 * This is a minimal scaffold. Hardware drivers, UI, GPS, algorithm, and
 * everything else gets built up in phases per the project board:
 * https://github.com/orgs/Temple-of-Epiphany/projects/19
 *
 * For the v0.1 codebase (full 6-screen LVGL implementation), see tag
 * v0.1.0-archive or branch archive/v0.1.
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#include <stdio.h>
#include "esp_log.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

static const char *TAG = "anchor_drag_pro";

/*
 * Firmware version. Manual bump per release.
 * Phase 0: 0.2.0-dev — minimal scaffold, no features
 */
#define FIRMWARE_VERSION_MAJOR  0
#define FIRMWARE_VERSION_MINOR  2
#define FIRMWARE_VERSION_PATCH  0
#define FIRMWARE_VERSION_TAG    "dev"
#define FIRMWARE_VERSION_STRING "0.2.0-dev"

static void log_chip_info(void)
{
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    uint32_t flash_size_bytes = 0;
    esp_flash_get_size(NULL, &flash_size_bytes);

    ESP_LOGI(TAG, "Anchor Drag Pro firmware v%s", FIRMWARE_VERSION_STRING);
    ESP_LOGI(TAG, "Chip: %s rev v%d.%d, %d cores, %d MB %s flash",
             CONFIG_IDF_TARGET,
             chip_info.revision / 100, chip_info.revision % 100,
             chip_info.cores,
             flash_size_bytes / (1024 * 1024),
             (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
    ESP_LOGI(TAG, "Free heap: %lu bytes", (unsigned long) esp_get_free_heap_size());
}

void app_main(void)
{
    log_chip_info();

    ESP_LOGI(TAG, "Phase 0 scaffold running. Heartbeat every 10s.");

    uint32_t tick = 0;
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        ESP_LOGI(TAG, "heartbeat tick=%lu free_heap=%lu",
                 (unsigned long) ++tick,
                 (unsigned long) esp_get_free_heap_size());
    }
}
