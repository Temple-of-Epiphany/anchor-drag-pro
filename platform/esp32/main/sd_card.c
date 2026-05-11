/*
 * SD card driver — implementation. See sd_card.h for usage.
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#include "sd_card.h"
#include "board_config.h"
#include "ch422g.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "esp_task_wdt.h"
#include "sdmmc_cmd.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <errno.h>
#include <unistd.h>

static const char *TAG = "sd_card";

/* SD SPI configuration — pins per WaveShare 4.3B board. CS is via CH422G,
 * NOT a GPIO; configure SDSPI with gpio_cs = -1. */
#define SD_SPI_HOST         SPI2_HOST
#define SD_SPI_NO_CS        ((gpio_num_t) -1)
#define SD_SPI_FREQ_KHZ     20000   /* 20 MHz default; SDSPI tops at this */

static sdmmc_card_t *s_card     = NULL;
static bool          s_mounted  = false;
static sdmmc_host_t  s_host     = SDSPI_HOST_DEFAULT();

/* ---- Watchdog helpers ----
 * Some SD operations (mount, format, large writes) take far longer than
 * the default task WDT timeout. We disable for the duration and re-enable.
 * Functions are no-op if the current task is not registered with WDT.
 */
static void wdt_pause(void)
{
    TaskHandle_t self = xTaskGetCurrentTaskHandle();
    /* esp_task_wdt_delete returns ESP_ERR_NOT_FOUND if the task isn't
     * tracked — that's normal at boot, not an error. */
    esp_task_wdt_delete(self);
}

static void wdt_resume(void)
{
    TaskHandle_t self = xTaskGetCurrentTaskHandle();
    esp_task_wdt_add(self);
}

esp_err_t sd_card_init(void)
{
    if (s_mounted) {
        ESP_LOGW(TAG, "already mounted");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "init: SPI%d MOSI=%d MISO=%d SCK=%d CS=via CH422G EXIO%d",
             SD_SPI_HOST + 1,
             SD_PIN_MOSI, SD_PIN_MISO, SD_PIN_SCK,
             CH422G_EXIO_SD_CS);

    /* Step 1: assert SD CS via CH422G (drive EXIO4 LOW). The SDSPI driver
     * will not toggle CS itself — we hold it asserted for the SD lifetime. */
    esp_err_t err = ch422g_sd_cs(true);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ch422g_sd_cs(true) failed: %s", esp_err_to_name(err));
        return err;
    }
    vTaskDelay(pdMS_TO_TICKS(10)); /* let CS settle before SPI traffic */

    /* Step 2: initialize SPI bus. Tolerate "already initialized" (other
     * code may have set it up — currently nothing else uses SPI2_HOST). */
    spi_bus_config_t bus_cfg = {
        .mosi_io_num     = SD_PIN_MOSI,
        .miso_io_num     = SD_PIN_MISO,
        .sclk_io_num     = SD_PIN_SCK,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = 4000,
    };
    err = spi_bus_initialize(SD_SPI_HOST, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "spi_bus_initialize failed: %s", esp_err_to_name(err));
        ch422g_sd_cs(false);
        return err;
    }

    /* Step 3: configure SDSPI device. gpio_cs = -1 = no CS toggle. */
    sdspi_device_config_t slot_cfg = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_cfg.gpio_cs = SD_SPI_NO_CS;
    slot_cfg.host_id = SD_SPI_HOST;

    /* Step 4: mount the FAT filesystem. This is the long-running call. */
    esp_vfs_fat_sdmmc_mount_config_t mount_cfg = {
        .format_if_mount_failed = false, /* never auto-format — that's destructive */
        .max_files              = 8,
        .allocation_unit_size   = 16 * 1024,
    };

    s_host.slot = SD_SPI_HOST;

    wdt_pause();
    err = esp_vfs_fat_sdspi_mount(SD_MOUNT_POINT, &s_host, &slot_cfg,
                                    &mount_cfg, &s_card);
    wdt_resume();

    if (err != ESP_OK) {
        if (err == ESP_FAIL) {
            ESP_LOGE(TAG, "mount failed — card may be unformatted or corrupt "
                          "(format_if_mount_failed is intentionally disabled)");
        } else {
            ESP_LOGE(TAG, "mount failed: %s", esp_err_to_name(err));
        }
        ch422g_sd_cs(false);
        return err;
    }

    s_mounted = true;
    ESP_LOGI(TAG, "mounted at %s", SD_MOUNT_POINT);
    return ESP_OK;
}

bool sd_card_is_mounted(void)
{
    return s_mounted;
}

esp_err_t sd_card_print_info(FILE *out)
{
    if (!s_mounted || s_card == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    sdmmc_card_print_info(out, s_card);
    return ESP_OK;
}

esp_err_t sd_card_get_usage(uint64_t *total_bytes,
                             uint64_t *free_bytes,
                             uint64_t *used_bytes)
{
    if (!s_mounted) {
        return ESP_ERR_INVALID_STATE;
    }
    uint64_t total = 0, free = 0;
    /* esp_vfs_fat_info is the ESP-IDF-native equivalent of statvfs;
     * the standard sys/statvfs.h is not available in ESP-IDF newlib. */
    esp_err_t err = esp_vfs_fat_info(SD_MOUNT_POINT, &total, &free);
    if (err != ESP_OK) {
        return err;
    }
    if (total_bytes) *total_bytes = total;
    if (free_bytes)  *free_bytes  = free;
    if (used_bytes)  *used_bytes  = total > free ? total - free : 0;
    return ESP_OK;
}

size_t sd_safe_write(const void *buf, size_t size, size_t count,
                      FILE *fp, int *err_out)
{
    if (fp == NULL || buf == NULL) {
        if (err_out) *err_out = EINVAL;
        return 0;
    }

    wdt_pause();
    size_t n = fwrite(buf, size, count, fp);
    int saved_errno = errno;
    wdt_resume();

    if (n != count && err_out) {
        *err_out = saved_errno;
    }
    return n;
}

int sd_safe_sync(FILE *fp)
{
    if (fp == NULL) return -1;
    int fd = fileno(fp);
    if (fd < 0) return -1;

    /* Flush stdio buffers first, then fsync to push to the card. */
    fflush(fp);
    wdt_pause();
    int rc = fsync(fd);
    wdt_resume();
    return rc;
}

esp_err_t sd_card_deinit(void)
{
    if (!s_mounted) return ESP_OK;

    wdt_pause();
    esp_err_t err = esp_vfs_fat_sdcard_unmount(SD_MOUNT_POINT, s_card);
    wdt_resume();

    /* Best-effort regardless of unmount status. */
    spi_bus_free(SD_SPI_HOST);
    ch422g_sd_cs(false); /* deassert */
    s_card    = NULL;
    s_mounted = false;

    if (err != ESP_OK) {
        ESP_LOGW(TAG, "unmount returned %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "unmounted");
    }
    return err;
}
