/**
 * UI Header Component Implementation
 *
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2025-12-22
 * Date Updated: 2025-12-25
 * Version: 0.5.0
 *
 * Changelog:
 * - 0.5.0 (2025-12-25): Updated icons to GPS, Compass, TF Card, Anchor (armed), WiFi, Bluetooth
 * - 0.4.0 (2025-12-25): Filled all 6 status icons (Battery, SD Card, WiFi, Alarm, Compass, GPS)
 * - 0.3.0 (2025-12-25): Changed GPS icon to satellite emoji for better visibility
 * - 0.2.0 (2025-12-22): Redesigned as full-width header bar with title and 6 icon placeholders
 * - 0.1.0 (2025-12-22): Initial implementation with GPS and Compass icons only
 */

#include "ui_header.h"
#include "ui_theme.h"
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
    lv_obj_t *time_label;      // Time display (HH:MM:SS)
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

    // Title label: "ANCHOR DRAG ALARM" (center, top)
    data->title_label = lv_label_create(data->header_bar);
    lv_label_set_text(data->title_label, "ANCHOR DRAG ALARM");
    lv_obj_set_style_text_color(data->title_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(data->title_label, FONT_TITLE, 0);
    lv_obj_align(data->title_label, LV_ALIGN_CENTER, 0, -20);

    // Time label: "HH:MM:SS" (center, below title)
    data->time_label = lv_label_create(data->header_bar);
    lv_label_set_text(data->time_label, "--:--:--");
    lv_obj_set_style_text_color(data->time_label, lv_color_hex(0x39CCCC), 0);  // Teal accent
    lv_obj_set_style_text_font(data->time_label, &lv_font_montserrat_14, 0);
    lv_obj_align(data->time_label, LV_ALIGN_CENTER, 0, 20);

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

    // LEFT ICONS (Connectivity)
    // Left icon 0 - Bluetooth status
    lv_label_set_text(data->icon_labels[0], LV_SYMBOL_BLUETOOTH);
    lv_obj_set_style_bg_color(data->left_icons[0], lv_color_hex(0x808080), 0);  // Gray (disconnected)
    lv_obj_set_style_border_color(data->left_icons[0], lv_color_hex(0x555555), 0);

    // Left icon 1 - WiFi status
    lv_label_set_text(data->icon_labels[1], LV_SYMBOL_WIFI);
    lv_obj_set_style_bg_color(data->left_icons[1], lv_color_hex(0x808080), 0);  // Gray (disconnected)
    lv_obj_set_style_border_color(data->left_icons[1], lv_color_hex(0x555555), 0);

    // Left icon 2 - TF Card (SD Card) status
    lv_label_set_text(data->icon_labels[2], LV_SYMBOL_SD_CARD);
    lv_obj_set_style_bg_color(data->left_icons[2], lv_color_hex(0x808080), 0);  // Gray (not detected)
    lv_obj_set_style_border_color(data->left_icons[2], lv_color_hex(0x555555), 0);

    // RIGHT ICONS (Navigation/Sensors)
    // Right icon 2 - Anchor Armed status (third from right)
    lv_label_set_text(data->icon_labels[5], "\xE2\x9A\x93");  // ⚓ anchor emoji (UTF-8)
    lv_obj_set_style_bg_color(data->right_icons[2], lv_color_hex(0x808080), 0);  // Gray (not armed)
    lv_obj_set_style_border_color(data->right_icons[2], lv_color_hex(0x555555), 0);

    // Right icon 0 - Compass (second from right)
    lv_label_set_text(data->icon_labels[3], LV_SYMBOL_DIRECTORY);
    lv_obj_set_style_bg_color(data->right_icons[0], lv_color_hex(0x808080), 0);  // Gray (not found)

    // Right icon 1 - GPS/Satellite (rightmost)
    lv_label_set_text(data->icon_labels[4], "\xF0\x9F\x9B\xB0");  // 🛰️ satellite emoji (UTF-8)
    lv_obj_set_style_bg_color(data->right_icons[1], lv_color_hex(0x808080), 0);  // Gray (not found)

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
        // Compass found - show green icon
        lv_obj_set_style_bg_color(data->right_icons[0], lv_color_hex(0x00AA00), 0);  // Green
        lv_obj_set_style_border_color(data->right_icons[0], lv_color_hex(0x008800), 0);
        ESP_LOGD(TAG, "Compass status: FOUND");
    } else {
        // Compass not found - show gray icon
        lv_obj_set_style_bg_color(data->right_icons[0], lv_color_hex(0x808080), 0);  // Gray
        lv_obj_set_style_border_color(data->right_icons[0], lv_color_hex(0x555555), 0);
        ESP_LOGD(TAG, "Compass status: NOT FOUND");
    }
}

/**
 * Update Anchor Armed status icon (right side, position 2)
 */
void ui_header_set_anchor_armed(lv_obj_t *header, bool armed) {
    if (header == NULL) return;

    ui_header_data_t *data = (ui_header_data_t *)lv_obj_get_user_data(header);
    if (data == NULL || data->right_icons[2] == NULL) return;

    if (armed) {
        // Anchor armed - show green icon
        lv_obj_set_style_bg_color(data->right_icons[2], lv_color_hex(0x00AA00), 0);  // Green
        lv_obj_set_style_border_color(data->right_icons[2], lv_color_hex(0x008800), 0);
        ESP_LOGD(TAG, "Anchor status: ARMED");
    } else {
        // Anchor not armed - show gray icon
        lv_obj_set_style_bg_color(data->right_icons[2], lv_color_hex(0x808080), 0);  // Gray
        lv_obj_set_style_border_color(data->right_icons[2], lv_color_hex(0x555555), 0);
        ESP_LOGD(TAG, "Anchor status: NOT ARMED");
    }
}

/**
 * Update TF Card (SD Card) status icon (left side, position 2)
 */
void ui_header_set_tfcard_status(lv_obj_t *header, bool detected) {
    if (header == NULL) return;

    ui_header_data_t *data = (ui_header_data_t *)lv_obj_get_user_data(header);
    if (data == NULL || data->left_icons[2] == NULL) return;

    if (detected) {
        // TF card detected - show green icon
        lv_obj_set_style_bg_color(data->left_icons[2], lv_color_hex(0x00AA00), 0);  // Green
        lv_obj_set_style_border_color(data->left_icons[2], lv_color_hex(0x008800), 0);
        ESP_LOGD(TAG, "TF Card status: DETECTED");
    } else {
        // TF card not detected - show gray icon
        lv_obj_set_style_bg_color(data->left_icons[2], lv_color_hex(0x808080), 0);  // Gray
        lv_obj_set_style_border_color(data->left_icons[2], lv_color_hex(0x555555), 0);
        ESP_LOGD(TAG, "TF Card status: NOT DETECTED");
    }
}

/**
 * Update WiFi status icon (left side, position 1)
 */
void ui_header_set_wifi_status(lv_obj_t *header, bool connected) {
    if (header == NULL) return;

    ui_header_data_t *data = (ui_header_data_t *)lv_obj_get_user_data(header);
    if (data == NULL || data->left_icons[1] == NULL) return;

    if (connected) {
        // WiFi connected - show green icon
        lv_obj_set_style_bg_color(data->left_icons[1], lv_color_hex(0x00AA00), 0);  // Green
        lv_obj_set_style_border_color(data->left_icons[1], lv_color_hex(0x008800), 0);
        ESP_LOGD(TAG, "WiFi status: CONNECTED");
    } else {
        // WiFi disconnected - show gray icon
        lv_obj_set_style_bg_color(data->left_icons[1], lv_color_hex(0x808080), 0);  // Gray
        lv_obj_set_style_border_color(data->left_icons[1], lv_color_hex(0x555555), 0);
        ESP_LOGD(TAG, "WiFi status: DISCONNECTED");
    }
}

/**
 * Update Bluetooth status icon (left side, position 0)
 */
void ui_header_set_bluetooth_status(lv_obj_t *header, bool connected) {
    if (header == NULL) return;

    ui_header_data_t *data = (ui_header_data_t *)lv_obj_get_user_data(header);
    if (data == NULL || data->left_icons[0] == NULL) return;

    if (connected) {
        // Bluetooth connected - show blue icon
        lv_obj_set_style_bg_color(data->left_icons[0], lv_color_hex(0x0080FF), 0);  // Blue
        lv_obj_set_style_border_color(data->left_icons[0], lv_color_hex(0x0060CC), 0);
        ESP_LOGD(TAG, "Bluetooth status: CONNECTED");
    } else {
        // Bluetooth disconnected - show gray icon
        lv_obj_set_style_bg_color(data->left_icons[0], lv_color_hex(0x808080), 0);  // Gray
        lv_obj_set_style_border_color(data->left_icons[0], lv_color_hex(0x555555), 0);
        ESP_LOGD(TAG, "Bluetooth status: DISCONNECTED");
    }
}

/**
 * Update time display in header
 */
bool ui_header_set_time(lv_obj_t *header, int hour, int min, int sec) {
    if (header == NULL) {
        return false;
    }

    ui_header_data_t *data = (ui_header_data_t *)lv_obj_get_user_data(header);
    if (data == NULL) {
        return false;
    }

    if (data->time_label == NULL) {
        return false;
    }

    // Update time label with HH:MM:SS format
    // Use snprintf instead of lv_label_set_text_fmt to avoid visual artifacts
    char time_str[16];
    snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d", hour, min, sec);

    // Only update if the text has changed to prevent unnecessary redraws and blinking
    const char *current_text = lv_label_get_text(data->time_label);
    if (current_text == NULL || strcmp(current_text, time_str) != 0) {
        lv_label_set_text(data->time_label, time_str);
    }

    return true;
}
