# Library Choices — ESP-IDF Drivers and Managed Components

**Version:** 0.1.0
**Status:** Draft
**Date:** 2026-04-26
**Author:** Colin Bitterfield <colin@bitterfield.com>

This document captures the deliberate choice of which library, managed component, or vendored driver is used for each hardware subsystem on the WaveShare ESP32-S3-Touch-LCD-4.3B.

## Decision principles

In priority order:

1. **ESP-IDF built-in driver** when one exists and is current (e.g., TWAI, UART, SDMMC, esp_lcd_panel_rgb)
2. **Espressif-published managed component** when one exists on the [Component Registry](https://components.espressif.com/) (e.g., `espressif/esp_io_expander_ch422g`, `espressif/esp_lcd_touch_gt911`)
3. **Other reputable managed component** (e.g., `lvgl/lvgl`)
4. **Vendored driver** (in our `platform/esp32/main/` or `components/`) only when no maintained option exists (e.g., PCF85063A RTC)

## Reference: WaveShare's official ESP-IDF examples

[`waveshareteam/ESP32-S3-Touch-LCD-4.3B`](https://github.com/waveshareteam/ESP32-S3-Touch-LCD-4.3B/tree/main/examples/ESP-IDF) ships eight numbered examples. We use them as **reference** for hardware sequences (init order, magic registers, timing) but **not as a library bundle to copy verbatim** — WaveShare vendors their own CH422G/SD/RTC drivers rather than using current managed components, and uses the legacy `driver/i2c.h` API rather than the modern `i2c_master`.

| WaveShare example | What we learn from it |
|---|---|
| `01_I2C_Test` | Pin assignments (GPIO 8 SDA / GPIO 9 SCL), 400 kHz, address scan pattern |
| `03_SD_Test` | SD card via SPI, FAT mount points, CS via CH422G EXIO4 |
| `04_RTC_Test` | PCF85063A register layout, init sequence — vendored driver is our reference |
| `05_IO_Test` | CH422G register addressing scheme (0x24 mode / 0x23 OD / 0x38 IO) — confirms managed component behavior |
| `06_TWAItransmit` / `07_TWAIreceive` | TWAI pin assignments (GPIO 15 TX / 16 RX), 250 kbps for N2K |
| `08_lvgl_Porting` | RGB LCD pin map, esp_lcd_panel_rgb usage, LVGL bounce buffer config |

## Per-subsystem choice and rationale

### I²C bus

| | |
|---|---|
| **Choice** | New ESP-IDF `i2c_master` API (`driver/i2c_master.h`) — not legacy `driver/i2c.h` |
| **Why** | New API uses bus + device handle pattern, designed for shared-bus scenarios with multiple devices. Legacy `driver/i2c.h` is deprecated in IDF 6.x. Espressif's managed components (esp_io_expander_ch422g v1.0+, esp_lcd_touch_gt911 v1.5+) support both, but new code should use the new API. |
| **WaveShare uses** | Legacy `driver/i2c.h` — diverges from our choice but their example code can be ported by replacing the I2C calls. Hardware behavior is identical. |
| **Local** | `platform/esp32/main/i2c_bus.{c,h}` — wraps the bus handle with a FreeRTOS mutex (every transaction must hold the lock per Workstream 0 prereq #37). |

### CH422G I/O expander

| | |
|---|---|
| **Choice** | `espressif/esp_io_expander_ch422g` (managed component) |
| **Why** | Maintained by Espressif, used by other boards too, abstracts the unusual register-as-address scheme cleanly. The v0.1 archive already used this — it's a known-good choice. |
| **WaveShare uses** | Vendored driver in `lib/CH422G/`. They appear to predate the espressif component or prefer total control. We don't have either reason. |
| **Pin functions** | EXIO0 reserved · EXIO1 touch reset · EXIO2 LCD backlight · EXIO3 LCD reset · EXIO4 SD CS · EXIO5 USB/CAN switch |
| **I²C address** | The chip uses different register addresses (0x24 mode, 0x23 OD, 0x38 IO) instead of a single slave address. Managed component handles this. |

### Touch controller (GT911)

| | |
|---|---|
| **Choice** | `espressif/esp_lcd_touch_gt911 ^1` (managed component) |
| **Why** | Standard. WaveShare uses it directly in their LVGL example. |
| **I²C address** | 0x5D · IRQ on GPIO 4 · Reset via CH422G EXIO1 |

### Display (ST7262 RGB LCD)

| | |
|---|---|
| **Choice** | ESP-IDF built-in `esp_lcd_panel_rgb` (no separate component needed) |
| **Why** | Built into IDF since 4.4. WaveShare uses it directly. |
| **Configuration** | 800×480, 16 bpp RGB565, 16-bit data bus, bounce buffer 800×CONFIG height, Mode 3 direct render |
| **Pins** | VSYNC=3, HSYNC=46, DE=5, PCLK=7, DATA0..15 across various GPIO (see board_config.h when populated). Backlight via CH422G EXIO2. Reset via CH422G EXIO3. |
| **Local wrapper** | A thin `display.{c,h}` that wires the panel + RGB driver into our app structure (deferred until graphics phase per product priority). |

### LVGL

| | |
|---|---|
| **Choice** | `lvgl/lvgl ~8.3` (managed component) — pin to v8.x |
| **Why** | WaveShare uses 8.3.x. v9 has breaking API changes; defer migration until LVGL 9.x ecosystem matures. |
| **Status** | Deferred. Product priority is config / CLI / OTA before graphics. |

### SD card

| | |
|---|---|
| **Choice** | ESP-IDF built-in: `sdmmc_host` (SPI mode) + `esp_vfs_fat` |
| **Why** | Modern, well-maintained, supports both SPI and SDIO. WaveShare's vendored `sd_card.{c,h}` is essentially a thin wrapper over the same APIs. Re-implement using IDF directly. |
| **Mode** | SPI (CS via CH422G EXIO4 — must be asserted by the I/O expander before SD operations) |
| **Critical** | **Watchdog discipline.** Every `f_write()` / `f_sync()` call must be wrapped in disable/re-enable of the task watchdog. Local wrapper `sd_card.{c,h}` will provide `sd_safe_write()` per Workstream 6 logger requirements. |

### RTC PCF85063A

| | |
|---|---|
| **Choice** | **Vendored driver** in `platform/esp32/main/rtc_pcf85063a.{c,h}` |
| **Why** | No maintained Espressif managed component exists. WaveShare's vendored driver is the canonical reference. We rewrite to use our `i2c_bus` + WITH_I2C_MUTEX pattern (WaveShare's version uses legacy I2C without app-level locking). |
| **I²C address** | 0x51 · battery-backed |

### NMEA 2000 / CAN bus (TWAI)

| | |
|---|---|
| **Choice** | ESP-IDF built-in `driver/twai.h` (with future migration to v2 API when stable) |
| **Why** | The standard. Espressif maintains it. Supports listen-only mode (Display variant default) and normal mode (IMU/GPS variant). |
| **Pins** | GPIO 15 TX / GPIO 16 RX · 250 kbps (N2K standard) |
| **PGN parsing** | In `core/` — portable, separate from this firmware-specific transport layer (Workstream 1) |

### RS485 (HWT901B IMU + serial GPS)

| | |
|---|---|
| **Choice** | ESP-IDF built-in `driver/uart.h` with manual DE/RE pin control |
| **Why** | Standard. The on-board RS485 transceiver doesn't need a special library. |
| **Pins** | GPIO 43 RX / GPIO 44 TX · DE/RE controlled if hardware requires (check board datasheet) |

### WiFi / BLE

| | |
|---|---|
| **Choice** | ESP-IDF built-in `esp_wifi.h`, `esp_wifi_netif.h`, etc. |
| **Why** | Standard. No managed component beats it. |
| **Modes** | AP for first-run / config (default), STA when joined to boat network |

### HTTP server / WebSocket

| | |
|---|---|
| **Choice** | ESP-IDF built-in `esp_http_server.h` |
| **Why** | Standard. Supports HTTP + WebSocket out of the box. |
| **Status** | Deferred to web UI phase. |

### OTA

| | |
|---|---|
| **Choice** | ESP-IDF built-in `esp_ota_ops.h` + `app_update.h` |
| **Why** | The standard. Built-in dual-app rollback, partition management. |
| **SD source** | We read `.bin` from SD card and feed to `esp_ota_write()` chunk by chunk. |

### TOML parser (config file)

| | |
|---|---|
| **Choice** | [`tomlc99`](https://github.com/cktan/tomlc99) — vendored as a single source file |
| **Why** | MIT, ~600 LOC, no dependencies, well-tested. No managed component option for TOML on ESP-IDF. |
| **Status** | Deferred to Workstream 4 (config implementation). |

### SHA256 (OTA verification)

| | |
|---|---|
| **Choice** | ESP-IDF built-in `mbedtls` |
| **Why** | Already in IDF. No need to add. |

### JSON (web UI / CLI output)

| | |
|---|---|
| **Choice** | ESP-IDF built-in `cJSON` |
| **Why** | Already in IDF, sufficient for our needs (status payloads, config dumps, CLI `--json` output). |

## Component manifest pattern

Managed components declared per-subdirectory in `idf_component.yml`. We start minimal and add as features land:

```yaml
# platform/esp32/main/idf_component.yml — updated as phases land
dependencies:
  idf:
    version: ">=5.1.0"

  # Phase 2B: I/O expander
  espressif/esp_io_expander_ch422g: "~1.0"

  # Phase 5+: Touch (when graphics return)
  # espressif/esp_lcd_touch_gt911: "^1"

  # Graphics phase: LVGL
  # lvgl/lvgl: "~8.3"
```

## What we explicitly DON'T use

- **Arduino-ESP32 framework** — ESP-IDF only. Arduino has its place but adds an abstraction layer we don't want for a marine safety device.
- **PlatformIO** — direct ESP-IDF + CMake. PIO is fine but adds a layer of build-system indirection.
- **WaveShare's vendored CH422G/SD drivers** — superseded by managed components / built-in IDF drivers as documented above.
- **TouchGFX, embedded GUI alternatives to LVGL** — sticking with LVGL for graphics consistency.
- **NimBLE for BLE** until a feature actually needs BLE (not in v0.2 scope).

## Rule for adding new dependencies

Before declaring a new managed component or vendoring a driver:

1. **Check the [Espressif Component Registry](https://components.espressif.com/) first.** If a maintained component exists for what you need, use it.
2. **If vendoring, document why** in this file (no managed option, or managed option has specific bug we work around).
3. **Pin versions** — never use floating tags. `~1.0` (compatible patches) or `^1` (compatible minor) at minimum.
4. **Update this document** when you add or change a choice.

## Change log

- **0.1.0** (2026-04-26): Initial document. Captures Phase 2A choices and the planned Phase 2B-5+ component set.
