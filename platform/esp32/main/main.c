/*
 * Anchor Drag Pro — ESP32 firmware
 * v0.2 clean rebuild — Phase 2D (OTA from SD).
 *
 * Boot sequence:
 *   1. Log chip info
 *   2. If running partition is PENDING_VERIFY (just OTA'd), run brief
 *      self-test then mark-valid (or roll back on failure).
 *   3. Bring up I2C0 + scan
 *   4. Init CH422G I/O expander (smoke test: blink backlight)
 *   5. Init SD card (assert CS via CH422G, mount FAT)
 *   6. Check /sdcard/firmware/ for a newer .bin + .sha256 — prompt
 *      user via serial, verify, flash, reboot.
 *   7. If still running, heartbeat every 10s.
 *
 * For the v0.1 codebase (full 6-screen LVGL implementation), see tag
 * v0.1.0-archive or branch archive/v0.1.
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#include <stdio.h>
#include <inttypes.h>
#include "esp_log.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#include "nvs_flash.h"
#include "board_config.h"
#include "i2c_bus.h"
#include "ch422g.h"
#include "sd_card.h"
#include "ota.h"
#include "anchor_config.h"

static const char *TAG = "anchor_drag_pro";

/* Phase 2D: 0.2.0-dev — I2C + CH422G + SD + OTA */
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
    ESP_LOGI(TAG, "CH422G smoke test: backlight blink");
    for (int i = 0; i < 2; i++) {
        ch422g_lcd_backlight(true);
        vTaskDelay(pdMS_TO_TICKS(150));
        ch422g_lcd_backlight(false);
        vTaskDelay(pdMS_TO_TICKS(150));
    }
    ESP_LOGI(TAG, "CH422G state after test: 0x%02X", ch422g_get_state_cached());
}

static void sd_smoke_test(void)
{
    if (!sd_card_is_mounted()) {
        ESP_LOGW(TAG, "SD smoke test: card not mounted, skipping");
        return;
    }
    ESP_LOGI(TAG, "SD card info:");
    sd_card_print_info(stdout);

    uint64_t total = 0, free = 0, used = 0;
    if (sd_card_get_usage(&total, &free, &used) == ESP_OK) {
        ESP_LOGI(TAG, "SD usage: total=%llu MB  used=%llu MB  free=%llu MB (%.1f%% free)",
                 total / (1024ULL * 1024ULL),
                 used  / (1024ULL * 1024ULL),
                 free  / (1024ULL * 1024ULL),
                 total > 0 ? 100.0 * (double) free / (double) total : 0.0);
    }
}

void app_main(void)
{
    log_chip_info();

    /* NVS — required by anchor_config (cache layer) and other future
     * subsystems. Initialize once early; ignore "not erased" / "new
     * version" errors that normally just mean we should erase + re-init. */
    esp_err_t nvs_err = nvs_flash_init();
    if (nvs_err == ESP_ERR_NVS_NO_FREE_PAGES || nvs_err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS needs erase (%s) — erasing and re-init", esp_err_to_name(nvs_err));
        nvs_flash_erase();
        nvs_flash_init();
    }

    /* Phase 2D self-test: if we just OTA'd, run a brief self-test then
     * mark-valid. If the previous boot was already VALID this is a no-op. */
    ota_handle_pending_verify();
    ota_log_status();

    ESP_LOGI(TAG, "--- Phase 2A: I2C0 bus init ---");
    if (i2c_bus_init(I2C0_SDA_GPIO, I2C0_SCL_GPIO, I2C0_FREQ_HZ) != ESP_OK) {
        ESP_LOGE(TAG, "i2c_bus_init failed; halting before peripheral init");
        return;
    }
    i2c_bus_scan();

    ESP_LOGI(TAG, "--- Phase 2B: CH422G I/O expander init ---");
    if (ch422g_init() != ESP_OK) {
        ESP_LOGE(TAG, "ch422g_init failed; SD and display will be unavailable");
    } else {
        ch422g_smoke_test();
    }

    ESP_LOGI(TAG, "--- Phase 2C: SD card init ---");
    esp_err_t sd_err = sd_card_init();
    if (sd_err != ESP_OK) {
        ESP_LOGW(TAG, "sd_card_init failed: %s — proceeding without SD",
                 esp_err_to_name(sd_err));
    } else {
        sd_smoke_test();
    }

    ESP_LOGI(TAG, "--- Phase 2D: OTA check on SD ---");
    /* If a newer firmware is on /sdcard/firmware/, prompt + flash + reboot.
     * On no-update / decline / error, returns and continues boot. */
    ota_check_and_apply_from_sd();

    ESP_LOGI(TAG, "--- Phase 3: load configuration ---");
    /* Three-tier load: SD config.toml -> NVS cache -> built-in defaults.
     * Always returns OK; device must always boot with some config. */
    static anchor_config_t g_config;
    anchor_config_load(&g_config);
    anchor_config_log(&g_config);

    ESP_LOGI(TAG, "--- Boot complete — heartbeat every 10s ---");

    uint32_t tick = 0;
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        ESP_LOGI(TAG, "heartbeat tick=%lu free_heap=%lu",
                 (unsigned long) ++tick,
                 (unsigned long) esp_get_free_heap_size());
    }
}
