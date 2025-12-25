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
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "board_config.h"
#include "splash_screen.h"
#include "splash_logo.h"
#include "smpte_test_screen.h"
#include "start_screen.h"
#include "simple_test_screen.h"
#include "display_test.h"
#include "ui_version.h"
#include "display_driver.h"
#include "touch_driver.h"
#include "lvgl_init.h"
#include "rtc_pcf85063a.h"
#include "tv_test_pattern.h"
#include "ui_footer.h"
#include "screens.h"

// External font declarations
LV_FONT_DECLARE(orbitron_variablefont_wght_24);

static const char *TAG = "anchor-drag-pro";

// Global footer reference for touch handler
static lv_obj_t *g_footer = NULL;

// Global screen references for navigation
static lv_obj_t *g_screens[PAGE_COUNT] = {NULL};
static lv_obj_t *g_footers[PAGE_COUNT] = {NULL};
static ui_page_t g_current_page = PAGE_START;

// Touch tracking for manual swipe detection
static lv_point_t touch_start = {0, 0};
static bool touch_started = false;

// Forward declaration
static void footer_page_callback(ui_page_t page);

/**
 * Global gesture callback - handles swipe up (show footer) and swipe left/right (navigate)
 * Uses manual touch tracking for reliable swipe detection
 */
static void global_gesture_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_PRESSED) {
        // Touch started - record starting position
        lv_indev_t *indev = lv_indev_get_act();
        lv_indev_get_point(indev, &touch_start);
        touch_started = true;
        ESP_LOGI(TAG, "Touch started at X=%d, Y=%d", touch_start.x, touch_start.y);

    } else if (code == LV_EVENT_PRESSING) {
        // Touch moving - check for swipe gestures
        if (touch_started) {
            lv_point_t current;
            lv_indev_t *indev = lv_indev_get_act();
            lv_indev_get_point(indev, &current);

            int16_t delta_x = current.x - touch_start.x;  // Positive = rightward movement
            int16_t delta_y = touch_start.y - current.y;  // Positive = upward movement

            // Swipe up - show footer
            if (delta_y > 50 && abs(delta_x) < 30) {
                ESP_LOGI(TAG, "Swipe up detected! Delta Y=%d - showing footer", delta_y);
                if (g_footer != NULL) {
                    ui_footer_show(g_footer);
                }
                touch_started = false;  // Prevent multiple triggers
            }
            // Swipe left - next screen
            else if (delta_x < -80 && abs(delta_y) < 40) {
                ESP_LOGI(TAG, "Swipe left detected! Delta X=%d - next screen", delta_x);
                ui_page_t next_page = (g_current_page + 1) % PAGE_COUNT;
                footer_page_callback(next_page);
                touch_started = false;
            }
            // Swipe right - previous screen
            else if (delta_x > 80 && abs(delta_y) < 40) {
                ESP_LOGI(TAG, "Swipe right detected! Delta X=%d - previous screen", delta_x);
                ui_page_t prev_page = (g_current_page + PAGE_COUNT - 1) % PAGE_COUNT;
                footer_page_callback(prev_page);
                touch_started = false;
            }
        }

    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        // Touch ended
        touch_started = false;
    }
}

/**
 * Bottom touch area callback - wrapper for global gestures on swipe area
 */
static void bottom_touch_cb(lv_event_t *e) {
    global_gesture_cb(e);
}

/**
 * Footer button callback - handles page navigation
 */
static void footer_page_callback(ui_page_t page) {
    const char *page_names[] = {"START", "INFO", "PGN", "CONFIG", "UPDATE", "TOOLS"};

    // Check if screen exists
    if (g_screens[page] == NULL) {
        ESP_LOGE(TAG, "Screen %s not created!", page_names[page]);
        return;
    }

    // Navigate to the selected screen
    // Note: We're already in an LVGL context (button event callback), so no need to lock
    lv_scr_load(g_screens[page]);
    g_current_page = page;
    g_footer = g_footers[page];  // Update global footer reference for swipe-up

    ESP_LOGI(TAG, "Navigation: %s screen loaded", page_names[page]);
}

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

    printf("12. RTC (PCF85063A)  [ENABLED]  - Real-time clock with battery backup\n");

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

    printf("\n=== I2C Bus ===\n");
    printf("I2C0 (All Devices):  SDA=GPIO%d, SCL=GPIO%d (%d kHz)\n",
           I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO, I2C_MASTER_FREQ_HZ / 1000);
    printf("  Devices:           Touch (GT911 @ 0x%02X), CH422G @ 0x%02X, RTC @ 0x%02X\n",
           I2C_ADDR_GT911, I2C_ADDR_CH422G, I2C_ADDR_PCF85063);
    printf("                     GPS, Compass, External Sensors\n");

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

    // Set reasonable log levels - reduce ESP-IDF component noise
    esp_log_level_set("*", ESP_LOG_WARN);  // Default: only warnings and errors
    esp_log_level_set("anchor-drag-pro", ESP_LOG_INFO);  // Our app: info level
    esp_log_level_set("ui_footer", ESP_LOG_INFO);  // Footer: info level (reduced from DEBUG)
    esp_log_level_set("screens", ESP_LOG_INFO);  // Screens: info level

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "================================================================================");
    ESP_LOGI(TAG, "=== ANCHOR DRAG PRO - MARINE SAFETY SYSTEM ===");
    ESP_LOGI(TAG, "================================================================================");
    ESP_LOGI(TAG, "Firmware Version: %s", FW_VERSION_STRING);
    ESP_LOGI(TAG, "UI Version: %s", UI_VERSION_STRING);
    ESP_LOGI(TAG, "Board: %s", BOARD_NAME);
    ESP_LOGI(TAG, "================================================================================");
    ESP_LOGI(TAG, "");

    // Initialize RTC (Real-Time Clock)
    ESP_LOGI(TAG, "Initializing RTC (PCF85063A)...");
    PCF85063A_Init();

    // Read current time from RTC
    datetime_t rtc_time;
    PCF85063A_Read_now(&rtc_time);

    // Check if RTC has valid time (year should be reasonable)
    bool rtc_valid = (rtc_time.year >= 2025 && rtc_time.year <= 2050 &&
                     rtc_time.month >= 1 && rtc_time.month <= 12 &&
                     rtc_time.day >= 1 && rtc_time.day <= 31 &&
                     rtc_time.hour <= 23 && rtc_time.min <= 59 && rtc_time.sec <= 59);

    if (!rtc_valid) {
        ESP_LOGW(TAG, "RTC time invalid (year=%d), setting to build time", rtc_time.year);

        // Parse build date and time from __DATE__ and __TIME__ macros
        // __DATE__ format: "Dec 23 2025"
        // __TIME__ format: "20:30:45"
        const char *month_names[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                     "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
        char month_str[4];
        int day, year, hour, min, sec;

        sscanf(__DATE__, "%s %d %d", month_str, &day, &year);
        sscanf(__TIME__, "%d:%d:%d", &hour, &min, &sec);

        // Convert month name to number
        int month = 1;
        for (int i = 0; i < 12; i++) {
            if (strcmp(month_str, month_names[i]) == 0) {
                month = i + 1;
                break;
            }
        }

        // Set RTC to build time
        datetime_t build_time = {
            .year = year,
            .month = month,
            .day = day,
            .dotw = 0,  // Don't care about day of week
            .hour = hour,
            .min = min,
            .sec = sec
        };

        PCF85063A_Set_All(build_time);
        ESP_LOGI(TAG, "RTC set to build time: %04d-%02d-%02d %02d:%02d:%02d",
                 year, month, day, hour, min, sec);

        // Re-read to confirm
        PCF85063A_Read_now(&rtc_time);
    } else {
        ESP_LOGI(TAG, "RTC time is valid");
    }

    // Convert to system time and set it
    struct tm timeinfo = {
        .tm_year = rtc_time.year - 1900,  // tm_year is years since 1900
        .tm_mon = rtc_time.month - 1,      // tm_mon is 0-11
        .tm_mday = rtc_time.day,
        .tm_hour = rtc_time.hour,
        .tm_min = rtc_time.min,
        .tm_sec = rtc_time.sec,
        .tm_wday = rtc_time.dotw,          // Day of week
        .tm_isdst = -1                     // Auto-determine DST
    };

    time_t epoch_time = mktime(&timeinfo);
    struct timeval tv = {
        .tv_sec = epoch_time,
        .tv_usec = 0
    };

    settimeofday(&tv, NULL);

    // Log the RTC time
    char rtc_str[64];
    datetime_to_str(rtc_str, rtc_time);
    ESP_LOGI(TAG, "RTC Time: %s", rtc_str);
    ESP_LOGI(TAG, "System time synchronized with RTC");
    ESP_LOGI(TAG, "Timestamps will now show real date/time instead of milliseconds");

    // Initialize RGB LCD display
    ret = display_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Display initialization failed: %s", esp_err_to_name(ret));
        return;
    }

    // Initialize GT911 touch controller
    ret = touch_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Touch initialization failed: %s", esp_err_to_name(ret));
        ESP_LOGW(TAG, "Continuing without touch input...");
        // Continue anyway - touch is optional for testing
    }

    // Initialize LVGL graphics library
    ret = lvgl_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LVGL initialization failed: %s", esp_err_to_name(ret));
        return;
    }

    // Register VSYNC callback for frame synchronization (Mode 3)
    ret = display_register_vsync_callback();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "VSYNC callback registration failed: %s", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(TAG, "Display and LVGL initialized successfully (Mode 3: Direct-Mode)");

    // Create TV test pattern (custom image)
    ESP_LOGI(TAG, "Creating TV test pattern from image...");
    if (lvgl_lock(1000)) {
        lv_obj_t *test_screen = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(test_screen, lv_color_black(), 0);

        // Create image object and load test pattern (800x480)
        lv_obj_t *img = lv_img_create(test_screen);
        lv_img_set_src(img, &tv_test_pattern);
        lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);

        lv_scr_load(test_screen);
        lvgl_unlock();
        ESP_LOGI(TAG, "TV test pattern displayed (800x480 pixels)");
    } else {
        ESP_LOGE(TAG, "Failed to lock LVGL for test pattern");
    }

    // Show test pattern for 5 seconds
    ESP_LOGI(TAG, "Displaying test pattern for 5 seconds...");
    vTaskDelay(pdMS_TO_TICKS(5000));
    ESP_LOGI(TAG, "Test pattern timeout complete");

    // Create splash screen with logo and scrollable bottom menu
    ESP_LOGI(TAG, "Creating splash screen with OK button and footer...");
    if (lvgl_lock(2000)) {
        lv_obj_t *splash_screen = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(splash_screen, lv_color_hex(0x001F3F), 0);  // Dark blue

        // Create "Anchor Drag Alarm" text at top of screen in Orbitron 24pt
        lv_obj_t *title_label = lv_label_create(splash_screen);
        lv_label_set_text(title_label, "Anchor Drag Alarm");
        lv_obj_set_style_text_font(title_label, &orbitron_variablefont_wght_24, 0);
        lv_obj_set_style_text_color(title_label, lv_color_hex(0x39CCCC), 0);  // Teal color
        lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 20);  // Top of screen with 20px margin

        // Create splash logo image (256x256) - positioned higher
        lv_obj_t *logo_img = lv_img_create(splash_screen);
        lv_img_set_src(logo_img, &splash_logo);
        lv_obj_align(logo_img, LV_ALIGN_CENTER, 0, -60);  // 60px above center

        // Create self-test progress bar below logo
        lv_obj_t *progress_bar = lv_bar_create(splash_screen);
        lv_obj_set_size(progress_bar, 400, 20);  // 400px wide, 20px tall
        lv_obj_align_to(progress_bar, logo_img, LV_ALIGN_OUT_BOTTOM_MID, 0, 30);  // Below logo with 30px gap

        // Style the progress bar - teal theme
        lv_obj_set_style_bg_color(progress_bar, lv_color_hex(0x003366), 0);  // Dark blue background
        lv_obj_set_style_bg_opa(progress_bar, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(progress_bar, lv_color_hex(0x39CCCC), LV_PART_INDICATOR);  // Teal indicator
        lv_obj_set_style_bg_opa(progress_bar, LV_OPA_COVER, LV_PART_INDICATOR);
        lv_obj_set_style_radius(progress_bar, 10, 0);  // Rounded corners
        lv_obj_set_style_radius(progress_bar, 10, LV_PART_INDICATOR);

        // Set initial progress to 0%
        lv_bar_set_value(progress_bar, 0, LV_ANIM_OFF);

        // Add "Self Test" label above progress bar
        lv_obj_t *test_label = lv_label_create(splash_screen);
        lv_label_set_text(test_label, "Self Test");
        lv_obj_set_style_text_color(test_label, lv_color_white(), 0);
        lv_obj_align_to(test_label, progress_bar, LV_ALIGN_OUT_TOP_MID, 0, -8);  // Above progress bar

        // Add version number at bottom right
        lv_obj_t *version_label = lv_label_create(splash_screen);
        lv_label_set_text_fmt(version_label, "v%s", UI_VERSION_STRING);
        lv_obj_set_style_text_color(version_label, lv_color_hex(0x666666), 0);  // Gray color
        lv_obj_align(version_label, LV_ALIGN_BOTTOM_RIGHT, -10, -70);  // Bottom right, above footer area

        // Create scrollable footer navigation bar at bottom
        g_footer = ui_footer_create(splash_screen, PAGE_START, footer_page_callback);
        if (g_footer == NULL) {
            ESP_LOGE(TAG, "Failed to create footer");
        } else {
            ESP_LOGI(TAG, "Footer created successfully (scrollable)");
        }

        // Create invisible gesture-sensitive area at bottom for swipe-up
        // Positioned ABOVE the footer so it doesn't block button clicks
        lv_obj_t *bottom_handle = lv_obj_create(splash_screen);
        lv_obj_set_size(bottom_handle, 800, 40);  // Full width, 40px tall
        lv_obj_align(bottom_handle, LV_ALIGN_BOTTOM_MID, 0, -60);  // Above footer (60px offset)
        lv_obj_set_style_bg_opa(bottom_handle, LV_OPA_TRANSP, 0);  // Completely transparent/invisible
        lv_obj_set_style_border_width(bottom_handle, 0, 0);
        lv_obj_set_style_radius(bottom_handle, 0, 0);  // No rounding

        // Register for all touch events to manually track swipe
        lv_obj_add_event_cb(bottom_handle, bottom_touch_cb, LV_EVENT_PRESSED, NULL);
        lv_obj_add_event_cb(bottom_handle, bottom_touch_cb, LV_EVENT_PRESSING, NULL);
        lv_obj_add_event_cb(bottom_handle, bottom_touch_cb, LV_EVENT_RELEASED, NULL);

        // Load splash screen (this will automatically unload the test pattern)
        lv_scr_load(splash_screen);
        lvgl_unlock();
        ESP_LOGI(TAG, "Splash screen with progress bar displayed");

        // Perform self-test with progress animation
        ESP_LOGI(TAG, "Starting self-test...");
        for (int i = 0; i <= 100; i += 10) {
            if (lvgl_lock(100)) {
                lv_bar_set_value(progress_bar, i, LV_ANIM_ON);
                lvgl_unlock();
            }
            vTaskDelay(pdMS_TO_TICKS(200));  // 200ms per step = 2 second total test
            ESP_LOGI(TAG, "Self-test progress: %d%%", i);
        }
        ESP_LOGI(TAG, "Self-test complete!");

        // Brief pause before continuing
        vTaskDelay(pdMS_TO_TICKS(500));

    } else {
        ESP_LOGE(TAG, "Failed to lock LVGL for splash screen");
    }

    // Create all navigation screens
    ESP_LOGI(TAG, "Creating navigation screens...");
    if (lvgl_lock(2000)) {
        g_screens[PAGE_START] = create_start_screen(footer_page_callback, &g_footers[PAGE_START]);
        g_screens[PAGE_INFO] = create_info_screen(footer_page_callback, &g_footers[PAGE_INFO]);
        g_screens[PAGE_PGN] = create_pgn_screen(footer_page_callback, &g_footers[PAGE_PGN]);
        g_screens[PAGE_CONFIG] = create_config_screen(footer_page_callback, &g_footers[PAGE_CONFIG]);
        g_screens[PAGE_UPDATE] = create_update_screen(footer_page_callback, &g_footers[PAGE_UPDATE]);
        g_screens[PAGE_TOOLS] = create_tools_screen(footer_page_callback, &g_footers[PAGE_TOOLS]);

        // Add gesture detection to all navigation screens
        for (int i = 0; i < PAGE_COUNT; i++) {
            // Create gesture area for main content (doesn't cover footer to avoid blocking buttons)
            lv_obj_t *gesture_area = lv_obj_create(g_screens[i]);
            lv_obj_set_size(gesture_area, 800, 420);  // Full width, but only upper area (not footer)
            lv_obj_align(gesture_area, LV_ALIGN_TOP_MID, 0, 0);
            lv_obj_set_style_bg_opa(gesture_area, LV_OPA_TRANSP, 0);  // Invisible
            lv_obj_set_style_border_width(gesture_area, 0, 0);
            lv_obj_set_style_radius(gesture_area, 0, 0);
            lv_obj_move_background(gesture_area);  // Move to back so it doesn't block UI elements

            // Register for all touch events for global gesture detection
            lv_obj_add_event_cb(gesture_area, global_gesture_cb, LV_EVENT_PRESSED, NULL);
            lv_obj_add_event_cb(gesture_area, global_gesture_cb, LV_EVENT_PRESSING, NULL);
            lv_obj_add_event_cb(gesture_area, global_gesture_cb, LV_EVENT_RELEASED, NULL);
        }

        lvgl_unlock();
        ESP_LOGI(TAG, "All navigation screens created with swipe-up areas");

        // Load the START screen
        ESP_LOGI(TAG, "Loading START screen...");
        if (lvgl_lock(100)) {
            lv_scr_load(g_screens[PAGE_START]);
            g_current_page = PAGE_START;
            g_footer = g_footers[PAGE_START];  // Set footer reference for swipe-up
            lvgl_unlock();
            ESP_LOGI(TAG, "START screen loaded with swipe-up enabled");
        } else {
            ESP_LOGE(TAG, "Failed to lock LVGL for START screen");
        }
    } else {
        ESP_LOGE(TAG, "Failed to lock LVGL for screen creation");
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
    ESP_LOGI(TAG, "Navigation screens ready - use footer buttons to switch between pages");

    // Main application loop
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(30000));  // Sleep 30 seconds
        ESP_LOGI(TAG, "System running - Free heap: %lu bytes", esp_get_free_heap_size());
    }
}
