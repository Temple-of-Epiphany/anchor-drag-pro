# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Anchor Drag Pro is a marine safety device for the Waveshare ESP32-S3-Touch-LCD-4.3B that monitors boat anchor position via GPS and alerts when anchor drag is detected. The project is built using ESP-IDF and developed in CLion IDE.

**Platform:** ESP-IDF (Espressif IoT Development Framework)
**IDE:** JetBrains CLion with ESP-IDF plugin
**Hardware:** Waveshare ESP32-S3-Touch-LCD-4.3B
**Display:** 4.3" 800×480 IPS touchscreen
**MCU:** ESP32-S3 (Dual-core Xtensa LX7 @ 240MHz, 16MB Flash, 8MB PSRAM)

## Architecture

### Component Overview

1. **ESP-IDF Firmware** (`main/`) - Production code for ESP32-S3 hardware
   - `main.c` - Main application with 6-screen navigation system
   - `CMakeLists.txt` - Component build configuration
   - Implemented: Display, Touch, RTC, Power Management, SD Card
   - In Progress: CAN/NMEA 2000, GPS parsing

2. **Hardware Drivers** (`main/`)
   - `display_driver.c` - ST7262 RGB LCD driver (800×480, Mode 3 direct rendering)
   - `touch_driver.c` - GT911 capacitive touch controller (I2C, 5-point)
   - `ch422g.c` - CH422G I/O expander (backlight, reset, SD CS control)
   - `sd_card.c` - SD card via SPI with FAT filesystem support
   - `power_management.c` - Deep sleep and wake-up management
   - `rtc_pcf85063a.c` - Real-time clock with battery backup
   - Future: CAN/TWAI driver for NMEA 2000

3. **UI Framework** (LVGL 9.2.0)
   - `screens.c` - All 6 navigation screens (START, INFO, PGN, CONFIG, UPDATE, TOOLS)
   - `ui_header.c` - Header with RTC time, GPS status, battery indicator
   - `ui_footer.c` - Swipeable footer navigation bar
   - `ui_theme.h` - Centralized Marine Blue color theme
   - `datetime_settings.c` - RTC configuration screen
   - Custom fonts: Orbitron, Poppins, GolosText, SF Rounded, Apple Symbols (23 fonts)
   - Gesture support: Swipe left/right (navigate), swipe up (show footer)

### State Machine Flow

The anchor alarm follows this state sequence:

```
OFF → ON → ARMING → ARMED → ALARM → MUTED → OFF
     (btn) (btn)   (GPS)   (drag)  (btn)   (btn)
```

**States:**
- `OFF` - System idle, no monitoring
- `ON` - RED LED/indicator, system activated
- `ARMING` - YELLOW LED, collecting GPS points for 60 seconds to establish anchor position
- `ARMED` - GREEN LED, monitoring GPS against calculated anchor circle
- `ALARM` - BLINKING RED LED + buzzer, anchor drag detected
- `MUTED` - BLINKING RED LED only, sound silenced

**Key Behaviors:**
- Button presses have debounce protection
- ARMING collects GPS points to calculate anchor swing circle (centroid calculation)
- ARMED monitors if current GPS exceeds distance threshold from anchor center
- Distance calculation uses equirectangular approximation (accurate for small distances)

### Data Flow

```
NMEA 2000 → CAN Transceiver → ESP32 TWAI → PGN Parser → GPS Data
                                                             ↓
                                            Anchor Alarm State Machine
                                                             ↓
                                    ┌────────────────────────┴────────────────┐
                                    ↓                        ↓                ↓
                            LVGL Display              Buzzer Driver     NVS Storage
                            (800×480)                 (Isolated I/O)    (Settings)
```

## Development Commands

### ESP-IDF Build System

```bash
# Set up environment (run in each terminal session)
. $IDF_PATH/export.sh

# Configure project (opens menuconfig)
idf.py menuconfig

# Build firmware
idf.py build

# Flash to device
idf.py -p /dev/ttyUSB0 flash

# Monitor serial output
idf.py -p /dev/ttyUSB0 monitor

# Flash and monitor in one command
idf.py -p /dev/ttyUSB0 flash monitor

# Clean build artifacts
idf.py fullclean

# Set build target (ESP32-S3)
idf.py set-target esp32s3
```

### CLion Workflow

1. Open project in CLion
2. Ensure ESP-IDF plugin is installed and configured
3. Select "flash" or "app-flash" build target from dropdown
4. Click build/run button or use keyboard shortcut
5. Use "monitor" target to view serial output

### Pin Configuration

**Critical for Waveshare ESP32-S3-Touch-LCD-4.3B:**

See `docs/ESP32-S3-Touch-LCD-4.3B-BOX-KNOWLEDGE.md` for complete pin mapping.

**Key Peripherals:**
- **I2C0** (Shared by ALL devices): GPIO8 (SDA), GPIO9 (SCL) @ 400kHz
  - CH422G I/O Expander: 0x24 (read) / 0x38 (write)
  - GT911 Touch Controller: 0x5D
  - PCF85063A RTC: 0x51
  - GPS (optional): 0x42
- **CAN/TWAI**: GPIO15 (TX), GPIO16 (RX) - NMEA 2000 @ 250kbps
- **Touch Interrupt**: GPIO4 - GT911 touch IRQ (active low)
- **SD Card SPI**: GPIO11 (MOSI), GPIO12 (SCK), GPIO13 (MISO), CS via CH422G EXIO4
- **RGB LCD**: 16-bit parallel interface (see knowledge doc for full pin list)
- **RS485**: GPIO43 (RX), GPIO44 (TX) - NMEA 0183

**I/O Expander (CH422G at 0x24):**
- EXIO0: Reserved
- EXIO1: Touch reset
- EXIO2: LCD backlight
- EXIO3: LCD reset
- EXIO4: SD card chip select
- EXIO5: USB selection

## File Organization

```
/
├── main/                  # Main application
│   ├── main.c            # Entry point, app_main()
│   └── CMakeLists.txt    # Component build
├── components/           # Custom components (future)
├── docs/                 # Documentation
│   ├── ESP32_S3_HARDWARE_CONFIG.md  # **Critical** hardware config guide
│   ├── Adding_Custom_Fonts.md
│   ├── Font_Converter_Setup.md
│   ├── convert_image_to_lvgl_specification.md
│   ├── Waveshare_ESP32-S3-Touch-LCD-4.3B_Summary.md
│   ├── ESP32-S3-Touch-LCD-4.3B-BOX-KNOWLEDGE.md
│   ├── BOM_Anchor_Alarm.md
│   ├── Wiring_Guide.md
│   ├── UI_Screens.md
│   ├── Screw_Terminal_Connections.md
│   ├── anchoring_mode_specification.md
│   ├── nmea2000_pgn_anchor_alarm_table.md
│   ├── garmin_nmea2000_pgn_reference.md
│   ├── wind_drift_ui_logic.md
│   ├── LVGL_Simulator_Guide.md
│   └── OUTSTANDING_ISSUES.md
├── assets/               # UI resources (images, fonts)
├── backups/              # Backup files
├── build/                # Build output (gitignored)
├── CMakeLists.txt        # Project CMake
├── sdkconfig             # ESP-IDF configuration
├── sdkconfig.defaults    # Default config values
└── README.md             # Project overview
```

## Important Implementation Details

### GPS Distance Calculation

Uses equirectangular approximation for distance calculation:
- Accurate for small distances (boat swing radius)
- Converts lat/lon degrees to feet using `FEET_PER_DEGREE_LAT = 364000.0`
- Adjusts longitude for latitude: `cos(avg_lat)`
- Returns Euclidean distance: `sqrt(lat_diff² + lon_diff²)`

### Anchor Position Calculation

During ARMING state:
1. Collect GPS points for configured time (default 60s)
2. Calculate centroid (average of all points)
3. Find maximum distance from centroid (swing radius)
4. Store as anchor circle with center and radius
5. In ARMED state, compare current GPS to center
6. Trigger ALARM if distance > configured threshold

### Display Initialization Sequence

**Critical for ST7262 RGB LCD:**

1. Initialize I2C0 bus (GPIO8/9) at 400kHz
2. Initialize CH422G I/O expander (address 0x24)
3. Set EXIO2 HIGH (LCD backlight ON)
4. Set EXIO3 HIGH (LCD reset released)
5. Create RGB bus with 16-bit data pins
6. Create LCD_ST7262 instance
7. Configure bounce buffer (800×10 pixels minimum)
8. Call lcd->init()
9. Call lcd->reset()
10. Call lcd->begin()
11. Call lcd->setDisplayOnOff(true)

**Touch Controller (GT911):**
1. Initialize on I2C1 (GPIO15 SDA, GPIO7 SCL)
2. Address: 0x5D
3. Configure interrupt on GPIO4
4. Reset via CH422G EXIO1 pulse

### NMEA 2000 Integration

**CAN Bus Configuration:**
- Speed: 250 kbps (NMEA 2000 standard)
- TX: GPIO15
- RX: GPIO16
- Termination: 120Ω via onboard switch (enable if at network end)

**Primary PGN Messages:**
- PGN 129029: GNSS Position Data (GPS coordinates, fix quality)
- PGN 130306: Wind Data (future: wind drift compensation)
- PGN 128267: Depth (future: depth display)

See `docs/garmin_nmea2000_pgn_reference.md` for complete PGN listing.

## Common Development Tasks

### Adding Display Support

1. Add ESP32_Display_Panel component to `components/`
2. Add ESP32_IO_Expander component (dependency)
3. Create display driver module in `main/display_driver.c`
4. Initialize I2C0, CH422G, RGB bus, LCD
5. Implement frame buffer management
6. Add to main.c initialization

### Adding LVGL GUI

1. Add LVGL component (v9.2.0) to `components/`
2. Configure `lv_conf.h` for ESP32-S3 (800×480, RGB565)
3. Implement display flush callback (RGB interface)
4. Implement touch input driver (GT911)
5. Create screen modules per `docs/UI_Screens.md` specification
6. Initialize LVGL in main.c, start LVGL timer task

### Adding CAN/NMEA 2000 Support

1. Configure TWAI driver (GPIO15/16, 250 kbps)
2. Implement PGN parser (start with PGN 129029)
3. Create GPS data structure and callbacks
4. Add state machine integration
5. Test with real NMEA 2000 network

### Modifying Anchor Algorithm

Edit the calculation logic:
- Centroid calculation during ARMING
- Distance threshold comparison during ARMED
- State transitions based on GPS fix quality

Keep Arduino and ESP-IDF implementations synchronized if both exist.

## Configuration Storage (NVS)

**Persistent Settings:**
- Distance alarm threshold (feet): 25-250, default 50
- Arming time (seconds): 30-300, default 60
- GPS source priority: N2K / I2C / RS485
- Display brightness: 0-100%
- Buzzer volume: 0-100%

**NVS Namespace:** "anchor_alarm"

**Implementation:**
```c
nvs_handle_t nvs_handle;
nvs_open("anchor_alarm", NVS_READWRITE, &nvs_handle);
nvs_get_u32(nvs_handle, "alarm_distance", &distance);
nvs_set_u32(nvs_handle, "alarm_distance", distance);
nvs_commit(nvs_handle);
nvs_close(nvs_handle);
```

## Testing

### Hardware Testing Checklist

1. **Display:**
   - Backlight illuminates
   - All pixels render correctly
   - Colors accurate (use test pattern)
   - Touch responds to all 5 points

2. **Communication:**
   - CAN bus receives PGN 129029
   - GPS coordinates parse correctly
   - I2C devices respond (GT911, CH422G, RTC)
   - RS485 receives NMEA 0183 (if configured)

3. **I/O:**
   - Buzzer sounds on isolated output
   - Isolated inputs detect 5-36V signals
   - SD card mounts and reads/writes files
   - RTC maintains time during power cycle

4. **Functional:**
   - State machine transitions correctly
   - Anchor circle calculation accurate
   - Alarm triggers at configured distance
   - Settings persist after power cycle

## Deployment

### Building Firmware

```bash
# Clean build
idf.py fullclean
idf.py build

# Output: build/anchor-drag-pro.bin
```

### Flashing to Device

**Method 1: USB Serial (Development)**
```bash
idf.py -p /dev/ttyUSB0 flash
```

**Method 2: OTA Update (Production)**
```bash
# Requires OTA partition configuration
# Upload via WiFi to running device
```

**Method 3: SD Card (Field Update)**
```bash
# Copy firmware.bin to SD card
# Boot device, select UPDATE screen
# Select firmware file and confirm
```

## GitHub Issue Labels

**Component Labels:**

- **`hardware`** (Purple #a020f0) - Hardware configuration, pins, peripherals
  - Display driver issues
  - Touch controller problems
  - CAN bus configuration
  - I2C device integration
  - GPIO assignments

- **`ui`** (Blue #1d76db) - User interface and display
  - LVGL screen layouts
  - Touch input handling
  - Screen navigation
  - Graphics rendering
  - Web UI issues (if applicable)

- **`gps`** (Green #0e8a16) - GPS and position tracking
  - NMEA 2000/0183 parsing
  - Position calculation
  - Anchor circle algorithm
  - Fix quality assessment

- **`firmware`** (Red #d73a4a) - Core firmware functionality
  - State machine logic
  - Alarm triggering
  - NVS storage
  - FreeRTOS tasks

- **`documentation`** (Yellow #fbca04) - Documentation updates
  - README changes
  - Code comments
  - Wiring guides
  - Specifications

### Label Usage

```bash
# Create issue with component label
gh issue create --title "Display backlight not working" \
  --body-file /tmp/issue.md \
  --label "hardware,bug"

# Add label to existing issue
gh issue edit 123 --add-label "ui"

# List issues by component
gh issue list --label "gps"
```

## Version Management

**Versioning Scheme:** MAJOR.MINOR.PATCH

- **MAJOR:** Breaking changes or major feature additions
- **MINOR:** New features, backward compatible
- **PATCH:** Bug fixes, documentation updates

**Version Locations:**
- `README.md` header
- `main/main.c` header comment
- Git tags (e.g., `v0.1.0`)

**Update Process:**
1. Increment version in all files
2. Update CHANGELOG section in README.md
3. Create git tag: `git tag -a v0.1.0 -m "Initial ESP-IDF release"`
4. Push with tags: `git push --tags`

## Hardware Configuration

**📘 Complete Hardware Setup Guide:** `docs/ESP32_S3_HARDWARE_CONFIG.md`

### Critical Configuration Highlights

**I2C Architecture:**
- **ALL I2C devices share ONE bus:** I2C0 on GPIO 8/9 at 400kHz
  - CH422G I/O Expander: 0x24 (read) / 0x38 (write)
  - GT911 Touch Controller: 0x5D
  - PCF85063A RTC: 0x51

**CH422G I/O Expander Pin Functions:**
```
EXIO1 = Touch Reset (Active LOW)
EXIO2 = LCD Backlight (Active HIGH)
EXIO3 = LCD Reset (Active HIGH)
EXIO4 = SD Card CS (Active LOW)
EXIO5 = USB/CAN Select (HIGH = CAN mode)
```

**SD Card Critical Requirements:**
- ⚠️ **MUST disable watchdog before operations** (30+ second format can trigger timeout)
- ⚠️ **MUST re-enable watchdog on ALL return paths** (success AND errors)
- CS controlled via CH422G EXIO4, NOT direct GPIO

**LVGL Display (Waveshare Mode 3):**
- Must set `disp_drv.direct_mode = 1`
- Wait for VSYNC in flush callback to prevent tearing
- Task stack: 10KB minimum (increased for SD operations)

## Important Notes

- **No Simulators:** This project no longer uses Python simulators. All development is on ESP32-S3 hardware.
- **UI Reference:** Always check `docs/UI_Screens.md` for current screen specifications before implementing UI changes.
- **I2C Bus:** ONE shared I2C0 bus for all peripherals (GPIO 8/9) - no separate buses
- **CAN Termination:** Only enable 120Ω termination if device is at end of NMEA 2000 backbone.
- **Power Requirements:** Device supports 7-36V input, ideal for 12V/24V marine systems.
- **Flash Partitions:** Ensure OTA partition is configured if implementing firmware updates.

## Quick Reference

### Build and Flash
```bash
idf.py build && idf.py -p /dev/ttyUSB0 flash monitor
```

### View Logs
```bash
idf.py -p /dev/ttyUSB0 monitor
```

### Clean Build
```bash
idf.py fullclean && idf.py build
```

### Configure
```bash
idf.py menuconfig
```

## Resources

- **ESP-IDF Documentation:** https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/
- **Waveshare Wiki:** https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-4.3B
- **Waveshare Demo Code:** https://github.com/waveshareteam/ESP32-S3-Touch-LCD-4.3B
- **LVGL Documentation:** https://docs.lvgl.io/
- **NMEA 2000 Standard:** https://www.nmea.org/nmea-2000.html

---

**Last Updated:** 2025-12-26
**Project Status:** Core firmware functional - CAN/NMEA 2000 integration next
**Current Version:** 0.1.1

## Recent Changes (v0.1.1)

- ✅ All 6 navigation screens implemented with swipeable footer
- ✅ RTC integration with real-time display updates
- ✅ Power management with deep sleep support
- ✅ SD card filesystem support (SPI mode)
- ✅ Custom fonts (23 fonts, 5 families)
- ✅ CH422G I/O expander driver
- ✅ Test pattern generation and display validation
- ⚠️ **Known Issue:** SD card using direct I2C instead of ESP_IO_Expander (causes occasional reboots)
- 🔜 **Next:** Migrate CH422G/SD to use ESP_IO_Expander library properly
- 🔜 **Next:** CAN/TWAI driver for NMEA 2000 integration
