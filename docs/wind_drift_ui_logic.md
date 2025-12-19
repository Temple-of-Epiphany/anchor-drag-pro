# Wind and Drift UI Logic Diagram

## Current Behavior Issues
- Wind direction and drift direction are separate inputs
- Confusing when wind should control drift vs manual drift direction
- No clear relationship between wind presence and drift behavior

## Proposed Logic Flow

```
┌─────────────────────────────────────────────────────────────┐
│                    WIND CONDITIONS WIDGET                    │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
                    ┌──────────────────┐
                    │  Wind Speed = 0? │
                    └──────────────────┘
                       │              │
                   YES │              │ NO
                       │              │
         ┌─────────────┘              └──────────────┐
         ▼                                           ▼
┌──────────────────────┐                  ┌──────────────────────┐
│  Show Drift Settings │                  │   Hide Drift Input   │
│                      │                  │                      │
│ • Drift Direction    │                  │ Wind controls drift  │
│   (0-359°)           │                  │ direction            │
│ • Drift Speed        │                  │                      │
│   (knots)            │                  │ Drift = Wind + 180°  │
└──────────────────────┘                  └──────────────────────┘
         │                                           │
         │                                           │
         └───────────────┬───────────────────────────┘
                         ▼
                ┌─────────────────┐
                │   DRIFT MODE    │
                │    ACTIVATED    │
                └─────────────────┘
                         │
                         ▼
              ┌────────────────────┐
              │  Wind Speed > 0?   │
              └────────────────────┘
                   │          │
               YES │          │ NO
                   │          │
        ┌──────────┘          └─────────┐
        ▼                               ▼
┌─────────────────┐           ┌─────────────────┐
│ Use Wind-Driven │           │  Use Manual     │
│ Drift           │           │  Drift Settings │
│                 │           │                 │
│ Direction:      │           │ Direction: User │
│  Wind + 180°    │           │ Speed: User     │
│ Speed:          │           │                 │
│  Wind speed     │           │                 │
└─────────────────┘           └─────────────────┘
        │                               │
        └───────────┬───────────────────┘
                    ▼
          ┌──────────────────┐
          │  Boat Drifts in  │
          │  Calculated      │
          │  Direction       │
          └──────────────────┘
```

## UI Widget Changes

### Wind Conditions Widget
```
┌─────────────────────────────────────┐
│      Wind Conditions                │
├─────────────────────────────────────┤
│ Wind Speed: [___] kts               │
│ Wind Direction: [___]°              │
│                                     │
│ ☐ Variable Wind                     │
│                                     │
│ [Apply Wind]                        │
└─────────────────────────────────────┘
```

### Drift Settings Widget (Conditional - Only shows when Wind Speed = 0)
```
┌─────────────────────────────────────┐
│      Drift Settings                 │
│      (No Wind Mode)                 │
├─────────────────────────────────────┤
│ Drift Direction: [___]°             │
│   (Direction boat is drifting TO)   │
│                                     │
│ Drift Speed: [___] kts              │
│                                     │
│ [Apply Drift]                       │
└─────────────────────────────────────┘
```

## State Machine

```
STATE: STATIONARY
  ├─> Wind = 0, Drift Settings visible
  └─> Wind > 0, Drift Settings hidden

STATE: DRIFT MODE
  ├─> IF Wind > 0:
  │     - Drift Direction = Wind Direction + 180°
  │     - Drift Speed = Wind Speed
  │     - Drift Settings disabled/hidden
  │
  └─> IF Wind = 0:
        - Drift Direction = User Setting
        - Drift Speed = User Setting
        - Drift Settings enabled/visible

STATE: ANCHORING MODE
  ├─> Wind always affects swing behavior
  └─> Drift settings ignored

STATE: CIRCLE MODE
  └─> Wind and Drift settings ignored
```

## Validation Rules

1. **Wind Speed = 0 AND Drift Mode**
   - Require drift direction (0-359°)
   - Require drift speed (> 0 kts)
   - Show warning if drift settings not provided

2. **Wind Speed > 0**
   - Hide/disable drift direction input
   - Auto-calculate drift from wind
   - Show info: "Drift controlled by wind"

3. **Mode Changes**
   - Switching TO drift mode with no wind → prompt for drift settings
   - Switching TO drift mode with wind → auto-start wind-driven drift

## User Messages

- **Wind > 0, Drift Mode**:
  `"Drifting with wind: 15 kts from North (drifting South)"`

- **Wind = 0, Drift Mode, No Settings**:
  `"⚠️ Please set drift direction and speed"`

- **Wind = 0, Drift Mode, Settings Applied**:
  `"Drifting manually: 2 kts to East"`

- **Wind Changes From 0 to >0 During Drift**:
  `"Wind detected - switching to wind-driven drift"`

## Implementation Priority

1. **Phase 1** (High Priority)
   - Show/hide drift settings based on wind speed
   - Auto-calculate drift from wind when wind > 0
   - Validate drift settings when wind = 0

2. **Phase 2** (Medium Priority)
   - Add visual indicators for drift source (wind vs manual)
   - Add smooth transitions when wind changes

3. **Phase 3** (Low Priority)
   - Add drift speed multiplier (e.g., current effects)
   - Add drift history/trail visualization
