/**
 * Date/Time Settings Screen
 *
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2025-12-25
 * Version: 0.1.0
 *
 * Screen for setting date, time, and timezone:
 * - Manual date/time entry (year, month, day, hour, minute, second)
 * - GPS time sync button
 * - Timezone selector (GMT offset -12 to +14)
 * - Save/Cancel buttons
 */

#ifndef DATETIME_SETTINGS_H
#define DATETIME_SETTINGS_H

#include "lvgl.h"
#include "ui_footer.h"

/**
 * Create date/time settings screen
 * @param page_callback Callback for page navigation
 * @param footer_out Pointer to store footer object
 * @return Screen object
 */
lv_obj_t* create_datetime_settings_screen(ui_footer_page_cb_t page_callback, lv_obj_t **footer_out);

#endif // DATETIME_SETTINGS_H
