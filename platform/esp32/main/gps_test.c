/**
 * GPS M8N Test Utility
 *
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2026-01-12
 * Version: 0.1.0
 *
 * Utility to test and debug GPS M8N module on I2C bus
 */

#include "gps_test.h"
#include "gps_manager.h"
#include "board_config.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include <string.h>

static const char *TAG = "gps_test";

// GPS M8N I2C configuration
#define GPS_M8N_I2C_ADDR    0x42
#define GPS_M8N_I2C_PORT    I2C_NUM_0  // Shared I2C bus (GPIO8 SDA, GPIO9 SCL)

/**
 * Scan I2C bus for devices
 * Returns true if GPS module is found at expected address
 */
bool gps_test_scan_i2c(void) {
    ESP_LOGI(TAG, "=== I2C BUS SCAN START ===");
    ESP_LOGI(TAG, "Scanning I2C bus 0 (GPIO8 SDA, GPIO9 SCL) for devices...");
    ESP_LOGI(TAG, "I2C Port: %d, Scanning addresses 0x01 to 0x7E", GPS_M8N_I2C_PORT);

    int devices_found = 0;
    bool gps_found = false;

    for (uint8_t addr = 1; addr < 127; addr++) {
        ESP_LOGD(TAG, "  Probing address 0x%02X...", addr);

        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);

        esp_err_t ret = i2c_master_cmd_begin(GPS_M8N_I2C_PORT, cmd, pdMS_TO_TICKS(50));
        i2c_cmd_link_delete(cmd);

        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "  ✓ Found device at address: 0x%02X", addr);
            devices_found++;

            if (addr == GPS_M8N_I2C_ADDR) {
                ESP_LOGI(TAG, "    ^^^ GPS M8N MODULE DETECTED! ^^^");
                gps_found = true;
            } else if (addr == 0x24) {
                ESP_LOGI(TAG, "    (CH422G I/O Expander)");
            } else if (addr == 0x51) {
                ESP_LOGI(TAG, "    (PCF85063A RTC)");
            } else if (addr == 0x5D) {
                ESP_LOGI(TAG, "    (GT911 Touch Controller)");
            }
        }
    }

    ESP_LOGI(TAG, "I2C scan complete: %d device(s) found", devices_found);
    ESP_LOGI(TAG, "=== I2C BUS SCAN END ===\n");

    return gps_found;
}

/**
 * Read raw data from GPS M8N module
 * Shows first 256 bytes of data for debugging
 */
void gps_test_read_raw(void) {
    ESP_LOGI(TAG, "=== GPS M8N RAW READ TEST ===");

    uint8_t buffer[256];
    memset(buffer, 0, sizeof(buffer));

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (GPS_M8N_I2C_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, buffer, sizeof(buffer) - 1, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(GPS_M8N_I2C_PORT, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Successfully read %d bytes from GPS M8N", sizeof(buffer) - 1);

        // Display as hex dump
        ESP_LOGI(TAG, "Raw data (hex):");
        for (int i = 0; i < 256; i += 16) {
            ESP_LOG_BUFFER_HEX_LEVEL(TAG, &buffer[i], 16, ESP_LOG_INFO);
        }

        // Display as ASCII (replace non-printable with '.')
        ESP_LOGI(TAG, "\nRaw data (ASCII):");
        char ascii_buffer[257];
        for (int i = 0; i < 256; i++) {
            if (buffer[i] >= 32 && buffer[i] <= 126) {
                ascii_buffer[i] = buffer[i];
            } else if (buffer[i] == '\r' || buffer[i] == '\n') {
                ascii_buffer[i] = buffer[i];
            } else {
                ascii_buffer[i] = '.';
            }
        }
        ascii_buffer[256] = '\0';
        ESP_LOGI(TAG, "%s", ascii_buffer);

        // Look for NMEA sentences
        char *nmea_start = strchr((char*)buffer, '$');
        if (nmea_start) {
            ESP_LOGI(TAG, "\nFound NMEA sentence starting at offset %d:",
                    (int)(nmea_start - (char*)buffer));

            // Find up to 3 NMEA sentences
            for (int i = 0; i < 3; i++) {
                char *next_start = strchr(nmea_start + 1, '$');
                if (next_start) {
                    *next_start = '\0';
                }

                // Remove any trailing \r\n
                char *end = nmea_start + strlen(nmea_start) - 1;
                while (end > nmea_start && (*end == '\r' || *end == '\n' || *end == '\0')) {
                    *end = '\0';
                    end--;
                }

                if (strlen(nmea_start) > 0) {
                    ESP_LOGI(TAG, "  NMEA[%d]: %s", i, nmea_start);
                }

                if (!next_start) break;
                nmea_start = next_start;
            }
        } else {
            ESP_LOGW(TAG, "No NMEA sentences found in data (no '$' character)");
            ESP_LOGW(TAG, "GPS may still be acquiring satellite fix...");
        }
    } else {
        ESP_LOGE(TAG, "Failed to read from GPS M8N: %s", esp_err_to_name(ret));
        ESP_LOGE(TAG, "Check wiring: SDA=GPIO8, SCL=GPIO9, VCC=3.3V, GND=GND");
    }

    ESP_LOGI(TAG, "=== GPS M8N RAW READ TEST END ===\n");
}

/**
 * Test GPS data parsing using GPS manager
 */
void gps_test_parse_data(void) {
    ESP_LOGI(TAG, "=== GPS M8N PARSE TEST ===");

    gps_data_t gps_data = {0};
    esp_err_t ret = gps_read_external(&gps_data);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "✓ Successfully parsed GPS data!");
        ESP_LOGI(TAG, "  Latitude:     %.6f°", gps_data.latitude);
        ESP_LOGI(TAG, "  Longitude:    %.6f°", gps_data.longitude);
        ESP_LOGI(TAG, "  Altitude:     %.1f m", gps_data.altitude);
        ESP_LOGI(TAG, "  Satellites:   %d", gps_data.satellites_used);
        ESP_LOGI(TAG, "  Fix Quality:  %d (%s)", gps_data.fix_quality,
                gps_data.fix_quality == GPS_FIX_INVALID ? "No Fix" :
                gps_data.fix_quality == GPS_FIX_GPS ? "GPS" :
                gps_data.fix_quality == GPS_FIX_DGPS ? "DGPS" : "Other");
        ESP_LOGI(TAG, "  HDOP:         %.1f", gps_data.hdop);
        ESP_LOGI(TAG, "  Speed:        %.1f knots (%.1f km/h)",
                gps_data.speed_knots, gps_data.speed_kmh);
        ESP_LOGI(TAG, "  Course:       %.1f°", gps_data.course);
        ESP_LOGI(TAG, "  Valid:        %s", gps_data.valid ? "YES" : "NO");
    } else if (ret == ESP_ERR_NOT_FOUND) {
        ESP_LOGW(TAG, "✗ No NMEA data available (GPS may be acquiring fix)");
    } else if (ret == ESP_ERR_INVALID_CRC) {
        ESP_LOGW(TAG, "✗ NMEA checksum failed (corrupted data)");
    } else if (ret == ESP_ERR_NOT_SUPPORTED) {
        ESP_LOGW(TAG, "✗ Unsupported NMEA sentence type");
    } else {
        ESP_LOGE(TAG, "✗ Failed to read GPS: %s", esp_err_to_name(ret));
    }

    ESP_LOGI(TAG, "=== GPS M8N PARSE TEST END ===\n");
}

/**
 * Run complete GPS test sequence
 */
void gps_test_run_all(void) {
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "==========================================================");
    ESP_LOGI(TAG, "GPS M8N MODULE TEST UTILITY STARTING");
    ESP_LOGI(TAG, "==========================================================");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔═══════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║   GPS M8N MODULE TEST UTILITY             ║");
    ESP_LOGI(TAG, "╚═══════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Expected Configuration:");
    ESP_LOGI(TAG, "  I2C Address: 0x42");
    ESP_LOGI(TAG, "  I2C Bus:     I2C0 (GPIO8 SDA, GPIO9 SCL)");
    ESP_LOGI(TAG, "  Protocol:    NMEA 0183 over I2C");
    ESP_LOGI(TAG, "");

    ESP_LOGI(TAG, "Starting I2C bus scan...");
    // Step 1: Scan I2C bus
    bool gps_detected = gps_test_scan_i2c();
    ESP_LOGI(TAG, "I2C scan complete: GPS %s", gps_detected ? "FOUND" : "NOT FOUND");

    if (!gps_detected) {
        ESP_LOGE(TAG, "GPS M8N NOT FOUND on I2C bus!");
        ESP_LOGE(TAG, "Troubleshooting:");
        ESP_LOGE(TAG, "  1. Check wiring connections");
        ESP_LOGE(TAG, "  2. Verify 3.3V power supply");
        ESP_LOGE(TAG, "  3. Check SDA/SCL not swapped");
        ESP_LOGE(TAG, "  4. Try different I2C address (some modules use 0x43)");
        return;
    }

    // Wait for GPS to initialize
    ESP_LOGI(TAG, "Waiting 2 seconds for GPS to initialize...");
    vTaskDelay(pdMS_TO_TICKS(2000));

    // Step 2: Read raw data
    gps_test_read_raw();

    // Step 3: Parse GPS data
    gps_test_parse_data();

    ESP_LOGI(TAG, "╔═══════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║   GPS TEST COMPLETE                       ║");
    ESP_LOGI(TAG, "╚═══════════════════════════════════════════╝");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Note: If no GPS fix, move module near window for satellite view");
}

/**
 * Monitor GPS continuously (for testing)
 */
void gps_test_monitor(int duration_seconds) {
    ESP_LOGI(TAG, "=== GPS CONTINUOUS MONITOR ===");
    ESP_LOGI(TAG, "Monitoring GPS for %d seconds...", duration_seconds);
    ESP_LOGI(TAG, "Reading every 1 second\n");

    for (int i = 0; i < duration_seconds; i++) {
        ESP_LOGI(TAG, "[%02d] Reading GPS...", i + 1);

        gps_data_t gps_data = {0};
        esp_err_t ret = gps_read_external(&gps_data);

        if (ret == ESP_OK && gps_data.valid) {
            ESP_LOGI(TAG, "     Pos: %.6f, %.6f | Sats: %d | Alt: %.1fm | HDOP: %.1f",
                    gps_data.latitude, gps_data.longitude,
                    gps_data.satellites_used, gps_data.altitude, gps_data.hdop);
        } else {
            ESP_LOGW(TAG, "     No valid GPS data (%s)", esp_err_to_name(ret));
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    ESP_LOGI(TAG, "=== GPS MONITOR END ===\n");
}
