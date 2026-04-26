/**
 * GPS Manager - Multi-Source GPS Data Handler
 *
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2025-12-29
 * Date Updated: 2025-12-29
 * Version: 0.1.0
 *
 * Manages GPS data from multiple sources:
 * - NMEA 2000 (N2K) via CAN bus
 * - NMEA 0183 via RS485
 * - External GPS via I2C (address 0x42)
 * - Network GPS data via URL (WiFi)
 * - AUTO mode (automatic source selection priority: N2K -> 0183 -> EXTERNAL -> URL)
 */

#ifndef GPS_MANAGER_H
#define GPS_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// GPS Data Source Types
// ============================================================================

typedef enum {
    GPS_SOURCE_AUTO = 0,      // Automatic source selection (N2K -> 0183 -> EXTERNAL -> URL)
    GPS_SOURCE_N2K = 1,       // NMEA 2000 via CAN bus
    GPS_SOURCE_0183 = 2,      // NMEA 0183 via RS485
    GPS_SOURCE_EXTERNAL = 3,  // External GPS via I2C
    GPS_SOURCE_URL = 4        // Network GPS via WiFi
} gps_source_t;

// ============================================================================
// GPS Fix Quality
// ============================================================================

typedef enum {
    GPS_FIX_INVALID = 0,      // No fix
    GPS_FIX_GPS = 1,          // GPS fix (autonomous)
    GPS_FIX_DGPS = 2,         // Differential GPS fix
    GPS_FIX_PPS = 3,          // PPS fix
    GPS_FIX_RTK = 4,          // Real Time Kinematic (RTK) fix
    GPS_FIX_RTK_FLOAT = 5,    // RTK Float
    GPS_FIX_DR = 6,           // Dead reckoning
    GPS_FIX_MANUAL = 7,       // Manual input mode
    GPS_FIX_SIMULATION = 8    // Simulation mode
} gps_fix_quality_t;

// ============================================================================
// GPS Data Structure
// ============================================================================

typedef struct {
    // Position
    double latitude;          // Latitude in decimal degrees (-90 to +90)
    double longitude;         // Longitude in decimal degrees (-180 to +180)
    float altitude;           // Altitude in meters above mean sea level

    // Velocity
    float speed_knots;        // Speed over ground in knots
    float speed_kmh;          // Speed over ground in km/h
    float course;             // Course over ground in degrees (0-360)

    // Accuracy
    float hdop;               // Horizontal dilution of precision
    float vdop;               // Vertical dilution of precision
    float pdop;               // Position dilution of precision

    // Fix info
    gps_fix_quality_t fix_quality;  // GPS fix quality
    uint8_t satellites_used;        // Number of satellites used in fix
    uint8_t satellites_visible;     // Number of satellites visible

    // Time (UTC)
    uint8_t hour;             // 0-23
    uint8_t minute;           // 0-59
    uint8_t second;           // 0-59
    uint16_t millisecond;     // 0-999
    uint8_t day;              // 1-31
    uint8_t month;            // 1-12
    uint16_t year;            // Full year (e.g., 2025)

    // Metadata
    gps_source_t source;      // Which GPS source provided this data
    uint32_t timestamp_ms;    // System timestamp when data was received (ms)
    bool valid;               // True if GPS data is valid and can be used
} gps_data_t;

// ============================================================================
// GPS Event Callback
// ============================================================================

/**
 * Callback function type for GPS data updates
 * @param data Pointer to GPS data structure
 * @param user_data User data passed during callback registration
 */
typedef void (*gps_event_cb_t)(const gps_data_t *data, void *user_data);

// ============================================================================
// GPS Manager Functions
// ============================================================================

/**
 * Initialize GPS manager and all GPS sources
 * Must be called before any other GPS functions
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t gps_manager_init(void);

/**
 * Set active GPS source
 * @param source GPS source to use (AUTO, N2K, 0183, EXTERNAL, URL)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t gps_manager_set_source(gps_source_t source);

/**
 * Get currently configured GPS source
 * @return Current GPS source setting
 */
gps_source_t gps_manager_get_source(void);

/**
 * Get actual GPS source currently providing data (may differ from setting in AUTO mode)
 * @return Active GPS source
 */
gps_source_t gps_manager_get_active_source(void);

/**
 * Update GPS data from configured source
 * Call this periodically to poll GPS sources and update cached data
 * @return ESP_OK if data was updated, ESP_FAIL otherwise
 */
esp_err_t gps_manager_update(void);

/**
 * Get latest GPS data
 * @param data Pointer to GPS data structure to fill
 * @return ESP_OK if valid data available, ESP_FAIL otherwise
 */
esp_err_t gps_manager_get_data(gps_data_t *data);

/**
 * Check if GPS has valid fix
 * @return true if GPS has valid fix, false otherwise
 */
bool gps_manager_has_fix(void);

/**
 * Register callback for GPS data updates
 * @param callback Callback function to call on GPS updates
 * @param user_data User data to pass to callback
 */
void gps_manager_register_callback(gps_event_cb_t callback, void *user_data);

/**
 * Get human-readable GPS source name
 * @param source GPS source enum value
 * @return String name (e.g., "NMEA 2000")
 */
const char* gps_manager_source_to_string(gps_source_t source);

// ============================================================================
// GPS Source-Specific Functions
// ============================================================================

/**
 * Read GPS data from NMEA 2000 CAN bus (PGN 129029)
 * GPIO15 (TX), GPIO16 (RX) at 250 kbps
 *
 * @param data Pointer to GPS data structure to fill
 * @return ESP_OK if data read successfully, error code otherwise
 */
esp_err_t gps_read_n2k(gps_data_t *data);

/**
 * Read GPS data from NMEA 0183 via RS485
 * GPIO43 (RX), GPIO44 (TX)
 * Parses NMEA sentences: $GPGGA, $GPRMC, $GPGSA
 *
 * @param data Pointer to GPS data structure to fill
 * @return ESP_OK if data read successfully, error code otherwise
 */
esp_err_t gps_read_0183(gps_data_t *data);

/**
 * Read GPS data from external GPS module via I2C
 * I2C address 0x42 on GPIO8 (SDA), GPIO9 (SCL)
 *
 * @param data Pointer to GPS data structure to fill
 * @return ESP_OK if data read successfully, error code otherwise
 */
esp_err_t gps_read_external(gps_data_t *data);

/**
 * Read GPS data from network URL via WiFi
 * Fetches JSON GPS data from configured URL endpoint
 *
 * @param url URL to fetch GPS data from (e.g., "http://192.168.1.100:8080/gps")
 * @param data Pointer to GPS data structure to fill
 * @return ESP_OK if data read successfully, error code otherwise
 */
esp_err_t gps_read_url(const char *url, gps_data_t *data);

/**
 * Check if NMEA 2000 GPS source is available
 * @return true if N2K GPS is responding, false otherwise
 */
bool gps_check_n2k_available(void);

/**
 * Check if NMEA 0183 GPS source is available
 * @return true if 0183 GPS is responding, false otherwise
 */
bool gps_check_0183_available(void);

/**
 * Check if external GPS module is available
 * @return true if external GPS is responding on I2C, false otherwise
 */
bool gps_check_external_available(void);

#ifdef __cplusplus
}
#endif

#endif // GPS_MANAGER_H
