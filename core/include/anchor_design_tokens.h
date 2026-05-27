/*
 * Anchor Drag Pro — design tokens (cross-platform).
 *
 * Single source of truth for every colour referenced by the UI specs in
 * the firmware repo wiki. The ESP32 LVGL implementation, the iOS app,
 * and the Android app all consume the same token names. Hex literals in
 * widget code are a spec violation; reference tokens instead.
 *
 * Wiki contract:
 *   https://github.com/Temple-of-Epiphany/anchor-drag-pro/wiki/Color-Tokens
 *
 * Versioning: bump ANCHOR_TOKENS_SCHEMA_VERSION whenever a token is
 * renamed or removed. Adding new tokens is non-breaking.
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ANCHOR_TOKENS_SCHEMA_VERSION 1

/* ---- Token type ----
 * 24-bit RGB stored as 0x00RRGGBB. Platform layers convert to native
 * color types (LVGL lv_color_t, SwiftUI Color, Compose Color). */
typedef uint32_t anchor_color_t;

/* ---- Surfaces ---- */
#define ANCHOR_COLOR_BG_APP            ((anchor_color_t) 0x0A1929)  /* deep marine navy */
#define ANCHOR_COLOR_BG_CANVAS         ((anchor_color_t) 0x0E2438)  /* plot background */
#define ANCHOR_COLOR_SURFACE_ELEVATED  ((anchor_color_t) 0x152A40)
#define ANCHOR_COLOR_SURFACE_SUBTLE    ((anchor_color_t) 0x1C3550)
#define ANCHOR_COLOR_SURFACE_PRESSED   ((anchor_color_t) 0x223D5F)
#define ANCHOR_COLOR_SURFACE_BORDER    ((anchor_color_t) 0x2A4868)
#define ANCHOR_COLOR_SURFACE_MODAL     ((anchor_color_t) 0x14283D)
#define ANCHOR_COLOR_SURFACE_SCRIM     ((anchor_color_t) 0x000000)  /* applied at 60% alpha */

/* ---- Text ---- */
#define ANCHOR_COLOR_TEXT_BODY_STRONG  ((anchor_color_t) 0xFFFFFF)
#define ANCHOR_COLOR_TEXT_BODY         ((anchor_color_t) 0xD8E1ED)
#define ANCHOR_COLOR_TEXT_BODY_DIM     ((anchor_color_t) 0x8FA0B4)
#define ANCHOR_COLOR_TEXT_LABEL        ((anchor_color_t) 0xA8B6C8)
#define ANCHOR_COLOR_TEXT_DIM          ((anchor_color_t) 0x6B7B8F)

/* ---- State colours (per anchor state) ---- */
#define ANCHOR_COLOR_STATE_OFF         ((anchor_color_t) 0x1C3550)  /* matches surface.subtle */
#define ANCHOR_COLOR_STATE_ON          ((anchor_color_t) 0x2A6FD6)  /* marine blue */
#define ANCHOR_COLOR_STATE_ARMING      ((anchor_color_t) 0xE8A93D)  /* amber */
#define ANCHOR_COLOR_STATE_ARMED       ((anchor_color_t) 0x2EBE6E)  /* safe green */
#define ANCHOR_COLOR_STATE_ALARM       ((anchor_color_t) 0xE0413C)  /* alarm red */
#define ANCHOR_COLOR_STATE_WARNING     ((anchor_color_t) 0xF0B43A)  /* warning amber */

/* ---- Action buttons ---- */
#define ANCHOR_COLOR_ACTION_PRIMARY      ((anchor_color_t) 0x2A6FD6)
#define ANCHOR_COLOR_ACTION_SECONDARY    ((anchor_color_t) 0x3D5878)
#define ANCHOR_COLOR_ACTION_DESTRUCTIVE  ((anchor_color_t) 0xC23A35)
#define ANCHOR_COLOR_ACTION_WARNING      ((anchor_color_t) 0xE8A93D)

#ifdef __cplusplus
}
#endif
