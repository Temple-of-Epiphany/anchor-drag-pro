/**
 * LVGL Initialization Implementation
 * Waveshare Mode 3: Direct-Mode with hardware-managed synchronization
 * LVGL 8.x API
 *
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2025-12-20
 * Date Updated: 2025-12-24
 * Version: 0.3.0
 */

#include "lvgl_init.h"
#include "display_driver.h"
#include "touch_driver.h"
#include "board_config.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_lcd_touch.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

static const char *TAG = "lvgl_init";

// LVGL display driver
static lv_disp_drv_t disp_drv;
static lv_disp_draw_buf_t disp_buf;
static lv_disp_t *lvgl_display = NULL;

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
 * LVGL touch read callback - reads touch input (LVGL 8.x API)
 */
static void lvgl_touch_read_cb(lv_indev_drv_t *indev_drv, lv_indev_data_t *data) {
    esp_lcd_touch_handle_t tp = (esp_lcd_touch_handle_t)indev_drv->user_data;

    if (tp == NULL) {
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }

    uint16_t touchpad_x;
    uint16_t touchpad_y;
    uint8_t touchpad_cnt = 0;

    // Read data from touch controller into memory (I2C transaction)
    if (esp_lcd_touch_read_data(tp) != ESP_OK) {
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }

    // Get touch coordinates
    bool touchpad_pressed = esp_lcd_touch_get_coordinates(tp, &touchpad_x, &touchpad_y, NULL, &touchpad_cnt, 1);

    if (touchpad_pressed && touchpad_cnt > 0) {
        data->point.x = touchpad_x;
        data->point.y = touchpad_y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

/**
 * LVGL flush callback - Mode 3 Direct-Mode with VSYNC synchronization (LVGL 8.x API)
 *
 * This matches the Waveshare Mode 3 implementation exactly:
 * - LVGL renders to frame buffer in direct mode
 * - On last flush area, switch buffers and WAIT for VSYNC
 * - This prevents tearing by ensuring we don't render to a buffer being scanned out
 */
static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map) {
    esp_lcd_panel_handle_t panel = display_get_panel();

    int offsetx1 = area->x1;
    int offsety1 = area->y1;
    int offsetx2 = area->x2;
    int offsety2 = area->y2;

    // Check if this is the last flush area (LVGL 8.x API)
    // This matches Waveshare lvgl_port.c lines 279-285
    if (lv_disp_flush_is_last(drv)) {
        // Switch the RGB panel to display this frame buffer
        // In direct mode, color_map IS one of the RGB panel's frame buffers
        esp_lcd_panel_draw_bitmap(panel, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_map);

        // Wait for the frame buffer transmission to complete (VSYNC)
        // This is the key to Mode 3: wait HERE in flush callback, not in LVGL task
        ulTaskNotifyValueClear(NULL, ULONG_MAX);
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    }

    lv_disp_flush_ready(drv);
}

/**
 * LVGL task - Mode 3: Simple polling (matches Waveshare implementation)
 *
 * Note: VSYNC synchronization happens in the flush callback, NOT here.
 * This task just calls lv_timer_handler() regularly to process UI updates.
 */
static void lvgl_task(void *arg) {
    ESP_LOGI(TAG, "LVGL task started (Mode 3: Direct-Mode)");

    uint32_t task_delay_ms = 500;  // Max delay

    while (1) {
        // Lock mutex and process LVGL tasks
        if (xSemaphoreTake(lvgl_mutex, portMAX_DELAY) == pdTRUE) {
            // Handle LVGL tasks (rendering, timers, animations)
            // This may trigger flush callback which will wait for VSYNC
            task_delay_ms = lv_timer_handler();

            // Unlock mutex
            xSemaphoreGive(lvgl_mutex);
        }

        // Clamp delay to reasonable range (like Waveshare does)
        if (task_delay_ms > 500) {
            task_delay_ms = 500;
        } else if (task_delay_ms < 1) {
            task_delay_ms = 1;
        }

        vTaskDelay(pdMS_TO_TICKS(task_delay_ms));
    }
}

/**
 * Initialize LVGL (LVGL 8.x API)
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

    // Get RGB panel handle for direct-mode rendering (Waveshare Mode 3)
    esp_lcd_panel_handle_t panel = display_get_panel();

    // Get RGB panel's internal frame buffers (allocated in PSRAM by panel driver)
    void *fb1 = NULL;
    void *fb2 = NULL;
    ret = esp_lcd_rgb_panel_get_frame_buffer(panel, 2, &fb1, &fb2);
    if (ret != ESP_OK || fb1 == NULL || fb2 == NULL) {
        ESP_LOGE(TAG, "Failed to get RGB panel frame buffers: %s", esp_err_to_name(ret));
        return ret;
    }

    size_t buffer_size = LCD_WIDTH * LCD_HEIGHT;  // Size in pixels
    ESP_LOGI(TAG, "RGB panel frame buffers: fb1=%p, fb2=%p, size=%zu pixels each",
             fb1, fb2, buffer_size);

    // Initialize LVGL draw buffer with RGB panel frame buffers (LVGL 8.x API)
    lv_disp_draw_buf_init(&disp_buf, fb1, fb2, buffer_size);

    // Initialize display driver (LVGL 8.x API)
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = LCD_WIDTH;
    disp_drv.ver_res = LCD_HEIGHT;
    disp_drv.flush_cb = lvgl_flush_cb;
    disp_drv.draw_buf = &disp_buf;
    disp_drv.user_data = panel;

    // Enable direct mode - CRITICAL for Mode 3 (Waveshare lvgl_port.c line 424)
    disp_drv.direct_mode = 1;

    ESP_LOGI(TAG, "Display driver configured for Mode 3 (direct_mode=1)");

    // Register display driver
    lvgl_display = lv_disp_drv_register(&disp_drv);
    if (lvgl_display == NULL) {
        ESP_LOGE(TAG, "Failed to register display driver");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Display driver registered with LVGL");

    // Create and register touch input device (LVGL 8.x API)
    esp_lcd_touch_handle_t touch_handle = touch_get_handle();
    if (touch_handle != NULL) {
        static lv_indev_drv_t indev_drv;
        lv_indev_drv_init(&indev_drv);
        indev_drv.type = LV_INDEV_TYPE_POINTER;
        indev_drv.read_cb = lvgl_touch_read_cb;
        indev_drv.user_data = touch_handle;

        lv_indev_t *touch_indev = lv_indev_drv_register(&indev_drv);
        if (touch_indev != NULL) {
            ESP_LOGI(TAG, "Touch input device registered with LVGL");
        }
    } else {
        ESP_LOGW(TAG, "Touch handle is NULL, skipping touch registration");
    }

    // Create and start tick timer (10ms period - Waveshare standard)
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

    // Create LVGL task (increased to 10KB for SD card operations)
    const int LVGL_TASK_PRIORITY = 2;
    const int LVGL_TASK_STACK = 10240;  // 10KB (was 6KB)

    BaseType_t task_ret = xTaskCreatePinnedToCore(
        lvgl_task,
        "lvgl_task",
        LVGL_TASK_STACK,
        NULL,
        LVGL_TASK_PRIORITY,
        &lvgl_task_handle,
        1  // Pin to core 1
    );

    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create LVGL task");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "LVGL task created (priority %d, stack %d bytes, core 1)",
             LVGL_TASK_PRIORITY, LVGL_TASK_STACK);

    ESP_LOGI(TAG, "LVGL initialization complete - Mode 3 Direct-Mode with VSYNC synchronization");

    return ESP_OK;
}

/**
 * Get LVGL display object
 */
lv_disp_t* lvgl_get_display(void) {
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

/**
 * Notify LVGL task from VSYNC ISR (called when RGB frame transmission completes)
 */
bool lvgl_notify_vsync_isr(void) {
    BaseType_t need_yield = pdFALSE;

    if (lvgl_task_handle != NULL) {
        xTaskNotifyFromISR(lvgl_task_handle, ULONG_MAX, eNoAction, &need_yield);
    }

    return (need_yield == pdTRUE);
}
