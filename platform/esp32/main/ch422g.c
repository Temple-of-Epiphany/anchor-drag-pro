/*
 * CH422G I/O expander driver — implementation.
 *
 * See ch422g.h for usage. See WaveShare's lib/CH422G/ for the
 * register reference (the chip's quirky address-as-register scheme).
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#include "ch422g.h"
#include "i2c_bus.h"
#include "board_config.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "ch422g";

/* CH422G mode register bits (written to address 0x24). */
#define CH422G_MODE_IO_OE       0x01    /* Output enable (must be 1 for IO writes) */
#define CH422G_MODE_A_SCAN      0x02    /* Dynamic scan enable (unused on this board) */
#define CH422G_MODE_OD_EN       0x04    /* OC pin open-drain output enable */
#define CH422G_MODE_SLEEP       0x08    /* Low power sleep */

/* I2C clock for CH422G. Datasheet limit is 100 kHz; we ran at 400 kHz on the
 * shared bus historically with no issues, but staying conservative for the
 * device-specific transmission. */
#define CH422G_DEV_FREQ_HZ      100000

/* Mask of pins we are allowed to drive (EXIO0 reserved). */
#define CH422G_DRIVABLE_MASK    0xFEU

/* Per-address device handles on the shared I2C bus. */
static i2c_master_dev_handle_t s_dev_mode = NULL;   /* 0x24 — mode register */
static i2c_master_dev_handle_t s_dev_io   = NULL;   /* 0x38 — IO output / readback */

/* Cached state of the IO output register (write-only on the chip itself, but
 * we cache so single-pin writes don't need a read-modify-write). */
static uint8_t s_io_state = 0;
static bool    s_init     = false;

static esp_err_t add_dev(uint16_t addr, i2c_master_dev_handle_t *out)
{
    i2c_device_config_t cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = addr,
        .scl_speed_hz    = CH422G_DEV_FREQ_HZ,
    };
    return i2c_master_bus_add_device(i2c_bus_handle(), &cfg, out);
}

static esp_err_t write_byte(i2c_master_dev_handle_t dev, uint8_t value)
{
    /* Caller must already hold the I2C mutex. */
    return i2c_master_transmit(dev, &value, 1, 100 /* ms */);
}

esp_err_t ch422g_init(void)
{
    if (s_init) {
        ESP_LOGW(TAG, "already initialized");
        return ESP_OK;
    }
    if (i2c_bus_handle() == NULL) {
        ESP_LOGE(TAG, "i2c_bus not initialized; call i2c_bus_init() first");
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err;
    err = add_dev(I2C_ADDR_CH422G_MODE, &s_dev_mode);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "add mode dev failed: %s", esp_err_to_name(err));
        return err;
    }
    err = add_dev(I2C_ADDR_CH422G_IO, &s_dev_io);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "add io dev failed: %s", esp_err_to_name(err));
        i2c_master_bus_rm_device(s_dev_mode);
        s_dev_mode = NULL;
        return err;
    }

    /* Take the bus mutex for the init sequence: enable IO output, then
     * push the initial pin state. */
    if (!i2c_bus_lock(500)) {
        ESP_LOGE(TAG, "init: bus busy");
        return ESP_ERR_TIMEOUT;
    }

    err = write_byte(s_dev_mode, CH422G_MODE_IO_OE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "set mode failed: %s", esp_err_to_name(err));
        i2c_bus_unlock();
        return err;
    }

    s_io_state = (uint8_t) (CH422G_EXIO_INIT_STATE & CH422G_DRIVABLE_MASK);
    err = write_byte(s_dev_io, s_io_state);
    i2c_bus_unlock();

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "init IO write failed: %s", esp_err_to_name(err));
        return err;
    }

    s_init = true;
    ESP_LOGI(TAG, "init OK; initial IO state = 0x%02X", s_io_state);
    return ESP_OK;
}

esp_err_t ch422g_set_all(uint8_t io_mask)
{
    if (!s_init) return ESP_ERR_INVALID_STATE;
    io_mask &= CH422G_DRIVABLE_MASK;

    esp_err_t err = ESP_FAIL;
    if (i2c_bus_lock(100)) {
        err = write_byte(s_dev_io, io_mask);
        if (err == ESP_OK) {
            s_io_state = io_mask;
        }
        i2c_bus_unlock();
    }
    return err;
}

esp_err_t ch422g_set_pin(uint8_t pin, bool level)
{
    if (!s_init) return ESP_ERR_INVALID_STATE;
    if (pin > 7) return ESP_ERR_INVALID_ARG;

    uint8_t bit = (uint8_t) (1U << pin);
    if (!(bit & CH422G_DRIVABLE_MASK)) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t new_state = level ? (s_io_state | bit) : (s_io_state & ~bit);
    if (new_state == s_io_state) {
        return ESP_OK; /* no-op write */
    }

    esp_err_t err = ESP_FAIL;
    if (i2c_bus_lock(100)) {
        err = write_byte(s_dev_io, new_state);
        if (err == ESP_OK) {
            s_io_state = new_state;
        }
        i2c_bus_unlock();
    }
    return err;
}

esp_err_t ch422g_get_pin(uint8_t pin, bool *out_level)
{
    if (!s_init) return ESP_ERR_INVALID_STATE;
    if (pin > 7 || out_level == NULL) return ESP_ERR_INVALID_ARG;

    /* For output pins (which is all we use on this board), return the
     * cached state — that's the actual pin level. */
    *out_level = (s_io_state & (1U << pin)) != 0;
    return ESP_OK;
}

uint8_t ch422g_get_state_cached(void)
{
    return s_io_state;
}

/* ---- Named accessors ---- */

esp_err_t ch422g_lcd_backlight(bool on)
{
    return ch422g_set_pin(CH422G_EXIO_LCD_BACKLIGHT, on);
}

esp_err_t ch422g_lcd_reset_pulse(uint32_t hold_ms)
{
    esp_err_t err = ch422g_set_pin(CH422G_EXIO_LCD_RESET, false);
    if (err != ESP_OK) return err;
    vTaskDelay(pdMS_TO_TICKS(hold_ms));
    return ch422g_set_pin(CH422G_EXIO_LCD_RESET, true);
}

esp_err_t ch422g_touch_reset_pulse(uint32_t hold_ms)
{
    esp_err_t err = ch422g_set_pin(CH422G_EXIO_TOUCH_RESET, false);
    if (err != ESP_OK) return err;
    vTaskDelay(pdMS_TO_TICKS(hold_ms));
    return ch422g_set_pin(CH422G_EXIO_TOUCH_RESET, true);
}

esp_err_t ch422g_sd_cs(bool assert)
{
    /* SD CS is active LOW: assert = true → drive low. */
    return ch422g_set_pin(CH422G_EXIO_SD_CS, !assert);
}

esp_err_t ch422g_usb_can_select(bool can_mode)
{
    return ch422g_set_pin(CH422G_EXIO_USB_CAN_SEL, can_mode);
}
