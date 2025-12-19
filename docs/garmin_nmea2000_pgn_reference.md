# Garmin NMEA 2000 PGN Reference

**Author:** Colin Bitterfield
**Email:** colin@bitterfield.com
**Date Created:** 2025-12-02
**Version:** 0.1.0

## Document Purpose

This document provides a comprehensive reference of NMEA 2000 Parameter Group Numbers (PGNs) supported by Garmin marine devices, with specific focus on the Chartplotter Control Unit (CCU) from the GHP 10 autopilot system and common marine sensor PGNs.

---

## Table of Contents

1. [Garmin CCU (GHP 10 Autopilot) PGNs](#garmin-ccu-ghp-10-autopilot-pgns)
2. [Common Marine Sensor PGNs](#common-marine-sensor-pgns)
3. [PGN Categories](#pgn-categories)
4. [Implementation Notes](#implementation-notes)
5. [References](#references)

---

## Garmin CCU (GHP 10 Autopilot) PGNs

The Chartplotter Control Unit (CCU) is part of the Garmin GHP 10 autopilot system. It communicates via NMEA 2000 network to receive navigation data and transmit heading information.

### Transmit PGNs (CCU Sends)

| PGN    | Hex    | Name                                          | Description                                    |
|--------|--------|-----------------------------------------------|------------------------------------------------|
| 059392 | 0xE800 | ISO Acknowledgment                            | Standard ISO acknowledgment message            |
| 059904 | 0xEA00 | ISO Request                                   | Request for specific PGN data                  |
| 060928 | 0xEE00 | ISO Address Claim                             | Device address claim on network                |
| 126208 | 0x1ED00| NMEA Command/Request/Acknowledge Group        | NMEA command and acknowledge functions         |
| 126464 | 0x1EE00| Transmit/Receive PGN List Group Function      | List of PGNs device can transmit/receive       |
| 126996 | 0x1F014| Product Information                           | Device manufacturer and product details        |
| 127250 | 0x1F112| Vessel Heading                                | Current magnetic heading of vessel             |

### Receive PGNs (CCU Listens For)

| PGN    | Hex    | Name                                          | Description                                    |
|--------|--------|-----------------------------------------------|------------------------------------------------|
| 059392 | 0xE800 | ISO Acknowledgment                            | Standard ISO acknowledgment message            |
| 059904 | 0xEA00 | ISO Request                                   | Request for specific PGN data                  |
| 060928 | 0xEE00 | ISO Address Claim                             | Device address claim on network                |
| 126208 | 0x1ED00| NMEA Command/Request/Acknowledge Group        | NMEA command and acknowledge functions         |
| 126464 | 0x1EE00| Transmit/Receive PGN List Group Function      | List of PGNs device can transmit/receive       |
| 126996 | 0x1F014| Product Information                           | Device manufacturer and product details        |
| 127258 | 0x1F11A| Magnetic Variation                            | Local magnetic variation (declination)         |
| 127488 | 0x1F200| Engine Parameters - Rapid Update              | Engine RPM, boost pressure, tilt/trim          |
| 129025 | 0x1F801| Position - Rapid Update                       | GPS position at high update rate               |
| 129026 | 0x1F802| COG & SOG - Rapid Update                      | Course Over Ground & Speed Over Ground         |
| 129283 | 0x1F903| Cross Track Error                             | Distance off desired track                     |
| 129284 | 0x1F904| Navigation Data                               | Waypoint info, bearing, distance to waypoint   |

### Proprietary PGNs

**Note:** All Garmin NMEA 2000 devices use proprietary PGN numbers:
- `126720` (0x1EF00) - Garmin proprietary
- `061184` (0xEF00) - Garmin proprietary

---

## Common Marine Sensor PGNs

These PGNs are commonly supported by Garmin chartplotters, marine instruments, and sensors. This list represents the data types you might want to simulate or receive from other marine devices.

### Navigation & Position

| PGN    | Hex    | Name                                          | Update Rate | Description                                    |
|--------|--------|-----------------------------------------------|-------------|------------------------------------------------|
| 129025 | 0x1F801| Position - Rapid Update                       | 5-10 Hz     | Latitude/longitude rapid update                |
| 129026 | 0x1F802| COG & SOG - Rapid Update                      | 5-10 Hz     | Course Over Ground & Speed Over Ground         |
| 129029 | 0x1F805| GNSS Position Data                            | 1 Hz        | Complete GNSS position with quality indicators |
| 129033 | 0x1F809| Local Time Offset                             | On change   | Local time zone offset from UTC                |
| 129283 | 0x1F903| Cross Track Error                             | 1 Hz        | Distance and direction off intended track      |
| 129284 | 0x1F904| Navigation Data                               | 1 Hz        | Active waypoint, bearing, distance, ETA        |
| 129285 | 0x1F905| Navigation - Route/WP Information             | On change   | Route and waypoint details                     |

### Heading & Attitude

| PGN    | Hex    | Name                                          | Update Rate | Description                                    |
|--------|--------|-----------------------------------------------|-------------|------------------------------------------------|
| 127237 | 0x1F10D| Heading/Track Control                         | On change   | Autopilot heading control commands             |
| 127250 | 0x1F112| Vessel Heading                                | 10 Hz       | Magnetic heading from compass                  |
| 127251 | 0x1F113| Rate of Turn                                  | 10 Hz       | Rate of turn in degrees per second             |
| 127257 | 0x1F119| Attitude                                      | 10 Hz       | Pitch, roll, yaw (3-axis orientation)          |
| 127258 | 0x1F11A| Magnetic Variation                            | On change   | Local magnetic variation/declination           |

### Water & Depth Data

| PGN    | Hex    | Name                                          | Update Rate | Description                                    |
|--------|--------|-----------------------------------------------|-------------|------------------------------------------------|
| 128259 | 0x1F503| Speed - Water Referenced                      | 1-2 Hz      | Speed through water (from paddle wheel)        |
| 128267 | 0x1F50B| Water Depth                                   | 1 Hz        | Depth below transducer                         |
| 128275 | 0x1F513| Distance Log                                  | 1 Hz        | Cumulative and trip distance traveled          |

### Engine & Propulsion

| PGN    | Hex    | Name                                          | Update Rate | Description                                    |
|--------|--------|-----------------------------------------------|-------------|------------------------------------------------|
| 127488 | 0x1F200| Engine Parameters - Rapid Update              | 10 Hz       | Engine RPM, boost pressure, tilt/trim          |
| 127489 | 0x1F201| Engine Parameters - Dynamic                   | 1 Hz        | Oil pressure, temperature, alternator voltage  |
| 127493 | 0x1F205| Transmission Parameters - Dynamic             | 1 Hz        | Gear status, oil pressure/temperature          |
| 127497 | 0x1F209| Trip Fuel Consumption - Engine                | 1 Hz        | Fuel rate, economy, trip fuel used             |
| 127505 | 0x1F211| Fluid Level                                   | 2 s         | Tank levels (fuel, water, waste, oil)          |

### Environmental Data

| PGN    | Hex    | Name                                          | Update Rate | Description                                    |
|--------|--------|-----------------------------------------------|-------------|------------------------------------------------|
| 130306 | 0x1FD02| Wind Data                                     | 1 Hz        | Wind speed and direction (apparent/true)       |
| 130310 | 0x1FD06| Environmental Parameters                      | 2 s         | Outside/sea/inside temp, humidity, pressure    |
| 130312 | 0x1FD08| Temperature                                   | 2 s         | Temperature from specific source/instance      |
| 130313 | 0x1FD09| Humidity                                      | 2 s         | Relative humidity (inside/outside)             |
| 130314 | 0x1FD0A| Actual Pressure                               | 2 s         | Barometric or other pressure reading           |
| 130316 | 0x1FD0C| Temperature - Extended Range                  | 2 s         | Temperature with extended range (-273°C+)      |
| 130323 | 0x1FD13| Meteorological Station Data                   | 1 s         | Comprehensive weather data                     |

### Electrical Systems

| PGN    | Hex    | Name                                          | Update Rate | Description                                    |
|--------|--------|-----------------------------------------------|-------------|------------------------------------------------|
| 127506 | 0x1F212| DC Detailed Status                            | 1.5 s       | DC voltage, current, temperature per instance  |
| 127508 | 0x1F214| Battery Status                                | 1.5 s       | Battery voltage, current, temperature, SOC     |
| 127513 | 0x1F219| Battery Configuration Status                  | On change   | Battery type, capacity, chemistry              |

### AIS & Safety

| PGN    | Hex    | Name                                          | Update Rate | Description                                    |
|--------|--------|-----------------------------------------------|-------------|------------------------------------------------|
| 129038 | 0x1F80E| AIS Class A Position Report                   | Variable    | AIS target position (Class A vessels)          |
| 129039 | 0x1F80F| AIS Class B Position Report                   | Variable    | AIS target position (Class B vessels)          |
| 129040 | 0x1F810| AIS Class B Extended Position Report          | Variable    | Extended Class B position data                 |
| 129041 | 0x1F811| AIS Aids to Navigation (AtoN) Report          | Variable    | Navigation aids position and info              |
| 129794 | 0x1FAE2| AIS Class A Static and Voyage Related Data    | On change   | Ship name, dimensions, destination             |
| 129809 | 0x1FAF1| AIS Class B Static Data - Part A              | On change   | Ship name for Class B                          |
| 129810 | 0x1FAF2| AIS Class B Static Data - Part B              | On change   | Ship type, dimensions for Class B              |
| 127233 | 0x1F109| Man Overboard Notification (MOB)              | On event    | MOB position and device info                   |

### Equipment Control & Status

| PGN    | Hex    | Name                                          | Update Rate | Description                                    |
|--------|--------|-----------------------------------------------|-------------|------------------------------------------------|
| 128776 | 0x1F708| Windlass Control Status                       | 250 ms      | Windlass commands (up/down/stop)               |
| 128777 | 0x1F709| Windlass Operating Status                     | 1 Hz        | Windlass motion status and direction           |
| 128778 | 0x1F70A| Windlass Monitoring Status                    | 1 Hz        | Rode count, speed, tension                     |
| 127501 | 0x1F20D| Binary Status Report                          | On change   | General on/off status indicators               |
| 130576 | 0x1FE10| Trim Tab Status                               | 1 Hz        | Trim tab position                              |
| 130577 | 0x1FE11| Direction Data                                | 10 Hz       | Directional data for various sources           |

### System & Network Management

| PGN    | Hex    | Name                                          | Update Rate | Description                                    |
|--------|--------|-----------------------------------------------|-------------|------------------------------------------------|
| 059392 | 0xE800 | ISO Acknowledgment                            | On demand   | Acknowledge receipt of message                 |
| 059904 | 0xEA00 | ISO Request                                   | On demand   | Request specific PGN from device               |
| 060928 | 0xEE00 | ISO Address Claim                             | On change   | Claim network address during initialization    |
| 126208 | 0x1ED00| NMEA Command/Request/Acknowledge Group        | On demand   | Group function commands                        |
| 126464 | 0x1EE00| Transmit/Receive PGN List Group Function      | On demand   | Query or report supported PGNs                 |
| 126992 | 0x1F010| System Time                                   | 1 s         | UTC date and time                              |
| 126996 | 0x1F014| Product Information                           | On demand   | Device name, model, serial, firmware           |
| 126998 | 0x1F016| Configuration Information                     | On change   | Installation description fields                |

---

## PGN Categories

### Fast Packet PGNs

These PGNs use the NMEA 2000 Fast Packet protocol to send data larger than 8 bytes:

- 129029 (GNSS Position Data)
- 129038-129041 (AIS Reports)
- 129283-129285 (Navigation Data)
- 129794, 129809-129810 (AIS Static Data)
- 126208, 126464, 126996, 126998 (System PGNs)

### Single Frame PGNs

These PGNs fit within a single 8-byte CAN frame:

- 127250 (Vessel Heading)
- 127488 (Engine Rapid Update)
- 129025 (Position Rapid Update)
- 129026 (COG & SOG Rapid Update)
- 128259 (Water Speed)
- 128267 (Water Depth)
- 130306 (Wind Data)

### Priority Levels

| Priority | Usage                  | Examples                           |
|----------|------------------------|------------------------------------|
| 0-1      | Emergency              | MOB, collision alarm               |
| 2        | Safety Critical        | Heading, position rapid updates    |
| 3        | Control                | Autopilot commands                 |
| 4-5      | Navigation             | COG/SOG, navigation data           |
| 6        | Standard               | Most sensor data                   |
| 7        | Low Priority           | Configuration, static data         |

---

## Implementation Notes

### For N2K Simulator

Currently implemented:
- ✅ `129029` - GNSS Position Data (via NMEA 0183 $GPGGA conversion)

Planned for Additional PGNs widget:
- ⏳ `128267` - Water Depth
- ⏳ `128259` - Water Speed
- ⏳ `130316` - Water Temperature

Recommended additions:
- `127250` - Vessel Heading (for compass accuracy)
- `130306` - Wind Data (already simulated, needs PGN encoding)
- `127508` - Battery Status (useful for embedded device simulation)
- `127488` - Engine Parameters (for powerboat scenarios)

### Data Types & Units

Common NMEA 2000 data types:
- **Position:** Degrees (latitude/longitude) × 10^-7
- **Speed:** m/s × 0.01
- **Distance:** m × 0.01 or m × 1
- **Temperature:** K × 0.01 (Kelvin, 0°C = 273.15K)
- **Pressure:** Pa × 1 (Pascals)
- **Angle:** Radians × 0.0001 or degrees × 1
- **Voltage:** V × 0.01
- **Current:** A × 0.01

### Update Rates

Recommended transmission rates based on NMEA 2000 standards:
- **Rapid updates (5-10 Hz):** Position, heading, COG/SOG
- **Standard rate (1 Hz):** Depth, speed, navigation data
- **Slow rate (2-5 s):** Environmental, tank levels, battery status
- **On change:** Static data, configuration, alarms

### Device Addressing

NMEA 2000 devices must:
1. Claim a unique address (0-251) on initialization
2. Support ISO Address Claim (PGN 060928)
3. Respond to ISO Requests (PGN 059904)
4. Send Product Information (PGN 126996) when requested

---

## References

### Official Documentation
- [Garmin Technical Reference for NMEA 2000 Products](https://www8.garmin.com/manuals/webhelp/GUID-1415AAD0-FE63-42A6-8F8D-DB713D616122/EN-US/Technical_Reference_for_Garmin_NMEA_2000_Products_EN-US.pdf)
- [Garmin GPSMAP 8400/8600 NMEA 2000 PGN Information](https://www8.garmin.com/manuals/webhelp/gpsmap8400-8600/EN-US/GUID-0C4B3FAB-3E41-438C-B31E-9B5489790913.html)
- [Garmin Support - NMEA 2000 PGN Data](https://support.garmin.com/en-US/?faq=DX2m95oWWF4z4bFpNKXRA8)

### Open Source Resources
- [NMEA2000 Library - PGN List](https://ttlappalainen.github.io/NMEA2000/list_msg.html)
- [NMEA 2000 PGN Deciphered](https://endige.com/2050/nmea-2000-pgns-deciphered/)
- [Continuous Wave - NMEA 2000 PGN Reference](https://continuouswave.com/whaler/reference/PGN.html)

### Standards Organizations
- [NMEA (National Marine Electronics Association)](https://web.nmea.org/)
- [ISO 11783 / SAE J1939](https://www.iso.org/standard/57556.html) - Base CAN protocol

---

## Garmin Product Clarification

**Note:** No "MC10" product was found in Garmin's marine lineup. Similar products include:

- **MSC 10** - Marine Satellite Compass (heading sensor)
- **GMI 10** - Marine Instrument Display (multifunction display)
- **GHC 10** - Marine Autopilot Control Unit (helm control)
- **GHP 10** - Complete autopilot system (includes CCU)

If you have the specific model number, please provide it for accurate PGN documentation.

---

## Revision History

| Version | Date       | Author          | Changes                                    |
|---------|------------|-----------------|-------------------------------------------|
| 0.1.0   | 2025-12-02 | Colin Bitterfield | Initial document creation with comprehensive PGN enumeration |

---

**Document Status:** Draft for Review
**Next Review Date:** TBD after initial review

---

*This document is part of the Anchor Drag Alarm System project.*
*Last updated: 2025-12-02*
