/*
 * Modal stack manager — implementation skeleton (milestone 3 of #69).
 *
 * Maintains a small fixed-size stack of currently-open modals,
 * enforcing the precedence rules from
 * https://github.com/Temple-of-Epiphany/anchor-drag-pro/wiki/Information-Architecture
 *
 * Implementation: each modal is a centred lv_obj_t over a tinted scrim
 * covering the whole screen. Opening a higher-priority modal closes
 * lower-priority ones first. v0.2 caps the stack at 4 modals (more
 * than enough — practically there are 1 or 2 ever open at once).
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#include "ui_modals.h"
#include "ui_tokens.h"
#include "esp_log.h"
#include "lvgl.h"
#include <stdlib.h>
#include <string.h>

static const char *TAG = "modal";

#define UI_MODAL_STACK_MAX 4

struct ui_modal_handle {
    lv_obj_t           *scrim;       /* full-screen tint behind the modal */
    lv_obj_t           *content;     /* the centred container the caller fills */
    ui_modal_priority_t priority;
    bool                open;
};

/* Active stack. Slot may be NULL. */
static ui_modal_handle_t *s_stack[UI_MODAL_STACK_MAX];

static void detach_from_stack(ui_modal_handle_t *m)
{
    for (int i = 0; i < UI_MODAL_STACK_MAX; i++) {
        if (s_stack[i] == m) { s_stack[i] = NULL; return; }
    }
}

static bool attach_to_stack(ui_modal_handle_t *m)
{
    for (int i = 0; i < UI_MODAL_STACK_MAX; i++) {
        if (s_stack[i] == NULL) { s_stack[i] = m; return true; }
    }
    return false;
}

ui_modal_handle_t *ui_modal_create(int width, int height,
                                    ui_modal_priority_t prio)
{
    ui_modal_handle_t *m = calloc(1, sizeof(*m));
    if (!m) return NULL;
    m->priority = prio;

    /* Full-screen scrim — invisible until ui_modal_open(). */
    m->scrim = lv_obj_create(lv_scr_act());
    lv_obj_set_size(m->scrim, LV_HOR_RES, LV_VER_RES);
    lv_obj_align(m->scrim, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color    (m->scrim, UI_COLOR(SURFACE_SCRIM), LV_PART_MAIN);
    lv_obj_set_style_bg_opa      (m->scrim, LV_OPA_60,               LV_PART_MAIN);
    lv_obj_set_style_border_width(m->scrim, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all     (m->scrim, 0, LV_PART_MAIN);
    lv_obj_clear_flag            (m->scrim, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag              (m->scrim, LV_OBJ_FLAG_HIDDEN);

    /* Content container — centred over the scrim. */
    m->content = lv_obj_create(m->scrim);
    lv_obj_set_size(m->content, width, height);
    lv_obj_center(m->content);
    lv_obj_set_style_bg_color    (m->content, UI_COLOR(SURFACE_MODAL),  LV_PART_MAIN);
    lv_obj_set_style_bg_opa      (m->content, LV_OPA_COVER,             LV_PART_MAIN);
    lv_obj_set_style_border_color(m->content, UI_COLOR(SURFACE_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(m->content, 1, LV_PART_MAIN);
    lv_obj_set_style_radius      (m->content, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_all     (m->content, 20, LV_PART_MAIN);
    lv_obj_clear_flag            (m->content, LV_OBJ_FLAG_SCROLLABLE);

    return m;
}

lv_obj_t *ui_modal_get_content(ui_modal_handle_t *m)
{
    return m ? m->content : NULL;
}

void ui_modal_open(ui_modal_handle_t *m)
{
    if (!m || m->open) return;

    /* Close anything strictly lower priority. */
    for (int i = 0; i < UI_MODAL_STACK_MAX; i++) {
        ui_modal_handle_t *other = s_stack[i];
        if (other && other->priority < m->priority) {
            ui_modal_close(other);
        }
    }

    if (!attach_to_stack(m)) {
        ESP_LOGW(TAG, "modal stack full; refusing to open prio=%d", m->priority);
        return;
    }

    lv_obj_clear_flag(m->scrim, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(m->scrim);
    m->open = true;
    ESP_LOGI(TAG, "modal opened (prio=%d)", m->priority);
}

void ui_modal_close(ui_modal_handle_t *m)
{
    if (!m) return;
    if (m->open) {
        lv_obj_add_flag(m->scrim, LV_OBJ_FLAG_HIDDEN);
        m->open = false;
        detach_from_stack(m);
        ESP_LOGI(TAG, "modal closed (prio=%d)", m->priority);
    }
    /* Note: we do NOT free m here — the caller may reopen. Use
     * lv_obj_del(m->scrim) + free(m) when the modal is truly disposed. */
}

bool ui_modal_is_open(ui_modal_handle_t *m)
{
    return m && m->open;
}

bool ui_modal_any_at_or_above(ui_modal_priority_t prio)
{
    for (int i = 0; i < UI_MODAL_STACK_MAX; i++) {
        if (s_stack[i] && s_stack[i]->open && s_stack[i]->priority >= prio) {
            return true;
        }
    }
    return false;
}
