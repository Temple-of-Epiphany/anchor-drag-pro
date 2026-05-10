# Configuration Schema — `config.toml` on SD card

**Version:** 0.1.0
**Status:** Draft (specification — implementation per Workstream 4)
**Date:** 2026-04-26
**Author:** Colin Bitterfield <colin@bitterfield.com>

This document specifies the human-readable configuration file consumed by the Anchor Drag Pro firmware. The file lives at `/anchor/config.toml` on the SD card. See [`docs/config.example.toml`](./config.example.toml) for a complete annotated sample.

## Goals

- **Human-editable on a laptop** — TOML format, comments preserved, no special tooling required
- **Portable across devices** — copy a config card from one boat to another and it works
- **Customer-tunable without code changes** — distance presets, units, source preferences
- **Safe by default** — invalid values raise warnings but never block boot
- **Forward-compatible** — unknown fields are ignored, not rejected

## Three-tier resolution

Settings are resolved top-down at boot. Each layer fills in fields the layer above didn't provide.

```
┌──────────────────────────────────────────────┐
│  1. SD card config.toml  (if present + valid) │  primary
├──────────────────────────────────────────────┤
│  2. NVS cache             (last-known-good)   │  fallback
├──────────────────────────────────────────────┤
│  3. Built-in defaults     (compiled in)       │  always present
└──────────────────────────────────────────────┘
```

Properties this gives:
- **Device always boots** — defaults guarantee something usable
- **NVS as cache** — pull SD card mid-trip, device keeps running with last known config; reinsert and SD wins again
- **Web UI writes both** — saves to SD (if present) AND updates NVS cache so changes survive card removal

## Validation philosophy

- **Invalid values are warnings, not errors.** Bad config triggers a persistent banner on the touchscreen and a flag in the web UI; the value falls back to firmware default; boot proceeds.
- **Unit-tagged values are required for distances.** No bare numbers (`50` rejected; `"50ft"` or `"15m"` accepted).
- **Each setting has a hardcoded `setting_bounds_t`** in firmware defining safety floor/ceiling — even valid syntax can't escape the bound.
- **Unknown keys are ignored.** Future-proof: a v0.3 config file works on v0.2 firmware (extra sections silently skipped).

## Schema

### `[device]`

```toml
[device]
name = "Hylas 51"
metric = false
```

| Field | Type | Default | Notes |
|---|---|---|---|
| `name` | string | `""` | Boat name. Free text. Displayed in web UI title bar; never exposed in firmware logic. |
| `metric` | bool | `false` | `true` = display values in meters; `false` = display in feet. Customer-facing setting. |

### `[display]`

```toml
[display]
rotation = 0
brightness = 80
```

| Field | Type | Default | Allowed | Notes |
|---|---|---|---|---|
| `rotation` | integer | `0` | `0`, `90`, `180`, `270` | Mount orientation (degrees clockwise from native). Applied via `lv_disp_set_rotation()`. |
| `brightness` | integer | `80` | `0`-`100` | Backlight percent, applied at boot via CH422G EXIO2 PWM. |

### `[anchor]`

```toml
[anchor]
alarm_distance = "50ft"
arming_seconds = 60
sound_volume = "medium"

[anchor.options]
distances = ["25ft", "50ft", "100ft"]
```

| Field | Type | Default | Notes |
|---|---|---|---|
| `alarm_distance` | unit-tagged string | `"50ft"` | Must be one of `[anchor.options.distances]`. Locked while in ARMED/ALARM/MUTED. |
| `arming_seconds` | integer | `60` | Range 30-300. Time spent collecting GPS samples to compute anchor center. |
| `sound_volume` | enum | `"medium"` | `"low"`, `"medium"`, or `"high"`. Maps to PWM duty cycle for buzzer. |

#### `[anchor.options]`

| Field | Type | Default | Notes |
|---|---|---|---|
| `distances` | list of 3 unit-tagged strings | `["25ft", "50ft", "100ft"]` | Exactly 3 entries. All must use the same unit (homogeneous). Each within `1.5 m ≤ value ≤ 152 m` (≈ 5–500 ft). Values stored at 1-decimal precision. Reject mixed units, duplicates, malformed strings. |

**Unit-tagged string format:** `<number><optional space><unit>`, where unit is `ft` or `m` (case-insensitive). Examples: `"50ft"`, `"15.5m"`, `"50 ft"`. Bare numbers (`50`) are rejected. See [`glossary.md`](https://github.com/Temple-of-Epiphany/anchor-drag-pro-meta/blob/main/docs/glossary.md) for unit handling rules.

### `[gps]` — cross-transport priority

```toml
[gps]
source_priority = ["n2k", "url", "serial", "internal"]
freshness_seconds = 5
```

| Field | Type | Default | Notes |
|---|---|---|---|
| `source_priority` | list of strings | `["n2k", "url", "serial", "internal"]` | Order in which to prefer GPS sources when multiple have valid recent fixes. First with a recent fix wins. Sources not listed are not consulted. |
| `freshness_seconds` | integer | `5` | A fix older than this is considered stale and skipped in favor of next priority. |

### `[gps.n2k]` — NMEA 2000 GPS source

```toml
[gps.n2k]
enabled = true
source_address = 0xff
fallback_address = 0xff
```

| Field | Type | Default | Notes |
|---|---|---|---|
| `enabled` | bool | `true` | Whether to consume N2K GPS at all. |
| `source_address` | integer | `0xff` (auto) | N2K source address (0-255) of the GPS device to use. **Critical: most boats have multiple N2K GPS sources** (chartplotter, AIS, dedicated antenna). `0xff` = auto-detect first available; otherwise lock to a specific source. Set via web UI / CLI source picker after device discovers what's on the bus. |
| `fallback_address` | integer | `0xff` (none) | If primary source goes silent for `freshness_seconds`, fall back to this source. Set to `0xff` to disable fallback. |

**Source discovery:** the device reads PGN 126996 (Product Info) from each transmitter to display friendly identifiers in the web UI / CLI ("0x07 — Garmin GPS24xd HVS"). Customer picks; the source address is what's persisted.

### `[gps.serial]` — RS485 / serial GPS source

```toml
[gps.serial]
enabled = false
protocol = "nmea0183"
baud = 4800
```

| Field | Type | Default | Notes |
|---|---|---|---|
| `enabled` | bool | `false` | |
| `protocol` | enum | `"nmea0183"` | `"nmea0183"` (most common for marine GPS receivers) or `"ublox_ubx"` (binary u-blox protocol). |
| `baud` | integer | `4800` | Common: `4800` (NMEA 0183 standard), `9600`, `38400`, `115200`. Validate against driver-supported list. |

Wired on RS485 (GPIO43 RX / GPIO44 TX with built-in transceiver). Sentences parsed: GGA, RMC, VTG.

### `[gps.url]` — network gateway GPS source

```toml
[gps.url]
enabled = false
url = "tcp://192.168.1.100:1457"
protocol = "yd_raw"
reconnect_seconds = 10
```

| Field | Type | Default | Notes |
|---|---|---|---|
| `enabled` | bool | `false` | |
| `url` | string | `""` | Format: `tcp://<host>:<port>`. Future: `mdns://anchor-gateway.local:1457` for Bonjour-discovered gateways. |
| `protocol` | enum | `"yd_raw"` | `"yd_raw"` (Yacht Devices RAW format — N2K text frames over TCP) or `"nmea0183"` (ASCII sentences). |
| `reconnect_seconds` | integer | `10` | Delay before reconnect attempt after disconnect. Exponential backoff caps at 60s. |

For YD RAW mode, the gateway must be configured in RAW mode (not 0183 mode) on its admin page. See [`docs/sensor-selection.md`](./sensor-selection.md) for gateway compatibility.

### `[gps.internal]` — built-in GPS receiver

```toml
[gps.internal]
enabled = false
```

| Field | Type | Default | Notes |
|---|---|---|---|
| `enabled` | bool | `false` | IMU/GPS variant only. Display variant ignores this section. Auto-detected at boot when `PRODUCT_VARIANT=IMU_GPS`. |

Hardware: u-blox NEO-M9N or equivalent on UART; antenna on external SMA. See [`docs/sensor-selection.md`](./sensor-selection.md).

### `[imu]` — heading source priority

Same shape as `[gps]`. Three external sources + one internal:

```toml
[imu]
source_priority = ["n2k", "internal", "serial"]
freshness_seconds = 2

[imu.n2k]
enabled = true
source_address = 0xff
fallback_address = 0xff

[imu.serial]
enabled = false
protocol = "witmotion_modbus"
baud = 115200

[imu.internal]
enabled = false
chip = "auto"
```

| Section | Field | Notes |
|---|---|---|
| `[imu]` | `source_priority` / `freshness_seconds` | Same semantics as `[gps]`. Default freshness shorter (heading staleness matters more for swing detection). |
| `[imu.n2k]` | `enabled` / `source_address` / `fallback_address` | Reads PGN 127250 (Vessel Heading), 127251 (Rate of Turn), 127257 (Attitude). Same source-address selection logic as N2K GPS. |
| `[imu.serial]` | `enabled` / `protocol` / `baud` | `protocol`: `"witmotion_modbus"` (HWT901B) — others added when supported. |
| `[imu.internal]` | `enabled` / `chip` | IMU/GPS variant only. `chip`: `"auto"` (detect at I²C scan), `"bno085"`, `"hwt901b_i2c"`. |

### `[wifi]`

```toml
[wifi]
mode = "ap"

[wifi.ap]
ssid_prefix = "AnchorAlarm"
password = ""

[wifi.sta]
ssid = ""
password = ""
```

| Field | Type | Default | Notes |
|---|---|---|---|
| `mode` | enum | `"ap"` | `"ap"` = device runs its own access point (default for first-time setup); `"sta"` = device joins an existing network; `"off"` = disabled. |
| `[wifi.ap].ssid_prefix` | string | `"AnchorAlarm"` | Device serial number is appended (e.g., `AnchorAlarm-A1B2`). |
| `[wifi.ap].password` | string | empty | Empty = use auto-generated WPA2 password printed on device label. |
| `[wifi.sta].ssid` | string | `""` | Joined network when `mode = "sta"`. |
| `[wifi.sta].password` | string | `""` | Stored in NVS, encrypted at rest where supported. |

### `[n2k_transmit]` — IMU/GPS variant only

```toml
[n2k_transmit]
enabled = false
heading_hz = 10
attitude_hz = 5
rate_of_turn_hz = 10
```

| Field | Type | Default | Notes |
|---|---|---|---|
| `enabled` | bool | `false` | Transmit synthesized PGNs from internal IMU onto the N2K bus. **Display variant ignores this** (TWAI is `LISTEN_ONLY` per prereq #40). |
| `heading_hz` | integer | `10` | PGN 127250 cadence (Hz). |
| `attitude_hz` | integer | `5` | PGN 127257 cadence. |
| `rate_of_turn_hz` | integer | `10` | PGN 127251 cadence. |

Activating this requires Workstream 5 ISO Address Claim implementation. The variant compile flag `PRODUCT_VARIANT=IMU_GPS` is the gate.

### `[logger]` — N2K data logger

```toml
[logger]
enabled = false
format = "yd_raw"
rotation = "hourly"
pgns = "default"
min_free_pct = 5
```

| Field | Type | Default | Notes |
|---|---|---|---|
| `enabled` | bool | `false` | |
| `format` | enum | `"yd_raw"` | `"yd_raw"` (Yacht Devices text format) or `"actisense"` (binary). |
| `rotation` | enum | `"hourly"` | `"hourly"`, `"daily"`, or `"size:8MB"`. |
| `pgns` | enum or list | `"default"` | `"default"` (curated list — GPS, wind, depth, heading, engine), `"all"` (high SD wear), or explicit list `[129029, 130306, 128267]`. |
| `min_free_pct` | integer | `5` | Stop logging when SD free space drops below this percentage. Alarm path keeps working. |

### `[locale]` — future expansion

```toml
[locale]
language = "en"
timezone = "America/Los_Angeles"
```

Reserved. Single-language English ships in v0.2. Structure permits multilingual (`"es"`, `"fr"`, etc.) without schema migration.

## Atomic write protocol

When the device writes config (e.g., user saves from web UI):

1. Write to `config.toml.tmp`
2. `f_sync()` to flush
3. Rename `config.toml.tmp` → `config.toml` (atomic on FAT)
4. Update NVS cache mirror

If power dies mid-write, FAT rename leaves either the old file or the new file — never half-written. Same pattern that databases use.

## Operational state — NOT in this file

Live runtime state (current alarm state, mute timer, last-known position, etc.) **does not live in `config.toml`**. It's in RAM with a separate `state.json` for crash-recovery. Config = setup; state = runtime.

## What CAN'T be set from this file

- State colors (red = ALARM is universal — safety-critical, hardcoded)
- Algorithm constants (centroid math, distance formulas)
- Hardware pin assignments (compile-time `board_config.h`)
- Variant selection (Display vs IMU/GPS — `PRODUCT_VARIANT` compile flag, prereq #49)
- Brand assets (logo, vendor name — separate brand build, prereq #46)

## Migration / version handling

Schema version is implicit. v0.2 firmware accepts any v0.2-compatible config. When v0.3 introduces a breaking change (rare):

- v0.3 firmware reads v0.2 config, applies any field renames, writes back v0.3-shape on next save
- Document migrations in this file's changelog
- Never silently drop user data

For v0.2, no migration is required — this is the first canonical schema.

## Examples

See [`docs/config.example.toml`](./config.example.toml) for a fully-commented sample covering every option.

## Implementation references

- TOML parser: candidates include [`tomlc99`](https://github.com/cktan/tomlc99) (single-source-file, MIT, ~600 LOC) or `cpptoml` (heavier). Recommend `tomlc99` for ESP-IDF.
- See Workstream 4 epic #33 (SD card config system) for implementation tracking.
- Schema validation: per-setting `setting_bounds_t` struct in firmware. Bounds defined in C; reject-and-warn behavior on load.

## Change log

- **0.1.0** (2026-04-26): Initial schema. Three-tier fallback. Three GPS source modalities (N2K with source-address selection / Serial / URL). Three IMU source modalities (N2K / Serial / Internal). Distance presets with unit-tagged validation. Brand-customizable but safety colors hardcoded.
