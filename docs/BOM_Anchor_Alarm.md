# Anchor Alarm Project - Bill of Materials

## Version 1.8.0 - December 13, 2025

**Author:** Colin Bitterfield  
**Email:** colin@bitterfield.com  
**Project:** ANCHOR-ALARM

**Changelog:**
- v1.8.0 (2025-12-13): Consolidated from duplicate BOM.md; fixed connector pricing to $10 each
- v1.7.0 (2025-12-13): Clarified M12 5-pin as N2K compatible; split harness into N2K (5-pin) + Aux (12-pin) connectors
- v1.6.0 (2025-12-13): Added M12 12-pin harness connector for internal wiring
- v1.5.0 (2025-12-12): Restructured to Basic + Options; added MC8 3-pin bulkhead for 0183, button, TF card
- v1.4.0 (2025-12-11): Simplified to MVP (ESP32 + connector only), all else optional
- v1.3.0 (2025-12-11): Updated N2K connector to 5-pack M12 panel mount
- v1.2.0 (2025-12-11): Moved buzzer to optional category
- v1.1.0 (2025-12-11): Added GPS/Compass module to optional category
- v1.0.0 (2025-12-11): Initial BOM

---

## Basic System

The Basic System provides complete N2K integration with auxiliary I/O capability.

| Item | Description | Source | Part Number | Qty | Unit Price | Total |
|------|-------------|--------|-------------|-----|------------|-------|
| B1 | Waveshare ESP32-S3-Touch-LCD-4.3B | Amazon | B0DCNSRT31 | 1 | $45.00 | $45.00 |
| B2 | Waveshare 4.3B Plastic Case | Amazon | B0DD6VLH1Y | 1 | $8.00 | $8.00 |
| B3 | M12 5-Pin Panel Mount Connector (N2K/CAN) | Amazon | B0FJY18H5N | 1 | $10.00 | $10.00 |
| B4 | M12 12-Pin Panel Mount Connector (Aux I/O) | Amazon | B0FLDB4FQF | 1 | $10.00 | $10.00 |

### **Basic System Total: $73.00**

---

## Basic System Capabilities

With the Basic System ($73), you get:

1. **4.3" Touch Display** - 800x480 IPS for anchor position visualization
2. **Protective Enclosure** - Waveshare plastic case with mounting options
3. **N2K Interface** - M12 5-pin connector (NMEA 2000 compatible, CAN_H/CAN_L/VIN/GND)
4. **Auxiliary I/O** - M12 12-pin connector for RS485, I2C, buzzer, button, spare signals
5. **Wide Voltage Input** - 7-36V powered directly from N2K bus
6. **WiFi/Bluetooth** - Remote monitoring via phone app (future)
7. **RTC with Battery** - Maintains time during power loss
8. **Persistent Settings** - NVS flash storage for configuration (see Assembly Notes)

---

## Optional Components

| Item | Description | Source | Part Number | Qty | Unit Price | Total |
|------|-------------|--------|-------------|-----|------------|-------|
| O1 | 12V Waterproof Piezo Buzzer IP67 110dB | Amazon | B0D7W5BYL5 | 1 | $12.00 | $12.00 |
| O2 | Momentary Push Button (Waterproof) | Amazon | TBD | 1 | $8.00 | $8.00 |
| O3 | Deegoo-FPV GPS + Compass Module | Amazon | B0FLV6RTY1 | 1 | $18.00 | $18.00 |
| O4 | 32GB TF/MicroSD Card | Amazon | Various | 1 | $8.00 | $8.00 |

### Optional Components Details

**O1 - Buzzer**: Local audible alarm independent of MFD. Driven directly by 4.3B isolated output (no relay needed). Only required if you want audible alert separate from chartplotter alarm.

**O2 - Button**: Physical acknowledge/silence button for alarm. Waterproof panel mount. Can be used for alarm acknowledge, menu navigation, or mode switching.

**O3 - External GPS**: Standalone GPS + Compass module for operation without N2K network or as backup positioning. Connects via I2C through Aux connector. Compass chip auto-detected in software.

**O4 - TF Card**: 32GB MicroSD for data logging, configuration storage, and firmware updates. SPI mode via CH422G I/O expander.

---

## Price Summary

| Configuration | Components | Total |
|---------------|------------|-------|
| **Basic** | ESP32-4.3B + Case + N2K + Aux Connectors | **$73.00** |
| **+ Buzzer** | Basic + Local audible alarm | **$85.00** |
| **+ Button** | Basic + Physical control button | **$81.00** |
| **+ GPS** | Basic + Standalone positioning | **$91.00** |
| **+ TF Card** | Basic + Data logging/storage | **$81.00** |
| **Full Build** | Basic + All Options | **$119.00** |

---

## Ordering Links

### Basic System (Required)
1. **Waveshare ESP32-S3-Touch-LCD-4.3B:** https://www.amazon.com/dp/B0DCNSRT31
2. **Waveshare 4.3B Plastic Case:** https://www.amazon.com/dp/B0DD6VLH1Y
3. **M12 5-Pin Panel Mount (N2K):** https://www.amazon.com/dp/B0FJY18H5N
4. **M12 12-Pin Panel Mount (Aux):** https://www.amazon.com/dp/B0FLDB4FQF

### Optional Components
5. **12V Buzzer IP67:** https://www.amazon.com/dp/B0D7W5BYL5
6. **Waterproof Push Button:** TBD
7. **GPS + Compass Module:** https://www.amazon.com/dp/B0FLV6RTY1
8. **32GB TF Card:** Various (SanDisk recommended)

---

## Connector Pinouts

### M12 5-Pin (NMEA 2000 / CAN Bus) - B3

**Standard NMEA 2000 Micro-C Compatible (IEC 61076-2-101, A-Code)**

```
Pin 1: Shield (bare/drain) -> Optional chassis ground
Pin 2: NET-S / +12V (red)  -> ESP32 VIN terminal
Pin 3: NET-C / GND (black) -> ESP32 GND terminal
Pin 4: CAN_H (white)       -> ESP32 CAN_H terminal
Pin 5: CAN_L (blue)        -> ESP32 CAN_L terminal
```

**Note:** This is the standard NMEA 2000 pinout. The M12 5-pin A-coded connector is fully compatible with Garmin, Raymarine, Simrad, Lowrance, and other NMEA 2000 certified devices. Wire colors follow DeviceNet/NMEA 2000 standard.

### M12 12-Pin (Auxiliary I/O) - B4

```
Pin 1:  RS485_A (TX+)      -> NMEA 0183 transmit positive
Pin 2:  RS485_B (TX-)      -> NMEA 0183 transmit negative
Pin 3:  RS485_GND          -> NMEA 0183 ground reference
Pin 4:  I2C_SDA            -> GPS/Sensor data (3.3V)
Pin 5:  I2C_SCL            -> GPS/Sensor clock (3.3V)
Pin 6:  I2C_3V3            -> 3.3V power for I2C devices
Pin 7:  I2C_GND            -> I2C ground
Pin 8:  BUZZER+            -> Isolated output positive (5-36V)
Pin 9:  BUZZER-            -> Isolated output negative
Pin 10: BUTTON             -> Button input (active low)
Pin 11: BUTTON_GND         -> Button ground
Pin 12: SPARE              -> Future expansion
```

**Note:** The M12 12-pin uses 0.6mm diameter pins rated at 30V / 1.5A per contact. Adequate for all signal-level connections.

---

## Component Specifications

### Waveshare ESP32-S3-Touch-LCD-4.3B (B1)
- Display: 4.3" IPS 800x480, 5-point capacitive touch
- Processor: ESP32-S3 dual-core 240MHz
- Memory: 16MB Flash, 8MB PSRAM
- Power Input: 7-36V DC
- Isolated I/O: 5-36V, 450mA sink (for buzzer)
- Interfaces: CAN, RS485, I2C (screw terminals)
- TF Card: SPI mode via CH422G (SD_CS on EXIO4)
- RTC: Onboard with battery backup
- **Persistent Storage: NVS partition in flash (see below)**

### Waveshare 4.3B Plastic Case (B2)
- Material: ABS plastic
- Mounting: Panel mount or desktop
- Display window: Clear acrylic
- Rear access: Screw terminal cutouts

### M12 5-Pin Panel Mount Connector - N2K (B3)
- Quantity: 1 (panel mount with mating cable connector)
- Type: Male/Female pair, A-Code
- Thread: M12 x 1.0
- Diameter: 12mm thread (~18mm overall with coupling)
- Waterproof: IP67
- Rating: 125V / 4A per pin
- Wire: Pre-wired 0.5m leads
- Standard: IEC 61076-2-101 (NMEA 2000 Micro-C compatible)

### M12 12-Pin Panel Mount Connector - Aux (B4)
- Quantity: 1 (panel mount with mating cable connector)
- Type: Male/Female pair, A-Code
- Thread: M12 x 1.0
- Diameter: 12mm thread (~18mm overall, <0.75")
- Waterproof: IP67
- Rating: 30V / 1.5A per pin
- Wire Size: 26-30 AWG (0.6mm pin diameter)
- Application: Auxiliary I/O (RS485, I2C, buzzer, button)

### External GPS Module (O3)
- Chipset: u-blox compatible
- Compass: Auto-detect (0x0D, 0x0E, or 0x1E)
- Interface: I2C via Aux connector
- Power: 3.3V from Aux connector Pin 6

### TF Card (O4)
- Capacity: 32GB recommended
- Format: FAT32
- Speed: Class 10 or better
- Use: Data logging, config, firmware updates

---

## Assembly Notes

1. **TF Card Interface**: SD_CS pin is controlled via CH422G I/O expander (EXIO4). Initialize I2C before SPI for TF card access.

2. **I2C Bus**: Use GPIO8 (SDA) and GPIO9 (SCL). Route through Aux connector pins 4/5 for external GPS module.

3. **Power Input**: 7-36V wide input via onboard regulator. Connect to N2K bus Pin 2 (+12V) and Pin 3 (GND).

4. **CAN Bus**: Built-in TJA1051 transceiver. Enable 120Ω termination jumper if device is at end of NMEA 2000 backbone.

5. **RS485/0183**: Onboard MAX485 transceiver. Route through Aux connector pins 1/2/3 for NMEA 0183 output.

6. **Isolated Output**: Use for buzzer drive via Aux connector pins 8/9. No relay needed for typical 12V buzzers under 450mA.

7. **Button Input**: Connect to Aux connector pin 10 with ground on pin 11. Internal pull-up enabled, active low. Software debounce required.

8. **Connector Mounting**: Both M12 connectors mount in rear panel of enclosure. Use included lock nuts and gaskets for IP67 seal.

9. **Persistent Settings Storage (NVS)**:
   The ESP32-S3 provides **Non-Volatile Storage (NVS)** in the 16MB flash for saving settings:
   - **NVS Library**: Built into ESP-IDF and Arduino frameworks
   - **Capacity**: Default 24KB partition (configurable up to several MB)
   - **Wear Leveling**: Automatic, ~100,000 write cycles per sector
   - **Data Types**: Integers, strings, blobs up to 4KB each
   - **Use Cases**: Anchor position, alarm radius, calibration data, user preferences
   - **Alternative**: SPIFFS/LittleFS for larger files (logs, waypoints)
   - **TF Card**: Optional for data logging, firmware updates, large datasets

---

## Persistent Storage Options

The ESP32-S3 with 16MB flash provides multiple options for saving settings:

| Method | Size | Best For | Write Cycles | Notes |
|--------|------|----------|--------------|-------|
| **NVS** | 24KB-1MB | Settings, calibration | ~100,000 | Key-value pairs, auto wear-leveling |
| **SPIFFS/LittleFS** | 1-4MB | Config files, logs | ~100,000 | File system, JSON configs |
| **Preferences** | (uses NVS) | Arduino settings | ~100,000 | Simple Arduino API |
| **TF Card** | 32GB+ | Logs, waypoints, updates | Unlimited | Optional, external |
| **EEPROM Emulation** | 4KB | Legacy compatibility | ~100,000 | Deprecated, use NVS |

**Recommended Approach:**
- Use **Preferences/NVS** for: Anchor position, alarm settings, calibration, user prefs
- Use **LittleFS** for: Larger config files, screen layouts, waypoint lists
- Use **TF Card** for: Trip logs, position history, firmware updates

---

## Wiring Diagram Summary

```
                    +------------------+
                    |   ESP32-S3-4.3B  |
                    |                  |
  N2K Backbone      |   VIN  GND      |     External Harness
  (M12 5-Pin)       |   CAN_H CAN_L   |     (M12 12-Pin)
       |            |                  |           |
       +------------+   RS485_A/B     +-----------+
                    |   I2C SDA/SCL   |
                    |   ISO_OUT+/-    |
                    |   GPIO (Button) |
                    +------------------+
```

**Two-Connector Design Benefits:**
- Standard N2K connector for direct backbone connection
- All auxiliary signals on single 12-pin connector
- Clean separation of marine network vs. local I/O
- Both connectors < 0.75" diameter (fits tight spaces)
- IP67 waterproof on both connections
