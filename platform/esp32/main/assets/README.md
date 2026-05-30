# Anchor Drag Pro — brand assets

## Layout

```
assets/
  source/             # editable PNGs — source of truth
    anchor_logo.png   # default brand logo (1024x1024 RGB)
  generated/          # auto-generated LVGL C arrays — DO NOT hand-edit
    anchor_logo_140.c # 140x140 RGB565 little-endian
  anchor_logo.h       # public declarations for the generated descriptors
  convert_logo.py     # PNG -> RGB565 LVGL image descriptor converter
```

## Regenerating after a logo swap

Replace `source/anchor_logo.png` (any size — it's resized to square)
then re-run the converter and rebuild:

```bash
cd platform/esp32/main/assets
python3 convert_logo.py 140
cd ../../.. && make build flash
```

Pass extra sizes on the command line to emit more variants
(`python3 convert_logo.py 140 96 256`). Add the corresponding
`extern` for each new size to `anchor_logo.h` so screens can pick
them up at compile time.

## Brand abstraction

This is the default brand. Per issue #46 (OEM brand abstraction) the
runtime resolution of which logo to display will eventually go
through the `[[brand]]` config and a per-brand `assets/<brand>/`
directory. For v0.2 we ship one logo and link it directly.

## Format

- `LV_IMG_CF_TRUE_COLOR` (no palette, no alpha mask)
- RGB565 little-endian, 2 bytes per pixel
- Matches `LV_COLOR_DEPTH 16` in `lv_conf.h`; LVGL flushes the bytes
  straight to the ST7262 panel without channel reorder
