/**
 * GPS M8N Test Utility Header
 *
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2026-01-12
 * Version: 0.1.0
 *
 * Utility to test and debug GPS M8N module on I2C bus
 */

#ifndef GPS_TEST_H
#define GPS_TEST_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Scan I2C bus for devices
 * Returns true if GPS module is found at expected address
 */
bool gps_test_scan_i2c(void);

/**
 * Read raw data from GPS M8N module
 * Shows first 256 bytes of data for debugging
 */
void gps_test_read_raw(void);

/**
 * Test GPS data parsing using GPS manager
 */
void gps_test_parse_data(void);

/**
 * Run complete GPS test sequence
 * This is the main function to call for testing GPS M8N
 */
void gps_test_run_all(void);

/**
 * Monitor GPS continuously (for testing)
 * @param duration_seconds How long to monitor (in seconds)
 */
void gps_test_monitor(int duration_seconds);

#ifdef __cplusplus
}
#endif

#endif // GPS_TEST_H
