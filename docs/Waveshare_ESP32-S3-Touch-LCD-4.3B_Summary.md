# Waveshare ESP32-S3-Touch-LCD-4.3B Specifications

## Version 1.0.0 - December 11, 2025

**Author:** Colin Bitterfield  
**Email:** colin@bitterfield.com  
**Project:** ANCHOR-ALARM

---

## Overview

The Waveshare ESP32-S3-Touch-LCD-4.3B (Type B) is an industrial-grade ESP32-S3 development board with integrated 4.3" capacitive touch display, wide voltage input, isolated digital I/O, and industrial screw terminals.

**Amazon:** B0DCNSRT31  
**Price:** ~$45-50

---

## Key Specifications

### Processor
- **SoC:** ESP32-S3-WROOM-1-N16R8
- **CPU:** Dual-core Xtensa LX7 @ 240MHz
- **Flash:** 16MB (Quad SPI)
- **PSRAM:** 8MB (Octal SPI)

### Display
- **Size:** 4.3 inches
- **Resolution:** 800 x 480 pixels
- **Type:** IPS, 65K colors
- **Driver:** ST7262 (RGB interface)
- **Backlight:** Adjustable via PWM

### Touch
- **Type:** 5-point capacitive
- **Controller:** GT911
- **Interface:** I2C (address 0x5D)
- **I2C Bus:** Internal I2C1 (GPIO15 SDA, GPIO7 SCL)

### Power
- **Input Voltage:** 7-36V DC (wide range)
- **Power Chip:** CS8501 (battery management)
- **Charge Current:** 580mA max
- **Battery Connector:** MX1.25 (3.7V LiPo)

### I/O Expander
- **Chip:** CH422G
- **Controls:** LCD backlight, touch reset, SD card, isolated outputs
- **Interface:** I2C

### Real-Time Clock
- **Feature:** Onboard RTC with rechargeable battery backup
- **Purpose:** Maintains time during power loss

---

## Interfaces (Screw Terminal)

16-pin 3.5mm screw terminal block with dual wiring options (back or bottom mount).

| Pin | Function | Notes |
|-----|----------|-------|
| 1 | VIN | 7-36V DC input |
| 2 | GND | Ground |
| 3 | 3V3 | 3.3V output (limited current) |
| 4 | CAN_H | CAN bus high (GPIO0) |
| 5 | CAN_L | CAN bus low (GPIO6) |
| 6 | RS485_A | RS485 A line |
| 7 | RS485_B | RS485 B line |
| 8 | SDA | I2C data (GPIO8) |
| 9 | SCL | I2C clock (GPIO9) |
| 10-12 | DIO | Digital I/O via CH422G |
| 13-16 | ISO | Isolated I/O (5-36V, 450mA sink) |

---

## CAN Bus (NMEA 2000)

- **Transceiver:** Onboard CAN transceiver
- **Termination:** 120Ω switchable (onboard jumper)
- **Pins:** CAN_H (GPIO0), CAN_L (GPIO6)
- **Speed:** Up to 1 Mbps (N2K uses 250 kbps)

---

## Isolated Digital I/O

The 4.3B includes optically isolated digital outputs capable of:
- **Voltage Range:** 5-36V
- **Sink Current:** Up to 450mA per channel
- **Channels:** 4 isolated outputs

This allows direct driving of 12V buzzers, relays, and indicators without external relay modules.

---

## I2C Buses

| Bus | SDA | SCL | Usage |
|-----|-----|-----|-------|
| I2C0 | GPIO8 | GPIO9 | External devices (screw terminal) |
| I2C1 | GPIO15 | GPIO7 | Internal (touch controller GT911) |

**Note:** External I2C devices connect to I2C0 via the screw terminal.

---

## Comparison: 4.3B vs 4.0

| Feature | 4.3B (Type B) | 4.0 |
|---------|---------------|-----|
| Display Size | 4.3" | 4.0" |
| Resolution | 800x480 | 480x480 |
| Voltage Input | 7-36V | 7-12V |
| Isolated I/O | Yes (5-36V, 450mA) | No |
| Screw Terminal | 16-pin, back/bottom | 16-pin |
| RTC | Yes (with battery) | Varies |
| Price | ~$45 | ~$45 |

The 4.3B is recommended for marine applications due to wider voltage tolerance and isolated outputs.

---

## Software Resources

### Arduino/PlatformIO
- **Board:** ESP32-S3 Dev Module
- **Flash Mode:** QIO 80MHz
- **PSRAM:** OPI PSRAM
- **Partition:** 16MB Flash (3MB APP/9.9MB FATFS)

### Libraries
- **Display:** LovyanGFX or Arduino_GFX (RGB interface)
- **Touch:** GT911 via I2C1
- **LVGL:** Supported (use RGB flush callback)

### Waveshare Resources
- **Wiki:** https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-4.3B
- **Demo Code:** https://github.com/waveshareteam/ESP32-S3-Touch-LCD-4.3B
- **Schematic:** Available on wiki

---

## Pin Mapping Summary

### Display (RGB Interface)
```
HSYNC: GPIO39
VSYNC: GPIO40
DE:    GPIO41
PCLK:  GPIO42
R0-R4: GPIO45, GPIO48, GPIO47, GPIO21, GPIO14
G0-G5: GPIO10, GPIO9, GPIO46, GPIO3, GPIO8, GPIO18
B0-B4: GPIO17, GPIO16, GPIO15, GPIO7, GPIO6
```

### Touch (I2C1)
```
SDA:   GPIO15
SCL:   GPIO7
INT:   GPIO4
RST:   Via CH422G
ADDR:  0x5D
```

### CAN Bus
```
CAN_H: GPIO0 (via transceiver)
CAN_L: GPIO6 (via transceiver)
```

### External I2C (I2C0)
```
SDA:   GPIO8 (screw terminal pin 8)
SCL:   GPIO9 (screw terminal pin 9)
```

---

## Application Notes

### NMEA 2000 Connection
The onboard CAN transceiver connects directly to NMEA 2000 networks:
1. Connect CAN_H (white) to terminal pin 4
2. Connect CAN_L (blue) to terminal pin 5
3. Connect +12V (red) to VIN terminal pin 1
4. Connect GND (black) to GND terminal pin 2
5. Set termination jumper based on network position

### Buzzer Direct Drive
The isolated outputs can drive 12V buzzers directly:
1. Connect buzzer positive to N2K +12V
2. Connect buzzer negative to isolated output
3. Software activates output to sink current and sound buzzer

### External Sensors
GPS and compass modules connect via:
- **UART:** Internal GPIOs (not on screw terminal)
- **I2C:** Screw terminal pins 8 (SDA) and 9 (SCL)
