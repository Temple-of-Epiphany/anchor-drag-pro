/**
 * WiFi Manager - WiFi Station Mode Connection Handler
 *
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2025-12-27
 * Version: 0.1.0
 */

#include "wifi_manager.h"
#include "board_config.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "ping/ping_sock.h"
#include "esp_netif.h"

static const char *TAG = "wifi_manager";

// ============================================================================
// FreeRTOS Event Group Bits
// ============================================================================
#define WIFI_CONNECTED_BIT     BIT0
#define WIFI_FAIL_BIT          BIT1

// ============================================================================
// Module State
// ============================================================================

static EventGroupHandle_t s_wifi_event_group = NULL;
static wifi_status_t s_wifi_status = WIFI_STATUS_DISCONNECTED;
static int s_retry_num = 0;
static wifi_event_cb_t s_event_callback = NULL;
static void *s_callback_user_data = NULL;

// ============================================================================
// Internal Functions
// ============================================================================

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "WiFi station started");
        esp_wifi_connect();
        s_wifi_status = WIFI_STATUS_CONNECTING;
        if (s_event_callback) {
            s_event_callback(s_wifi_status, s_callback_user_data);
        }
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < WIFI_RECONNECT_ATTEMPTS) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Retry to connect to the AP (attempt %d/%d)", s_retry_num, WIFI_RECONNECT_ATTEMPTS);
            s_wifi_status = WIFI_STATUS_CONNECTING;
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            s_wifi_status = WIFI_STATUS_FAILED;
            ESP_LOGI(TAG, "Failed to connect to AP after %d attempts", WIFI_RECONNECT_ATTEMPTS);
        }
        if (s_event_callback) {
            s_event_callback(s_wifi_status, s_callback_user_data);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP address: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        s_wifi_status = WIFI_STATUS_CONNECTED;
        if (s_event_callback) {
            s_event_callback(s_wifi_status, s_callback_user_data);
        }
    }
}

// ============================================================================
// Public Functions
// ============================================================================

esp_err_t wifi_manager_init(void)
{
    if (s_wifi_event_group != NULL) {
        ESP_LOGW(TAG, "WiFi manager already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing WiFi manager");

    // Create event group
    s_wifi_event_group = xEventGroupCreate();
    if (s_wifi_event_group == NULL) {
        ESP_LOGE(TAG, "Failed to create event group");
        return ESP_FAIL;
    }

    // Initialize network interface
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    // Initialize WiFi with default config
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Register event handlers
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    // Set WiFi mode to station
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    s_wifi_status = WIFI_STATUS_IDLE;
    ESP_LOGI(TAG, "WiFi manager initialized successfully");

    return ESP_OK;
}

esp_err_t wifi_manager_connect(const char *ssid, const char *password, bool save_credentials)
{
    if (s_wifi_event_group == NULL) {
        ESP_LOGE(TAG, "WiFi manager not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (ssid == NULL || strlen(ssid) == 0) {
        ESP_LOGE(TAG, "Invalid SSID");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Connecting to WiFi SSID: %s", ssid);

    // Configure WiFi
    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    if (password != NULL && strlen(password) > 0) {
        strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
    }
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    // Save credentials if requested
    if (save_credentials) {
        nvs_handle_t nvs_handle;
        esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
        if (err == ESP_OK) {
            nvs_set_str(nvs_handle, NVS_KEY_WIFI_SSID, ssid);
            if (password != NULL) {
                nvs_set_str(nvs_handle, NVS_KEY_WIFI_PASS, password);
            }
            nvs_commit(nvs_handle);
            nvs_close(nvs_handle);
            ESP_LOGI(TAG, "WiFi credentials saved to NVS");
        } else {
            ESP_LOGW(TAG, "Failed to save credentials: %s", esp_err_to_name(err));
        }
    }

    // Reset retry counter
    s_retry_num = 0;

    // Clear previous event bits
    xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);

    // Start connection
    ESP_ERROR_CHECK(esp_wifi_connect());

    return ESP_OK;
}

esp_err_t wifi_manager_connect_saved(void)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "No saved credentials found");
        return ESP_ERR_NOT_FOUND;
    }

    char ssid[WIFI_SSID_MAX_LEN + 1] = {0};
    char password[WIFI_PASSWORD_MAX_LEN + 1] = {0};
    size_t ssid_len = sizeof(ssid);
    size_t pass_len = sizeof(password);

    err = nvs_get_str(nvs_handle, NVS_KEY_WIFI_SSID, ssid, &ssid_len);
    if (err != ESP_OK) {
        nvs_close(nvs_handle);
        ESP_LOGW(TAG, "No SSID in NVS");
        return ESP_ERR_NOT_FOUND;
    }

    // Password is optional
    nvs_get_str(nvs_handle, NVS_KEY_WIFI_PASS, password, &pass_len);
    nvs_close(nvs_handle);

    ESP_LOGI(TAG, "Connecting using saved credentials");
    return wifi_manager_connect(ssid, password, false);
}

esp_err_t wifi_manager_disconnect(void)
{
    if (s_wifi_event_group == NULL) {
        ESP_LOGE(TAG, "WiFi manager not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Disconnecting WiFi");
    esp_err_t err = esp_wifi_disconnect();
    s_wifi_status = WIFI_STATUS_DISCONNECTED;

    if (s_event_callback) {
        s_event_callback(s_wifi_status, s_callback_user_data);
    }

    return err;
}

wifi_status_t wifi_manager_get_status(void)
{
    return s_wifi_status;
}

bool wifi_manager_is_connected(void)
{
    return (s_wifi_status == WIFI_STATUS_CONNECTED);
}

esp_err_t wifi_manager_get_ip(char *ip_str)
{
    if (ip_str == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!wifi_manager_is_connected()) {
        return ESP_FAIL;
    }

    esp_netif_ip_info_t ip_info;
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (netif == NULL) {
        return ESP_FAIL;
    }

    if (esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
        snprintf(ip_str, 16, IPSTR, IP2STR(&ip_info.ip));
        return ESP_OK;
    }

    return ESP_FAIL;
}

esp_err_t wifi_manager_get_rssi(int8_t *rssi)
{
    if (rssi == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!wifi_manager_is_connected()) {
        return ESP_FAIL;
    }

    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        *rssi = ap_info.rssi;
        return ESP_OK;
    }

    return ESP_FAIL;
}

esp_err_t wifi_manager_get_ssid(char *ssid)
{
    if (ssid == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!wifi_manager_is_connected()) {
        return ESP_FAIL;
    }

    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        strncpy(ssid, (char *)ap_info.ssid, 33);
        ssid[32] = '\0';
        return ESP_OK;
    }

    return ESP_FAIL;
}

esp_err_t wifi_manager_scan_start(void)
{
    if (s_wifi_event_group == NULL) {
        ESP_LOGE(TAG, "WiFi manager not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = false
    };

    ESP_LOGI(TAG, "Starting WiFi scan");
    return esp_wifi_scan_start(&scan_config, true);
}

esp_err_t wifi_manager_scan_get_results(wifi_ap_record_t *ap_list, uint16_t max_aps, uint16_t *num_aps)
{
    if (ap_list == NULL || num_aps == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    *num_aps = max_aps;
    esp_err_t err = esp_wifi_scan_get_ap_records(num_aps, ap_list);

    ESP_LOGI(TAG, "WiFi scan found %d networks", *num_aps);
    return err;
}

void wifi_manager_register_callback(wifi_event_cb_t callback, void *user_data)
{
    s_event_callback = callback;
    s_callback_user_data = user_data;
    ESP_LOGI(TAG, "WiFi event callback registered");
}

esp_err_t wifi_manager_clear_credentials(void)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        return err;
    }

    nvs_erase_key(nvs_handle, NVS_KEY_WIFI_SSID);
    nvs_erase_key(nvs_handle, NVS_KEY_WIFI_PASS);
    nvs_commit(nvs_handle);
    nvs_close(nvs_handle);

    ESP_LOGI(TAG, "WiFi credentials cleared from NVS");
    return ESP_OK;
}

// Ping callback data structure
typedef struct {
    uint32_t elapsed_time_ms;
    bool success;
} ping_result_t;

// Ping callback to capture timing
static void ping_success_callback(esp_ping_handle_t hdl, void *args)
{
    ping_result_t *result = (ping_result_t *)args;
    uint32_t elapsed_time;
    esp_ping_get_profile(hdl, ESP_PING_PROF_TIMEGAP, &elapsed_time, sizeof(elapsed_time));
    result->elapsed_time_ms = elapsed_time;
    result->success = true;
    ESP_LOGI(TAG, "Ping success: %lu ms", elapsed_time);
}

static void ping_timeout_callback(esp_ping_handle_t hdl, void *args)
{
    ping_result_t *result = (ping_result_t *)args;
    result->success = false;
    ESP_LOGW(TAG, "Ping timeout");
}

esp_err_t wifi_manager_ping_gateway(uint32_t timeout_ms, uint32_t *ping_time_ms)
{
    if (!wifi_manager_is_connected()) {
        ESP_LOGW(TAG, "Cannot ping: WiFi not connected");
        return ESP_FAIL;
    }

    // Get gateway IP
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (netif == NULL) {
        ESP_LOGE(TAG, "Failed to get netif handle");
        return ESP_FAIL;
    }

    esp_netif_ip_info_t ip_info;
    if (esp_netif_get_ip_info(netif, &ip_info) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get IP info");
        return ESP_FAIL;
    }

    // Configure ping
    esp_ping_config_t ping_config = ESP_PING_DEFAULT_CONFIG();
    ping_config.target_addr.u_addr.ip4.addr = ip_info.gw.addr;  // Gateway IP
    ping_config.target_addr.type = IPADDR_TYPE_V4;
    ping_config.count = 1;  // Send 1 ping for timing
    ping_config.interval_ms = 1000;
    ping_config.timeout_ms = timeout_ms > 0 ? timeout_ms : 5000;

    // Setup callbacks for timing
    ping_result_t result = {0};
    esp_ping_callbacks_t callbacks = {
        .on_ping_success = ping_success_callback,
        .on_ping_timeout = ping_timeout_callback,
        .cb_args = &result
    };

    ESP_LOGI(TAG, "Pinging gateway: " IPSTR, IP2STR(&ip_info.gw));

    // Create ping session with callbacks
    esp_ping_handle_t ping_handle;
    esp_err_t err = esp_ping_new_session(&ping_config, &callbacks, &ping_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create ping session: %s", esp_err_to_name(err));
        return err;
    }

    // Start ping
    err = esp_ping_start(ping_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start ping: %s", esp_err_to_name(err));
        esp_ping_delete_session(ping_handle);
        return err;
    }

    // Wait for ping to complete
    vTaskDelay(pdMS_TO_TICKS(timeout_ms > 0 ? timeout_ms + 1000 : 6000));

    // Stop and cleanup
    esp_ping_stop(ping_handle);
    esp_ping_delete_session(ping_handle);

    // Return timing if requested
    if (ping_time_ms != NULL && result.success) {
        *ping_time_ms = result.elapsed_time_ms;
    }

    ESP_LOGI(TAG, "Ping test completed: %s (%lu ms)",
             result.success ? "success" : "failed", result.elapsed_time_ms);

    return result.success ? ESP_OK : ESP_FAIL;
}
