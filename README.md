# Anchor Drag Pro - Marine Safety System

**Author:** Colin Bitterfield
**Email:** colin@bitterfield.com
**Version:** 0.1.0
**Platform:** ESP-IDF
**IDE:** CLion
**Hardware:** Waveshare ESP32-S3-Touch-LCD-4.3B

---

## Overview

Anchor Drag Pro is a marine safety device that monitors boat anchor position via GPS and provides real-time alerts when anchor drag is detected. The system integrates with NMEA 2000 marine networks and features a high-resolution 4.3" touchscreen display.

**Key Features:**
- Real-time GPS position monitoring from NMEA 2000
- Visual anchor swing circle display with trail plotting
- Configurable drag detection threshold
- Direct 12V marine power integration (7-36V input)
- Isolated digital outputs for buzzers and alarms
- Touch-based configuration interface
- Persistent settings storage (NVS flash)
- Optional external GPS/compass support
- Weather-resistant enclosure

---

## Hardware Platform

### Waveshare ESP32-S3-Touch-LCD-4.3B Specifications

**Processor:**
- ESP32-S3-WROOM-1-N16R8
- Dual-core Xtensa LX7 @ 240MHz
- 16MB Flash, 8MB PSRAM

**Display:**
- 4.3" IPS LCD, 800×480 resolution
- 5-point capacitive touch (GT911)
- ST7262 RGB LCD controller
- Adjustable backlight

**Connectivity:**
- NMEA 2000 / CAN bus (onboard transceiver)
- RS485 serial interface
- I2C bus (external devices)
- WiFi + Bluetooth LE

**I/O:**
- 4 isolated digital outputs (5-36V, 450mA sink)
- 2 isolated digital inputs (5-36V)
- SD card slot (SPI)
- Real-time clock with battery backup

**Power:**
- Wide voltage input: 7-36V DC
- Direct connection to NMEA 2000 power bus
- Battery charging circuit (3.7V LiPo)

**Enclosure:**
- Waveshare plastic case with mounting options
- Screw terminal connections (16-pin)
- M12 panel mount connectors for marine installation

**Price:** ~$73 for basic system (see BOM for full build)

For complete specifications, see: `docs/Waveshare_ESP32-S3-Touch-LCD-4.3B_Summary.md`

---

## Quick Start

### Prerequisites

1. **ESP-IDF:** Install ESP-IDF v5.1 or later
   ```bash
   # macOS with Homebrew
   brew install espressif/esp-idf/esp-idf

   # Or follow official guide
   https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/
   ```

2. **CLion:** JetBrains CLion with ESP-IDF plugin
   - Install ESP-IDF plugin from Settings → Plugins
   - Configure ESP-IDF path in Settings → ESP-IDF

3. **USB Drivers:** Install CH343 USB-to-serial drivers for ESP32-S3

### Build and Flash

```bash
# Set up IDF environment
. $IDF_PATH/export.sh

# Configure project (optional - defaults are set)
idf.py menuconfig

# Build firmware
idf.py build

# Flash to board
idf.py -p /dev/ttyUSB0 flash

# Monitor serial output
idf.py -p /dev/ttyUSB0 monitor
```

### CLion Workflow

1. Open project in CLion
2. Select ESP-IDF configuration from toolbar
3. Click "Build" button
4. Click "Flash & Monitor" to deploy and view logs
5. Use CLion's integrated serial monitor for debugging

---

## Project Structure

```
anchor-drag-pro/
├── main/                   # Application source code
│   ├── main.c             # Main application entry point
│   ├── CMakeLists.txt     # Main component build config
│   └── [future modules]   # Display, GPS, UI, CAN drivers
├── components/            # Reusable components (future)
├── docs/                  # Documentation
│   ├── Waveshare_ESP32-S3-Touch-LCD-4.3B_Summary.md
│   ├── ESP32-S3-Touch-LCD-4.3B-BOX-KNOWLEDGE.md
│   ├── BOM_Anchor_Alarm.md
│   ├── Wiring_Guide.md
│   ├── UI_Screens.md
│   ├── Screw_Terminal_Connections.md
│   ├── anchoring_mode_specification.md
│   └── OUTSTANDING_ISSUES.md
├── assets/                # Images, fonts, UI resources
├── backups/               # Backup files (not version controlled)
├── build/                 # Build output (not version controlled)
├── CMakeLists.txt         # Project CMake configuration
├── sdkconfig              # ESP-IDF project configuration
├── sdkconfig.defaults     # Default configuration values
├── README.md              # This file
└── CLAUDE.md              # Claude Code development guide
```

---

## System Architecture

### State Machine

The anchor alarm operates through six states:

```
OFF → ON → ARMING → ARMED → ALARM → MUTED → OFF
     (btn) (btn)   (GPS)   (drag)  (btn)   (btn)
```

**States:**
1. **OFF** - System idle, display shows main menu
2. **ON** - RED LED, system activated, waiting for ARMING
3. **ARMING** - YELLOW LED, collecting GPS points for 60 seconds
4. **ARMED** - GREEN LED, monitoring position against anchor circle
5. **ALARM** - BLINKING RED LED + buzzer, drag detected (>threshold)
6. **MUTED** - BLINKING RED LED only, buzzer silenced

**Anchor Position Calculation:**
- During ARMING, system collects GPS coordinates
- Calculates centroid (anchor position)
- Determines maximum swing radius
- Creates virtual "anchor circle" boundary
- Triggers alarm when boat exits this circle

For detailed algorithm specification, see: `docs/anchoring_mode_specification.md`

### Data Flow

```
NMEA 2000 Network → CAN Transceiver → ESP32-S3 TWAI → GPS Parser
                                                          ↓
                                         Position Data ← PGN 129029
                                                          ↓
                                         Anchor Alarm State Machine
                                                          ↓
                                      ┌──────────────────┴──────────────────┐
                                      ↓                  ↓                   ↓
                              LVGL Display      Isolated Output      Data Logging
                              (800×480)         (Buzzer/Relay)        (SD Card)
```

### GPS Sources (Priority Order)

1. **NMEA 2000** - Primary source via CAN bus (PGN 129029)
2. **I2C GPS Module** - NEO-8M standalone GPS (address 0x42)
3. **RS485 NMEA 0183** - Legacy GPS via serial (4800-38400 baud)

System automatically selects first available source with valid fix.

---

## User Interface

### Screen Navigation

The system features 6 main screens accessible via touch swipe:

```
START ←→ INFO ←→ PGN ←→ CONFIG ←→ UPDATE ←→ TOOLS
```

**Navigation:**
- **Swipe Left/Right:** Move between screens
- **Button Tap:** Select options or change modes
- **ESC/Back Button:** Return to previous screen

**Page Indicators:**
```
▶ ◀ ◀ ◀ ◀ ◀  = START   (Mode selection)
▶ ▶ ◀ ◀ ◀ ◀  = INFO    (GPS details, compass)
▶ ▶ ▶ ◀ ◀ ◀  = PGN     (NMEA 2000 monitor)
▶ ▶ ▶ ▶ ◀ ◀  = CONFIG  (System settings)
▶ ▶ ▶ ▶ ▶ ◀  = UPDATE  (Firmware update)
▶ ▶ ▶ ▶ ▶ ▶  = TOOLS   (Diagnostics)
```

### Main Screens

1. **SPLASH** - Boot screen with logo (30 seconds)
2. **START** - Mode selection (OFF/READY/CONFIG)
3. **INFO** - Compass rose and detailed GPS status
4. **DISPLAY (Ready to Anchor)** - Main monitoring before anchor set
5. **DISPLAY (Anchoring)** - GPS plotting and anchor tracking
6. **PGN** - NMEA 2000 message monitor
7. **CONFIG** - System configuration (saved to NVS)
8. **UPDATE** - Firmware update interface
9. **TOOLS** - System utilities and diagnostics
10. **TEST** - Hardware testing (accessed from TOOLS)

For complete UI specification with mockups, see: `docs/UI_Screens.md`

---

## Configuration

### System Settings (NVS Storage)

**Anchor Alarm:**
- Distance alarm threshold: 25-250 feet (default: 50)
- Arming time: 30-300 seconds (default: 60)
- GPS timeout: 30-600 seconds (default: 60)

**GPS Sources:**
- Primary source: NMEA 2000 / I2C / RS485
- Backup source: Enabled/Disabled
- Update rate: 0.5-10 Hz

**Display:**
- Brightness: 0-100%
- Screen timeout: 10-600 seconds
- Color scheme: Day/Night/Auto

**I/O:**
- Buzzer volume: 0-100%
- Isolated output mode: Buzzer/Relay/Disabled
- Button debounce: 1-10 seconds

All settings persist across power cycles via ESP32 NVS (Non-Volatile Storage).

---

## Hardware Connections

### NMEA 2000 Connection (Recommended)

**M12 5-Pin N2K Connector:**
| Pin | N2K Wire | Function | Terminal |
|-----|----------|----------|----------|
| 1 | Red | +12V | VIN (1) |
| 2 | Black | GND | GND (2) |
| 3 | White | CAN_H | CAN-H (8) |
| 4 | Blue | CAN_L | CAN-L (7) |
| 5 | Shield | Drain | GND (2) |

**Termination:** Enable 120Ω terminator switch if device is at end of backbone.

### External GPS (Optional)

**NEO-8M GPS Module via I2C:**
| GPS Wire | Function | Terminal |
|----------|----------|----------|
| Red | VCC (5V) | VOUT (3) |
| Black | GND | GND (4) |
| White | SDA | SDA (5) |
| Orange | SCL | SCL (6) |

**Note:** Set VOUT to 5V using onboard resistor jumper.

### Buzzer/Alarm Connection

**12V Piezo Buzzer (Direct Drive):**
| Buzzer Wire | Connection | Terminal |
|-------------|------------|----------|
| Red (+) | N2K +12V | VIN (1) |
| Black (-) | Isolated Output | DO0 (11) |

**Note:** Isolated outputs sink up to 450mA at 5-36V. No external relay needed.

For complete wiring diagrams, see: `docs/Wiring_Guide.md`

---

## Bill of Materials

### Basic System ($73)

| Item | Part | Price |
|------|------|-------|
| ESP32-S3-Touch-LCD-4.3B | Amazon B0DCNSRT31 | $45 |
| Waveshare Plastic Case | Amazon B0DD6VLH1Y | $8 |
| M12 5-Pin N2K Connector | Amazon B0FJY18H5N | $10 |
| M12 12-Pin Aux Connector | Amazon B0FLDB4FQF | $10 |

### Optional Components

| Item | Part | Price |
|------|------|-------|
| 12V Piezo Buzzer IP67 | Amazon B0D7W5BYL5 | $12 |
| Waterproof Push Button | TBD | $8 |
| NEO-8M GPS + Compass | Amazon B0FLV6RTY1 | $18 |
| 32GB MicroSD Card | Various | $8 |

**Full Build:** $119 (includes all optional components)

For detailed BOM with links, see: `docs/BOM_Anchor_Alarm.md`

---

## Development Roadmap

### Phase 1: Hardware Bring-Up ✅ Current Phase
- [x] ESP-IDF project structure
- [ ] Board pin configuration
- [ ] Display driver initialization
- [ ] Touch controller integration
- [ ] Basic LVGL "Hello World"

### Phase 2: Core UI
- [ ] LVGL screen framework
- [ ] Splash screen and main display
- [ ] Screen navigation (swipe gestures)
- [ ] Configuration screen with NVS storage

### Phase 3: GPS Integration
- [ ] NMEA 2000 CAN bus driver
- [ ] PGN 129029 GPS message parsing
- [ ] Position display on INFO screen
- [ ] Compass rose visualization

### Phase 4: Anchor Alarm Logic
- [ ] State machine implementation
- [ ] Anchor position calculation (centroid algorithm)
- [ ] Drag detection logic
- [ ] Buzzer and alarm activation

### Phase 5: Advanced Features
- [ ] External GPS/compass module support
- [ ] SD card logging
- [ ] WiFi/Bluetooth connectivity
- [ ] OTA firmware updates

For detailed task tracking, see: `docs/OUTSTANDING_ISSUES.md`

---

## Testing

### Hardware Testing Checklist

**Display:**
- [ ] Backlight illuminates at 50%, 100%
- [ ] All pixels display correctly (use test pattern)
- [ ] Touch responds to 5 simultaneous points
- [ ] Brightness adjustment via settings

**Communication:**
- [ ] CAN bus receives PGN 129029 from chartplotter
- [ ] GPS coordinates parse correctly
- [ ] I2C GPS module detected and providing fix
- [ ] RS485 receives NMEA 0183 sentences

**I/O:**
- [ ] Buzzer sounds on isolated output
- [ ] Isolated inputs detect 12V signals
- [ ] SD card mounts and writes log file
- [ ] RTC maintains time during power cycle

**Functional:**
- [ ] State machine transitions correctly
- [ ] Anchor circle calculation accurate
- [ ] Alarm triggers at configured distance
- [ ] Settings persist after power cycle

---

## Troubleshooting

### Build Errors

**"esp_lcd_rgb_panel.h not found"**
```bash
# Update ESP-IDF to v5.1+
cd $IDF_PATH
git pull
git submodule update --init --recursive
```

**"GT911 driver missing"**
```bash
# Add ESP32_Display_Panel component
cd components
git clone https://github.com/esp-arduino-libs/ESP32_Display_Panel.git
```

### Runtime Issues

**Display shows nothing:**
1. Check I2C bus initialization (GPIO8/9)
2. Verify CH422G I/O expander responds (address 0x24)
3. Confirm backlight enable (EXIO2) is HIGH
4. Check LCD reset (EXIO3) is HIGH

**Touch not responding:**
1. Verify GT911 I2C address (0x5D on I2C1: GPIO15/7)
2. Check interrupt line (GPIO4) configured correctly
3. Reset touch controller via CH422G EXIO1

**No GPS data:**
1. Check CAN bus termination (120Ω if at end of backbone)
2. Verify CAN_H/CAN_L not swapped
3. Monitor CAN bus with logic analyzer (250 kbps for N2K)
4. Confirm chartplotter is broadcasting PGN 129029

**Buzzer not sounding:**
1. Check isolated output configuration
2. Verify buzzer polarity (+ to VIN, - to DO0)
3. Confirm 12V power on VIN terminal
4. Test with multimeter (should show 12V when activated)

---

## Contributing

This is a personal project by Colin Bitterfield. Suggestions and bug reports welcome at colin@bitterfield.com.

---

## License

See LICENSE file in project root.

---

## Resources

### Documentation
- [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/)
- [LVGL Documentation](https://docs.lvgl.io/)
- [Waveshare Wiki](https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-4.3B)
- [NMEA 2000 Standard](https://www.nmea.org/nmea-2000.html)

### Hardware
- [Waveshare Demo Code](https://github.com/waveshareteam/ESP32-S3-Touch-LCD-4.3B)
- [ESP32-S3 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf)

### Marine Standards
- [BoatUS Anchoring Guide](https://www.boatus.com/expert-advice/expert-advice-archive/2019/may/anchoring-basics)
- [NMEA 2000 PGN Reference](docs/garmin_nmea2000_pgn_reference.md)

---

## Version History

- **0.1.0** (2025-12-19) - Initial ESP-IDF project setup
  - Migrated from Arduino/Wio Terminal to ESP-IDF
  - Basic FreeRTOS application
  - Documentation refactored for ESP32-S3 platform
  - Hardware specifications documented

---

**Current Status:** ESP-IDF migration in progress - hardware configuration next

For the latest development status, see: `docs/OUTSTANDING_ISSUES.md`
