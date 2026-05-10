/*
 * Anchor Drag Pro — ESP32 firmware
 * v0.2 clean rebuild — Phase 2 (OTA-first development).
 *
 * Currently: Phase 2A — I2C0 bus init + mutex + boot-time scan.
 * Next: CH422G driver, then SD driver, then OTA from SD.
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

#include "i2c_bus.h"

static const char *TAG = "anchor_drag_pro";

/*
 * Firmware version. Manual bump per release.
 * Phase 2A: 0.2.0-dev — I2C bus + mutex
 */
#define FIRMWARE_VERSION_MAJOR  0
#define FIRMWARE_VERSION_MINOR  2
#define FIRMWARE_VERSION_PATCH  0
#define FIRMWARE_VERSION_TAG    "dev"
#define FIRMWARE_VERSION_STRING "0.2.0-dev"

/*
 * WaveShare ESP32-S3-Touch-LCD-4.3B I2C0 wiring.
 * All on-board peripherals share this bus; per-device clocks set in
 * each driver via i2c_master_bus_add_device().
 */
#define I2C0_SDA_GPIO   8
#define I2C0_SCL_GPIO   9
#define I2C0_FREQ_HZ    400000

static void log_chip_info(void)
{
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    uint32_t flash_size_bytes = 0;
    esp_flash_get_size(NULL, &flash_size_bytes);

    ESP_LOGI(TAG, "Anchor Drag Pro firmware v%s", FIRMWARE_VERSION_STRING);
    ESP_LOGI(TAG, "Chip: %s rev v%d.%d, %d cores, %lu MB %s flash",
             CONFIG_IDF_TARGET,
             chip_info.revision / 100, chip_info.revision % 100,
             chip_info.cores,
             (unsigned long) (flash_size_bytes / (1024 * 1024)),
             (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
    ESP_LOGI(TAG, "Free heap: %lu bytes", (unsigned long) esp_get_free_heap_size());
}

void app_main(void)
{
    log_chip_info();

    /* Phase 2A: bring up the shared I2C0 bus and scan for devices. */
    ESP_LOGI(TAG, "--- Phase 2A: I2C bus init ---");
    esp_err_t err = i2c_bus_init(I2C0_SDA_GPIO, I2C0_SCL_GPIO, I2C0_FREQ_HZ);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_bus_init failed: %s — continuing without I2C",
                 esp_err_to_name(err));
    } else {
        i2c_bus_scan();
    }

    ESP_LOGI(TAG, "--- Boot complete — heartbeat every 10s ---");

    uint32_t tick = 0;
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        ESP_LOGI(TAG, "heartbeat tick=%lu free_heap=%lu",
                 (unsigned long) ++tick,
                 (unsigned long) esp_get_free_heap_size());
    }
}
