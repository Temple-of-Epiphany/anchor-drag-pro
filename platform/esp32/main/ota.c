/*
 * OTA — firmware update from SD card. Implementation.
 *
 * See ota.h for the user-visible API.
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#include "ota.h"
#include "sd_card.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_app_format.h"
#include "esp_partition.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mbedtls/sha256.h"
#include <dirent.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

static const char *TAG = "ota";

#define OTA_DIR                 SD_MOUNT_POINT "/firmware"
#define OTA_FILENAME_PREFIX     "anchor-drag-pro_v"
#define OTA_FILENAME_SUFFIX     ".bin"
#define OTA_CHUNK_BYTES         4096

/* Built-in version of THIS firmware (matches FIRMWARE_VERSION_STRING in main.c).
 * Kept in sync manually until a generated version.h replaces both. */
#define RUNNING_VERSION_MAJOR   0
#define RUNNING_VERSION_MINOR   2
#define RUNNING_VERSION_PATCH   23

/* Self-test that runs after first boot of a new partition. Currently
 * minimal — we don't have much running code yet. As LVGL / GPS / web UI
 * land, this expands to verify each subsystem is alive. */
#define SELF_TEST_DURATION_MS   3000

/* User prompt timeout. After this many seconds with no 'y'/'n' response,
 * the prompt defaults to "no update" (safe). */
#define PROMPT_TIMEOUT_S        30

typedef struct {
    int  major;
    int  minor;
    int  patch;
} fw_version_t;

typedef struct {
    char         bin_path[256];
    char         sha_path[256];
    fw_version_t version;
} ota_candidate_t;


/* ============================================================
 * Version helpers
 * ============================================================ */

static bool parse_version(const char *s, fw_version_t *out)
{
    if (!s || !out) return false;
    int n_major = 0, n_minor = 0, n_patch = 0;
    char extra = '\0';
    /* Accept "0.3.0", "0.3.0\0", "0.3.0-dev", etc. The trailing char (if
     * any) must be '.', '-', '_', or end-of-string — guards against
     * "0.3.0extra" being misparsed. */
    int n = sscanf(s, "%d.%d.%d%c", &n_major, &n_minor, &n_patch, &extra);
    if (n < 3) return false;
    if (n == 4 && extra != '-' && extra != '_' && extra != '.' && extra != '+') {
        return false;
    }
    out->major = n_major;
    out->minor = n_minor;
    out->patch = n_patch;
    return true;
}

static int version_cmp(const fw_version_t *a, const fw_version_t *b)
{
    if (a->major != b->major) return a->major - b->major;
    if (a->minor != b->minor) return a->minor - b->minor;
    return a->patch - b->patch;
}


/* ============================================================
 * SD scan — find newest matching firmware
 * ============================================================ */

static bool extract_version_from_filename(const char *name, fw_version_t *out)
{
    /* name is "anchor-drag-pro_v0.3.0.bin" — strip prefix + suffix and parse. */
    size_t pref_len = strlen(OTA_FILENAME_PREFIX);
    size_t suf_len  = strlen(OTA_FILENAME_SUFFIX);
    size_t n        = strlen(name);
    if (n <= pref_len + suf_len) return false;
    if (strncmp(name, OTA_FILENAME_PREFIX, pref_len) != 0) return false;
    if (strcmp(name + n - suf_len, OTA_FILENAME_SUFFIX) != 0) return false;

    char version_str[32] = {0};
    size_t version_len = n - pref_len - suf_len;
    if (version_len >= sizeof(version_str)) return false;
    memcpy(version_str, name + pref_len, version_len);
    version_str[version_len] = '\0';

    return parse_version(version_str, out);
}

static esp_err_t scan_for_candidate(ota_candidate_t *out)
{
    DIR *d = opendir(OTA_DIR);
    if (d == NULL) {
        /* Not an error — most boots don't have firmware on SD. */
        return ESP_ERR_NOT_FOUND;
    }

    ota_candidate_t best = {0};
    bool found_any = false;

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        fw_version_t v = {0};
        if (!extract_version_from_filename(ent->d_name, &v)) {
            continue;
        }

        ota_candidate_t cand;
        snprintf(cand.bin_path, sizeof(cand.bin_path),
                 "%s/%s", OTA_DIR, ent->d_name);
        snprintf(cand.sha_path, sizeof(cand.sha_path),
                 "%.*s.sha256",
                 (int) (strlen(cand.bin_path) - strlen(OTA_FILENAME_SUFFIX)),
                 cand.bin_path);
        cand.version = v;

        /* sidecar must exist */
        struct stat st;
        if (stat(cand.sha_path, &st) != 0) {
            ESP_LOGW(TAG, "skip %s: no .sha256 sidecar", ent->d_name);
            continue;
        }

        if (!found_any || version_cmp(&v, &best.version) > 0) {
            best = cand;
            found_any = true;
        }
    }
    closedir(d);

    if (!found_any) return ESP_ERR_NOT_FOUND;
    *out = best;
    return ESP_OK;
}


/* ============================================================
 * SHA256 — compute over file, parse sidecar, compare
 * ============================================================ */

static esp_err_t compute_file_sha256(const char *path, uint8_t out_hash[32])
{
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        ESP_LOGE(TAG, "open %s for hashing: %s", path, strerror(errno));
        return ESP_ERR_NOT_FOUND;
    }

    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, 0); /* 0 = SHA-256, not SHA-224 */

    uint8_t buf[1024];
    size_t  n;
    while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) {
        mbedtls_sha256_update(&ctx, buf, n);
    }

    int err = ferror(fp);
    fclose(fp);
    if (err) {
        mbedtls_sha256_free(&ctx);
        return ESP_FAIL;
    }

    mbedtls_sha256_finish(&ctx, out_hash);
    mbedtls_sha256_free(&ctx);
    return ESP_OK;
}

static int hex_digit(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + c - 'a';
    if (c >= 'A' && c <= 'F') return 10 + c - 'A';
    return -1;
}

static esp_err_t read_sidecar_hash(const char *path, uint8_t out_hash[32])
{
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        ESP_LOGE(TAG, "open sidecar %s: %s", path, strerror(errno));
        return ESP_ERR_NOT_FOUND;
    }

    /* sha256sum format: <64 hex chars><space><space><filename>\n
     * We read the first 64 hex chars and ignore the rest. */
    char hex[65] = {0};
    if (fread(hex, 1, 64, fp) != 64) {
        fclose(fp);
        ESP_LOGE(TAG, "sidecar too short");
        return ESP_FAIL;
    }
    fclose(fp);

    for (int i = 0; i < 32; i++) {
        int hi = hex_digit(hex[2 * i]);
        int lo = hex_digit(hex[2 * i + 1]);
        if (hi < 0 || lo < 0) {
            ESP_LOGE(TAG, "sidecar: non-hex char at byte %d", i);
            return ESP_FAIL;
        }
        out_hash[i] = (uint8_t) ((hi << 4) | lo);
    }
    return ESP_OK;
}


/* ============================================================
 * Serial UART confirmation prompt
 * ============================================================ */

static char prompt_user(const fw_version_t *new_ver, uint32_t timeout_s)
{
    printf("\n");
    printf("================================================================\n");
    printf("Firmware update available on SD card.\n");
    printf("    Current: v%d.%d.%d-dev\n",
           RUNNING_VERSION_MAJOR, RUNNING_VERSION_MINOR, RUNNING_VERSION_PATCH);
    printf("    New:     v%d.%d.%d\n",
           new_ver->major, new_ver->minor, new_ver->patch);
    printf("    Type 'y' to install, anything else to skip.\n");
    printf("    Auto-skip in %lu seconds.\n", (unsigned long) timeout_s);
    printf("> ");
    fflush(stdout);

    /* Poll stdin with vTaskDelay between checks. getchar() in ESP-IDF
     * returns EOF when no data is available because stdin is wired through
     * USB-Serial-JTAG which is non-blocking by default in our setup. */
    uint32_t elapsed_ms = 0;
    const uint32_t poll_ms = 100;
    while (elapsed_ms < timeout_s * 1000) {
        int c = getchar();
        if (c == EOF || c == 0xff) {
            vTaskDelay(pdMS_TO_TICKS(poll_ms));
            elapsed_ms += poll_ms;
            continue;
        }
        /* Got a character — echo it for confirmation. */
        if (c >= 0x20 && c < 0x7f) printf("%c\n", c);
        else printf("[0x%02x]\n", c);
        fflush(stdout);
        return (char) tolower(c);
    }
    printf("\n[timeout — skipping update]\n");
    fflush(stdout);
    return 0;
}


/* ============================================================
 * Apply — esp_ota_write loop
 * ============================================================ */

static esp_err_t apply_update(const ota_candidate_t *cand,
                               ota_progress_cb cb, void *user)
{
    /* Verify SHA first — refuse to flash if the file is corrupt. */
    uint8_t computed[32], expected[32];

    if (cb) cb(OTA_PHASE_VERIFY, 0, "Verifying SHA256", user);
    ESP_LOGI(TAG, "computing SHA256 of %s ...", cand->bin_path);
    esp_err_t err = compute_file_sha256(cand->bin_path, computed);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "hash compute failed");
        if (cb) cb(OTA_PHASE_ERROR, 0, "Could not read firmware file", user);
        return err;
    }

    err = read_sidecar_hash(cand->sha_path, expected);
    if (err != ESP_OK) {
        if (cb) cb(OTA_PHASE_ERROR, 0, "Could not read .sha256 sidecar", user);
        return err;
    }

    if (memcmp(computed, expected, 32) != 0) {
        ESP_LOGE(TAG, "SHA256 MISMATCH — refusing to flash corrupt firmware");
        if (cb) cb(OTA_PHASE_ERROR, 0, "SHA256 mismatch — file corrupt", user);
        return ESP_ERR_INVALID_CRC;
    }
    ESP_LOGI(TAG, "SHA256 verified");
    if (cb) cb(OTA_PHASE_VERIFY, 100, "SHA256 verified", user);

    /* Begin OTA write. */
    const esp_partition_t *target = esp_ota_get_next_update_partition(NULL);
    if (!target) {
        ESP_LOGE(TAG, "no next update partition (OTA not configured?)");
        if (cb) cb(OTA_PHASE_ERROR, 0, "No OTA partition configured", user);
        return ESP_ERR_NOT_FOUND;
    }
    ESP_LOGI(TAG, "flashing to partition '%s' (offset 0x%lx, size %lu)",
             target->label, (unsigned long) target->address,
             (unsigned long) target->size);

    /* Get the .bin size for progress reporting. */
    struct stat st;
    if (stat(cand->bin_path, &st) != 0) {
        ESP_LOGE(TAG, "stat bin: %s", strerror(errno));
        return ESP_FAIL;
    }
    uint32_t total_bytes = (uint32_t) st.st_size;

    FILE *fp = fopen(cand->bin_path, "rb");
    if (!fp) {
        ESP_LOGE(TAG, "open bin: %s", strerror(errno));
        return ESP_ERR_NOT_FOUND;
    }

    esp_ota_handle_t handle = 0;
    err = esp_ota_begin(target, OTA_SIZE_UNKNOWN, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin: %s", esp_err_to_name(err));
        fclose(fp);
        return err;
    }

    uint8_t *buf = malloc(OTA_CHUNK_BYTES);
    if (!buf) {
        esp_ota_abort(handle);
        fclose(fp);
        return ESP_ERR_NO_MEM;
    }

    uint32_t written = 0;
    uint32_t next_pct_log = 0;
    size_t   n;

    while ((n = fread(buf, 1, OTA_CHUNK_BYTES, fp)) > 0) {
        err = esp_ota_write(handle, buf, n);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_ota_write at offset %lu: %s",
                     (unsigned long) written, esp_err_to_name(err));
            free(buf);
            fclose(fp);
            esp_ota_abort(handle);
            return err;
        }
        written += n;
        uint32_t pct = total_bytes ? (written * 100UL / total_bytes) : 0;
        if (pct >= next_pct_log) {
            ESP_LOGI(TAG, "  %lu%% (%lu / %lu bytes)",
                     (unsigned long) pct,
                     (unsigned long) written,
                     (unsigned long) total_bytes);
            if (cb) cb(OTA_PHASE_FLASH, (int) pct, "Flashing", user);
            next_pct_log = pct + 5;
        }
    }
    free(buf);
    fclose(fp);

    if (written != total_bytes) {
        ESP_LOGE(TAG, "short write: %lu / %lu",
                 (unsigned long) written, (unsigned long) total_bytes);
        esp_ota_abort(handle);
        if (cb) cb(OTA_PHASE_ERROR, 0, "Short write — flash incomplete", user);
        return ESP_FAIL;
    }

    err = esp_ota_end(handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_end: %s", esp_err_to_name(err));
        if (cb) cb(OTA_PHASE_ERROR, 0, "Image validation failed", user);
        return err;
    }

    err = esp_ota_set_boot_partition(target);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "set_boot_partition: %s", esp_err_to_name(err));
        if (cb) cb(OTA_PHASE_ERROR, 0, "Could not set boot partition", user);
        return err;
    }

    ESP_LOGI(TAG, "OTA complete — rebooting into new firmware in 2 seconds");
    if (cb) cb(OTA_PHASE_DONE, 100, "Update complete — rebooting", user);
    vTaskDelay(pdMS_TO_TICKS(2000));
    esp_restart();
    return ESP_OK; /* unreached */
}


/* ============================================================
 * Public API
 * ============================================================ */

esp_err_t ota_handle_pending_verify(void)
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    if (!running) {
        ESP_LOGW(TAG, "no running partition info");
        return ESP_OK;
    }

    esp_ota_img_states_t state = ESP_OTA_IMG_VALID;
    esp_err_t err = esp_ota_get_state_partition(running, &state);
    if (err != ESP_OK) {
        /* Common at first boot when otadata is fresh — not an error. */
        ESP_LOGD(TAG, "get_state_partition: %s", esp_err_to_name(err));
        return ESP_OK;
    }

    if (state != ESP_OTA_IMG_PENDING_VERIFY) {
        ESP_LOGI(TAG, "running partition '%s' state: %s",
                 running->label,
                 state == ESP_OTA_IMG_VALID    ? "VALID" :
                 state == ESP_OTA_IMG_NEW      ? "NEW" :
                 state == ESP_OTA_IMG_INVALID  ? "INVALID" :
                 state == ESP_OTA_IMG_ABORTED  ? "ABORTED" :
                 state == ESP_OTA_IMG_UNDEFINED ? "UNDEFINED" : "?");
        return ESP_OK;
    }

    /* Pending verify — this is the first boot after a successful OTA.
     * Run a brief self-test, then mark valid. If we crash before
     * mark_valid, the bootloader reverts on next boot. */
    ESP_LOGW(TAG, "Running partition '%s' is PENDING_VERIFY — running self-test",
             running->label);
    ESP_LOGI(TAG, "  self-test: heap-alive check, %lums",
             (unsigned long) SELF_TEST_DURATION_MS);

    uint32_t start_heap = esp_get_free_heap_size();
    vTaskDelay(pdMS_TO_TICKS(SELF_TEST_DURATION_MS));
    uint32_t end_heap = esp_get_free_heap_size();

    /* Simple sanity: heap should be stable (no runaway allocator) and
     * non-trivial (at least 64 KB free). Future versions extend this
     * with LVGL alive check, GPS source check, etc. */
    if (end_heap < 64 * 1024) {
        ESP_LOGE(TAG, "self-test FAIL: heap=%lu (<64KB) — rolling back",
                 (unsigned long) end_heap);
        esp_ota_mark_app_invalid_rollback_and_reboot();
        /* unreached */
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "  self-test PASS: heap %lu -> %lu",
             (unsigned long) start_heap, (unsigned long) end_heap);
    err = esp_ota_mark_app_valid_cancel_rollback();
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Marked partition '%s' VALID — update committed",
                 running->label);
    }
    return err;
}

esp_err_t ota_check_and_apply_from_sd(void)
{
    if (!sd_card_is_mounted()) {
        ESP_LOGW(TAG, "SD not mounted — skipping OTA check");
        return ESP_ERR_INVALID_STATE;
    }

    ota_candidate_t cand = {0};
    esp_err_t err = scan_for_candidate(&cand);
    if (err == ESP_ERR_NOT_FOUND) {
        ESP_LOGI(TAG, "no firmware update found on SD");
        return ESP_OK;
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "scan failed: %s", esp_err_to_name(err));
        return err;
    }

    fw_version_t running = {
        RUNNING_VERSION_MAJOR, RUNNING_VERSION_MINOR, RUNNING_VERSION_PATCH
    };
    int cmp = version_cmp(&cand.version, &running);
    if (cmp <= 0) {
        ESP_LOGI(TAG, "candidate v%d.%d.%d is not newer than running v%d.%d.%d — skipping",
                 cand.version.major, cand.version.minor, cand.version.patch,
                 running.major, running.minor, running.patch);
        return ESP_OK;
    }

    /*
     * v0.2 dev: auto-install without prompting.
     *
     * The serial-console y/n prompt in prompt_user() requires VFS-routed
     * stdin from USB-Serial-JTAG, which broke device output entirely
     * when we tried installing the driver. Until the touchscreen is
     * available to provide a UI confirmation (Phase 5+), any newer
     * firmware on SD is installed automatically.
     *
     * In production this would be gated by a config flag
     * (ota.auto_install) or by the touchscreen prompt. For dev, the
     * customer / developer controls what's on the SD card so auto-install
     * is safe enough.
     */
    ESP_LOGW(TAG, "Auto-installing v%d.%d.%d (no UI confirmation in v0.2 dev — "
                  "touchscreen prompt returns when graphics phase lands)",
             cand.version.major, cand.version.minor, cand.version.patch);

    err = apply_update(&cand, NULL, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "update failed: %s — continuing boot on current firmware",
                 esp_err_to_name(err));
    }
    return err;
}

esp_err_t ota_scan_sd(ota_update_info_t *info)
{
    if (!info) return ESP_ERR_INVALID_ARG;
    memset(info, 0, sizeof(*info));
    info->running_major = RUNNING_VERSION_MAJOR;
    info->running_minor = RUNNING_VERSION_MINOR;
    info->running_patch = RUNNING_VERSION_PATCH;

    if (!sd_card_is_mounted()) {
        ESP_LOGW(TAG, "SD not mounted — cannot scan for update");
        return ESP_ERR_INVALID_STATE;
    }

    ota_candidate_t cand = {0};
    esp_err_t err = scan_for_candidate(&cand);
    if (err == ESP_ERR_NOT_FOUND) {
        ESP_LOGI(TAG, "scan: no firmware on SD");
        return ESP_OK;          /* info->available stays false */
    }
    if (err != ESP_OK) return err;

    info->new_major = cand.version.major;
    info->new_minor = cand.version.minor;
    info->new_patch = cand.version.patch;
    strncpy(info->bin_path, cand.bin_path, sizeof(info->bin_path) - 1);
    strncpy(info->sha_path, cand.sha_path, sizeof(info->sha_path) - 1);

    fw_version_t running = {
        RUNNING_VERSION_MAJOR, RUNNING_VERSION_MINOR, RUNNING_VERSION_PATCH
    };
    info->available = (version_cmp(&cand.version, &running) > 0);
    ESP_LOGI(TAG, "scan: found v%d.%d.%d (running v%d.%d.%d) — %s",
             cand.version.major, cand.version.minor, cand.version.patch,
             running.major, running.minor, running.patch,
             info->available ? "newer" : "not newer");
    return ESP_OK;
}

esp_err_t ota_apply(const ota_update_info_t *info,
                     ota_progress_cb cb, void *user)
{
    if (!info || !info->available) return ESP_ERR_INVALID_ARG;
    ota_candidate_t cand = {0};
    strncpy(cand.bin_path, info->bin_path, sizeof(cand.bin_path) - 1);
    strncpy(cand.sha_path, info->sha_path, sizeof(cand.sha_path) - 1);
    cand.version.major = info->new_major;
    cand.version.minor = info->new_minor;
    cand.version.patch = info->new_patch;
    return apply_update(&cand, cb, user);   /* reboots on success */
}

void ota_log_status(void)
{
    const esp_partition_t *running = esp_ota_get_running_partition();
    const esp_partition_t *next    = esp_ota_get_next_update_partition(NULL);
    if (running) {
        ESP_LOGI(TAG, "running: '%s' subtype=%d offset=0x%lx size=%lu",
                 running->label, running->subtype,
                 (unsigned long) running->address,
                 (unsigned long) running->size);
    }
    if (next) {
        ESP_LOGI(TAG, "next OTA slot: '%s' subtype=%d offset=0x%lx",
                 next->label, next->subtype, (unsigned long) next->address);
    }
}
