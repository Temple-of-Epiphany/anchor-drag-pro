/*
 * GT911 touch driver — real implementation (#69 m4).
 *
 * Replaces the v0.1 archive driver (which used legacy driver/i2c.h) with
 * one built on the new ESP-IDF i2c_master API + shared i2c_bus, and the
 * existing ch422g_touch_reset_pulse() for the hardware reset sequence.
 *
 * Pin map (board_config.h):
 *   I²C bus       — shared (i2c_bus.h), 400 kHz, GPIO 8/9
 *   GT911 addr    — 0x5D (I2C_ADDR_GT911_TOUCH)
 *   IRQ           — GPIO 4 (TOUCH_INT_GPIO) — input, pulled by GT911
 *   RST           — via CH422G EXIO1 (CH422G_EXIO_TOUCH_RESET)
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#include "touch_driver.h"
#include "board_config.h"
#include "ch422g.h"
#include "i2c_bus.h"
#include "esp_log.h"
#include "esp_lcd_io_i2c.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_touch_gt911.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "touch_driver";

static esp_lcd_touch_handle_t s_touch_handle = NULL;

static esp_err_t touch_int_gpio_output_low(void)
{
    gpio_config_t cfg = {
        .intr_type    = GPIO_INTR_DISABLE,
        .mode         = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << TOUCH_INT_GPIO),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
    };
    esp_err_t e = gpio_config(&cfg);
    if (e == ESP_OK) gpio_set_level(TOUCH_INT_GPIO, 0);
    return e;
}

static esp_err_t touch_int_gpio_input(void)
{
    gpio_config_t cfg = {
        .intr_type    = GPIO_INTR_DISABLE,
        .mode         = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << TOUCH_INT_GPIO),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
    };
    return gpio_config(&cfg);
}

/* GT911 reset+address-select sequence per datasheet:
 *
 *   1. INT pin driven LOW (address-select line)
 *   2. RST pulled LOW (≥ 10 µs)
 *   3. RST released HIGH while INT still LOW → chip latches addr 0x5D
 *   4. Hold INT LOW for ≥ 55 ms (chip enters normal mode)
 *   5. Reconfigure INT as input
 *
 * Our RST is on CH422G EXIO1, which uses ch422g_set_pin(level=false=LOW).
 * Don't use ch422g_touch_reset_pulse here — that helper releases RST
 * before we'd have INT held, which would latch the wrong address. */
static esp_err_t touch_reset_with_address_select(void)
{
    esp_err_t e;

    /* Step 1: drive INT low. */
    e = touch_int_gpio_output_low();
    if (e != ESP_OK) return e;

    /* Step 2: drive RST low. */
    e = ch422g_set_pin(CH422G_EXIO_TOUCH_RESET, false);
    if (e != ESP_OK) return e;
    vTaskDelay(pdMS_TO_TICKS(5));

    /* Step 3: release RST while INT still low → addr 0x5D latched. */
    e = ch422g_set_pin(CH422G_EXIO_TOUCH_RESET, true);
    if (e != ESP_OK) return e;

    /* Step 4: hold INT low long enough for normal-mode entry. */
    vTaskDelay(pdMS_TO_TICKS(60));

    /* Step 5: hand INT back as an input — chip will use it for IRQs. */
    return touch_int_gpio_input();
}

esp_err_t touch_init(void)
{
    if (s_touch_handle) {
        ESP_LOGW(TAG, "already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "init: GT911 @ 0x%02X, IRQ GPIO%d, RST via CH422G EXIO%d",
             I2C_ADDR_GT911_TOUCH, TOUCH_INT_GPIO, CH422G_EXIO_TOUCH_RESET);

    /* GT911 reset + address selection — INT pin must be held LOW across
     * the RST release to latch the 0x5D address (which is what's on the
     * I²C bus and what ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG uses). */
    esp_err_t err = touch_reset_with_address_select();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "reset+address-select failed: %s", esp_err_to_name(err));
        return err;
    }

    /* Wrap the shared i2c_master bus in an esp_lcd panel-io.
     *
     * The GT911 config macro doesn't set scl_speed_hz; the new
     * i2c_master API rejects device adds when it's 0. Set it
     * explicitly to the bus rate (400 kHz). */
    esp_lcd_panel_io_handle_t io = NULL;
    esp_lcd_panel_io_i2c_config_t io_cfg = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
    io_cfg.scl_speed_hz = I2C0_FREQ_HZ;
    err = esp_lcd_new_panel_io_i2c_v2(i2c_bus_handle(), &io_cfg, &io);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "panel_io create failed: %s", esp_err_to_name(err));
        return err;
    }

    esp_lcd_touch_config_t cfg = {
        .x_max         = LCD_WIDTH,
        .y_max         = LCD_HEIGHT,
        .rst_gpio_num  = -1,                /* reset via CH422G — not a GPIO */
        .int_gpio_num  = TOUCH_INT_GPIO,
        .levels = {
            .reset     = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy   = 0,
            .mirror_x  = 0,
            .mirror_y  = 0,
        },
    };
    err = esp_lcd_touch_new_i2c_gt911(io, &cfg, &s_touch_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "gt911 create failed: %s", esp_err_to_name(err));
        s_touch_handle = NULL;
        return err;
    }

    ESP_LOGI(TAG, "GT911 ready (%dx%d, up to %d touch points)",
             LCD_WIDTH, LCD_HEIGHT, CONFIG_ESP_LCD_TOUCH_MAX_POINTS);
    return ESP_OK;
}

esp_lcd_touch_handle_t touch_get_handle(void)
{
    return s_touch_handle;
}

esp_err_t touch_reset(void)
{
    return ch422g_touch_reset_pulse(100);
}
