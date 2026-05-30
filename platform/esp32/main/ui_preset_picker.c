/*
 * Modal: Preset Picker — implementation.
 *
 * Spec: https://github.com/Temple-of-Epiphany/anchor-drag-pro/wiki/Modal-Preset-Picker
 *
 * Three 140x120 tiles laid out horizontally. Active tile gets the
 * primary-action fill + a brighter border; inactive tiles use a
 * subtle surface fill. Locked tiles dim everything one notch.
 *
 * Tap a tile: fire callback (caller persists + decides whether to
 * close), unless locked, in which case the tile briefly highlights
 * and a transient toast appears.
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#include "ui_preset_picker.h"
#include "ui_tokens.h"
#include "lvgl_init.h"
#include "esp_log.h"
#include "lvgl.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static const char *TAG = "preset_picker";

typedef struct {
    ui_modal_handle_t        *modal;
    ui_preset_picker_params_t params;
    anchor_distance_t         distances_copy[3];   /* defensive copy so caller can free */
    lv_obj_t                 *tiles[3];
    lv_obj_t                 *tile_labels[3];      /* values, e.g. "50" */
    lv_obj_t                 *toast;               /* lazy-created on locked tap */
} picker_state_t;

/* ---- Helpers ---- */

static void close_and_free(picker_state_t *st)
{
    if (!st) return;
    if (st->modal) {
        ui_modal_close(st->modal);
        lv_obj_t *content = ui_modal_get_content(st->modal);
        if (content) {
            lv_obj_t *scrim = lv_obj_get_parent(content);
            if (scrim) lv_obj_del(scrim);
        }
        free(st->modal);
    }
    free(st);
}

static const char *unit_str(anchor_unit_t u)
{
    return (u == UNIT_M) ? "m" : "ft";
}

static void apply_tile_style(lv_obj_t *tile, bool active, bool locked)
{
    lv_color_t bg = active ? UI_COLOR(ACTION_PRIMARY) : UI_COLOR(SURFACE_ELEVATED);
    lv_color_t border = active ? UI_COLOR(ACTION_PRIMARY) : UI_COLOR(SURFACE_BORDER);
    lv_opa_t   opa = locked ? LV_OPA_50 : LV_OPA_COVER;

    lv_obj_set_style_bg_color    (tile, bg, LV_PART_MAIN);
    lv_obj_set_style_bg_opa      (tile, opa, LV_PART_MAIN);
    lv_obj_set_style_border_color(tile, border, LV_PART_MAIN);
    lv_obj_set_style_border_width(tile, active ? 2 : 1, LV_PART_MAIN);
}

/* ---- Event handlers ---- */

static void show_locked_toast(picker_state_t *st)
{
    if (!st->toast) {
        lv_obj_t *content = ui_modal_get_content(st->modal);
        st->toast = lv_label_create(content);
        lv_label_set_text(st->toast, "Locked — DISARM first");
        lv_obj_set_style_text_color(st->toast, UI_COLOR(STATE_WARNING), LV_PART_MAIN);
        lv_obj_set_style_text_font (st->toast, &lv_font_montserrat_14, LV_PART_MAIN);
        lv_obj_align(st->toast, LV_ALIGN_BOTTOM_MID, 0, -8);
    }
}

static void on_tile_clicked(lv_event_t *e)
{
    picker_state_t *st  = (picker_state_t *) lv_event_get_user_data(e);
    if (!st) return;
    int idx = -1;
    lv_obj_t *target = lv_event_get_target(e);
    for (int i = 0; i < 3; i++) {
        if (st->tiles[i] == target) { idx = i; break; }
    }
    if (idx < 0) return;

    if (st->params.locked) {
        ESP_LOGI(TAG, "tile %d tapped but locked", idx);
        show_locked_toast(st);
        return;
    }

    if (idx == st->params.current_idx) {
        ESP_LOGI(TAG, "tile %d already active — dismiss without write", idx);
        close_and_free(st);
        return;
    }

    ESP_LOGI(TAG, "tile %d selected (was %d)", idx, st->params.current_idx);
    ui_preset_picker_cb cb = st->params.on_selected;
    void *ud               = st->params.user_data;
    /* Brief visual update before dismiss — re-style the tiles to show
     * the new selection, then auto-dismiss. */
    for (int i = 0; i < 3; i++) {
        apply_tile_style(st->tiles[i], i == idx, false);
    }
    close_and_free(st);
    if (cb) cb(idx, ud);
}

static void on_scrim_clicked(lv_event_t *e)
{
    lv_obj_t *target  = lv_event_get_target(e);
    lv_obj_t *content = (lv_obj_t *) lv_event_get_user_data(e);
    if (target == content) return;
    picker_state_t *st = (picker_state_t *) lv_obj_get_user_data(content);
    ESP_LOGI(TAG, "dismissed via outside tap");
    close_and_free(st);
}

static void on_close_clicked(lv_event_t *e)
{
    picker_state_t *st = (picker_state_t *) lv_event_get_user_data(e);
    ESP_LOGI(TAG, "dismissed via Close");
    close_and_free(st);
}

/* ---- Public ---- */

ui_modal_handle_t *ui_preset_picker_show(const ui_preset_picker_params_t *params)
{
    if (!params || !params->distances) {
        ESP_LOGE(TAG, "distances required");
        return NULL;
    }

    if (!lvgl_lock(2000)) {
        ESP_LOGE(TAG, "could not acquire LVGL mutex");
        return NULL;
    }

    picker_state_t *st = calloc(1, sizeof(*st));
    if (!st) { lvgl_unlock(); return NULL; }
    st->params = *params;
    memcpy(st->distances_copy, params->distances, sizeof(st->distances_copy));
    st->params.distances = st->distances_copy;

    st->modal = ui_modal_create(520, 280, UI_MODAL_PRIO_USER);
    if (!st->modal) { free(st); lvgl_unlock(); return NULL; }

    lv_obj_t *content = ui_modal_get_content(st->modal);
    lv_obj_set_user_data(content, st);

    /* Title. */
    lv_obj_t *title = lv_label_create(content);
    lv_label_set_text(title, "Alarm distance");
    lv_obj_set_style_text_color(title, UI_COLOR(TEXT_BODY_STRONG), LV_PART_MAIN);
    lv_obj_set_style_text_font (title, &lv_font_montserrat_22, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);

    if (st->params.locked) {
        lv_obj_t *sub = lv_label_create(content);
        lv_label_set_text(sub, "Locked while anchor watch is active");
        lv_obj_set_style_text_color(sub, UI_COLOR(STATE_WARNING), LV_PART_MAIN);
        lv_obj_set_style_text_font (sub, &lv_font_montserrat_14, LV_PART_MAIN);
        lv_obj_align(sub, LV_ALIGN_TOP_MID, 0, 30);
    }

    /* Tile row. */
    lv_obj_t *tile_row = lv_obj_create(content);
    lv_obj_set_size(tile_row, 460, 140);
    lv_obj_align(tile_row, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa      (tile_row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(tile_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all     (tile_row, 0, LV_PART_MAIN);
    lv_obj_set_layout            (tile_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow         (tile_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align        (tile_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag            (tile_row, LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < 3; i++) {
        st->tiles[i] = lv_obj_create(tile_row);
        lv_obj_set_size(st->tiles[i], 140, 120);
        lv_obj_set_style_radius(st->tiles[i], 10, LV_PART_MAIN);
        lv_obj_clear_flag(st->tiles[i], LV_OBJ_FLAG_SCROLLABLE);
        apply_tile_style(st->tiles[i], i == st->params.current_idx, st->params.locked);
        lv_obj_add_flag(st->tiles[i], LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(st->tiles[i], on_tile_clicked, LV_EVENT_CLICKED, st);

        /* Value (e.g. "50") */
        char val_buf[8];
        snprintf(val_buf, sizeof(val_buf), "%.0f", st->distances_copy[i].value);
        st->tile_labels[i] = lv_label_create(st->tiles[i]);
        lv_label_set_text(st->tile_labels[i], val_buf);
        lv_obj_set_style_text_color(st->tile_labels[i], UI_COLOR(TEXT_BODY_STRONG), LV_PART_MAIN);
        lv_obj_set_style_text_font (st->tile_labels[i], &lv_font_montserrat_32, LV_PART_MAIN);
        lv_obj_align(st->tile_labels[i], LV_ALIGN_CENTER, 0, -10);

        /* Unit (e.g. "ft") */
        lv_obj_t *unit = lv_label_create(st->tiles[i]);
        lv_label_set_text(unit, unit_str(st->distances_copy[i].unit));
        lv_obj_set_style_text_color(unit, UI_COLOR(TEXT_BODY), LV_PART_MAIN);
        lv_obj_set_style_text_font (unit, &lv_font_montserrat_14, LV_PART_MAIN);
        lv_obj_align(unit, LV_ALIGN_CENTER, 0, 24);
    }

    /* Close button at the bottom. */
    lv_obj_t *close = lv_btn_create(content);
    lv_obj_set_size(close, 120, 36);
    lv_obj_align(close, LV_ALIGN_BOTTOM_MID, 0, 4);
    lv_obj_set_style_bg_color    (close, UI_COLOR(ACTION_SECONDARY), LV_PART_MAIN);
    lv_obj_set_style_radius      (close, 8, LV_PART_MAIN);
    lv_obj_set_style_border_width(close, 0, LV_PART_MAIN);
    lv_obj_t *close_lbl = lv_label_create(close);
    lv_label_set_text(close_lbl, "Close");
    lv_obj_set_style_text_color(close_lbl, UI_COLOR(TEXT_BODY_STRONG), LV_PART_MAIN);
    lv_obj_set_style_text_font (close_lbl, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_center(close_lbl);
    lv_obj_add_event_cb(close, on_close_clicked, LV_EVENT_CLICKED, st);

    /* Outside-tap dismissal. */
    lv_obj_t *scrim = lv_obj_get_parent(content);
    if (scrim) {
        lv_obj_add_flag(scrim, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(scrim, on_scrim_clicked, LV_EVENT_CLICKED, content);
    }

    ui_modal_open(st->modal);
    lvgl_unlock();

    ESP_LOGI(TAG, "shown (current_idx=%d, locked=%d)",
             st->params.current_idx, (int) st->params.locked);
    return st->modal;
}
