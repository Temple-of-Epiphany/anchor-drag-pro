/**
 * Date/Time Settings Screen Implementation
 *
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2025-12-25
 * Version: 0.1.0
 */

#include "datetime_settings.h"
#include "ui_header.h"
#include "board_config.h"
#include "rtc_pcf85063a.h"
#include "fonts/custom_fonts.h"
#include "esp_log.h"

static const char *TAG = "datetime_settings";

// UI element references
static lv_obj_t *year_roller;
static lv_obj_t *month_roller;
static lv_obj_t *day_roller;
static lv_obj_t *hour_roller;
static lv_obj_t *minute_roller;
static lv_obj_t *second_roller;
static lv_obj_t *timezone_roller;
static lv_obj_t *current_time_label;

// Timezone offset in hours (-12 to +14)
static int8_t timezone_offset = 0;

// GPS sync checkbox state
static lv_obj_t *gps_sync_checkbox;
static bool gps_sync_enabled = false;

/**
 * Update current time display
 * Shows local time (UTC + timezone offset)
 */
static void update_current_time_display(lv_timer_t *timer) {
    datetime_t utc_time;
    PCF85063A_Read_now(&utc_time);  // RTC always stores UTC

    // Calculate local time by applying timezone offset
    int local_hour = utc_time.hour + timezone_offset;
    int local_day = utc_time.day;
    int local_month = utc_time.month;
    int local_year = utc_time.year;

    // Handle day/month/year rollover
    if (local_hour < 0) {
        local_hour += 24;
        local_day--;
        if (local_day < 1) {
            local_month--;
            if (local_month < 1) {
                local_month = 12;
                local_year--;
            }
            // Simplified: assume 30 days per month for display
            local_day = 30;
        }
    } else if (local_hour >= 24) {
        local_hour -= 24;
        local_day++;
        // Simplified: assume 30 days per month for display
        if (local_day > 30) {
            local_day = 1;
            local_month++;
            if (local_month > 12) {
                local_month = 1;
                local_year++;
            }
        }
    }

    char time_str[64];
    snprintf(time_str, sizeof(time_str),
        "Local: %04d-%02d-%02d %02d:%02d:%02d (UTC%+d)",
        local_year, local_month, local_day,
        local_hour, utc_time.min, utc_time.sec,
        timezone_offset);
    lv_label_set_text(current_time_label, time_str);

    // TODO: Check GPS sync if enabled and sync if drift > 5 seconds
    // This will be implemented when GPS module is integrated
}

/**
 * GPS Time Sync checkbox callback
 */
static void gps_sync_changed(lv_event_t *e) {
    lv_obj_t *cb = lv_event_get_target(e);
    gps_sync_enabled = lv_obj_get_state(cb) & LV_STATE_CHECKED;

    ESP_LOGI(TAG, "GPS Time Sync %s", gps_sync_enabled ? "enabled" : "disabled");

    if (gps_sync_enabled) {
        // TODO: Start GPS time sync monitoring (check every update if drift > 5 seconds)
        lv_obj_t *mbox = lv_msgbox_create(lv_scr_act(), "GPS Sync",
            "GPS time sync enabled.\nWill auto-sync when drift > 5 seconds.\n(GPS integration pending)",
            NULL, true);
        lv_obj_center(mbox);
    }
}

/**
 * Timer callback to auto-close msgbox after 5 seconds
 */
static void msgbox_auto_close_timer(lv_timer_t *timer) {
    lv_obj_t *mbox = (lv_obj_t *)timer->user_data;
    if (mbox != NULL) {
        lv_obj_del(mbox);
    }
}

/**
 * Save button callback - apply changes to RTC (stores UTC time)
 */
static void save_clicked(lv_event_t *e) {
    ESP_LOGI(TAG, "Save button clicked - applying date/time changes");

    // Get selected values from rollers (LOCAL time)
    uint16_t local_year = 2000 + lv_roller_get_selected(year_roller);
    uint8_t local_month = 1 + lv_roller_get_selected(month_roller);
    uint8_t local_day = 1 + lv_roller_get_selected(day_roller);
    uint8_t local_hour = lv_roller_get_selected(hour_roller);
    uint8_t local_minute = lv_roller_get_selected(minute_roller);
    uint8_t local_second = lv_roller_get_selected(second_roller);

    // Get timezone offset
    timezone_offset = lv_roller_get_selected(timezone_roller) - 12;  // -12 to +14

    // Convert LOCAL time to UTC by subtracting timezone offset
    int utc_hour = local_hour - timezone_offset;
    int utc_day = local_day;
    int utc_month = local_month;
    int utc_year = local_year;

    // Handle day/month/year rollover
    if (utc_hour < 0) {
        utc_hour += 24;
        utc_day--;
        if (utc_day < 1) {
            utc_month--;
            if (utc_month < 1) {
                utc_month = 12;
                utc_year--;
            }
            utc_day = 30;  // Simplified
        }
    } else if (utc_hour >= 24) {
        utc_hour -= 24;
        utc_day++;
        if (utc_day > 30) {  // Simplified
            utc_day = 1;
            utc_month++;
            if (utc_month > 12) {
                utc_month = 1;
                utc_year++;
            }
        }
    }

    ESP_LOGI(TAG, "Local: %04d-%02d-%02d %02d:%02d:%02d (UTC%+d)",
             local_year, local_month, local_day, local_hour, local_minute, local_second, timezone_offset);
    ESP_LOGI(TAG, "Setting RTC (UTC): %04d-%02d-%02d %02d:%02d:%02d",
             utc_year, utc_month, utc_day, utc_hour, local_minute, local_second);

    // Create datetime structure with UTC time
    datetime_t utc_time = {
        .year = utc_year,
        .month = utc_month,
        .day = utc_day,
        .hour = utc_hour,
        .min = local_minute,
        .sec = local_second,
        .dotw = 0  // Day of week - will be calculated by RTC
    };

    // Set RTC time (always stores UTC)
    PCF85063A_Set_Time(utc_time);

    // Save timezone offset to NVS
    // TODO: Add NVS storage for timezone

    ESP_LOGI(TAG, "RTC time updated successfully (stored as UTC)");

    // Show confirmation message
    lv_obj_t *mbox = lv_msgbox_create(lv_scr_act(), "Success",
        "Date/Time updated!\nRTC stores UTC time.", NULL, true);
    lv_obj_center(mbox);

    // Auto-close msgbox after 5 seconds (keep X button for manual close)
    lv_timer_create(msgbox_auto_close_timer, 5000, mbox);
}

/**
 * Back/Cancel button callback - return to TOOLS screen
 */
static void cancel_clicked(lv_event_t *e) {
    ESP_LOGI(TAG, "Back button clicked");
    // Get tools screen from user data
    lv_obj_t *tools_screen = (lv_obj_t *)lv_event_get_user_data(e);
    if (tools_screen != NULL) {
        lv_scr_load(tools_screen);
    }
}

/**
 * Create date/time settings screen
 */
lv_obj_t* create_datetime_settings_screen(lv_obj_t *tools_screen_ref, ui_footer_page_cb_t page_callback, lv_obj_t **footer_out) {
    ESP_LOGI(TAG, "Creating date/time settings screen");

    // Create screen
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x1A1A2E), 0);

    // Create header
    lv_obj_t *header = ui_header_create(screen);
    ui_header_set_gps_status(header, false);

    // Title
    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "DATE/TIME SETTINGS");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 90);

    // Current time display
    current_time_label = lv_label_create(screen);
    lv_label_set_text(current_time_label, "Current: --");
    lv_obj_set_style_text_color(current_time_label, lv_color_hex(0x39CCCC), 0);
    lv_obj_set_style_text_font(current_time_label, &lv_font_montserrat_14, 0);
    lv_obj_align(current_time_label, LV_ALIGN_TOP_MID, 0, 115);

    // Create timer to update current time every second
    lv_timer_create(update_current_time_display, 1000, NULL);
    update_current_time_display(NULL);  // Initial update

    // Date section label
    lv_obj_t *date_label = lv_label_create(screen);
    lv_label_set_text(date_label, "SET DATE:");
    lv_obj_set_style_text_color(date_label, lv_color_white(), 0);
    lv_obj_align(date_label, LV_ALIGN_TOP_LEFT, 40, 150);

    // Year roller (2025-2050)
    year_roller = lv_roller_create(screen);
    lv_roller_set_options(year_roller,
        "2025\n2026\n2027\n2028\n2029\n2030\n2031\n2032\n2033\n2034\n2035\n"
        "2036\n2037\n2038\n2039\n2040\n2041\n2042\n2043\n2044\n2045\n2046\n"
        "2047\n2048\n2049\n2050", LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(year_roller, 3);
    lv_obj_set_width(year_roller, 100);
    lv_obj_align(year_roller, LV_ALIGN_TOP_LEFT, 40, 175);

    // Month roller (1-12)
    month_roller = lv_roller_create(screen);
    lv_roller_set_options(month_roller,
        "01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n12",
        LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(month_roller, 3);
    lv_obj_set_width(month_roller, 80);
    lv_obj_align(month_roller, LV_ALIGN_TOP_LEFT, 150, 175);

    // Day roller (1-31)
    day_roller = lv_roller_create(screen);
    lv_roller_set_options(day_roller,
        "01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n12\n13\n14\n15\n"
        "16\n17\n18\n19\n20\n21\n22\n23\n24\n25\n26\n27\n28\n29\n30\n31",
        LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(day_roller, 3);
    lv_obj_set_width(day_roller, 80);
    lv_obj_align(day_roller, LV_ALIGN_TOP_LEFT, 240, 175);

    // Time section label
    lv_obj_t *time_label = lv_label_create(screen);
    lv_label_set_text(time_label, "SET TIME:");
    lv_obj_set_style_text_color(time_label, lv_color_white(), 0);
    lv_obj_align(time_label, LV_ALIGN_TOP_LEFT, 40, 260);

    // Hour roller (0-23)
    hour_roller = lv_roller_create(screen);
    lv_roller_set_options(hour_roller,
        "00\n01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n12\n13\n14\n15\n"
        "16\n17\n18\n19\n20\n21\n22\n23", LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(hour_roller, 3);
    lv_obj_set_width(hour_roller, 80);
    lv_obj_align(hour_roller, LV_ALIGN_TOP_LEFT, 40, 285);

    // Minute roller (0-59)
    minute_roller = lv_roller_create(screen);
    char min_options[400];
    strcpy(min_options, "00");
    for (int i = 1; i < 60; i++) {
        char buf[8];
        snprintf(buf, sizeof(buf), "\n%02d", i);
        strcat(min_options, buf);
    }
    lv_roller_set_options(minute_roller, min_options, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(minute_roller, 3);
    lv_obj_set_width(minute_roller, 80);
    lv_obj_align(minute_roller, LV_ALIGN_TOP_LEFT, 130, 285);

    // Second roller (0-59)
    second_roller = lv_roller_create(screen);
    lv_roller_set_options(second_roller, min_options, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(second_roller, 3);
    lv_obj_set_width(second_roller, 80);
    lv_obj_align(second_roller, LV_ALIGN_TOP_LEFT, 220, 285);

    // Timezone section
    lv_obj_t *tz_label = lv_label_create(screen);
    lv_label_set_text(tz_label, "TIMEZONE:");
    lv_obj_set_style_text_color(tz_label, lv_color_white(), 0);
    lv_obj_align(tz_label, LV_ALIGN_TOP_RIGHT, -200, 150);

    // Timezone roller (UTC-12 to UTC+14) - minimal format
    timezone_roller = lv_roller_create(screen);
    lv_roller_set_options(timezone_roller,
        "-12\n-11\n-10\n-9\n-8\n-7\n-6\n-5\n"
        "-4\n-3\n-2\n-1\n+0\n+1\n+2\n+3\n"
        "+4\n+5\n+6\n+7\n+8\n+9\n+10\n+11\n"
        "+12\n+13\n+14", LV_ROLLER_MODE_NORMAL);
    lv_roller_set_selected(timezone_roller, 12, LV_ANIM_OFF);  // Default UTC+0
    lv_roller_set_visible_row_count(timezone_roller, 5);
    lv_obj_set_width(timezone_roller, 100);
    lv_obj_align(timezone_roller, LV_ALIGN_TOP_RIGHT, -180, 175);

    // GPS Sync checkbox - positioned on left side, larger for better visibility
    gps_sync_checkbox = lv_checkbox_create(screen);
    lv_checkbox_set_text(gps_sync_checkbox, "GPS Time Sync");
    lv_obj_set_style_text_color(gps_sync_checkbox, lv_color_white(), 0);
    lv_obj_set_style_text_font(gps_sync_checkbox, &orbitron_variablefont_wght_20, 0);  // Larger font
    lv_obj_align(gps_sync_checkbox, LV_ALIGN_TOP_LEFT, 50, 340);  // Moved to left side
    lv_obj_add_event_cb(gps_sync_checkbox, gps_sync_changed, LV_EVENT_VALUE_CHANGED, NULL);

    // Save button - positioned lower to avoid covering widgets
    lv_obj_t *save_btn = lv_btn_create(screen);
    lv_obj_set_size(save_btn, 150, 50);
    lv_obj_align(save_btn, LV_ALIGN_BOTTOM_LEFT, 100, -20);
    lv_obj_set_style_bg_color(save_btn, lv_color_hex(0x00CC66), 0);
    lv_obj_add_event_cb(save_btn, save_clicked, LV_EVENT_CLICKED, NULL);

    lv_obj_t *save_label = lv_label_create(save_btn);
    lv_label_set_text(save_label, "SAVE");
    lv_obj_set_style_text_color(save_label, lv_color_white(), 0);
    lv_obj_center(save_label);

    // Back/Cancel button - positioned lower to avoid covering widgets
    lv_obj_t *cancel_btn = lv_btn_create(screen);
    lv_obj_set_size(cancel_btn, 150, 50);
    lv_obj_align(cancel_btn, LV_ALIGN_BOTTOM_RIGHT, -100, -20);
    lv_obj_set_style_bg_color(cancel_btn, lv_color_hex(0xCC3333), 0);
    lv_obj_add_event_cb(cancel_btn, cancel_clicked, LV_EVENT_CLICKED, tools_screen_ref);

    lv_obj_t *cancel_label = lv_label_create(cancel_btn);
    lv_label_set_text(cancel_label, "BACK");
    lv_obj_set_style_text_color(cancel_label, lv_color_white(), 0);
    lv_obj_center(cancel_label);

    // Create footer (hidden by default for more space)
    lv_obj_t *footer = ui_footer_create(screen, PAGE_TOOLS, page_callback);
    if (footer != NULL) {
        ui_footer_hide(footer);  // Hide footer for more screen space
    }
    if (footer_out != NULL) {
        *footer_out = footer;
    }

    // Initialize rollers to current RTC time
    datetime_t current;
    PCF85063A_Read_now(&current);
    lv_roller_set_selected(year_roller, current.year - 2025, LV_ANIM_OFF);
    lv_roller_set_selected(month_roller, current.month - 1, LV_ANIM_OFF);
    lv_roller_set_selected(day_roller, current.day - 1, LV_ANIM_OFF);
    lv_roller_set_selected(hour_roller, current.hour, LV_ANIM_OFF);
    lv_roller_set_selected(minute_roller, current.min, LV_ANIM_OFF);
    lv_roller_set_selected(second_roller, current.sec, LV_ANIM_OFF);

    ESP_LOGI(TAG, "Date/Time settings screen created");
    return screen;
}
