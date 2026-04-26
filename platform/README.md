# platform/ — per-platform implementations

Each subdirectory contains the platform-specific glue code (drivers, UI, transport) that wraps the portable `core/` library for a specific target.

| Subdirectory | Status | Notes |
|---|---|---|
| `esp32/` | Active | ESP-IDF firmware for the device. Run `idf.py build` from this directory. |
| `ios/` | Empty (planned) | iOS app — see [Workstream 2 epic #31](https://github.com/Temple-of-Epiphany/anchor-drag-pro/issues/31) |
| `android/` | Empty (planned) | Android app — v0.4+ |

## Why split this way

Each platform has its own native build system and tooling:
- ESP-IDF + CMake for ESP32
- Xcode + Swift for iOS
- Gradle + Kotlin for Android

Mixing these in one directory creates conflicts; separating them lets each use native tooling without contamination.

The shared algorithm + protocol code lives in `core/` and is consumed by every platform (compiled in for ESP32, bridged in for iOS, JNI-linked for Android, WASM-compiled for the web simulator).
