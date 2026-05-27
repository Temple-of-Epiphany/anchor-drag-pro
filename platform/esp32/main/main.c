/*
 * Anchor Drag Pro — ESP32 firmware
 * Phase B (#69) — LVGL infrastructure + minimum-viable Splash.
 *
 * Boot sequence:
 *   1. Log chip info
 *   2. OTA pending-verify (mark valid or roll back)
 *   3. Bring up I2C0 + scan
 *   4. Init CH422G I/O expander
 *   5. Init SD card (mount FAT)
 *   6. OTA check on SD (auto-install if newer)
 *   7. Load configuration (SD -> NVS -> defaults, then mirror back)
 *   8. Bring up display + LVGL + touch reset
 *   9. Show Splash screen
 *  10. Heartbeat every 10 s (Splash stays on screen until subsequent
 *      screens land in follow-up issues)
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
#include "display_driver.h"
#include "touch_driver.h"
#include "lvgl_init.h"
#include "screen_splash.h"
#include "screen_monitor.h"
#include "screen_connections.h"
#include "screen_info.h"
#include "screen_settings.h"
#include "screen_diagnostics.h"
#include "screen_nav.h"
#include "wifi_manager.h"
#include "lvgl.h"

static const char *TAG = "anchor_drag_pro";

/* Phase B (#69) — LVGL infrastructure + Splash */
#define FIRMWARE_VERSION_STRING "0.2.19"

/* The active configuration — read by anchor_config_load(), then consumed
 * by other modules (e.g., screen_monitor reads g_config.anchor.options[]
 * to populate the Preset Picker). Declared at file scope so its address
 * is stable for the device's lifetime. */
anchor_config_t g_config;

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

/* Helper: push a splash row update if the splash is up. Pre-display
 * boot phases also pre-stage their results in a small cache so we can
 * replay them onto the splash once it appears. */
typedef struct {
    splash_row_status_t status;
    char                detail[64];
    bool                set;
} splash_row_cache_t;

static splash_row_cache_t s_row_cache[SPLASH_ROW_COUNT];
static bool               s_splash_up = false;

static void report_row(splash_row_id_t row, splash_row_status_t status,
                        const char *fmt, ...)
{
    /* Cache + log regardless of whether the splash is on screen. */
    s_row_cache[row].set    = true;
    s_row_cache[row].status = status;
    s_row_cache[row].detail[0] = '\0';
    if (fmt) {
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(s_row_cache[row].detail,
                  sizeof(s_row_cache[row].detail), fmt, ap);
        va_end(ap);
    }
    if (s_splash_up) {
        screen_splash_set_row(row, status, s_row_cache[row].detail);
    }
}

static void replay_rows(void)
{
    for (int i = 0; i < SPLASH_ROW_COUNT; i++) {
        if (s_row_cache[i].set) {
            screen_splash_set_row((splash_row_id_t) i,
                                   s_row_cache[i].status,
                                   s_row_cache[i].detail);
        }
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
    /* OTA partition row reflects the running partition state. */
    report_row(SPLASH_ROW_OTA, SPLASH_STATUS_PASS, "running app, VALID");

    ESP_LOGI(TAG, "--- Phase 2A: I2C0 bus init ---");
    if (i2c_bus_init(I2C0_SDA_GPIO, I2C0_SCL_GPIO, I2C0_FREQ_HZ) != ESP_OK) {
        ESP_LOGE(TAG, "i2c_bus_init failed; halting before peripheral init");
        report_row(SPLASH_ROW_I2C, SPLASH_STATUS_FAIL, "init failed");
        return;
    }
    i2c_bus_scan();
    report_row(SPLASH_ROW_I2C, SPLASH_STATUS_PASS, "%d Hz", (int) I2C0_FREQ_HZ);

    ESP_LOGI(TAG, "--- Phase 2B: CH422G I/O expander init ---");
    if (ch422g_init() != ESP_OK) {
        ESP_LOGE(TAG, "ch422g_init failed; SD and display will be unavailable");
        report_row(SPLASH_ROW_CH422G, SPLASH_STATUS_FAIL, "init failed");
    } else {
        ch422g_smoke_test();
        report_row(SPLASH_ROW_CH422G, SPLASH_STATUS_PASS, NULL);
    }

    /* RTC isn't actively probed yet; mark as skip until its driver lands. */
    report_row(SPLASH_ROW_RTC, SPLASH_STATUS_SKIP, "not implemented");

    ESP_LOGI(TAG, "--- Phase 2C: SD card init ---");
    esp_err_t sd_err = sd_card_init();
    if (sd_err != ESP_OK) {
        ESP_LOGW(TAG, "sd_card_init failed: %s — proceeding without SD",
                 esp_err_to_name(sd_err));
        report_row(SPLASH_ROW_SD, SPLASH_STATUS_FAIL,
                   "mount failed: %s", esp_err_to_name(sd_err));
    } else {
        sd_smoke_test();
        uint64_t total = 0, free = 0;
        if (sd_card_get_usage(&total, &free, NULL) == ESP_OK) {
            report_row(SPLASH_ROW_SD, SPLASH_STATUS_PASS,
                       "%llu MB free", free / (1024ULL * 1024ULL));
        } else {
            report_row(SPLASH_ROW_SD, SPLASH_STATUS_PASS, NULL);
        }
    }

    ESP_LOGI(TAG, "--- Phase 2D: OTA check on SD ---");
    /* If a newer firmware is on /sdcard/firmware/, prompt + flash + reboot.
     * On no-update / decline / error, returns and continues boot. */
    ota_check_and_apply_from_sd();

    ESP_LOGI(TAG, "--- Phase 3: load configuration ---");
    /* Three-tier load: SD config.toml -> NVS cache -> built-in defaults.
     * Always returns OK; device must always boot with some config. */
    anchor_config_load(&g_config);
    anchor_config_log(&g_config);
    report_row(SPLASH_ROW_CONFIG, SPLASH_STATUS_PASS,
               sd_card_is_mounted() ? "from SD" : "from NVS / defaults");

    /* WiFi — kick off the manager in the background. wifi_manager_start
     * returns immediately; scan + connect happen on the worker task.
     * Splash shows "scanning..." now; the IP_EVENT_STA_GOT_IP handler
     * will update the row to PASS later when the IP arrives. */
    if (wifi_manager_init() == ESP_OK && wifi_manager_start(&g_config.wifi) == ESP_OK) {
        if (g_config.wifi.mode == WIFI_STA && g_config.wifi.sta_network_count > 0) {
            report_row(SPLASH_ROW_WIFI, SPLASH_STATUS_RUNNING, "scanning...");
        } else {
            report_row(SPLASH_ROW_WIFI, SPLASH_STATUS_SKIP, "disabled in config");
        }
    } else {
        report_row(SPLASH_ROW_WIFI, SPLASH_STATUS_FAIL, "init failed");
    }

    /* GPS / IMU sources still not implemented. */
    report_row(SPLASH_ROW_GPS,  SPLASH_STATUS_SKIP, "not implemented");
    report_row(SPLASH_ROW_IMU,  SPLASH_STATUS_SKIP, "not implemented");

    ESP_LOGI(TAG, "--- Phase B (#69): bring up display + LVGL + Splash ---");

    /* Display panel — bring backlight ON via CH422G before the LCD reset
     * so the first frame isn't black-on-black. */
    ch422g_lcd_backlight(true);

    if (display_init() != ESP_OK) {
        ESP_LOGE(TAG, "display_init failed — proceeding headless (boot continues)");
        report_row(SPLASH_ROW_DISPLAY, SPLASH_STATUS_FAIL, "init failed");
    } else if (lvgl_init() != ESP_OK) {
        ESP_LOGE(TAG, "lvgl_init failed — proceeding headless");
        report_row(SPLASH_ROW_DISPLAY, SPLASH_STATUS_FAIL, "lvgl init failed");
    } else {
        /* CRITICAL: wire the RGB bounce-buffer event to the LVGL VSYNC
         * notifier. Without this, the flush callback's portMAX_DELAY
         * wait for VSYNC never returns, the LVGL task holds the mutex
         * forever, and any screen-load call (e.g. screen_splash_show)
         * fails with "could not acquire LVGL mutex". Must come AFTER
         * lvgl_init so the task handle exists by the time the
         * bounce-buffer ISR fires. */
        if (display_register_vsync_callback() != ESP_OK) {
            ESP_LOGE(TAG, "display_register_vsync_callback failed");
        }

        report_row(SPLASH_ROW_DISPLAY, SPLASH_STATUS_PASS,
                   "%dx%d", LCD_WIDTH, LCD_HEIGHT);
        /* Touch initialised AFTER lvgl_init so the GT911 device-handle
         * exists by the time we register it as an LVGL input device. */
        if (touch_init() == ESP_OK) {
            lvgl_register_touch_indev();
        }

        if (screen_splash_show(FIRMWARE_VERSION_STRING) != ESP_OK) {
            ESP_LOGE(TAG, "screen_splash_show failed");
        } else {
            s_splash_up = true;
            replay_rows();
        }
    }

    screen_splash_done();

    /* Splash → Monitor handoff. Hold the splash for 2 seconds so the
     * user can read the boot summary, then switch to Monitor. The
     * splash screen object is abandoned (lv_obj_clean handles it when
     * the screen tree changes — fine for v0.2 boot path).
     *
     * If display isn't up (display_init or lvgl_init failed earlier),
     * skip — the device runs headless. */
    if (s_splash_up) {
        vTaskDelay(pdMS_TO_TICKS(2000));

        /* Build the top-level carousel and hand off from Splash to the
         * landing screen (Monitor, in the middle of the swipe order).
         * Order matches Information-Architecture.md:
         *   0 = Connections | 1 = Info | 2 = Monitor (default) |
         *   3 = Settings    | 4 = Diagnostics */
        lv_obj_t *connections = screen_connections_create();
        lv_obj_t *info        = screen_info_create();
        lv_obj_t *monitor     = screen_monitor_create();
        lv_obj_t *settings    = screen_settings_create();
        lv_obj_t *diagnostics = screen_diagnostics_create();
        if (monitor) {
            screen_monitor_set_boat_name(monitor, g_config.device.name);
        }

        if (connections) {
            screen_nav_register(0, connections);
            screen_nav_attach_gestures(connections);
        }
        if (info) {
            screen_nav_register(1, info);
            screen_nav_attach_gestures(info);
        }
        if (monitor) {
            screen_nav_register(2, monitor);
            screen_nav_attach_gestures(monitor);
        }
        if (settings) {
            screen_nav_register(3, settings);
            screen_nav_attach_gestures(settings);
        }
        if (diagnostics) {
            screen_nav_register(4, diagnostics);
            screen_nav_attach_gestures(diagnostics);
        }

        /* Land on Monitor (centre of the carousel) per spec. */
        screen_nav_switch_to(2);
        ESP_LOGI(TAG, "splash -> Monitor (5-screen carousel)");
    }

    ESP_LOGI(TAG, "--- Boot complete — heartbeat every 10s ---");

    uint32_t tick = 0;
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        ESP_LOGI(TAG, "heartbeat v%s tick=%lu free_heap=%lu",
                 FIRMWARE_VERSION_STRING,
                 (unsigned long) ++tick,
                 (unsigned long) esp_get_free_heap_size());
    }
}
