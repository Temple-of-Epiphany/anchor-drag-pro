/**
 * Simple Test Screen - Minimal UI for testing LVGL 8.4.0
 *
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2025-12-24
 * Version: 0.1.0
 */

#include "simple_test_screen.h"
#include "lvgl_init.h"
#include "esp_log.h"

static const char *TAG = "simple_test";

static lv_obj_t *screen = NULL;
static lv_obj_t *ok_button = NULL;

/**
 * OK button click event handler
 */
static void ok_button_event_cb(lv_event_t *e) {
    ESP_LOGI(TAG, "OK button clicked!");
}

/**
 * Create simple test screen
 */
esp_err_t simple_test_screen_create(void) {
    ESP_LOGI(TAG, "Creating simple test screen...");

    // Get display
    lv_disp_t *display = lvgl_get_display();
    if (display == NULL) {
        ESP_LOGE(TAG, "Failed to get LVGL display");
        return ESP_FAIL;
    }

    // Lock LVGL
    if (!lvgl_lock(1000)) {
        ESP_LOGE(TAG, "Failed to lock LVGL");
        return ESP_FAIL;
    }

    // Create screen (LVGL 8.x API)
    screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), 0);  // Black background

    // Create title label
    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "Simple Test Screen");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);  // White text
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 60);

    // Create OK button
    ok_button = lv_btn_create(screen);
    lv_obj_set_size(ok_button, 200, 80);
    lv_obj_align(ok_button, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(ok_button, ok_button_event_cb, LV_EVENT_CLICKED, NULL);

    // Button label
    lv_obj_t *btn_label = lv_label_create(ok_button);
    lv_label_set_text(btn_label, "OK");
    lv_obj_center(btn_label);

    // Load screen (LVGL 8.x API)
    lv_scr_load(screen);

    // Unlock LVGL
    lvgl_unlock();

    ESP_LOGI(TAG, "Simple test screen created successfully");
    return ESP_OK;
}

/**
 * Delete simple test screen
 */
void simple_test_screen_delete(void) {
    if (!lvgl_lock(1000)) {
        ESP_LOGE(TAG, "Failed to lock LVGL for deletion");
        return;
    }

    if (screen != NULL) {
        lv_obj_del(screen);  // LVGL 8.x API
        screen = NULL;
        ok_button = NULL;
    }

    lvgl_unlock();
    ESP_LOGI(TAG, "Simple test screen deleted");
}
