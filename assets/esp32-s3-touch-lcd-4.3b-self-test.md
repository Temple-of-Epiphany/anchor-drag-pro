# ESP32-S3-Touch-LCD-4.3B Self-Test Recommendations

**Version:** 1.0.0  
**Author:** Colin Bitterfield  
**Email:** colin@bitterfield.com  
**Date Created:** 2025-12-26  
**Date Updated:** 2025-12-26  

## Changelog

- v1.0.0 (2025-12-26): Initial version

---

## Hardware Overview

The Waveshare ESP32-S3-Touch-LCD-4.3B includes the following testable components:

| Component | Interface | Chip/Controller | I2C Address |
|-----------|-----------|-----------------|-------------|
| ESP32-S3-WROOM-1-N16R8 | - | Xtensa LX7 Dual-core | - |
| 4.3" LCD (800x480) | RGB Parallel | ST7701S | - |
| Capacitive Touch | I2C | GT911 | 0x5D |
| IO Expander | I2C | CH422G | 0x20-0x27 |
| RTC | I2C | PCF85063 | 0x51 |
| CAN Bus | TWAI | SN65HVD230 | - |
| RS485 | UART | Auto TX/RX | - |
| TF Card Slot | SPI | - | - |
| Battery Charger | - | CS8501 | - |
| Isolated Digital I/O | CH422G | Optocoupler | - |
| WiFi/Bluetooth | Internal | ESP32-S3 | - |
| Flash | Internal | 16MB | - |
| PSRAM | Internal | 8MB | - |

---

## Self-Test Categories

### 1. Core System Tests (Critical)

These tests verify the ESP32-S3 is functioning correctly and should run first.

#### 1.1 CPU and Clock Verification

**Test:** Verify dual-core operation and clock frequency.

**Method:** Read CPU frequency, verify both cores operational.

**Pass Criteria:** CPU0 and CPU1 report 240MHz.

```cpp
uint32_t cpuFreq = getCpuFrequencyMhz();
bool dualCore = (xPortGetCoreID() >= 0);
```

#### 1.2 Flash Memory Test

**Test:** Verify 16MB flash is accessible and chip ID readable.

**Method:** Read flash chip ID and size.

**Pass Criteria:** Flash size reports 16MB (16777216 bytes).

```cpp
uint32_t flashSize = ESP.getFlashChipSize();
uint32_t flashSpeed = ESP.getFlashChipSpeed();
```

#### 1.3 PSRAM Test

**Test:** Verify 8MB PSRAM is accessible.

**Method:** Allocate and write/read test pattern to PSRAM.

**Pass Criteria:** 8MB reported, read-back matches write.

```cpp
size_t psramSize = ESP.getPsramSize();
size_t freePsram = ESP.getFreePsram();
// Allocate test buffer in PSRAM
uint8_t* testBuf = (uint8_t*)ps_malloc(1024);
```

#### 1.4 Internal RAM Test

**Test:** Verify SRAM heap availability.

**Method:** Check free heap size.

**Pass Criteria:** Free heap > 200KB at boot.

```cpp
size_t freeHeap = ESP.getFreeHeap();
size_t minFreeHeap = ESP.getMinFreeHeap();
```

---

### 2. I2C Bus Scan (Critical)

**Test:** Scan I2C bus and verify all expected devices respond.

**Expected Devices:**

| Address | Device | Required |
|---------|--------|----------|
| 0x20-0x27 | CH422G IO Expander | Yes |
| 0x51 | PCF85063 RTC | Yes |
| 0x5D | GT911 Touch Controller | Yes |

**Pass Criteria:** All three devices respond to I2C scan.

```cpp
Wire.begin(8, 9);  // SDA=GPIO8, SCL=GPIO9
for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
        // Device found at addr
    }
}
```

---

### 3. Display Tests

#### 3.1 LCD Initialization

**Test:** Initialize RGB parallel display and verify communication.

**Method:** Initialize display driver, check for errors.

**Pass Criteria:** No initialization errors, display accepts commands.

#### 3.2 Display Pattern Test

**Test:** Fill screen with solid colors and patterns.

**Method:** Sequential fill with Red, Green, Blue, White, Black, then checkerboard pattern.

**Pass Criteria:** Visual verification of correct colors and no artifacts.

#### 3.3 Backlight Control

**Test:** Verify backlight on/off via CH422G.

**Method:** Toggle LCD_BL pin through IO expander.

**Pass Criteria:** Backlight turns on and off.

```cpp
// Via CH422G IO Expander
expander->digitalWrite(LCD_BL_PIN, HIGH);  // On
expander->digitalWrite(LCD_BL_PIN, LOW);   // Off
```

#### 3.4 Display Frame Rate

**Test:** Measure actual display refresh capability.

**Method:** Run frame counter for 1 second.

**Pass Criteria:** >= 25 FPS achievable.

---

### 4. Touch Panel Tests

#### 4.1 Touch Controller Communication

**Test:** Verify GT911 responds and reports firmware version.

**Method:** Read GT911 product ID and firmware version registers.

**Pass Criteria:** Valid product ID returned (should read "911").

#### 4.2 Touch Calibration Verification

**Test:** Touch four corners of display.

**Method:** Display touch targets, record coordinates.

**Pass Criteria:** Coordinates within 5% of expected values.

#### 4.3 Multi-Touch Test

**Test:** Verify 5-point touch capability.

**Method:** Request user to place multiple fingers on screen.

**Pass Criteria:** Reports correct number of simultaneous touch points (up to 5).

#### 4.4 Touch Interrupt Test

**Test:** Verify touch interrupt signal functions.

**Method:** Monitor interrupt pin while touching screen.

**Pass Criteria:** Interrupt triggers on touch.

---

### 5. IO Expander Tests (CH422G)

#### 5.1 IO Expander Communication

**Test:** Read/write CH422G registers.

**Method:** Write test pattern, read back.

**Pass Criteria:** Read matches write.

#### 5.2 Digital Output Test

**Test:** Toggle each output pin.

**Method:** Set each pin high/low, measure with scope or LED.

**Pass Criteria:** All outputs toggle correctly.

#### 5.3 Digital Input Test

**Test:** Read input pins with known states.

**Method:** Apply known voltage to input, read via CH422G.

**Pass Criteria:** Read value matches applied state.

---

### 6. Isolated I/O Tests

#### 6.1 Isolated Digital Output Test

**Test:** Verify optocoupler-isolated outputs function.

**Method:** Toggle isolated outputs, verify with external meter.

**Pass Criteria:** Outputs can sink up to 450mA, correct logic levels.

#### 6.2 Isolated Digital Input Test

**Test:** Verify optocoupler-isolated inputs read correctly.

**Method:** Apply 5-36V signals to isolated inputs.

**Pass Criteria:** Inputs correctly report state for voltage range 5-36V.

**Note:** Test both passive (dry contact) and active (wet contact NPN) input modes.

---

### 7. RTC Tests (PCF85063)

#### 7.1 RTC Communication

**Test:** Read/write RTC registers.

**Method:** Read time, write new time, verify.

**Pass Criteria:** RTC responds, time can be set and read.

#### 7.2 RTC Timekeeping

**Test:** Verify RTC maintains accurate time.

**Method:** Set time, wait 60 seconds, compare to system millis().

**Pass Criteria:** Drift < 2 seconds per minute.

#### 7.3 RTC Alarm Test

**Test:** Verify alarm interrupt functionality.

**Method:** Set alarm 5 seconds in future, wait for interrupt.

**Pass Criteria:** Interrupt fires within 1 second of expected time.

#### 7.4 RTC Battery Backup

**Test:** Verify RTC maintains time during power loss.

**Method:** Set time, power cycle, read time.

**Pass Criteria:** Time maintained (requires backup battery installed).

---

### 8. Communication Interface Tests

#### 8.1 CAN Bus Test

**Test:** Verify CAN transceiver and TWAI peripheral function.

**Method:** Send CAN frame, verify with external CAN analyzer or loopback.

**Pass Criteria:** Frame transmitted successfully at configured baud rate.

**Loopback Test:**
```cpp
// Configure TWAI in loopback mode for self-test
twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(TX_PIN, RX_PIN, TWAI_MODE_NO_ACK);
```

**External Test:** Requires USB-CAN adapter or second CAN node.

#### 8.2 RS485 Test

**Test:** Verify RS485 transceiver and auto-direction switching.

**Method:** Send data, verify loopback or echo from external device.

**Pass Criteria:** Data transmitted and received correctly.

**Loopback Test:** Short A to A, B to B on two devices or use RS485 echo device.

```cpp
Serial1.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
Serial1.print("TEST");
// Read echo back
```

#### 8.3 I2C External Port Test

**Test:** Verify external I2C connector functions.

**Method:** Connect known I2C device, scan bus.

**Pass Criteria:** External device detected at expected address.

---

### 9. Storage Tests

#### 9.1 TF Card Detection

**Test:** Verify SD card slot detects card presence.

**Method:** Insert card, check detection.

**Pass Criteria:** Card detected when inserted.

#### 9.2 TF Card Read/Write

**Test:** Verify SD card read/write operations.

**Method:** Write test file, read back, compare.

**Pass Criteria:** Data integrity verified, speed meets expectations.

```cpp
// Initialize via CH422G for SD_CS control
File testFile = SD.open("/test.txt", FILE_WRITE);
testFile.print("TEST DATA");
testFile.close();
```

#### 9.3 TF Card Speed Test

**Test:** Measure read/write throughput.

**Method:** Write/read 1MB file, measure time.

**Pass Criteria:** Document actual speed for reference.

#### 9.4 NVS Test

**Test:** Verify NVS (Preferences) storage functions.

**Method:** Write/read test values to NVS.

**Pass Criteria:** Values persist across reboot.

---

### 10. Wireless Tests

#### 10.1 WiFi Scan

**Test:** Verify WiFi radio functions.

**Method:** Scan for available networks.

**Pass Criteria:** Networks detected, RSSI values reasonable.

```cpp
int n = WiFi.scanNetworks();
for (int i = 0; i < n; i++) {
    Serial.printf("%s (%d dBm)\n", WiFi.SSID(i).c_str(), WiFi.RSSI(i));
}
```

#### 10.2 WiFi Connection

**Test:** Connect to known access point.

**Method:** Attempt connection with known credentials.

**Pass Criteria:** Connection established, IP assigned.

#### 10.3 WiFi Signal Strength

**Test:** Measure received signal strength.

**Method:** Read RSSI while connected.

**Pass Criteria:** RSSI > -70 dBm at 1 meter from AP.

#### 10.4 Bluetooth Scan

**Test:** Verify Bluetooth radio functions.

**Method:** Scan for BLE devices.

**Pass Criteria:** Nearby BLE devices detected.

```cpp
BLEDevice::init("");
BLEScan* pBLEScan = BLEDevice::getScan();
BLEScanResults results = pBLEScan->start(5);
```

#### 10.5 Bluetooth Advertising

**Test:** Verify BLE advertising capability.

**Method:** Start advertising, verify with phone app.

**Pass Criteria:** Device visible in BLE scanner app.

---

### 11. Power System Tests

#### 11.1 Input Voltage Range

**Test:** Verify operation across 7-36V input range.

**Method:** Apply voltages at 7V, 12V, 24V, 36V, monitor stability.

**Pass Criteria:** Stable operation at all voltage points.

**Caution:** Test incrementally, monitor for overheating.

#### 11.2 Power Indicator LEDs

**Test:** Verify PWR, CHG, DONE LEDs function correctly.

**Method:** Observe LEDs under various power/charge states.

**Pass Criteria:**
- PWR: Lit when powered
- CHG: Lit/blinking when charging battery
- DONE: Lit when battery fully charged

#### 11.3 Battery Charging

**Test:** Verify CS8501 battery charger functions.

**Method:** Connect discharged battery, monitor charging.

**Pass Criteria:** CHG LED indicates charging, battery voltage increases.

#### 11.4 Battery Discharge/Boost

**Test:** Verify battery can power device.

**Method:** Disconnect external power, run from battery.

**Pass Criteria:** Device operates normally on battery power.

#### 11.5 Current Consumption

**Test:** Measure system current draw.

**Method:** Insert ammeter in power line.

**Pass Criteria:** Document baseline current for reference.

| Mode | Expected Current |
|------|-----------------|
| Idle (display on) | ~200-300mA |
| Active (WiFi + display) | ~350-450mA |
| Sleep | < 10mA |

---

### 12. USB Tests

#### 12.1 USB Serial Communication

**Test:** Verify USB CDC serial functions.

**Method:** Send/receive data via USB serial.

**Pass Criteria:** Bidirectional communication at 115200 baud.

#### 12.2 USB Enumeration

**Test:** Verify device enumerates correctly.

**Method:** Connect to PC, check device manager.

**Pass Criteria:** Device recognized as ESP32-S3 or CH343 serial port.

---

## Self-Test Execution Order

For production or commissioning testing, execute tests in this order:

1. Core System Tests (CPU, Flash, PSRAM, RAM)
2. I2C Bus Scan
3. IO Expander Communication
4. Display Initialization and Pattern Test
5. Touch Panel Communication
6. RTC Communication
7. NVS Read/Write
8. TF Card Detection and Read/Write
9. CAN Bus Loopback
10. RS485 Communication
11. Isolated I/O Tests
12. WiFi Scan
13. Bluetooth Scan
14. Power System Tests
15. USB Tests

---

## Test Result Storage

Store test results in NVS for production tracking:

```cpp
Preferences testResults;
testResults.begin("selftest", false);
testResults.putBool("cpu_pass", cpuTestPassed);
testResults.putBool("flash_pass", flashTestPassed);
testResults.putBool("psram_pass", psramTestPassed);
testResults.putBool("i2c_pass", i2cTestPassed);
testResults.putBool("lcd_pass", lcdTestPassed);
testResults.putBool("touch_pass", touchTestPassed);
testResults.putBool("rtc_pass", rtcTestPassed);
testResults.putBool("can_pass", canTestPassed);
testResults.putBool("rs485_pass", rs485TestPassed);
testResults.putBool("wifi_pass", wifiTestPassed);
testResults.putBool("bt_pass", btTestPassed);
testResults.putBool("sd_pass", sdTestPassed);
testResults.putBool("iso_io_pass", isoIoTestPassed);
testResults.putULong("test_time", millis());
testResults.putString("fw_ver", FIRMWARE_VERSION);
testResults.end();
```

---

## Display Self-Test Results

Use the LCD to display test results with color coding:

| Result | Color |
|--------|-------|
| Pass | Green |
| Fail | Red |
| Skip | Yellow |
| Running | Blue |

---

## References

- [Waveshare Wiki: ESP32-S3-Touch-LCD-4.3B](https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-4.3B)
- [ESP32-S3 Technical Reference Manual](https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf)
- [GT911 Datasheet](https://www.waveshare.com/w/upload/4/43/GT911_Datasheet.pdf)
- [PCF85063 Datasheet](https://www.nxp.com/docs/en/data-sheet/PCF85063A.pdf)
- [CH422G Datasheet](http://www.wch-ic.com/products/CH422.html)
