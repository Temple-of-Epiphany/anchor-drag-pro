/**
 * UI Header Component - Consistent Header Bar
 *
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2025-12-22
 * Date Updated: 2025-12-25
 * Version: 0.5.0
 *
 * Full-width header bar (800x80px) with:
 * - "ANCHOR DRAG ALARM" title (center)
 * - Left side icons: Bluetooth, WiFi, TF Card (SD Card)
 * - Right side icons: Anchor (Armed), Compass, GPS/Satellite
 *
 * Icon colors indicate status:
 * - Green: Active/Good (GPS found, Anchor armed, TF card detected)
 * - Blue: Connected (WiFi, Bluetooth, Compass found)
 * - Gray: Inactive/Off/Not found (default state)
 *
 * Appears on all screens as a persistent header
 *
 * Changelog:
 * - 0.5.0 (2025-12-25): Updated icons to GPS, Compass, TF Card, Anchor (armed), WiFi, Bluetooth
 * - 0.4.0 (2025-12-25): Filled all 6 status icons (Battery, SD Card, WiFi, Alarm, Compass, GPS)
 * - 0.3.0 (2025-12-25): Changed GPS icon to satellite emoji for better visibility
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

/**
 * Update Anchor Armed status icon (right side, position 2)
 * @param header Header object returned from ui_header_create()
 * @param armed true = Anchor armed (green), false = Not armed (gray)
 */
void ui_header_set_anchor_armed(lv_obj_t *header, bool armed);

/**
 * Update TF Card (SD Card) status icon (left side, position 2)
 * @param header Header object returned from ui_header_create()
 * @param detected true = TF card detected (green), false = Not detected (gray)
 */
void ui_header_set_tfcard_status(lv_obj_t *header, bool detected);

/**
 * Update WiFi status icon (left side, position 1)
 * @param header Header object returned from ui_header_create()
 * @param connected true = WiFi connected (blue), false = Disconnected (gray)
 */
void ui_header_set_wifi_status(lv_obj_t *header, bool connected);

/**
 * Update Bluetooth status icon (left side, position 0)
 * @param header Header object returned from ui_header_create()
 * @param connected true = Bluetooth connected (blue), false = Disconnected (gray)
 */
void ui_header_set_bluetooth_status(lv_obj_t *header, bool connected);

/**
 * Update time display in header
 * @param header Header object returned from ui_header_create()
 * @param hour Hour (0-23)
 * @param min Minute (0-59)
 * @param sec Second (0-59)
 * @return true if update succeeded, false if header invalid
 */
bool ui_header_set_time(lv_obj_t *header, int hour, int min, int sec);

#endif // UI_HEADER_H
