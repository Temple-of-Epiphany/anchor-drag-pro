# ESP32-S3-Touch-LCD-4.3B Hardware Configuration Guide

**Board:** Waveshare ESP32-S3-Touch-LCD-4.3B
**MCU:** ESP32-S3-WROOM-1-N16R8 (16MB Flash, 8MB PSRAM)
**Display:** 4.3" 800×480 IPS LCD (ST7262 RGB controller)
**Touch:** GT911 5-point capacitive touch
**Created:** 2025-12-26
**Author:** Colin Bitterfield

---

## Critical Lessons Learned

### Library Discovery Failures

**What Went Wrong:**
When reviewing the Waveshare wiki for ESP-IDF setup, I made these mistakes:
1. ❌ Focused only on low-level I2C code instead of checking official component dependencies
2. ❌ Didn't read the wiki's "Resources" or "Preparation" sections listing required libraries
3. ❌ Failed to notice the Arduino library directory containing the ESP_IO_Expander component
4. ❌ Didn't recognize ESP_IO_Expander was an official Espressif component

**Root Cause:**
- **Tunnel Vision:** Replicated ESP-IDF demo's direct I2C approach without questioning if it was the best practice
- **Incomplete Research:** Jumped to code examples without reading setup/requirements
- **Assumption Bias:** Assumed low-level I2C writes were "official" when they were just one implementation approach

**Correct Process:**
1. ✅ Read **entire wiki page** including Dependencies/Libraries sections
2. ✅ Check `idf_component.yml` in Waveshare demos for declared components
3. ✅ Search for **managed components** before writing custom drivers
4. ✅ Recognize professional vendors use abstraction libraries, not raw protocols

---

## Required ESP-IDF Components

### Component Dependencies (idf_component.yml)

Add to `main/idf_component.yml`:

```yaml
dependencies:
  idf:
    version: ">=5.1.0"

  lvgl/lvgl:
    version: ">8.3.9,<9"  # LVGL 8.x (NOT 9.x)
    public: true

  espressif/esp_lcd_touch_gt911:
    version: ^1.0.0
    public: true
```

**Note:** ESP_IO_Expander component for CH422G is available via Espressif Component Registry but requires additional device-specific libraries. Current implementation uses direct I2C as a working fallback.

### CMake Component Requirements

Add to `main/CMakeLists.txt`:

```cmake
idf_component_register(
    ...
    REQUIRES
        fatfs           # FAT filesystem for SD card
        console         # Console/REPL for debugging
        sdmmc           # SD/MMC card driver
        vfs             # Virtual filesystem
        spi_flash       # SPI flash access
        nvs_flash       # Non-volatile storage
)
```

---

## Complete Pin Configuration

### RGB LCD Interface (ST7262 Controller)

**Resolution:** 800×480 pixels
**Color Depth:** RGB565 (16-bit)
**Interface:** 16-bit parallel RGB
**Pixel Clock:** 16 MHz

#### Control Signals
| Signal | GPIO | Description |
|--------|------|-------------|
| VSYNC  | 3    | Vertical sync |
| HSYNC  | 46   | Horizontal sync |
| DE     | 5    | Data enable |
| PCLK   | 7    | Pixel clock |

#### Data Lines (RGB565)
| Color Channel | Bits  | GPIO Pins |
|---------------|-------|-----------|
| **Blue**  | B3-B7 (5 bits) | 14, 38, 18, 17, 10 |
| **Green** | G2-G7 (6 bits) | 39, 0, 45, 48, 47, 21 |
| **Red**   | R3-R7 (5 bits) | 1, 2, 42, 41, 40 |

**Timing Parameters:**
```c
#define LCD_PIXEL_CLOCK_HZ  (16 * 1000 * 1000)  // 16 MHz
#define LCD_HPW             4   // Horizontal pulse width
#define LCD_HBP             8   // Horizontal back porch
#define LCD_HFP             8   // Horizontal front porch
#define LCD_VPW             4   // Vertical pulse width
#define LCD_VBP             8   // Vertical back porch
#define LCD_VFP             8   // Vertical front porch
#define LCD_BOUNCE_BUFFER_SIZE  (LCD_WIDTH * 10)  // 800×10 pixels
```

---

### I2C Buses

**CRITICAL:** This board uses **ONE I2C bus** (I2C0) for ALL peripherals, not separate buses.

#### I2C0 - ALL Peripherals
| Parameter | Value | Description |
|-----------|-------|-------------|
| SDA | GPIO 8 | I2C data line |
| SCL | GPIO 9 | I2C clock line |
| Frequency | 400 kHz | Fast mode |

**Devices on I2C0:**
- **0x24/0x38:** CH422G I/O Expander (read/write addresses)
- **0x5D:** GT911 Touch Controller
- **0x51:** PCF85063A RTC
- **0x42:** NEO-8M GPS (optional external sensor)

---

### CH422G I/O Expander

**I2C Addresses:**
- **Read Address:** 0x24
- **Write Address:** 0x38

**Pin Assignments:**
| EXIO Pin | Function | Active State |
|----------|----------|--------------|
| EXIO0 | Reserved | - |
| EXIO1 | Touch Reset | Active LOW |
| EXIO2 | LCD Backlight | Active HIGH |
| EXIO3 | LCD Reset | Active HIGH |
| EXIO4 | SD Card CS | Active LOW |
| EXIO5 | USB Selection | HIGH = CAN mode |

**Initialization Sequence (Working Implementation):**
```c
// Configure CH422G for SD card access
uint8_t write_buf = 0x01;
i2c_master_write_to_device(I2C_NUM_0, 0x24, &write_buf, 1, pdMS_TO_TICKS(1000));

write_buf = 0x0A;  // 0b00001010:
// EXIO1=1 (Touch reset released)
// EXIO3=1 (LCD reset released)
// EXIO4=0 (SD CS active)
// EXIO2=1 (LCD backlight ON)
i2c_master_write_to_device(I2C_NUM_0, 0x38, &write_buf, 1, pdMS_TO_TICKS(1000));
```

**Future Enhancement - ESP_IO_Expander Library:**
```c
// Preferred approach using official Espressif library
// (Requires additional device-specific component not yet integrated)
#include "esp_io_expander.h"
#include "esp_io_expander_ch422g.h"

esp_io_expander_handle_t io_expander;
esp_io_expander_new_i2c_ch422g(I2C_NUM_0, 0x24, &io_expander);
esp_io_expander_set_dir(io_expander, SD_CS | LCD_BL | LCD_RST, IO_EXPANDER_OUTPUT);
esp_io_expander_set_level(io_expander, SD_CS, 0);  // Enable SD card
```

---

### GT911 Touch Controller

| Parameter | Value | Description |
|-----------|-------|-------------|
| I2C Address | 0x5D | Touch data |
| I2C Bus | I2C0 | GPIO 8/9 |
| Interrupt | GPIO 4 | Active LOW |
| Reset | CH422G EXIO1 | Via I/O expander |
| Max Points | 5 | 5-point multi-touch |

---

### SD Card (SPI Mode)

**Interface:** SPI2_HOST
**Speed:** 20 MHz
**Filesystem:** FAT32

| Signal | GPIO | Notes |
|--------|------|-------|
| MOSI | 11 | SPI data out |
| MISO | 13 | SPI data in |
| SCK | 12 | SPI clock |
| CS | -1 | Controlled via CH422G EXIO4 |

**Mount Configuration:**
```c
esp_vfs_fat_sdmmc_mount_config_t mount_config = {
    .format_if_mount_failed = false,
    .max_files = 5,
    .allocation_unit_size = 16 * 1024
};

spi_bus_config_t bus_cfg = {
    .mosi_io_num = 11,
    .miso_io_num = 13,
    .sclk_io_num = 12,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = 4000,
};

sdmmc_host_t host = SDSPI_HOST_DEFAULT();
spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);

sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
slot_config.gpio_cs = -1;  // CS controlled via CH422G
slot_config.host_id = host.slot;
```

**Critical: Watchdog Management**

SD card operations can take 30+ seconds (especially format). Disable watchdog during operations:

```c
TaskHandle_t current_task = xTaskGetCurrentTaskHandle();
esp_task_wdt_delete(current_task);

// ... SD card operation ...

esp_task_wdt_add(current_task);  // Re-enable on ALL return paths!
```

---

### CAN Bus / NMEA 2000

**Interface:** TWAI (Two-Wire Automotive Interface)
**Speed:** 250 kbps (NMEA 2000 standard)
**Termination:** 120Ω onboard resistor (enable if at network end)

| Signal | GPIO | Description |
|--------|------|-------------|
| CAN_TX | 15 | Transmit |
| CAN_RX | 16 | Receive |

**Note:** When `USB_SEL` (CH422G EXIO5) is HIGH, the FSUSB42UMX chip routes GPIO19/20 to CAN_TX/RX instead of USB D+/D-.

---

### RS485 Serial

**Interface:** UART1
**Default Baud:** 4800 (NMEA 0183)

| Signal | GPIO | Description |
|--------|------|-------------|
| TXD | 44 | Transmit data |
| RXD | 43 | Receive data |

---

### RTC - PCF85063A

| Parameter | Value | Description |
|-----------|-------|-------------|
| I2C Address | 0x51 | Real-time clock |
| I2C Bus | I2C0 | GPIO 8/9 |
| Interrupt | GPIO 6 | Alarm output |

---

### USB Interface

| Signal | GPIO | Description |
|--------|------|-------------|
| USB_DN | 19 | USB D- (negative) |
| USB_DP | 20 | USB D+ (positive) |

**USB/CAN Selection:**
- When `USB_SEL` (EXIO5) = LOW: USB function enabled
- When `USB_SEL` (EXIO5) = HIGH: CAN bus enabled, GPIO19/20 → CAN_TX/RX

---

### Isolated Digital I/O

**Outputs (Open-Drain, 5-36V, 450mA max):**
- ISO_OUTPUT_0: Buzzer / Relay
- ISO_OUTPUT_1: Relay output

**Inputs (Optocoupler, 5-36V):**
- ISO_INPUT_0: Digital input 0
- ISO_INPUT_1: Digital input 1

---

## LVGL Configuration

### Display Driver Setup (Waveshare Mode 3)

**Mode 3:** Direct-Mode with hardware-managed VSYNC synchronization

**Key Configuration:**
```c
lv_disp_drv_t disp_drv;
disp_drv.direct_mode = 1;  // CRITICAL for Mode 3
disp_drv.flush_cb = lvgl_flush_cb;

// In flush callback - wait for VSYNC on LAST flush area
if (lv_disp_flush_is_last(drv)) {
    esp_lcd_panel_draw_bitmap(panel, ...);
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);  // Wait for VSYNC ISR
}
```

**Frame Buffers:**
- Use RGB panel's internal frame buffers (allocated in PSRAM)
- Buffer size: 800×480 pixels × 2 buffers
- Retrieved via `esp_lcd_rgb_panel_get_frame_buffer()`

**Task Configuration:**
```c
#define LVGL_TASK_STACK     10240  // 10KB (increased for SD card ops)
#define LVGL_TASK_PRIORITY  2
#define LVGL_TICK_PERIOD_MS 10     // 10ms tick timer
```

---

## Power Configuration

**Input Voltage:** 7-36V DC
**Nominal Systems:** 12V/24V marine/automotive
**Protection:** Reverse polarity, overcurrent

---

## Common Pitfalls and Solutions

### 1. I2C Bus Conflicts
**Problem:** Trying to use separate I2C buses for touch vs. expander
**Solution:** ALL I2C devices share I2C0 (GPIO 8/9)

### 2. SD Card Watchdog Timeouts
**Problem:** System reboots during SD format/mount
**Solution:** Disable watchdog before long operations, re-enable on ALL return paths

### 3. LVGL Message Box Crashes
**Problem:** Accessing button children returns NULL
**Solution:** Use `LV_EVENT_VALUE_CHANGED` on message box, not `LV_EVENT_CLICKED` on children

### 4. Display Tearing
**Problem:** Visible screen artifacts during rendering
**Solution:** Enable `direct_mode=1` and wait for VSYNC in flush callback

### 5. Touch Not Responding
**Problem:** GT911 not detecting touches
**Solution:** Ensure CH422G EXIO1 is HIGH (touch reset released)

---

## Verification Checklist

- [ ] I2C bus initialized at 400kHz on GPIO 8/9
- [ ] CH422G responds at addresses 0x24 and 0x38
- [ ] LCD backlight illuminates (CH422G EXIO2 HIGH)
- [ ] Touch controller responds at 0x5D
- [ ] Display shows test pattern without tearing
- [ ] Touch responds to all 5 simultaneous points
- [ ] SD card mounts and reads/writes files
- [ ] RTC maintains time across power cycles
- [ ] CAN bus receives NMEA 2000 messages at 250kbps

---

## Reference Documentation

- **Waveshare Wiki:** https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-4.3B
- **Waveshare GitHub:** https://github.com/waveshareteam/ESP32-S3-Touch-LCD-4.3B
- **ESP-IDF Docs:** https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/
- **LVGL 8.x Docs:** https://docs.lvgl.io/8.3/
- **Component Registry:** https://components.espressif.com/

---

**Last Updated:** 2025-12-26
**Status:** Verified working configuration with direct I2C for CH422G
