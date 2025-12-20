/**
 * Board Configuration - Waveshare ESP32-S3-Touch-LCD-4.3B
 *
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2025-12-19
 * Date Updated: 2025-12-19
 * Version: 0.1.0
 *
 * Pin definitions and hardware configuration for Waveshare ESP32-S3-Touch-LCD-4.3B
 * Reference: docs/ESP32-S3-Touch-LCD-4.3B-BOX-KNOWLEDGE.md
 */

#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

// ============================================================================
// Version Information
// ============================================================================
#define FW_VERSION_MAJOR    0
#define FW_VERSION_MINOR    1
#define FW_VERSION_PATCH    1
#define FW_VERSION_STRING   "0.1.1"
#define FW_BUILD_DATE       __DATE__
#define FW_BUILD_TIME       __TIME__

// ============================================================================
// Board Identification
// ============================================================================
#define BOARD_NAME          "Waveshare ESP32-S3-Touch-LCD-4.3B"
#define BOARD_VARIANT       "Type B"
#define MCU_NAME            "ESP32-S3-WROOM-1-N16R8"

// ============================================================================
// Display Configuration - ST7262 RGB LCD
// ============================================================================
#define LCD_WIDTH           800
#define LCD_HEIGHT          480
#define LCD_COLOR_BITS      16      // RGB565
#define LCD_RGB_DATA_WIDTH  16      // 16-bit parallel

// LCD Timing Parameters
#define LCD_PIXEL_CLOCK_HZ  (16 * 1000 * 1000)  // 16 MHz
#define LCD_HPW             4       // Horizontal pulse width
#define LCD_HBP             8       // Horizontal back porch
#define LCD_HFP             8       // Horizontal front porch
#define LCD_VPW             4       // Vertical pulse width
#define LCD_VBP             8       // Vertical back porch
#define LCD_VFP             8       // Vertical front porch

// Bounce buffer to prevent display drift
#define LCD_BOUNCE_BUFFER_SIZE  (LCD_WIDTH * 10)

// ============================================================================
// RGB LCD Pins (16-bit parallel interface)
// ============================================================================

// LCD Control Signals
#define LCD_PIN_VSYNC       3       // Vertical sync
#define LCD_PIN_HSYNC       46      // Horizontal sync
#define LCD_PIN_DE          5       // Data enable
#define LCD_PIN_PCLK        7       // Pixel clock

// Blue channel (5 bits: B3-B7)
#define LCD_PIN_B3          14      // D0
#define LCD_PIN_B4          38      // D1
#define LCD_PIN_B5          18      // D2
#define LCD_PIN_B6          17      // D3
#define LCD_PIN_B7          10      // D4

// Green channel (6 bits: G2-G7)
#define LCD_PIN_G2          39      // D5
#define LCD_PIN_G3          0       // D6
#define LCD_PIN_G4          45      // D7
#define LCD_PIN_G5          48      // D8
#define LCD_PIN_G6          47      // D9
#define LCD_PIN_G7          21      // D10

// Red channel (5 bits: R3-R7)
#define LCD_PIN_R3          1       // D11
#define LCD_PIN_R4          2       // D12
#define LCD_PIN_R5          42      // D13
#define LCD_PIN_R6          41      // D14
#define LCD_PIN_R7          40      // D15

// ============================================================================
// I2C Bus Configuration
// ============================================================================

// I2C0 - External devices (GPS, sensors, compass)
#define I2C0_SDA_PIN        8
#define I2C0_SCL_PIN        9
#define I2C0_FREQ_HZ        400000  // 400 kHz

// I2C1 - Internal devices (Touch controller GT911)
#define I2C1_SDA_PIN        15
#define I2C1_SCL_PIN        7
#define I2C1_FREQ_HZ        400000  // 400 kHz

// I2C Device Addresses
#define I2C_ADDR_CH422G     0x24    // I/O Expander
#define I2C_ADDR_GT911      0x5D    // Touch controller
#define I2C_ADDR_PCF85063   0x51    // RTC
#define I2C_ADDR_NEO8M_GPS  0x42    // NEO-8M GPS module (optional)

// ============================================================================
// Touch Controller - GT911
// ============================================================================
#define TOUCH_I2C_NUM       1       // Use I2C1
#define TOUCH_INT_PIN       4       // Touch interrupt (active low)
#define TOUCH_POINTS_MAX    5       // 5-point multi-touch

// Touch reset controlled via CH422G I/O expander EXIO1

// ============================================================================
// CH422G I/O Expander Configuration
// ============================================================================
#define CH422G_EXIO0        0       // Reserved
#define CH422G_EXIO1        1       // Touch reset
#define CH422G_EXIO2        2       // LCD backlight enable
#define CH422G_EXIO3        3       // LCD reset
#define CH422G_EXIO4        4       // SD card chip select
#define CH422G_EXIO5        5       // USB selection

// ============================================================================
// CAN Bus / NMEA 2000 (TWAI)
// ============================================================================
#define CAN_TX_PIN          15      // CAN transmit
#define CAN_RX_PIN          16      // CAN receive
#define CAN_SPEED_KBPS      250     // NMEA 2000 standard speed

// CAN termination is via onboard 120Ω resistor (enable jumper if at network end)

// ============================================================================
// RS485 Serial Interface
// ============================================================================
#define RS485_TX_PIN        44      // RS485 transmit (TXD)
#define RS485_RX_PIN        43      // RS485 receive (RXD)
#define RS485_UART_NUM      1       // UART1
#define RS485_BAUD_RATE     4800    // Default NMEA 0183 baud rate

// ============================================================================
// SD Card (SPI Interface)
// ============================================================================
#define SD_MOSI_PIN         11      // SPI MOSI
#define SD_MISO_PIN         13      // SPI MISO
#define SD_SCK_PIN          12      // SPI clock
// SD card chip select controlled via CH422G EXIO4

#define SD_SPI_HOST         SPI2_HOST
#define SD_SPI_FREQ_HZ      20000000    // 20 MHz

// ============================================================================
// USB Interface
// ============================================================================
#define USB_DN_PIN          19      // USB D- (negative)
#define USB_DP_PIN          20      // USB D+ (positive)

// ============================================================================
// Isolated Digital I/O
// ============================================================================
// Isolated outputs (open-drain, 5-36V, 450mA max sink)
// Controlled via CH422G I/O expander
#define ISO_OUTPUT_0        0       // Buzzer / Relay output 0
#define ISO_OUTPUT_1        1       // Relay output 1

// Isolated inputs (optocoupler, 5-36V range)
// Via CH422G EXIO0 and EXIO5
#define ISO_INPUT_0         0       // Digital input 0
#define ISO_INPUT_1         1       // Digital input 1

// ============================================================================
// Power Configuration
// ============================================================================
#define VIN_MIN_VOLTS       7.0f    // Minimum input voltage
#define VIN_MAX_VOLTS       36.0f   // Maximum input voltage
#define VIN_NOMINAL_12V     12.0f   // Nominal 12V marine system
#define VIN_NOMINAL_24V     24.0f   // Nominal 24V marine system

// ============================================================================
// Application Configuration
// ============================================================================

// Anchor Alarm Settings (defaults)
#define ALARM_DISTANCE_MIN_FT       25      // Minimum alarm distance
#define ALARM_DISTANCE_MAX_FT       250     // Maximum alarm distance
#define ALARM_DISTANCE_DEFAULT_FT   50      // Default alarm distance

#define ARMING_TIME_MIN_SEC         30      // Minimum arming time
#define ARMING_TIME_MAX_SEC         300     // Maximum arming time
#define ARMING_TIME_DEFAULT_SEC     60      // Default arming time

#define GPS_TIMEOUT_SEC             60      // GPS signal timeout

// Button debounce
#define BUTTON_DEBOUNCE_MS          3500    // Button debounce time

// ============================================================================
// Library Feature Flags
// ============================================================================
#define ENABLE_LVGL                 1       // Enable LVGL GUI framework
#define ENABLE_LVGL_ANIMATIONS      1       // Enable LVGL animations
#define ENABLE_PNG_DECODER          1       // Enable PNG image decoder
#define ENABLE_JPG_DECODER          0       // Disable JPG (not needed)
#define ENABLE_BMP_DECODER          1       // Enable BMP decoder

#define ENABLE_CAN_BUS              1       // Enable CAN/TWAI (NMEA 2000)
#define ENABLE_RS485                0       // Disable RS485 (not used yet)
#define ENABLE_EXTERNAL_GPS         0       // Disable external GPS (not used yet)
#define ENABLE_SD_CARD              0       // Disable SD card (not used yet)
#define ENABLE_WIFI                 0       // Disable WiFi (not used yet)
#define ENABLE_BLUETOOTH            0       // Disable Bluetooth (not used yet)

// ============================================================================
// NVS (Non-Volatile Storage) Configuration
// ============================================================================
#define NVS_NAMESPACE       "anchor_alarm"  // NVS namespace for settings

// NVS Keys
#define NVS_KEY_ALARM_DISTANCE  "alarm_dist"
#define NVS_KEY_ARMING_TIME     "arming_time"
#define NVS_KEY_GPS_SOURCE      "gps_source"
#define NVS_KEY_BRIGHTNESS      "brightness"
#define NVS_KEY_BUZZER_VOL      "buzzer_vol"

// ============================================================================
// FreeRTOS Task Configuration
// ============================================================================
#define TASK_PRIORITY_HIGH      10
#define TASK_PRIORITY_NORMAL    5
#define TASK_PRIORITY_LOW       2

#define TASK_STACK_SIZE_LARGE   8192
#define TASK_STACK_SIZE_MEDIUM  4096
#define TASK_STACK_SIZE_SMALL   2048

// ============================================================================
// Macros and Helper Functions
// ============================================================================

// Version comparison
#define FW_VERSION_CODE     ((FW_VERSION_MAJOR << 16) | (FW_VERSION_MINOR << 8) | FW_VERSION_PATCH)

// Pin validation
#define IS_VALID_GPIO(pin)  ((pin) >= 0 && (pin) < 49)

// Feature check macros
#define FEATURE_ENABLED(feature)    (feature == 1)

#endif // BOARD_CONFIG_H
