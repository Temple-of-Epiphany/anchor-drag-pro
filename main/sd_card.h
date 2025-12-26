/**
 * SD Card Driver
 *
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2025-12-25
 * Version: 0.1.0
 *
 * SD card operations for Waveshare ESP32-S3-Touch-LCD-4.3B
 * - SPI mode using GPIO11 (MOSI), GPIO12 (SCK), GPIO13 (MISO)
 * - CS controlled via CH422G EXIO4
 * - FAT filesystem support
 */

#ifndef SD_CARD_H
#define SD_CARD_H

#include <stdbool.h>
#include <stdint.h>

// SD card mount point
#define SD_MOUNT_POINT "/sdcard"

// File info structure
typedef struct {
    char name[256];
    bool is_directory;
    uint64_t size;
} sd_file_info_t;

/**
 * Initialize and mount SD card
 * @return true if successful, false otherwise
 */
bool sd_card_init(void);

/**
 * Unmount SD card
 */
void sd_card_deinit(void);

/**
 * Check if SD card is mounted
 * @return true if mounted, false otherwise
 */
bool sd_card_is_mounted(void);

/**
 * Format SD card (FAT32)
 * @return true if successful, false otherwise
 */
bool sd_card_format(void);

/**
 * Get SD card total and free space
 * @param total_bytes Pointer to store total bytes
 * @param free_bytes Pointer to store free bytes
 * @return true if successful, false otherwise
 */
bool sd_card_get_space(uint64_t *total_bytes, uint64_t *free_bytes);

/**
 * List files in directory
 * @param path Directory path (relative to mount point)
 * @param files Array to store file info
 * @param max_files Maximum number of files to list
 * @param file_count Pointer to store actual file count
 * @return true if successful, false otherwise
 */
bool sd_card_list_dir(const char *path, sd_file_info_t *files, int max_files, int *file_count);

#endif // SD_CARD_H
