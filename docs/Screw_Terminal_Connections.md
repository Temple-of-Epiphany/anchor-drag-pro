
go# M12 Connector to Screw Terminal Wiring Map

## Version 1.0.0 - December 14, 2025

**Author:** Colin Bitterfield
**Email:** colin@bitterfield.com
**Project:** ANCHOR-ALARM

---

## Overview

This document maps the M12 panel mount connectors to the ESP32-S3-Touch-LCD-4.3B screw terminals. The system uses two M12 connectors:

- **M12 5-Pin (B3)**: NMEA 2000 / CAN Bus power and data
- **M12 12-Pin (B4)**: Auxiliary I/O (RS485, I2C, outputs, inputs)

---

## M12 5-Pin (N2K) Connector to ESP32 Terminals

**Standard NMEA 2000 Micro-C Compatible (IEC 61076-2-101, A-Code)**

| M12 Pin | Signal Name | Wire Color | ESP32 Terminal | Terminal Function | Notes |
|---------|-------------|------------|----------------|-------------------|-------|
| 1 | Shield/Drain | Bare | Optional | Chassis GND | Optional connection to enclosure |
| 2 | NET-S / +12V | Red | 1 (VIN) | Power Input | 7-36V DC from N2K bus |
| 3 | NET-C / GND | Black | 2 (GND) | Ground | Power ground |
| 4 | CAN_H | White | 8 (CAN-H) | CAN Bus High | Via TJA1051 transceiver |
| 5 | CAN_L | Blue | 7 (CAN-L) | CAN Bus Low | Via TJA1051 transceiver |

### Wiring Diagram (M12 5-Pin)

M12 5-Pin Connector (N2K)              ESP32-S3-Touch-LCD-4.3B
┌────────────────────┐                 ┌─────────────────┐
│                    │                 │                 │
│ Pin 1 (Shield)     │────────────────►│ Chassis GND     │ (Optional)
│                    │                 │                 │
│ Pin 2 (+12V Red)   │────────────────►│ Terminal 1 VIN  │
│                    │                 │                 │
│ Pin 3 (GND Black)  │────────────────►│ Terminal 2 GND  │
│                    │                 │                 │
│ Pin 4 (CAN_H White)│────────────────►│ Terminal 8 CAN-H│
│                    │                 │                 │
│ Pin 5 (CAN_L Blue) │────────────────►│ Terminal 7 CAN-L│
│                    │                 │                 │
└────────────────────┘                 └─────────────────┘

### Notes
- This is the standard NMEA 2000 pinout, fully compatible with Garmin, Raymarine, Simrad, Lowrance, and other NMEA 2000 certified devices
- Wire colors follow DeviceNet/NMEA 2000 standard
- 120Ω termination: Enable via onboard switch if device is at backbone end
- Receives PGN 129029 (Position) and PGN 129025 (Rapid Position)

---

## M12 12-Pin (Aux) Connector to ESP32 Terminals

**Auxiliary I/O for RS485, I2C, Outputs, and Inputs**

| M12 Pin | Signal Name | ESP32 Terminal | Terminal Function | GPIO/Control | Specifications |
|---------|-------------|----------------|-------------------|--------------|----------------|
| 1 | RS485_A (TX+) | 10 | RS485-A | GPIO44 (TX) | Via MAX485 transceiver |
| 2 | RS485_B (TX-) | 9 | RS485-B | GPIO43 (RX) | Via MAX485 transceiver |
| 3 | RS485_GND | 14 | GND | - | Ground reference for RS485 |
| 4 | I2C_SDA | 5 | SDA | GPIO8 | 3.3V logic, shared bus |
| 5 | I2C_SCL | 6 | SCL | GPIO9 | 3.3V logic, shared bus |
| 6 | I2C_3V3 | 3 | VOUT | - | 5V or 3.3V (resistor select) |
| 7 | I2C_GND | 4 | GND | - | I2C ground |
| 8 | BUZZER+ | External | - | - | Connect to boat 12V/24V |
| 9 | BUZZER- | 11 | DO0 | CH422G OD0 | 5-36V, 450mA max, open-drain |
| 10 | BUTTON | 15 | DI0 | CH422G EXIO0 | 5-36V isolated input |
| 11 | BUTTON_GND | 13 | DI COM | - | Reference for digital inputs |
| 12 | SPARE | - | - | - | Future expansion |


### Wiring Diagram (M12 12-Pin)

M12 12-Pin Connector (Aux)             ESP32-S3-Touch-LCD-4.3B
┌────────────────────┐                 ┌─────────────────┐
│                    │                 │                 │
│ Pin 1 (RS485_A)    │────────────────►│ Terminal 10     │ RS485-A (GPIO44 TX)
│                    │                 │                 │
│ Pin 2 (RS485_B)    │────────────────►│ Terminal 9      │ RS485-B (GPIO43 RX)
│                    │                 │                 │
│ Pin 3 (RS485_GND)  │────────────────►│ Terminal 14 GND │
│                    │                 │                 │
│ Pin 4 (I2C_SDA)    │────────────────►│ Terminal 5 SDA  │ GPIO8
│                    │                 │                 │
│ Pin 5 (I2C_SCL)    │────────────────►│ Terminal 6 SCL  │ GPIO9
│                    │                 │                 │
│ Pin 6 (I2C_3V3)    │◄───────────────│ Terminal 3 VOUT │ 5V or 3.3V
│                    │                 │                 │
│ Pin 7 (I2C_GND)    │────────────────►│ Terminal 4 GND  │
│                    │                 │                 │
│ Pin 8 (BUZZER+)    │◄─────────────── Boat 12V/24V DC
│                    │                 │                 │
│ Pin 9 (BUZZER-)    │────────────────►│ Terminal 11 DO0 │ CH422G OD0
│                    │                 │                 │
│ Pin 10 (BUTTON)    │────────────────►│ Terminal 15 DI0 │ CH422G EXIO0
│                    │                 │                 │
│ Pin 11 (BUTTON_GND)│────────────────►│ Terminal 13 DI  │ COM
│                    │                 │                 │
│ Pin 12 (SPARE)     │ (No connection) │                 │
│                    │                 │                 │
└────────────────────┘                 └─────────────────┘

---

## Buzzer Wiring Detail

The buzzer requires external power and ground-side switching:

Boat +12V/24V ──────────────┬─────────────────────────
                            │
                            │    ┌──────────────┐
                            └────┤ Buzzer (+)   │ ◄── M12 Pin 8
                                 │              │
                      M12 Pin 9 ─┤ Buzzer (-)   │ ──► Terminal 11 (DO0)
                                 └──────────────┘
                                         │
                                    Open-drain sinks to ground when active

**Operation:** Firmware sets DO0 LOW → Buzzer sounds

---

## Button Wiring Detail

The button provides alarm mute/acknowledge functionality:

                            ┌─────────────────────────┐
                            │   MOMENTARY SWITCH      │
                            │      (Normally Open)    │
              M12 Pin 10 ───┤ Contact 1               │ ──► Terminal 15 (DI0)
                            │                         │
              M12 Pin 11 ───┤ Contact 2               │ ──► Terminal 13 (DI COM)
                            │                         │
                            └─────────────────────────┘

**Operation:** Press button → DI0 sees continuity → Firmware mutes alarm

---

## Complete Terminal Assignment Table

### ESP32-S3-Touch-LCD-4.3B 16-Pin Screw Terminals

| Terminal | Label | Function | Connected To | M12 Source |
|----------|-------|----------|--------------|------------|
| 1 | VIN | Power Input | M12 5-Pin Pin 2 (Red) | N2K +12V |
| 2 | GND | Ground | M12 5-Pin Pin 3 (Black) | N2K GND |
| 3 | VOUT | I2C Power Out | M12 12-Pin Pin 6 | I2C_3V3 |
| 4 | GND | I2C Ground | M12 12-Pin Pin 7 | I2C_GND |
| 5 | SDA | I2C Data | M12 12-Pin Pin 4 | I2C_SDA |
| 6 | SCL | I2C Clock | M12 12-Pin Pin 5 | I2C_SCL |
| 7 | CAN-L | CAN Bus Low | M12 5-Pin Pin 5 (Blue) | N2K CAN_L |
| 8 | CAN-H | CAN Bus High | M12 5-Pin Pin 4 (White) | N2K CAN_H |
| 9 | RS485-B | RS485 B (-) | M12 12-Pin Pin 2 | RS485_B |
| 10 | RS485-A | RS485 A (+) | M12 12-Pin Pin 1 | RS485_A |
| 11 | DO0 | Digital Output 0 | M12 12-Pin Pin 9 | BUZZER- |
| 12 | DO1 | Digital Output 1 | *Not connected* | (Alarm Panel / Relay) |
| 13 | DI COM | Digital Input Common | M12 12-Pin Pin 11 | BUTTON_GND |
| 14 | GND | Digital I/O Ground | M12 12-Pin Pin 3 | RS485_GND |
| 15 | DI0 | Digital Input 0 | M12 12-Pin Pin 10 | BUTTON |
| 16 | DI1 | Digital Input 1 | *Not connected* | (Anchor Sensor) |

---

## Unused Resources

### Available for Future Expansion

| Resource | Terminal | M12 Pin | Potential Use |
|----------|----------|---------|---------------|
| DO1 | 12 | - | Alarm panel trigger, external relay |
| DI1 | 16 | - | Anchor down sensor, windlass interlock |
| SPARE | - | 12-Pin Pin 12 | Additional signal wire |

---

## Revision History

| Version | Date | Changes |
|---------|------|---------|
| 1.0.0 | 2025-12-14 | Initial wiring map for M12 5-pin and 12-pin connectors |

