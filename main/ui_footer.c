/**
 * UI Footer Navigation Bar Implementation
 *
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2025-12-22
 * Date Updated: 2025-12-23
 * Version: 0.6.0
 *
 * Changelog:
 * - 0.6.0 (2025-12-23): Fixed timer - use repeating timer that pauses after hiding
 * - 0.5.0 (2025-12-23): Fixed auto-hide timer to be one-shot instead of repeating
 * - 0.4.0 (2025-12-22): Added debug control, removed visual artifacts
 * - 0.3.0 (2025-12-22): Improved button styling, fixed auto-hide timer
 * - 0.2.0 (2025-12-22): Added auto-hide, swipe gestures, button navigation
 * - 0.1.0 (2025-12-22): Initial implementation with page indicators
 */

#include "ui_footer.h"
#include "fonts/custom_fonts.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static const char *TAG = "ui_footer";

// Debug control - can be toggled at runtime
static bool debug_enabled = FOOTER_DEBUG_ENABLED;

// Debug logging macro - always logs with [DEBUG] prefix when enabled
#define FOOTER_LOG_DEBUG(format, ...) \
    do { \
        if (debug_enabled) { \
            ESP_LOGI(TAG, "[DEBUG] " format, ##__VA_ARGS__); \
        } \
    } while(0)

// Always log important events regardless of debug flag
#define FOOTER_LOG_EVENT(format, ...) ESP_LOGI(TAG, "[EVENT] " format, ##__VA_ARGS__)

// Page names for navigation buttons
static const char *page_names[] = {
    "START",
    "INFO",
    "PGN",
    "CONFIG",
    "UPDATE",
    "TOOLS"
};

// Footer data structure
typedef struct {
    lv_obj_t *footer_bar;              // Main footer container
    lv_obj_t *button_container;        // Container for buttons
    lv_obj_t *page_buttons[PAGE_COUNT]; // Buttons for each page
    lv_timer_t *auto_hide_timer;       // Timer for auto-hide
    ui_page_t current_page;            // Current page index
    ui_footer_page_cb_t page_callback; // Page navigation callback
    bool is_visible;                   // Visibility state
} ui_footer_data_t;

// Forward declarations
static void auto_hide_timer_cb(lv_timer_t *timer);
static void test_timer_cb(lv_timer_t *timer);
static void button_event_cb(lv_event_t *e);
static void footer_gesture_event_cb(lv_event_t *e);

/**
 * Enable or disable debug logging
 */
void ui_footer_set_debug(bool enable) {
    debug_enabled = enable;
    ESP_LOGI(TAG, "Debug logging %s", enable ? "ENABLED" : "DISABLED");
}

/**
 * Test timer callback - fires every 2 seconds to verify timer system works
 */
static void test_timer_cb(lv_timer_t *timer) {
    static uint32_t count = 0;
    count++;
    ESP_LOGI(TAG, "[TEST_TIMER] Fired! Count=%lu (timer system is working)", count);
}

/**
 * Auto-hide timer callback - hides footer after timeout
 */
static void auto_hide_timer_cb(lv_timer_t *timer) {
    FOOTER_LOG_DEBUG("=== TIMER CALLBACK FIRED ===");

    ui_footer_data_t *data = (ui_footer_data_t *)timer->user_data;  // LVGL 8.x API
    if (data == NULL) {
        ESP_LOGE(TAG, "Timer callback: data is NULL!");
        return;
    }

    FOOTER_LOG_DEBUG("    Timer callback: data=%p, visible=%d", (void*)data, data->is_visible);

    if (data->is_visible) {
        FOOTER_LOG_EVENT("AUTO-HIDE TIMER EXPIRED - Hiding footer after 10 seconds");
        ui_footer_hide(data->footer_bar);
        // Pause timer after hiding to prevent repeated firing
        lv_timer_pause(timer);
        FOOTER_LOG_DEBUG("    Timer paused after hiding");
    } else {
        FOOTER_LOG_DEBUG("    Footer already hidden, timer callback ignored");
    }
}

/**
 * Button click event handler
 */
static void button_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn = lv_event_get_target(e);
    ui_footer_data_t *data = (ui_footer_data_t *)lv_event_get_user_data(e);

    // Log ALL events to debug why clicks aren't working
    ESP_LOGI(TAG, "[EVENT] button_event_cb called: code=%d, btn=%p, data=%p", code, (void*)btn, (void*)data);

    if (code == LV_EVENT_CLICKED && data != NULL) {
        // Find which button was clicked
        for (int i = 0; i < PAGE_COUNT; i++) {
            if (btn == data->page_buttons[i]) {
                // Always log button presses
                FOOTER_LOG_EVENT("BUTTON PRESSED: [%s] (Page %d)", page_names[i], i);

                // Update current page
                ui_footer_set_page(data->footer_bar, (ui_page_t)i);

                // Call page navigation callback
                if (data->page_callback != NULL) {
                    FOOTER_LOG_DEBUG("    Calling page callback for [%s]", page_names[i]);
                    data->page_callback((ui_page_t)i);
                }

                // Reset auto-hide timer
                ui_footer_reset_timer(data->footer_bar);
                break;
            }
        }
    }
}

/**
 * Gesture event handler for swipe-up to reveal footer
 */
static void footer_gesture_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    ui_footer_data_t *data = (ui_footer_data_t *)lv_event_get_user_data(e);

    if (code == LV_EVENT_GESTURE && data != NULL) {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());

        if (dir == LV_DIR_TOP && !data->is_visible) {
            // Swipe up - show footer
            FOOTER_LOG_EVENT("SWIPE UP - Showing footer");
            ui_footer_show(data->footer_bar);
        } else if (dir == LV_DIR_BOTTOM && data->is_visible) {
            // Swipe down - hide footer
            FOOTER_LOG_EVENT("SWIPE DOWN - Hiding footer");
            ui_footer_hide(data->footer_bar);
        } else if (dir == LV_DIR_LEFT && data->is_visible) {
            // Swipe left - next page
            ui_page_t next_page = (data->current_page + 1) % PAGE_COUNT;
            FOOTER_LOG_EVENT("SWIPE LEFT - Navigating to [%s] (Page %d)",
                           page_names[next_page], next_page);
            ui_footer_set_page(data->footer_bar, next_page);
            if (data->page_callback != NULL) {
                data->page_callback(next_page);
            }
            ui_footer_reset_timer(data->footer_bar);
        } else if (dir == LV_DIR_RIGHT && data->is_visible) {
            // Swipe right - previous page
            ui_page_t prev_page = (data->current_page == 0) ? (PAGE_COUNT - 1) : (data->current_page - 1);
            FOOTER_LOG_EVENT("SWIPE RIGHT - Navigating to [%s] (Page %d)",
                           page_names[prev_page], prev_page);
            ui_footer_set_page(data->footer_bar, prev_page);
            if (data->page_callback != NULL) {
                data->page_callback(prev_page);
            }
            ui_footer_reset_timer(data->footer_bar);
        }
    }
}

/**
 * Create navigation footer bar
 */
lv_obj_t* ui_footer_create(lv_obj_t *parent, ui_page_t current_page, ui_footer_page_cb_t page_callback) {
    ESP_LOGI(TAG, "=== CREATING FOOTER BAR ===");
    ESP_LOGI(TAG, "    Initial page: [%s] (%d)", page_names[current_page], current_page);
    ESP_LOGI(TAG, "    Auto-hide timeout: %d ms", FOOTER_AUTO_HIDE_MS);

    // Allocate footer data
    ui_footer_data_t *data = malloc(sizeof(ui_footer_data_t));
    if (data == NULL) {
        ESP_LOGE(TAG, "Failed to allocate footer data");
        return NULL;
    }
    memset(data, 0, sizeof(ui_footer_data_t));
    data->current_page = current_page;
    data->page_callback = page_callback;
    data->is_visible = true;  // Start visible

    // Create footer bar container (full width, clean design)
    data->footer_bar = lv_obj_create(parent);
    lv_obj_set_size(data->footer_bar, FOOTER_WIDTH, FOOTER_HEIGHT);
    lv_obj_align(data->footer_bar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(data->footer_bar, lv_color_hex(0x001F3F), 0);  // Marine dark
    lv_obj_set_style_bg_opa(data->footer_bar, LV_OPA_90, 0);  // Nearly opaque
    lv_obj_set_style_border_width(data->footer_bar, 0, 0);  // No border
    lv_obj_set_style_pad_all(data->footer_bar, 10, 0);
    lv_obj_set_style_radius(data->footer_bar, 0, 0);
    lv_obj_clear_flag(data->footer_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_shadow_width(data->footer_bar, 20, 0);
    lv_obj_set_style_shadow_color(data->footer_bar, lv_color_black(), 0);
    lv_obj_set_style_shadow_opa(data->footer_bar, LV_OPA_60, 0);
    lv_obj_set_style_shadow_ofs_y(data->footer_bar, -8, 0);

    // Enable gesture detection on footer
    lv_obj_add_event_cb(data->footer_bar, footer_gesture_event_cb, LV_EVENT_GESTURE, data);
    lv_obj_add_flag(data->footer_bar, LV_OBJ_FLAG_GESTURE_BUBBLE);

    // Create button container (horizontally arranged buttons with scrolling)
    data->button_container = lv_obj_create(data->footer_bar);
    lv_obj_set_size(data->button_container, FOOTER_WIDTH - 20, FOOTER_HEIGHT - 20);
    lv_obj_center(data->button_container);
    lv_obj_set_style_bg_opa(data->button_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(data->button_container, 0, 0);
    lv_obj_set_style_pad_all(data->button_container, 0, 0);
    lv_obj_set_style_pad_column(data->button_container, 6, 0);  // 6px gap between buttons
    lv_obj_set_flex_flow(data->button_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(data->button_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Enable horizontal scrolling, disable vertical
    lv_obj_set_scrollbar_mode(data->button_container, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_scroll_dir(data->button_container, LV_DIR_HOR);  // Horizontal only
    lv_obj_set_style_pad_right(data->button_container, 10, 0);  // Padding at end

    // Create navigation buttons for each page with fixed width
    int button_width = 120;  // Fixed width - allows scrolling if too many buttons
    FOOTER_LOG_DEBUG("    Creating %d buttons (width: %d px each, scrollable)", PAGE_COUNT, button_width);

    for (int i = 0; i < PAGE_COUNT; i++) {
        data->page_buttons[i] = lv_btn_create(data->button_container);
        lv_obj_set_size(data->page_buttons[i], button_width, 38);

        // Button styling - highlight current page
        if (i == current_page) {
            // Active button - ocean teal with gradient effect
            lv_obj_set_style_bg_color(data->page_buttons[i], lv_color_hex(0x39CCCC), 0);
            lv_obj_set_style_bg_grad_color(data->page_buttons[i], lv_color_hex(0x2A9999), 0);
            lv_obj_set_style_bg_grad_dir(data->page_buttons[i], LV_GRAD_DIR_VER, 0);
            lv_obj_set_style_shadow_width(data->page_buttons[i], 10, 0);
            lv_obj_set_style_shadow_color(data->page_buttons[i], lv_color_hex(0x39CCCC), 0);
            lv_obj_set_style_shadow_opa(data->page_buttons[i], LV_OPA_70, 0);
            lv_obj_set_style_shadow_ofs_y(data->page_buttons[i], 0, 0);
        } else {
            // Inactive button - darker blue
            lv_obj_set_style_bg_color(data->page_buttons[i], lv_color_hex(0x003366), 0);
            lv_obj_set_style_bg_grad_color(data->page_buttons[i], lv_color_hex(0x002244), 0);
            lv_obj_set_style_bg_grad_dir(data->page_buttons[i], LV_GRAD_DIR_VER, 0);
            lv_obj_set_style_shadow_width(data->page_buttons[i], 5, 0);
            lv_obj_set_style_shadow_color(data->page_buttons[i], lv_color_black(), 0);
            lv_obj_set_style_shadow_opa(data->page_buttons[i], LV_OPA_50, 0);
            lv_obj_set_style_shadow_ofs_y(data->page_buttons[i], 2, 0);
        }

        // Common button styling
        lv_obj_set_style_border_width(data->page_buttons[i], 0, 0);  // No border
        lv_obj_set_style_radius(data->page_buttons[i], 6, 0);

        // Pressed state - bright teal with white border for obvious feedback
        lv_obj_set_style_bg_color(data->page_buttons[i], lv_color_hex(0x00FFFF), LV_STATE_PRESSED);  // Bright cyan
        lv_obj_set_style_bg_grad_color(data->page_buttons[i], lv_color_hex(0x00CCCC), LV_STATE_PRESSED);
        lv_obj_set_style_border_width(data->page_buttons[i], 2, LV_STATE_PRESSED);  // White border when pressed
        lv_obj_set_style_border_color(data->page_buttons[i], lv_color_white(), LV_STATE_PRESSED);
        lv_obj_set_style_shadow_width(data->page_buttons[i], 15, LV_STATE_PRESSED);  // Bigger glow
        lv_obj_set_style_shadow_color(data->page_buttons[i], lv_color_hex(0x00FFFF), LV_STATE_PRESSED);
        lv_obj_set_style_shadow_opa(data->page_buttons[i], LV_OPA_80, LV_STATE_PRESSED);
        lv_obj_set_style_transform_height(data->page_buttons[i], -3, LV_STATE_PRESSED);  // Slightly inset

        // Button label with GolosText font (geometric, excellent readability)
        lv_obj_t *label = lv_label_create(data->page_buttons[i]);
        lv_label_set_text(label, page_names[i]);
        lv_obj_set_style_text_color(label, lv_color_white(), 0);
        lv_obj_set_style_text_font(label, &golostext_regular_16, 0);  // Custom font
        lv_obj_center(label);

        // Add click event handler
        lv_obj_add_event_cb(data->page_buttons[i], button_event_cb, LV_EVENT_CLICKED, data);
        lv_obj_add_flag(data->page_buttons[i], LV_OBJ_FLAG_CLICKABLE);
    }

    // Store data as user data
    lv_obj_set_user_data(data->footer_bar, data);

    // Create auto-hide timer only if timeout > 0
    if (FOOTER_AUTO_HIDE_MS > 0) {
        // IMPORTANT: This is a repeating timer - callback will pause it after hiding
        data->auto_hide_timer = lv_timer_create(auto_hide_timer_cb, FOOTER_AUTO_HIDE_MS, data);
        if (data->auto_hide_timer == NULL) {
            ESP_LOGE(TAG, "Failed to create auto-hide timer!");
        } else {
            FOOTER_LOG_EVENT("Auto-hide timer created: will hide footer in %d ms", FOOTER_AUTO_HIDE_MS);
            FOOTER_LOG_DEBUG("    Timer address: %p", (void*)data->auto_hide_timer);
        }
    } else {
        data->auto_hide_timer = NULL;
        FOOTER_LOG_EVENT("Auto-hide disabled (timeout = 0)");
    }

    // Test timer disabled - timers are now working correctly
    // lv_timer_t *test_timer = lv_timer_create(test_timer_cb, 2000, NULL);
    // if (test_timer != NULL) {
    //     ESP_LOGI(TAG, "[TEST] Created 2-second test timer to verify timer system works");
    // }

    FOOTER_LOG_DEBUG("=== FOOTER BAR CREATED (visible, timer running) ===");
    return data->footer_bar;
}

/**
 * Update current page
 */
void ui_footer_set_page(lv_obj_t *footer, ui_page_t current_page) {
    if (footer == NULL) return;

    ui_footer_data_t *data = (ui_footer_data_t *)lv_obj_get_user_data(footer);
    if (data == NULL) return;

    data->current_page = current_page;

    // Update button styling to highlight current page
    for (int i = 0; i < PAGE_COUNT; i++) {
        if (i == current_page) {
            // Active button - ocean teal with gradient and glow
            lv_obj_set_style_bg_color(data->page_buttons[i], lv_color_hex(0x39CCCC), 0);
            lv_obj_set_style_bg_grad_color(data->page_buttons[i], lv_color_hex(0x2A9999), 0);
            lv_obj_set_style_shadow_width(data->page_buttons[i], 10, 0);
            lv_obj_set_style_shadow_color(data->page_buttons[i], lv_color_hex(0x39CCCC), 0);
            lv_obj_set_style_shadow_opa(data->page_buttons[i], LV_OPA_70, 0);
            lv_obj_set_style_shadow_ofs_y(data->page_buttons[i], 0, 0);
            FOOTER_LOG_DEBUG("    Button [%d] set to ACTIVE (teal)", i);
        } else {
            // Inactive button - darker blue
            lv_obj_set_style_bg_color(data->page_buttons[i], lv_color_hex(0x003366), 0);
            lv_obj_set_style_bg_grad_color(data->page_buttons[i], lv_color_hex(0x002244), 0);
            lv_obj_set_style_shadow_width(data->page_buttons[i], 5, 0);
            lv_obj_set_style_shadow_color(data->page_buttons[i], lv_color_black(), 0);
            lv_obj_set_style_shadow_opa(data->page_buttons[i], LV_OPA_50, 0);
            lv_obj_set_style_shadow_ofs_y(data->page_buttons[i], 2, 0);
            FOOTER_LOG_DEBUG("    Button [%d] set to INACTIVE (dark blue)", i);
        }
        // Force redraw of this button
        lv_obj_invalidate(data->page_buttons[i]);
    }

    FOOTER_LOG_DEBUG("    Page changed to: [%s] (%d)", page_names[current_page], current_page);
}

/**
 * Show footer and start auto-hide timer
 */
void ui_footer_show(lv_obj_t *footer) {
    if (footer == NULL) return;

    ui_footer_data_t *data = (ui_footer_data_t *)lv_obj_get_user_data(footer);
    if (data == NULL) return;

    if (!data->is_visible) {
        lv_obj_clear_flag(footer, LV_OBJ_FLAG_HIDDEN);
        data->is_visible = true;
        FOOTER_LOG_EVENT("FOOTER ACTIVATED (shown)");
    }

    // Reset auto-hide timer
    ui_footer_reset_timer(footer);
}

/**
 * Hide footer immediately
 */
void ui_footer_hide(lv_obj_t *footer) {
    if (footer == NULL) return;

    ui_footer_data_t *data = (ui_footer_data_t *)lv_obj_get_user_data(footer);
    if (data == NULL) return;

    if (data->is_visible) {
        lv_obj_add_flag(footer, LV_OBJ_FLAG_HIDDEN);
        data->is_visible = false;

        // Pause auto-hide timer when hidden
        if (data->auto_hide_timer != NULL) {
            lv_timer_pause(data->auto_hide_timer);
            FOOTER_LOG_EVENT("FOOTER DEACTIVATED (hidden, timer paused)");
        }
    }
}

/**
 * Check if footer is currently visible
 */
bool ui_footer_is_visible(lv_obj_t *footer) {
    if (footer == NULL) return false;

    ui_footer_data_t *data = (ui_footer_data_t *)lv_obj_get_user_data(footer);
    if (data == NULL) return false;

    return data->is_visible;
}

/**
 * Reset auto-hide timer (call when user interacts with footer)
 */
void ui_footer_reset_timer(lv_obj_t *footer) {
    if (footer == NULL) return;

    ui_footer_data_t *data = (ui_footer_data_t *)lv_obj_get_user_data(footer);
    if (data == NULL || data->auto_hide_timer == NULL) {
        return;
    }

    // Reset timer countdown and resume if paused
    lv_timer_reset(data->auto_hide_timer);
    lv_timer_resume(data->auto_hide_timer);
    FOOTER_LOG_EVENT("Auto-hide timer RESET - footer will hide in %d ms (10 sec)", FOOTER_AUTO_HIDE_MS);
    FOOTER_LOG_DEBUG("    Timer reset and resumed");
}

/**
 * Clean up footer resources (call before deleting footer)
 */
void ui_footer_cleanup(lv_obj_t *footer) {
    if (footer == NULL) return;

    ui_footer_data_t *data = (ui_footer_data_t *)lv_obj_get_user_data(footer);
    if (data == NULL) return;

    // Delete auto-hide timer
    if (data->auto_hide_timer != NULL) {
        lv_timer_del(data->auto_hide_timer);
        data->auto_hide_timer = NULL;
        FOOTER_LOG_DEBUG("=== FOOTER CLEANUP: Timer deleted ===");
    }

    // Free footer data
    free(data);
    lv_obj_set_user_data(footer, NULL);

    FOOTER_LOG_DEBUG("=== FOOTER CLEANUP: Resources freed ===");
}
