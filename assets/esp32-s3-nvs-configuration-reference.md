# ESP32-S3 NVS Configuration Reference

**Version:** 1.0.0  
**Author:** Colin Bitterfield  
**Email:** colin@bitterfield.com  
**Date Created:** 2025-12-26  
**Date Updated:** 2025-12-26  

## Changelog

- v1.0.0 (2025-12-26): Initial version

---

## Overview

This document defines recommended persistent configuration storage for ESP32-S3 devices using the Preferences library (NVS). The target hardware includes WiFi, Bluetooth, CAN bus, and dual RS485 ports.

---

## Namespace Organization

| Namespace | Purpose | Max Keys |
|-----------|---------|----------|
| `network` | WiFi, Bluetooth, remote services | ~20 |
| `canbus` | CAN bus configuration | ~10 |
| `rs485-1` | RS485 Port 1 settings | ~10 |
| `rs485-2` | RS485 Port 2 settings | ~10 |
| `device` | Device identity and metadata | ~10 |
| `cal` | Calibration coefficients | ~15 |
| `system` | Boot, watchdog, OTA, debug | ~12 |
| `security` | Keys, tokens, certificates | ~6 |

---

## Network Namespace ("network")

### WiFi Configuration

| Key | Type | Default | Notes |
|-----|------|---------|-------|
| `wifi_ssid` | String | "" | Max 32 chars |
| `wifi_pass` | String | "" | Max 64 chars |
| `wifi_static` | Bool | false | Enable static IP |
| `wifi_ip` | UInt32 | 0 | Static IP as packed uint32 |
| `wifi_subnet` | UInt32 | 0 | Subnet mask |
| `wifi_gw` | UInt32 | 0 | Gateway address |
| `wifi_dns1` | UInt32 | 0 | Primary DNS |
| `wifi_dns2` | UInt32 | 0 | Secondary DNS |
| `hostname` | String | "esp32" | mDNS hostname |

### Bluetooth Configuration

| Key | Type | Default | Notes |
|-----|------|---------|-------|
| `bt_name` | String | "ESP32" | BT device name |
| `bt_pin` | String | "0000" | Pairing PIN |
| `bt_enable` | Bool | false | Enable Bluetooth |
| `bt_bonded` | Bytes | - | Bonded device addresses |

### Remote Services

| Key | Type | Default | Notes |
|-----|------|---------|-------|
| `mqtt_host` | String | "" | Broker address |
| `mqtt_port` | UInt16 | 1883 | Broker port |
| `mqtt_user` | String | "" | Username |
| `mqtt_pass` | String | "" | Password |
| `mqtt_client` | String | "" | Client ID |
| `ntp_server` | String | "pool.ntp.org" | NTP server |
| `tz_offset` | Int16 | 0 | Timezone offset minutes |

---

## CAN Bus Namespace ("canbus")

| Key | Type | Default | Notes |
|-----|------|---------|-------|
| `baud` | UInt32 | 250000 | 125k, 250k, 500k, 1M |
| `node_id` | UInt8 | 1 | CAN node identifier |
| `filters` | Bytes | - | Filter IDs (uint32 array) |
| `masks` | Bytes | - | Filter masks (uint32 array) |
| `silent` | Bool | false | Silent/listen-only mode |
| `loopback` | Bool | false | Loopback mode |
| `term_en` | Bool | false | Termination resistor enable |
| `schema_ver` | Int | 1 | Configuration schema version |

---

## RS485 Port 1 Namespace ("rs485-1")

| Key | Type | Default | Notes |
|-----|------|---------|-------|
| `baud` | UInt32 | 9600 | Baud rate |
| `databits` | UInt8 | 8 | Data bits (7 or 8) |
| `parity` | UInt8 | 0 | 0=none, 1=even, 2=odd |
| `stopbits` | UInt8 | 1 | Stop bits (1 or 2) |
| `mb_addr` | UInt8 | 1 | Modbus slave address |
| `protocol` | UInt8 | 1 | 0=raw, 1=RTU, 2=ASCII |
| `tx_pol` | Bool | true | TX enable pin polarity |
| `ifd_us` | UInt16 | 1750 | Inter-frame delay (us) |
| `resp_ms` | UInt16 | 1000 | Response timeout (ms) |
| `schema_ver` | Int | 1 | Configuration schema version |

---

## RS485 Port 2 Namespace ("rs485-2")

Same structure as RS485 Port 1.

---

## Device Identity Namespace ("device")

| Key | Type | Default | Notes |
|-----|------|---------|-------|
| `uuid` | String | - | Device UUID (36 chars) |
| `serial` | String | - | Serial number |
| `hw_rev` | String | - | Hardware revision |
| `fw_init` | String | - | Initial firmware version |
| `name` | String | "ESP32" | User-assigned name |
| `location` | String | "" | Location description |
| `owner` | String | "" | Owner/operator ID |
| `schema_ver` | Int | 1 | Configuration schema version |

---

## Calibration Namespace ("cal")

| Key | Type | Default | Notes |
|-----|------|---------|-------|
| `adc_offset` | Bytes | - | ADC offset per channel (int16 array) |
| `adc_gain` | Bytes | - | ADC gain per channel (float array) |
| `temp_offset` | Float | 0.0 | Temperature sensor offset |
| `vref_corr` | Float | 1.0 | Voltage reference correction |
| `curr_zero` | Float | 0.0 | Current sensor zero-point |
| `schema_ver` | Int | 1 | Configuration schema version |

---

## System Namespace ("system")

| Key | Type | Default | Notes |
|-----|------|---------|-------|
| `boot_cnt` | UInt32 | 0 | Boot counter |
| `boot_reason` | UInt8 | 0 | Last boot reason code |
| `wdt_sec` | UInt16 | 30 | Watchdog timeout seconds |
| `sleep_en` | Bool | false | Sleep mode enable |
| `sleep_ms` | UInt32 | 60000 | Sleep interval ms |
| `debug_en` | Bool | false | Debug logging enable |
| `ota_en` | Bool | true | OTA update enable |
| `ota_url` | String | "" | OTA update URL |
| `factory_rst` | Bool | false | Factory reset flag |
| `schema_ver` | Int | 1 | Configuration schema version |

---

## Security Namespace ("security")

| Key | Type | Default | Notes |
|-----|------|---------|-------|
| `api_key` | String | "" | API key |
| `auth_token` | String | "" | Auth token |
| `tls_fp` | Bytes | - | TLS certificate fingerprint |
| `enc_key` | Bytes | - | Local encryption key |
| `acl_level` | UInt8 | 0 | Access control level |
| `schema_ver` | Int | 1 | Configuration schema version |

---

## Code Examples

### Basic Usage

```cpp
#include <Preferences.h>

Preferences prefs;

void loadNetworkConfig() {
    prefs.begin("network", true);  // true = read-only
    
    String ssid = prefs.getString("wifi_ssid", "");
    String password = prefs.getString("wifi_pass", "");
    bool useStatic = prefs.getBool("wifi_static", false);
    
    prefs.end();
}

void saveNetworkConfig(const char* ssid, const char* password) {
    prefs.begin("network", false);  // false = read-write
    
    prefs.putString("wifi_ssid", ssid);
    prefs.putString("wifi_pass", password);
    
    prefs.end();
}
```

### Schema Versioning for Firmware Updates

```cpp
#define CANBUS_SCHEMA_VERSION 2

void migrateCanConfig() {
    prefs.begin("canbus", false);
    
    int currentSchema = prefs.getInt("schema_ver", 0);
    
    if (currentSchema < 1) {
        // Migration from v0 to v1: add default node_id
        prefs.putUInt("node_id", 1);
    }
    
    if (currentSchema < 2) {
        // Migration from v1 to v2: add termination resistor flag
        prefs.putBool("term_en", false);
    }
    
    if (currentSchema < CANBUS_SCHEMA_VERSION) {
        prefs.putInt("schema_ver", CANBUS_SCHEMA_VERSION);
    }
    
    prefs.end();
}
```

### Factory Reset

```cpp
void factoryReset() {
    const char* namespaces[] = {
        "network", "canbus", "rs485-1", "rs485-2",
        "device", "cal", "system", "security"
    };
    
    for (const char* ns : namespaces) {
        prefs.begin(ns, false);
        prefs.clear();
        prefs.end();
    }
    
    ESP.restart();
}
```

### Storing Arrays with putBytes

```cpp
void saveCanFilters(uint32_t* filters, size_t count) {
    prefs.begin("canbus", false);
    prefs.putBytes("filters", filters, count * sizeof(uint32_t));
    prefs.end();
}

void loadCanFilters(uint32_t* filters, size_t maxCount, size_t* actualCount) {
    prefs.begin("canbus", true);
    size_t len = prefs.getBytesLength("filters");
    *actualCount = len / sizeof(uint32_t);
    if (*actualCount > maxCount) *actualCount = maxCount;
    prefs.getBytes("filters", filters, *actualCount * sizeof(uint32_t));
    prefs.end();
}
```

---

## Design Guidelines

### Key Naming

- Maximum key length: 15 characters
- Use lowercase with underscores
- Be consistent across namespaces

### Write Cycle Considerations

NVS flash has approximately 100,000 write cycles per sector. Avoid storing:

- Sensor readings or logs (use SPIFFS/LittleFS)
- Data changing more frequently than once per minute
- Large binary blobs over a few KB

### Alternative Storage

| Data Type | Recommended Storage |
|-----------|---------------------|
| Configuration | NVS (Preferences) |
| Logs | SPIFFS/LittleFS or SD card |
| Large files | SPIFFS/LittleFS or SD card |
| Sensor data | SD card or external flash |
| Firmware images | OTA partitions |

### Namespace Limits

- Maximum namespace name: 15 characters
- No hard limit on namespaces, but each consumes NVS entries
- Plan for ~100 total keys across all namespaces

---

## References

- [ESP-IDF NVS Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/storage/nvs_flash.html)
- [Arduino ESP32 Preferences Library](https://github.com/espressif/arduino-esp32/tree/master/libraries/Preferences)
