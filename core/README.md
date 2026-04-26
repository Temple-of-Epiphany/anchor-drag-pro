# core/ — portable C library

Pure C99 algorithm + protocol code shared across all platforms (ESP32 firmware, future iOS app, future Android app, future WASM web simulator).

## Status

Empty scaffold. Implementation tracked under [Workstream 1 epic #30](https://github.com/Temple-of-Epiphany/anchor-drag-pro/issues/30) with 9 child issues (#50-#58).

## Build (host-side, when implemented)

```bash
cd core
cmake -B build
cmake --build build
ctest --test-dir build
```

## Rules

- **Pure C99.** No platform headers (`esp_*.h`, `freertos/*.h`, `lvgl.h`, `<UIKit/...>`, `<jni.h>`).
- **No globals.** Everything goes through opaque context structs.
- **Time as input parameter.** Never call platform time directly. Functions take `now_ms` arguments.
- **strtok_r() not strtok().** Thread-safety contract.

## Planned structure

```
core/
├── include/
│   ├── anchor_fix.h         # gps_fix_t canonical struct
│   ├── anchor_geo.h         # distance + centroid math
│   ├── anchor_state.h       # state machine
│   ├── anchor_attitude.h    # IMU + heading source manager
│   ├── nmea2000.h           # N2K PGN parser
│   ├── yd_raw.h             # YachtDevices RAW format decoder
│   └── anchor_module.h      # module registry
├── src/
│   └── (corresponding .c files)
├── tests/
│   ├── test_anchor_geo.c
│   ├── test_anchor_state.c
│   ├── (etc.)
│   └── test_scenarios/      # full-algorithm scenario tests
└── CMakeLists.txt
```
