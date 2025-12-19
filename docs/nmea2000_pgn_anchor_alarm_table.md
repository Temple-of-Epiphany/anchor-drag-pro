# NMEA 2000 PGN Reference for Anchor Alarm

## Version 1.0.0 - December 14, 2025

**Author:** Colin Bitterfield
**Email:** colin@bitterfield.com
**Project:** ANCHOR-ALARM

---

## Overview

This document lists the NMEA 2000 Parameter Group Numbers (PGNs) used by the Anchor Drag Alarm system for GPS position, compass heading, and sensor data.

---

## GPS Position PGNs

### PGN 129029 - GNSS Position Data

**Primary GPS source for anchor position monitoring**

| Field | Description | Resolution | Units |
|-------|-------------|------------|-------|
| SID | Sequence ID | 1 | - |
| Date | Days since 1970-01-01 | 1 | days |
| Time | Seconds since midnight | 0.0001 | s |
| Latitude | Position latitude | 1e-7 | degrees |
| Longitude | Position longitude | 1e-7 | degrees |
| Altitude | Height above ellipsoid | 0.01 | m |
| GNSS type | GPS/GLONASS/GALILEO | - | enum |
| Method | Fix type | - | enum |
| Integrity | Quality indicator | - | enum |
| Number of SVs | Satellites in view | 1 | count |
| HDOP | Horizontal dilution | 0.01 | - |
| PDOP | Position dilution | 0.01 | - |
| Geoidal Separation | Geoid height | 0.01 | m |

**Default Device ID:** 0x00 (first GPS on bus)

---

### PGN 129025 - Position, Rapid Update

**Fast-update GPS position (alternative to 129029)**

| Field | Description | Resolution | Units |
|-------|-------------|------------|-------|
| Latitude | Position latitude | 1e-7 | degrees |
| Longitude | Position longitude | 1e-7 | degrees |

**Update Rate:** 100ms (10 Hz)  
**Default Device ID:** 0x00 (first GPS on bus)

---

## Compass/Heading PGNs

### PGN 127250 - Vessel Heading

**Primary compass source for boat orientation**

| Field | Description | Resolution | Units |
|-------|-------------|------------|-------|
| SID | Sequence ID | 1 | - |
| Heading | True or magnetic heading | 0.0001 | radians |
| Deviation | Magnetic deviation | 0.0001 | radians |
| Variation | Magnetic variation | 0.0001 | radians |
| Reference | Magnetic/True | - | enum |

**Default Device ID:** 0x01 (compass)

---

### PGN 127251 - Rate of Turn

**Compass turn rate (optional)**

| Field | Description | Resolution | Units |
|-------|-------------|------------|-------|
| SID | Sequence ID | 1 | - |
| Rate of Turn | Angular velocity | 1e-6 | rad/s |

**Default Device ID:** 0x01 (compass)

---

## Additional Sensor PGNs

### PGN 130306 - Wind Data

**Wind speed and direction (optional enhancement)**

| Field | Description | Resolution | Units |
|-------|-------------|------------|-------|
| SID | Sequence ID | 1 | - |
| Wind Speed | Wind velocity | 0.01 | m/s |
| Wind Angle | Relative to bow | 0.0001 | radians |
| Reference | True/Apparent | - | enum |

**Use Case:** Display wind conditions on INFO screen

---

### PGN 128267 - Water Depth

**Depth sounder data (optional enhancement)**

| Field | Description | Resolution | Units |
|-------|-------------|------------|-------|
| SID | Sequence ID | 1 | - |
| Depth | Water depth | 0.01 | m |
| Offset | Transducer offset | 0.001 | m |
| Range | Maximum range | 0.1 | m |

**Use Case:** Anchor drag correlation with depth changes

---

### PGN 127508 - Battery Status

**DC power monitoring (optional enhancement)**

| Field | Description | Resolution | Units |
|-------|-------------|------------|-------|
| Battery Instance | Battery number | 1 | - |
| Voltage | DC voltage | 0.01 | V |
| Current | DC current | 0.1 | A |
| Temperature | Battery temp | 0.01 | K |
| SID | Sequence ID | 1 | - |

**Use Case:** Low battery warning on INFO screen

---

## Configuration Guidelines

### GPS Source Configuration

**Option 1: Auto-detect (Default)**
- Device ID: 0x00
- PGN: 129029 (preferred) or 129025 (fallback)
- System scans for first available GPS on N2K bus

**Option 2: Manual Selection**
- User specifies Device ID (0x00-0xFF)
- User specifies PGN (129029 or 129025)
- System listens only to specified source

### Compass Source Configuration

**Option 1: Auto-detect (Default)**
- Device ID: 0x01
- PGN: 127250
- System scans for first available compass

**Option 2: Manual Selection**
- User specifies Device ID (0x00-0xFF)
- User specifies PGN (127250 or 127251)
- System listens only to specified source

---

## Device ID Assignment

Common device IDs on NMEA 2000 networks:

| Device ID | Typical Assignment |
|-----------|-------------------|
| 0x00 | Primary GPS/GNSS receiver |
| 0x01 | Magnetic compass/heading sensor |
| 0x02 | Wind transducer |
| 0x03 | Depth sounder |
| 0x04 | Speed sensor |
| 0x05-0x0F | Additional sensors |
| 0x10-0xFF | User-assigned devices |

---

## NMEA 0183 Equivalent Sentences

For systems using NMEA 0183 instead of N2K:

| N2K PGN | NMEA 0183 Sentence | Description |
|---------|-------------------|-------------|
| 129029 | $GPGGA | GPS fix data |
| 129029 | $GPRMC | Recommended minimum |
| 127250 | $HCHDM | Magnetic heading |
| 127250 | $HCHDT | True heading |
| 130306 | $WIMWV | Wind speed/angle |
| 128267 | $SDDBT | Depth below transducer |

---

## References

- **NMEA 2000 Standard:** IEC 61162-3
- **Garmin PGN Reference:** https://www8.garmin.com/manuals/webhelp/GUID-329B065A-1FBA-47E7-ACDE-EA9C4AF4FB98/EN-US/NMEA_2000_GUID-0DABBB2C-8C2C-44BC-831D-2D8A4C7FD8BE.html
- **NMEA 0183 Standard:** IEC 61162-1

---

**End of Document**
