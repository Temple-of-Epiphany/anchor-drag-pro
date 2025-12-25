/**
 * UI Header Component - Consistent Header Bar
 *
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2025-12-22
 * Date Updated: 2025-12-22
 * Version: 0.2.0
 *
 * Full-width header bar (800x80px) with:
 * - "ANCHOR DRAG ALARM" title (center)
 * - 3 icon placeholders (left side)
 * - 3 icon placeholders (right side)
 *
 * Appears on all screens as a persistent header
 *
 * Changelog:
 * - 0.2.0 (2025-12-22): Redesigned as full-width header bar with title and 6 icon placeholders
 * - 0.1.0 (2025-12-22): Initial implementation with GPS and Compass icons only
 */

#ifndef UI_HEADER_H
#define UI_HEADER_H

#include "lvgl.h"
#include <stdbool.h>

// Header dimensions
#define HEADER_HEIGHT 80
#define HEADER_WIDTH  800

/**
 * Create full-width header bar with title and icon placeholders
 * @param parent Parent screen/container to attach header to
 * @return Pointer to header container object
 */
lv_obj_t* ui_header_create(lv_obj_t *parent);

/**
 * Update GPS status icon (right side, position 1)
 * @param header Header object returned from ui_header_create()
 * @param found true = GPS found (green), false = GPS not found (gray)
 */
void ui_header_set_gps_status(lv_obj_t *header, bool found);

/**
 * Update Compass status icon (right side, position 0)
 * @param header Header object returned from ui_header_create()
 * @param found true = Compass found (blue), false = Compass not found (gray)
 */
void ui_header_set_compass_status(lv_obj_t *header, bool found);

#endif // UI_HEADER_H
