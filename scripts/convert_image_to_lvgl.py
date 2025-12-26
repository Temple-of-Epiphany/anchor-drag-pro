#!/usr/bin/env python3
"""
Convert PNG image to LVGL C array format (RGB565)

Author: Colin Bitterfield
Email: colin@bitterfield.com
Date Created: 2025-12-22
Date Updated: 2025-12-22
Version: 0.1.0
"""

import sys
from PIL import Image
import os

def rgb888_to_rgb565(r, g, b):
    """Convert RGB888 (8-bit per channel) to RGB565 (16-bit)"""
    r5 = (r >> 3) & 0x1F  # 5 bits for red
    g6 = (g >> 2) & 0x3F  # 6 bits for green
    b5 = (b >> 3) & 0x1F  # 5 bits for blue
    return (r5 << 11) | (g6 << 5) | b5

def convert_image_to_lvgl_c(input_png, output_c, output_h, var_name="splash_logo"):
    """
    Convert PNG image to LVGL C array format with proper RGB565 byte ordering
    """
    # Load image
    img = Image.open(input_png)

    # Convert to RGB if needed
    if img.mode != 'RGB':
        img = img.convert('RGB')

    width, height = img.size
    pixels = img.load()

    print(f"Converting {input_png} ({width}x{height}) to LVGL C format...")

    # Generate C file
    with open(output_c, 'w') as f:
        f.write(f"/**\n")
        f.write(f" * Splash Logo - Compiled Image Data\n")
        f.write(f" *\n")
        f.write(f" * Auto-generated from {os.path.basename(input_png)}\n")
        f.write(f" * Format: RGB565, {width}x{height} pixels\n")
        f.write(f" *\n")
        f.write(f" * Author: Colin Bitterfield\n")
        f.write(f" * Email: colin@bitterfield.com\n")
        f.write(f" * Date Created: 2025-12-22\n")
        f.write(f" * Version: 0.1.0\n")
        f.write(f" */\n\n")
        f.write(f"#include \"lvgl.h\"\n\n")

        # Write pixel data as byte array
        f.write(f"static const uint8_t {var_name}_data[] = {{\n")

        byte_count = 0
        for y in range(height):
            f.write("  ")
            for x in range(width):
                r, g, b = pixels[x, y][:3]  # Get RGB, ignore alpha if present
                rgb565 = rgb888_to_rgb565(r, g, b)

                # Split into two bytes (little-endian)
                low_byte = rgb565 & 0xFF
                high_byte = (rgb565 >> 8) & 0xFF

                f.write(f"0x{low_byte:02X}, 0x{high_byte:02X}, ")
                byte_count += 2

                # Line break every 12 pixels (24 bytes)
                if (x + 1) % 12 == 0 and x < width - 1:
                    f.write("\n  ")

            f.write("\n")

        f.write("};\n\n")

        # Write LVGL image descriptor
        f.write(f"const lv_img_dsc_t {var_name} = {{\n")
        f.write(f"  .header = {{\n")
        f.write(f"    .cf = LV_IMG_CF_TRUE_COLOR,\n")
        f.write(f"    .always_zero = 0,\n")
        f.write(f"    .reserved = 0,\n")
        f.write(f"    .w = {width},\n")
        f.write(f"    .h = {height},\n")
        f.write(f"  }},\n")
        f.write(f"  .data_size = {byte_count},\n")
        f.write(f"  .data = {var_name}_data,\n")
        f.write(f"}};\n")

    # Generate H file
    with open(output_h, 'w') as f:
        f.write(f"/**\n")
        f.write(f" * Splash Logo - Compiled Image Data\n")
        f.write(f" *\n")
        f.write(f" * Auto-generated from {os.path.basename(input_png)}\n")
        f.write(f" * Format: RGB565, {width}x{height} pixels\n")
        f.write(f" */\n\n")
        f.write(f"#ifndef SPLASH_LOGO_H\n")
        f.write(f"#define SPLASH_LOGO_H\n\n")
        f.write(f"#include \"lvgl.h\"\n\n")
        f.write(f"extern const lv_img_dsc_t {var_name};\n\n")
        f.write(f"#endif // SPLASH_LOGO_H\n")

    print(f"Generated {output_c} ({byte_count} bytes)")
    print(f"Generated {output_h}")
    print(f"Image size: {width}x{height} pixels")

if __name__ == "__main__":
    if len(sys.argv) != 4:
        print("Usage: convert_image_to_lvgl.py <input.png> <output.c> <output.h>")
        sys.exit(1)

    input_png = sys.argv[1]
    output_c = sys.argv[2]
    output_h = sys.argv[3]

    if not os.path.exists(input_png):
        print(f"Error: Input file '{input_png}' not found")
        sys.exit(1)

    convert_image_to_lvgl_c(input_png, output_c, output_h)
    print("Conversion complete!")