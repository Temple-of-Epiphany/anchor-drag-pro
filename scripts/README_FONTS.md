# Font Conversion Script

## Quick Start

### Convert a font with default sizes (16, 18, 20, 24, 32, 48)
```bash
./scripts/convert_font.sh path/to/YourFont.ttf
```

### Convert specific sizes only
```bash
./scripts/convert_font.sh path/to/YourFont.ttf 16 24 48
```

## What the script does:

1. ✅ Checks if Node.js is installed
2. ✅ Checks if lv_font_conv is installed (offers to install if not)
3. ✅ Validates your font file
4. ✅ Converts font to all requested sizes
5. ✅ Creates `main/fonts/` directory
6. ✅ Generates `custom_fonts.h` header file
7. ✅ Shows you exactly what to add to CMakeLists.txt
8. ✅ Provides usage examples

## Requirements

- Node.js 14+ (install: `brew install node`)
- lv_font_conv (script will install it if needed)

## Examples

### Example 1: Convert Roboto
```bash
# Download Roboto-Regular.ttf first from Google Fonts
./scripts/convert_font.sh ~/Downloads/Roboto-Regular.ttf
```

Output:
```
✓ Created main/fonts/roboto_regular_16.c
✓ Created main/fonts/roboto_regular_18.c
✓ Created main/fonts/roboto_regular_20.c
✓ Created main/fonts/roboto_regular_24.c
✓ Created main/fonts/roboto_regular_32.c
✓ Created main/fonts/roboto_regular_48.c
✓ Created main/fonts/custom_fonts.h
```

### Example 2: Convert only navigation sizes
```bash
./scripts/convert_font.sh ~/Downloads/OpenSans-Bold.ttf 16 20 24
```

### Example 3: Convert large display sizes
```bash
./scripts/convert_font.sh ~/Downloads/Teko-Bold.ttf 32 48
```

## After conversion:

### 1. Update CMakeLists.txt
The script will show you what to add. Example:
```cmake
idf_component_register(
    SRCS
        "main.c"
        # ... other files ...
        "fonts/roboto_regular_16.c"
        "fonts/roboto_regular_24.c"
    INCLUDE_DIRS "."
)
```

### 2. Use in your code
```c
#include "fonts/custom_fonts.h"

// Use the font
lv_obj_t *label = lv_label_create(parent);
lv_label_set_text(label, "Distance: 125 FT");
lv_obj_set_style_text_font(label, &roboto_regular_24, 0);
```

## Troubleshooting

### "Node.js is not installed"
```bash
# macOS
brew install node

# Verify
node --version
```

### "lv_font_conv not found"
The script will offer to install it. Say yes (y).

Or install manually:
```bash
npm install -g lv_font_conv
```

### "Font file not found"
Use absolute path or relative path from project root:
```bash
# Absolute
./scripts/convert_font.sh /Users/yourname/Downloads/Font.ttf

# Relative
./scripts/convert_font.sh ~/Downloads/Font.ttf
```

### Conversion fails for specific size
- Font might not support that size well
- Try different BPP value (edit script: BPP=2 instead of BPP=4)
- Check font file isn't corrupted

## Customization

Edit the script to change defaults:

```bash
# Line 26-28
DEFAULT_SIZES=(16 18 20 24 32 48)  # Change these
DEFAULT_RANGE="0x20-0x7F,0xB0"     # Add more characters
BPP=4                               # Quality (1-8)
```

### Character ranges:
- `0x20-0x7F` - Basic ASCII
- `0xB0` - Degree symbol (°)
- `0xB1` - Plus-minus (±)
- `0x2190-0x21FF` - Arrows

## Font Sources

### Free fonts (Google Fonts):
- Roboto: https://fonts.google.com/specimen/Roboto
- Open Sans: https://fonts.google.com/specimen/Open+Sans
- Source Sans Pro: https://fonts.google.com/specimen/Source+Sans+Pro
- Teko (condensed): https://fonts.google.com/specimen/Teko
- Orbitron (tech): https://fonts.google.com/specimen/Orbitron

Download as TTF and convert!
