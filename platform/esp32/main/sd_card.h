/*
 * SD card driver — WaveShare ESP32-S3-Touch-LCD-4.3B.
 *
 * Uses ESP-IDF built-in sdspi + vfs_fat. Mounted at /sdcard.
 *
 * Wiring quirk: SD CS is wired through CH422G EXIO4, not a real GPIO.
 * SDSPI driver is configured with gpio_cs = -1 (no CS toggle in driver),
 * and CH422G holds EXIO4 LOW (CS asserted) for the entire mounted
 * lifetime. This is safe because the SPI2_HOST bus has only the SD card
 * on it.
 *
 * Watchdog discipline: f_write / f_sync / mount can take 10+ seconds
 * (especially first-time format). Use sd_safe_write() / sd_safe_sync()
 * wrappers which disable+re-enable the task watchdog around the call.
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SD_MOUNT_POINT      "/sdcard"

/*
 * Initialize the SD card.
 * Requires i2c_bus_init() AND ch422g_init() to have already succeeded.
 *
 * On success: card is mounted at /sdcard, ready for fopen/fread/fwrite.
 * On failure: returns specific esp_err_t. /sdcard is NOT available.
 *
 * Internally disables the task watchdog for the duration of mount
 * (can take 10+ seconds, especially on first-format paths).
 */
esp_err_t sd_card_init(void);

/* Returns true if the SD card is currently mounted. */
bool sd_card_is_mounted(void);

/*
 * Print card information (capacity, type, max freq, etc.) to the given
 * FILE* (use stdout for serial console output). Returns ESP_ERR_INVALID_STATE
 * if not mounted.
 */
esp_err_t sd_card_print_info(FILE *out);

/*
 * Get total / free / used bytes on the mounted filesystem.
 * Pass NULL for any field you don't want.
 */
esp_err_t sd_card_get_usage(uint64_t *total_bytes,
                             uint64_t *free_bytes,
                             uint64_t *used_bytes);

/*
 * Watchdog-safe write to a file. Wraps fwrite() with the task watchdog
 * disabled for the duration of the call. Returns the number of items
 * written (same semantics as fwrite). On any error, *err_out (if non-NULL)
 * is set to the errno value.
 *
 * Use this for any write that might exceed ~5 seconds (large logs,
 * OTA-from-SD reads, etc.). For small writes, plain fwrite() is fine.
 */
size_t sd_safe_write(const void *buf, size_t size, size_t count,
                      FILE *fp, int *err_out);

/*
 * Watchdog-safe fsync — flushes any buffered data to the card.
 * Wraps fsync(fileno(fp)).
 */
int sd_safe_sync(FILE *fp);

/*
 * Unmount and tear down. Releases CH422G EXIO4 (returns CS to deasserted).
 * Safe to call even if not mounted.
 */
esp_err_t sd_card_deinit(void);

#ifdef __cplusplus
}
#endif
