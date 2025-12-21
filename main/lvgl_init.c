/**
 * LVGL Initialization Implementation
 *
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2025-12-20
 * Date Updated: 2025-12-20
 * Version: 0.1.1
 */

#include "lvgl_init.h"
#include "display_driver.h"
#include "board_config.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

static const char *TAG = "lvgl_init";

// LVGL display object
static lv_display_t *lvgl_display = NULL;

// LVGL mutex for thread safety
static SemaphoreHandle_t lvgl_mutex = NULL;

// LVGL task handle
static TaskHandle_t lvgl_task_handle = NULL;

// LVGL tick timer
static esp_timer_handle_t lvgl_tick_timer = NULL;

/**
 * LVGL tick timer callback
 */
static void lvgl_tick_timer_cb(void *arg) {
    lv_tick_inc(10);  // Increment LVGL tick by 10ms
}

/**
 * LVGL flush callback - called when LVGL needs to update display
 * For RGB panels, this is a no-op as LVGL draws directly to frame buffer
 */
static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    // For RGB panels using direct mode, just notify LVGL we're done
    // The data is already in the frame buffer
    lv_display_flush_ready(disp);
}

/**
 * LVGL task - handles UI updates
 */
static void lvgl_task(void *arg) {
    ESP_LOGI(TAG, "LVGL task started");

    while (1) {
        // Lock mutex
        if (xSemaphoreTake(lvgl_mutex, portMAX_DELAY) == pdTRUE) {
            // Handle LVGL tasks
            uint32_t delay_ms = lv_timer_handler();

            // Unlock mutex
            xSemaphoreGive(lvgl_mutex);

            // Sleep for calculated delay (minimum 10ms)
            if (delay_ms < 10) {
                delay_ms = 10;
            }
            vTaskDelay(pdMS_TO_TICKS(delay_ms));
        }
    }
}

/**
 * Initialize LVGL
 */
esp_err_t lvgl_init(void) {
    esp_err_t ret;

    ESP_LOGI(TAG, "Initializing LVGL v%d.%d.%d",
             lv_version_major(), lv_version_minor(), lv_version_patch());

    // Create mutex for LVGL thread safety
    lvgl_mutex = xSemaphoreCreateMutex();
    if (lvgl_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create LVGL mutex");
        return ESP_ERR_NO_MEM;
    }

    // Initialize LVGL
    lv_init();
    ESP_LOGI(TAG, "LVGL core initialized");

    // Create display object
    lvgl_display = lv_display_create(LCD_WIDTH, LCD_HEIGHT);
    if (lvgl_display == NULL) {
        ESP_LOGE(TAG, "Failed to create LVGL display");
        return ESP_FAIL;
    }

    // Get RGB panel frame buffer (allocated in PSRAM by RGB driver)
    esp_lcd_panel_handle_t panel = display_get_panel();
    void *buf1 = NULL;

    // Get the single frame buffer from RGB panel
    ret = esp_lcd_rgb_panel_get_frame_buffer(panel, 1, &buf1);
    if (ret != ESP_OK || buf1 == NULL) {
        ESP_LOGE(TAG, "Failed to get RGB panel frame buffer: %s", esp_err_to_name(ret));
        return ESP_ERR_NO_MEM;
    }

    size_t buffer_size = LCD_WIDTH * LCD_HEIGHT * sizeof(lv_color_t);
    ESP_LOGI(TAG, "Using RGB panel frame buffer: %zu bytes in PSRAM", buffer_size);

    // Set display buffer to use RGB panel's frame buffer directly (single buffer)
    lv_display_set_buffers(lvgl_display, buf1, NULL, buffer_size, LV_DISPLAY_RENDER_MODE_DIRECT);
    ESP_LOGI(TAG, "LVGL configured with single buffering (direct mode)");

    // Set flush callback (no-op for RGB panels in direct mode)
    lv_display_set_flush_cb(lvgl_display, lvgl_flush_cb);

    ESP_LOGI(TAG, "Display driver registered with LVGL");

    // Create and start tick timer (10ms period)
    const esp_timer_create_args_t timer_args = {
        .callback = lvgl_tick_timer_cb,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "lvgl_tick"
    };

    ret = esp_timer_create(&timer_args, &lvgl_tick_timer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create LVGL tick timer: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_timer_start_periodic(lvgl_tick_timer, 10000);  // 10ms = 10000us
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start LVGL tick timer: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "LVGL tick timer started (10ms period)");

    // Create LVGL task
    BaseType_t task_ret = xTaskCreatePinnedToCore(
        lvgl_task,
        "lvgl_task",
        TASK_STACK_SIZE_LARGE,
        NULL,
        TASK_PRIORITY_NORMAL,
        &lvgl_task_handle,
        1  // Pin to core 1
    );

    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create LVGL task");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "LVGL task created (priority %d, stack %d bytes, core 1)",
             TASK_PRIORITY_NORMAL, TASK_STACK_SIZE_LARGE);

    ESP_LOGI(TAG, "LVGL initialization complete");

    return ESP_OK;
}

/**
 * Get LVGL display object
 */
lv_display_t* lvgl_get_display(void) {
    return lvgl_display;
}

/**
 * Lock LVGL mutex
 */
bool lvgl_lock(uint32_t timeout_ms) {
    if (lvgl_mutex == NULL) {
        return false;
    }
    return xSemaphoreTake(lvgl_mutex, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
}

/**
 * Unlock LVGL mutex
 */
void lvgl_unlock(void) {
    if (lvgl_mutex != NULL) {
        xSemaphoreGive(lvgl_mutex);
    }
}
