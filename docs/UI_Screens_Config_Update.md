# CONFIG SCREEN SPECIFICATION UPDATE

## Version 2.0.0 - December 14, 2025

**Replaces:** Section "### 5. CONFIG SCREEN" in UI_Screens.md v1.3.0

---

### 5. CONFIG SCREEN (System Configuration)

**Purpose:** Configure GPS/compass sources, hardware components, and alarm settings

**Page Indicator:** ▶ ▶ ▶ ▶ ◀ ◀

**Content:**
- **GPS Source:** Select N2K, NMEA 0183, or External GPS
- **Compass Source:** Select N2K, NMEA 0183, or External compass
- **Hardware Components:** Enable/disable Alarm Panel, Push Button, Relay
- **Alarm Distance:** Set drift alert threshold (25-250 ft)
- **Save/Cancel:** Persist settings to ESP32 NVS flash

**Self-Test Behavior:**
- **If a component is DISABLED in config:** Self-test SKIPS that component
- **Example:** Alarm Panel OFF → Self-test shows "[--] Alarm Panel - Disabled in config"

---

**Layout:**
```
┌───────────────────────────────────────────────────────────────┐
│ ⚙️ CONFIGURATION                                    [BACK]    │
├───────────────────────────────────────────────────────────────┤
│                                                               │
│ GPS SOURCE                                                    │
│ ┌─────────────────────────────────────────────────────┐      │
│ │ Source: ⦿ N2K    ○ NMEA 0183    ○ External         │      │
│ │ ┌─ N2K Configuration ────────────────────────────┐  │      │
│ │ │ Device ID:  [0x00]  (0x00-0xFF)                │  │      │
│ │ │ PGN:        ⦿ 129029 (GNSS Data)               │  │      │
│ │ │             ○ 129025 (Rapid Update)            │  │      │
│ │ └────────────────────────────────────────────────┘  │      │
│ └─────────────────────────────────────────────────────┘      │
│                                                               │
│ COMPASS SOURCE                                                │
│ ┌─────────────────────────────────────────────────────┐      │
│ │ Source: ⦿ N2K    ○ NMEA 0183    ○ External         │      │
│ │ ┌─ N2K Configuration ────────────────────────────┐  │      │
│ │ │ Device ID:  [0x01]  (0x00-0xFF)                │  │      │
│ │ │ PGN:        ⦿ 127250 (Vessel Heading)          │  │      │
│ │ │             ○ 127251 (Rate of Turn)            │  │      │
│ │ └────────────────────────────────────────────────┘  │      │
│ └─────────────────────────────────────────────────────┘      │
│                                                               │
│ HARDWARE COMPONENTS                                           │
│ ┌─────────────────────────────────────────────────────┐      │
│ │  Alarm Panel    [●═══════○] ON   (Terminal 12 DO1) │      │
│ │  Push Button    [●═══════○] ON   (Terminal 15 DI0) │      │
│ │  Relay          [○═══════●] OFF  (Terminal 12 DO1) │      │
│ │                                                      │      │
│ │  ⚠️ Note: DO1 shared between Alarm Panel and Relay  │      │
│ │           Only one can be active at a time          │      │
│ └─────────────────────────────────────────────────────┘      │
│                                                               │
│ ALARM SETTINGS                                                │
│ ┌─────────────────────────────────────────────────────┐      │
│ │  Distance Threshold                                  │      │
│ │  25 ft ├────●──────────────────────┤ 250 ft          │      │
│ │        Current: 50 ft                                │      │
│ └─────────────────────────────────────────────────────┘      │
│                                                               │
│           [💾 SAVE CONFIGURATION]    [❌ CANCEL]              │
│                                                               │
├───────────────────────────────────────────────────────────────┤
│              Global Navigation                                │
└───────────────────────────────────────────────────────────────┘
```

---

## Configuration Details

### GPS Source Options

#### Option 1: N2K (NMEA 2000)
- **Device ID:** 0x00 (default) or user-specified (0x00-0xFF)
- **PGN:**
  - `129029` - GNSS Position Data (default, full GPS data)
  - `129025` - Position Rapid Update (fast updates, basic data)
- **When selected:** Show Device ID and PGN input fields
- **Self-test:** Check for PGN messages from specified device

#### Option 2: NMEA 0183
- **Device ID:** 0x00 (default) or user-specified (0x00-0xFF)
- **PGN:** Not applicable for 0183 (field hidden)
- **Sentences:** GPGGA, GPRMC, GPVTG (auto-detected)
- **When selected:** Show Device ID field only
- **Self-test:** Listen for NMEA 0183 sentences on RS485

#### Option 3: External GPS
- **Connection:** I2C via Terminals 5/6 (SDA/SCL)
- **Device:** NEO-8M or compatible GPS module
- **Address:** 0x42 (auto-detected)
- **When selected:** Hide Device ID and PGN fields
- **Self-test:** I2C scan for GPS module

---

### Compass Source Options

#### Option 1: N2K (NMEA 2000)
- **Device ID:** 0x01 (default) or user-specified (0x00-0xFF)
- **PGN:**
  - `127250` - Vessel Heading (default, magnetic & true heading)
  - `127251` - Rate of Turn (angular velocity)
- **When selected:** Show Device ID and PGN input fields
- **Self-test:** Check for PGN messages from specified device

#### Option 2: NMEA 0183
- **Device ID:** 0x01 (default) or user-specified (0x00-0xFF)
- **PGN:** Not applicable (field hidden)
- **Sentences:** HCHDM (magnetic), HCHDT (true heading)
- **When selected:** Show Device ID field only
- **Self-test:** Listen for NMEA 0183 heading sentences

#### Option 3: External Compass
- **Connection:** I2C via Terminals 5/6 (SDA/SCL)
- **Device:** QMC5883L, HMC5883L, or compatible magnetometer
- **Address:** 0x0D, 0x0E, or 0x1E (auto-detected)
- **When selected:** Hide Device ID and PGN fields
- **Self-test:** I2C scan for compass module

---

### Hardware Components

#### Alarm Panel (Terminal 12, DO1)
- **Toggle:** ON / OFF
- **Function:** Pull DO1 LOW to trigger external alarm panel
- **Wiring:** DO1 → Alarm Panel Trigger Input
- **Default:** OFF
- **Self-test:** If OFF → Skip alarm panel test
- **Conflict:** Shares DO1 with Relay (only one can be active)

#### Push Button (Terminal 15, DI0)
- **Toggle:** ON / OFF
- **Function:** Alarm mute/acknowledge button input
- **Wiring:** DI0 → Momentary Switch → DI COM
- **Default:** ON
- **Behavior:** Press button → Mute active alarm
- **Self-test:** If OFF → Skip button test

#### Relay (Terminal 12, DO1)
- **Toggle:** ON / OFF
- **Function:** Pull DO1 LOW to activate external relay
- **Wiring:** DO1 → Relay Coil Negative
- **Default:** OFF
- **Use Case:** Anchor light, secondary alarm, auxiliary control
- **Self-test:** If OFF → Skip relay test
- **Conflict:** Shares DO1 with Alarm Panel (only one can be active)

---

### DO1 Conflict Resolution

**Behavior:**
- If user enables **Alarm Panel** → Relay automatically DISABLED
- If user enables **Relay** → Alarm Panel automatically DISABLED
- **UI Feedback:** Show warning when user attempts to enable both
- **Message:** "⚠️ DO1 shared: Enabling Relay will disable Alarm Panel"

---

### Alarm Distance Threshold

- **Range:** 25-250 feet
- **Default:** 50 feet
- **Slider Control:** Continuous adjustment
- **Real-time Display:** Show current value above slider
- **Recommendations:**
  - Calm conditions: 25-50 ft
  - Moderate winds: 75-100 ft
  - Strong winds/current: 150-250 ft

---

### Persistent Storage (ESP32 NVS)

All configuration settings saved to ESP32 NVS flash:

```cpp
typedef struct {
  // GPS Configuration
  uint8_t gps_source;       // 0=N2K, 1=NMEA0183, 2=External
  uint8_t gps_device_id;    // N2K device ID (0x00-0xFF)
  uint32_t gps_pgn;         // PGN number (129029 or 129025)
  
  // Compass Configuration
  uint8_t compass_source;   // 0=N2K, 1=NMEA0183, 2=External
  uint8_t compass_device_id;// N2K device ID (0x00-0xFF)
  uint32_t compass_pgn;     // PGN number (127250 or 127251)
  
  // Hardware Components
  bool alarm_panel_enabled; // Alarm panel ON/OFF
  bool push_button_enabled; // Push button ON/OFF
  bool relay_enabled;       // Relay ON/OFF
  
  // Alarm Settings
  uint16_t alarm_distance;  // Feet (25-250)
} config_t;
```

**NVS Key:** `anchor_config`  
**Namespace:** `anchor_alarm`  
**Backup:** Optional TF card backup to `tf-card/config/config.json`

---

### User Interactions

#### GPS Source Selection
1. User taps radio button: N2K / NMEA 0183 / External
2. **If N2K selected:**
   - Show Device ID input field (default 0x00)
   - Show PGN radio buttons (129029 or 129025)
3. **If NMEA 0183 selected:**
   - Show Device ID input field (default 0x00)
   - Hide PGN field
4. **If External selected:**
   - Hide Device ID and PGN fields

#### Compass Source Selection
1. User taps radio button: N2K / NMEA 0183 / External
2. **If N2K selected:**
   - Show Device ID input field (default 0x01)
   - Show PGN radio buttons (127250 or 127251)
3. **If NMEA 0183 selected:**
   - Show Device ID input field (default 0x01)
   - Hide PGN field
4. **If External selected:**
   - Hide Device ID and PGN fields

#### Hardware Component Toggles
1. User taps toggle switch (ON/OFF)
2. **If enabling Alarm Panel:**
   - Check if Relay is ON
   - If yes → Show warning: "Disable Relay first (DO1 shared)"
   - Auto-disable Relay
3. **If enabling Relay:**
   - Check if Alarm Panel is ON
   - If yes → Show warning: "Disable Alarm Panel first (DO1 shared)"
   - Auto-disable Alarm Panel

#### Save Configuration
1. User taps **SAVE CONFIGURATION** button
2. Validate all inputs (Device ID 0x00-0xFF, PGN valid, no conflicts)
3. Write to ESP32 NVS flash
4. Show confirmation: "✅ Configuration saved successfully"
5. Optional: Backup to TF card `config/config.json`
6. Return to previous screen

#### Cancel
1. User taps **CANCEL** button
2. Discard all changes
3. Reload previous configuration from NVS
4. Return to previous screen

---

### Self-Test Integration

**Modified Self-Test Behavior:**

```cpp
// Pseudo-code for self-test integration

if (config.alarm_panel_enabled) {
  // Test alarm panel
  lv_label_set_text(status_label, "[..] Alarm Panel - Testing...");
  hardware_sim_set_alarm_panel(true);  // Activate for 1 second
} else {
  // Skip test
  lv_label_set_text(status_label, "[--] Alarm Panel - Disabled in config");
}

if (config.push_button_enabled) {
  // Test push button
  lv_label_set_text(status_label, "[..] Button - Waiting for press...");
  // Monitor DI0 input
} else {
  // Skip test
  lv_label_set_text(status_label, "[--] Button - Disabled in config");
}

if (config.relay_enabled) {
  // Test relay
  lv_label_set_text(status_label, "[..] Relay - Testing...");
  hardware_sim_set_relay(true);  // Activate for 1 second
} else {
  // Skip test
  lv_label_set_text(status_label, "[--] Relay - Disabled in config");
}

// GPS source selection
switch (config.gps_source) {
  case GPS_SOURCE_N2K:
    // Check for PGN messages from device_id
    break;
  case GPS_SOURCE_NMEA0183:
    // Listen for NMEA 0183 sentences
    break;
  case GPS_SOURCE_EXTERNAL:
    // I2C scan for GPS module
    break;
}
```

---

## Navigation

- **BACK Button:** Return to START screen
- **Swipe Left:** Navigate to PGN screen
- **Swipe Right:** Navigate to UPDATE screen
- **Global Navigation Bar:** Always visible at bottom

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 2.0.0 | 2025-12-14 | Complete redesign: GPS/Compass source selection, hardware component toggles, self-test integration |
| 1.0.0 | 2025-12-13 | Initial config screen (alarm distance only) |

---

**End of Specification**
