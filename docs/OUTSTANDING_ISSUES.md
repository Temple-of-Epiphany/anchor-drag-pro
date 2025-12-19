# Outstanding Issues - Anchor Drag Alarm System (ESP-IDF)

**Date:** 2025-12-19
**Version:** 0.1.0
**Platform:** ESP-IDF with CLion IDE
**Hardware:** Waveshare ESP32-S3-Touch-LCD-4.3B

---

## Project Status

The project has been migrated from Arduino/Wio Terminal to ESP-IDF for the Waveshare ESP32-S3-Touch-LCD-4.3B platform. The current codebase is a minimal ESP-IDF application that outputs "Anchor Drag Pro" to serial every 30 seconds.

---

## ✅ Completed

### Project Setup
1. **ESP-IDF Project Structure** - CREATED
   - CMakeLists.txt configured for ESP-IDF
   - Basic main.c with FreeRTOS task
   - Project compiles and runs on ESP32-S3
   - Author blocks and version headers added

2. **Documentation Migration** - IN PROGRESS
   - Backed up old README.md and simulator docs
   - Hardware specification docs preserved
   - Creating new ESP-IDF focused documentation

---

## 🔧 Outstanding Issues

### High Priority - Core Functionality

1. **Hardware Configuration**
   - **Status:** NOT STARTED
   - **Task:** Configure board-specific pins and peripherals
   - **Components:**
     - I2C buses (I2C0: GPIO8/9 external, I2C1: GPIO15/7 touch)
     - RGB LCD interface (16-bit parallel, ST7262 driver)
     - Touch controller (GT911, I2C address 0x5D)
     - CAN bus (TWAI driver, GPIO15/16)
     - CH422G I/O expander
     - SD card SPI
   - **Reference:** `docs/ESP32-S3-Touch-LCD-4.3B-BOX-KNOWLEDGE.md`
   - **Estimated Effort:** 2-3 hours

2. **Display Driver Integration**
   - **Status:** NOT STARTED
   - **Task:** Initialize RGB LCD and backlight control
   - **Components:**
     - RGB parallel interface configuration
     - ST7262 LCD driver initialization
     - CH422G I/O expander for backlight and reset
     - Bounce buffer configuration (800×10 pixels)
   - **Libraries Required:**
     - ESP32_Display_Panel
     - ESP32_IO_Expander
   - **Reference:** `docs/ESP32-S3-Touch-LCD-4.3B-BOX-KNOWLEDGE.md:166-179`
   - **Estimated Effort:** 4-6 hours

3. **Touch Input Integration**
   - **Status:** NOT STARTED
   - **Task:** Configure GT911 capacitive touch controller
   - **Components:**
     - I2C1 initialization (GPIO15 SDA, GPIO7 SCL)
     - GT911 driver integration (address 0x5D)
     - Interrupt handling (GPIO4)
     - Touch calibration
   - **Estimated Effort:** 2-3 hours

4. **LVGL GUI Framework**
   - **Status:** NOT STARTED
   - **Task:** Integrate LVGL 9.2.0 for UI rendering
   - **Components:**
     - LVGL initialization and configuration
     - Display flush callback for RGB interface
     - Touch input driver
     - Frame buffer management
   - **Reference:** `docs/UI_Screens.md` for screen designs
   - **Estimated Effort:** 6-8 hours

### Medium Priority - Communication

5. **CAN Bus (NMEA 2000) Integration**
   - **Status:** NOT STARTED
   - **Task:** Implement TWAI driver for NMEA 2000
   - **Components:**
     - TWAI (CAN) driver initialization (GPIO15 TX, GPIO16 RX)
     - NMEA 2000 protocol stack
     - PGN message parsing (PGN 129029 for GPS)
     - 250 kbps CAN speed for N2K
   - **Reference:** `docs/garmin_nmea2000_pgn_reference.md`
   - **Estimated Effort:** 8-10 hours

6. **GPS Data Processing**
   - **Status:** NOT STARTED
   - **Task:** Parse and process GPS position data
   - **Sources:**
     - NMEA 2000 via CAN bus (primary)
     - I2C GPS module (NEO-8M, address 0x42)
     - RS485 NMEA 0183 (fallback)
   - **Components:**
     - GPS coordinate parsing
     - Fix quality assessment
     - Position update callbacks
   - **Estimated Effort:** 4-6 hours

7. **I2C External Devices**
   - **Status:** NOT STARTED
   - **Task:** Support external GPS and compass modules
   - **Components:**
     - I2C0 bus initialization (GPIO8/9)
     - NEO-8M GPS driver (address 0x42)
     - Compass driver (auto-detect chip)
   - **Reference:** `docs/Wiring_Guide.md`
   - **Estimated Effort:** 3-4 hours

### Medium Priority - System Features

8. **Configuration Storage (NVS)**
   - **Status:** NOT STARTED
   - **Task:** Implement persistent configuration storage
   - **Settings:**
     - Distance alarm threshold (feet)
     - Arming time (seconds)
     - GPS source selection
     - Calibration data
   - **Components:**
     - NVS partition initialization
     - Config read/write functions
     - Default value handling
   - **Reference:** `docs/UI_Screens.md:9` (mentions NVS storage)
   - **Estimated Effort:** 2-3 hours

9. **RTC and Timekeeping**
   - **Status:** NOT STARTED
   - **Task:** Initialize onboard RTC with battery backup
   - **Components:**
     - PCF85063 RTC driver (I2C address 0x51)
     - Time synchronization from GPS
     - Battery backup configuration
   - **Estimated Effort:** 2-3 hours

10. **SD Card File System**
    - **Status:** NOT STARTED
    - **Task:** Mount SD card for logging and config
    - **Components:**
      - SPI bus for SD card (GPIO11/12/13)
      - CH422G EXIO4 for chip select
      - FatFS file system
      - Data logging
    - **Estimated Effort:** 3-4 hours

### Low Priority - Advanced Features

11. **Isolated Digital I/O**
    - **Status:** NOT STARTED
    - **Task:** Configure isolated outputs for buzzer and relays
    - **Components:**
      - CH422G I/O expander digital outputs
      - Buzzer drive logic (12V, 450mA sink)
      - Input monitoring (5-36V range)
    - **Reference:** `docs/Waveshare_ESP32-S3-Touch-LCD-4.3B_Summary.md:87-94`
    - **Estimated Effort:** 2-3 hours

12. **WiFi/Bluetooth Connectivity**
    - **Status:** NOT STARTED
    - **Task:** Enable wireless connectivity for remote monitoring
    - **Components:**
      - WiFi station mode
      - Bluetooth LE advertising
      - Web server for remote UI
      - Mobile app integration (future)
    - **Estimated Effort:** 6-8 hours

13. **Firmware Update (OTA)**
    - **Status:** NOT STARTED
    - **Task:** Implement over-the-air firmware updates
    - **Components:**
      - OTA partition configuration
      - HTTP/HTTPS update client
      - Update verification and rollback
      - UI screen for updates
    - **Reference:** `docs/UI_Screens.md` (UPDATE screen)
    - **Estimated Effort:** 4-6 hours

---

## 🎯 Implementation Roadmap

### Phase 1: Hardware Bring-Up (Week 1)
1. Configure board pins and I2C buses
2. Initialize display driver and backlight
3. Integrate touch controller
4. Basic LVGL "Hello World" display

### Phase 2: Core UI (Week 2-3)
1. Implement LVGL screen framework
2. Create splash screen and main display
3. Implement screen navigation (swipe left/right)
4. Configuration screen with NVS storage

### Phase 3: GPS Integration (Week 4)
1. NMEA 2000 CAN bus driver
2. Parse PGN 129029 GPS messages
3. Display position on INFO screen
4. Compass rose visualization

### Phase 4: Anchor Alarm Logic (Week 5-6)
1. Implement state machine (OFF/ON/ARMING/ARMED/ALARM/MUTED)
2. Anchor position calculation (centroid algorithm)
3. Drag detection logic
4. Buzzer and alarm activation

### Phase 5: Advanced Features (Week 7+)
1. External GPS/compass module support
2. SD card logging
3. WiFi/Bluetooth connectivity
4. OTA firmware updates

---

## 📊 Current System Status

### Working Features
- ✅ Basic ESP-IDF project compiles and runs
- ✅ Serial output functioning
- ✅ FreeRTOS task scheduling working

### Not Yet Implemented
- ❌ Display driver
- ❌ Touch input
- ❌ LVGL GUI
- ❌ CAN bus / NMEA 2000
- ❌ GPS parsing
- ❌ Anchor alarm logic
- ❌ All UI screens
- ❌ Configuration storage
- ❌ Isolated I/O
- ❌ External sensors

---

## 🐛 Known Limitations

1. **No Hardware Testing:** Code not yet tested on actual Waveshare board
2. **No Library Integration:** Required libraries (ESP32_Display_Panel, LVGL) not yet added
3. **No Pin Configuration:** GPIO pins not yet configured in code
4. **Basic Functionality Only:** Current code is minimal placeholder

---

## 📝 Development Notes

- Development environment: ESP-IDF in CLion
- Target board: Waveshare ESP32-S3-Touch-LCD-4.3B
- Display resolution: 800×480 pixels
- LVGL version: 9.2.0
- All simulators removed from project scope
- Focus on hardware implementation only

---

## 📚 Reference Documentation

- `docs/Waveshare_ESP32-S3-Touch-LCD-4.3B_Summary.md` - Board specifications
- `docs/ESP32-S3-Touch-LCD-4.3B-BOX-KNOWLEDGE.md` - Pin configuration and drivers
- `docs/BOM_Anchor_Alarm.md` - Bill of materials
- `docs/Wiring_Guide.md` - Connection diagrams
- `docs/UI_Screens.md` - Screen layouts and navigation
- `docs/anchoring_mode_specification.md` - Anchor alarm logic specification

---

**Last Updated:** 2025-12-19
**Status:** ESP-IDF migration in progress - hardware configuration next
