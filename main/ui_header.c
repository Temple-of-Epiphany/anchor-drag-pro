/**
 * UI Header Component Implementation
 *
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2025-12-22
 * Date Updated: 2025-12-22
 * Version: 0.2.0
 *
 * Changelog:
 * - 0.2.0 (2025-12-22): Redesigned as full-width header bar with title and 6 icon placeholders
 * - 0.1.0 (2025-12-22): Initial implementation with GPS and Compass icons only
 */

#include "ui_header.h"
#include "board_config.h"
#include "esp_log.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "ui_header";

// Header styling
#define ICON_SIZE 50
#define ICON_MARGIN 15
#define TITLE_FONT_SIZE 32

// Child objects stored as user data
typedef struct {
    lv_obj_t *header_bar;      // Main header container
    lv_obj_t *title_label;     // "ANCHOR DRAG ALARM" text
    lv_obj_t *left_icons[3];   // 3 left icon placeholders
    lv_obj_t *right_icons[3];  // 3 right icon placeholders (0=compass, 1=gps, 2=reserved)
    lv_obj_t *icon_labels[6];  // Labels for each icon
} ui_header_data_t;

/**
 * Create full-width header bar with title and icon placeholders
 */
lv_obj_t* ui_header_create(lv_obj_t *parent) {
    ESP_LOGI(TAG, "Creating full-width header bar");

    // Allocate header data (LVGL 8.x)
    ui_header_data_t *data = malloc(sizeof(ui_header_data_t));
    if (data == NULL) {
        ESP_LOGE(TAG, "Failed to allocate header data");
        return NULL;
    }
    memset(data, 0, sizeof(ui_header_data_t));

    // Create header bar container (full width, dark blue background)
    data->header_bar = lv_obj_create(parent);
    lv_obj_set_size(data->header_bar, HEADER_WIDTH, HEADER_HEIGHT);
    lv_obj_align(data->header_bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(data->header_bar, lv_color_hex(0x001F3F), 0);  // Marine dark
    lv_obj_set_style_bg_opa(data->header_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(data->header_bar, 0, 0);
    lv_obj_set_style_pad_all(data->header_bar, 0, 0);
    lv_obj_set_style_radius(data->header_bar, 0, 0);
    lv_obj_clear_flag(data->header_bar, LV_OBJ_FLAG_SCROLLABLE);

    // Title label: "ANCHOR DRAG ALARM" (center)
    data->title_label = lv_label_create(data->header_bar);
    lv_label_set_text(data->title_label, "ANCHOR DRAG ALARM");
    lv_obj_set_style_text_color(data->title_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(data->title_label, &lv_font_montserrat_14, 0);
    lv_obj_align(data->title_label, LV_ALIGN_CENTER, 0, 0);

    // Create 3 left icon placeholders
    for (int i = 0; i < 3; i++) {
        data->left_icons[i] = lv_obj_create(data->header_bar);
        lv_obj_set_size(data->left_icons[i], ICON_SIZE, ICON_SIZE);
        lv_obj_set_style_radius(data->left_icons[i], ICON_SIZE / 2, 0);  // Circular
        lv_obj_set_style_bg_color(data->left_icons[i], lv_color_hex(0x333333), 0);  // Dark gray (placeholder)
        lv_obj_set_style_bg_opa(data->left_icons[i], LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(data->left_icons[i], 2, 0);
        lv_obj_set_style_border_color(data->left_icons[i], lv_color_hex(0x555555), 0);
        lv_obj_set_pos(data->left_icons[i],
                       ICON_MARGIN + (i * (ICON_SIZE + ICON_MARGIN)),
                       (HEADER_HEIGHT - ICON_SIZE) / 2);

        // Placeholder label (empty for now)
        data->icon_labels[i] = lv_label_create(data->left_icons[i]);
        lv_label_set_text(data->icon_labels[i], "");
        lv_obj_set_style_text_color(data->icon_labels[i], lv_color_white(), 0);
        lv_obj_center(data->icon_labels[i]);
    }

    // Create 3 right icon placeholders
    for (int i = 0; i < 3; i++) {
        data->right_icons[i] = lv_obj_create(data->header_bar);
        lv_obj_set_size(data->right_icons[i], ICON_SIZE, ICON_SIZE);
        lv_obj_set_style_radius(data->right_icons[i], ICON_SIZE / 2, 0);  // Circular
        lv_obj_set_style_bg_color(data->right_icons[i], lv_color_hex(0x333333), 0);  // Dark gray (placeholder)
        lv_obj_set_style_bg_opa(data->right_icons[i], LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(data->right_icons[i], 2, 0);
        lv_obj_set_style_border_color(data->right_icons[i], lv_color_hex(0x555555), 0);
        lv_obj_set_pos(data->right_icons[i],
                       HEADER_WIDTH - ICON_MARGIN - ((i + 1) * (ICON_SIZE + ICON_MARGIN)),
                       (HEADER_HEIGHT - ICON_SIZE) / 2);

        // Icon labels
        data->icon_labels[3 + i] = lv_label_create(data->right_icons[i]);
        lv_obj_set_style_text_color(data->icon_labels[3 + i], lv_color_white(), 0);
        lv_obj_center(data->icon_labels[3 + i]);
    }

    // Set icons for specific positions
    // Right icon 0 (compass) - second from right
    lv_label_set_text(data->icon_labels[3], LV_SYMBOL_DIRECTORY);
    lv_obj_set_style_bg_color(data->right_icons[0], lv_color_hex(0x808080), 0);  // Gray (off by default)

    // Right icon 1 (GPS) - rightmost
    lv_label_set_text(data->icon_labels[4], LV_SYMBOL_GPS);
    lv_obj_set_style_bg_color(data->right_icons[1], lv_color_hex(0x808080), 0);  // Gray (off by default)

    // Right icon 2 - third from right (placeholder, empty for now)
    lv_label_set_text(data->icon_labels[5], "");

    // Store data as user data (use header_bar as the handle)
    lv_obj_set_user_data(data->header_bar, data);

    ESP_LOGI(TAG, "Header bar created: %dx%d at top", HEADER_WIDTH, HEADER_HEIGHT);
    return data->header_bar;
}

/**
 * Update GPS status icon (right side, position 1)
 */
void ui_header_set_gps_status(lv_obj_t *header, bool found) {
    if (header == NULL) return;

    ui_header_data_t *data = (ui_header_data_t *)lv_obj_get_user_data(header);
    if (data == NULL || data->right_icons[1] == NULL) return;

    if (found) {
        // GPS found - show green icon
        lv_obj_set_style_bg_color(data->right_icons[1], lv_color_hex(0x00FF00), 0);  // Green
        lv_obj_set_style_border_color(data->right_icons[1], lv_color_hex(0x00AA00), 0);
        ESP_LOGD(TAG, "GPS status: FOUND");
    } else {
        // GPS not found - show gray icon
        lv_obj_set_style_bg_color(data->right_icons[1], lv_color_hex(0x808080), 0);  // Gray
        lv_obj_set_style_border_color(data->right_icons[1], lv_color_hex(0x555555), 0);
        ESP_LOGD(TAG, "GPS status: NOT FOUND");
    }
}

/**
 * Update Compass status icon (right side, position 0)
 */
void ui_header_set_compass_status(lv_obj_t *header, bool found) {
    if (header == NULL) return;

    ui_header_data_t *data = (ui_header_data_t *)lv_obj_get_user_data(header);
    if (data == NULL || data->right_icons[0] == NULL) return;

    if (found) {
        // Compass found - show blue icon
        lv_obj_set_style_bg_color(data->right_icons[0], lv_color_hex(0x0080FF), 0);  // Blue
        lv_obj_set_style_border_color(data->right_icons[0], lv_color_hex(0x0060CC), 0);
        ESP_LOGD(TAG, "Compass status: FOUND");
    } else {
        // Compass not found - show gray icon
        lv_obj_set_style_bg_color(data->right_icons[0], lv_color_hex(0x808080), 0);  // Gray
        lv_obj_set_style_border_color(data->right_icons[0], lv_color_hex(0x555555), 0);
        ESP_LOGD(TAG, "Compass status: NOT FOUND");
    }
}
