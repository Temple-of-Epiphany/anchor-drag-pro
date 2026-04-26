/**
 * GPS Manager - Multi-Source GPS Data Handler Implementation
 *
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2025-12-29
 * Date Updated: 2025-12-29
 * Version: 0.1.0
 */

#include "gps_manager.h"
#include "board_config.h"
#include "esp_log.h"
#include "driver/twai.h"
#include "driver/uart.h"
#include "driver/i2c.h"
#include "esp_http_client.h"
#include "cJSON.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>
#include <math.h>

static const char *TAG = "GPS_MGR";

// ============================================================================
// Global State
// ============================================================================

static gps_source_t g_gps_source = GPS_SOURCE_AUTO;
static gps_source_t g_active_source = GPS_SOURCE_AUTO;
static gps_data_t g_gps_data = {0};
static gps_event_cb_t g_gps_callback = NULL;
static void *g_callback_user_data = NULL;
static bool g_initialized = false;

// ============================================================================
// NMEA 2000 CAN Bus Configuration
// ============================================================================

#define N2K_BITRATE     250000      // 250 kbps
#define N2K_TX_GPIO     GPIO_NUM_15
#define N2K_RX_GPIO     GPIO_NUM_16

// PGN 129029: GNSS Position Data
#define PGN_GNSS_POSITION_DATA  129029

// ============================================================================
// NMEA 0183 RS485 Configuration
// ============================================================================

#define NMEA_0183_UART  UART_NUM_1
#define NMEA_0183_RX    GPIO_NUM_43
#define NMEA_0183_TX    GPIO_NUM_44
#define NMEA_0183_BAUD  4800
#define NMEA_BUF_SIZE   1024

// ============================================================================
// External GPS I2C Configuration
// ============================================================================

#define EXT_GPS_I2C_ADDR    0x42
#define EXT_GPS_I2C_PORT    I2C_NUM_0  // Shared I2C bus (GPIO8/9)

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * Parse NMEA 0183 checksum
 */
static bool nmea_verify_checksum(const char *sentence) {
    if (sentence[0] != '$') return false;

    const char *asterisk = strchr(sentence, '*');
    if (!asterisk) return false;

    uint8_t checksum = 0;
    for (const char *p = sentence + 1; p < asterisk; p++) {
        checksum ^= *p;
    }

    uint8_t expected = (uint8_t)strtol(asterisk + 1, NULL, 16);
    return checksum == expected;
}

/**
 * Convert NMEA latitude to decimal degrees
 * Format: DDMM.MMMM (e.g., 3723.2475 = 37 degrees, 23.2475 minutes)
 */
static double nmea_to_decimal_lat(const char *nmea_lat, char hemisphere) {
    double lat = atof(nmea_lat);
    int degrees = (int)(lat / 100);
    double minutes = lat - (degrees * 100);
    double decimal = degrees + (minutes / 60.0);

    if (hemisphere == 'S' || hemisphere == 's') {
        decimal = -decimal;
    }

    return decimal;
}

/**
 * Convert NMEA longitude to decimal degrees
 * Format: DDDMM.MMMM (e.g., 12158.3416 = 121 degrees, 58.3416 minutes)
 */
static double nmea_to_decimal_lon(const char *nmea_lon, char hemisphere) {
    double lon = atof(nmea_lon);
    int degrees = (int)(lon / 100);
    double minutes = lon - (degrees * 100);
    double decimal = degrees + (minutes / 60.0);

    if (hemisphere == 'W' || hemisphere == 'w') {
        decimal = -decimal;
    }

    return decimal;
}

/**
 * Call registered callback with GPS data
 */
static void gps_notify_callback(const gps_data_t *data) {
    if (g_gps_callback) {
        g_gps_callback(data, g_callback_user_data);
    }
}

// ============================================================================
// GPS Manager Core Functions
// ============================================================================

esp_err_t gps_manager_init(void) {
    if (g_initialized) {
        ESP_LOGW(TAG, "GPS manager already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing GPS manager");

    // Initialize GPS data structure
    memset(&g_gps_data, 0, sizeof(g_gps_data));
    g_gps_data.valid = false;

    // NMEA 2000 CAN bus will be initialized separately when needed
    ESP_LOGI(TAG, "NMEA 2000 CAN bus deferred initialization");

    // Initialize NMEA 0183 UART (RS485)
    uart_config_t uart_config = {
        .baud_rate = NMEA_0183_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    esp_err_t ret = uart_driver_install(NMEA_0183_UART, NMEA_BUF_SIZE * 2, 0, 0, NULL, 0);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "UART driver install failed: %s", esp_err_to_name(ret));
    } else {
        ret = uart_param_config(NMEA_0183_UART, &uart_config);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "UART param config failed: %s", esp_err_to_name(ret));
        } else {
            ret = uart_set_pin(NMEA_0183_UART, NMEA_0183_TX, NMEA_0183_RX,
                              UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
            if (ret != ESP_OK) {
                ESP_LOGW(TAG, "UART set pin failed: %s", esp_err_to_name(ret));
            } else {
                ESP_LOGI(TAG, "NMEA 0183 UART initialized on GPIO%d(RX)/GPIO%d(TX)",
                        NMEA_0183_RX, NMEA_0183_TX);
            }
        }
    }

    // External GPS I2C uses shared I2C bus (already initialized by system)
    ESP_LOGI(TAG, "External GPS I2C (0x%02X) using shared I2C0 bus", EXT_GPS_I2C_ADDR);

    g_initialized = true;
    ESP_LOGI(TAG, "GPS manager initialized successfully");

    return ESP_OK;
}

esp_err_t gps_manager_set_source(gps_source_t source) {
    if (source > GPS_SOURCE_URL) {
        ESP_LOGE(TAG, "Invalid GPS source: %d", source);
        return ESP_ERR_INVALID_ARG;
    }

    g_gps_source = source;
    ESP_LOGI(TAG, "GPS source set to: %s", gps_manager_source_to_string(source));

    return ESP_OK;
}

gps_source_t gps_manager_get_source(void) {
    return g_gps_source;
}

gps_source_t gps_manager_get_active_source(void) {
    return g_active_source;
}

esp_err_t gps_manager_update(void) {
    if (!g_initialized) {
        ESP_LOGW(TAG, "GPS manager not initialized");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "=== GPS UPDATE START ===");
    ESP_LOGI(TAG, "Configured source: %d (%s)", g_gps_source, gps_manager_source_to_string(g_gps_source));

    esp_err_t ret = ESP_FAIL;
    gps_data_t temp_data = {0};

    // Try configured source
    switch (g_gps_source) {
        case GPS_SOURCE_AUTO:
            ESP_LOGI(TAG, "AUTO mode: Trying sources in priority order...");
            // Try sources in priority order: N2K -> 0183 -> EXTERNAL -> URL
            if (gps_read_n2k(&temp_data) == ESP_OK) {
                memcpy(&g_gps_data, &temp_data, sizeof(gps_data_t));
                g_active_source = GPS_SOURCE_N2K;
                ret = ESP_OK;
                ESP_LOGI(TAG, "SUCCESS: Using N2K source");
            } else if (gps_read_0183(&temp_data) == ESP_OK) {
                memcpy(&g_gps_data, &temp_data, sizeof(gps_data_t));
                g_active_source = GPS_SOURCE_0183;
                ret = ESP_OK;
                ESP_LOGI(TAG, "SUCCESS: Using 0183 source");
            } else if (gps_read_external(&temp_data) == ESP_OK) {
                memcpy(&g_gps_data, &temp_data, sizeof(gps_data_t));
                g_active_source = GPS_SOURCE_EXTERNAL;
                ret = ESP_OK;
                ESP_LOGI(TAG, "SUCCESS: Using EXTERNAL source");
            } else {
                ESP_LOGI(TAG, "N2K/0183/EXTERNAL failed, trying URL...");
                // Try URL source (read URL from NVS)
                nvs_handle_t nvs_handle;
                esp_err_t nvs_open_ret = nvs_open("anchor_alarm", NVS_READONLY, &nvs_handle);
                ESP_LOGI(TAG, "NVS open: %s", esp_err_to_name(nvs_open_ret));

                if (nvs_open_ret == ESP_OK) {
                    char url[128];
                    size_t url_len = sizeof(url);
                    esp_err_t nvs_get_ret = nvs_get_str(nvs_handle, "gps_url", url, &url_len);
                    ESP_LOGI(TAG, "NVS get_str: %s (len=%d)", esp_err_to_name(nvs_get_ret), url_len);

                    if (nvs_get_ret == ESP_OK) {
                        ESP_LOGI(TAG, "URL from NVS: '%s'", url);
                        esp_err_t url_ret = gps_read_url(url, &temp_data);
                        ESP_LOGI(TAG, "URL read result: %s", esp_err_to_name(url_ret));

                        if (url_ret == ESP_OK) {
                            memcpy(&g_gps_data, &temp_data, sizeof(gps_data_t));
                            g_active_source = GPS_SOURCE_URL;
                            ret = ESP_OK;
                            ESP_LOGI(TAG, "SUCCESS: Using URL - Lat=%.6f Lon=%.6f Sats=%d Valid=%d",
                                    temp_data.latitude, temp_data.longitude,
                                    temp_data.satellites_used, temp_data.valid);
                        }
                    }
                    nvs_close(nvs_handle);
                }
            }
            break;

        case GPS_SOURCE_N2K:
            ESP_LOGI(TAG, "Trying N2K source...");
            if (gps_read_n2k(&temp_data) == ESP_OK) {
                memcpy(&g_gps_data, &temp_data, sizeof(gps_data_t));
                g_active_source = GPS_SOURCE_N2K;
                ret = ESP_OK;
                ESP_LOGI(TAG, "SUCCESS: N2K");
            }
            break;

        case GPS_SOURCE_0183:
            ESP_LOGI(TAG, "Trying 0183 source...");
            if (gps_read_0183(&temp_data) == ESP_OK) {
                memcpy(&g_gps_data, &temp_data, sizeof(gps_data_t));
                g_active_source = GPS_SOURCE_0183;
                ret = ESP_OK;
                ESP_LOGI(TAG, "SUCCESS: 0183");
            }
            break;

        case GPS_SOURCE_EXTERNAL:
            ESP_LOGI(TAG, "Trying EXTERNAL source...");
            if (gps_read_external(&temp_data) == ESP_OK) {
                memcpy(&g_gps_data, &temp_data, sizeof(gps_data_t));
                g_active_source = GPS_SOURCE_EXTERNAL;
                ret = ESP_OK;
                ESP_LOGI(TAG, "SUCCESS: EXTERNAL");
            }
            break;

        case GPS_SOURCE_URL:
            ESP_LOGI(TAG, "URL mode: Reading from NVS...");
            // Read URL from NVS
            nvs_handle_t nvs_handle;
            esp_err_t nvs_open_ret = nvs_open("anchor_alarm", NVS_READONLY, &nvs_handle);
            ESP_LOGI(TAG, "NVS open: %s", esp_err_to_name(nvs_open_ret));

            if (nvs_open_ret == ESP_OK) {
                char url[128];
                size_t url_len = sizeof(url);
                esp_err_t nvs_ret = nvs_get_str(nvs_handle, "gps_url", url, &url_len);
                ESP_LOGI(TAG, "NVS get_str: %s (len=%d)", esp_err_to_name(nvs_ret), url_len);

                if (nvs_ret == ESP_OK) {
                    ESP_LOGI(TAG, "Fetching GPS from URL: '%s'", url);
                    esp_err_t url_ret = gps_read_url(url, &temp_data);
                    ESP_LOGI(TAG, "URL read result: %s", esp_err_to_name(url_ret));

                    if (url_ret == ESP_OK) {
                        memcpy(&g_gps_data, &temp_data, sizeof(gps_data_t));
                        g_active_source = GPS_SOURCE_URL;
                        ret = ESP_OK;
                        ESP_LOGI(TAG, "SUCCESS: URL - Lat=%.6f Lon=%.6f Sats=%d Valid=%d",
                                temp_data.latitude, temp_data.longitude,
                                temp_data.satellites_used, temp_data.valid);
                    } else {
                        ESP_LOGW(TAG, "FAILED: URL read error");
                    }
                } else {
                    ESP_LOGW(TAG, "FAILED: GPS URL not in NVS");
                }
                nvs_close(nvs_handle);
            } else {
                ESP_LOGW(TAG, "FAILED: Cannot open NVS");
            }
            break;
    }

    // Call registered callback if data was updated
    if (ret == ESP_OK && g_gps_callback) {
        ESP_LOGI(TAG, "Calling GPS callback");
        g_gps_callback(&g_gps_data, g_callback_user_data);
    }

    ESP_LOGI(TAG, "=== GPS UPDATE END: %s ===", esp_err_to_name(ret));
    return ret;
}

esp_err_t gps_manager_get_data(gps_data_t *data) {
    if (!data) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!g_gps_data.valid) {
        return ESP_FAIL;
    }

    memcpy(data, &g_gps_data, sizeof(gps_data_t));
    return ESP_OK;
}

bool gps_manager_has_fix(void) {
    return g_gps_data.valid &&
           g_gps_data.fix_quality != GPS_FIX_INVALID &&
           g_gps_data.satellites_used > 0;
}

void gps_manager_register_callback(gps_event_cb_t callback, void *user_data) {
    g_gps_callback = callback;
    g_callback_user_data = user_data;
    ESP_LOGI(TAG, "GPS callback registered");
}

const char* gps_manager_source_to_string(gps_source_t source) {
    switch (source) {
        case GPS_SOURCE_AUTO:      return "AUTO";
        case GPS_SOURCE_N2K:       return "NMEA 2000";
        case GPS_SOURCE_0183:      return "NMEA 0183";
        case GPS_SOURCE_EXTERNAL:  return "External GPS";
        case GPS_SOURCE_URL:       return "Network URL";
        default:                   return "Unknown";
    }
}

// ============================================================================
// NMEA 2000 CAN Bus Reader
// ============================================================================

// NMEA 2000 uses 29-bit extended CAN identifiers
// Format: Priority (3 bits) | PGN (18 bits) | Source (8 bits)

static bool g_twai_initialized = false;
static bool g_twai_init_failed = false;  // Prevent endless retry on failure

/**
 * Initialize TWAI (CAN) driver for NMEA 2000
 */
static esp_err_t n2k_twai_init(void) {
    ESP_LOGI(TAG, "n2k_twai_init() called - initialized=%d, failed=%d",
             g_twai_initialized, g_twai_init_failed);

    if (g_twai_initialized) {
        ESP_LOGI(TAG, "TWAI already initialized - returning OK");
        return ESP_OK;
    }

    // Don't retry if previous initialization failed
    if (g_twai_init_failed) {
        ESP_LOGI(TAG, "TWAI init previously failed - skipping retry");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Attempting TWAI (CAN) initialization for NMEA 2000");

    // Configure TWAI timing for 250 kbps
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_250KBITS();

    // Configure TWAI filter to accept all messages (we'll filter by PGN in software)
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    // Configure TWAI general settings
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(N2K_TX_GPIO, N2K_RX_GPIO, TWAI_MODE_NORMAL);
    g_config.rx_queue_len = 20;  // Increase queue size for GPS messages

    // Install TWAI driver
    esp_err_t ret = twai_driver_install(&g_config, &t_config, &f_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "TWAI driver install failed: %s (Likely no free interrupts - too many peripherals active)", esp_err_to_name(ret));
        g_twai_init_failed = true;  // Don't retry
        ESP_LOGI(TAG, "Setting g_twai_init_failed=true to prevent further retries");
        return ret;
    }

    // Start TWAI driver
    ret = twai_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "TWAI start failed: %s", esp_err_to_name(ret));
        twai_driver_uninstall();
        g_twai_init_failed = true;  // Don't retry
        return ret;
    }

    g_twai_initialized = true;
    ESP_LOGI(TAG, "TWAI initialized for NMEA 2000 (250 kbps, GPIO%d TX, GPIO%d RX)",
            N2K_TX_GPIO, N2K_RX_GPIO);

    return ESP_OK;
}

/**
 * Extract PGN from NMEA 2000 CAN identifier
 */
static uint32_t n2k_get_pgn(uint32_t can_id) {
    // PGN is bits 8-25 of the 29-bit identifier
    uint32_t pgn = (can_id >> 8) & 0x3FFFF;

    // PDU1 format (PDU Format < 240): PGN is in bits 8-23, destination is 0
    // PDU2 format (PDU Format >= 240): PGN includes destination in bits 8-15
    uint8_t pdu_format = (pgn >> 8) & 0xFF;
    if (pdu_format < 240) {
        pgn &= 0x3FF00;  // Clear destination address for PDU1
    }

    return pgn;
}

// Fast packet reassembly buffer for multi-frame NMEA 2000 messages
#define N2K_FAST_PACKET_MAX_SIZE 223
static uint8_t g_fast_packet_buffer[N2K_FAST_PACKET_MAX_SIZE];
static uint8_t g_fast_packet_len = 0;
static uint8_t g_fast_packet_frame_count = 0;
static uint8_t g_fast_packet_seq = 0;

/**
 * Parse PGN 129029 GNSS Position Data (multi-frame fast packet, up to 51 bytes)
 *
 * Byte layout (little-endian):
 * 0: SID (Sequence ID)
 * 1-2: Date (days since 1970-01-01)
 * 3-6: Time (seconds since midnight, 0.0001s resolution)
 * 7-14: Latitude (1e-16 degrees)
 * 15-22: Longitude (1e-16 degrees)
 * 23-30: Altitude (1e-6 meters)
 * 31-32: GNSS type + Method
 * 33: Integrity
 * 34: Number of SVs (satellites)
 * 35-36: HDOP (0.01 resolution)
 * 37-38: PDOP (0.01 resolution)
 */
static esp_err_t n2k_parse_pgn_129029(const uint8_t *data, uint8_t len, gps_data_t *gps) {
    if (len < 31) {
        ESP_LOGW(TAG, "PGN 129029 too short: %d bytes (need at least 31)", len);
        return ESP_ERR_INVALID_SIZE;
    }

    // Extract latitude (bytes 7-14, 64-bit signed integer, 1e-16 degrees)
    int64_t lat_raw = 0;
    memcpy(&lat_raw, &data[7], 8);
    gps->latitude = (double)lat_raw * 1e-16;

    // Extract longitude (bytes 15-22, 64-bit signed integer, 1e-16 degrees)
    int64_t lon_raw = 0;
    memcpy(&lon_raw, &data[15], 8);
    gps->longitude = (double)lon_raw * 1e-16;

    // Extract altitude (bytes 23-30, 64-bit signed integer, 1e-6 meters)
    int64_t alt_raw = 0;
    memcpy(&alt_raw, &data[23], 8);
    gps->altitude = (float)(alt_raw * 1e-6);

    // Extract number of satellites (byte 34) if available
    if (len >= 35) {
        gps->satellites_used = data[34];
    } else {
        gps->satellites_used = 0;  // Unknown
    }

    // Extract HDOP (bytes 35-36, 16-bit unsigned, 0.01 resolution)
    if (len >= 37) {
        uint16_t hdop_raw = 0;
        memcpy(&hdop_raw, &data[35], 2);
        gps->hdop = (float)(hdop_raw * 0.01);
    }

    // Extract PDOP (bytes 37-38, 16-bit unsigned, 0.01 resolution)
    if (len >= 39) {
        uint16_t pdop_raw = 0;
        memcpy(&pdop_raw, &data[37], 2);
        gps->pdop = (float)(pdop_raw * 0.01);
    }

    // Determine fix quality - if we have valid coordinates, assume valid fix
    if (gps->latitude != 0.0 || gps->longitude != 0.0) {
        gps->fix_quality = GPS_FIX_GPS;
        gps->valid = true;
    } else {
        gps->fix_quality = GPS_FIX_INVALID;
        gps->valid = false;
    }

    gps->source = GPS_SOURCE_N2K;
    gps->timestamp_ms = esp_log_timestamp();

    ESP_LOGI(TAG, "N2K GPS: %.6f, %.6f, alt=%.1fm (sats=%d, hdop=%.1f)",
            gps->latitude, gps->longitude, gps->altitude,
            gps->satellites_used, gps->hdop);

    return ESP_OK;
}

/**
 * Assemble fast packet from multiple CAN frames
 * Returns true when packet is complete
 */
static bool n2k_fast_packet_assemble(const uint8_t *frame_data, uint8_t frame_len) {
    if (frame_len < 2) {
        return false;
    }

    uint8_t frame_counter = frame_data[0] & 0x1F;  // Lower 5 bits
    uint8_t seq = (frame_data[0] >> 5) & 0x07;     // Upper 3 bits

    if (frame_counter == 0) {
        // First frame - contains total length
        g_fast_packet_len = frame_data[1];
        g_fast_packet_frame_count = 0;
        g_fast_packet_seq = seq;

        // Copy payload (skip first 2 bytes: counter and length)
        uint8_t payload_len = frame_len - 2;
        if (payload_len > 0 && payload_len <= N2K_FAST_PACKET_MAX_SIZE) {
            memcpy(g_fast_packet_buffer, &frame_data[2], payload_len);
            g_fast_packet_frame_count = payload_len;
        }
        return false;  // Need more frames
    } else {
        // Subsequent frame
        if (seq != g_fast_packet_seq) {
            // Sequence mismatch, reset
            g_fast_packet_len = 0;
            g_fast_packet_frame_count = 0;
            return false;
        }

        // Copy payload (skip first byte: counter)
        uint8_t payload_len = frame_len - 1;
        if (g_fast_packet_frame_count + payload_len <= N2K_FAST_PACKET_MAX_SIZE &&
            g_fast_packet_frame_count + payload_len <= g_fast_packet_len) {
            memcpy(&g_fast_packet_buffer[g_fast_packet_frame_count], &frame_data[1], payload_len);
            g_fast_packet_frame_count += payload_len;
        }

        // Check if packet is complete
        return (g_fast_packet_frame_count >= g_fast_packet_len);
    }
}

esp_err_t gps_read_n2k(gps_data_t *data) {
    if (!data) {
        return ESP_ERR_INVALID_ARG;
    }

    // Initialize TWAI if needed
    esp_err_t ret = n2k_twai_init();
    if (ret != ESP_OK) {
        return ret;
    }

    ESP_LOGD(TAG, "Reading GPS from NMEA 2000 CAN bus");

    // Receive CAN message with timeout
    twai_message_t rx_msg;
    ret = twai_receive(&rx_msg, pdMS_TO_TICKS(1000));

    if (ret != ESP_OK) {
        if (ret != ESP_ERR_TIMEOUT) {
            ESP_LOGW(TAG, "TWAI receive error: %s", esp_err_to_name(ret));
        }
        return ret;
    }

    // Check if this is an extended frame (NMEA 2000 uses 29-bit identifiers)
    if (!(rx_msg.flags & TWAI_MSG_FLAG_EXTD)) {
        ESP_LOGD(TAG, "Ignoring standard CAN frame (not NMEA 2000)");
        return ESP_ERR_NOT_FOUND;
    }

    // Extract PGN from CAN identifier
    uint32_t pgn = n2k_get_pgn(rx_msg.identifier);

    // Check if this is PGN 129029 (GNSS Position Data)
    if (pgn == PGN_GNSS_POSITION_DATA) {
        ESP_LOGD(TAG, "Received PGN 129029 (GNSS Position), %d bytes", rx_msg.data_length_code);

        // PGN 129029 uses fast packet protocol (multi-frame)
        // Assemble the complete message from multiple frames
        bool complete = n2k_fast_packet_assemble(rx_msg.data, rx_msg.data_length_code);

        if (complete) {
            ESP_LOGD(TAG, "Fast packet complete: %d bytes", g_fast_packet_len);

            // Parse the assembled GPS data
            ret = n2k_parse_pgn_129029(g_fast_packet_buffer, g_fast_packet_len, data);
            if (ret == ESP_OK) {
                // Update global GPS data and notify callback
                memcpy(&g_gps_data, data, sizeof(gps_data_t));
                g_active_source = GPS_SOURCE_N2K;
                gps_notify_callback(data);
                return ESP_OK;
            }
        } else {
            // Need more frames
            ESP_LOGD(TAG, "Fast packet incomplete, waiting for more frames...");
            return ESP_ERR_NOT_FINISHED;
        }
    }

    // Not the PGN we're looking for
    ESP_LOGD(TAG, "Ignoring PGN %u", pgn);
    return ESP_ERR_NOT_FOUND;
}

bool gps_check_n2k_available(void) {
    ESP_LOGD(TAG, "Checking N2K GPS availability");

    // Initialize TWAI if needed
    if (n2k_twai_init() != ESP_OK) {
        return false;
    }

    // Check TWAI status
    twai_status_info_t status;
    if (twai_get_status_info(&status) == ESP_OK) {
        // Consider N2K available if bus is active and we've seen messages
        if (status.state == TWAI_STATE_RUNNING) {
            ESP_LOGD(TAG, "N2K CAN bus running (rx: %u, tx: %u, errors: %u)",
                    status.msgs_to_rx, status.msgs_to_tx, status.bus_error_count);
            return status.msgs_to_rx > 0;
        }
    }

    return false;
}

// ============================================================================
// NMEA 0183 RS485 Reader
// ============================================================================

esp_err_t gps_read_0183(gps_data_t *data) {
    if (!data) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGD(TAG, "Reading GPS from NMEA 0183");

    char line[128];
    int pos = 0;
    uint8_t byte;

    // Read UART with timeout
    int len = uart_read_bytes(NMEA_0183_UART, &byte, 1, pdMS_TO_TICKS(1000));
    if (len <= 0) {
        ESP_LOGD(TAG, "No NMEA 0183 data available");
        return ESP_ERR_TIMEOUT;
    }

    // Read line until \n or buffer full
    while (len > 0 && pos < sizeof(line) - 1) {
        if (byte == '\n') {
            line[pos] = '\0';
            break;
        }
        if (byte != '\r') {
            line[pos++] = byte;
        }
        len = uart_read_bytes(NMEA_0183_UART, &byte, 1, pdMS_TO_TICKS(100));
    }
    line[pos] = '\0';

    if (pos == 0) {
        return ESP_ERR_NOT_FOUND;
    }

    // Verify checksum
    if (!nmea_verify_checksum(line)) {
        ESP_LOGW(TAG, "NMEA checksum failed: %s", line);
        return ESP_ERR_INVALID_CRC;
    }

    // Parse NMEA sentence
    // Support $GPGGA, $GPRMC, $GPGSA
    if (strncmp(line, "$GPGGA", 6) == 0) {
        // Example: $GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47
        char *token = strtok(line, ",");
        int field = 0;
        char lat_str[16] = {0}, lat_hem = 'N';
        char lon_str[16] = {0}, lon_hem = 'E';

        while (token != NULL && field < 15) {
            switch (field) {
                case 2: strncpy(lat_str, token, sizeof(lat_str) - 1); break;
                case 3: lat_hem = token[0]; break;
                case 4: strncpy(lon_str, token, sizeof(lon_str) - 1); break;
                case 5: lon_hem = token[0]; break;
                case 6: data->fix_quality = (gps_fix_quality_t)atoi(token); break;
                case 7: data->satellites_used = atoi(token); break;
                case 8: data->hdop = atof(token); break;
                case 9: data->altitude = atof(token); break;
            }
            token = strtok(NULL, ",");
            field++;
        }

        if (strlen(lat_str) > 0 && strlen(lon_str) > 0) {
            data->latitude = nmea_to_decimal_lat(lat_str, lat_hem);
            data->longitude = nmea_to_decimal_lon(lon_str, lon_hem);
            data->valid = (data->fix_quality != GPS_FIX_INVALID);
            data->source = GPS_SOURCE_0183;
            data->timestamp_ms = esp_log_timestamp();

            ESP_LOGI(TAG, "NMEA 0183 GPS: %.6f, %.6f (fix=%d, sats=%d)",
                    data->latitude, data->longitude, data->fix_quality, data->satellites_used);

            return ESP_OK;
        }
    }

    ESP_LOGD(TAG, "Unsupported NMEA sentence: %s", line);
    return ESP_ERR_NOT_SUPPORTED;
}

bool gps_check_0183_available(void) {
    ESP_LOGD(TAG, "Checking NMEA 0183 GPS availability");

    // Check if UART is receiving data
    size_t available = 0;
    uart_get_buffered_data_len(NMEA_0183_UART, &available);

    if (available > 0) {
        ESP_LOGD(TAG, "NMEA 0183 has %d bytes available", available);
        return true;
    }

    return false;
}

// ============================================================================
// External GPS I2C Reader
// ============================================================================

/**
 * Read NMEA data from external GPS I2C module
 * Many GPS modules output NMEA sentences over I2C
 * This implementation assumes the GPS streams NMEA 0183 sentences via I2C
 */
esp_err_t gps_read_external(gps_data_t *data) {
    if (!data) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGD(TAG, "Reading GPS from external I2C module");

    // Read up to 128 bytes from I2C GPS module
    uint8_t buffer[128];
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (EXT_GPS_I2C_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, buffer, sizeof(buffer) - 1, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(EXT_GPS_I2C_PORT, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK) {
        ESP_LOGD(TAG, "I2C read failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Null-terminate the buffer
    buffer[sizeof(buffer) - 1] = '\0';

    // Find NMEA sentence (starts with $, ends with \n)
    char *line_start = strchr((char*)buffer, '$');
    if (!line_start) {
        ESP_LOGD(TAG, "No NMEA sentence found in I2C data");
        return ESP_ERR_NOT_FOUND;
    }

    char *line_end = strchr(line_start, '\n');
    if (line_end) {
        *line_end = '\0';  // Null-terminate the sentence
    }

    // Verify checksum
    if (!nmea_verify_checksum(line_start)) {
        ESP_LOGW(TAG, "External GPS NMEA checksum failed: %s", line_start);
        return ESP_ERR_INVALID_CRC;
    }

    // Parse NMEA sentence (reuse NMEA 0183 parser logic)
    if (strncmp(line_start, "$GPGGA", 6) == 0) {
        char *token = strtok(line_start, ",");
        int field = 0;
        char lat_str[16] = {0}, lat_hem = 'N';
        char lon_str[16] = {0}, lon_hem = 'E';

        while (token != NULL && field < 15) {
            switch (field) {
                case 2: strncpy(lat_str, token, sizeof(lat_str) - 1); break;
                case 3: lat_hem = token[0]; break;
                case 4: strncpy(lon_str, token, sizeof(lon_str) - 1); break;
                case 5: lon_hem = token[0]; break;
                case 6: data->fix_quality = (gps_fix_quality_t)atoi(token); break;
                case 7: data->satellites_used = atoi(token); break;
                case 8: data->hdop = atof(token); break;
                case 9: data->altitude = atof(token); break;
            }
            token = strtok(NULL, ",");
            field++;
        }

        if (strlen(lat_str) > 0 && strlen(lon_str) > 0) {
            data->latitude = nmea_to_decimal_lat(lat_str, lat_hem);
            data->longitude = nmea_to_decimal_lon(lon_str, lon_hem);
            data->valid = (data->fix_quality != GPS_FIX_INVALID);
            data->source = GPS_SOURCE_EXTERNAL;
            data->timestamp_ms = esp_log_timestamp();

            ESP_LOGI(TAG, "External GPS (I2C): %.6f, %.6f (fix=%d, sats=%d)",
                    data->latitude, data->longitude, data->fix_quality, data->satellites_used);

            // Update global GPS data and notify callback
            memcpy(&g_gps_data, data, sizeof(gps_data_t));
            g_active_source = GPS_SOURCE_EXTERNAL;
            gps_notify_callback(data);

            return ESP_OK;
        }
    } else if (strncmp(line_start, "$GPRMC", 6) == 0) {
        // Parse $GPRMC (Recommended Minimum Specific GNSS Data)
        // Example: $GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A
        char *token = strtok(line_start, ",");
        int field = 0;
        char lat_str[16] = {0}, lat_hem = 'N';
        char lon_str[16] = {0}, lon_hem = 'E';
        char status = 'V';  // V = invalid, A = valid

        while (token != NULL && field < 12) {
            switch (field) {
                case 2: status = token[0]; break;  // A = valid, V = invalid
                case 3: strncpy(lat_str, token, sizeof(lat_str) - 1); break;
                case 4: lat_hem = token[0]; break;
                case 5: strncpy(lon_str, token, sizeof(lon_str) - 1); break;
                case 6: lon_hem = token[0]; break;
                case 7: data->speed_knots = atof(token); break;
                case 8: data->course = atof(token); break;
            }
            token = strtok(NULL, ",");
            field++;
        }

        if (status == 'A' && strlen(lat_str) > 0 && strlen(lon_str) > 0) {
            data->latitude = nmea_to_decimal_lat(lat_str, lat_hem);
            data->longitude = nmea_to_decimal_lon(lon_str, lon_hem);
            data->valid = true;
            data->fix_quality = GPS_FIX_GPS;
            data->source = GPS_SOURCE_EXTERNAL;
            data->timestamp_ms = esp_log_timestamp();

            // Convert speed from knots to km/h
            data->speed_kmh = data->speed_knots * 1.852f;

            ESP_LOGI(TAG, "External GPS (I2C): %.6f, %.6f, speed=%.1fkn, course=%.1f°",
                    data->latitude, data->longitude, data->speed_knots, data->course);

            // Update global GPS data and notify callback
            memcpy(&g_gps_data, data, sizeof(gps_data_t));
            g_active_source = GPS_SOURCE_EXTERNAL;
            gps_notify_callback(data);

            return ESP_OK;
        }
    }

    ESP_LOGD(TAG, "Unsupported external GPS NMEA sentence: %s", line_start);
    return ESP_ERR_NOT_SUPPORTED;
}

bool gps_check_external_available(void) {
    ESP_LOGD(TAG, "Checking external GPS I2C availability");

    // Try to detect device on I2C bus
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (EXT_GPS_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(EXT_GPS_I2C_PORT, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);

    if (ret == ESP_OK) {
        ESP_LOGD(TAG, "External GPS I2C device found at 0x%02X", EXT_GPS_I2C_ADDR);
        return true;
    }

    return false;
}

// ============================================================================
// Network URL GPS Reader
// ============================================================================

static esp_err_t http_event_handler(esp_http_client_event_t *evt) {
    // Handle HTTP events if needed
    return ESP_OK;
}

esp_err_t gps_read_url(const char *url, gps_data_t *data) {
    if (!url || !data) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGD(TAG, "Reading GPS from URL: %s", url);

    // Configure HTTP client for streaming NMEA data
    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_event_handler,
        .timeout_ms = 5000,
        .buffer_size = 2048,  // Larger buffer for NMEA stream
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return ESP_FAIL;
    }

    // Open connection
    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    // Read response data
    char buffer[512];
    memset(buffer, 0, sizeof(buffer));  // Zero the buffer first
    int total_read = 0;
    int read_len = esp_http_client_read(client, buffer, sizeof(buffer) - 1);

    if (read_len > 0) {
        total_read = read_len;
        buffer[total_read] = '\0';

        ESP_LOGD(TAG, "Received %d bytes from URL", total_read);

        // Clean the buffer - replace non-printable characters
        for (int i = 0; i < total_read; i++) {
            if (buffer[i] < 32 && buffer[i] != '\r' && buffer[i] != '\n') {
                buffer[i] = ' ';  // Replace with space
            }
        }

        ESP_LOGD(TAG, "Cleaned data: %s", buffer);

        // Search for $GPGGA sentence in the buffer
        char *gpgga_start = strstr(buffer, "$GPGGA");
        if (gpgga_start) {
            // Find end of sentence (newline or end of buffer)
            char *sentence_end = strpbrk(gpgga_start, "\r\n");
            if (sentence_end) {
                *sentence_end = '\0';  // Null-terminate the sentence
            }

            ESP_LOGI(TAG, "Found GPGGA: %s", gpgga_start);

            // Verify checksum
            if (!nmea_verify_checksum(gpgga_start)) {
                ESP_LOGW(TAG, "NMEA checksum failed: %s", gpgga_start);
                esp_http_client_close(client);
                esp_http_client_cleanup(client);
                return ESP_ERR_INVALID_CRC;
            }

            // Parse GPGGA sentence
            // Example: $GPGGA,211029.00,3001.3590,N,09002.1989,W,1,08,1.2,0.0,M,0.0,M,,*43
            char sentence_copy[256];
            strncpy(sentence_copy, gpgga_start, sizeof(sentence_copy) - 1);
            sentence_copy[sizeof(sentence_copy) - 1] = '\0';

            char *token = strtok(sentence_copy, ",");
            int field = 0;
            char lat_str[16] = {0}, lat_hem = 'N';
            char lon_str[16] = {0}, lon_hem = 'E';

            while (token != NULL && field < 15) {
                switch (field) {
                    case 1:  // Time (HHMMSS.SS)
                        if (strlen(token) >= 6) {
                            data->hour = (token[0] - '0') * 10 + (token[1] - '0');
                            data->minute = (token[2] - '0') * 10 + (token[3] - '0');
                            data->second = (token[4] - '0') * 10 + (token[5] - '0');
                        }
                        break;
                    case 2: strncpy(lat_str, token, sizeof(lat_str) - 1); break;
                    case 3: lat_hem = token[0]; break;
                    case 4: strncpy(lon_str, token, sizeof(lon_str) - 1); break;
                    case 5: lon_hem = token[0]; break;
                    case 6: data->fix_quality = (gps_fix_quality_t)atoi(token); break;
                    case 7: data->satellites_used = atoi(token); break;
                    case 8: data->hdop = atof(token); break;
                    case 9: data->altitude = atof(token); break;
                }
                token = strtok(NULL, ",");
                field++;
            }

            if (strlen(lat_str) > 0 && strlen(lon_str) > 0) {
                data->latitude = nmea_to_decimal_lat(lat_str, lat_hem);
                data->longitude = nmea_to_decimal_lon(lon_str, lon_hem);
                data->valid = (data->fix_quality != GPS_FIX_INVALID && data->satellites_used > 0);
                data->source = GPS_SOURCE_URL;
                data->timestamp_ms = esp_log_timestamp();

                ESP_LOGI(TAG, "URL GPS (NMEA): %.6f, %.6f (fix=%d, sats=%d, hdop=%.1f)",
                        data->latitude, data->longitude, data->fix_quality,
                        data->satellites_used, data->hdop);

                esp_http_client_close(client);
                esp_http_client_cleanup(client);
                return ESP_OK;
            }
        } else {
            ESP_LOGW(TAG, "No GPGGA sentence found in response (got %d bytes)", total_read);
        }
    } else if (read_len == 0) {
        ESP_LOGW(TAG, "No data received from URL (read returned 0)");
    } else {
        ESP_LOGE(TAG, "HTTP read error: read_len=%d", read_len);
    }

    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    return ESP_FAIL;
}

// ============================================================================
// End of gps_manager.c
// ============================================================================
