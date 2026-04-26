/**
 * SMPTE Test Pattern Screen Implementation
 *
 * Displays custom SMPTE test pattern with Anchor Drag Pro logo for screen self-test on boot.
 * Tests all pixels and color accuracy of the display.
 *
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2025-12-22
 * Date Updated: 2025-12-23
 * Version: 0.2.0
 *
 * Changelog:
 * - 0.2.0 (2025-12-23): Use pre-rendered test pattern with logo and text
 * - 0.1.0 (2025-12-22): Initial implementation with SMPTE color bars
 */

#include "smpte_test_screen.h"
#include "smpte_test_pattern.h"
#include "lvgl_init.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "smpte_test";

// LVGL UI objects
static lv_obj_t *smpte_screen = NULL;
static lv_obj_t *test_image = NULL;

/**
 * Create SMPTE test pattern from pre-rendered image
 */
static esp_err_t create_smpte_pattern(void) {
    ESP_LOGI(TAG, "Creating SMPTE test pattern from image...");

    if (!lvgl_lock(1000)) {
        ESP_LOGE(TAG, "Failed to lock LVGL mutex");
        return ESP_FAIL;
    }

    // Create screen
    smpte_screen = lv_obj_create(NULL);
    if (smpte_screen == NULL) {
        ESP_LOGE(TAG, "Failed to create SMPTE screen object");
        lvgl_unlock();
        return ESP_FAIL;
    }

    // Remove padding and scrollbars
    lv_obj_set_style_pad_all(smpte_screen, 0, 0);
    lv_obj_clear_flag(smpte_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(smpte_screen, lv_color_hex(0x000000), 0);

    // Create image descriptor for LVGL 9.x
    static lv_img_dsc_t test_pattern_dsc = {
        .header = {
            .cf = LV_COLOR_FORMAT_RGB565,
            .w = 800,
            .h = 480,
        },
        .data_size = 800 * 480 * 2,
        .data = NULL
    };
    test_pattern_dsc.data = smpte_test_pattern_data;

    // Create image object
    test_image = lv_img_create(smpte_screen);
    if (test_image == NULL) {
        ESP_LOGE(TAG, "Failed to create image object");
        lvgl_unlock();
        return ESP_FAIL;
    }

    // Set the image source
    lv_img_set_src(test_image, &test_pattern_dsc);

    // Position at (0, 0) to fill the screen
    lv_obj_set_pos(test_image, 0, 0);
    lv_obj_clear_flag(test_image, LV_OBJ_FLAG_SCROLLABLE);

    // Load the screen
    lv_scr_load(smpte_screen);

    lvgl_unlock();

    // Force immediate redraw
    vTaskDelay(pdMS_TO_TICKS(10));

    if (lvgl_lock(100)) {
        lv_obj_invalidate(smpte_screen);
        lv_refr_now(lvgl_get_display());
        lvgl_unlock();
    }

    ESP_LOGI(TAG, "SMPTE test pattern created successfully");
    return ESP_OK;
}

/**
 * Run SMPTE test screen
 */
esp_err_t smpte_test_screen_run(uint32_t duration_sec) {
    ESP_LOGI(TAG, "Starting SMPTE test pattern (duration: %lu seconds)", duration_sec);

    // Create test pattern
    esp_err_t ret = create_smpte_pattern();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create SMPTE pattern");
        return ret;
    }

    // Display for specified duration
    ESP_LOGI(TAG, "Displaying SMPTE test pattern...");
    vTaskDelay(pdMS_TO_TICKS(duration_sec * 1000));

    ESP_LOGI(TAG, "SMPTE test complete");
    return ESP_OK;
}
