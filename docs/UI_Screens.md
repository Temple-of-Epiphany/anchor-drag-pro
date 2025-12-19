
# Anchor Drag Alarm - UI Screen Guide

**Version:** 1.3.0
**Author:** Colin Bitterfield
**Date Updated:** 2025-12-13
**Hardware:** Waveshare ESP32-S3-Touch-LCD-4.3B (800×480 IPS display)
**UI Framework:** LVGL 9.2.0
**Configuration Storage:** ESP32 NVS (Non-Volatile Storage)

---

## Table of Contents

1. [Screen Navigation Flow](#screen-navigation-flow)
2. [Screen Index](#screen-index)
3. [Screen Details](#screen-details)
4. [Color Scheme](#color-scheme)
5. [Operating Modes](#operating-modes)

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
- **8 Tool Buttons (4×2 grid):**
  1. **Format TF Card** - Format SD card
  2. **View Logs** - Browse system logs
  3. **Clear GPS Tracks** - Delete GPS history
  4. **Save Config** - Write config to TF card (backup)
  5. **System Info** - View diagnostics
  6. **Test Hardware** - Hardware test mode
  7. **Load Config** - Read config from TF card (restore)
  8. **Factory Reset** - Reset to defaults
- **System Status Panel:** Real-time system info

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
| 1.3.0 | 2025-12-13 | Screen restructuring:<br>• **Split DISPLAY SCREEN into two separate screens:**<br>&nbsp;&nbsp;- **Screen 3: DISPLAY SCREEN (Ready to Anchor)** - Shows main monitoring before anchor is set (READY state)<br>&nbsp;&nbsp;- **Screen 3a: DISPLAY SCREEN (Anchoring)** - Shows GPS plotting and anchor tracking (ARMING/ARMED/ALARM states)<br>• **Screen 3 (Ready to Anchor):** Large anchor button in center, connection info, waiting for user to press anchor<br>• **Screen 3a (Anchoring):** Real-time GPS plotting canvas with anchor circle, boat position, and trail dots<br>• Updated Screen Index table to show both screens<br>• Clarified state transitions and navigation between screens |
| 1.2.0 | 2025-12-13 | UI refinements:<br>• **START Screen:** Swapped INFO and CONFIG button positions (INFO now 3rd, CONFIG now 4th)<br>• **START Screen:** Changed CONFIG button color from Blue to Purple (light purple gradient #ba68c8 → #9c27b0)<br>• **START Screen:** Enhanced all buttons with 3D styling (vertical gradients, drop shadows, border highlights, press animations)<br>• All buttons now have 15px rounded corners (increased from 12px)<br>• Added tactile press effect (shadow compresses from 8px to 3px offset when pressed) |
| 1.1.0 | 2025-12-13 | Major updates:<br>• Changed mode from "ON" to "READY" throughout UI<br>• Added real-time GPS plotting at 2Hz on DISPLAY screen<br>• Implemented anchor circle calculation and visualization<br>• Added boat icon tracking with trail dots<br>• Fixed GPS coordinate formatting (fixed-width: lat %9.6f, lon %10.6f, sats %2d)<br>• Added ENV variable support for N2K connection (N2K_HOST, N2K_PORT, N2K_HTTP_PORT)<br>• Documented NVS (Non-Volatile Storage) configuration system<br>• Added compass rose display (upper right, or "/\|" placeholder)<br>• Added real-time clock display (bottom right)<br>• Updated DISPLAY screen to show GPS plotting phases (Initial Collection → Anchor Circle → Real-Time Tracking)<br>• Added plotting algorithm documentation (coordinate transformation, trail management, auto-scale)<br>• Updated Operating Modes section with READY mode states<br>• Added GPS trail logging (CSV format) and syslog event logging (10MB rotation)<br>• **Self-Test:** Added N2K priority detection (skip NMEA 0183/external GPS if N2K present)<br>• **TOOLS Screen:** Added config backup/restore (Save/Load Config to TF card as JSON)<br>• Expanded TOOLS grid from 6 to 8 buttons (4×2 layout)<br>• **Global Navigation Bar:** Documented consistent NAV bar across all screens with page indicators<br>• **START Screen:** Added INFO button (top right corner) for quick access to INFO screen<br>• **Bluetooth:** Added pairing code to configuration (default: "1234", stored in NVS, included in config.json)<br>• **CONFIG Screen:** Updated to include bluetooth pairing code field in System Settings |
| 1.0.0 | 2025-12-13 | Initial UI specification with 9 screens |

---

## Configuration Storage (NVS)

**ESP32 Non-Volatile Storage** - All configuration settings persist across power cycles:

### Stored Settings:
- **Alarm Settings:** Distance threshold, units (feet/yards/meters)
- **N2K Data:** GPS PGN (129029), Compass PGN (127250), External GPS toggle
- **Display:** Background color, font color (16-color palette or hex)
- **System:** Boat name, WiFi credentials, Bluetooth enable/disable, Bluetooth pairing code

### Storage Implementation:
- **Library:** ESP32 Preferences (NVS wrapper)
- **Namespace:** `anchor_alarm`
- **Features:**
  - Automatic wear-leveling (flash longevity)
  - Version management for future migrations
  - Factory reset capability
  - Individual getter/setter functions
  - Batch load/save operations

### Usage Files:
- `src/config_storage.h` - API declarations
- `src/config_storage.cpp` - Implementation (#ifdef ESP32)
- `docs/config_storage_usage.md` - Complete usage guide

### Access Methods:
```cpp
// Load all settings at startup
config_storage_t config;
config_storage_load(&config);

// Individual value access
uint16_t alarm_dist = config_get_alarm_distance();
config_set_boat_name("Sea Breeze");

// Save changes
config_storage_save(&config);
```

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
