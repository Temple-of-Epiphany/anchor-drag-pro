/*
 * Anchor Drag Pro — LVGL adapter for anchor_design_tokens.h.
 *
 * Converts the cross-platform anchor_color_t (0x00RRGGBB) into LVGL's
 * lv_color_t at compile time. Use UI_COLOR(NAME) wherever LVGL needs a
 * colour — never write a hex literal.
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#pragma once

#include "anchor_design_tokens.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Convert anchor_color_t -> lv_color_t. Inline so the optimiser folds it
 * into a single mov when the input is a literal token. */
static inline lv_color_t anchor_color_to_lv(anchor_color_t c)
{
    return lv_color_make((uint8_t)((c >> 16) & 0xFF),
                         (uint8_t)((c >>  8) & 0xFF),
                         (uint8_t)( c        & 0xFF));
}

/* Sugar — UI_COLOR(BG_APP) reads ANCHOR_COLOR_BG_APP and converts. */
#define UI_COLOR(name) anchor_color_to_lv(ANCHOR_COLOR_##name)

#ifdef __cplusplus
}
#endif
