/**
 * ESP IO Expander - CH422G Device Driver Implementation
 *
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2025-12-26
 * Version: 0.1.0
 */

#include <string.h>
#include "esp_log.h"
#include "esp_check.h"
#include "esp_io_expander_ch422g.h"

static const char *TAG = "ch422g_expander";

// CH422G I2C addresses
#define CH422G_READ_ADDR    0x24
#define CH422G_WRITE_ADDR   0x38

// CH422G has 6 I/O pins (EXIO0-EXIO5)
#define CH422G_IO_COUNT     6

// Device-specific data - "inherits" from esp_io_expander_t
typedef struct {
    esp_io_expander_t base;     // MUST be first for casting
    i2c_port_t i2c_num;
    uint8_t i2c_address;
    uint8_t direction_mask;     // 0 = output, 1 = input
    uint8_t output_state;       // Current output state
} ch422g_dev_t;

// Forward declarations of interface functions
static esp_err_t ch422g_read_input_reg(esp_io_expander_handle_t handle, uint32_t *value);
static esp_err_t ch422g_write_output_reg(esp_io_expander_handle_t handle, uint32_t value);
static esp_err_t ch422g_read_output_reg(esp_io_expander_handle_t handle, uint32_t *value);
static esp_err_t ch422g_write_direction_reg(esp_io_expander_handle_t handle, uint32_t value);
static esp_err_t ch422g_read_direction_reg(esp_io_expander_handle_t handle, uint32_t *value);
static esp_err_t ch422g_del(esp_io_expander_handle_t handle);
static esp_err_t ch422g_reset(esp_io_expander_handle_t handle);

esp_err_t esp_io_expander_new_i2c_ch422g(i2c_port_t i2c_num, uint8_t i2c_address, esp_io_expander_handle_t *handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "Invalid handle");

    // Allocate device structure (includes base + device-specific fields)
    ch422g_dev_t *dev = calloc(1, sizeof(ch422g_dev_t));
    ESP_RETURN_ON_FALSE(dev, ESP_ERR_NO_MEM, TAG, "Malloc failed");

    // Initialize device-specific fields
    dev->i2c_num = i2c_num;
    dev->i2c_address = i2c_address;
    dev->direction_mask = 0x00;  // All outputs by default
    dev->output_state = 0x00;    // All low by default

    // Initialize base structure config
    dev->base.config.io_count = CH422G_IO_COUNT;
    dev->base.config.flags.dir_out_bit_zero = 0;      // 1 = output
    dev->base.config.flags.output_high_bit_zero = 0;  // 1 = high
    dev->base.config.flags.input_high_bit_zero = 0;   // 1 = high

    // Set interface functions
    dev->base.read_input_reg = ch422g_read_input_reg;
    dev->base.write_output_reg = ch422g_write_output_reg;
    dev->base.read_output_reg = ch422g_read_output_reg;
    dev->base.write_direction_reg = ch422g_write_direction_reg;
    dev->base.read_direction_reg = ch422g_read_direction_reg;
    dev->base.del = ch422g_del;
    dev->base.reset = ch422g_reset;

    // Initialize CH422G - write initial state
    uint8_t write_buf = 0x01;
    esp_err_t ret = i2c_master_write_to_device(i2c_num, CH422G_READ_ADDR, &write_buf, 1, pdMS_TO_TICKS(1000));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize CH422G (read address): %s", esp_err_to_name(ret));
        free(dev);
        return ret;
    }

    write_buf = 0x0A;  // Initial state: EXIO1=1, EXIO3=1, others=0
    ret = i2c_master_write_to_device(i2c_num, CH422G_WRITE_ADDR, &write_buf, 1, pdMS_TO_TICKS(1000));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize CH422G (write address): %s", esp_err_to_name(ret));
        free(dev);
        return ret;
    }

    dev->output_state = 0x0A;

    // Return handle as pointer to base structure
    *handle = &dev->base;
    ESP_LOGI(TAG, "CH422G IO expander initialized successfully");
    return ESP_OK;
}

static esp_err_t ch422g_read_input_reg(esp_io_expander_handle_t handle, uint32_t *value)
{
    ESP_RETURN_ON_FALSE(handle && value, ESP_ERR_INVALID_ARG, TAG, "Invalid argument");

    ch422g_dev_t *dev = (ch422g_dev_t *)handle;  // Cast base to derived
    uint8_t read_buf;

    esp_err_t ret = i2c_master_read_from_device(dev->i2c_num, dev->i2c_address, &read_buf, 1, pdMS_TO_TICKS(1000));
    ESP_RETURN_ON_ERROR(ret, TAG, "Read input failed");

    *value = read_buf & 0x3F;  // Mask to 6 bits (EXIO0-EXIO5)
    return ESP_OK;
}

static esp_err_t ch422g_write_output_reg(esp_io_expander_handle_t handle, uint32_t value)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "Invalid argument");

    ch422g_dev_t *dev = (ch422g_dev_t *)handle;  // Cast base to derived
    uint8_t write_buf = value & 0x3F;  // Mask to 6 bits

    esp_err_t ret = i2c_master_write_to_device(dev->i2c_num, CH422G_WRITE_ADDR, &write_buf, 1, pdMS_TO_TICKS(1000));
    ESP_RETURN_ON_ERROR(ret, TAG, "Write output failed");

    dev->output_state = write_buf;
    ESP_LOGD(TAG, "Write output: 0x%02X", write_buf);
    return ESP_OK;
}

static esp_err_t ch422g_read_output_reg(esp_io_expander_handle_t handle, uint32_t *value)
{
    ESP_RETURN_ON_FALSE(handle && value, ESP_ERR_INVALID_ARG, TAG, "Invalid argument");

    ch422g_dev_t *dev = (ch422g_dev_t *)handle;  // Cast base to derived
    *value = dev->output_state;  // Return cached state
    return ESP_OK;
}

static esp_err_t ch422g_write_direction_reg(esp_io_expander_handle_t handle, uint32_t value)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "Invalid argument");

    ch422g_dev_t *dev = (ch422g_dev_t *)handle;  // Cast base to derived
    dev->direction_mask = value & 0x3F;

    // CH422G doesn't have a direction register - all pins are bidirectional
    // This is just for tracking purposes
    ESP_LOGD(TAG, "Direction mask updated: 0x%02X", dev->direction_mask);
    return ESP_OK;
}

static esp_err_t ch422g_read_direction_reg(esp_io_expander_handle_t handle, uint32_t *value)
{
    ESP_RETURN_ON_FALSE(handle && value, ESP_ERR_INVALID_ARG, TAG, "Invalid argument");

    ch422g_dev_t *dev = (ch422g_dev_t *)handle;  // Cast base to derived
    *value = dev->direction_mask;
    return ESP_OK;
}

static esp_err_t ch422g_reset(esp_io_expander_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "Invalid argument");

    ch422g_dev_t *dev = (ch422g_dev_t *)handle;  // Cast base to derived

    // Reset to default state
    uint8_t write_buf = 0x01;
    esp_err_t ret = i2c_master_write_to_device(dev->i2c_num, CH422G_READ_ADDR, &write_buf, 1, pdMS_TO_TICKS(1000));
    ESP_RETURN_ON_ERROR(ret, TAG, "Reset read address failed");

    write_buf = 0x00;
    ret = i2c_master_write_to_device(dev->i2c_num, CH422G_WRITE_ADDR, &write_buf, 1, pdMS_TO_TICKS(1000));
    ESP_RETURN_ON_ERROR(ret, TAG, "Reset write address failed");

    dev->output_state = 0x00;
    dev->direction_mask = 0x00;

    ESP_LOGI(TAG, "CH422G reset complete");
    return ESP_OK;
}

static esp_err_t ch422g_del(esp_io_expander_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "Invalid argument");

    ch422g_dev_t *dev = (ch422g_dev_t *)handle;  // Cast base to derived
    free(dev);  // Free entire structure (includes base)

    ESP_LOGI(TAG, "CH422G IO expander deleted");
    return ESP_OK;
}
