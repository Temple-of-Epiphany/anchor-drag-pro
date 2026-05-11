/*
 * I2C0 shared bus + mutex — implementation.
 *
 * See i2c_bus.h for usage.
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#include "i2c_bus.h"
#include "esp_log.h"
#include <stdbool.h>

static const char *TAG = "i2c_bus";

static i2c_master_bus_handle_t s_bus  = NULL;
static SemaphoreHandle_t       s_lock = NULL;
static bool                    s_init = false;

esp_err_t i2c_bus_init(int sda_gpio, int scl_gpio, uint32_t clk_speed_hz)
{
    if (s_init) {
        ESP_LOGW(TAG, "already initialized; ignoring");
        return ESP_OK;
    }

    s_lock = xSemaphoreCreateMutex();
    if (s_lock == NULL) {
        ESP_LOGE(TAG, "mutex alloc failed");
        return ESP_ERR_NO_MEM;
    }

    i2c_master_bus_config_t cfg = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = sda_gpio,
        .scl_io_num = scl_gpio,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    esp_err_t err = i2c_new_master_bus(&cfg, &s_bus);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_new_master_bus failed: %s", esp_err_to_name(err));
        vSemaphoreDelete(s_lock);
        s_lock = NULL;
        return err;
    }

    s_init = true;
    ESP_LOGI(TAG, "I2C0 init: SDA=%d SCL=%d %lu Hz", sda_gpio, scl_gpio,
             (unsigned long) clk_speed_hz);
    /*
     * Note: clk_speed_hz is per-device on the new ESP-IDF I2C master API;
     * the bus itself runs at whatever each registered device specifies.
     * We log the requested speed for documentation. Devices register with
     * i2c_master_bus_add_device() and pass their per-device clock.
     */

    return ESP_OK;
}

i2c_master_bus_handle_t i2c_bus_handle(void)
{
    return s_bus;
}

bool i2c_bus_lock(uint32_t timeout_ms)
{
    if (s_lock == NULL) {
        return false;
    }
    return xSemaphoreTake(s_lock, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
}

void i2c_bus_unlock(void)
{
    if (s_lock != NULL) {
        xSemaphoreGive(s_lock);
    }
}

void i2c_bus_scan(void)
{
    if (!s_init) {
        ESP_LOGE(TAG, "scan called before init");
        return;
    }

    ESP_LOGI(TAG, "Scanning I2C0 (7-bit addresses 0x08-0x77)...");
    int found = 0;

    /*
     * Probe each address. i2c_master_probe() takes the bus mutex implicitly
     * via the underlying driver, but we also take our application mutex so
     * other tasks don't interleave probe attempts with their own transactions.
     */
    if (!i2c_bus_lock(2000)) {
        ESP_LOGW(TAG, "scan: bus busy, aborting");
        return;
    }

    /*
     * The CH422G I/O expander aliases across its register-as-address
     * scheme (0x20-0x27 for OD bank, 0x30-0x3F for IO bank). All those
     * addresses ACK but represent ONE physical chip. We probe everything
     * but only LOG the canonical addresses to keep the scan output
     * uncluttered. Unknown addresses outside any known alias range are
     * still logged because they could be unexpected hardware.
     */
    int  ch422g_aliases  = 0;     /* CH422G OD bank 0x20-0x27 + IO bank 0x30-0x3F */
    bool ch422g_present  = false;

    for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
        if (i2c_master_probe(s_bus, addr, 50) != ESP_OK) {
            continue;
        }
        found++;

        /* CH422G register-as-address aliases. */
        bool is_ch422g_alias =
            (addr >= 0x20 && addr <= 0x27) ||
            (addr >= 0x30 && addr <= 0x3F);

        if (is_ch422g_alias) {
            ch422g_aliases++;
            ch422g_present = true;
            continue;     /* don't log every alias */
        }

        const char *known = "";
        switch (addr) {
            case 0x4A: known = " (BNO085 / HWT901B IMU)";        break;
            case 0x4B: known = " (BNO085 alternate)";            break;
            case 0x51: known = " (PCF85063A RTC)";               break;
            case 0x5D: known = " (GT911 touch controller)";      break;
            default:   known = "  (unknown)";                    break;
        }
        ESP_LOGI(TAG, "  0x%02X%s", addr, known);
    }

    if (ch422g_present) {
        ESP_LOGI(TAG, "  0x24/0x38 (CH422G I/O expander; %d aliases collapsed)",
                 ch422g_aliases);
    }

    i2c_bus_unlock();
    ESP_LOGI(TAG, "Scan complete: %d address(es) responding (%s)",
             found,
             ch422g_present
                ? "1 CH422G + the rest unique"
                : "all unique");
}
