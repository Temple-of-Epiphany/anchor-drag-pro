/*
 * OTA — firmware update from SD card.
 *
 * Flow at boot:
 *   1. ota_handle_pending_verify()    — if running from a NEW partition,
 *                                       run self-test then mark-valid;
 *                                       otherwise the bootloader rolls
 *                                       us back on next reboot.
 *   2. ota_check_and_apply_from_sd()  — scan /sdcard/firmware/ for a
 *                                       newer firmware bin + SHA256
 *                                       sidecar, prompt the user via
 *                                       serial, verify, flash, reboot.
 *
 * Filename convention:
 *   anchor-drag-pro_v<MAJOR>.<MINOR>.<PATCH>.bin
 *   anchor-drag-pro_v<MAJOR>.<MINOR>.<PATCH>.sha256
 *
 * SHA256 sidecar format: standard `sha256sum` output —
 *   <64 hex chars>  <filename>\n
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Handle "pending verify" state after a previous OTA installed new firmware.
 * Call once near the start of app_main. Behavior:
 *   - Running partition is in PENDING_VERIFY state → run self-test
 *     (currently a short heap-alive check + delay), then call
 *     esp_ota_mark_app_valid_cancel_rollback(). Logs the transition.
 *   - Running partition is already VALID or FACTORY → no-op.
 *   - Self-test fails → call esp_ota_mark_app_invalid_rollback_and_reboot()
 *     and the device reverts to the previous app.
 *
 * Returns ESP_OK on success or no-op. ESP_FAIL only if the self-test
 * detected a problem (in which case the function reboots the device
 * before returning — caller is unlikely to see a return value).
 */
esp_err_t ota_handle_pending_verify(void);

/*
 * Scan /sdcard/firmware/ for the highest-version anchor-drag-pro_v*.bin
 * with a matching .sha256 sidecar.
 *
 * If found AND version > running:
 *   1. Print release notes if .txt sidecar exists
 *   2. Prompt user "Update v0.2.0-dev to v0.3.0? (y/N, 30s timeout):"
 *   3. If user types 'y' within timeout:
 *        a. Verify SHA256 — refuse if mismatch
 *        b. esp_ota_write loop reading 4 KB chunks from SD
 *        c. Log percent progress every 5 percent
 *        d. esp_ota_set_boot_partition + esp_restart
 *      Function does not return on success — device reboots.
 *
 * Returns:
 *   ESP_OK                 — no update found OR user declined OR error
 *                            (caller continues normal boot)
 *   ESP_ERR_INVALID_STATE  — SD not mounted
 *   (does not return on successful flash; device reboots)
 *
 * Requires: sd_card_init() succeeded.
 */
esp_err_t ota_check_and_apply_from_sd(void);

/*
 * Diagnostic: print current OTA state, running partition, next OTA slot.
 */
void ota_log_status(void);


/* ============================================================
 * UI-driven OTA (touchscreen confirmation + progress)
 *
 * The boot path (ota_check_and_apply_from_sd) runs before LVGL is up
 * and auto-installs. These two functions let the Diagnostics screen
 * offer a manual "Check for update" flow once the UI is available:
 *   ota_scan_sd()  — non-blocking scan; reports whether a newer bin
 *                    exists and the from/to versions.
 *   ota_apply()    — verify SHA + flash a scanned update, reporting
 *                    progress through a callback. Reboots on success
 *                    (does not return); returns an error on failure.
 * ============================================================ */

typedef struct {
    bool available;                 /* true if new_* > running_* */
    int  running_major, running_minor, running_patch;
    int  new_major, new_minor, new_patch;
    char bin_path[256];
    char sha_path[256];
} ota_update_info_t;

/* Scan SD for the newest valid candidate. Fills *info. Returns:
 *   ESP_OK                — scan ran; check info->available
 *   ESP_ERR_INVALID_STATE — SD not mounted */
esp_err_t ota_scan_sd(ota_update_info_t *info);

/* Progress phases reported through ota_progress_cb. */
typedef enum {
    OTA_PHASE_VERIFY = 0,   /* computing/comparing SHA256 */
    OTA_PHASE_FLASH,        /* writing partition */
    OTA_PHASE_DONE,         /* about to reboot */
    OTA_PHASE_ERROR,        /* failed — see message */
} ota_phase_t;

/* pct is 0..100 within the current phase. msg is a short human string
 * (may be NULL). Called from the task that invoked ota_apply(). */
typedef void (*ota_progress_cb)(ota_phase_t phase, int pct,
                                 const char *msg, void *user);

/* Apply a scanned update: verify SHA, flash, set boot partition, reboot.
 * Does NOT return on success. Returns an esp_err_t on failure (after
 * reporting OTA_PHASE_ERROR through cb). Safe to call from a worker
 * task; do NOT call from the LVGL task (it blocks for the whole flash). */
esp_err_t ota_apply(const ota_update_info_t *info,
                     ota_progress_cb cb, void *user);

#ifdef __cplusplus
}
#endif
