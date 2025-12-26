# Adding Custom TTF Fonts to LVGL

## Method 1: LVGL Online Font Converter (Recommended)

### Step 1: Convert TTF to C Array
1. Go to https://lvgl.io/tools/fontconverter
2. **Upload your TTF file** (e.g., Roboto-Regular.ttf, Arial.ttf, etc.)
3. **Configure settings:**
   - Name: `my_font_16` (use lowercase, include size)
   - Size: `16` (or desired size)
   - Bpp: `4` (4-bit per pixel for good quality)
   - Range:
     - Basic: `0x20-0x7F` (ASCII printable)
     - Extended: `0x20-0x7F, 0xA0-0xFF` (Latin-1)
     - Symbols: Add specific Unicode ranges as needed
   - Format: `LVGL` (default)
4. **Click "Convert"** and download the `.c` file

### Step 2: Add to Project
```bash
# Create fonts directory
mkdir -p main/fonts

# Copy the downloaded file
cp ~/Downloads/my_font_16.c main/fonts/
```

### Step 3: Update CMakeLists.txt
Edit `main/CMakeLists.txt`:
```cmake
idf_component_register(
    SRCS
        "main.c"
        "display_driver.c"
        "lvgl_init.c"
        "splash_screen.c"
        "start_screen.c"
        "ui_footer.c"
        "ui_header.c"
        "smpte_test_screen.c"
        "splash_logo.c"
        "fonts/my_font_16.c"    # Add this line
    INCLUDE_DIRS "."
)
```

### Step 4: Declare Font in Header
Create `main/fonts/custom_fonts.h`:
```c
#ifndef CUSTOM_FONTS_H
#define CUSTOM_FONTS_H

#include "lvgl.h"

// Declare custom fonts (defined in generated .c files)
LV_FONT_DECLARE(my_font_16);
LV_FONT_DECLARE(my_font_24);
// Add more as needed

#endif // CUSTOM_FONTS_H
```

### Step 5: Use in Code
In your UI code (e.g., `ui_footer.c`):
```c
#include "fonts/custom_fonts.h"

// Use custom font
lv_obj_set_style_text_font(label, &my_font_16, 0);
```

## Method 2: Enable Additional Built-in Fonts

Edit `lv_conf.h` to enable more built-in fonts:

```c
// Enable more Montserrat sizes
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1  // Already enabled
#define LV_FONT_MONTSERRAT_18 1  // Already enabled
#define LV_FONT_MONTSERRAT_20 1  // Already enabled

// Enable other built-in fonts
#define LV_FONT_UNSCII_8  1      // Pixel font
#define LV_FONT_UNSCII_16 1      // Larger pixel font
```

Then use in code:
```c
lv_obj_set_style_text_font(label, &lv_font_unscii_16, 0);  // Pixel font
```

## Method 3: Offline Font Converter (Advanced)

For more control, use the offline converter:

```bash
# Install dependencies
pip install lv_font_conv

# Convert font
lv_font_conv --font YourFont.ttf \
             --size 16 \
             --bpp 4 \
             --format lvgl \
             --range 0x20-0x7F \
             --output my_font_16.c
```

## Recommended Fonts for Marine Display

### Good TTF Fonts for Readability:
- **Roboto** (Android default) - clean, modern
- **Open Sans** - highly readable
- **Source Sans Pro** - designed for UI
- **Ubuntu** - clear even at small sizes
- **Teko** (condensed) - fits more text in buttons
- **Orbitron** - futuristic/technical look
- **Seven Segment** - digital display style

### Font Sources:
- Google Fonts: https://fonts.google.com (free, open-source)
- Download as TTF/OTF and convert

## Example: Adding Roboto Font

```bash
# Download Roboto from Google Fonts
# Extract Roboto-Regular.ttf

# Convert sizes: 14, 16, 18, 20, 24
# Upload to https://lvgl.io/tools/fontconverter

# Results:
#   roboto_14.c
#   roboto_16.c
#   roboto_18.c
#   roboto_20.c
#   roboto_24.c

# Copy to project
cp roboto_*.c main/fonts/

# Update CMakeLists.txt to include all .c files
# Update custom_fonts.h to declare all fonts
# Use in code: &roboto_16, &roboto_24, etc.
```

## Font Size Recommendations

For 800×480 display at typical viewing distance:
- **Buttons/Footer**: 16-20px (readable from 2-3 feet)
- **Headers**: 20-24px
- **Body text**: 16-18px
- **Small labels**: 14px minimum
- **Large displays**: 24-32px

## Memory Considerations

Each font size takes flash memory:
- 16px ASCII: ~8-12 KB
- 24px ASCII: ~15-20 KB
- 32px ASCII: ~25-30 KB

Current flash usage: 555 KB / 1024 KB (47% free)
Safe to add 5-10 custom fonts without issue.

## Testing Fonts

Add test function to display all available fonts:
```c
void test_fonts(void) {
    lv_obj_t *screen = lv_scr_act();
    int y = 10;

    // Test each font
    lv_obj_t *label1 = lv_label_create(screen);
    lv_label_set_text(label1, "Montserrat 16: The quick brown fox");
    lv_obj_set_style_text_font(label1, &lv_font_montserrat_16, 0);
    lv_obj_set_pos(label1, 10, y);
    y += 30;

    lv_obj_t *label2 = lv_label_create(screen);
    lv_label_set_text(label2, "Roboto 18: The quick brown fox");
    lv_obj_set_style_text_font(label2, &roboto_18, 0);
    lv_obj_set_pos(label2, 10, y);
    y += 30;

    // Add more font tests...
}
```

---

**Current Project Status:**
- Enabled fonts: montserrat_16, 18, 20, 24, 32, 48
- Footer currently uses: montserrat_16 (v1.3.8)

**Next Steps:**
1. Rebuild current version with montserrat_16
2. Test readability on actual display
3. If needed, add custom TTF fonts using Method 1
