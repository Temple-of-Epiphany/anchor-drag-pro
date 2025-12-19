# Anchor Drag Alarm - Wiring Guide

## Version 2.0.0 - December 12, 2025

**Author:** Colin Bitterfield  
**Email:** colin@bitterfield.com  
**Project:** ANCHOR-ALARM

---

## Changelog

- v2.0.0 (2025-12-12): Complete rewrite with correct 16-pin terminal mapping, three GPS options, alarm panel support
- v1.0.0 (2025-12-11): Initial version

---

## Hardware Platform

**Controller:** Waveshare ESP32-S3-Touch-LCD-4.3B  
**Display:** 4.3" 800x480 Capacitive Touch  
**Power Input:** 7-36V DC  
**Processor:** ESP32-S3 Dual-core 240MHz, 16MB Flash, 8MB PSRAM

---

## 16-Pin Screw Terminal Pinout

| Terminal | Label | Function | GPIO/Control | Specifications |
|----------|-------|----------|--------------|----------------|
| 1 | VIN | Power Input | - | 7-36V DC |
| 2 | GND | Ground | - | Power ground |
| 3 | VOUT | I2C Power Out | - | 5V or 3.3V (resistor selectable) |
| 4 | GND | Ground | - | I2C ground |
| 5 | SDA | I2C Data | GPIO8 | 3.3V logic, shared bus |
| 6 | SCL | I2C Clock | GPIO9 | 3.3V logic, shared bus |
| 7 | CAN-L | CAN Bus Low | GPIO16 | Via transceiver |
| 8 | CAN-H | CAN Bus High | GPIO15 | Via transceiver |
| 9 | RS485-B | RS485 B (-) | GPIO43 (RX) | Via transceiver |
| 10 | RS485-A | RS485 A (+) | GPIO44 (TX) | Via transceiver |
| 11 | DO0 | Digital Output 0 | CH422G OD0 | 5-36V, 450mA max, open-drain |
| 12 | DO1 | Digital Output 1 | CH422G OD1 | 5-36V, 450mA max, open-drain |
| 13 | DI COM | Digital Input Common | - | Reference for inputs |
| 14 | GND | Ground | - | Digital I/O ground |
| 15 | DI0 | Digital Input 0 | CH422G EXIO0 | 5-36V, optocoupler isolated |
| 16 | DI1 | Digital Input 1 | CH422G EXIO5 | 5-36V, optocoupler isolated |

---

## GPS Source Options

Three mutually exclusive GPS configurations. Select one based on boat electronics.

### Option A: NEO-8M Standalone GPS (I2C)

**Use Case:** Boats without existing marine electronics  
**Power Source:** 12V or 24V DC direct  

| Terminal | Connection | Wire Color |
|----------|------------|------------|
| 3 (VOUT) | NEO-8M VCC | Red |
| 4 (GND) | NEO-8M GND | Black |
| 5 (SDA) | NEO-8M SDA | White |
| 6 (SCL) | NEO-8M SCL | Orange |

**Notes:**
- Set VOUT to 5V via onboard resistor selection
- NEO-8M I2C address: 0x42 (no conflict with onboard devices)
- Green (TX) and Yellow (RX) wires not connected for I2C mode

### Option B: NMEA 0183 GPS (RS485)

**Use Case:** Boats with older chartplotter/GPS outputting NMEA 0183  
**Power Source:** 12V or 24V DC direct  

| Terminal | Connection |
|----------|------------|
| 9 (RS485-B) | NMEA 0183 Data (-) |
| 10 (RS485-A) | NMEA 0183 Data (+) |
| 14 (GND) | NMEA 0183 Ground (if required) |

**Notes:**
- RS485 transceiver auto-switches TX/RX direction
- Standard NMEA 0183 baud rate: 4800 (configurable to 38400)
- Receives GGA, RMC, VTG sentences

### Option C: NMEA 2000 GPS (CAN)

**Use Case:** Modern boats with N2K network  
**Power Source:** N2K bus power (12V)  

| Terminal | Connection | N2K Wire Color |
|----------|------------|----------------|
| 1 (VIN) | N2K V+ | Red |
| 2 (GND) | N2K V- | Black |
| 7 (CAN-L) | N2K Net-L | Blue |
| 8 (CAN-H) | N2K Net-H | White |

**Notes:**
- 120Ω termination: Enable via onboard switch if at backbone end
- Receives PGN 129029 (Position) and PGN 129025 (Rapid Position)
- Shield/drain wire optional to terminal 2 (GND)

---

## Common Wiring (All Options)

### Power Input

| Boat Voltage | Terminal 1 (VIN) | Terminal 2 (GND) |
|--------------|------------------|------------------|
| 12V System | +12V DC | Negative |
| 24V System | +24V DC | Negative |
| N2K Powered | N2K Red | N2K Black |

### Alarm Outputs

#### DO0: Buzzer (Direct Drive)

For 12V or 24V buzzers under 450mA:

```
Boat +V ──────────────┬─────────────────────────
(12V or 24V)          │
                      │    ┌──────────────┐
                      └────┤ Buzzer (+)   │
                           │              │
Terminal 11 (DO0) ────┬────┤ Buzzer (-)   │
                      │    └──────────────┘
                      │
                      └─── Open-drain sinks to ground when active
```

**Operation:** Firmware sets DO0 LOW → Buzzer sounds

#### DO1: Alarm Panel (Pull-to-Ground Trigger)

For alarm panels requiring ground trigger:

```
                           ┌─────────────────────────┐
                           │      ALARM PANEL        │
                           │                         │
Terminal 12 (DO1) ─────────┤ Trigger Input (-)       │
                           │                         │
Terminal 14 (GND) ─────────┤ Ground (if needed)      │
                           │                         │
                           └─────────────────────────┘
```

**Operation:** Firmware sets DO1 LOW → Panel triggers

#### DO1: External Relay (Alternative)

For auxiliary functions (anchor light, secondary alert, etc.):

```
Boat +V ──────────────┬─────────────────────────
(12V or 24V)          │
                      │    ┌──────────────┐
                      └────┤ Relay Coil + │
                           │              │
Terminal 12 (DO1) ─────────┤ Relay Coil - │
                           └──────────────┘
                                  │
                           ┌──────┴──────┐
                           │   Relay     │
                           │  Contacts   │──── To controlled load
                           └─────────────┘
```

### Mute Button Input (DI0)

#### Dry Contact (Simple Switch)

```
                           ┌─────────────────────────┐
                           │     MOMENTARY SWITCH    │
                           │        (N/O)            │
Terminal 15 (DI0) ─────────┤ Contact 1               │
                           │                         │
Terminal 13 (DI COM) ──────┤ Contact 2               │
                           │                         │
                           └─────────────────────────┘
```

**Operation:** Press button → DI0 sees continuity → Firmware mutes alarm

#### Wet Contact (Voltage Signal)

For panel switches providing voltage output:

```
External 12V+ ─────────────┬─────────────────────────
                           │
                           │    ┌───────────────────┐
                           │    │   PANEL SWITCH    │
                           └────┤ V+ In             │
                                │                   │
Terminal 15 (DI0) ──────────────┤ Signal Out        │
                                │                   │
Terminal 13 (DI COM) ───────────┤ V- / Common       │
                                │                   │
                                └───────────────────┘
```

### Optional: Anchor Down Sensor (DI1)

For automatic arm/disarm based on anchor deployment:

```
                           ┌─────────────────────────┐
                           │   ANCHOR SWITCH         │
                           │   (Windlass or Manual)  │
Terminal 16 (DI1) ─────────┤ Contact 1               │
                           │                         │
Terminal 13 (DI COM) ──────┤ Contact 2               │
                           │                         │
                           └─────────────────────────┘
```

**Operation:** Switch closes when anchor deployed → Auto-arm system

---

## Complete Wiring Diagrams

### Option A: Standalone System (NEO-8M GPS)

```
                                    ┌─────────────────┐
     12V/24V DC ───────────────────►│ 1  VIN          │
                                    │                 │
     DC Negative ──────────────────►│ 2  GND          │
                                    │                 │
     NEO-8M VCC (Red) ◄────────────│ 3  VOUT (5V)    │
                                    │                 │
     NEO-8M GND (Black) ◄──────────│ 4  GND          │
                                    │                 │
     NEO-8M SDA (White) ◄──────────│ 5  SDA          │
                                    │                 │
     NEO-8M SCL (Orange) ◄─────────│ 6  SCL          │
                                    │                 │
                         (unused)   │ 7  CAN-L        │
                                    │                 │
                         (unused)   │ 8  CAN-H        │
                                    │                 │
                         (unused)   │ 9  RS485-B      │
                                    │                 │
                         (unused)   │ 10 RS485-A      │
                                    │                 │
     Buzzer (-) ◄──────────────────│ 11 DO0          │
                                    │                 │
     Alarm Panel Trigger ◄─────────│ 12 DO1          │
                                    │                 │
     Mute Switch COM ◄─────────────│ 13 DI COM       │
                                    │                 │
     Alarm Panel GND ◄─────────────│ 14 GND          │
                                    │                 │
     Mute Switch Signal ◄──────────│ 15 DI0          │
                                    │                 │
     Anchor Sensor ◄───────────────│ 16 DI1          │
                                    └─────────────────┘
                                     ESP32-S3-Touch
                                       LCD-4.3B

     External Connections:
     ─────────────────────
     Buzzer (+) ◄──────────────────── 12V/24V DC Positive
```

### Option B: NMEA 0183 System

```
                                    ┌─────────────────┐
     12V/24V DC ───────────────────►│ 1  VIN          │
                                    │                 │
     DC Negative ──────────────────►│ 2  GND          │
                                    │                 │
                         (unused)   │ 3  VOUT         │
                                    │                 │
                         (unused)   │ 4  GND          │
                                    │                 │
                         (unused)   │ 5  SDA          │
                                    │                 │
                         (unused)   │ 6  SCL          │
                                    │                 │
                         (unused)   │ 7  CAN-L        │
                                    │                 │
                         (unused)   │ 8  CAN-H        │
                                    │                 │
     NMEA 0183 Data (-) ───────────►│ 9  RS485-B      │
                                    │                 │
     NMEA 0183 Data (+) ───────────►│ 10 RS485-A      │
                                    │                 │
     Buzzer (-) ◄──────────────────│ 11 DO0          │
                                    │                 │
     Alarm Panel Trigger ◄─────────│ 12 DO1          │
                                    │                 │
     Mute Switch COM ◄─────────────│ 13 DI COM       │
                                    │                 │
     NMEA/Alarm Panel GND ◄────────│ 14 GND          │
                                    │                 │
     Mute Switch Signal ◄──────────│ 15 DI0          │
                                    │                 │
     Anchor Sensor ◄───────────────│ 16 DI1          │
                                    └─────────────────┘
```

### Option C: NMEA 2000 System

```
                                    ┌─────────────────┐
     N2K V+ (Red) ─────────────────►│ 1  VIN          │
                                    │                 │
     N2K V- (Black) ───────────────►│ 2  GND          │
                                    │                 │
                         (unused)   │ 3  VOUT         │
                                    │                 │
                         (unused)   │ 4  GND          │
                                    │                 │
                         (unused)   │ 5  SDA          │
                                    │                 │
                         (unused)   │ 6  SCL          │
                                    │                 │
     N2K Net-L (Blue) ─────────────►│ 7  CAN-L        │
                                    │                 │
     N2K Net-H (White) ────────────►│ 8  CAN-H        │
                                    │                 │
                         (unused)   │ 9  RS485-B      │
                                    │                 │
                         (unused)   │ 10 RS485-A      │
                                    │                 │
     Buzzer (-) ◄──────────────────│ 11 DO0          │
                                    │                 │
     Alarm Panel Trigger ◄─────────│ 12 DO1          │
                                    │                 │
     Mute Switch COM ◄─────────────│ 13 DI COM       │
                                    │                 │
     Alarm Panel GND ◄─────────────│ 14 GND          │
                                    │                 │
     Mute Switch Signal ◄──────────│ 15 DI0          │
                                    │                 │
     Anchor Sensor ◄───────────────│ 16 DI1          │
                                    └─────────────────┘

     N2K Note: Buzzer (+) connects to boat 12V (same source as N2K power)
```

---

## I/O Allocation Summary

| Resource | Terminal | Function | Status |
|----------|----------|----------|--------|
| DO0 | 11 | Buzzer | Allocated |
| DO1 | 12 | Alarm Panel / Relay | Allocated |
| DI0 | 15 | Mute Button | Allocated |
| DI1 | 16 | Anchor Down Sensor | Optional |

---

## Power Considerations

### Input Voltage Range

| Source | Voltage | Compatible |
|--------|---------|------------|
| 12V Boat System | 10.5-14.4V | Yes |
| 24V Boat System | 22-28V | Yes |
| N2K Bus Power | 9-16V | Yes |

### Current Draw (Estimated)

| Mode | Current Draw |
|------|--------------|
| Active (display on, GPS polling) | 150-250mA |
| Display dimmed | 80-120mA |
| Alarm active (with buzzer) | +50-150mA |
| Deep sleep (future) | 10-50mA |

### Battery Backup

Connect 3.7V LiPo to onboard MX1.25 battery connector for power-loss protection.

---

## CAN Bus Termination (Option C Only)

NMEA 2000 requires 120Ω termination at each backbone end.

- **Device at backbone end:** Set termination switch to ON
- **Device mid-backbone:** Set termination switch to OFF

---

## I2C Bus Notes (Option A Only)

### Shared I2C Bus Devices

| Address | Device | Purpose |
|---------|--------|---------|
| 0x20-0x27 | CH422G | IO Expander (onboard) |
| 0x5D | GT911 | Touch Controller (onboard) |
| 0x42 | NEO-8M | GPS Module (external) |

### VOUT Voltage Selection

Resistor selection on PCB determines VOUT voltage:
- **5V:** Recommended for NEO-8M
- **3.3V:** For 3.3V-only I2C sensors

Check/modify resistor position before connecting external I2C devices.

---

## Wire Specifications

### Recommended Wire Gauge

| Connection | Gauge | Notes |
|------------|-------|-------|
| Power (VIN/GND) | 18-22 AWG | Match boat DC wiring standards |
| CAN/RS485 | 22-24 AWG | Twisted pair recommended |
| I2C | 24-26 AWG | Keep under 1 meter |
| Digital I/O | 22-24 AWG | Standard signal wire |

### Marine Considerations

- Use tinned copper wire for corrosion resistance
- Apply dielectric grease to terminal connections
- Use heat shrink or liquid electrical tape on splices
- Route cables away from high-voltage AC and RF sources

---

## Troubleshooting

### No Power

1. Verify VIN voltage at terminal 1 (7-36V required)
2. Check GND connection at terminal 2
3. Verify power switch position (if equipped)

### GPS Not Detected (Option A)

1. Verify VOUT voltage (should be 5V or 3.3V)
2. Check SDA/SCL connections
3. Run I2C scan to detect address 0x42
4. Confirm NEO-8M is in DDC (I2C) mode

### CAN Communication Failure (Option C)

1. Verify CAN-H and CAN-L are not swapped
2. Check termination switch position
3. Confirm N2K backbone is powered
4. Monitor CAN bus with analyzer tool

### Buzzer Not Sounding

1. Verify buzzer polarity (+ to power, - to DO0)
2. Check buzzer current draw (must be under 450mA)
3. Test DO0 with multimeter (should pull to GND when active)

### Alarm Panel Not Triggering

1. Verify panel accepts ground trigger
2. Check GND connection at terminal 14
3. Test DO1 output with multimeter

---

## Revision History

| Version | Date | Changes |
|---------|------|---------|
| 2.0.0 | 2025-12-12 | Complete rewrite with correct pinout, three GPS options |
| 1.0.0 | 2025-12-11 | Initial draft |
