/**
 * RGB LCD Display Driver Implementation
 *
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2025-12-20
 * Date Updated: 2025-12-20
 * Version: 0.1.1
 */

#include "display_driver.h"
#include "board_config.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "display_driver";

// Display panel handle
esp_lcd_panel_handle_t display_panel = NULL;

/**
 * Initialize RGB LCD display
 */
esp_err_t display_init(void) {
    esp_err_t ret;

    ESP_LOGI(TAG, "Initializing RGB LCD display (ST7262, %dx%d, RGB%d)",
             LCD_WIDTH, LCD_HEIGHT, LCD_COLOR_BITS);

    // Configure RGB panel
    esp_lcd_rgb_panel_config_t panel_config = {
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .timings = {
            .pclk_hz = LCD_PIXEL_CLOCK_HZ,
            .h_res = LCD_WIDTH,
            .v_res = LCD_HEIGHT,
            .hsync_pulse_width = LCD_HPW,
            .hsync_back_porch = LCD_HBP,
            .hsync_front_porch = LCD_HFP,
            .vsync_pulse_width = LCD_VPW,
            .vsync_back_porch = LCD_VBP,
            .vsync_front_porch = LCD_VFP,
            .flags = {
                .hsync_idle_low = 0,
                .vsync_idle_low = 0,
                .de_idle_high = 0,
                .pclk_active_neg = 0,
                .pclk_idle_high = 0,
            },
        },
        .data_width = LCD_RGB_DATA_WIDTH,
        .bits_per_pixel = LCD_COLOR_BITS,
        .num_fbs = 1,  // Single frame buffer
        .bounce_buffer_size_px = LCD_BOUNCE_BUFFER_SIZE,
        .sram_trans_align = 4,
        .psram_trans_align = 64,
        .hsync_gpio_num = LCD_PIN_HSYNC,
        .vsync_gpio_num = LCD_PIN_VSYNC,
        .de_gpio_num = LCD_PIN_DE,
        .pclk_gpio_num = LCD_PIN_PCLK,
        .disp_gpio_num = -1,  // Not used (controlled via CH422G)
        .data_gpio_nums = {
            // Blue channel (5 bits: B3-B7)
            LCD_PIN_B3,   // D0
            LCD_PIN_B4,   // D1
            LCD_PIN_B5,   // D2
            LCD_PIN_B6,   // D3
            LCD_PIN_B7,   // D4
            // Green channel (6 bits: G2-G7)
            LCD_PIN_G2,   // D5
            LCD_PIN_G3,   // D6
            LCD_PIN_G4,   // D7
            LCD_PIN_G5,   // D8
            LCD_PIN_G6,   // D9
            LCD_PIN_G7,   // D10
            // Red channel (5 bits: R3-R7)
            LCD_PIN_R3,   // D11
            LCD_PIN_R4,   // D12
            LCD_PIN_R5,   // D13
            LCD_PIN_R6,   // D14
            LCD_PIN_R7,   // D15
        },
        .flags = {
            .fb_in_psram = 1,  // Allocate frame buffer in PSRAM
        },
    };

    // Create RGB panel
    ret = esp_lcd_new_rgb_panel(&panel_config, &display_panel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create RGB panel: %s", esp_err_to_name(ret));
        return ret;
    }

    // Initialize display
    ret = esp_lcd_panel_init(display_panel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize panel: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "RGB LCD initialized successfully");
    ESP_LOGI(TAG, "Note: RGB panels are always on. Use CH422G EXIO2 to control backlight.");

    return ESP_OK;
}

/**
 * Get display panel handle
 */
esp_lcd_panel_handle_t display_get_panel(void) {
    return display_panel;
}

/**
 * Get display width
 */
int display_get_width(void) {
    return LCD_WIDTH;
}

/**
 * Get display height
 */
int display_get_height(void) {
    return LCD_HEIGHT;
}
