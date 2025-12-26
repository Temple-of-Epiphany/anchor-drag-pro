#!/usr/bin/env python3
"""
Modify TV Test Pattern - Replace center circle and text

Author: Colin Bitterfield
Email: colin@bitterfield.com
Date Created: 2025-12-22
Date Updated: 2025-12-23
Version: 1.4.0

This script modifies the TV test pattern image by:
1. Detecting transparent areas in the template (alpha channel)
2. Filling the transparent circle with the splash logo (circular mask)
3. Filling the small square with "ADA" text
4. Filling the large rectangle with gray box + text
"""

from PIL import Image, ImageDraw, ImageFont
import os

# File paths
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_DIR = os.path.dirname(SCRIPT_DIR)
ASSETS_DIR = os.path.join(PROJECT_DIR, 'assets')

TEST_PATTERN_PATH = os.path.join(ASSETS_DIR, 'tv-test-patterns-template.png')
SPLASH_LOGO_PATH = os.path.join(ASSETS_DIR, 'splash_logo.png')
OUTPUT_PATH = os.path.join(ASSETS_DIR, 'tv-test-pattern-custom.png')

def find_transparent_circle(img):
    """Find the bounds of the transparent circle in the image"""
    import numpy as np

    # Must have alpha channel
    if img.mode != 'RGBA':
        raise ValueError("Image must have alpha channel (RGBA)")

    img_array = np.array(img)
    height, width = img_array.shape[:2]

    # Look for transparent pixels (alpha < 255) in center area
    alpha_channel = img_array[:, :, 3]
    transparent_mask = alpha_channel < 255

    # Search center region
    center_start_y = height // 4
    center_end_y = 3 * height // 4
    center_start_x = width // 4
    center_end_x = 3 * width // 4

    center_transparent = transparent_mask[center_start_y:center_end_y, center_start_x:center_end_x]

    if np.any(center_transparent):
        rows, cols = np.where(center_transparent)
        min_y = rows.min() + center_start_y
        max_y = rows.max() + center_start_y
        min_x = cols.min() + center_start_x
        max_x = cols.max() + center_start_x

        center_x = (min_x + max_x) // 2
        center_y = (min_y + max_y) // 2
        radius = min((max_x - min_x) // 2, (max_y - min_y) // 2)

        print(f"  DEBUG: Transparent circle bounds: x({min_x}-{max_x}), y({min_y}-{max_y})")
        return center_x, center_y, radius
    else:
        raise ValueError("No transparent circle found in center area")

def find_transparent_regions(img):
    """Find all three transparent regions using connected component analysis"""
    import numpy as np
    from scipy import ndimage

    # Must have alpha channel
    if img.mode != 'RGBA':
        raise ValueError("Image must have alpha channel (RGBA)")

    img_array = np.array(img)
    alpha_channel = img_array[:, :, 3]
    transparent_mask = alpha_channel < 255

    # Label connected transparent regions
    labeled_array, num_features = ndimage.label(transparent_mask)

    # Analyze each region
    regions = []
    for region_id in range(1, num_features + 1):
        region_mask = labeled_array == region_id
        rows, cols = np.where(region_mask)

        min_y = rows.min()
        max_y = rows.max()
        min_x = cols.min()
        max_x = cols.max()

        width = max_x - min_x
        height = max_y - min_y
        area = np.sum(region_mask)

        regions.append({
            'id': region_id,
            'bounds': (min_x, min_y, max_x, max_y),
            'width': width,
            'height': height,
            'area': area
        })

    # Sort by area (largest first)
    regions.sort(key=lambda r: r['area'], reverse=True)

    if len(regions) < 3:
        raise ValueError(f"Expected 3 transparent regions, found {len(regions)}")

    # Regions are: [0]=circle (largest), [1]=rectangle, [2]=square (smallest)
    circle = regions[0]['bounds']
    rectangle = regions[1]['bounds']
    square = regions[2]['bounds']

    return circle, square, rectangle

def modify_test_pattern(new_text_line1="ANCHOR DRAG PRO", new_text_line2="Display Self-Test Pattern"):
    """
    Modify the test pattern by filling transparent areas

    Args:
        new_text_line1: First line of text (default: "ANCHOR DRAG PRO")
        new_text_line2: Second line of text (default: "Display Self-Test Pattern")
    """

    print("Loading test pattern template...")
    test_pattern = Image.open(TEST_PATTERN_PATH).convert('RGBA')
    width, height = test_pattern.size
    print(f"  Template size: {width}x{height}")

    print("Loading splash logo...")
    splash_logo = Image.open(SPLASH_LOGO_PATH).convert('RGBA')
    print(f"  Original logo size: {splash_logo.size[0]}x{splash_logo.size[1]}")

    # Find all three transparent regions FIRST (before filling anything)
    print("Detecting all transparent regions...")
    circle_bounds, square_bounds, rect_bounds = find_transparent_regions(test_pattern)

    circle_x1, circle_y1, circle_x2, circle_y2 = circle_bounds
    square_x1, square_y1, square_x2, square_y2 = square_bounds
    text_x1, text_y1, text_x2, text_y2 = rect_bounds

    print(f"  Circle: ({circle_x1}, {circle_y1}) to ({circle_x2}, {circle_y2})")
    print(f"  Small square: ({square_x1}, {square_y1}) to ({square_x2}, {square_y2})")
    print(f"  Large rectangle: ({text_x1}, {text_y1}) to ({text_x2}, {text_y2})")

    # Calculate circle center and radius for logo placement
    center_x = (circle_x1 + circle_x2) // 2
    center_y = (circle_y1 + circle_y2) // 2
    radius = min((circle_x2 - circle_x1) // 2, (circle_y2 - circle_y1) // 2)
    print(f"  Circle center: ({center_x}, {center_y}), radius: {radius}")

    # Resize logo to fill entire white circle area (gray border is separate)
    # Use full diameter of white circle plus 7px
    logo_size = int(radius * 2.0) + 7  # Full diameter + 7px as requested
    logo_resized = splash_logo.resize((logo_size, logo_size), Image.Resampling.LANCZOS)
    print(f"  Resized logo to: {logo_size}x{logo_size}")

    # Create circular mask for logo to make it appear as a circle
    mask = Image.new('L', (logo_size, logo_size), 0)
    mask_draw = ImageDraw.Draw(mask)
    mask_draw.ellipse((0, 0, logo_size, logo_size), fill=255)

    # Position logo centered in circle
    logo_x = center_x - logo_size // 2
    logo_y = center_y - logo_size // 2

    print(f"  Pasting logo at ({logo_x}, {logo_y}) with circular mask...")

    # Create a circular version of the logo
    # Start with transparent background
    logo_circular = Image.new('RGBA', (logo_size, logo_size), (0, 0, 0, 0))

    # Get logo's alpha channel and combine with circular mask
    logo_alpha = logo_resized.split()[3] if len(logo_resized.split()) >= 4 else Image.new('L', (logo_size, logo_size), 255)

    # Multiply logo alpha with circular mask
    import numpy as np
    logo_alpha_np = np.array(logo_alpha)
    mask_np = np.array(mask)
    combined_alpha = (logo_alpha_np.astype(float) * mask_np.astype(float) / 255.0).astype(np.uint8)
    combined_mask = Image.fromarray(combined_alpha)

    # Composite logo with combined mask
    logo_circular.paste(logo_resized, (0, 0), combined_mask)

    # Paste circular logo onto test pattern
    test_pattern.paste(logo_circular, (logo_x, logo_y), logo_circular)

    # Fill small square with gray background + "ADA"
    print("\nFilling small square with 'ADA'...")
    draw_square = ImageDraw.Draw(test_pattern)
    draw_square.rectangle((square_x1, square_y1, square_x2, square_y2), fill=(128, 128, 128, 255))

    # Draw "ADA" text in small square
    try:
        font_ada = ImageFont.truetype("/System/Library/Fonts/Helvetica.ttc", 20)
    except:
        font_ada = ImageFont.load_default()

    square_center_x = (square_x1 + square_x2) // 2
    square_center_y = (square_y1 + square_y2) // 2
    bbox_ada = draw_square.textbbox((0, 0), "ADA", font=font_ada)
    ada_width = bbox_ada[2] - bbox_ada[0]
    ada_height = bbox_ada[3] - bbox_ada[1]
    ada_x = square_center_x - ada_width // 2
    ada_y = square_center_y - ada_height // 2

    draw_square.text((ada_x, ada_y), "ADA", fill=(255, 255, 255, 255), font=font_ada)
    print(f"  Drew 'ADA' at ({ada_x}, {ada_y})")

    # Fill large rectangle with gray background (NO PADDING)
    print("\nFilling large rectangle with text...")
    text_width = text_x2 - text_x1
    text_height = text_y2 - text_y1
    print(f"  Rectangle size: {text_width}x{text_height}")

    # Create a separate image for the text rectangle
    text_rect_img = Image.new('RGBA', (text_width, text_height), (128, 128, 128, 255))
    draw_rect = ImageDraw.Draw(text_rect_img)

    # Load font
    try:
        font_large = ImageFont.truetype("/System/Library/Fonts/Helvetica.ttc", 28)
        font_small = ImageFont.truetype("/System/Library/Fonts/Helvetica.ttc", 18)
    except:
        print("  Warning: Could not load system font, using default")
        font_large = ImageFont.load_default()
        font_small = ImageFont.load_default()

    # Draw text centered in the rectangle image
    rect_center_x = text_width // 2
    rect_center_y = text_height // 2

    # First line
    bbox1 = draw_rect.textbbox((0, 0), new_text_line1, font=font_large)
    text1_width = bbox1[2] - bbox1[0]
    text1_height = bbox1[3] - bbox1[1]
    text1_x = rect_center_x - text1_width // 2

    # Second line
    bbox2 = draw_rect.textbbox((0, 0), new_text_line2, font=font_small)
    text2_width = bbox2[2] - bbox2[0]
    text2_height = bbox2[3] - bbox2[1]
    text2_x = rect_center_x - text2_width // 2

    # Center both lines vertically in the rectangle
    total_text_height = text1_height + 8 + text2_height  # 8px gap between lines
    text1_y = rect_center_y - total_text_height // 2
    text2_y = text1_y + text1_height + 8

    print(f"  Drawing text at ({text1_x}, {text1_y}) and ({text2_x}, {text2_y})")
    draw_rect.text((text1_x, text1_y), new_text_line1, fill=(255, 255, 255, 255), font=font_large)
    draw_rect.text((text2_x, text2_y), new_text_line2, fill=(255, 255, 255, 255), font=font_small)

    # Paste the text rectangle onto the test pattern
    test_pattern.paste(text_rect_img, (text_x1, text_y1), text_rect_img)
    print(f"  Pasted text rectangle at ({text_x1}, {text_y1})")

    # Save
    print(f"Saving modified test pattern to: {OUTPUT_PATH}")
    test_pattern = test_pattern.convert('RGB')
    test_pattern.save(OUTPUT_PATH, 'PNG')
    print("Done!")
    print(f"\nModified test pattern saved as: {os.path.basename(OUTPUT_PATH)}")
    print(f"  Circle filled with splash logo (circular mask)")
    print(f"  Text: '{new_text_line1}' / '{new_text_line2}'")

if __name__ == '__main__':
    import sys

    if len(sys.argv) >= 3:
        line1 = sys.argv[1]
        line2 = sys.argv[2]
        print(f"Using custom text:")
        print(f"  Line 1: {line1}")
        print(f"  Line 2: {line2}")
        modify_test_pattern(line1, line2)
    else:
        print("Using default text:")
        print("  Line 1: ANCHOR DRAG PRO")
        print("  Line 2: Display Self-Test Pattern")
        print("\nTo use custom text, run:")
        print("  python modify_test_pattern.py 'Your Line 1' 'Your Line 2'")
        print()
        modify_test_pattern()
