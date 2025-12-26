/**
 * CH422G I/O Expander Driver Implementation
 *
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2025-12-25
 * Date Updated: 2025-12-26
 * Version: 0.2.0
 *
 * Now uses ESP_IO_Expander library for proper abstraction
 */

#include "ch422g.h"
#include "esp_log.h"
#include "board_config.h"
#include <string.h>

static const char *TAG = "ch422g";

// Global CH422G expander handle
esp_io_expander_handle_t ch422g_handle = NULL;

esp_err_t ch422g_init(i2c_port_t i2c_num) {
    ESP_LOGI(TAG, "Initializing CH422G I/O expander using ESP_IO_Expander library");

    // Create CH422G IO expander instance
    esp_err_t ret = esp_io_expander_new_i2c_ch422g(i2c_num, 0x24, &ch422g_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create CH422G expander: %s", esp_err_to_name(ret));
        return ret;
    }

    // Configure all pins as outputs
    ret = esp_io_expander_set_dir(ch422g_handle,
                                   TP_RST | LCD_BL | LCD_RST | SD_CS | USB_SEL,
                                   IO_EXPANDER_OUTPUT);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set pin directions: %s", esp_err_to_name(ret));
        return ret;
    }

    // Match Waveshare initialization: 0x0A = 0b00001010
    // - EXIO0 (bit 0): 0 = LOW (reserved)
    // - EXIO1 (bit 1): 1 = HIGH (Touch reset released)
    // - EXIO2 (bit 2): 0 = LOW (LCD backlight OFF initially - will be turned on by display driver)
    // - EXIO3 (bit 3): 1 = HIGH (LCD reset released)
    // - EXIO4 (bit 4): 0 = LOW (SD CS active - ready for SD access)
    // - EXIO5 (bit 5): 0 = LOW (USB mode, not CAN)

    // Set pins HIGH: TP_RST (bit 1), LCD_RST (bit 3)
    ret = esp_io_expander_set_level(ch422g_handle, TP_RST | LCD_RST, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set TP_RST|LCD_RST HIGH: %s", esp_err_to_name(ret));
        return ret;
    }

    // Set pins LOW: LCD_BL, SD_CS, USB_SEL (bits 2, 4, 5)
    ret = esp_io_expander_set_level(ch422g_handle, LCD_BL | SD_CS | USB_SEL, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set LCD_BL|SD_CS|USB_SEL LOW: %s", esp_err_to_name(ret));
        return ret;
    }

    // Verify final state
    uint32_t final_state;
    ret = esp_io_expander_get_level(ch422g_handle, 0x3F, &final_state);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "CH422G initialized - final state: 0x%02X (expected: 0x0A)", (uint8_t)final_state);
    }

    ESP_LOGI(TAG, "CH422G initialized successfully via ESP_IO_Expander");
    return ESP_OK;
}

esp_io_expander_handle_t ch422g_get_handle(void) {
    return ch422g_handle;
}
