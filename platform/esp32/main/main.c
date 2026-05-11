/*
 * Anchor Drag Pro — ESP32 firmware
 * v0.2 clean rebuild — Phase 2B (CH422G I/O expander).
 *
 * Boot sequence so far:
 *   1. Log chip info
 *   2. Bring up I2C0 + scan for devices
 *   3. Init CH422G I/O expander
 *   4. Smoke test: blink backlight twice to confirm CH422G is responding
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

#include "board_config.h"
#include "i2c_bus.h"
#include "ch422g.h"

static const char *TAG = "anchor_drag_pro";

/*
 * Firmware version. Manual bump per release.
 * Phase 2B: 0.2.0-dev — I2C bus + CH422G driver
 */
#define FIRMWARE_VERSION_STRING "0.2.0-dev"

static void log_chip_info(void)
{
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    uint32_t flash_size_bytes = 0;
    esp_flash_get_size(NULL, &flash_size_bytes);

    ESP_LOGI(TAG, "Anchor Drag Pro firmware v%s", FIRMWARE_VERSION_STRING);
    ESP_LOGI(TAG, "Board: %s", BOARD_NAME);
    ESP_LOGI(TAG, "MCU:   %s rev v%d.%d, %d cores, %lu MB flash",
             BOARD_MCU,
             chip_info.revision / 100, chip_info.revision % 100,
             chip_info.cores,
             (unsigned long) (flash_size_bytes / (1024 * 1024)));
    ESP_LOGI(TAG, "Free heap: %lu bytes", (unsigned long) esp_get_free_heap_size());
}

static void ch422g_smoke_test(void)
{
    /*
     * Blink the LCD backlight twice (off-on-off-on) so we can visually
     * confirm the I/O expander is talking to the chip. After the test,
     * leave the backlight OFF — the display driver will turn it on when
     * it's ready (later phase).
     */
    ESP_LOGI(TAG, "CH422G smoke test: backlight blink");
    for (int i = 0; i < 2; i++) {
        ch422g_lcd_backlight(true);
        vTaskDelay(pdMS_TO_TICKS(150));
        ch422g_lcd_backlight(false);
        vTaskDelay(pdMS_TO_TICKS(150));
    }
    ESP_LOGI(TAG, "CH422G state after test: 0x%02X", ch422g_get_state_cached());
}

void app_main(void)
{
    log_chip_info();

    ESP_LOGI(TAG, "--- Phase 2A: I2C0 bus init ---");
    if (i2c_bus_init(I2C0_SDA_GPIO, I2C0_SCL_GPIO, I2C0_FREQ_HZ) != ESP_OK) {
        ESP_LOGE(TAG, "i2c_bus_init failed; halting before peripheral init");
        return;
    }
    i2c_bus_scan();

    ESP_LOGI(TAG, "--- Phase 2B: CH422G I/O expander init ---");
    if (ch422g_init() != ESP_OK) {
        ESP_LOGE(TAG, "ch422g_init failed; subsequent peripherals (display, SD) "
                      "will not work");
    } else {
        ch422g_smoke_test();
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
