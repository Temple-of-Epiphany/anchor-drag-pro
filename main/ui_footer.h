/**
 * UI Footer Navigation Bar Component
 *
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2025-12-22
 * Date Updated: 2025-12-22
 * Version: 0.2.0
 *
 * Changelog:
 * - 0.2.0 (2025-12-22): Added auto-hide, swipe gestures, button navigation
 * - 0.1.0 (2025-12-22): Initial implementation with page indicators
 *
 * Navigation footer bar (800x60px) with:
 * - Clickable buttons for each screen (START, INFO, PGN, CONFIG, UPDATE, TOOLS)
 * - Auto-hide after 10 seconds of inactivity
 * - Swipe-up gesture to reveal hidden footer
 * - Swipe left/right to navigate between buttons
 * - Page indicators showing current position
 *
 * Appears on navigation screens (START, INFO, PGN, CONFIG, UPDATE, TOOLS)
 * Hidden on DISPLAY and SPLASH screens
 */

#ifndef UI_FOOTER_H
#define UI_FOOTER_H

#include "lvgl.h"
#include <stdint.h>
#include <stdbool.h>

// Footer dimensions
#define FOOTER_HEIGHT 60
#define FOOTER_WIDTH  800
#define FOOTER_AUTO_HIDE_MS 10000  // Auto-hide after 10 seconds of inactivity

// Debug control
#define FOOTER_DEBUG_ENABLED 0  // Set to 1 to enable verbose debug logging

// Page indices
typedef enum {
    PAGE_START = 0,
    PAGE_INFO = 1,
    PAGE_PGN = 2,
    PAGE_CONFIG = 3,
    PAGE_UPDATE = 4,
    PAGE_TOOLS = 5,
    PAGE_COUNT = 6
} ui_page_t;

// Page navigation callback
typedef void (*ui_footer_page_cb_t)(ui_page_t page);

/**
 * Create navigation footer bar
 * @param parent Parent screen/container to attach footer to
 * @param current_page Current page index (0-5)
 * @param page_callback Callback function when page button is clicked
 * @return Pointer to footer container object
 */
lv_obj_t* ui_footer_create(lv_obj_t *parent, ui_page_t current_page, ui_footer_page_cb_t page_callback);

/**
 * Update current page
 * @param footer Footer object returned from ui_footer_create()
 * @param current_page Current page index (0-5)
 */
void ui_footer_set_page(lv_obj_t *footer, ui_page_t current_page);

/**
 * Show footer and start auto-hide timer
 * @param footer Footer object
 */
void ui_footer_show(lv_obj_t *footer);

/**
 * Hide footer immediately
 * @param footer Footer object
 */
void ui_footer_hide(lv_obj_t *footer);

/**
 * Check if footer is currently visible
 * @param footer Footer object
 * @return true if visible, false if hidden
 */
bool ui_footer_is_visible(lv_obj_t *footer);

/**
 * Reset auto-hide timer (call when user interacts with footer)
 * @param footer Footer object
 */
void ui_footer_reset_timer(lv_obj_t *footer);

/**
 * Clean up footer resources (call before deleting footer)
 * @param footer Footer object
 */
void ui_footer_cleanup(lv_obj_t *footer);

/**
 * Enable or disable debug logging
 * @param enable true to enable, false to disable
 */
void ui_footer_set_debug(bool enable);

#endif // UI_FOOTER_H
