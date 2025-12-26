/**
 * CH422G I/O Expander Driver Implementation
 *
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2025-12-25
 * Version: 0.1.0
 */

#include "ch422g.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "ch422g";
static uint8_t current_state = 0xFF;  // All pins high by default

esp_err_t ch422g_init(i2c_port_t i2c_num) {
    ESP_LOGI(TAG, "Initializing CH422G I/O expander");

    // Waveshare demo sequence for SD card CS control
    // First write 0x01 to read address 0x24
    uint8_t write_buf = 0x01;
    esp_err_t ret = i2c_master_write_to_device(i2c_num, 0x24, &write_buf, 1, pdMS_TO_TICKS(1000));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write to CH422G read address: %s", esp_err_to_name(ret));
        return ret;
    }

    // Then write 0x0A to write address 0x38
    // 0x0A = 0b00001010:
    //   EXIO0 (bit 0): 0 = LOW
    //   EXIO1 (bit 1): 1 = HIGH (Touch reset released)
    //   EXIO2 (bit 2): 0 = LOW  (LCD backlight off initially)
    //   EXIO3 (bit 3): 1 = HIGH (LCD reset released)
    //   EXIO4 (bit 4): 0 = LOW  (SD card CS active/enabled)
    //   EXIO5 (bit 5): 0 = LOW
    write_buf = 0x0A;
    ret = i2c_master_write_to_device(i2c_num, 0x38, &write_buf, 1, pdMS_TO_TICKS(1000));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write to CH422G write address: %s", esp_err_to_name(ret));
        return ret;
    }

    current_state = 0x0A;
    ESP_LOGI(TAG, "CH422G initialized with state: 0x%02X", current_state);
    return ESP_OK;
}

esp_err_t ch422g_write(i2c_port_t i2c_num, uint8_t value) {
    // Use same API as Waveshare demo - write to address 0x38
    esp_err_t ret = i2c_master_write_to_device(i2c_num, CH422G_ADDR_WRITE, &value, 1, pdMS_TO_TICKS(1000));

    if (ret == ESP_OK) {
        current_state = value;
        ESP_LOGD(TAG, "CH422G write: 0x%02X", value);
    } else {
        ESP_LOGE(TAG, "CH422G write failed: %s", esp_err_to_name(ret));
    }

    return ret;
}

esp_err_t ch422g_set_pin(i2c_port_t i2c_num, uint8_t pin) {
    uint8_t new_state = current_state | pin;
    return ch422g_write(i2c_num, new_state);
}

esp_err_t ch422g_clear_pin(i2c_port_t i2c_num, uint8_t pin) {
    uint8_t new_state = current_state & ~pin;
    return ch422g_write(i2c_num, new_state);
}

esp_err_t ch422g_read(i2c_port_t i2c_num, uint8_t *value) {
    if (value == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // Use same API as Waveshare demo - read from address 0x24
    esp_err_t ret = i2c_master_read_from_device(i2c_num, CH422G_ADDR_READ, value, 1, pdMS_TO_TICKS(1000));

    if (ret == ESP_OK) {
        ESP_LOGD(TAG, "CH422G read: 0x%02X", *value);
    } else {
        ESP_LOGE(TAG, "CH422G read failed: %s", esp_err_to_name(ret));
    }

    return ret;
}
