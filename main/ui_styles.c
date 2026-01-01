/**
 * UI Style Manager - Implementation
 *
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2026-01-01
 * Version: 0.1.0
 */

#include "ui_styles.h"
#include "ui_theme.h"
#include "esp_log.h"
#include <stdlib.h>

static const char *TAG = "ui_styles";

/**
 * Private structure - all style objects contained here
 */
struct ui_styles_t {
    // Screen styles
    lv_style_t screen_bg;

    // Text styles
    lv_style_t title;
    lv_style_t subtitle;
    lv_style_t body_text;

    // Button styles
    lv_style_t button_3d;              // Base 3D effect (shadow, radius)
    lv_style_t button_3d_pressed;      // Pressed state
    lv_style_t button_green;           // Color variants
    lv_style_t button_red;
    lv_style_t button_yellow;
    lv_style_t button_blue;
    lv_style_t button_gray;

    // Panel/container styles
    lv_style_t panel;
    lv_style_t panel_border;
};

/**
 * Create and initialize style manager
 */
ui_styles_t* ui_styles_create(void) {
    ESP_LOGI(TAG, "Creating style manager...");

    // Allocate structure
    ui_styles_t* styles = (ui_styles_t*)malloc(sizeof(ui_styles_t));
    if (styles == NULL) {
        ESP_LOGE(TAG, "Failed to allocate style manager");
        return NULL;
    }

    // Initialize screen background style
    lv_style_init(&styles->screen_bg);
    lv_style_set_bg_color(&styles->screen_bg, lv_color_hex(THEME_SCREEN_BG));
    lv_style_set_bg_opa(&styles->screen_bg, LV_OPA_COVER);

    // Initialize title text style
    lv_style_init(&styles->title);
    lv_style_set_text_color(&styles->title, lv_color_hex(THEME_TITLE_COLOR));
    lv_style_set_text_font(&styles->title, FONT_TITLE);
    lv_style_set_text_align(&styles->title, LV_TEXT_ALIGN_CENTER);

    // Initialize subtitle text style
    lv_style_init(&styles->subtitle);
    lv_style_set_text_color(&styles->subtitle, lv_color_hex(COLOR_TEXT_SECONDARY));
    lv_style_set_text_font(&styles->subtitle, FONT_SUBTITLE);
    lv_style_set_text_align(&styles->subtitle, LV_TEXT_ALIGN_CENTER);

    // Initialize body text style
    lv_style_init(&styles->body_text);
    lv_style_set_text_color(&styles->body_text, lv_color_hex(COLOR_TEXT_PRIMARY));
    lv_style_set_text_font(&styles->body_text, FONT_BODY_NORMAL);

    // Initialize 3D button base style (shadow effects, rounded corners)
    lv_style_init(&styles->button_3d);
    lv_style_set_shadow_width(&styles->button_3d, 10);
    lv_style_set_shadow_color(&styles->button_3d, lv_color_hex(0x000000));
    lv_style_set_shadow_opa(&styles->button_3d, LV_OPA_50);
    lv_style_set_shadow_ofs_x(&styles->button_3d, 0);
    lv_style_set_shadow_ofs_y(&styles->button_3d, 5);
    lv_style_set_radius(&styles->button_3d, 8);
    lv_style_set_pad_all(&styles->button_3d, 12);

    // Initialize 3D button pressed state (shadow reduced for "pressed" effect)
    lv_style_init(&styles->button_3d_pressed);
    lv_style_set_shadow_width(&styles->button_3d_pressed, 5);
    lv_style_set_shadow_ofs_y(&styles->button_3d_pressed, 2);
    lv_style_set_translate_y(&styles->button_3d_pressed, 2);

    // Initialize button color variants
    lv_style_init(&styles->button_green);
    lv_style_set_bg_color(&styles->button_green, lv_color_hex(0x00AA00));
    lv_style_set_bg_grad_color(&styles->button_green, lv_color_hex(0x008A00));
    lv_style_set_bg_grad_dir(&styles->button_green, LV_GRAD_DIR_VER);

    lv_style_init(&styles->button_red);
    lv_style_set_bg_color(&styles->button_red, lv_color_hex(COLOR_DANGER));
    lv_style_set_bg_grad_color(&styles->button_red, lv_color_hex(0xDF2116));
    lv_style_set_bg_grad_dir(&styles->button_red, LV_GRAD_DIR_VER);

    lv_style_init(&styles->button_yellow);
    lv_style_set_bg_color(&styles->button_yellow, lv_color_hex(COLOR_WARNING));
    lv_style_set_bg_grad_color(&styles->button_yellow, lv_color_hex(0xDFBC00));
    lv_style_set_bg_grad_dir(&styles->button_yellow, LV_GRAD_DIR_VER);

    lv_style_init(&styles->button_blue);
    lv_style_set_bg_color(&styles->button_blue, lv_color_hex(COLOR_PRIMARY));
    lv_style_set_bg_grad_color(&styles->button_blue, lv_color_hex(0x0054B9));
    lv_style_set_bg_grad_dir(&styles->button_blue, LV_GRAD_DIR_VER);

    lv_style_init(&styles->button_gray);
    lv_style_set_bg_color(&styles->button_gray, lv_color_hex(0x808080));
    lv_style_set_bg_grad_color(&styles->button_gray, lv_color_hex(0x606060));
    lv_style_set_bg_grad_dir(&styles->button_gray, LV_GRAD_DIR_VER);

    // Initialize panel style
    lv_style_init(&styles->panel);
    lv_style_set_bg_color(&styles->panel, lv_color_hex(0x003366));
    lv_style_set_bg_opa(&styles->panel, LV_OPA_COVER);
    lv_style_set_radius(&styles->panel, 8);
    lv_style_set_pad_all(&styles->panel, 10);

    // Initialize panel border style
    lv_style_init(&styles->panel_border);
    lv_style_set_border_width(&styles->panel_border, 2);
    lv_style_set_border_color(&styles->panel_border, lv_color_hex(0x39CCCC));
    lv_style_set_border_opa(&styles->panel_border, LV_OPA_50);

    ESP_LOGI(TAG, "Style manager created successfully");
    return styles;
}

/**
 * Destroy style manager
 */
void ui_styles_destroy(ui_styles_t* styles) {
    if (styles != NULL) {
        ESP_LOGI(TAG, "Destroying style manager");
        // LVGL styles don't require explicit cleanup
        // They're just data structures, no dynamic allocations inside
        free(styles);
    }
}

/**
 * Getter functions - return pointers to internal style objects
 */

lv_style_t* ui_styles_get_screen_bg(ui_styles_t* styles) {
    return styles ? &styles->screen_bg : NULL;
}

lv_style_t* ui_styles_get_title(ui_styles_t* styles) {
    return styles ? &styles->title : NULL;
}

lv_style_t* ui_styles_get_subtitle(ui_styles_t* styles) {
    return styles ? &styles->subtitle : NULL;
}

lv_style_t* ui_styles_get_body_text(ui_styles_t* styles) {
    return styles ? &styles->body_text : NULL;
}

lv_style_t* ui_styles_get_button_3d(ui_styles_t* styles) {
    return styles ? &styles->button_3d : NULL;
}

lv_style_t* ui_styles_get_button_3d_pressed(ui_styles_t* styles) {
    return styles ? &styles->button_3d_pressed : NULL;
}

lv_style_t* ui_styles_get_button_green(ui_styles_t* styles) {
    return styles ? &styles->button_green : NULL;
}

lv_style_t* ui_styles_get_button_red(ui_styles_t* styles) {
    return styles ? &styles->button_red : NULL;
}

lv_style_t* ui_styles_get_button_yellow(ui_styles_t* styles) {
    return styles ? &styles->button_yellow : NULL;
}

lv_style_t* ui_styles_get_button_blue(ui_styles_t* styles) {
    return styles ? &styles->button_blue : NULL;
}

lv_style_t* ui_styles_get_button_gray(ui_styles_t* styles) {
    return styles ? &styles->button_gray : NULL;
}

lv_style_t* ui_styles_get_panel(ui_styles_t* styles) {
    return styles ? &styles->panel : NULL;
}

lv_style_t* ui_styles_get_panel_border(ui_styles_t* styles) {
    return styles ? &styles->panel_border : NULL;
}

/**
 * Helper: Apply 3D button style with color variant
 */
void ui_styles_apply_button(ui_styles_t* styles, lv_obj_t* btn, lv_style_t* color_style) {
    if (styles == NULL || btn == NULL || color_style == NULL) {
        return;
    }

    // Apply base 3D effect
    lv_obj_add_style(btn, &styles->button_3d, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Apply pressed state
    lv_obj_add_style(btn, &styles->button_3d_pressed, LV_PART_MAIN | LV_STATE_PRESSED);

    // Apply color variant
    lv_obj_add_style(btn, color_style, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Enable event bubbling for gestures
    lv_obj_add_flag(btn, LV_OBJ_FLAG_EVENT_BUBBLE);
}
