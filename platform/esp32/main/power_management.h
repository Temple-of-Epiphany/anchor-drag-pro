/**
 * Power Management Module
 *
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2025-12-25
 * Version: 0.1.0
 *
 * Deep sleep and power management for ESP32-S3
 * - Deep sleep mode with touch wake-up
 * - State preservation in NVS
 * - Wake-up detection and restoration
 */

#ifndef POWER_MANAGEMENT_H
#define POWER_MANAGEMENT_H

#include <stdbool.h>
#include "esp_sleep.h"

/**
 * Initialize power management system
 * Call during app startup to configure wake sources
 */
void power_mgmt_init(void);

/**
 * Enter deep sleep mode (software power off)
 * Device will wake on:
 * - Touch screen press
 * - GPIO button press (if configured)
 * - Timer (if configured)
 */
void power_mgmt_sleep(void);

/**
 * Check if device woke from deep sleep
 * @return true if woke from sleep, false if cold boot
 */
bool power_mgmt_is_wake_from_sleep(void);

/**
 * Get the wake-up cause
 * @return Wake-up cause (esp_sleep_wakeup_cause_t)
 */
esp_sleep_wakeup_cause_t power_mgmt_get_wake_cause(void);

/**
 * Configure timer wake-up (optional)
 * @param hours Number of hours to sleep before auto wake-up
 */
void power_mgmt_set_timer_wakeup(int hours);

/**
 * Save current system state to NVS before sleep
 */
void power_mgmt_save_state(void);

/**
 * Restore system state from NVS after wake-up
 */
void power_mgmt_restore_state(void);

#endif // POWER_MANAGEMENT_H
