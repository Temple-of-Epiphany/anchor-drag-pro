# LVGL Font Converter - Offline Setup

## Installation

### Prerequisites
- Node.js 14+ (check: `node --version`)

### Install Globally
```bash
npm install -g lv_font_conv
```

### Verify Installation
```bash
lv_font_conv --help
```

### Alternative: Run via npx (no install)
```bash
npx lv_font_conv --help
```

## Quick Start - Convert TTF to LVGL C File

### Basic Conversion
```bash
lv_font_conv \
  --font YourFont.ttf \
  --size 16 \
  --bpp 4 \
  --format lvgl \
  --range 0x20-0x7F \
  --no-compress \
  -o my_font_16.c
```

### Explanation:
- `--font` - Path to TTF/OTF file
- `--size 16` - Font height in pixels (16px)
- `--bpp 4` - Bits per pixel (4 = 16 gray levels, good quality)
- `--format lvgl` - Output C file for LVGL
- `--range 0x20-0x7F` - ASCII printable characters
- `--no-compress` - Disable RLE compression (simpler)
- `-o` - Output file name

## Common Character Ranges

```bash
# Basic ASCII (English)
--range 0x20-0x7F

# Add degree symbol and special chars
--range 0x20-0x7F,0xB0,0xB1,0xB2

# Add arrows
--range 0x20-0x7F,0x2190-0x21FF

# Specific characters (alternative to ranges)
--symbols "0123456789.,-°±×÷"
```

## Batch Convert Multiple Sizes

```bash
#!/bin/bash
# convert_roboto.sh

FONT="Roboto-Regular.ttf"
SIZES=(16 18 20 24 32 48)

for size in "${SIZES[@]}"; do
  echo "Converting size $size..."
  lv_font_conv \
    --font "$FONT" \
    --size $size \
    --bpp 4 \
    --format lvgl \
    --range 0x20-0x7F,0xB0 \
    --no-compress \
    -o "roboto_${size}.c"
done

echo "Done! Created fonts:"
ls -lh roboto_*.c
```

Make executable and run:
```bash
chmod +x convert_roboto.sh
./convert_roboto.sh
```

## Advanced: Merge Multiple Fonts

Merge English text + Font Awesome icons:
```bash
lv_font_conv \
  --font Roboto-Regular.ttf \
  --range 0x20-0x7F \
  --font FontAwesome.ttf \
  --range 0xF000-0xF0FF \
  --size 16 \
  --bpp 4 \
  --format lvgl \
  --no-compress \
  -o roboto_with_icons_16.c
```

## Add to Project

### 1. Copy fonts to project
```bash
mkdir -p main/fonts
cp roboto_*.c main/fonts/
```

### 2. Update main/CMakeLists.txt
```cmake
idf_component_register(
    SRCS
        "main.c"
        "display_driver.c"
        # ... other files ...
        "fonts/roboto_16.c"
        "fonts/roboto_24.c"
    INCLUDE_DIRS "."
)
```

### 3. Declare fonts in header
Create `main/fonts/custom_fonts.h`:
```c
#ifndef CUSTOM_FONTS_H
#define CUSTOM_FONTS_H

#include "lvgl.h"

// Declare custom fonts
LV_FONT_DECLARE(roboto_16);
LV_FONT_DECLARE(roboto_18);
LV_FONT_DECLARE(roboto_20);
LV_FONT_DECLARE(roboto_24);
LV_FONT_DECLARE(roboto_32);
LV_FONT_DECLARE(roboto_48);

#endif
```

### 4. Use in code
```c
#include "fonts/custom_fonts.h"

lv_obj_t *label = lv_label_create(parent);
lv_label_set_text(label, "Distance: 125 FT");
lv_obj_set_style_text_font(label, &roboto_24, 0);
```

## Recommended Fonts for Marine Display

Download from Google Fonts (https://fonts.google.com):

**High Readability:**
- **Roboto** - Modern, clean (Android default)
- **Open Sans** - Optimized for screen reading
- **Source Sans Pro** - Adobe's UI font

**Condensed (fit more text):**
- **Roboto Condensed**
- **Teko**

**Bold/Strong:**
- **Roboto Bold**
- **Montserrat Bold** (already have regular)

**Digital/Technical:**
- **Orbitron** - Futuristic
- **DSEG7** - Seven-segment display style

## Memory Usage

Approximate C file sizes:
- 16px ASCII: 8-12 KB
- 24px ASCII: 15-20 KB
- 32px ASCII: 25-30 KB
- 48px ASCII: 40-50 KB

Current binary: 555 KB / 1024 KB
**Plenty of room for custom fonts!**

## Troubleshooting

### "command not found"
```bash
# Check Node.js installed
node --version

# Reinstall globally
npm install -g lv_font_conv

# Or use npx
npx lv_font_conv --help
```

### Font looks wrong
- Try different `--bpp` values (1, 2, 4, 8)
- Check character range includes needed glyphs
- Verify font file isn't corrupted

### File too large
- Reduce `--bpp` (4 → 2)
- Use `--no-kerning` to drop kerning data
- Limit `--range` to only needed characters

## Example: Create All Sizes for Project

```bash
#!/bin/bash
# Setup script for Anchor Drag Pro fonts

FONT_DIR="main/fonts"
mkdir -p "$FONT_DIR"

# Download Roboto from Google Fonts first
# Place Roboto-Regular.ttf in current directory

echo "Converting Roboto fonts..."

for size in 16 18 20 24 32 48; do
  lv_font_conv \
    --font Roboto-Regular.ttf \
    --size $size \
    --bpp 4 \
    --format lvgl \
    --range 0x20-0x7F,0xB0 \
    --no-compress \
    -o "$FONT_DIR/roboto_${size}.c"

  echo "✓ Created roboto_${size}.c"
done

echo ""
echo "Next steps:"
echo "1. Update main/CMakeLists.txt to include font .c files"
echo "2. Create main/fonts/custom_fonts.h with LV_FONT_DECLARE"
echo "3. Rebuild project"
echo "4. Use fonts: &roboto_16, &roboto_24, etc."
```

## Reference

Official docs: https://github.com/lvgl/lv_font_conv
Online converter: https://lvgl.io/tools/fontconverter
