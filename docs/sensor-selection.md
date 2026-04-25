# Sensor Selection — IMU and GPS for the Anchor Drag Pro Product Family

**Version:** 0.1.0
**Author:** Colin Bitterfield
**Email:** colin@bitterfield.com
**Date Created:** 2026-04-25
**Date Updated:** 2026-04-25
**Status:** Draft / Decision Pending Confirmation
**Related epics:** #34 (Workstream 5 — IMU integration)

## Purpose

This document captures the engineering analysis behind sensor selection for the Anchor Drag Pro product family — specifically which IMU and GPS components feed the anchor-drag detection algorithm, and how they connect to the device.

The decision affects:
- Algorithm accuracy (false-alarm rate, drag-detection latency)
- Bill-of-materials cost
- Product differentiation between the Display ($150) and IMU/GPS ($250+) variants
- Marketing claims about heading data on N2K

## Product variants and their sensor needs

| Variant | Price | GPS source | IMU source | Heading on N2K? |
|---|---|---|---|---|
| **Anchor Drag Display** | $150 | External — boat's existing N2K GPS, or YachtDevices gateway | None (algorithm degrades to GPS-only swing detection) | Consumes if present, doesn't transmit |
| **Anchor Drag IMU/GPS** | $250+ | Internal | Internal | **Transmits to N2K** as heading source for the rest of the boat |

The IMU/GPS variant's marketing differentiator is twofold:
1. Standalone — works on any boat without N2K instrumentation
2. Adds professional heading data to the N2K bus (replaces $400-800 dedicated heading sensors like Garmin SteadyCast, B&G Precision-9, Maretron SSC200)

## Requirements for the IMU sensor

| Requirement | Why it matters |
|---|---|
| Stable heading over multi-hour anchor watches | Anchor watches run overnight; gyro-only heading drifts unusably over 4-8 hours |
| Sensor fusion (no host AHRS code) | Avoid implementing Madgwick/Mahory in firmware; use chip's onboard fusion |
| ≤5° absolute heading accuracy after calibration | Adequate for swing-vs-drag discrimination; not autopilot-grade |
| Pitch/roll for sea-state / impact detection | Useful for logging and storm-condition awareness |
| Calibration that survives reboot | Magnetometer calibration is in-place; must persist |
| Magnetic environment tolerance | Boat is magnetically hostile (engine, batteries, wiring) |
| Low-power, marine temperature range | Marine devices run unattended in hot/cold cabins |

## Candidates evaluated

### Option A — WitMotion WTGAHRS3

Combined GPS + 6-axis IMU in a single RS485/TTL module.

| Spec | Value |
|---|---|
| Sensors | 3-axis accel (±16g), 3-axis gyro (±500°/s), GNSS (multi-constellation) |
| **Magnetometer** | **None** |
| Heading source | Gyro integration only — drifts ~1-3°/hour |
| Attitude accuracy | 0.05° static / 0.1° dynamic |
| GPS update rate | 1 Hz default (up to 10 Hz selectable) |
| Output | RS485 (MODBUS), TTL, RS232; GPS variant also outputs NMEA 0183 |
| Approx cost | ~$120-150 |

**Verdict: Rejected for IMU/GPS variant.** No magnetometer means heading is gyro-only and drifts unusably over an overnight watch. Could work for short stops (lunch hook) but the algorithm value-add is degraded for the use case that matters most.

### Option B — WitMotion HWT901B (RS485 variant)

9-axis IMU with high-grade RM3100 magnetometer; **no GPS**.

| Spec | Value |
|---|---|
| Sensors | 3-axis accel, 3-axis gyro, **3-axis magnetometer (RM3100)**, barometer |
| Magnetometer chip | RM3100 — scientific-grade, ~30 nT RMS noise (vs ~600 nT for typical consumer chips) |
| Heading accuracy | ~0.5-1° magnetic, **stable indefinitely** |
| Attitude accuracy | 0.1° static |
| GPS | None — must be paired with separate source |
| Output | RS485 (MODBUS), TTL, RS232, CAN (proprietary protocol — not N2K) |
| Update rate | Up to 200 Hz |
| Environmental | IP67/IP68 |
| Approx cost | ~$80-120 |

**Verdict: Strong candidate for IMU/GPS variant.** RM3100 is the differentiating component — scientific-grade magnetometer eliminates the heading-drift problem. Requires pairing with a separate GPS module.

### Option C — Bosch BNO085 (chip on PCB)

9-axis IMU with onboard sensor fusion, designed for product integration.

| Spec | Value |
|---|---|
| Sensors | 3-axis accel, 3-axis gyro, 3-axis magnetometer (consumer-grade) |
| Sensor fusion | Onboard (Bosch BSX) — outputs quaternion, Euler, linear accel directly |
| Heading accuracy | ~2-3° magnetic after in-place calibration |
| Attitude accuracy | ~1° dynamic |
| GPS | None — must be paired with separate source |
| Output | I²C / SPI / UART |
| Update rate | Up to 100 Hz |
| Form factor | Bare chip + breakout board (requires PCB integration) |
| Approx cost | ~$15-25 |

**Verdict: Acceptable but not premium.** Adequate magnetometer for anchor alarm; much cheaper. Sensor fusion is excellent (Bosch BSX is best-in-class). Lower-quality magnetometer is the only meaningful gap vs HWT901B.

## GPS module options

The IMU/GPS variant needs an internal GPS regardless of which IMU is chosen (HWT901B or BNO085 — neither has GPS).

| GPS module | Notes | Approx cost |
|---|---|---|
| **u-blox NEO-M9N** | Multi-GNSS (GPS, GLONASS, Galileo, BeiDou, QZSS), ~1.5 m CEP, well-supported | ~$30 |
| u-blox NEO-7M / NEO-8M | Older, single-GNSS, ~2.5 m CEP | ~$15 |
| Quectel L80 | Low-cost integrated patch antenna | ~$10 |

**Selection: u-blox NEO-M9N.** Multi-GNSS support, best-in-class accuracy in the price tier, well-documented, abundant community resources.

## Decision

### IMU/GPS variant ($250) — primary BOM
**HWT901B (RS485) + u-blox NEO-M9N**

- HWT901B externally mounted (clean magnetic environment, away from LCD backlight and ESP32 EMI)
- RS485 cable run from HWT901B to main board (long runs supported, marine-friendly wiring)
- u-blox NEO-M9N on the main board with external GPS antenna connector (SMA) for masthead/arch mounting
- BOM cost: ~$80 (HWT901B) + ~$30 (u-blox) + cabling = ~$120 in sensor components

### Display variant ($150) — sensor BOM
**No internal IMU or GPS.** Consumes external N2K data only.

### Premium tier (deferred — possible v1.0+)
Same HWT901B + u-blox stack but with:
- Higher-grade enclosure
- Pre-calibrated magnetometer for vessel
- Extended N2K transmission (PGN 127251 rate-of-turn, 130306 wind if connected, etc.)

## Architecture: RS485 → ESP32 → N2K bridging

The IMU/GPS variant's firmware role:

```
HWT901B  ──RS485 (MODBUS)──▶  ESP32  ──TWAI/N2K──▶  N2K bus
                                │
                                ├──▶  Anchor algorithm (uses heading internally)
                                ├──▶  Touchscreen UI (heading display)
                                └──▶  PGN 127250 transmission (heading to other boat instruments)
```

### N2K PGNs the device transmits (IMU/GPS variant only)

| PGN | Name | Cadence | Source data |
|---|---|---|---|
| 60928 | ISO Address Claim | At boot + on conflict | Required for bus citizenship |
| 126996 | Product Information | On request | Static product info |
| 126998 | Configuration Information | On request | Static config strings |
| 127250 | Vessel Heading | 10 Hz | HWT901B magnetic heading |
| 127251 | Rate of Turn | 10 Hz | HWT901B gyro Z-axis |
| 127257 | Attitude | 5 Hz | HWT901B pitch/roll/yaw |
| 129025 | Position — Rapid | 10 Hz | u-blox |
| 129026 | COG/SOG — Rapid | 10 Hz | u-blox |
| 129029 | GNSS Position Data | 1 Hz | u-blox |
| 129539 | GNSS DOPs | 1 Hz | u-blox |

### Display variant — TWAI listen-only

Display variant runs TWAI in `TWAI_MODE_LISTEN_ONLY` — it cannot transmit. Compile-time flag:

```c
#define VARIANT_DISPLAY    1
#define VARIANT_IMU_GPS    2

#if PRODUCT_VARIANT == VARIANT_IMU_GPS
  #define HAS_RS485_IMU         1
  #define HAS_INTERNAL_GPS      1
  #define HAS_N2K_TRANSMIT      1
  #define HAS_ISO_ADDR_CLAIM    1
#else
  #define HAS_RS485_IMU         0
  #define HAS_INTERNAL_GPS      0
  #define HAS_N2K_TRANSMIT      0
  #define HAS_ISO_ADDR_CLAIM    0
#endif
```

One firmware codebase, two product builds.

## Heading source priority (algorithm input)

The anchor algorithm uses whichever heading source is highest-priority and recent:

```
Priority 1: PGN 127250 from N2K bus (CCU / MSC10 / Garmin SteadyCast / our own transmission if present)
Priority 2: Internal HWT901B (IMU/GPS variant only)
Priority 3: GPS COG (only when SOG > 1 kt — meaningless when stationary)
Priority 4: No heading (algorithm degrades to GPS-only swing detection)
```

For the IMU/GPS variant in standalone use, internal HWT901B is the source. For the IMU/GPS variant on a boat with an existing CCU, the external CCU could win on priority — but typically internal HWT901B is more recent and lower-latency, so realistic priority comparison is by recency, not source.

## Accuracy comparison summary

For an 8-hour overnight anchor watch:

| | WTGAHRS3 (rejected) | HWT901B + u-blox (selected) | BNO085 + u-blox (alternate) | Garmin HVS via N2K |
|---|---|---|---|---|
| Position accuracy | ~3 m | ~1.5 m | ~1.5 m | ~1 m |
| Heading drift after 8 hrs | unusable | <1° | ~2-3° | <1° |
| Heading absolute reference | none | RM3100 magnetometer | consumer magnetometer | fluxgate compass |
| Attitude accuracy | 0.1° | 0.1° | ~1° | ~0.5° |
| Cost (BOM) | $130 | $110 | $50 | $400-500 (retail) |
| Form factor | One box | Two parts (HWT901B external) | One PCB | External separate device |

HWT901B+u-blox lands close to Garmin HVS on accuracy at a fraction of the cost.

## Mounting considerations

### Magnetometer placement (HWT901B)

The RM3100 chip's accuracy depends entirely on its magnetic environment. Mounting recommendations:

- **Minimum 50 cm** from any DC motor, large iron mass, or switching power supply
- **Minimum 30 cm** from the LCD backlight assembly (the WaveShare 4.3B uses a switching backlight driver that emits magnetic noise)
- **Minimum 30 cm** from the ESP32 itself and any active high-current circuitry
- Best practice: mount HWT901B in its own enclosure, install on a non-ferrous bulkhead away from the helm electronics
- RS485 supports cable runs of tens of meters at 115200 baud, hundreds of meters at lower baud rates

### GPS antenna placement (u-blox NEO-M9N)

GPS performance depends on sky view far more than chip choice:

| Mount location | Typical real-world accuracy |
|---|---|
| Masthead | Best, but moves with rig |
| Stern arch with clear sky | Excellent |
| Hardtop / dodger | Good |
| Inside cabin (touchscreen-mounted) | Worst — multipath and signal attenuation |

The IMU/GPS variant should ship with an external GPS antenna on a 5 m SMA cable, mounted by the customer at a clear-sky location. The main device installs at the helm.

## Engineering work required

For the IMU/GPS variant (vs the Display variant which is firmware-simpler):

| Component | Estimated effort |
|---|---|
| RS485 driver + WitMotion MODBUS protocol parser | 2-3 days |
| u-blox NEO-M9N driver (UBX protocol or NMEA passthrough) | 1-2 days |
| `attitude_sample_t` integration in `core/` (already planned for BNO085) | (no extra) |
| Switch TWAI from LISTEN_ONLY to NORMAL mode for IMU/GPS variant | 0.5 day |
| ISO Address Claim protocol (PGN 60928) | 3-5 days |
| Product Info responses (PGN 126996, 126998) | 1 day |
| Periodic transmission task for heading PGNs | 1 day |
| Bus arbitration / source address conflict handling | 1-2 days |
| End-to-end test on real N2K bus with chartplotter and autopilot consumers | 2-3 days |
| **Total** | **~12-18 days** |

## Decisions deferred

1. **Single-firmware variant flag vs separate firmware builds** — leaning single-firmware with `PRODUCT_VARIANT` compile-time flag (cleaner). To be confirmed.
2. **u-blox NEO-M9N vs alternative GPS chip** — current selection based on accuracy/cost. Evaluate antenna kit options when prototype hardware ordered.
3. **Premium tier ($350+) with HWT901B Pro features** — possibility for v1.0+; not v0.2.
4. **NMEA 2000 certification** — defer indefinitely. Industry norm for small vendors is "NMEA 2000 compatible" without formal certification (Yacht Devices, Maretron uncertified product lines, etc.). Cost is ~$5K + lab time + membership; not justified for v0.2 scope.

## Marketing implications

The IMU/GPS variant gains a meaningful third product pitch beyond just "anchor alarm with internal sensors":

> **Anchor Drag IMU/GPS — $250**
> - Standalone anchor alarm (no boat instrumentation required)
> - Adds professional heading data to your NMEA 2000 network
> - Compatible with chartplotter, autopilot, and AIS for heading reference
> - Replaces $400-800 dedicated heading sensors at a fraction of the price

Without N2K transmission, the IMU/GPS variant is just "Display with built-in GPS" — harder to justify the $100 markup over the Display variant. With it, the value proposition is clear and competitive.

## References

- Yacht Devices YDWG-02 documentation: https://www.yachtd.com/products/wifi_gateway.html
- WitMotion HWT901B product page: https://witmotion-sensor.com/products/military-grade-accelerometer-inclinometer-hwt901b-mpu9250-9-axis-gyroscope-anglexy-0-05-accuracy-digital-compass-air-pressure-altitude-rm3100-magnetometer-compensation-and-kalman-filtering
- u-blox NEO-M9N datasheet: https://www.u-blox.com/en/product/neo-m9n
- NMEA 2000 PGN 127250 (Vessel Heading): standard reference
- CANBoat library (open-source N2K parsing): https://github.com/canboat/canboat
- Original review of WitMotion devices: this document, sections "Candidates evaluated"

## Change log

- **0.1.0** (2026-04-25): Initial draft capturing comparison of WTGAHRS3, HWT901B, and BNO085 candidates. Selection: HWT901B + u-blox NEO-M9N for IMU/GPS variant, no internal sensors for Display variant. Architecture: RS485 sensor link with N2K transmission of heading PGNs.
