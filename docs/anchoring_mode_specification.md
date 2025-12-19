# Anchoring Mode Specification

**Author:** Colin Bitterfield
**Email:** colin@bitterfield.com
**Date Created:** 2025-12-02
**Version:** 0.1.0

## Overview

This document defines the complete specification for the anchoring simulation mode in the N2K simulator.

---

## Anchoring Process Phases

### Phase 1: Anchor Drop
**Duration:** Until boat reaches anchor rode distance from drop point

1. **Initial State:**
   - Record anchor position = current boat position
   - Set mode = 'anchoring'
   - Set anchor_drop_phase = 'dropping'
   - Clear anchor trail

2. **Movement During Drop:**
   - **IF wind_speed > 0:**
     - Move in direction of wind at wind speed
     - Direction = wind_direction (degrees)
     - Speed = wind_speed (knots) converted to m/s
   - **ELSE (no wind):**
     - Move backwards at 0.5 knots
     - Direction = current_course + 180 degrees

3. **Transition to Swinging:**
   - When distance from anchor ≥ anchor_rode_distance
   - Set anchor_drop_phase = 'swinging'
   - Initialize pendulum parameters

### Phase 2: Anchor Swinging
**Duration:** Continuous while in anchoring mode

1. **Physics Model:**
   ```
   Pendulum motion constrained by:
   - Max radius = anchor_rode_distance
   - Wind force influence
   - Chain weight creating forward/backward motion
   - Random perturbations
   ```

2. **Position Calculation:**
   ```python
   # Current angle from anchor (0 = North, clockwise)
   swing_angle = atan2(boat_lon - anchor_lon, boat_lat - anchor_lat)

   # Angular acceleration from wind
   wind_force = wind_speed * sin(wind_direction_rad - swing_angle)
   angular_accel = wind_force / anchor_rode_distance

   # Add damping and randomness
   angular_accel += -damping * swing_velocity
   angular_accel += random_perturbation

   # Update velocity and position
   swing_velocity += angular_accel * dt
   swing_angle += swing_velocity * dt

   # Constrain to max radius
   distance = anchor_rode_distance * (0.7 + 0.3 * random())

   # Convert back to lat/lon
   new_lat = anchor_lat + distance * cos(swing_angle) / METERS_PER_DEGREE
   new_lon = anchor_lon + distance * sin(swing_angle) / METERS_PER_DEGREE_LON
   ```

3. **Wind Influence:**
   - Stronger winds create larger swing amplitude
   - Wind direction changes cause boat to migrate around anchor
   - Calm conditions: boat settles near anchor with small oscillations

4. **Random Movement:**
   - Add gaussian noise to position (±2 meters)
   - Simulate wave action, current variations
   - More randomness in higher wind conditions

---

## Drift Mode with Anchor

**Behavior:** Both boat AND anchor move together

1. **Anchor dragging:**
   - Anchor position updates at reduced rate (0.5x drift speed)
   - Boat position leads anchor position
   - Maintain relative distance approximately at anchor_rode_distance
   - Some oscillation in relative position

2. **Formula:**
   ```python
   # Boat moves at full drift speed
   boat_position += drift_velocity * dt

   # Anchor follows at reduced speed
   anchor_velocity = drift_velocity * 0.5
   anchor_position += anchor_velocity * dt
   ```

---

## Route Mode Interaction

**Behavior:** Stop route and anchor from current position

1. When anchoring mode activated during route:
   - Stop waypoint navigation
   - Clear active_route
   - Set anchor_position = current_position
   - Begin anchor drop process

2. Route mode button:
   - Disabled while anchoring
   - Re-enabled when anchor is retrieved

---

## Trail/Scatter Plot

**Purpose:** Show historical boat movement while anchored

1. **Data Collection:**
   - Record position every update cycle (2x/second or configured rate)
   - Store as: `(latitude, longitude, timestamp)`
   - Limit to last N points (e.g., 200 points = 100 seconds at 2 Hz)

2. **Storage:**
   ```python
   anchor_trail = deque(maxlen=200)  # Circular buffer
   anchor_trail.append((lat, lon, time.time()))
   ```

3. **Visualization:**
   - Display as scatter plot on anchor visualization widget
   - Points fade with age (older = more transparent)
   - Different color based on wind strength

---

## Anchor Visualization Widget

**Location:** Below Leaflet map widget

### Components:

1. **Canvas/SVG Drawing Area:**
   - 300x300 pixel square
   - Centered on anchor position
   - Scale adjusts to show full rode distance

2. **Elements to Display:**

   **a) Anchor Symbol:**
   - Fixed at center
   - Icon: ⚓ or custom SVG
   - Color: Gold/yellow

   **b) Rode Distance Circle:**
   - Radius = anchor_rode_distance in pixels (scaled)
   - Stroke: Dashed white line
   - Label: Distance in meters on circle edge

   **c) Boat Symbol:**
   - Current position relative to anchor
   - Icon: 🚢 or triangle pointing in heading direction
   - Color: Based on status (green = normal, yellow = near limit)

   **d) Movement Trail:**
   - Dots for each recorded position
   - Size: 2-4 pixels
   - Color gradient: Recent (bright green) → Old (dim gray)
   - Alpha: 0.8 for recent, 0.2 for old

   **e) Wind Indicator:**
   - Arrow showing wind direction
   - Length proportional to wind speed
   - Position: Top-right corner

3. **Coordinate System:**
   ```python
   # Convert lat/lon offset to pixel coordinates
   def geo_to_pixel(lat, lon, anchor_lat, anchor_lon, scale):
       dx_meters = (lon - anchor_lon) * meters_per_degree_lon(anchor_lat)
       dy_meters = (lat - anchor_lat) * METERS_PER_DEGREE_LAT

       pixel_x = canvas_center_x + dx_meters / scale
       pixel_y = canvas_center_y - dy_meters / scale  # Y inverted

       return (pixel_x, pixel_y)
   ```

4. **Auto-scaling:**
   - Scale = (canvas_width / 2) / (anchor_rode_distance * 1.2)
   - Ensures full circle fits with 20% margin

---

## Backend API Endpoints

### 1. Set Anchor Rode Distance
```http
POST /api/anchor/rode-distance
Content-Type: application/json

{
  "distance": 75.0  // meters
}

Response: { "status": "success", "distance": 75.0 }
```

### 2. Set Mode to Anchoring
```http
POST /api/mode
Content-Type: application/json

{
  "mode": "anchoring"
}

Response: { "status": "success", "mode": "anchoring" }
```

### 3. Get Anchor State
```http
GET /api/anchor/state

Response: {
  "anchor_position": [30.0245, -90.034533],
  "boat_position": [30.0246, -90.034520],
  "rode_distance": 50.0,
  "current_distance": 42.3,
  "phase": "swinging",
  "trail": [[30.0246, -90.034520], ...],  // Last N positions
  "wind": {
    "speed": 10.5,
    "direction": 45
  }
}
```

---

## Frontend Integration

### 1. Settings Panel
- Added: Anchor Rode Distance input (10-300 meters)
- Saved to backend via applySettings()

### 2. Mode Button
- Remove "Coming Soon" alert
- Implement actual anchoring activation
- Show visual feedback when anchoring active

### 3. Anchor Visualization Widget
```html
<div class="card">
  <h2>⚓ Anchor Visualization</h2>
  <canvas id="anchor-canvas" width="300" height="300"></canvas>
  <div id="anchor-status">
    <span>Distance from Anchor: <span id="anchor-dist">--</span> m</span>
    <span>Phase: <span id="anchor-phase">--</span></span>
  </div>
</div>
```

### 4. JavaScript Functions
```javascript
function drawAnchorVisualization(anchorData) {
  const canvas = document.getElementById('anchor-canvas');
  const ctx = canvas.getContext('2d');

  // Clear canvas
  ctx.clearRect(0, 0, canvas.width, canvas.height);

  // Draw circle (rode distance)
  drawRodeCircle(ctx, anchorData.rode_distance);

  // Draw anchor
  drawAnchor(ctx, 150, 150);

  // Draw trail
  drawTrail(ctx, anchorData.trail);

  // Draw boat
  drawBoat(ctx, anchorData.boat_position, anchorData.anchor_position);

  // Draw wind arrow
  drawWind(ctx, anchorData.wind);
}
```

---

## Physics Constants

```python
METERS_PER_DEGREE_LAT = 111320.0  # Constant
DAMPING_COEFFICIENT = 0.1  # Swing damping
RANDOM_FORCE_STD = 0.5  # Standard deviation for random perturbations
WIND_FORCE_MULTIPLIER = 0.01  # Convert knots to acceleration
CHAIN_WEIGHT_FORCE = 0.005  # Forward/backward chain pull
KNOTS_TO_METERS_PER_SEC = 0.514444
```

---

## Testing Scenarios

### Test 1: No Wind Anchor Drop
- Start with no wind
- Drop anchor
- Verify boat backs away at 0.5 knots, 180° from heading
- Verify stops at rode distance
- Verify minimal swinging

### Test 2: High Wind Anchor Drop
- Set wind to 20 knots from north
- Drop anchor
- Verify boat moves south at wind speed
- Verify pendulum swinging when reached rode distance

### Test 3: Wind Direction Change
- Anchor in light wind from north
- Wait for boat to stabilize
- Change wind to south
- Verify boat migrates to opposite side of anchor

### Test 4: Drift Mode with Anchor
- Enter anchoring mode
- Switch to drift mode
- Verify both anchor and boat move
- Verify relative distance maintained

### Test 5: Trail Visualization
- Anchor in variable wind
- Observe scatter plot forming arc pattern
- Verify 100+ points collected
- Verify old points fade out

---

## Implementation Phases

### Phase 1: ✅ Complete
- Settings UI for anchor rode distance
- Backend state variables

### Phase 2: Backend Physics
- Anchor drop logic
- Pendulum swing simulation
- Wind force calculations
- Trail data collection

### Phase 3: Frontend Visualization
- Canvas widget below map
- Real-time drawing functions
- WebSocket data updates

### Phase 4: Testing & Refinement
- Test all scenarios
- Tune physics parameters
- Optimize performance

---

## Future Enhancements

1. **Multiple Anchor Types:**
   - Different holding power
   - Different rode materials (chain vs rope)

2. **Sea State Simulation:**
   - Wave height affects swing amplitude
   - Current in addition to wind

3. **Anchor Alarm Integration:**
   - Trigger alarm if boat exceeds rode distance
   - Warn if anchor appears to be dragging

4. **3D Visualization:**
   - Show depth
   - Show catenary (chain curve)

---

*This specification provides the complete blueprint for implementing realistic anchoring simulation.*
