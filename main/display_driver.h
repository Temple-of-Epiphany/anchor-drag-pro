/**
 * RGB LCD Display Driver for ST7262
 *
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2025-12-20
 * Date Updated: 2025-12-20
 * Version: 0.1.1
 *
 * Initializes the ST7262 RGB LCD (800x480, 16-bit parallel interface)
 * for the Waveshare ESP32-S3-Touch-LCD-4.3B board.
 */

#ifndef DISPLAY_DRIVER_H
#define DISPLAY_DRIVER_H

#include "esp_err.h"
#include "esp_lcd_panel_ops.h"

// Display handle (opaque pointer to esp_lcd_panel_handle_t)
extern esp_lcd_panel_handle_t display_panel;

/**
 * Initialize the RGB LCD display
 *
 * Configures the ESP32-S3 RGB LCD peripheral with the following:
 * - Resolution: 800x480 pixels
 * - Color depth: RGB565 (16-bit)
 * - Interface: 16-bit parallel RGB
 * - Pixel clock: 16 MHz
 * - Bounce buffer to prevent display drift
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t display_init(void);

/**
 * Get the display panel handle
 *
 * @return Pointer to the LCD panel handle
 */
esp_lcd_panel_handle_t display_get_panel(void);

/**
 * Get display width
 *
 * @return Display width in pixels
 */
int display_get_width(void);

/**
 * Get display height
 *
 * @return Display height in pixels
 */
int display_get_height(void);

#endif // DISPLAY_DRIVER_H
