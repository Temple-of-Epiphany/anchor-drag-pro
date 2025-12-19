# LVGL Simulator Setup Guide - macOS

## Version 1.0.0 - December 11, 2025

**Author:** Colin Bitterfield  
**Email:** colin@bitterfield.com  
**Project:** ANCHOR-ALARM

---

## Overview

This guide covers setting up an LVGL simulator on macOS for rapid GUI development without flashing hardware. The simulator allows sub-second rebuild times versus 30+ seconds for ESP32 flash cycles.

---

## Option 1: SDL2 Native Simulator (Recommended)

### Prerequisites

Install SDL2 via MacPorts:

```bash
sudo port install libsdl2 libsdl2_ttf libsdl2_image
```

### PlatformIO Configuration

Add a native environment to `platformio.ini`:

```ini
[env:native]
platform = native
build_flags = 
    -D LV_CONF_INCLUDE_SIMPLE
    -D LV_USE_SDL
    -D SDL_HOR_RES=800
    -D SDL_VER_RES=480
    -I /opt/local/include
    -I /opt/local/include/SDL2
    -L /opt/local/lib
    -lSDL2
    -lSDL2_image
lib_deps =
    lvgl/lvgl@^9.0.0
```

### Project Structure

```
anchor-drag-alarm/
├── src/
│   ├── main.cpp          # ESP32 entry point
│   └── main_native.cpp   # Desktop entry point
├── gui/
│   ├── ui.h              # Shared UI declarations
│   ├── ui.cpp            # Shared UI implementation
│   ├── screens/
│   │   ├── anchor_screen.cpp
│   │   ├── settings_screen.cpp
│   │   └── status_screen.cpp
│   └── components/
│       ├── compass_widget.cpp
│       └── position_widget.cpp
├── include/
│   └── lv_conf.h
└── platformio.ini
```

### Dual-Target Build

```ini
# platformio.ini

[platformio]
default_envs = esp32s3

[env:esp32s3]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
build_flags = 
    -D LV_CONF_INCLUDE_SIMPLE
    -D ARDUINO_USB_CDC_ON_BOOT=1
lib_deps =
    lvgl/lvgl@^9.0.0
    lovyan03/LovyanGFX@^1.1.9

[env:native]
platform = native
build_flags = 
    -D LV_CONF_INCLUDE_SIMPLE
    -D LV_USE_SDL
    -D SDL_HOR_RES=800
    -D SDL_VER_RES=480
    -I /opt/local/include
    -I /opt/local/include/SDL2
    -L /opt/local/lib
    -lSDL2
lib_deps =
    lvgl/lvgl@^9.0.0
```

### Build Commands

```bash
# Build and run simulator
pio run -e native && .pio/build/native/program

# Build for ESP32
pio run -e esp32s3

# Upload to ESP32
pio run -e esp32s3 -t upload
```

---

## Option 2: Wokwi Browser Simulator

No local setup required. Use https://wokwi.com for browser-based ESP32 simulation.

### Pros
- Zero setup
- Simulates actual ESP32
- Supports many peripherals

### Cons
- Slower than native
- Requires internet
- Limited to supported boards

---

## Option 3: SquareLine Studio

Commercial LVGL visual designer with built-in simulator.

### Pros
- Drag-and-drop UI design
- Generates LVGL code
- Built-in preview

### Cons
- Commercial license required
- Generated code may need cleanup
- Less control than manual coding

---

## Simulator Comparison

| Feature | SDL2 Native | Wokwi | SquareLine |
|---------|-------------|-------|------------|
| Setup | MacPorts | None | Installer |
| Rebuild Time | <1 sec | 5-10 sec | <1 sec |
| ESP32 Accurate | No | Yes | No |
| Offline | Yes | No | Yes |
| Cost | Free | Free | $$ |
| Best For | Rapid UI dev | Hardware test | Visual design |

---

## Recommended Workflow

1. **Design UI** in SDL2 simulator for fast iteration
2. **Test logic** in Wokwi for ESP32 accuracy (optional)
3. **Final test** on actual hardware
4. **Share code** between simulator and ESP32 via `gui/` folder

---

## LVGL Configuration (lv_conf.h)

Key settings for 800x480 display:

```c
#define LV_HOR_RES_MAX 800
#define LV_VER_RES_MAX 480
#define LV_COLOR_DEPTH 16
#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN

/* Enable features */
#define LV_USE_LABEL 1
#define LV_USE_BTN 1
#define LV_USE_ARC 1
#define LV_USE_CHART 1
#define LV_USE_CANVAS 1

/* Fonts */
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_28 1
```

---

## Native Main Entry Point

```cpp
// src/main_native.cpp
#ifdef NATIVE_BUILD

#include "lvgl.h"
#include "gui/ui.h"
#include <SDL2/SDL.h>

static lv_display_t *display;
static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *texture;

int main(int argc, char *argv[]) {
    // Initialize SDL
    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("Anchor Alarm", 
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        800, 480, 0);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565,
        SDL_TEXTUREACCESS_STREAMING, 800, 480);

    // Initialize LVGL
    lv_init();
    
    // Create display
    display = lv_display_create(800, 480);
    // ... setup flush callback ...
    
    // Create UI (shared code)
    ui_init();
    
    // Main loop
    while (1) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) return 0;
            // Handle touch/mouse events
        }
        lv_timer_handler();
        SDL_Delay(5);
    }
    
    return 0;
}

#endif
```

---

## Resources

- **LVGL Docs:** https://docs.lvgl.io/
- **LVGL Simulator:** https://github.com/lvgl/lv_port_pc_vscode
- **Wokwi:** https://wokwi.com
- **SquareLine:** https://squareline.io
- **MacPorts SDL2:** https://ports.macports.org/port/libsdl2/
