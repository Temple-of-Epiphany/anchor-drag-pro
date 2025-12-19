# ESP32-S3-Touch-LCD-4.3B Knowledge Base

## Hardware Specifications

### Display
- **Model**: ST7262 RGB LCD Controller
- **Resolution**: 800×480 pixels
- **Color Depth**: RGB565 (16-bit)
- **Brightness**: 550 Cd/m²
- **Interface**: 16-bit RGB parallel
- **Viewing Angle**: IPS panel

### Processor
- **MCU**: ESP32-S3
- **Architecture**: Dual-core Xtensa LX7
- **Clock**: Up to 240MHz
- **Flash**: 16MB
- **PSRAM**: 8MB

### Touch Screen
- **Controller**: GT911 (I2C address 0x5D)
- **Type**: Capacitive, 5-point multi-touch
- **Surface**: Toughened glass

## Complete Pin Configuration

### RGB LCD Data Pins (16-bit)
```
Blue channel (5 bits: B3-B7):
  GPIO14 - D0  (B3)
  GPIO38 - D1  (B4)
  GPIO18 - D2  (B5)
  GPIO17 - D3  (B6)
  GPIO10 - D4  (B7)

Green channel (6 bits: G2-G7):
  GPIO39 - D5  (G2)
  GPIO0  - D6  (G3)
  GPIO45 - D7  (G4)
  GPIO48 - D8  (G5)
  GPIO47 - D9  (G6)
  GPIO21 - D10 (G7)

Red channel (5 bits: R3-R7):
  GPIO1  - D11 (R3)
  GPIO2  - D12 (R4)
  GPIO42 - D13 (R5)
  GPIO41 - D14 (R6)
  GPIO40 - D15 (R7)
```

### RGB LCD Control Pins
```
GPIO3  - VSYNC (Vertical Sync)
GPIO5  - DE (Data Enable)
GPIO7  - PCLK (Pixel Clock)
GPIO46 - HSYNC (Horizontal Sync)
```

### RGB LCD Timing Parameters
```
Pixel Clock: 16 MHz
Horizontal: HPW=4, HBP=8, HFP=8
Vertical: VPW=4, VBP=8, VFP=8
Bounce Buffer: 800 * 10 pixels (to prevent drift)
```

### I2C Bus (Shared)
```
GPIO8 - SDA (I2C Data)
GPIO9 - SCL (I2C Clock)
Speed: 400kHz

I2C Devices:
  - GT911 Touch Controller (0x5D)
  - PCF85063 RTC (0x51)
  - CH422G IO Expander (0x24)
```

### Touch Controller (GT911)
```
I2C Address: 0x5D
GPIO4 - TP_IRQ (Interrupt, active low)
CH422G EXIO1 - TP_RST (Reset via IO expander)
```

### CH422G IO Expander Pins
```
EXIO0 - Reserved
EXIO1 - TP_RST (Touch Reset)
EXIO2 - LCD_BL (LCD Backlight)
EXIO3 - LCD_RST (LCD Reset)
EXIO4 - SD_CS (SD Card Chip Select, active low)
EXIO5 - USB_SEL (USB Selection)
```

### SD Card (SPI)
```
GPIO11 - MOSI
GPIO12 - SCK (Clock)
GPIO13 - MISO
CH422G EXIO4 - CS (Chip Select via IO expander)
```

### RS485 Serial
```
GPIO43 - RXD
GPIO44 - TXD
```

### CAN Bus (TWAI)
```
GPIO15 - CANTX
GPIO16 - CANRX
```

### USB
```
GPIO19 - USB_DN (D-)
GPIO20 - USB_DP (D+)
```

### Isolated Digital I/O
```
Inputs (via optocoupler):
  CH422G EXIO0/EXIO5 - DI0, DI1

Outputs (open drain):
  OD0, OD1 - DO0, DO1

Voltage Range: 5-36V
```

## Required Libraries

### From Waveshare Demo (in lib/ folder)
1. **ESP32_Display_Panel** (v1.0.0)
   - RGB LCD driver
   - Touch controller support
   - Board configuration

2. **ESP32_IO_Expander** (v1.0.1+)
   - CH422G driver
   - TCA95xx support
   - HT8574 support

3. **esp-lib-utils** (v0.1.x)
   - Logging utilities
   - Memory management
   - Common functions

### Optional (for advanced UI)
4. **lvgl** (v8.x)
   - GUI framework
   - Widgets and themes
   - Touch input handling

## Library Dependencies

ESP32_Display_Panel requires:
- ESP32_IO_Expander (>=1.0.0 && <2.0.0)
- esp-lib-utils (>=0.1.0 && <0.2.0)

## Initialization Sequence

### Minimal Display Setup
```cpp
1. Initialize I2C bus (GPIO8/9, 400kHz)
2. Initialize CH422G IO Expander
3. Set EXIO2 HIGH (LCD backlight ON)
4. Set EXIO3 HIGH (LCD reset released)
5. Create BusRGB with 16-bit data pins
6. Create LCD_ST7262 instance
7. Configure bounce buffer (800*10 pixels)
8. Call lcd->init()
9. Call lcd->reset()
10. Call lcd->begin()
11. Call lcd->setDisplayOnOff(true)
```

### With Touch Support
```cpp
12. Initialize GT911 on I2C (address 0x5D)
13. Configure interrupt on GPIO4
14. Reset touch via CH422G EXIO1
```

## Common Issues & Solutions

### Screen Drift/Tearing
**Solution**: Configure bounce buffer
```cpp
bus->configRGB_BounceBufferSize(LCD_WIDTH * 10);
```

### IO Expander Not Responding
**Cause**: I2C not initialized or wrong address
**Check**:
- I2C pins GPIO8/9
- Address 0x24
- 400kHz clock speed

### Display Not Turning On
**Check**:
1. CH422G EXIO2 (backlight) is HIGH
2. CH422G EXIO3 (reset) is HIGH
3. Power supply is adequate (5V/2A minimum)

### Touch Not Working
**Check**:
1. GT911 I2C address 0x5D
2. GPIO4 interrupt configured
3. EXIO1 reset pulse sent

## Reserved I2C Addresses
**Do not use**: 0x20-0x27, 0x30-0x3F, 0x51, 0x5D

Occupied by:
- CH422G (0x24)
- PCF85063 RTC (0x51)
- GT911 Touch (0x5D)

## Power Requirements
- **USB-C**: 5V, 2A minimum
- **DC Input**: 7-36V (isolated I/O voltage range)
- **Display**: ~500mA at full brightness

## Demo Code Locations
```
/Users/colin/Downloads/ESP32-S3-Touch-LCD-4.3B-BOX-Demo/Arduino/
├── examples/
│   ├── 08_DrawColorBar/     # Minimal RGB LCD example
│   └── 09_lvgl_Porting/     # LVGL GUI example
└── libraries/
    ├── ESP32_Display_Panel/ # Display driver library
    ├── ESP32_IO_Expander/   # CH422G driver
    ├── esp-lib-utils/       # Utility functions
    └── lvgl/                # LVGL GUI library
```

## Tested Working Configuration

From `waveshare_lcd_port.h`:
```cpp
// I2C
#define I2C_MASTER_SDA_IO 8
#define I2C_MASTER_SCL_IO 9

// IO Expander pins
#define TP_RST    1  // Touch reset
#define LCD_BL    2  // LCD backlight
#define LCD_RST   3  // LCD reset
#define SD_CS     4  // SD card CS
#define USB_SEL   5  // USB select

// LCD config
#define LCD_NAME ST7262
#define LCD_WIDTH 800
#define LCD_HEIGHT 480
#define LCD_COLOR_BITS 16
#define LCD_RGB_DATA_WIDTH 16
#define LCD_RGB_TIMING_FREQ_HZ (16 * 1000 * 1000)
#define LCD_RGB_TIMING_HPW 4
#define LCD_RGB_TIMING_HBP 8
#define LCD_RGB_TIMING_HFP 8
#define LCD_RGB_TIMING_VPW 4
#define LCD_RGB_TIMING_VBP 8
#define LCD_RGB_TIMING_VFP 8
#define LCD_RGB_BOUNCE_BUFFER_SIZE (LCD_WIDTH * 10)
```

## Build Flags Required
```ini
build_flags =
  -D ARDUINO_USB_CDC_ON_BOOT=1
  -D ESP32S3
  -D BOARD_HAS_PSRAM

build_unflags =
  -std=gnu++11
```

## Wiki Reference
https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-4.3B
