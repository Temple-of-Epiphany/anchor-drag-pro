/**
 * SD Card Driver Implementation
 *
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2025-12-25
 * Date Updated: 2025-12-26
 * Version: 0.2.0
 */

#include "sd_card.h"
#include "board_config.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "driver/i2c.h"
#include "esp_task_wdt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>

static const char *TAG = "sd_card";

// SD card handle
static sdmmc_card_t *card = NULL;
static bool is_mounted = false;

// SPI configuration for SD card (match Waveshare demo)
#define PIN_NUM_MISO     13
#define PIN_NUM_MOSI     11
#define PIN_NUM_CLK      12
#define PIN_NUM_CS       -1  // CS controlled via CH422G EXIO4

// SD host configuration (global, like Waveshare demo)
static sdmmc_host_t host = SDSPI_HOST_DEFAULT();

bool sd_card_init(void) {
    if (is_mounted) {
        ESP_LOGW(TAG, "SD card already mounted");
        return true;
    }

    // Disable watchdog for this task during SD card operations (can take 10+ seconds)
    TaskHandle_t current_task = xTaskGetCurrentTaskHandle();
    esp_task_wdt_delete(current_task);

    esp_err_t ret;
    ESP_LOGW(TAG, "=== SD CARD INIT START ===");

    // Configure CH422G for SD card (direct I2C - working approach)
    ESP_LOGI(TAG, "Configuring CH422G for SD card access");

    uint8_t write_buf = 0x01;
    ret = i2c_master_write_to_device(I2C_MASTER_NUM, 0x24, &write_buf, 1, pdMS_TO_TICKS(1000));
    ESP_LOGI(TAG, "CH422G write 0x01 to 0x24: %s", esp_err_to_name(ret));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure CH422G address 0x24");
        esp_task_wdt_add(current_task);
        return false;
    }

    write_buf = 0x0A;
    ret = i2c_master_write_to_device(I2C_MASTER_NUM, 0x38, &write_buf, 1, pdMS_TO_TICKS(1000));
    ESP_LOGI(TAG, "CH422G write 0x0A to 0x38: %s", esp_err_to_name(ret));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure CH422G address 0x38");
        esp_task_wdt_add(current_task);
        return false;
    }

    ESP_LOGI(TAG, "CH422G configured successfully");
    vTaskDelay(pdMS_TO_TICKS(200));  // Allow hardware to settle

    // Mount configuration
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    // SPI bus configuration
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };

    // Initialize SPI bus (use SDSPI_DEFAULT_DMA like Waveshare)
    ret = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "Failed to initialize bus: %s", esp_err_to_name(ret));
        esp_task_wdt_add(current_task);
        return false;
    }
    if (ret == ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "SPI bus already initialized (OK)");
    }

    // Configure SD card slot
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = host.slot;

    // Mount filesystem
    ESP_LOGW(TAG, "Mounting filesystem");
    ret = esp_vfs_fat_sdspi_mount(SD_MOUNT_POINT, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGW(TAG, "Failed to mount filesystem. Card may need formatting.");
        } else {
            ESP_LOGW(TAG, "Failed to initialize the card (%s).", esp_err_to_name(ret));
        }
        esp_task_wdt_add(current_task);
        return false;
    }

    is_mounted = true;
    ESP_LOGW(TAG, "Filesystem mounted");

    // Print card info
    if (card != NULL) {
        sdmmc_card_print_info(stdout, card);
    }

    // Re-enable watchdog
    esp_task_wdt_add(current_task);

    return true;
}

void sd_card_deinit(void) {
    if (!is_mounted) {
        return;
    }

    ESP_LOGW(TAG, "Unmounting SD card");
    esp_vfs_fat_sdcard_unmount(SD_MOUNT_POINT, card);
    spi_bus_free(host.slot);

    card = NULL;
    is_mounted = false;
}

bool sd_card_is_mounted(void) {
    return is_mounted;
}

bool sd_card_format(void) {
    ESP_LOGW(TAG, "Formatting SD card - this may take 30+ seconds...");

    // Disable watchdog for this task during SD card format (can take 30+ seconds)
    TaskHandle_t current_task = xTaskGetCurrentTaskHandle();
    esp_task_wdt_delete(current_task);

    // Unmount if mounted
    if (is_mounted) {
        ESP_LOGW(TAG, "Unmounting SD card before format");
        sd_card_deinit();
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    esp_err_t ret;

    // Configure CH422G for SD card (direct I2C)
    ESP_LOGI(TAG, "Configuring CH422G for SD card access (format)");

    uint8_t write_buf = 0x01;
    ret = i2c_master_write_to_device(I2C_MASTER_NUM, 0x24, &write_buf, 1, pdMS_TO_TICKS(1000));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write to CH422G address 0x24: %s", esp_err_to_name(ret));
        esp_task_wdt_add(current_task);
        return false;
    }

    write_buf = 0x0A;
    ret = i2c_master_write_to_device(I2C_MASTER_NUM, 0x38, &write_buf, 1, pdMS_TO_TICKS(1000));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write to CH422G address 0x38: %s", esp_err_to_name(ret));
        esp_task_wdt_add(current_task);
        return false;
    }

    ESP_LOGI(TAG, "CH422G configured for SD card");
    vTaskDelay(pdMS_TO_TICKS(200));  // Allow hardware to settle

    // Mount configuration with format enabled
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = true,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    // SPI bus configuration
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };

    // Initialize SPI bus (use SDSPI_DEFAULT_DMA like Waveshare)
    ret = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "Failed to initialize bus: %s", esp_err_to_name(ret));
        esp_task_wdt_add(current_task);
        return false;
    }
    if (ret == ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "SPI bus already initialized (OK)");
    }

    // Configure SD card slot
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = host.slot;

    ESP_LOGW(TAG, "Starting format operation (FAT32)...");

    // Format by mounting with format flag
    ret = esp_vfs_fat_sdspi_mount(SD_MOUNT_POINT, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to format SD card: %s", esp_err_to_name(ret));
        spi_bus_free(host.slot);
        esp_task_wdt_add(current_task);
        return false;
    }

    is_mounted = true;
    ESP_LOGW(TAG, "SD card formatted successfully!");

    // Print card info
    if (card != NULL) {
        sdmmc_card_print_info(stdout, card);
    }

    // Re-enable watchdog
    esp_task_wdt_add(current_task);

    return true;
}

bool sd_card_get_space(uint64_t *total_bytes, uint64_t *free_bytes) {
    if (!is_mounted) {
        ESP_LOGE(TAG, "SD card not mounted");
        return false;
    }

    FATFS *fs;
    DWORD fre_clust;
    esp_err_t ret = f_getfree("0:", &fre_clust, &fs);
    if (ret != FR_OK) {
        ESP_LOGE(TAG, "Failed to get free space: %d", ret);
        return false;
    }

    uint64_t total_sectors = (fs->n_fatent - 2) * fs->csize;
    uint64_t free_sectors = fre_clust * fs->csize;

    if (total_bytes) {
        *total_bytes = total_sectors * fs->ssize;
    }
    if (free_bytes) {
        *free_bytes = free_sectors * fs->ssize;
    }

    return true;
}

bool sd_card_list_dir(const char *path, sd_file_info_t *files, int max_files, int *file_count) {
    if (!is_mounted) {
        ESP_LOGE(TAG, "SD card not mounted");
        return false;
    }

    // Disable watchdog during directory listing
    TaskHandle_t current_task = xTaskGetCurrentTaskHandle();
    esp_task_wdt_delete(current_task);

    char full_path[300];
    snprintf(full_path, sizeof(full_path), "%s/%s", SD_MOUNT_POINT, path);

    ESP_LOGI(TAG, "Opening directory: %s", full_path);

    DIR *dir = opendir(full_path);
    if (!dir) {
        ESP_LOGE(TAG, "Failed to open directory: %s", full_path);
        esp_task_wdt_add(current_task);
        return false;
    }

    int count = 0;
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL && count < max_files) {
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Get file info
        char file_path[600];
        snprintf(file_path, sizeof(file_path), "%s/%s", full_path, entry->d_name);

        struct stat st;
        if (stat(file_path, &st) == 0) {
            strncpy(files[count].name, entry->d_name, sizeof(files[count].name) - 1);
            files[count].name[sizeof(files[count].name) - 1] = '\0';
            files[count].is_directory = S_ISDIR(st.st_mode);
            files[count].size = st.st_size;
            count++;
        }
    }

    closedir(dir);

    // Re-enable watchdog
    esp_task_wdt_add(current_task);

    if (file_count) {
        *file_count = count;
    }

    ESP_LOGI(TAG, "Listed %d files in %s", count, path);
    return true;
}
