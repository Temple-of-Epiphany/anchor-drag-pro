/**
 * WiFi Manager - WiFi Station Mode Connection Handler
 *
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2025-12-27
 * Version: 0.1.0
 *
 * Manages WiFi connectivity in station mode for the anchor alarm system.
 * Provides connection, disconnection, status monitoring, and credential management.
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <stdbool.h>
#include "esp_err.h"
#include "esp_wifi_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// WiFi Status
// ============================================================================

typedef enum {
    WIFI_STATUS_DISCONNECTED = 0,
    WIFI_STATUS_CONNECTING,
    WIFI_STATUS_CONNECTED,
    WIFI_STATUS_FAILED,
    WIFI_STATUS_IDLE
} wifi_status_t;

// ============================================================================
// WiFi Event Callback
// ============================================================================

typedef void (*wifi_event_cb_t)(wifi_status_t status, void *user_data);

// ============================================================================
// WiFi Manager Functions
// ============================================================================

/**
 * Initialize WiFi manager
 * Must be called before any other WiFi functions
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t wifi_manager_init(void);

/**
 * Connect to WiFi network with provided credentials
 *
 * @param ssid WiFi network SSID (max 32 chars)
 * @param password WiFi password (max 64 chars)
 * @param save_credentials If true, save credentials to NVS for auto-reconnect
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t wifi_manager_connect(const char *ssid, const char *password, bool save_credentials);

/**
 * Connect to WiFi using saved credentials from NVS
 *
 * @return ESP_OK on success, ESP_ERR_NOT_FOUND if no credentials saved
 */
esp_err_t wifi_manager_connect_saved(void);

/**
 * Disconnect from WiFi network
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t wifi_manager_disconnect(void);

/**
 * Get current WiFi connection status
 *
 * @return Current WiFi status
 */
wifi_status_t wifi_manager_get_status(void);

/**
 * Check if WiFi is connected
 *
 * @return true if connected, false otherwise
 */
bool wifi_manager_is_connected(void);

/**
 * Get IP address as string
 *
 * @param ip_str Buffer to store IP address (minimum 16 bytes: "255.255.255.255\0")
 * @return ESP_OK if connected and IP retrieved, ESP_FAIL otherwise
 */
esp_err_t wifi_manager_get_ip(char *ip_str);

/**
 * Get RSSI (signal strength) of current connection
 *
 * @param rssi Pointer to store RSSI value (dBm, typically -30 to -90)
 * @return ESP_OK if connected and RSSI retrieved, ESP_FAIL otherwise
 */
esp_err_t wifi_manager_get_rssi(int8_t *rssi);

/**
 * Get connected SSID
 *
 * @param ssid Buffer to store SSID (minimum 33 bytes)
 * @return ESP_OK if connected and SSID retrieved, ESP_FAIL otherwise
 */
esp_err_t wifi_manager_get_ssid(char *ssid);

/**
 * Start WiFi scan for available networks
 *
 * @return ESP_OK if scan started, error code otherwise
 */
esp_err_t wifi_manager_scan_start(void);

/**
 * Get scan results
 *
 * @param ap_list Array to store AP records
 * @param max_aps Maximum number of APs to return
 * @param num_aps Pointer to store actual number of APs found
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t wifi_manager_scan_get_results(wifi_ap_record_t *ap_list, uint16_t max_aps, uint16_t *num_aps);

/**
 * Register event callback for WiFi status changes
 *
 * @param callback Callback function to be called on status changes
 * @param user_data User data to pass to callback
 */
void wifi_manager_register_callback(wifi_event_cb_t callback, void *user_data);

/**
 * Delete saved WiFi credentials from NVS
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t wifi_manager_clear_credentials(void);

/**
 * Ping the gateway to test connectivity
 *
 * @param timeout_ms Timeout in milliseconds (default 5000ms)
 * @return ESP_OK if ping successful, ESP_FAIL if no response or not connected
 */
esp_err_t wifi_manager_ping_gateway(uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif // WIFI_MANAGER_H
