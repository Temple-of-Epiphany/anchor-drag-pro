/**
 * Display Refresh Test - Auto-updating screen without touch interaction
 *
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2025-12-24
 * Version: 0.1.0
 */

#ifndef DISPLAY_TEST_H
#define DISPLAY_TEST_H

#include "lvgl.h"

/**
 * Create and start display refresh test
 * Shows an auto-updating counter to test display refresh without touch
 */
void display_test_create(void);

/**
 * Stop display test and clean up
 */
void display_test_stop(void);

#endif // DISPLAY_TEST_H
