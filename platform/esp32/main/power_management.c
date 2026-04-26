/**
 * Power Management Module Implementation
 *
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2025-12-25
 * Version: 0.1.0
 */

#include "power_management.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "power_mgmt";

// Touch pad configuration for wake-up
#define TOUCH_WAKEUP_THRESHOLD 500  // Adjust based on touch sensitivity

/**
 * Initialize power management system
 */
void power_mgmt_init(void) {
    ESP_LOGI(TAG, "Initializing power management");

    // Check if we woke from deep sleep
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

    switch (cause) {
        case ESP_SLEEP_WAKEUP_EXT0:
            ESP_LOGI(TAG, "Wake from external GPIO");
            break;
        case ESP_SLEEP_WAKEUP_EXT1:
            ESP_LOGI(TAG, "Wake from external GPIO group");
            break;
        case ESP_SLEEP_WAKEUP_TIMER:
            ESP_LOGI(TAG, "Wake from timer");
            break;
        case ESP_SLEEP_WAKEUP_TOUCHPAD:
            ESP_LOGI(TAG, "Wake from touch pad");
            break;
        case ESP_SLEEP_WAKEUP_GPIO:
            ESP_LOGI(TAG, "Wake from GPIO");
            break;
        case ESP_SLEEP_WAKEUP_UART:
            ESP_LOGI(TAG, "Wake from UART");
            break;
        case ESP_SLEEP_WAKEUP_UNDEFINED:
        default:
            ESP_LOGI(TAG, "Cold boot (not waking from sleep)");
            break;
    }

    // Configure wake-up sources
    // NOTE: Touch wake-up disabled due to false triggers causing immediate reboot
    // Device will require physical EN/RST button press to wake from deep sleep
    // esp_sleep_enable_ext0_wakeup(GPIO_NUM_4, 0);  // DISABLED - touch interrupt too sensitive

    ESP_LOGI(TAG, "Power management initialized - wake via EN/RST button only");
}

/**
 * Enter deep sleep mode
 */
void power_mgmt_sleep(void) {
    ESP_LOGI(TAG, "Entering deep sleep mode...");
    ESP_LOGI(TAG, "Device will wake on EN/RST button press");
    ESP_LOGI(TAG, "Powering off in 2 seconds...");

    // Wait 2 seconds for visual feedback
    vTaskDelay(pdMS_TO_TICKS(2000));

    // Save state before sleeping (if needed)
    power_mgmt_save_state();

    ESP_LOGI(TAG, "Entering deep sleep NOW");
    vTaskDelay(pdMS_TO_TICKS(100));  // Allow final log to flush

    // Enter deep sleep
    // Wake sources already configured in power_mgmt_init()
    esp_deep_sleep_start();

    // Never reaches here - device will reset on wake
}

/**
 * Check if device woke from deep sleep
 */
bool power_mgmt_is_wake_from_sleep(void) {
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    return (cause != ESP_SLEEP_WAKEUP_UNDEFINED);
}

/**
 * Get the wake-up cause
 */
esp_sleep_wakeup_cause_t power_mgmt_get_wake_cause(void) {
    return esp_sleep_get_wakeup_cause();
}

/**
 * Configure timer wake-up
 */
void power_mgmt_set_timer_wakeup(int hours) {
    if (hours > 0) {
        uint64_t sleep_time_us = (uint64_t)hours * 3600 * 1000000;  // Convert hours to microseconds
        esp_sleep_enable_timer_wakeup(sleep_time_us);
        ESP_LOGI(TAG, "Timer wake-up configured for %d hours", hours);
    } else {
        esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
        ESP_LOGI(TAG, "Timer wake-up disabled");
    }
}

/**
 * Save current system state to NVS
 */
void power_mgmt_save_state(void) {
    ESP_LOGI(TAG, "Saving system state to NVS");

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("power_mgmt", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
        return;
    }

    // Save last sleep timestamp
    int64_t timestamp = esp_timer_get_time();
    nvs_set_i64(nvs_handle, "sleep_time", timestamp);

    // Save any other state here (e.g., current mode, settings, etc.)

    nvs_commit(nvs_handle);
    nvs_close(nvs_handle);

    ESP_LOGI(TAG, "State saved successfully");
}

/**
 * Restore system state from NVS
 */
void power_mgmt_restore_state(void) {
    ESP_LOGI(TAG, "Restoring system state from NVS");

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("power_mgmt", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "No saved state found: %s", esp_err_to_name(err));
        return;
    }

    // Restore last sleep timestamp
    int64_t sleep_time = 0;
    err = nvs_get_i64(nvs_handle, "sleep_time", &sleep_time);
    if (err == ESP_OK) {
        int64_t current_time = esp_timer_get_time();
        int sleep_duration_sec = (current_time - sleep_time) / 1000000;
        ESP_LOGI(TAG, "Device was asleep for %d seconds", sleep_duration_sec);
    }

    // Restore any other state here

    nvs_close(nvs_handle);
    ESP_LOGI(TAG, "State restored successfully");
}
