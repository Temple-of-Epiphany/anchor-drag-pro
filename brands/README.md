# brands/ — brand asset configuration

OEM-ready brand abstraction. Build with `BRAND=default` (or another brand directory name) to compile/render with that brand's colors, logo, icons, and metadata.

Tracked under [Workstream 0 prereq #46](https://github.com/Temple-of-Epiphany/anchor-drag-pro/issues/46).

| Directory | Purpose |
|---|---|
| `default/` | Anchor Drag Pro — the default brand |
| `oem-template/` | Starter for OEM partners to copy and customize |

Each brand directory has a `brand.json` (metadata + token overrides) and an `assets/` subdirectory (logo + icons in SVG).
