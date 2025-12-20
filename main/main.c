/**
 * Anchor Drag Pro - Main Application
 *
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2025-12-19
 * Date Updated: 2025-12-19
 *
 * Version: 0.1.1
 *
 * Changelog:
 * - 0.1.1 (2025-12-19): Added splash screen with self-test
 * - 0.1.0 (2025-12-19): Initial version with board configuration and boot info display
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "board_config.h"
#include "splash_screen.h"
#include "ui_version.h"

static const char *TAG = "anchor-drag-pro";

/**
 * Print banner separator line
 */
static void print_banner_line(void) {
    printf("================================================================================\n");
}

/**
 * Print centered text with padding
 */
static void print_centered(const char *text) {
    int padding = (80 - strlen(text)) / 2;
    printf("%*s%s\n", padding, "", text);
}

/**
 * Display firmware version information
 */
static void display_version_info(void) {
    print_banner_line();
    print_centered("ANCHOR DRAG PRO - MARINE SAFETY SYSTEM");
    print_banner_line();

    printf("Firmware Version:    %s\n", FW_VERSION_STRING);
    printf("Build Date:          %s\n", FW_BUILD_DATE);
    printf("Build Time:          %s\n", FW_BUILD_TIME);
    printf("Board:               %s (%s)\n", BOARD_NAME, BOARD_VARIANT);
    printf("MCU:                 %s\n", MCU_NAME);
    printf("\n");

    ESP_LOGI(TAG, "Firmware v%s built %s %s", FW_VERSION_STRING, FW_BUILD_DATE, FW_BUILD_TIME);
}

/**
 * Display ESP32-S3 chip information
 */
static void display_chip_info(void) {
    esp_chip_info_t chip_info;
    uint32_t flash_size;

    esp_chip_info(&chip_info);
    esp_flash_get_size(NULL, &flash_size);

    print_banner_line();
    print_centered("CHIP INFORMATION");
    print_banner_line();

    printf("Chip Model:          ESP32-S3\n");
    printf("CPU Cores:           %d\n", chip_info.cores);
    printf("CPU Frequency:       240 MHz\n");
    printf("Flash Size:          %lu MB (%s)\n",
           flash_size / (1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
    printf("PSRAM:               8 MB\n");
    printf("WiFi:                %s\n", (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "Yes" : "No");
    printf("Bluetooth:           %s\n", (chip_info.features & CHIP_FEATURE_BT) ? "Yes" : "No");
    printf("Silicon Revision:    %d\n", chip_info.revision);
    printf("\n");
}

/**
 * Display enabled libraries and features
 */
static void display_libraries(void) {
    print_banner_line();
    print_centered("COMPILED LIBRARIES AND FEATURES");
    print_banner_line();

    printf("1. FreeRTOS          [ENABLED]  - Real-time operating system\n");
    printf("2. ESP-IDF           [ENABLED]  - Espressif IoT Development Framework\n");

    #if ENABLE_LVGL
    printf("3. LVGL 9.2.0        [ENABLED]  - Graphics library\n");
    #else
    printf("3. LVGL 9.2.0        [DISABLED] - Graphics library\n");
    #endif

    #if ENABLE_LVGL_ANIMATIONS
    printf("4. LVGL Animations   [ENABLED]  - UI animation support\n");
    #else
    printf("4. LVGL Animations   [DISABLED] - UI animation support\n");
    #endif

    #if ENABLE_PNG_DECODER
    printf("5. PNG Decoder       [ENABLED]  - PNG image format support\n");
    #else
    printf("5. PNG Decoder       [DISABLED] - PNG image format support\n");
    #endif

    #if ENABLE_BMP_DECODER
    printf("6. BMP Decoder       [ENABLED]  - BMP image format support\n");
    #else
    printf("6. BMP Decoder       [DISABLED] - BMP image format support\n");
    #endif

    #if ENABLE_CAN_BUS
    printf("7. CAN/TWAI          [ENABLED]  - NMEA 2000 support\n");
    #else
    printf("7. CAN/TWAI          [DISABLED] - NMEA 2000 support\n");
    #endif

    #if ENABLE_RS485
    printf("8. RS485             [ENABLED]  - NMEA 0183 support\n");
    #else
    printf("8. RS485             [DISABLED] - NMEA 0183 support\n");
    #endif

    #if ENABLE_SD_CARD
    printf("9. SD Card           [ENABLED]  - Data logging\n");
    #else
    printf("9. SD Card           [DISABLED] - Data logging\n");
    #endif

    #if ENABLE_WIFI
    printf("10. WiFi             [ENABLED]  - Wireless connectivity\n");
    #else
    printf("10. WiFi             [DISABLED] - Wireless connectivity\n");
    #endif

    #if ENABLE_BLUETOOTH
    printf("11. Bluetooth LE     [ENABLED]  - BLE connectivity\n");
    #else
    printf("11. Bluetooth LE     [DISABLED] - BLE connectivity\n");
    #endif

    printf("\n");
}

/**
 * Display pin allocation for all peripherals
 */
static void display_pin_allocation(void) {
    print_banner_line();
    print_centered("GPIO PIN ALLOCATION");
    print_banner_line();

    printf("\n=== Display (ST7262 RGB LCD) ===\n");
    printf("Resolution:          %dx%d pixels (%d-bit color)\n", LCD_WIDTH, LCD_HEIGHT, LCD_COLOR_BITS);
    printf("VSYNC:               GPIO%d\n", LCD_PIN_VSYNC);
    printf("HSYNC:               GPIO%d\n", LCD_PIN_HSYNC);
    printf("DE (Data Enable):    GPIO%d\n", LCD_PIN_DE);
    printf("PCLK (Pixel Clock):  GPIO%d\n", LCD_PIN_PCLK);

    printf("\nBlue Channel (5-bit):\n");
    printf("  B3: GPIO%d, B4: GPIO%d, B5: GPIO%d, B6: GPIO%d, B7: GPIO%d\n",
           LCD_PIN_B3, LCD_PIN_B4, LCD_PIN_B5, LCD_PIN_B6, LCD_PIN_B7);

    printf("Green Channel (6-bit):\n");
    printf("  G2: GPIO%d, G3: GPIO%d, G4: GPIO%d, G5: GPIO%d, G6: GPIO%d, G7: GPIO%d\n",
           LCD_PIN_G2, LCD_PIN_G3, LCD_PIN_G4, LCD_PIN_G5, LCD_PIN_G6, LCD_PIN_G7);

    printf("Red Channel (5-bit):\n");
    printf("  R3: GPIO%d, R4: GPIO%d, R5: GPIO%d, R6: GPIO%d, R7: GPIO%d\n",
           LCD_PIN_R3, LCD_PIN_R4, LCD_PIN_R5, LCD_PIN_R6, LCD_PIN_R7);

    printf("\n=== I2C Buses ===\n");
    printf("I2C0 (External):     SDA=GPIO%d, SCL=GPIO%d (%d kHz)\n",
           I2C0_SDA_PIN, I2C0_SCL_PIN, I2C0_FREQ_HZ / 1000);
    printf("  Devices:           GPS, Compass, Sensors\n");
    printf("I2C1 (Internal):     SDA=GPIO%d, SCL=GPIO%d (%d kHz)\n",
           I2C1_SDA_PIN, I2C1_SCL_PIN, I2C1_FREQ_HZ / 1000);
    printf("  Devices:           Touch (GT911 @ 0x%02X), CH422G @ 0x%02X, RTC @ 0x%02X\n",
           I2C_ADDR_GT911, I2C_ADDR_CH422G, I2C_ADDR_PCF85063);

    printf("\n=== Touch Controller (GT911) ===\n");
    printf("I2C Address:         0x%02X\n", I2C_ADDR_GT911);
    printf("Interrupt:           GPIO%d (active low)\n", TOUCH_INT_PIN);
    printf("Max Touch Points:    %d\n", TOUCH_POINTS_MAX);
    printf("Reset:               Via CH422G EXIO%d\n", CH422G_EXIO1);

    printf("\n=== CAN Bus / NMEA 2000 ===\n");
    printf("TX:                  GPIO%d\n", CAN_TX_PIN);
    printf("RX:                  GPIO%d\n", CAN_RX_PIN);
    printf("Speed:               %d kbps\n", CAN_SPEED_KBPS);
    printf("Termination:         120Ω via onboard jumper\n");

    printf("\n=== RS485 Serial ===\n");
    printf("TX:                  GPIO%d\n", RS485_TX_PIN);
    printf("RX:                  GPIO%d\n", RS485_RX_PIN);
    printf("UART:                UART%d\n", RS485_UART_NUM);
    printf("Baud Rate:           %d bps\n", RS485_BAUD_RATE);

    printf("\n=== SD Card (SPI) ===\n");
    printf("MOSI:                GPIO%d\n", SD_MOSI_PIN);
    printf("MISO:                GPIO%d\n", SD_MISO_PIN);
    printf("SCK:                 GPIO%d\n", SD_SCK_PIN);
    printf("CS:                  Via CH422G EXIO%d\n", CH422G_EXIO4);
    printf("SPI Host:            SPI2\n");
    printf("Frequency:           %d MHz\n", SD_SPI_FREQ_HZ / 1000000);

    printf("\n=== USB Interface ===\n");
    printf("D-:                  GPIO%d\n", USB_DN_PIN);
    printf("D+:                  GPIO%d\n", USB_DP_PIN);

    printf("\n=== CH422G I/O Expander (I2C @ 0x%02X) ===\n", I2C_ADDR_CH422G);
    printf("EXIO0:               Reserved\n");
    printf("EXIO1:               Touch Reset\n");
    printf("EXIO2:               LCD Backlight Enable\n");
    printf("EXIO3:               LCD Reset\n");
    printf("EXIO4:               SD Card CS\n");
    printf("EXIO5:               USB Selection\n");

    printf("\n=== Isolated Digital I/O ===\n");
    printf("Outputs (5-36V, 450mA max sink):\n");
    printf("  DO0:               Buzzer / Relay 0 (via CH422G)\n");
    printf("  DO1:               Relay 1 (via CH422G)\n");
    printf("Inputs (5-36V, optocoupler isolated):\n");
    printf("  DI0:               Digital Input 0 (via CH422G EXIO0)\n");
    printf("  DI1:               Digital Input 1 (via CH422G EXIO5)\n");

    printf("\n");
}

/**
 * Display application configuration
 */
static void display_app_config(void) {
    print_banner_line();
    print_centered("APPLICATION CONFIGURATION");
    print_banner_line();

    printf("Alarm Distance:      %d-%d ft (default: %d ft)\n",
           ALARM_DISTANCE_MIN_FT, ALARM_DISTANCE_MAX_FT, ALARM_DISTANCE_DEFAULT_FT);
    printf("Arming Time:         %d-%d sec (default: %d sec)\n",
           ARMING_TIME_MIN_SEC, ARMING_TIME_MAX_SEC, ARMING_TIME_DEFAULT_SEC);
    printf("GPS Timeout:         %d seconds\n", GPS_TIMEOUT_SEC);
    printf("Button Debounce:     %d ms\n", BUTTON_DEBOUNCE_MS);
    printf("NVS Namespace:       %s\n", NVS_NAMESPACE);
    printf("\n");
}

/**
 * Display system status
 */
static void display_system_status(void) {
    print_banner_line();
    print_centered("SYSTEM STATUS");
    print_banner_line();

    printf("Boot Status:         OK\n");
    printf("Free Heap:           %lu bytes\n", esp_get_free_heap_size());
    printf("Min Free Heap:       %lu bytes\n", esp_get_minimum_free_heap_size());
    printf("\n");
}

/**
 * Main application task
 */
void app_main(void)
{
    esp_err_t ret;

    // Clear screen
    printf("\033[2J\033[H");

    ESP_LOGI(TAG, "=== Anchor Drag Pro Starting ===");
    ESP_LOGI(TAG, "Firmware Version: %s", FW_VERSION_STRING);
    ESP_LOGI(TAG, "UI Version: %s", UI_VERSION_STRING);
    ESP_LOGI(TAG, "Board: %s", BOARD_NAME);

    // Run splash screen with self-test (30 second timeout)
    ret = splash_screen_run(30);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Splash screen failed: %s", esp_err_to_name(ret));
    }

    // Display detailed system information after splash
    printf("\n");
    display_version_info();
    display_chip_info();
    display_libraries();
    display_pin_allocation();
    display_app_config();
    display_system_status();

    print_banner_line();
    print_centered("INITIALIZATION COMPLETE - READY FOR OPERATION");
    print_banner_line();
    printf("\n");

    ESP_LOGI(TAG, "Anchor Drag Pro v%s initialized successfully", FW_VERSION_STRING);
    ESP_LOGI(TAG, "UI Version: %s", UI_VERSION_STRING);
    ESP_LOGI(TAG, "Display: %dx%d RGB%d", LCD_WIDTH, LCD_HEIGHT, LCD_COLOR_BITS);

    // TODO: Transition to START screen (Screen 1)
    ESP_LOGI(TAG, "Next: Implement START screen for mode selection");

    // Main application loop
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(30000));  // Sleep 30 seconds
        ESP_LOGI(TAG, "System running - Free heap: %lu bytes", esp_get_free_heap_size());
    }
}
