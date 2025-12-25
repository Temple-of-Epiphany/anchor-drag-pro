/**
 * GT911 Touch Driver Implementation
 *
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2025-12-23
 * Date Updated: 2025-12-23
 * Version: 0.1.0
 *
 * Based on Waveshare ESP32-S3-Touch-LCD-4.3B demo code
 */

#include "touch_driver.h"
#include "board_config.h"
#include "esp_log.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_touch_gt911.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h"

static const char *TAG = "touch_driver";

// Touch panel handle
static esp_lcd_touch_handle_t touch_handle = NULL;

/**
 * Initialize GPIO for touch interrupt
 * Note: Temporarily set as OUTPUT for reset sequence, then INPUT for normal operation
 */
static esp_err_t touch_gpio_init(void) {
    // First configure as OUTPUT for reset sequence
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .pin_bit_mask = (1ULL << TOUCH_INT_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };

    return gpio_config(&io_conf);
}

/**
 * Reconfigure GPIO for touch interrupt as INPUT after reset
 */
static esp_err_t touch_gpio_set_input(void) {
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .pin_bit_mask = (1ULL << TOUCH_INT_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };

    return gpio_config(&io_conf);
}

/**
 * Reset touch controller via CH422G I/O expander
 * Based on Waveshare waveshare_esp32_s3_touch_reset()
 */
esp_err_t touch_reset(void) {
    esp_err_t ret;
    uint8_t write_buf;

    ESP_LOGI(TAG, "Resetting GT911 touch controller via CH422G");

    // Step 1: Configure CH422G to output mode
    write_buf = 0x01;
    ret = i2c_master_write_to_device(I2C_MASTER_NUM, I2C_ADDR_CH422G,
                                      &write_buf, 1,
                                      I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure CH422G to output mode: %s", esp_err_to_name(ret));
        return ret;
    }

    // Step 2: Send reset sequence to GT911 via address 0x38
    write_buf = 0x2C;
    ret = i2c_master_write_to_device(I2C_MASTER_NUM, 0x38,
                                      &write_buf, 1,
                                      I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write 0x2C to GT911: %s", esp_err_to_name(ret));
        return ret;
    }

    esp_rom_delay_us(100 * 1000);  // 100ms delay

    // Step 3: Toggle GPIO4 (touch interrupt pin)
    gpio_set_level(TOUCH_INT_PIN, 0);
    esp_rom_delay_us(100 * 1000);  // 100ms delay

    // Step 4: Complete reset sequence
    write_buf = 0x2E;
    ret = i2c_master_write_to_device(I2C_MASTER_NUM, 0x38,
                                      &write_buf, 1,
                                      I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write 0x2E to GT911: %s", esp_err_to_name(ret));
        return ret;
    }

    esp_rom_delay_us(200 * 1000);  // 200ms delay

    // Step 5: Switch GPIO4 back to INPUT mode for touch interrupt
    ret = touch_gpio_set_input();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure GPIO4 as input: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "GT911 reset sequence completed, GPIO4 configured as input");
    return ESP_OK;
}

/**
 * Initialize GT911 touch controller
 */
esp_err_t touch_init(void) {
    esp_err_t ret;

    ESP_LOGI(TAG, "Initializing GT911 touch controller");
    ESP_LOGI(TAG, "I2C Bus: I2C%d (GPIO%d SDA, GPIO%d SCL)",
             I2C_MASTER_NUM, I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);
    ESP_LOGI(TAG, "Touch INT: GPIO%d", TOUCH_INT_PIN);
    ESP_LOGI(TAG, "Touch Address: 0x%02X", I2C_ADDR_GT911);

    // Initialize GPIO for touch interrupt
    ret = touch_gpio_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize GPIO: %s", esp_err_to_name(ret));
        return ret;
    }

    // Reset touch controller
    ret = touch_reset();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Touch reset failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Create I2C panel IO handle for touch
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();

    ret = esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)I2C_MASTER_NUM,
                                    &tp_io_config, &tp_io_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create I2C panel IO: %s", esp_err_to_name(ret));
        return ret;
    }

    // Configure GT911 touch controller
    esp_lcd_touch_config_t tp_cfg = {
        .x_max = LCD_WIDTH,
        .y_max = LCD_HEIGHT,
        .rst_gpio_num = TOUCH_RST_PIN,  // -1 (controlled via CH422G)
        .int_gpio_num = TOUCH_INT_PIN,  // GPIO4
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
    };

    // Create GT911 touch controller
    ret = esp_lcd_touch_new_i2c_gt911(tp_io_handle, &tp_cfg, &touch_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create GT911 touch controller: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "GT911 touch controller initialized successfully");
    ESP_LOGI(TAG, "Touch resolution: %dx%d", LCD_WIDTH, LCD_HEIGHT);
    ESP_LOGI(TAG, "Max touch points: %d", TOUCH_POINTS_MAX);

    return ESP_OK;
}

/**
 * Get touch panel handle
 */
esp_lcd_touch_handle_t touch_get_handle(void) {
    return touch_handle;
}
