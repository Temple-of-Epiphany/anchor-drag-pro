# Changelog

All notable changes to the Anchor Drag Pro firmware will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

This changelog is automatically extracted by `.github/workflows/release.yml` —
each release on [anchor-drag-pro-releases](https://github.com/Temple-of-Epiphany/anchor-drag-pro-releases)
uses the matching section here as its release notes.

## [Unreleased]

### Added
- v0.2 clean rebuild scaffold (Phase 0): boots, logs chip info, heartbeats
- I2C0 bus + FreeRTOS mutex (Phase 2A) — shared-bus discipline for CH422G / GT911 / RTC / IMU
- CH422G I/O expander driver (Phase 2B) — vendored C driver, uses i2c_master + WITH_I2C_MUTEX pattern; named accessors for backlight, LCD reset, touch reset, SD CS, USB/CAN switch
- SD card driver (Phase 2C) — ESP-IDF built-in sdspi + FATFS at /sdcard, CS via CH422G EXIO4, watchdog-safe write/sync wrappers
- OTA from SD card (Phase 2D) — scan /sdcard/firmware/, SHA256 verify, esp_ota_write + auto-rollback. Closes Workstream 0 prereq #39 (16 MB partition table refresh).
- Cross-cutting design docs in [anchor-drag-pro-meta](https://github.com/Temple-of-Epiphany/anchor-drag-pro-meta): glossary, state-spec, design-tokens, adaptive-layout
- Engineering notes: `docs/sensor-selection.md` (IMU/GPS sensor choice rationale), `docs/library-choices.md` (canonical library set per subsystem), `docs/config-schema.md` + `docs/config.example.toml` (SD config file specification)

### Changed
- Repo reorganized to platform-segregated layout: `main/` → `platform/esp32/main/`, new `core/`, `platform/ios/`, `brands/`, `tokens/` directories scaffolded. Closes Workstream 0 prereq #48.
- Partition table: 16 MB dual-OTA layout (app0 + app1 = 6 MB each, LittleFS 3.8 MB, coredump 64 KB). Closes Workstream 0 prereq #39.

### Removed
- Full v0.1 six-screen LVGL implementation. Preserved at git tag `v0.1.0-archive` and branch `archive/v0.1` for reference (working LCD/touch/font/screen patterns).

## [v0.1.0-archive] - 2026-04-25

Reference snapshot of the v0.1 codebase, archived before the v0.2 clean rebuild began. Six-screen LVGL implementation with full hardware bring-up. See the tag annotation for details.
