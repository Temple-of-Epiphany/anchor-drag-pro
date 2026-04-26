/**
 * Display Refresh Test - Auto-updating screen without touch interaction
 *
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2025-12-24
 * Version: 0.1.0
 */

#include "display_test.h"
#include "lvgl_init.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "display_test";

static lv_obj_t *test_screen = NULL;
static lv_obj_t *counter_label = NULL;
static lv_obj_t *fps_label = NULL;
static lv_obj_t *info_label = NULL;
static lv_timer_t *update_timer = NULL;
static uint32_t counter = 0;

/**
 * Timer callback to update the display every 100ms
 */
static void update_timer_cb(lv_timer_t *timer) {
    counter++;

    // Update counter
    lv_label_set_text_fmt(counter_label, "Counter: %lu", counter);

    // Update FPS estimate
    static uint32_t last_time = 0;
    uint32_t now = lv_tick_get();
    uint32_t elapsed = now - last_time;
    if (elapsed > 0) {
        uint32_t fps = 1000 / elapsed;
        lv_label_set_text_fmt(fps_label, "Update Rate: ~%lu Hz", fps);
    }
    last_time = now;

    ESP_LOGI(TAG, "Display update #%lu - testing continuous refresh", counter);
}

/**
 * Create display test screen
 */
void display_test_create(void) {
    ESP_LOGI(TAG, "Creating display refresh test screen");

    // Get display
    lv_disp_t *display = lvgl_get_display();
    if (display == NULL) {
        ESP_LOGE(TAG, "Failed to get LVGL display");
        return;
    }

    // Create test screen (LVGL 8.x API)
    test_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(test_screen, lv_color_hex(0x000000), 0);

    // Create title
    lv_obj_t *title = lv_label_create(test_screen);
    lv_label_set_text(title, "Display Refresh Test");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    // Create info label
    info_label = lv_label_create(test_screen);
    lv_label_set_text(info_label,
        "This screen auto-updates every 100ms\n"
        "to test display refresh without touch.\n"
        "Watch for tearing or artifacts.");
    lv_obj_set_style_text_color(info_label, lv_color_hex(0xAAAAAA), 0);
    lv_obj_set_style_text_align(info_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(info_label, LV_ALIGN_TOP_MID, 0, 60);

    // Create counter label (larger text)
    counter_label = lv_label_create(test_screen);
    lv_label_set_text(counter_label, "Counter: 0");
    lv_obj_set_style_text_color(counter_label, lv_color_hex(0x00FF00), 0);
    lv_obj_align(counter_label, LV_ALIGN_CENTER, 0, -20);

    // Create FPS label
    fps_label = lv_label_create(test_screen);
    lv_label_set_text(fps_label, "Update Rate: -- Hz");
    lv_obj_set_style_text_color(fps_label, lv_color_hex(0xFFFF00), 0);
    lv_obj_align(fps_label, LV_ALIGN_CENTER, 0, 40);

    // Create exit instruction
    lv_obj_t *exit_label = lv_label_create(test_screen);
    lv_label_set_text(exit_label, "Touch screen to exit test");
    lv_obj_set_style_text_color(exit_label, lv_color_hex(0x888888), 0);
    lv_obj_align(exit_label, LV_ALIGN_BOTTOM_MID, 0, -20);

    // Load the screen (LVGL 8.x API)
    lv_scr_load(test_screen);

    // Create timer for auto-updates (100ms = 10 Hz)
    update_timer = lv_timer_create(update_timer_cb, 100, NULL);

    ESP_LOGI(TAG, "Display test screen created - auto-updating at 10 Hz");
}

/**
 * Stop display test
 */
void display_test_stop(void) {
    if (update_timer != NULL) {
        lv_timer_del(update_timer);  // LVGL 8.x API
        update_timer = NULL;
    }

    if (test_screen != NULL) {
        lv_obj_del(test_screen);  // LVGL 8.x API
        test_screen = NULL;
    }

    ESP_LOGI(TAG, "Display test stopped");
}
