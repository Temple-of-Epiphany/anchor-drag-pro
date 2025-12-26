

The# Anchor Drag Alarm - UI Screen Guide

**Version:** 1.5.0
**Author:** Colin Bitterfield
**Date Updated:** 2025-12-26
**Hardware:** Waveshare ESP32-S3-Touch-LCD-4.3B (800×480 IPS display)
**UI Framework:** LVGL 9.2.0
**Configuration Storage:** ESP32 NVS (Non-Volatile Storage)

---

## Table of Contents

1. [Global Header Bar](#global-header-bar)
2. [Screen Navigation Flow](#screen-navigation-flow)
3. [Screen Index](#screen-index)
4. [Screen Details](#screen-details)
5. [Color Scheme](#color-scheme)
6. [Operating Modes](#operating-modes)

---

## Global Header Bar

All screens (except SPLASH) feature a consistent full-width header bar (800×80px) with:

**Center:**
- "ANCHOR DRAG ALARM" title
- Current time display (HH:MM:SS) updated every second from RTC

**Header Icons Reference:**

| Position | Side  | Icon | Symbol | Disconnected Color | Connected Color | Status Meaning |
|----------|-------|------|--------|-------------------|-----------------|----------------|
| 0 | Left | Bluetooth | LV_SYMBOL_BLUETOOTH | Gray (#808080) | Blue (#0080FF) | Bluetooth paired/connected |
| 1 | Left | WiFi | LV_SYMBOL_WIFI | Gray (#808080) | Green (#00AA00) | WiFi network connected |
| 2 | Left | TF Card | LV_SYMBOL_SD_CARD | Gray (#808080) | Green (#00AA00) | SD card detected/mounted |
| 0 | Right | Compass/Helm | ⎈ U+2388 (Apple Symbols) | Gray (#808080) | Green (#00AA00) | Compass data available |
| 1 | Right | GPS | 🛰️ (satellite emoji) | Gray (#808080) | Green (#00AA00) | GPS fix acquired |
| 2 | Right | Anchor | ⚓ (anchor emoji) | Gray (#808080) | Green (#00AA00) | Anchor monitoring armed |

**Icon Specifications:**
- **Shape:** Circular (50×50px)
- **Border:** 2px, color matches background
- **Glow:** Background color serves as status indicator
- **Default State:** Gray (#808080) when inactive/disconnected
- **Active State:** Green (#00AA00) for most icons, Blue (#0080FF) for Bluetooth only

**Time Display:**
- Updated automatically every second
- Synchronized with RTC (PCF85063A)
- Shows local time (UTC + timezone offset)
- Format: HH:MM:SS (24-hour)

---

## Font Usage Reference

**Font Definitions (ui_theme.h):**

| Definition | Font | Size (pt) | Usage | Screens Using |
|------------|------|-----------|-------|---------------|
| FONT_TITLE | Orbitron Variable | 24 | Screen titles | All main screens (START, INFO, PGN, CONFIG, etc.) |
| FONT_SUBTITLE | Orbitron Variable | 20 | Subtitles, section headers | START, INFO, CONFIG screens |
| FONT_BUTTON_LARGE | Orbitron Variable | 20 | Large action buttons | START (mode buttons), CONFIG (action buttons) |
| FONT_BUTTON_SMALL | Orbitron Variable | 16 | Small buttons, tool buttons | TOOLS (9 buttons), navigation buttons |
| FONT_BODY_LARGE | Orbitron Variable | 20 | Large body text, important info | System Info, warnings, important labels |
| FONT_BODY_NORMAL | Orbitron Variable | 16 | Normal body text, labels | General text, form labels, descriptions |
| FONT_BODY_SMALL | Orbitron Variable | 16 | Small body text | Fine print, secondary information |
| FONT_LABEL | Orbitron Variable | 16 | Input labels, field labels | Form labels, configuration options |
| FONT_FOOTER | Orbitron Variable | 16 | Footer navigation buttons | Global footer on all screens |

**Font Consolidation Analysis:**

**Current Font Sizes in Use:**
- **24pt:** Titles only (1 definition)
- **20pt:** Subtitles, Large buttons, Large body text (3 definitions using same size)
- **16pt:** Small buttons, Normal body text, Small body text, Labels, Footer (5 definitions using same size)

**Font Families in Use:**
- ✅ **Orbitron Variable:** 100% of all text (all 9 font definitions)
- ❌ **Montserrat:** Removed - no longer used

**Consolidation:**
- ✅ **Fully consolidated to Orbitron** - Entire UI uses single font family for consistency
- ✅ **Only 3 actual font sizes** - 24pt, 20pt, 16pt (optimal for performance and visual hierarchy)
- ✅ **Semantic definitions maintained** - 9 definitions provide code clarity while using only 3 actual fonts

**Result:** Optimal font usage - minimal font files loaded, maximum UI consistency, semantic code clarity preserved.

---

## Screen Navigation Flow

```
BOOT
  ↓
[SPLASH SCREEN] (30 seconds)
  ↓
[START] ←→ [INFO] ←→ [PGN] ←→ [CONFIG] ←→ [UPDATE] ←→ [TOOLS]
   ↓          (Swipe Navigation: ◀ ◀ ◀ ◀ ◀ ▶ ▶ ▶ ▶ ▶)
   ↓
[DISPLAY] (Main screen - shown after mode selection)
   ↓
[TEST] (Accessed from TOOLS page)
```

**Navigation Pattern:**
- **Swipe Left/Right:** Navigate between pages (START ↔ INFO ↔ PGN ↔ CONFIG ↔ UPDATE ↔ TOOLS)
- **Button Press:** Select mode or activate functions
- **ESC/BACK Button:** Return to previous screen

**Global Navigation Bar:**

All screens (except SPLASH and DISPLAY) feature a consistent bottom navigation bar with swipe indicators showing current page position.

**Documentation Display:**
For simplicity, screen layouts in this document show:
```
├─────────────────────────────────────────────────────┤
│              Global Navigation                      │
└─────────────────────────────────────────────────────┘
```

**Actual Implementation:**
The real UI displays page indicators showing current position:
```
┌─────────────────────────────────────────────────────┐
│                 SCREEN CONTENT                      │
│                                                     │
├─────────────────────────────────────────────────────┤
│ ◀ PREV        ◀ ◀ ◀ CURRENT ▶ ▶ ▶        NEXT ▶   │ ← Active NAV Bar
└─────────────────────────────────────────────────────┘
```

**Page Indicators (Implementation):**
- ▶ ◀ ◀ ◀ ◀ ◀ = START (page 1 of 6)
- ▶ ▶ ◀ ◀ ◀ ◀ = INFO (page 2 of 6)
- ▶ ▶ ▶ ◀ ◀ ◀ = PGN (page 3 of 6)
- ▶ ▶ ▶ ▶ ◀ ◀ = CONFIG (page 4 of 6)
- ▶ ▶ ▶ ▶ ▶ ◀ = UPDATE (page 5 of 6)
- ▶ ▶ ▶ ▶ ▶ ▶ = TOOLS (page 6 of 6)

**Navigation Behavior:**
- Swipe left from START wraps to TOOLS
- Swipe right from TOOLS wraps to START
- Page indicators update automatically on swipe
- Current page highlighted in center of NAV bar
- Previous/next page names shown on left/right edges

---

## Screen Index

| # | Screen Name | Page Indicator | Purpose |
|---|-------------|----------------|---------|
| 0 | **SPLASH** | N/A | Boot screen with logo and version |
| 1 | **START** | ▶ ◀ ◀ ◀ ◀ ◀ | Mode selection (OFF/READY/CONFIG) |
| 2 | **INFO** | ▶ ▶ ◀ ◀ ◀ ◀ | Compass rose and detailed GPS |
| 3 | **DISPLAY (Ready to Anchor)** | N/A | Main monitoring before anchor set |
| 3a | **DISPLAY (Anchoring)** | N/A | GPS plotting and anchor tracking |
| 4 | **PGN** | ▶ ▶ ▶ ◀ ◀ ◀ | NMEA 2000 message monitor |
| 5 | **CONFIG** | ▶ ▶ ▶ ▶ ◀ ◀ | System configuration (NVS storage) |
| 6 | **UPDATE** | ▶ ▶ ▶ ▶ ▶ ◀ | Firmware update |
| 7 | **TOOLS** | ▶ ▶ ▶ ▶ ▶ ▶ | System utilities |
| 8 | **TEST** | N/A | Hardware testing (from TOOLS) |

---

## Screen Details

### 1. SPLASH SCREEN (Boot)

**Purpose:** Initial boot screen with branding and system initialization

**Display Time:** 30 seconds (configurable)

**Content:**
- **Logo:** Anchor icon (loaded from TF card: `/anchor_logo.bmp`)
- **Title:** "ANCHOR DRAG ALARM"
- **Version:** UI version number (matches UI_Screens.md version)
- **Status:** Loading message and animation
- **Self-Test:** TF card detection, N2K/GPS source detection

**Layout:**
```
┌─────────────────────────────────────────────────────┐
│                                                     │
│              🏴‍☠️ ANCHOR DRAG ALARM                 │
│                   Version 1.1.0                     │
│                                                     │
│                   [Loading...]                      │
│                   ●○○○○ (animation)                 │
│                                                     │
│               Self-Test: TF Card ✓                  │
│                         N2K Data ✓                  │
│                         GPS Ready                   │
└─────────────────────────────────────────────────────┘
```

**Version Display:**
- **IMPORTANT:** The version number displayed on boot must match the version in this document (UI_Screens.md)
- Current UI Version: **1.1.0**
- Define in code as: `#define UI_VERSION "1.1.0"`
- Update this define whenever UI_Screens.md version is incremented

**Self-Test Sequence:**

1. **TF Card Detection:**
   - Check for mounted TF card
   - Display: "TF Card ✓" or "TF Card ✗"
   - Check for `update.bin` → if found, transition to UPDATE screen

2. **GPS Source Detection (Priority Order):**
   - **First: Check N2K Data (NMEA 2000)**
     - Attempt connection to N2K simulator (N2K_HOST:N2K_PORT)
     - Wait for PGN 129029 (GPS Position Data)
     - If N2K data detected: Display "N2K Data ✓" and "GPS Ready"
     - **Skip remaining GPS checks** (no need to check NMEA 0183 or external GPS)

   - **Second: Check NMEA 0183 (only if N2K not present)**
     - If N2K not available, check for NMEA 0183 serial data
     - Display: "0183 Data ✓" or continue to next

   - **Third: Check External GPS (only if N2K and 0183 not present)**
     - If neither N2K nor 0183 available, check external GPS module
     - Display: "External GPS ✓" or "No GPS ✗"

3. **Status Display:**
   - If any GPS source found: "GPS Ready"
   - If no GPS source: "GPS Not Found" (warning, but allow boot)

**Transitions:**
- After self-test complete (max 30 seconds) → **START screen**
- If `update.bin` detected during TF card check → **UPDATE screen**

---

### 2. START SCREEN (Mode Selection)

**Purpose:** Select operating mode and shows DISPLAY SCREEN (Main Monitoring) when READY

**Page Indicator:** ▶ ◀ ◀ ◀ ◀ ◀

**Content:**
- **Title:** "SELECT MODE"
- **Subtitle:** "Choose your operating mode"
- **Four Action Buttons:**
  1. **OFF** - System Disabled (Gray) - Changes background to grey, stays on START screen
  2. **READY** - Activate Anchor Monitoring (Green) - Navigates to DISPLAY screen
  3. **INFO** - View GPS & Compass Details (Cyan) - Navigates to INFO screen
  4. **CONFIG** - Configure System Settings (Purple) - Navigates to CONFIG screen

**Layout:**
```
┌─────────────────────────────────────────────────────┐
│                   SELECT MODE                       │
│            Choose your operating mode               │
│                                                     │
│   ┌─────────────────────────────────────┐          │
│   │  ⚡ OFF                              │          │
│   │  System Disabled                    │          │
│   └─────────────────────────────────────┘          │
│                                                     │
│   ┌─────────────────────────────────────┐          │
│   │  🏠 READY                            │          │
│   │  Activate Anchor Monitoring         │          │
│   └─────────────────────────────────────┘          │
│                                                     │
│   ┌─────────────────────────────────────┐          │
│   │  ℹ️ INFO                             │          │
│   │  View GPS & Compass Details         │          │
│   └─────────────────────────────────────┘          │
│                                                     │
│   ┌─────────────────────────────────────┐          │
│   │  ⚙️ CONFIG                           │          │
│   │  Configure System Settings          │          │
│   └─────────────────────────────────────┘          │
│                                                     │
├─────────────────────────────────────────────────────┤
│              Global Navigation                      │
└─────────────────────────────────────────────────────┘
```

**Actions:**
- **Click OFF:** Sets mode to OFF, changes background to grey, stays on START screen
- **Click READY:** Sets mode to READY, navigates to **DISPLAY screen (Main Monitoring)**
- **Click CONFIG:** Navigates to **CONFIG screen**
- **Click INFO:** Navigates to **INFO screen** (compass & GPS details)
- **Swipe Right:** Navigate to **INFO screen**

**Color Scheme:**
- Background: Blue (#0074D9, configurable via NVS)
- OFF button: Grey (#AAAAAA)
- READY button: Green (#2ECC40)
- CONFIG button: Blue (#0074D9)
- INFO button: Teal (#39CCCC)
- Text: White (#FFFFFF, configurable via NVS)

---

### 3. DISPLAY SCREEN (Ready to Anchor)

**Purpose:** Main monitoring display when in READY mode, before anchor is set

**State:** READY mode (waiting for user to press anchor button)

**Color Scheme:**
- **Background:** Light Blue (#ADD8E6)
- **Text:** Black (#000000)
- **Panels:** Dark backgrounds with colored borders for contrast

**Content:**
- **Top Bar:** Operating mode (READY), GPS status indicator
- **GPS Data Panel:** Current position with fixed-width formatting, satellite count
- **Anchor Button:** Large anchor icon button to start anchor location process
- **Connection Info:** N2K simulator connection status

**Layout:**
```
┌─────────────────────────────────────────────────────┐
│ MODE: READY                          GPS: ●         │ ← Status bar
│                                                     │
│ Lat:  30.031355                      ┌────┐        │ ← GPS upper left
│ Lon: -90.034512                      │ N  │        │   Compass upper right
│ Satellites:  8                       │W⊕E │        │   (or "/|" if no data)
│                                      │ S  │        │
│                                      └────┘        │
│                                                     │
│                                                     │
│                    ┌───────┐                       │
│                    │       │                       │
│                    │   ⚓   │                       │ ← Large anchor button
│                    │       │                       │
│                    └───────┘                       │
│                                                     │
│              Press to set anchor location          │
│                                                     │
│                 
                                    │
│ N2K Connected: 127.0.0.1:10110      12:34:56       │ ← Connection & time
├─────────────────────────────────────────────────────┤
│   [INFO]        [CONFIG]        [MODE]             │ ← Navigation
└─────────────────────────────────────────────────────┘
```

**Display Elements:**

**Status Bar (Top):**
- MODE indicator: "READY"
- GPS indicator: Green dot (●) when data flowing, Red (●) when no data

**Upper Left - GPS Data:**
```
Lat:  30.031355
Lon: -90.034512
Satellites:  8
```
- Fixed-width formatting prevents shifting
- Latitude: `%9.6f` format
- Longitude: `%10.6f` format
- Satellites: `%2d` format
- Updates at 2Hz from N2K simulator

**Upper Right - Compass Rose:**
- Shows compass rose with heading indicator if compass PGN data present
- If no compass data available, shows placeholder icon: `/|`
- Heading displayed as degrees (e.g., "045°")

**Center - Anchor Button:**
- Large, prominent anchor icon button (200×200 pixels)
- Text below: "Press to set anchor location"
- Click to transition to **DISPLAY SCREEN (Anchoring)**

**Bottom Left - Connection Info:**
- N2K connection status: "N2K Connected: [host]:[port]"
- Shows actual N2K_HOST and N2K_PORT values

**Bottom Right - Time:**
- Current time from GPS or system clock
- Format: `HH:MM:SS`
- Updates every second

**Actions:**
- **Anchor Button:** Press to transition to **DISPLAY SCREEN (Anchoring)** - starts GPS collection for anchor circle
- **MODE Button:** Return to START screen
- **INFO Button:** Navigate to INFO screen
- **CONFIG Button:** Navigate to CONFIG screen

---

### 3a. DISPLAY SCREEN (Anchoring)

**Purpose:** GPS plotting and anchor tracking during ARMING and ARMED states

**States:** ARMING (collecting GPS points) → ARMED (monitoring anchor position) → ALARM (drag detected)

**Color Scheme:**
- **Background:** Light Blue (#ADD8E6)
- **Text:** Black (#000000)
- **Panels:** Dark backgrounds with colored borders for contrast

**Content:**
- **Top Bar:** Current state (ARMING/ARMED/ALARM), GPS status
- **GPS Data Panel:** Current position with fixed-width formatting
- **GPS Plot Canvas:** Real-time plotting of boat position and anchor circle
- **Distance Display:** Current distance from anchor
- **Navigation Buttons:** Return to START or INFO/CONFIG screens

**Layout:**
```
┌─────────────────────────────────────────────────────┐
│ STATE: ARMED                         GPS: ●         │ ← Status bar
│                                                     │
│ Lat:  30.031355                      ┌────┐        │ ← GPS upper left
│ Lon: -90.034512                      │ N  │        │   Compass upper right
│ Satellites:  8                       │W⊕E │        │
│ Distance: 35 ft                      │ S  │        │
│                                      └────┘        │
│                                                     │
│                    ●  ● ●                          │
│                  ●       ●                         │ ← GPS track dots
│                 ●    ⚓    ●                        │   Anchor at center
│                  ●       ●                         │
│                    ● 🚢 ●                          │   Boat = current pos
│                      ●                             │
│                                                     │
│                                                     │
│ Anchor Circle: 50 ft radius            12:34:56    │ ← Info & time
├─────────────────────────────────────────────────────┤
│   [INFO]        [CONFIG]        [MODE]             │ ← Navigation
└─────────────────────────────────────────────────────┘
```

**Real-Time GPS Plotting:**

**Phase 1: Initial GPS Collection (ARMING State)**
- GPS coordinates plotted relative to themselves in the middle of the screen
- Each new point (arriving at 2Hz) is plotted as a dot
- Points accumulate to establish movement pattern (typically 60 seconds)
- Screen auto-scales to fit all plotted points
- Status displays: "STATE: ARMING"

**Phase 2: Anchor Circle Established (ARMED State)**
- Once collection period complete:
  - Calculate centroid (center point): `(avg_lat, avg_lon)`
  - Plot anchor icon (⚓) at the center
  - Calculate maximum swing radius: `max(distance(centroid, point))`
  - Draw anchor circle boundary (radius + configured buffer)
  - Status displays: "STATE: ARMED"

**Phase 3: Real-Time Tracking (ARMED State, 2Hz updates)**
- **Boat Icon (🚢):** Shows current GPS position relative to anchor
- **Trail Dots (●):** Previous positions
- **Update Cycle:**
  1. New GPS point arrives at 2Hz (every 500ms)
  2. Move boat icon to new position
  3. Convert previous boat position to a dot
  4. Maintain trail of recent positions (configurable, e.g., last 100 points)
  5. Calculate distance from anchor center
  6. Update distance display
  7. Auto-scale view to keep both anchor and boat visible
  8. If distance > alarm threshold → transition to ALARM state

**Phase 4: Alarm State (ALARM)**
- Same display as ARMED state
- Status displays: "STATE: ALARM" (red background)
- Boat icon turns red
- Buzzer activates
- LED blinks red

**Display Elements:**

**Status Bar (Top):**
- STATE indicator: "ARMING", "ARMED", or "ALARM"
- GPS indicator: Green dot (●) when data flowing, Red (●) when no data

**Upper Left - GPS Data:**
```
Lat:  30.031355
Lon: -90.034512
Satellites:  8
Distance: 35 ft
```
- Fixed-width formatting prevents shifting
- Distance to anchor displayed in real-time

**Upper Right - Compass Rose:**
- Shows compass rose with heading indicator if compass PGN data present
- If no compass data available, shows placeholder icon: `/|`

**Center Area - GPS Plot Canvas:**
- Real-time plotting canvas (600×320 pixels)
- Auto-scaling to fit anchor circle and current boat position
- Grid overlay (optional, configurable)
- Distance scale indicator
- Anchor icon (⚓) at calculated center
- Boat icon (🚢) at current position
- Trail dots (●) showing recent positions
- Anchor circle boundary

**Bottom Left - Anchor Info:**
- "Anchor Circle: [radius] ft radius"

**Bottom Right - Time:**
- Current time from GPS or system clock
- Format: `HH:MM:SS`
- Updates every second

**Plotting Algorithm:**

1. **Coordinate Transformation:**
   - Convert GPS lat/lon to screen coordinates
   - Center on anchor position (or first GPS point if no anchor yet)
   - Scale to fit display area
   - Maintain aspect ratio (equirectangular projection)

2. **Trail Management:**
   - Store last N GPS points (e.g., 100)
   - Render as dots with optional fade effect
   - Oldest points gradually fade or drop off

3. **Anchor Circle Calculation:**
   - Collect GPS points during ARMING period (default 60 seconds)
   - Calculate centroid: `(avg_lat, avg_lon)`
   - Calculate max radius: `max(distance(centroid, point))`
   - Draw circle with radius + configured buffer (default: 50 ft)

4. **Auto-Scale:**
   - Calculate bounding box of all visible elements
   - Scale to fit 80% of display area (20% margin)
   - Update scale factor when boat approaches edge

**N2K Simulator Connection:**
- **Connection configured via ENV variables:**
  - `N2K_HOST` - Hostname/IP (default: 127.0.0.1)
  - `N2K_PORT` - TCP port for NMEA data (default: 10110)
  - `N2K_HTTP_PORT` - HTTP API port (default: 9001)
- **GPS updates at 2Hz** from N2K simulator

**Data Logging:**
- **GPS Track:** CSV format `YYYYMMDD_HHMMSS.csv` logged to TF card
- **Syslog Events:** State transitions, alarms (10MB rotation)

**Actions:**
- **MODE Button:** Return to START screen (stops monitoring, resets state)
- **INFO Button:** Navigate to INFO screen (monitoring continues)
- **CONFIG Button:** Navigate to CONFIG screen (monitoring continues)

---

### 4. INFO SCREEN (Compass & Details)

**Purpose:** Detailed GPS information and compass rose

**Page Indicator:** ▶ ▶ ◀ ◀ ◀ ◀

**Content:**
- **Compass Rose:** 360° graphical compass with heading indicator
- **GPS Details:**
  - Position (Lat/Lon with decimal precision)
  - Speed Over Ground (SOG)
  - Course Over Ground (COG)
  - Altitude
  - Satellites in view
  - HDOP/PDOP
- **Timestamp:** Last GPS update time

**Layout:**
```
┌─────────────────────────────────────────────────────┐
│              POSITION & NAVIGATION                  │
├─────────────────────────────────────────────────────┤
│                                                     │
│        ┌─────────────┐        GPS POSITION         │
│        │      N      │        Lat: 30.031355°N     │
│        │   W ⊕ E    │        Lon: 90.034512°W     │
│        │      S      │        Alt: 5.2 m           │
│        └─────────────┘                              │
│       Hdg: 045° (NE)          VELOCITY              │
│                               SOG: 0.2 kts          │
│                               COG: 045°             │
│                                                     │
│                               QUALITY               │
│                               Sats: 8               │
│                               HDOP: 1.2             │
│                               PDOP: 2.1             │
│                                                     │
│                        Last Update: 12:34:56        │
├─────────────────────────────────────────────────────┤
│              Global Navigation                      │
└─────────────────────────────────────────────────────┘
```

**Actions:**
- **Swipe Left:** Return to START screen
- **Swipe Right:** Navigate to PGN screen

---

### 5. PGN SCREEN (NMEA 2000 Monitor)

**Purpose:** Real-time NMEA 2000 PGN message monitoring

**Page Indicator:** ▶ ▶ ▶ ◀ ◀ ◀

**Content:**
- **Title:** "N2K PGN MONITOR"
- **Message List:** Scrolling list of incoming PGN messages
- **For Each Message:**
  - PGN number
  - Source address
  - Decoded data
  - Timestamp

**Layout:**
```
┌─────────────────────────────────────────────────────┐
│              N2K PGN MONITOR                        │
├─────────────────────────────────────────────────────┤
│                                                     │
│  PGN 129029 [GPS] Src: 0x15                        │
│  Lat: 30.031355 Lon: -90.034512 Sats: 8           │
│  12:34:56.123                                       │
│  ─────────────────────────────────────────────     │
│                                                     │
│  PGN 127250 [Heading] Src: 0x20                    │
│  HDG: 045.3° Variation: -5.2°                      │
│  12:34:55.891                                       │
│  ─────────────────────────────────────────────     │
│                                                     │
│  PGN 130306 [Wind] Src: 0x30                       │
│  Speed: 12.5 kts Dir: 270° (Relative)              │
│  12:34:55.456                                       │
│  ─────────────────────────────────────────────     │
│                                                     │
│  [Auto-scroll, 20 message buffer]                  │
│                                                     │
├─────────────────────────────────────────────────────┤
│              Global Navigation                      │
└─────────────────────────────────────────────────────┘
```

**Features:**
- Auto-scrolling message buffer (20 messages)
- Color-coded by message type
- Real-time updates
- Pause/resume scrolling

**Actions:**
- **Swipe Left:** Navigate to INFO screen
- **Swipe Right:** Navigate to CONFIG screen

---

### 6. CONFIG SCREEN (Configuration)

**Purpose:** System settings and configuration

**Page Indicator:** ▶ ▶ ▶ ▶ ◀ ◀

**Content:**
- **Alarm Settings:**
  - Distance threshold (feet/yards/meters)
  - Units selection
- **N2K Data Settings:**
  - GPS PGN source
  - Compass PGN source
  - External GPS enable/disable
- **Display Settings:**
  - Background color (16-color palette or custom hex)
  - Font color (16-color palette or custom hex)
- **System Settings:**
  - Boat name
  - WiFi configuration
  - Bluetooth enable/disable and pairing code

**Layout:**
```
┌─────────────────────────────────────────────────────┐
│                 CONFIGURATION                       │
├─────────────────────────────────────────────────────┤
│                                                     │
│  ALARM SETTINGS                                     │
│  ┌─────────────────────────────────────┐           │
│  │ Distance Threshold: 50 ft    [+][-] │           │
│  │ Units: Feet ▼                       │           │
│  └─────────────────────────────────────┘           │
│                                                     │
│  N2K DATA SETTINGS                                  │
│  ┌─────────────────────────────────────┐           │
│  │ GPS PGN: 129029              [Edit] │           │
│  │ Compass PGN: 127250          [Edit] │           │
│  │ External GPS: [ ] Enable            │           │
│  └─────────────────────────────────────┘           │
│                                                     │
│  DISPLAY SETTINGS                                   │
│  ┌─────────────────────────────────────┐           │
│  │ Background: Light Blue ▼     [HEX]  │           │
│  │ Font Color: Black ▼          [HEX]  │           │
│  └─────────────────────────────────────┘           │
│                                                     │
│  SYSTEM SETTINGS                                    │
│  ┌─────────────────────────────────────┐           │
│  │ Boat Name: [My Boat_______]         │           │
│  │ WiFi: Disabled          [Enable]    │           │
│  │ Bluetooth: Enabled      [Disable]   │           │
│  │ BT Pairing Code: [1234____]         │           │
│  └─────────────────────────────────────┘           │
│                                                     │
│          [SAVE]          [CANCEL]                   │
├─────────────────────────────────────────────────────┤
│              Global Navigation                      │
└─────────────────────────────────────────────────────┘
```

**Common NMEA 2000 PGNs:**

**GPS Position:**
- `129029` - GNSS Position Data (default)
- `129025` - Position, Rapid Update

**Compass/Heading:**
- `127250` - Vessel Heading (default, includes magnetic & true heading)
- `127251` - Rate of Turn
- `127257` - Attitude (pitch, roll, yaw)

**Configuration Details:**

**Compass PGN (127250):**
- **Fields:** Heading (magnetic), Deviation, Variation, Heading (true)
- **Update Rate:** 10 Hz typical
- **Source:** Magnetic compass, GPS compass, or fluxgate sensor
- **Usage:** Displayed on INFO screen compass rose

**Color Configuration:**
- **16-Color Palette:** Quick selection from standard marine colors
- **Custom Hex:** Enter 6-digit hex code (e.g., `ADD8E6` for light blue)
- **Settings Saved:** Stored in ESP32 NVS (Non-Volatile Storage)
- **Applies To:** All screens globally

**Actions:**
- **+/- Buttons:** Adjust numeric values
- **Dropdown:** Select units, colors, or PGN options
- **Edit Button:** Open PGN/hex code editor
- **Checkbox:** Toggle enable/disable options
- **Text Input:** Enter boat name, WiFi credentials
- **SAVE:** Save configuration to ESP32 NVS flash memory
- **CANCEL:** Discard changes
- **Swipe Left:** Navigate to PGN screen
- **Swipe Right:** Navigate to UPDATE screen

**Persistent Storage:**
- All settings saved to ESP32 NVS
- Survives power cycles and firmware updates
- Automatic backup to TF card (optional)

---

### 7. UPDATE SCREEN (Firmware Update)

**Purpose:** Firmware update from TF card

**Page Indicator:** ▶ ▶ ▶ ▶ ▶ ◀

**Content:**
- **Status:** Update file detection and progress
- **Progress Bar:** Update percentage
- **Instructions:** User guidance
- **Warning:** Do not power off during update

**Layout:**
```
┌─────────────────────────────────────────────────────┐
│              FIRMWARE UPDATE                        │
├─────────────────────────────────────────────────────┤
│                                                     │
│              ⚠️ FIRMWARE UPDATE MODE                │
│                                                     │
│  Status: update.bin detected on TF card            │
│  Size: 2.4 MB                                       │
│  Current Version: 1.0.0                             │
│  New Version: 1.1.0                                 │
│                                                     │
│  ┌─────────────────────────────────────┐           │
│  │ Progress: ████████░░░░░░░░  45%     │           │
│  └─────────────────────────────────────┘           │
│                                                     │
│  Flashing firmware... Do not power off!            │
│                                                     │
│  ⚠️ WARNING:                                        │
│  • Keep device powered during update                │
│  • Do not remove TF card                           │
│  • Device will restart automatically                │
│                                                     │
│                [START UPDATE]                       │
│                                                     │
├─────────────────────────────────────────────────────┤
│              Global Navigation                      │
└─────────────────────────────────────────────────────┘
```

**States:**
- **File Detected:** Ready to update (manual start)
- **Updating:** Progress bar, no user input
- **Complete:** Success message, auto-restart
- **Failed:** Error message, option to retry

**Actions:**
- **START UPDATE:** Begin firmware flash
- **Auto-detect:** If `update.bin` exists on boot → direct to UPDATE screen
- **Swipe Left:** Navigate to CONFIG screen (only when not updating)
- **Swipe Right:** Navigate to TOOLS screen

---

### 8. TOOLS SCREEN (System Utilities)

**Purpose:** System maintenance and diagnostics

**Page Indicator:** ▶ ▶ ▶ ▶ ▶ ▶

**Content:**
- **9 Tool Buttons (3×3 grid):**
  1. **TF Card** - Format SD card
  2. **Logs** - Browse/clear logs and set log level (opens sub-menu)
  3. **Clear GPS Track** - Delete GPS history
  4. **CONFIG** - System configuration (consolidated save/load)
  5. **WiFi/BT** - WiFi and Bluetooth configuration
  6. **System Info** - View diagnostics (firmware version, uptime, memory, etc.)
  7. **Test Hardware** - Hardware test mode
  8. **Factory Reset** - Reset to defaults
  9. **Date/Time Settings** - Configure date, time, and timezone

**Layout:**
```
┌─────────────────────────────────────────────────────┐
│                 SYSTEM TOOLS                        │
├─────────────────────────────────────────────────────┤
│                                                     │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌───────┐ │
│  │   [F]   │  │   [L]   │  │   [T]   │  │  [S]  │ │
│  │ Format  │  │  View   │  │  Clear  │  │ Save  │ │
│  │TF Card  │  │  Logs   │  │GPS Track│  │Config │ │
│  └─────────┘  └─────────┘  └─────────┘  └───────┘ │
│                                                     │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌───────┐ │
│  │   [i]   │  │   [H]   │  │   [L]   │  │  [R]  │ │
│  │ System  │  │  Test   │  │  Load   │  │Factory│ │
│  │  Info   │  │Hardware │  │Config   │  │ Reset │ │
│  └─────────┘  └─────────┘  └─────────┘  └───────┘ │
│                                                     │
│  ┌─────────────────────────────────────┐           │
│  │ Firmware: 1.0.0  Uptime: 0h 12m    │           │
│  │ Memory: 128 KB free                │           │
│  │ TF Card: 32 GB (24 GB free)        │           │
│  │ GPS Tracks: 15 files  Logs: 2.4 MB │           │
│  │ Config: config.json (2.1 KB)       │           │
│  └─────────────────────────────────────┘           │
│                                                     │
├─────────────────────────────────────────────────────┤
│              Global Navigation                      │
└─────────────────────────────────────────────────────┘
```

**Tool Functions:**

**Save Config (Backup):**
- Reads current configuration from NVS
- Exports to TF card as `/config.json`
- JSON format for human-readable backup
- Shows confirmation dialog with file size
- Status: "Config saved to TF card ✓"

**Load Config (Restore):**
- Reads `/config.json` from TF card
- Validates JSON structure
- Shows preview dialog with changes to be applied
- User confirms: "Apply these settings?"
- Writes to NVS and applies immediately
- Restart required for full effect
- Status: "Config loaded from TF card ✓"

**Config File Format (`/config.json`):**
```json
{
  "version": "1.0.0",
  "alarm_distance": 50,
  "distance_unit": "feet",
  "gps_pgn": 129029,
  "compass_pgn": 127250,
  "use_external_gps": false,
  "boat_name": "My Boat",
  "wifi_ssid": "",
  "wifi_password": "",
  "bluetooth_enabled": false,
  "bluetooth_pairing_code": "1234",
  "background_color": "0x0074D9",
  "font_color": "0xFFFFFF"
}
```

**Actions:**
- **Click Tool Button:** Execute corresponding function or open sub-screen
- **Save Config:** Export NVS → TF card JSON
- **Load Config:** Import TF card JSON → NVS (with confirmation)
- **Test Hardware:** Opens **TEST screen**
- **Swipe Left:** Navigate to UPDATE screen
- **BACK Button:** Return to START screen

---

## TOOLS Sub-Screens

All tool sub-screens accessible from the TOOLS screen. Each sub-screen includes a BACK button to return to TOOLS.

### 9A. TF CARD SCREEN (SD Card Management)

**Purpose:** Manage SD card operations

**Access:** From TOOLS screen → Click "TF Card" button

**Content:**
- **TF Card status indicator**
- **Card information (total space, free space)**
- **Operation buttons:**
  - Show Contents (file browser)
  - Format Card (with confirmation)
  - Test FileIO (read/write test)
- **Back button**

**Layout:**
```
┌─────────────────────────────────────────────────────┐
│                   TF CARD                           │
├─────────────────────────────────────────────────────┤
│                                                     │
│  Status: ✓ Mounted                                 │
│  Total: 32.0 GB  Free: 28.5 GB                     │
│                                                     │
│  ┌─────────────────────────────────────┐           │
│  │     SHOW CONTENTS                   │           │
│  └─────────────────────────────────────┘           │
│                                                     │
│  ┌─────────────────────────────────────┐           │
│  │     FORMAT CARD                     │           │
│  └─────────────────────────────────────┘           │
│                                                     │
│  ┌─────────────────────────────────────┐           │
│  │     TEST FILE I/O                   │           │
│  └─────────────────────────────────────┘           │
│                                                     │
│              [BACK TO TOOLS]                        │
└─────────────────────────────────────────────────────┘
```

**File Browser (from Show Contents):**
- Lists files and directories on SD card
- Shows file name, size, and type
- 10 files per page with scroll
- Back button returns to TF Card screen

**Actions:**
- **Show Contents:** Opens file browser showing SD card contents
- **Format Card:** Shows confirmation dialog, formats SD card, deletes all files
- **Test FileIO:** Runs read/write speed test on SD card
- **Back:** Return to TOOLS screen

---

### 9B. DATE/TIME SETTINGS SCREEN

**Purpose:** Configure system date, time, and timezone

**Access:** From TOOLS screen → Click "Date/Time Settings" button

**Content:**
- **Current time display (updates every second from RTC)**
- **Manual date/time rollers (year, month, day, hour, minute, second)**
- **Timezone selector (-12 to +14)**
- **GPS Time Sync checkbox (true/false)**
- **Save/Back buttons**

**Layout:**
```
┌─────────────────────────────────────────────────────┐
│ ⚓ ANCHOR DRAG ALARM            14:30:45            │ ← Header with clock
├─────────────────────────────────────────────────────┤
│              DATE/TIME SETTINGS                     │
│                                                     │
│  Current Time: 2025-12-26 14:30:45 (UTC-6)         │
│                                                     │
│  Year    Month    Day     Hour    Min     Sec      │
│  [2025]  [Dec ]   [26]    [14]    [30]    [45]     │
│    ▲       ▲       ▲       ▲       ▲       ▲       │
│    ▼       ▼       ▼       ▼       ▼       ▼       │
│                                                     │
│  Timezone: [-6]                                     │
│              ▲                                      │
│              ▼                                      │
│                                                     │
│  [✓] GPS Time Sync                                 │
│                                                     │
│                                                     │
│     [SAVE]              [BACK]                      │
└─────────────────────────────────────────────────────┘
```

**Time/Date Logic (Implementation, not shown on screen):**
- **RTC always stores UTC time** (PCF85063A Real-Time Clock)
- **Display shows local time** = UTC + timezone offset
- **Manual entry shows and sets local time**
- **Save converts local time back to UTC** before storing in RTC
- **GPS Sync checkbox behavior:**
  - **If true:** Enable automatic GPS time synchronization
    - Check GPS time every 15 minutes
    - Auto-sync RTC if drift > 5 seconds from GPS time
    - Fall back to RTC if GPS signal lost
  - **If false:** Disable GPS sync, use only manual time settings

**Actions:**
- **Rollers:** Adjust date/time values (changes local time display)
- **Timezone Roller:** Select UTC offset (-12 to +14)
- **GPS Sync Checkbox:** Toggle automatic GPS time synchronization on/off
- **Save:** Convert local time to UTC, store in RTC and NVS, return to TOOLS
- **Back:** Discard changes, return to TOOLS screen

---

### 9C. SYSTEM INFO SCREEN

**Purpose:** Display system diagnostics and hardware information

**Access:** From TOOLS screen → Click "System Info" button

**Content:**
- **ESP-IDF version**
- **Chip information (model, revision, cores)**
- **Flash size and type**
- **PSRAM status**
- **Memory usage (free heap, minimum free heap, PSRAM free)**
- **Back button**

**Layout:**
```
┌─────────────────────────────────────────────────────┐
│                 SYSTEM INFO                         │
├─────────────────────────────────────────────────────┤
│                                                     │
│  ESP-IDF Version: v5.5                             │
│  Chip: esp32s3 Rev 0                               │
│  Cores: 2                                           │
│  Flash: 16 MB embedded                             │
│  PSRAM: Yes                                         │
│                                                     │
│  Free Heap: 245 KB                                  │
│  Min Free Heap: 198 KB                             │
│  PSRAM Free: 7854 KB                               │
│                                                     │
│                                                     │
│              [BACK TO TOOLS]                        │
└─────────────────────────────────────────────────────┘
```

**Actions:**
- **Back:** Return to TOOLS screen

---

### 9D. TEST HARDWARE SCREEN

**Purpose:** Hardware test utilities and diagnostics

**Access:** From TOOLS screen → Click "Test Hardware" button

**Content:**
- **Placeholder for future hardware tests:**
  - Display test pattern
  - Touch calibration
  - SD card read/write test
  - GPS signal test
  - Buzzer test

**Layout:**
```
┌─────────────────────────────────────────────────────┐
│              TEST HARDWARE                          │
├─────────────────────────────────────────────────────┤
│                                                     │
│                                                     │
│  Hardware test options coming soon:                 │
│                                                     │
│  - Display test pattern                            │
│  - Touch calibration                               │
│  - SD card read/write test                         │
│  - GPS signal test                                  │
│  - Buzzer test                                      │
│                                                     │
│                                                     │
│                                                     │
│              [BACK TO TOOLS]                        │
└─────────────────────────────────────────────────────┘
```

**Actions:**
- **Back:** Return to TOOLS screen

---

### 9E. LOGS MENU SCREEN

**Purpose:** Access log viewing, clearing, and configuration

**Access:** From TOOLS screen → Click "Logs" button

**Content:**
- **Three large menu buttons:**
  1. **VIEW LOGS** - Browse system logs and GPS tracking data
  2. **CLEAR LOGS** - Delete all logs with confirmation (red/danger button)
  3. **SET LOG LEVEL** - Configure system log verbosity (purple/config button)

**Layout:**
```
┌─────────────────────────────────────────────────────┐
│                    LOGS                             │
├─────────────────────────────────────────────────────┤
│                                                     │
│            ┌─────────────────┐                      │
│            │   VIEW LOGS     │  (Blue)              │
│            └─────────────────┘                      │
│                                                     │
│            ┌─────────────────┐                      │
│            │   CLEAR LOGS    │  (Red)               │
│            └─────────────────┘                      │
│                                                     │
│            ┌─────────────────┐                      │
│            │  SET LOG LEVEL  │  (Purple)            │
│            └─────────────────┘                      │
│                                                     │
│  [BACK]                                             │
└─────────────────────────────────────────────────────┘
```

**Actions:**
- **View Logs:** Navigate to VIEW LOGS screen
- **Clear Logs:** Navigate to CLEAR LOGS confirmation screen
- **Set Log Level:** Navigate to SET LOG LEVEL selection screen
- **Back:** Return to TOOLS screen

---

### 9E-1. VIEW LOGS SCREEN

**Purpose:** Display recent system logs and events

**Access:** From LOGS MENU → Click "VIEW LOGS" button

**Content:**
- **Placeholder for log viewing functionality:**
  - Recent system logs
  - GPS tracking data
  - Event history

**Layout:**
```
┌─────────────────────────────────────────────────────┐
│                 VIEW LOGS                           │
├─────────────────────────────────────────────────────┤
│                                                     │
│                                                     │
│  Log viewing functionality coming soon.             │
│                                                     │
│  Will display recent system logs                    │
│  and GPS tracking data.                            │
│                                                     │
│                                                     │
│                                                     │
│  [BACK]                                             │
└─────────────────────────────────────────────────────┘
```

**Actions:**
- **Back:** Return to LOGS MENU

---

### 9E-2. CLEAR LOGS SCREEN

**Purpose:** Delete all system logs with confirmation

**Access:** From LOGS MENU → Click "CLEAR LOGS" button

**Content:**
- **Warning message about permanent deletion**
- **CLEAR ALL LOGS button** (red/danger)
- **BACK button**

**Layout:**
```
┌─────────────────────────────────────────────────────┐
│                 CLEAR LOGS                          │
├─────────────────────────────────────────────────────┤
│                                                     │
│  WARNING: This will permanently delete              │
│  all system logs and GPS track data.                │
│                                                     │
│  This action cannot be undone.                      │
│                                                     │
│                                                     │
│            ┌──────────────────────┐                 │
│            │  CLEAR ALL LOGS      │  (Red)          │
│            └──────────────────────┘                 │
│                                                     │
│  [BACK]                                             │
└─────────────────────────────────────────────────────┘
```

**Actions:**
- **Clear All Logs:** Delete all logs and return to LOGS MENU
- **Back:** Return to LOGS MENU without changes

---

### 9E-3. SET LOG LEVEL SCREEN

**Purpose:** Configure system log verbosity

**Access:** From LOGS MENU → Click "SET LOG LEVEL" button

**Content:**
- **Current log level display**
- **Six log level buttons** (2 columns × 3 rows):
  - **NONE** - No logging
  - **ERROR** - Errors only
  - **WARN** - Warnings & errors
  - **INFO** - Informational (default)
  - **DEBUG** - Detailed debug
  - **VERBOSE** - Maximum detail
- **Active level highlighted in green**

**Layout:**
```
┌─────────────────────────────────────────────────────┐
│               SET LOG LEVEL                         │
├─────────────────────────────────────────────────────┤
│                                                     │
│         Current Level: INFO                         │
│                                                     │
│   ┌──────────┐              ┌──────────┐           │
│   │   NONE   │              │   INFO   │           │
│   │No logging│              │Informati…│ (Green)    │
│   └──────────┘              └──────────┘           │
│                                                     │
│   ┌──────────┐              ┌──────────┐           │
│   │  ERROR   │              │  DEBUG   │           │
│   │Errors…   │              │Detailed…│            │
│   └──────────┘              └──────────┘           │
│                                                     │
│   ┌──────────┐              ┌──────────┐           │
│   │   WARN   │              │ VERBOSE  │           │
│   │Warnings…│              │Maximum…  │            │
│   └──────────┘              └──────────┘           │
│                                                     │
│  [BACK]                                             │
└─────────────────────────────────────────────────────┘
```

**Actions:**
- **Select Level:** Click any log level button to change system verbosity
- **Back:** Return to LOGS MENU

**Log Levels:**
- **NONE (0):** No logging (silent)
- **ERROR (1):** Only critical errors
- **WARN (2):** Warnings and errors
- **INFO (3):** General information, warnings, and errors (default)
- **DEBUG (4):** Detailed debugging information
- **VERBOSE (5):** Maximum detail for troubleshooting

---

### 9F. CLEAR GPS TRACK SCREEN

**Purpose:** Delete all GPS tracking data with confirmation

**Access:** From TOOLS screen → Click "Clear GPS Track" button

**Content:**
- **Warning message**
- **Confirmation prompt**
- **Clear/Cancel buttons**

**Layout:**
```
┌─────────────────────────────────────────────────────┐
│              CLEAR GPS TRACK                        │
├─────────────────────────────────────────────────────┤
│                                                     │
│                                                     │
│  This will clear all GPS tracking data.            │
│                                                     │
│  This action cannot be undone.                     │
│                                                     │
│  Continue?                                          │
│                                                     │
│                                                     │
│     [CANCEL]              [CLEAR]                   │
└─────────────────────────────────────────────────────┘
```

**Actions:**
- **Clear:** Delete all GPS tracking data, return to TOOLS
- **Cancel:** Return to TOOLS without changes

---

### 9G. SAVE CONFIG SCREEN

**Purpose:** Save current configuration to SD card

**Access:** From TOOLS screen → Click "Save Config" button

**Content:**
- **Information about config save functionality**
- **Status message**
- **Back button**

**Layout:**
```
┌─────────────────────────────────────────────────────┐
│                 SAVE CONFIG                         │
├─────────────────────────────────────────────────────┤
│                                                     │
│                                                     │
│  Configuration save functionality                   │
│  coming soon.                                       │
│                                                     │
│  Will save settings to SD card.                     │
│                                                     │
│                                                     │
│                                                     │
│              [BACK TO TOOLS]                        │
└─────────────────────────────────────────────────────┘
```

**Future Implementation:**
- Reads current configuration from NVS
- Exports to SD card as `/config.json`
- Shows confirmation with file size
- Status: "Config saved to SD card ✓"

**Actions:**
- **Back:** Return to TOOLS screen

---

### 9H. LOAD CONFIG SCREEN

**Purpose:** Load configuration from SD card

**Access:** From TOOLS screen → Click "Load Config" button

**Content:**
- **Information about config load functionality**
- **Status message**
- **Back button**

**Layout:**
```
┌─────────────────────────────────────────────────────┐
│                 LOAD CONFIG                         │
├─────────────────────────────────────────────────────┤
│                                                     │
│                                                     │
│  Configuration load functionality                   │
│  coming soon.                                       │
│                                                     │
│  Will load settings from SD card.                   │
│                                                     │
│                                                     │
│                                                     │
│              [BACK TO TOOLS]                        │
└─────────────────────────────────────────────────────┘
```

**Future Implementation:**
- Reads `/config.json` from SD card
- Validates JSON structure
- Shows preview dialog with changes
- User confirms before applying
- Writes to NVS and applies immediately

**Actions:**
- **Back:** Return to TOOLS screen

---

### 9I. FACTORY RESET SCREEN

**Purpose:** Reset all settings to factory defaults with confirmation

**Access:** From TOOLS screen → Click "Factory Reset" button

**Content:**
- **Strong warning message**
- **Confirmation prompt**
- **Reset/Cancel buttons**

**Layout:**
```
┌─────────────────────────────────────────────────────┐
│              FACTORY RESET                          │
├─────────────────────────────────────────────────────┤
│                                                     │
│  WARNING!                                           │
│                                                     │
│  This will reset all settings to defaults.          │
│                                                     │
│  All configuration will be lost.                    │
│                                                     │
│  This action cannot be undone.                     │
│                                                     │
│  Continue?                                          │
│                                                     │
│     [CANCEL]              [RESET]                   │
└─────────────────────────────────────────────────────┘
```

**Actions:**
- **Reset:** Clear all NVS settings, restore factory defaults, return to TOOLS
- **Cancel:** Return to TOOLS without changes

---

### 9J. WIFI/BLUETOOTH CONFIG SCREEN 🆕

**Purpose:** Configure WiFi networks and Bluetooth pairing

**Access:** From TOOLS screen → Click "WiFi/Bluetooth Config" button (under TOOLS)

**Content:**
- **WiFi Section:**
  - Network scanner showing available WiFi networks
  - Option to create adhoc network "anchor-drag-alarm"
  - Password setting for adhoc network
  - Connect/disconnect controls
- **Bluetooth Section:**
  - Bluetooth enable/disable toggle
  - Pairing menu with device discovery
  - Paired devices list
  - Unpair option

**Layout:**
```
┌─────────────────────────────────────────────────────┐
│           WIFI/BLUETOOTH CONFIG                     │
├─────────────────────────────────────────────────────┤
│                                                     │
│  WIFI NETWORKS                                      │
│  ┌─────────────────────────────────────┐           │
│  │ ○ Home-Network-5G    [-70 dBm] 🔒  │ [Connect] │
│  │ ○ CoffeeShop-Guest   [-82 dBm]     │ [Connect] │
│  │ ● anchor-drag-alarm  [-45 dBm] 🔒  │ Connected │
│  │                             [Scan]  │           │
│  └─────────────────────────────────────┘           │
│                                                     │
│  CREATE ADHOC NETWORK                               │
│  ┌─────────────────────────────────────┐           │
│  │ SSID: anchor-drag-alarm             │           │
│  │ Password: [**********]      [Show]  │           │
│  │                    [Create Network] │           │
│  └─────────────────────────────────────┘           │
│                                                     │
│  BLUETOOTH                                          │
│  ┌─────────────────────────────────────┐           │
│  │ Status: [✓] Enabled                 │           │
│  │                                     │           │
│  │ PAIRED DEVICES:                     │           │
│  │  • iPhone 12          [Disconnect]  │           │
│  │  • Garmin Chartplotter [Disconnect] │           │
│  │                                     │           │
│  │              [Scan for Devices]     │           │
│  └─────────────────────────────────────┘           │
│                                                     │
│              [BACK TO TOOLS]                        │
└─────────────────────────────────────────────────────┘
```

**WiFi Network Scanning:**
- Displays list of available WiFi networks with signal strength
- Shows security status (🔒 for secured networks)
- Signal strength in dBm (-30 to -90 dBm range)
- Active connection indicated with filled circle (●)
- Scan button refreshes network list

**Adhoc Network Creation:**
- Fixed SSID: "anchor-drag-alarm"
- User-configurable password (8-63 characters)
- Password visibility toggle
- Creates WPA2 secured adhoc network
- Allows direct connection from mobile devices for monitoring
- Network remains active until disabled or device powered off

**Bluetooth Pairing:**
- Enable/disable Bluetooth radio
- Device discovery scan (shows nearby Bluetooth devices)
- Pairing request with PIN confirmation
- List of currently paired devices
- Unpair/disconnect options for each device
- Supports Bluetooth Classic and BLE

**Security Notes:**
- WiFi passwords stored encrypted in NVS
- Bluetooth pairing codes use secure pairing (SSP)
- Adhoc network password minimum 8 characters
- Failed pairing attempts are logged

**Actions:**
- **Connect (WiFi):** Connect to selected WiFi network (prompts for password if secured)
- **Scan (WiFi):** Refresh available networks list
- **Create Network:** Start adhoc WiFi network with specified password
- **Enable Bluetooth:** Turn on Bluetooth radio and make device discoverable
- **Scan for Devices:** Search for nearby Bluetooth devices to pair
- **Disconnect:** Disconnect from WiFi network or unpair Bluetooth device
- **Back:** Return to TOOLS screen

---

### 9. TEST SCREEN (Hardware Testing) 🆕

**Purpose:** Comprehensive hardware testing and data source monitoring

**Access:** From TOOLS screen → Click "Test Hardware" button

**Content:**
- **Left Column:** Hardware test controls
  - Buzzer toggle
  - Relay toggle
  - Alarm panel toggle
  - LED state cycling
- **Right Column:** Data source indicators
  - GPS status
  - Compass status
  - Wind status
  - Water speed status

**Layout:**
```
┌─────────────────────────────────────────────────────┐
│              HARDWARE TEST                          │
├─────────────────────────────────────────────────────┤
│                                                     │
│  ┌───────────┐     ┌─────────────────────────────┐ │
│  │  BUZZER:  │     │    DATA SOURCES             │ │
│  │    OFF    │     │                             │ │
│  └───────────┘     │  GPS: ACTIVE (N2K)          │ │
│                    │  Sats: 8 | Lat: 30.031355   │ │
│  ┌───────────┐     │                             │ │
│  │  RELAY:   │     │  COMPASS: NO DATA           │ │
│  │    OFF    │     │                             │ │
│  └───────────┘     │                             │ │
│                    │  WIND: NO DATA              │ │
│  ┌───────────┐     │                             │ │
│  │  ALARM:   │     │                             │ │
│  │    OFF    │     │  WATER SPEED: NO DATA       │ │
│  └───────────┘     │                             │ │
│                    │                             │ │
│  ┌───────────┐     │                             │ │
│  │   LED:    │     │  [Updates every 1 second]   │ │
│  │    OFF    │     │                             │ │
│  └───────────┘     └─────────────────────────────┘ │
│                                                     │
├─────────────────────────────────────────────────────┤
│ ◀ BACK TO TOOLS                                     │
└─────────────────────────────────────────────────────┘
```

**Hardware Controls:**
- **BUZZER Button:** Toggle ON/OFF
  - OFF state: Blue button
  - ON state: Red button
  - Sends signal to N2K API → Web interface LED lights up

- **RELAY Button:** Toggle ON/OFF
  - OFF state: Blue button
  - ON state: Yellow button
  - Controls external relay output

- **ALARM Button:** Toggle ON/OFF
  - OFF state: Blue button
  - ON state: Red button (BLINKING_RED LED state)
  - Simulates alarm condition

- **LED Button:** Cycle through states
  - Click to cycle: OFF → RED → YELLOW → GREEN → BLINKING RED → OFF
  - Button color changes to match LED state

**Data Source Indicators:**
All indicators update every 1 second with real-time data:

- **GPS:**
  - Shows "GPS: ACTIVE (N2K)" when receiving data
  - Displays satellite count and current latitude
  - Green text when valid, gray when no data
  - Source: N2K simulator TCP port 10110

- **COMPASS:**
  - Shows heading when N2K compass data available
  - Placeholder: "COMPASS: NO DATA"

- **WIND:**
  - Shows wind speed/direction when N2K wind data available
  - Placeholder: "WIND: NO DATA"

- **WATER SPEED:**
  - Shows speed through water when N2K data available
  - Placeholder: "WATER SPEED: NO DATA"

**Bidirectional Communication:**
```
TEST Screen Button Click
  ↓
hardware_sim_set_*() function
  ↓
HTTP POST to N2K API (localhost:9001)
  ↓
N2K Simulator Web Interface
  ↓
Hardware Status Panel LEDs Update
```

**Actions:**
- **Click Hardware Buttons:** Toggle/cycle hardware outputs
- **BACK Button:** Return to TOOLS screen
- **Real-time Updates:** GPS and sensor data refresh every second

---

## Color Scheme

### Brand Colors
| Color Name | Hex Code | Usage |
|------------|----------|-------|
| Marine Dark | `#001f3f` | Header bar, panel backgrounds |
| Marine Blue | `#0074D9` | Default screen background (configurable) |
| Light Blue | `#ADD8E6` | DISPLAY screen background |
| Ocean Teal | `#39CCCC` | Highlights, active elements |
| Alarm Red | `#FF4136` | Alerts, danger states |
| Safe Green | `#2ECC40` | Success, safe states |
| Warning Yellow | `#FFDC00` | Warnings, caution states |
| Text White | `#FFFFFF` | Text on dark backgrounds |
| Text Black | `#000000` | Text on light backgrounds (configurable) |
| Text Gray | `#AAAAAA` | Secondary text, disabled |

### State Colors
| State | Color | Usage |
|-------|-------|-------|
| OFF/Disabled | Gray | Inactive mode |
| Normal/Armed | Green | Safe, operational |
| Warning | Yellow | Caution, intermediate state |
| Alarm | Red | Critical, emergency |
| Active | Teal | Selected, highlighted |

---

## Operating Modes

### Mode: OFF
- **Purpose:** System disabled
- **LED:** OFF (no light)
- **Display:** Grey background on START screen
- **Screen:** Stays on START screen
- **Behavior:** No monitoring, no alarms

### Mode: READY
- **Purpose:** Monitor for anchor drag
- **LED:**
  - YELLOW (arming - collecting GPS points)
  - GREEN (armed - monitoring)
  - BLINKING RED (alarm - drag detected)
- **Display:** Light blue background with black text
- **Screen:** DISPLAY screen showing GPS plotting and anchor status
- **Behavior:**
  - **Real-time GPS plotting at 2Hz**
  - **Monitors GPS position relative to anchor**
  - **Tracks distance from anchor circle**
  - **Triggers alarm when threshold exceeded**
  - **Logs GPS trail to TF card** (CSV format: `YYYYMMDD_HHMMSS.csv`)
  - **Logs syslog events** (10MB rotation)

### States within READY Mode:
1. **Main Monitoring:** GPS data displayed, waiting for anchor button press
2. **Anchor Locating (Arming):** Collecting GPS points to establish anchor circle (60 seconds default)
3. **Anchor Armed:** Monitoring anchor position with real-time boat tracking (green indicator)
4. **Alarm:** Anchor drag detected - boat exceeded alarm distance from anchor circle (red flashing)

---

## Screen Resolution & Layout

**Display:** 800×480 pixels (16:10 aspect ratio)

**Safe Areas:**
- Top Bar: 40px height
- Bottom Nav: 60px height
- Content Area: 800×380px
- Side Margins: 20px minimum

**Font Sizes:**
- Title: 24pt (Montserrat)
- Subtitle: 20pt
- Body: 16pt
- Small: 14pt
- Tiny: 12pt

**Touch Targets:**
- Minimum button size: 80×80px
- Recommended: 100×100px for primary actions
- Spacing between buttons: 10-20px

---

## Navigation Gestures

| Gesture | Action |
|---------|--------|
| **Swipe Left** | Previous page |
| **Swipe Right** | Next page |
| **Tap Button** | Activate function |
| **Press & Hold** | Additional options (future) |
| **ESC Key** | Back to previous screen (simulator only) |

---

## File References

### Screen Implementation Files
- `gui/splash_screen.cpp` - Splash screen
- `gui/mode_screen.cpp` - Mode selection (START)
- `gui/display_screen.cpp` - Main display
- `gui/info_screen.cpp` - Info screen
- `gui/pgn_screen.cpp` - PGN monitor
- `gui/config_screen.cpp` - Configuration
- `gui/update_screen.cpp` - Firmware update
- `gui/tools_screen.cpp` - System tools + TEST screen
- `gui/ui.cpp` - UI initialization and navigation
- `gui/ui.h` - UI declarations and app state

### Assets
- `/anchor_logo.bmp` - Splash screen logo (TF card)
- `/assets/anchor_icon.png` - Display screen anchor image (256×256)

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.5.0 | 2025-12-26 | WiFi/Bluetooth and header enhancements:<br>• **Added 9J. WiFi/Bluetooth Config Screen** - Network scanning, adhoc network creation, Bluetooth pairing menu<br>• **WiFi Network Scanning** - Displays available networks with signal strength (dBm), security status, and connect options<br>• **Adhoc Network Creation** - Fixed SSID "anchor-drag-alarm" with user-configurable password (8-63 chars)<br>• **Bluetooth Pairing Menu** - Device discovery, paired devices list, disconnect/unpair options<br>• **Security Features** - WiFi passwords encrypted in NVS, Bluetooth SSP pairing, minimum 8-char passwords<br>• **Global Header Bar Documentation** - Added comprehensive header section to Table of Contents<br>• **Header Icon Colors Updated** - Icons now backlight with **GREEN** when connected/active (was blue)<br>• **Header Icon Status** - Green (#00AA00) for connected/active, Gray (#808080) for inactive<br>• **Time Display** - Documented automatic RTC synchronization and local time display (UTC + offset) |
| 1.4.0 | 2025-12-26 | Tools screen expansion:<br>• **Added 9 new Tool sub-screens** accessible from TOOLS screen<br>• **9A. TF Card Screen** - SD card management with file browser, format, and test options<br>• **9B. Date/Time Settings Screen** - Complete time/timezone configuration with UTC/local time logic, GPS sync checkbox, and RTC fallback<br>• **9C. System Info Screen** - Hardware diagnostics (ESP-IDF version, chip info, flash, PSRAM, memory usage)<br>• **9D. Test Hardware Screen** - Placeholder for future hardware test utilities<br>• **9E. View Logs Screen** - Placeholder for system log viewing<br>• **9F. Clear GPS Track Screen** - GPS tracking data deletion with confirmation<br>• **9G. Save Config Screen** - Configuration backup to SD card (coming soon)<br>• **9H. Load Config Screen** - Configuration restore from SD card (coming soon)<br>• **9I. Factory Reset Screen** - Reset to defaults with strong warning confirmation<br>• **All tool screens include working BACK buttons** that return to TOOLS screen<br>• **Header clock fixed** - Uses snprintf instead of lv_label_set_text_fmt to prevent visual artifacts<br>• **Header clock updates every second on all pages** via RTC time update task in main.c |
| 1.3.0 | 2025-12-13 | Screen restructuring:<br>• **Split DISPLAY SCREEN into two separate screens:**<br>&nbsp;&nbsp;- **Screen 3: DISPLAY SCREEN (Ready to Anchor)** - Shows main monitoring before anchor is set (READY state)<br>&nbsp;&nbsp;- **Screen 3a: DISPLAY SCREEN (Anchoring)** - Shows GPS plotting and anchor tracking (ARMING/ARMED/ALARM states)<br>• **Screen 3 (Ready to Anchor):** Large anchor button in center, connection info, waiting for user to press anchor<br>• **Screen 3a (Anchoring):** Real-time GPS plotting canvas with anchor circle, boat position, and trail dots<br>• Updated Screen Index table to show both screens<br>• Clarified state transitions and navigation between screens |
| 1.2.0 | 2025-12-13 | UI refinements:<br>• **START Screen:** Swapped INFO and CONFIG button positions (INFO now 3rd, CONFIG now 4th)<br>• **START Screen:** Changed CONFIG button color from Blue to Purple (light purple gradient #ba68c8 → #9c27b0)<br>• **START Screen:** Enhanced all buttons with 3D styling (vertical gradients, drop shadows, border highlights, press animations)<br>• All buttons now have 15px rounded corners (increased from 12px)<br>• Added tactile press effect (shadow compresses from 8px to 3px offset when pressed) |
| 1.1.0 | 2025-12-13 | Major updates:<br>• Changed mode from "ON" to "READY" throughout UI<br>• Added real-time GPS plotting at 2Hz on DISPLAY screen<br>• Implemented anchor circle calculation and visualization<br>• Added boat icon tracking with trail dots<br>• Fixed GPS coordinate formatting (fixed-width: lat %9.6f, lon %10.6f, sats %2d)<br>• Added ENV variable support for N2K connection (N2K_HOST, N2K_PORT, N2K_HTTP_PORT)<br>• Documented NVS (Non-Volatile Storage) configuration system<br>• Added compass rose display (upper right, or "/\|" placeholder)<br>• Added real-time clock display (bottom right)<br>• Updated DISPLAY screen to show GPS plotting phases (Initial Collection → Anchor Circle → Real-Time Tracking)<br>• Added plotting algorithm documentation (coordinate transformation, trail management, auto-scale)<br>• Updated Operating Modes section with READY mode states<br>• Added GPS trail logging (CSV format) and syslog event logging (10MB rotation)<br>• **Self-Test:** Added N2K priority detection (skip NMEA 0183/external GPS if N2K present)<br>• **TOOLS Screen:** Added config backup/restore (Save/Load Config to TF card as JSON)<br>• Expanded TOOLS grid from 6 to 8 buttons (4×2 layout)<br>• **Global Navigation Bar:** Documented consistent NAV bar across all screens with page indicators<br>• **START Screen:** Added INFO button (top right corner) for quick access to INFO screen<br>• **Bluetooth:** Added pairing code to configuration (default: "1234", stored in NVS, included in config.json)<br>• **CONFIG Screen:** Updated to include bluetooth pairing code field in System Settings |
| 1.0.0 | 2025-12-13 | Initial UI specification with 9 screens |

---

## Configuration Storage (NVS)

**ESP32 Non-Volatile Storage** - All configuration settings persist across power cycles using the ESP-IDF NVS Flash component.

### NVS Implementation Details

- **Library:** ESP-IDF `nvs_flash` component
- **Primary Namespace:** `anchor_alarm`
- **Additional Namespaces:** `power_mgmt` (power management state)
- **Features:**
  - Automatic wear-leveling (flash longevity ~100,000 write cycles)
  - Version management for future migrations
  - Factory reset capability
  - Encrypted storage for sensitive data (WiFi passwords, BT pairing codes)
- **Partition:** 24KB NVS partition at 0x9000 (see `partitions.csv`)

### Complete NVS Variables Table

| Category | NVS Key | Type | Range/Options | Default | Description |
|----------|---------|------|---------------|---------|-------------|
| **SYSTEM SETTINGS** | | | | | |
| | `boat_name` | string | 1-32 chars | "Anchor Drag Alarm" | Vessel name displayed on screens |
| | `config_version` | uint8 | 0-255 | 1 | Config schema version for migrations |
| | `factory_reset_count` | uint16 | 0-65535 | 0 | Number of factory resets performed |
| **ALARM SETTINGS** | | | | | |
| | `alarm_dist` | uint16 | 25-500 | 50 | Alarm distance threshold (feet) |
| | `alarm_units` | uint8 | 0=ft, 1=yd, 2=m | 0 | Distance units (feet/yards/meters) |
| | `arming_time` | uint16 | 30-600 | 60 | GPS arming duration (seconds) |
| | `alarm_snooze_time` | uint16 | 30-600 | 300 | Alarm snooze duration (seconds) |
| | `buzzer_vol` | uint8 | 0-100 | 80 | Buzzer volume percentage |
| | `buzzer_pattern` | uint8 | 0-3 | 0 | 0=Continuous, 1=Pulse, 2=Triple beep, 3=SOS |
| **GPS SETTINGS** | | | | | |
| | `gps_source` | uint8 | 0-2 | 0 | 0=N2K priority, 1=I2C only, 2=RS485 only |
| | `gps_sync_enabled` | bool | 0-1 | 1 | Enable GPS time sync to RTC |
| | `gps_sync_drift_threshold` | uint8 | 1-60 | 5 | Sync if RTC drift > N seconds |
| | `gps_log_enabled` | bool | 0-1 | 1 | Enable GPS track logging to SD |
| | `gps_log_interval` | uint16 | 1-3600 | 5 | GPS log interval (seconds) |
| **NMEA 2000 SETTINGS** | | | | | |
| | `n2k_device_id` | uint8 | 0-252 | 42 | NMEA 2000 device address (0-252) |
| | `n2k_instance` | uint8 | 0-255 | 0 | Device instance number |
| | `n2k_manufacturer_code` | uint16 | 0-2046 | 2046 | Manufacturer code (2046=reserved) |
| | `n2k_unique_number` | uint32 | 0-2097151 | 123456 | 21-bit unique device number |
| | `n2k_pgn_gps` | uint32 | 0-999999 | 129029 | PGN for GPS position data |
| | `n2k_pgn_compass` | uint32 | 0-999999 | 127250 | PGN for compass/heading |
| | `n2k_pgn_wind` | uint32 | 0-999999 | 130306 | PGN for wind data |
| | `n2k_pgn_depth` | uint32 | 0-999999 | 128267 | PGN for depth data |
| | `n2k_transmit_enabled` | bool | 0-1 | 0 | Enable N2K transmission (not just receive) |
| **NMEA 0183 SETTINGS** | | | | | |
| | `nmea0183_enabled` | bool | 0-1 | 1 | Enable NMEA 0183 output via RS485 |
| | `nmea0183_baud` | uint32 | 4800/38400 | 4800 | NMEA 0183 baud rate |
| | `nmea0183_talker_id` | string | 2 chars | "GP" | Talker ID (GP=GPS, II=Integrated, AI=Alarm) |
| **WIFI SETTINGS** | | | | | |
| | `wifi_enabled` | bool | 0-1 | 0 | Enable WiFi (0=Off, 1=On) |
| | `wifi_mode` | uint8 | 0-2 | 1 | 0=Off, 1=AP mode, 2=Station mode |
| | `wifi_ap_ssid` | string | 1-32 chars | "anchor-drag-alarm" | AP mode SSID (fixed for adhoc) |
| | `wifi_ap_password` | blob | 8-63 chars | "12345678" | AP mode password (encrypted) |
| | `wifi_ap_channel` | uint8 | 1-13 | 6 | AP mode WiFi channel |
| | `wifi_ap_max_conn` | uint8 | 1-4 | 4 | AP mode max simultaneous connections |
| | `wifi_sta_ssid` | string | 1-32 chars | "" | Station mode SSID (network to join) |
| | `wifi_sta_password` | blob | 0-63 chars | "" | Station mode password (encrypted) |
| | `wifi_sta_auto_reconnect` | bool | 0-1 | 1 | Auto-reconnect to saved network |
| **BLUETOOTH SETTINGS** | | | | | |
| | `bt_enabled` | bool | 0-1 | 0 | Enable Bluetooth (0=Off, 1=On) |
| | `bt_device_name` | string | 1-32 chars | "Anchor Drag Alarm" | Bluetooth device name |
| | `bt_pairing_code` | blob | 4-16 chars | "1234" | Bluetooth pairing PIN (encrypted) |
| | `bt_discoverable` | bool | 0-1 | 1 | Allow device discovery |
| | `bt_paired_devices` | blob | JSON array | [] | List of paired device MAC addresses |
| | `bt_auto_reconnect` | bool | 0-1 | 1 | Auto-reconnect to last paired device |
| **DISPLAY SETTINGS - THEME** | | | | | |
| | `theme_mode` | uint8 | 0-2 | 0 | 0=Dark (current), 1=Light, 2=Auto |
| | `theme_auto_time_start` | uint16 | 0-1439 | 420 | Auto theme dark start (minutes since midnight, 7:00 AM) |
| | `theme_auto_time_end` | uint16 | 0-1439 | 1140 | Auto theme dark end (minutes since midnight, 7:00 PM) |
| **DISPLAY SETTINGS - COLORS** | | | | | |
| | `color_bg_screen` | uint32 | 24-bit RGB | 0x001F3F | Main screen background (dark navy) |
| | `color_primary` | uint32 | 24-bit RGB | 0x0074D9 | Primary accent color (marine blue) |
| | `color_primary_light` | uint32 | 24-bit RGB | 0x39CCCC | Titles/highlights (teal) |
| | `color_success` | uint32 | 24-bit RGB | 0x2ECC40 | Success/ready/armed (green) |
| | `color_warning` | uint32 | 24-bit RGB | 0xFFDC00 | Warning/caution (yellow) |
| | `color_danger` | uint32 | 24-bit RGB | 0xFF4136 | Alarm/error (red) |
| | `color_text_primary` | uint32 | 24-bit RGB | 0xFFFFFF | Primary text (white) |
| | `color_text_secondary` | uint32 | 24-bit RGB | 0xCCCCCC | Secondary text (light gray) |
| **DISPLAY SETTINGS - FONTS** | | | | | |
| | `font_family` | uint8 | 0-3 | 0 | 0=Orbitron, 1=Poppins, 2=GolosText, 3=SF Pro Rounded |
| | `font_size_title` | uint8 | 0-5 | 3 | Title font size: 0=14pt, 1=16pt, 2=20pt, 3=24pt, 4=32pt, 5=48pt |
| | `font_size_subtitle` | uint8 | 0-5 | 2 | Subtitle font size (same scale) |
| | `font_size_button` | uint8 | 0-5 | 2 | Button font size (same scale) |
| | `font_size_body` | uint8 | 0-5 | 1 | Body text font size (same scale) |
| **DISPLAY SETTINGS - BRIGHTNESS** | | | | | |
| | `brightness` | uint8 | 0-100 | 80 | LCD backlight brightness (%) |
| | `brightness_auto` | bool | 0-1 | 0 | Auto-brightness based on time |
| | `brightness_night` | uint8 | 0-100 | 20 | Night mode brightness (%) |
| | `brightness_night_start` | uint16 | 0-1439 | 1260 | Night mode start (minutes, 9:00 PM) |
| | `brightness_night_end` | uint16 | 0-1439 | 360 | Night mode end (minutes, 6:00 AM) |
| | `screen_timeout` | uint16 | 0-3600 | 0 | Screen auto-off timeout (seconds, 0=never) |
| **DATE/TIME SETTINGS** | | | | | |
| | `timezone_offset` | int8 | -12 to +14 | 0 | Timezone offset from UTC (hours) |
| | `dst_enabled` | bool | 0-1 | 0 | Enable Daylight Saving Time |
| | `dst_start_month` | uint8 | 1-12 | 3 | DST start month (March) |
| | `dst_end_month` | uint8 | 1-12 | 11 | DST end month (November) |
| | `time_format_24h` | bool | 0-1 | 1 | Time format: 0=12-hour, 1=24-hour |
| **POWER MANAGEMENT** | | | | | |
| | `power_save_enabled` | bool | 0-1 | 0 | Enable power save mode |
| | `power_save_timeout` | uint16 | 60-3600 | 300 | Idle time before sleep (seconds) |
| | `sleep_time` | int64 | timestamp | 0 | Last sleep timestamp (power_mgmt namespace) |
| **LOGGING SETTINGS** | | | | | |
| | `log_level` | uint8 | 0-5 | 3 | 0=None, 1=Error, 2=Warn, 3=Info, 4=Debug, 5=Verbose |
| | `log_to_sd` | bool | 0-1 | 1 | Enable logging to SD card |
| | `log_max_size` | uint32 | 1024-10485760 | 10485760 | Max log file size (bytes, default 10MB) |
| | `log_rotation` | bool | 0-1 | 1 | Enable log file rotation |

### Total NVS Variables: 79

**Categories Breakdown:**
- System: 3 variables
- Alarm: 6 variables
- GPS: 5 variables
- NMEA 2000: 9 variables
- NMEA 0183: 3 variables
- WiFi: 9 variables
- Bluetooth: 6 variables
- Theme: 3 variables
- Colors: 8 variables
- Fonts: 5 variables
- Brightness: 6 variables
- Date/Time: 5 variables
- Power: 3 variables
- Logging: 4 variables

### Font Configuration Details

**Available Font Families (4):**
1. **Orbitron** - Futuristic/technical, current default
2. **Poppins** - Clean and readable
3. **GolosText** - Geometric with excellent readability
4. **SF Pro Rounded** - Apple's rounded system font

**Font Sizes (6 available):**
- 0 = 14pt (Minimum - small body text, uses LVGL Montserrat)
- 1 = 16pt (Small - labels, body text)
- 2 = 20pt (Medium - buttons, subtitles)
- 3 = 24pt (Large - titles)
- 4 = 32pt (Extra large)
- 5 = 48pt (Huge - special emphasis)

**Current Font Assignments:**
- Title: Orbitron 24pt
- Subtitle: Orbitron 20pt
- Buttons: Orbitron 20pt (large), 16pt (small)
- Body: Orbitron 20pt (large), 16pt (normal), 14pt (small)

### Theme Modes

**Dark Theme (Current Default):**
- Background: Dark Navy (#001F3F)
- Text: White (#FFFFFF)
- Accents: Marine Blue (#0074D9), Teal (#39CCCC)
- Optimized for night operation on water

**Light Theme (Future):**
- Background: Light Gray (#F5F5F5)
- Text: Dark Gray (#333333)
- Accents: Bright Blue (#0080FF), Cyan (#00D4FF)
- Optimized for bright daylight conditions

**Auto Theme:**
- Switches between Dark/Light based on time of day
- Configurable transition times (default: 7 AM - 7 PM light, rest dark)
- Smooth color transitions during switch

### NVS Access Example (C)

```c
#include "nvs_flash.h"
#include "nvs.h"
#include "board_config.h"

// Initialize NVS
esp_err_t err = nvs_flash_init();
if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    nvs_flash_erase();
    err = nvs_flash_init();
}

// Open NVS handle
nvs_handle_t nvs_handle;
err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);

// Read settings
uint16_t alarm_distance = 50;  // default
nvs_get_u16(nvs_handle, NVS_KEY_ALARM_DISTANCE, &alarm_distance);

char boat_name[33] = "Anchor Drag Alarm";
size_t name_len = sizeof(boat_name);
nvs_get_str(nvs_handle, "boat_name", boat_name, &name_len);

uint8_t theme_mode = 0;  // default dark
nvs_get_u8(nvs_handle, "theme_mode", &theme_mode);

// Write settings
nvs_set_u16(nvs_handle, NVS_KEY_ALARM_DISTANCE, 75);
nvs_set_str(nvs_handle, "boat_name", "Sea Breeze");
nvs_set_u8(nvs_handle, "theme_mode", 2);  // auto mode

// Commit changes
nvs_commit(nvs_handle);

// Close handle
nvs_close(nvs_handle);
```

### Factory Reset Behavior

When factory reset is triggered from TOOLS screen:
1. Erases all keys in `anchor_alarm` namespace
2. Increments `factory_reset_count`
3. All settings revert to defaults listed in table above
4. WiFi/Bluetooth credentials are deleted
5. GPS logs and paired devices are cleared
6. System reboots to apply defaults

**Default Values:**
- Alarm Distance: 50 feet
- Distance Unit: Feet
- GPS PGN: 129029
- Compass PGN: 127250
- Boat Name: "My Boat"
- Bluetooth Enabled: False
- Bluetooth Pairing Code: "1234"
- Background Color: #0074D9 (Marine Blue)
- Font Color: #FFFFFF (White)

---

## N2K Simulator Connection

**Environment Variables** for configuring connection to N2K Data Simulator:

```bash
# Set in Makefile or shell environment
N2K_HOST=127.0.0.1        # Hostname/IP of N2K simulator (default: localhost)
N2K_PORT=10110            # TCP port for NMEA 0183 data (default: 10110)
N2K_HTTP_PORT=9001        # HTTP API port for control (default: 9001)
```

**Connection Details:**
- **NMEA Data Stream:** TCP socket connection on N2K_PORT
- **Control API:** HTTP REST API on N2K_HTTP_PORT
- **Data Format:** NMEA 0183 sentences (converted from N2K PGN 129029)
- **Update Rate:** 2Hz (500ms) for GPS data
- **Automatic Reconnection:** If connection lost, retry every 5 seconds

**Makefile Integration:**
```makefile
# Run simulator with custom N2K connection
make sim-run N2K_HOST=192.168.1.100 N2K_PORT=10110
```

---

**Document End**
