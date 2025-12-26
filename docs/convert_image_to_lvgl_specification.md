# Image to LVGL Conversion Script Specification

**Script Name:** `convert_image_to_lvgl.py`
**Version:** 0.1.0
**Author:** Colin Bitterfield
**Email:** colin@bitterfield.com
**Date Created:** 2025-12-22

## Purpose

Converts PNG images to LVGL-compatible C array format with proper RGB565 encoding. Generates both a C source file with pixel data and a header file with the LVGL image descriptor.

## Background

The original splash logo implementation stored 16-bit RGB565 values directly in a `uint8_t` array, causing thousands of overflow warnings during compilation. This script properly converts RGB888 PNG images to RGB565 format with correct little-endian byte ordering.

## Requirements

### Dependencies
- Python 3.x
- PIL/Pillow library (`pip install Pillow`)

### Input
- PNG image file (any size, any color mode)
- Output C file path
- Output H file path

### Output
- C source file containing:
  - Static byte array with RGB565 pixel data
  - LVGL image descriptor structure
- Header file containing:
  - Include guards
  - External declaration of image descriptor

## Technical Specifications

### RGB888 to RGB565 Conversion

The script converts 24-bit RGB (8 bits per channel) to 16-bit RGB565:

```
Red:   8 bits → 5 bits (bits 15-11)
Green: 8 bits → 6 bits (bits 10-5)
Blue:  8 bits → 5 bits (bits 4-0)
```

**Formula:**
```python
R5 = (R >> 3) & 0x1F
G6 = (G >> 2) & 0x3F
B5 = (B >> 3) & 0x1F
RGB565 = (R5 << 11) | (G6 << 5) | B5
```

### Byte Ordering

RGB565 values are stored in **little-endian** format:
- Low byte (bits 7-0) stored first
- High byte (bits 15-8) stored second

**Example:**
```c
RGB565 value: 0x2DFB
Stored as:    0xFB, 0x2D
```

### LVGL Image Descriptor Structure

```c
const lv_img_dsc_t splash_logo = {
  .header = {
    .cf = LV_IMG_CF_TRUE_COLOR,  // RGB565 format
    .always_zero = 0,
    .reserved = 0,
    .w = <width>,
    .h = <height>,
  },
  .data_size = <width * height * 2>,
  .data = splash_logo_data,
};
```

## Usage

### Command Line

```bash
python3 scripts/convert_image_to_lvgl.py <input.png> <output.c> <output.h>
```

### Example

```bash
python3 scripts/convert_image_to_lvgl.py \
  assets/splash_logo.png \
  main/splash_logo.c \
  main/splash_logo.h
```

### Integration with Build

For automatic regeneration during build, add to `CMakeLists.txt`:

```cmake
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/main/splash_logo.c
         ${CMAKE_CURRENT_SOURCE_DIR}/main/splash_logo.h
  COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/scripts/convert_image_to_lvgl.py
          ${CMAKE_CURRENT_SOURCE_DIR}/assets/splash_logo.png
          ${CMAKE_CURRENT_SOURCE_DIR}/main/splash_logo.c
          ${CMAKE_CURRENT_SOURCE_DIR}/main/splash_logo.h
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/assets/splash_logo.png
  COMMENT "Generating LVGL splash logo from PNG"
)
```

## Output Format

### C File Structure

```c
/**
 * Header comment with metadata
 */

#include "lvgl.h"

static const uint8_t splash_logo_data[] = {
  0xFB, 0x2D, 0xFB, 0x2D, ...  // Byte pairs, 12 pixels per line
  ...
};

const lv_img_dsc_t splash_logo = {
  .header = { ... },
  .data_size = ...,
  .data = splash_logo_data,
};
```

### H File Structure

```c
/**
 * Header comment with metadata
 */

#ifndef SPLASH_LOGO_H
#define SPLASH_LOGO_H

#include "lvgl.h"

extern const lv_img_dsc_t splash_logo;

#endif // SPLASH_LOGO_H
```

## Performance Considerations

### Memory Usage
- 256×256 image = 131,072 bytes (128 KB)
- Stored in program flash (`.rodata` section)
- No RAM overhead (const data)

### Compilation Impact
- Proper byte ordering eliminates overflow warnings
- No runtime conversion needed
- Optimized for ESP32-S3 flash access

## Error Handling

### Input Validation
- Checks if input PNG exists
- Validates command-line argument count
- Automatic RGB conversion if source isn't RGB mode

### Edge Cases
- Images with alpha channel: Alpha is ignored, only RGB used
- Grayscale images: Automatically converted to RGB
- Non-square images: Supported (uses actual width×height)

## Known Limitations

1. **Color Mode:** Only generates RGB565 output
   - Could be extended for other formats (RGB888, indexed, etc.)

2. **Compression:** No image compression
   - LVGL supports compressed images, not implemented here

3. **Alpha Channel:** Ignores transparency
   - Could be extended to support LV_IMG_CF_TRUE_COLOR_ALPHA

## Future Enhancements

1. **Multiple Format Support:**
   - RGB888 (24-bit)
   - Indexed color (palette-based)
   - Alpha channel support

2. **Compression:**
   - LZ4 compression for flash savings
   - RLE (Run-Length Encoding)

3. **Batch Processing:**
   - Convert entire directories
   - Generate asset index files

4. **Optimization:**
   - Automatic image resizing
   - Quality/size trade-off options

## Troubleshooting

### "unsigned conversion overflow" warnings

**Problem:** 16-bit values stored in 8-bit array
**Solution:** Regenerate with this script (uses proper byte pairs)

### Image appears corrupted or wrong colors

**Problem:** Incorrect byte ordering (big-endian vs little-endian)
**Solution:** Verify script uses little-endian (low byte first)

### Build fails with "undefined reference"

**Problem:** Header not included or C file not compiled
**Solution:** Verify CMakeLists.txt includes both .c and .h files

### Display shows nothing

**Problem:** Wrong color format or descriptor configuration
**Solution:** Ensure `LV_IMG_CF_TRUE_COLOR` matches RGB565 configuration in `lv_conf.h`

## Version History

### 0.1.0 (2025-12-22)
- Initial implementation
- RGB565 conversion with little-endian byte ordering
- LVGL image descriptor generation
- Proper header/source file structure

## Related Documentation

- `docs/splash_screen_specification.md` - Splash screen implementation details
- `lv_conf.h` - LVGL configuration (color format, buffer size)
- `main/splash_screen.c` - Splash screen usage of logo image

## References

- LVGL Documentation: https://docs.lvgl.io/
- RGB565 Format: https://en.wikipedia.org/wiki/High_color
- Pillow Documentation: https://pillow.readthedocs.io/