/*
 * Anchor Drag Pro — configuration model and loader. Implementation.
 *
 * See anchor_config.h and docs/config-schema.md.
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#include "anchor_config.h"
#include "sd_card.h"
#include "vendor/tomlc99/toml.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

static const char *TAG = "anchor_config";

#define CONFIG_PATH         SD_MOUNT_POINT "/anchor/config.toml"
#define NVS_NAMESPACE       "anchor_cfg"
#define NVS_KEY_DIST_M_FMT  "dist_m_%d"
#define NVS_KEY_DIST_U_FMT  "dist_u_%d"
#define NVS_KEY_SELECTED    "sel_idx"
#define NVS_KEY_ROTATION    "rot"
#define NVS_KEY_BRIGHT      "bright"
#define NVS_KEY_METRIC      "metric"
#define NVS_KEY_ARM_SEC     "arm_s"
#define NVS_KEY_DEV_NAME    "dev_name"
#define NVS_KEY_WIFI_MODE   "wifi_mode"
#define NVS_KEY_AP_SSID_PFX "ap_ssid_pfx"
#define NVS_KEY_AP_PASS     "ap_pass"
#define NVS_KEY_WIFI_BLOB   "wifi_nets"

/* Schema version for the WiFi networks blob — bump on layout change. */
#define NVS_WIFI_BLOB_VERSION  1

typedef struct {
    uint32_t              version;
    int32_t               count;
    anchor_wifi_network_t networks[ANCHOR_WIFI_MAX_NETWORKS];
} nvs_wifi_blob_t;

/* ---- Firmware safety bounds ---------------------------------------- */

#define DIST_MIN_M          1.5     /* ≈ 5 ft  — safety floor */
#define DIST_MAX_M          152.0   /* ≈ 500 ft — safety ceiling */
#define ARM_SEC_MIN         30
#define ARM_SEC_MAX         300
#define BRIGHT_MIN          0
#define BRIGHT_MAX          100

/* ---- errbuf helper ------------------------------------------------- */

static void errbuf_append(char *errbuf, size_t errbufsz, const char *fmt, ...)
{
    if (!errbuf || errbufsz == 0) return;
    size_t curlen = strlen(errbuf);
    if (curlen + 2 >= errbufsz) return;
    if (curlen > 0) {
        errbuf[curlen++] = '\n';
        errbuf[curlen]   = '\0';
    }
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(errbuf + curlen, errbufsz - curlen, fmt, ap);
    va_end(ap);
}

/* ---- Distance parsing --------------------------------------------- */

#define FT_TO_M 0.3048

static double round_1dp(double v)
{
    return roundf((float)(v * 10.0)) / 10.0;
}

static bool parse_distance_str(const char *s, anchor_distance_t *out)
{
    if (!s || !out || !*s) return false;

    /* Skip leading whitespace. */
    while (*s && isspace((unsigned char)*s)) s++;

    char *end = NULL;
    double v = strtod(s, &end);
    if (end == s) return false;          /* no number */
    if (v < 0) return false;             /* negative — reject */

    /* Skip optional whitespace between number and unit. */
    while (*end && isspace((unsigned char)*end)) end++;
    if (!*end) return false;             /* bare number — reject */

    /* Unit: "ft" or "m", case-insensitive. */
    anchor_unit_t unit;
    if ((end[0] == 'f' || end[0] == 'F') && (end[1] == 't' || end[1] == 'T') && end[2] == '\0') {
        unit = UNIT_FT;
    } else if ((end[0] == 'm' || end[0] == 'M') && end[1] == '\0') {
        unit = UNIT_M;
    } else {
        return false;                    /* unsupported unit */
    }

    out->value  = round_1dp(v);
    out->unit   = unit;
    out->meters = (unit == UNIT_FT) ? out->value * FT_TO_M : out->value;
    return true;
}

static bool distances_equal(const anchor_distance_t *a, const anchor_distance_t *b)
{
    return fabs(a->meters - b->meters) < 0.05;
}

/* ---- Defaults ------------------------------------------------------ */

void anchor_config_defaults(anchor_config_t *out)
{
    if (!out) return;
    memset(out, 0, sizeof(*out));

    /* Device */
    snprintf(out->device.name, sizeof(out->device.name), "%s", "");
    out->device.metric = false;          /* default to feet (US market) */

    /* Display */
    out->display.rotation   = 0;
    out->display.brightness = 80;

    /* Anchor: 25/50/100 ft presets, 50 ft selected, 60 s arming, medium volume */
    out->anchor.options[0] = (anchor_distance_t){.value = 25,  .unit = UNIT_FT, .meters = 25  * FT_TO_M};
    out->anchor.options[1] = (anchor_distance_t){.value = 50,  .unit = UNIT_FT, .meters = 50  * FT_TO_M};
    out->anchor.options[2] = (anchor_distance_t){.value = 100, .unit = UNIT_FT, .meters = 100 * FT_TO_M};
    out->anchor.selected_idx   = 1;
    out->anchor.arming_seconds = 60;
    out->anchor.sound_volume   = SOUND_MEDIUM;

    /* Wifi */
    out->wifi.mode = WIFI_AP;
    snprintf(out->wifi.ap_ssid_prefix, sizeof(out->wifi.ap_ssid_prefix), "AnchorAlarm");
    out->wifi.ap_password[0] = '\0';
    out->wifi.sta_network_count = 0;
}

/* ---- Parsing each section ----------------------------------------- */

/* Helper macros for tomlc99 datum lookup and free. */
#define D_FREE_STR(d) do { if ((d).ok) free((d).u.s); } while (0)

static void parse_device(toml_table_t *t, anchor_config_t *cfg,
                          char *eb, size_t ebsz)
{
    toml_table_t *dev = toml_table_in(t, "device");
    if (!dev) return;

    toml_datum_t name = toml_string_in(dev, "name");
    if (name.ok) {
        snprintf(cfg->device.name, sizeof(cfg->device.name), "%s", name.u.s);
        free(name.u.s);
    }

    toml_datum_t metric = toml_bool_in(dev, "metric");
    if (metric.ok) {
        cfg->device.metric = metric.u.b;
    }
}

static void parse_display(toml_table_t *t, anchor_config_t *cfg,
                           char *eb, size_t ebsz)
{
    toml_table_t *dis = toml_table_in(t, "display");
    if (!dis) return;

    toml_datum_t rot = toml_int_in(dis, "rotation");
    if (rot.ok) {
        int r = (int) rot.u.i;
        if (r == 0 || r == 90 || r == 180 || r == 270) {
            cfg->display.rotation = r;
        } else {
            errbuf_append(eb, ebsz, "display.rotation %d invalid (must be 0/90/180/270); keeping default %d",
                          r, cfg->display.rotation);
        }
    }

    toml_datum_t br = toml_int_in(dis, "brightness");
    if (br.ok) {
        int b = (int) br.u.i;
        if (b >= BRIGHT_MIN && b <= BRIGHT_MAX) {
            cfg->display.brightness = b;
        } else {
            errbuf_append(eb, ebsz, "display.brightness %d out of range (%d-%d); keeping default %d",
                          b, BRIGHT_MIN, BRIGHT_MAX, cfg->display.brightness);
        }
    }
}

static void parse_anchor_options(toml_table_t *anc, anchor_config_t *cfg,
                                  char *eb, size_t ebsz)
{
    toml_table_t *opts = toml_table_in(anc, "options");
    if (!opts) return;

    toml_array_t *distances = toml_array_in(opts, "distances");
    if (!distances) return;

    int n = toml_array_nelem(distances);
    if (n != 3) {
        errbuf_append(eb, ebsz, "anchor.options.distances must have exactly 3 entries (got %d); keeping defaults", n);
        return;
    }

    anchor_distance_t parsed[3];
    for (int i = 0; i < 3; i++) {
        toml_datum_t d = toml_string_at(distances, i);
        if (!d.ok) {
            errbuf_append(eb, ebsz, "anchor.options.distances[%d] not a string; keeping defaults", i);
            for (int k = i; k > 0; k--) {} /* nothing to free up to here */
            return;
        }
        if (!parse_distance_str(d.u.s, &parsed[i])) {
            errbuf_append(eb, ebsz, "anchor.options.distances[%d]=\"%s\" rejected (need e.g. \"50ft\" or \"15m\")",
                          i, d.u.s);
            free(d.u.s);
            return;
        }
        free(d.u.s);

        /* Bounds */
        if (parsed[i].meters < DIST_MIN_M || parsed[i].meters > DIST_MAX_M) {
            errbuf_append(eb, ebsz, "anchor.options.distances[%d] = %.1f m out of bounds (%.1f-%.1f m); keeping defaults",
                          i, parsed[i].meters, DIST_MIN_M, DIST_MAX_M);
            return;
        }
    }

    /* Homogeneous units */
    if (parsed[0].unit != parsed[1].unit || parsed[1].unit != parsed[2].unit) {
        errbuf_append(eb, ebsz, "anchor.options.distances must all use the same unit; keeping defaults");
        return;
    }

    /* No duplicates */
    if (distances_equal(&parsed[0], &parsed[1]) ||
        distances_equal(&parsed[1], &parsed[2]) ||
        distances_equal(&parsed[0], &parsed[2])) {
        errbuf_append(eb, ebsz, "anchor.options.distances entries must be distinct; keeping defaults");
        return;
    }

    memcpy(cfg->anchor.options, parsed, sizeof(parsed));
}

static void parse_anchor(toml_table_t *t, anchor_config_t *cfg,
                          char *eb, size_t ebsz)
{
    toml_table_t *anc = toml_table_in(t, "anchor");
    if (!anc) return;

    parse_anchor_options(anc, cfg, eb, ebsz);

    /* alarm_distance — must be one of options. */
    toml_datum_t sel = toml_string_in(anc, "alarm_distance");
    if (sel.ok) {
        anchor_distance_t want;
        if (parse_distance_str(sel.u.s, &want)) {
            int found = -1;
            for (int i = 0; i < 3; i++) {
                if (distances_equal(&cfg->anchor.options[i], &want)) {
                    found = i;
                    break;
                }
            }
            if (found >= 0) {
                cfg->anchor.selected_idx = found;
            } else {
                errbuf_append(eb, ebsz, "anchor.alarm_distance=\"%s\" not in options; keeping default (index %d)",
                              sel.u.s, cfg->anchor.selected_idx);
            }
        } else {
            errbuf_append(eb, ebsz, "anchor.alarm_distance=\"%s\" malformed", sel.u.s);
        }
        free(sel.u.s);
    }

    toml_datum_t arm = toml_int_in(anc, "arming_seconds");
    if (arm.ok) {
        int a = (int) arm.u.i;
        if (a >= ARM_SEC_MIN && a <= ARM_SEC_MAX) {
            cfg->anchor.arming_seconds = a;
        } else {
            errbuf_append(eb, ebsz, "anchor.arming_seconds=%d out of range (%d-%d); keeping default %d",
                          a, ARM_SEC_MIN, ARM_SEC_MAX, cfg->anchor.arming_seconds);
        }
    }

    toml_datum_t vol = toml_string_in(anc, "sound_volume");
    if (vol.ok) {
        if (strcasecmp(vol.u.s, "low") == 0)         cfg->anchor.sound_volume = SOUND_LOW;
        else if (strcasecmp(vol.u.s, "medium") == 0) cfg->anchor.sound_volume = SOUND_MEDIUM;
        else if (strcasecmp(vol.u.s, "high") == 0)   cfg->anchor.sound_volume = SOUND_HIGH;
        else {
            errbuf_append(eb, ebsz, "anchor.sound_volume=\"%s\" invalid (low/medium/high); keeping default",
                          vol.u.s);
        }
        free(vol.u.s);
    }
}

static bool append_sta_network(anchor_wifi_cfg_t *wcfg,
                                const char *ssid, const char *pw,
                                char *eb, size_t ebsz, int idx_for_errs)
{
    if (!ssid || !*ssid) {
        errbuf_append(eb, ebsz, "wifi.networks[%d] ssid empty; skipping", idx_for_errs);
        return false;
    }
    if (strlen(ssid) > 32) {
        errbuf_append(eb, ebsz, "wifi.networks[%d] ssid >32 chars; skipping", idx_for_errs);
        return false;
    }
    if (pw && strlen(pw) > 63) {
        errbuf_append(eb, ebsz, "wifi.networks[%d] password >63 chars; skipping", idx_for_errs);
        return false;
    }
    if (wcfg->sta_network_count >= ANCHOR_WIFI_MAX_NETWORKS) {
        errbuf_append(eb, ebsz, "wifi.networks[%d] dropped: max %d networks supported",
                      idx_for_errs, ANCHOR_WIFI_MAX_NETWORKS);
        return false;
    }
    anchor_wifi_network_t *n = &wcfg->sta_networks[wcfg->sta_network_count++];
    snprintf(n->ssid,     sizeof(n->ssid),     "%s", ssid);
    snprintf(n->password, sizeof(n->password), "%s", pw ? pw : "");
    return true;
}

static void parse_wifi(toml_table_t *t, anchor_config_t *cfg,
                        char *eb, size_t ebsz)
{
    toml_table_t *wifi = toml_table_in(t, "wifi");
    if (!wifi) return;

    toml_datum_t mode = toml_string_in(wifi, "mode");
    if (mode.ok) {
        if (strcasecmp(mode.u.s, "ap") == 0)       cfg->wifi.mode = WIFI_AP;
        else if (strcasecmp(mode.u.s, "sta") == 0) cfg->wifi.mode = WIFI_STA;
        else if (strcasecmp(mode.u.s, "off") == 0) cfg->wifi.mode = WIFI_OFF;
        else errbuf_append(eb, ebsz, "wifi.mode=\"%s\" invalid (ap/sta/off); keeping default", mode.u.s);
        free(mode.u.s);
    }

    toml_table_t *ap = toml_table_in(wifi, "ap");
    if (ap) {
        toml_datum_t s = toml_string_in(ap, "ssid_prefix");
        if (s.ok) { snprintf(cfg->wifi.ap_ssid_prefix, sizeof(cfg->wifi.ap_ssid_prefix), "%s", s.u.s); free(s.u.s); }
        toml_datum_t p = toml_string_in(ap, "password");
        if (p.ok) { snprintf(cfg->wifi.ap_password,    sizeof(cfg->wifi.ap_password),    "%s", p.u.s); free(p.u.s); }
    }

    /* Reset before parsing — SD config is authoritative when present. */
    cfg->wifi.sta_network_count = 0;

    /* Preferred form: [[wifi.networks]] array of tables. Order is priority. */
    toml_array_t *nets = toml_array_in(wifi, "networks");
    if (nets) {
        int n = toml_array_nelem(nets);
        for (int i = 0; i < n; i++) {
            toml_table_t *net = toml_table_at(nets, i);
            if (!net) continue;
            toml_datum_t s = toml_string_in(net, "ssid");
            toml_datum_t p = toml_string_in(net, "password");
            append_sta_network(&cfg->wifi, s.ok ? s.u.s : NULL, p.ok ? p.u.s : NULL,
                                eb, ebsz, i);
            if (s.ok) free(s.u.s);
            if (p.ok) free(p.u.s);
        }
    }

    /* Backward-compat: legacy single [wifi.sta] is appended after networks
     * if it has an ssid and the networks list didn't already hit the cap. */
    toml_table_t *sta = toml_table_in(wifi, "sta");
    if (sta) {
        toml_datum_t s = toml_string_in(sta, "ssid");
        toml_datum_t p = toml_string_in(sta, "password");
        if (s.ok && s.u.s[0] != '\0') {
            append_sta_network(&cfg->wifi, s.u.s, p.ok ? p.u.s : NULL,
                                eb, ebsz, cfg->wifi.sta_network_count);
        }
        if (s.ok) free(s.u.s);
        if (p.ok) free(p.u.s);
    }
}

/* ---- Public: parse from string ------------------------------------- */

esp_err_t anchor_config_parse_string(const char *toml_text,
                                      anchor_config_t *out,
                                      char *errbuf, size_t errbufsz)
{
    if (!toml_text || !out) return ESP_ERR_INVALID_ARG;
    if (errbuf && errbufsz > 0) errbuf[0] = '\0';

    /* tomlc99 needs a writable buffer. */
    char *buf = strdup(toml_text);
    if (!buf) return ESP_ERR_NO_MEM;

    char parse_err[256];
    toml_table_t *root = toml_parse(buf, parse_err, sizeof(parse_err));
    free(buf);

    if (!root) {
        errbuf_append(errbuf, errbufsz, "TOML parse error: %s", parse_err);
        return ESP_FAIL;
    }

    parse_device (root, out, errbuf, errbufsz);
    parse_display(root, out, errbuf, errbufsz);
    parse_anchor (root, out, errbuf, errbufsz);
    parse_wifi   (root, out, errbuf, errbufsz);

    toml_free(root);
    return ESP_OK;
}

/* ---- Load from /sdcard/anchor/config.toml -------------------------- */

static esp_err_t load_from_sd(anchor_config_t *out, char *errbuf, size_t errbufsz)
{
    if (!sd_card_is_mounted()) {
        return ESP_ERR_INVALID_STATE;
    }

    FILE *fp = fopen(CONFIG_PATH, "r");
    if (!fp) {
        return ESP_ERR_NOT_FOUND;
    }

    /* Read whole file into memory. Max 16 KB (config files should be far
     * smaller; cap protects against unbounded reads of a wrong file). */
    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (sz <= 0 || sz > 16 * 1024) {
        fclose(fp);
        errbuf_append(errbuf, errbufsz, "config.toml size %ld out of range (>0, <16KB)", sz);
        return ESP_FAIL;
    }
    char *buf = malloc((size_t) sz + 1);
    if (!buf) { fclose(fp); return ESP_ERR_NO_MEM; }
    size_t n = fread(buf, 1, (size_t) sz, fp);
    buf[n] = '\0';
    fclose(fp);

    esp_err_t err = anchor_config_parse_string(buf, out, errbuf, errbufsz);
    free(buf);
    return err;
}

/* ---- NVS cache (last-known-good) ----------------------------------- */
/* For v0.2 minimal: we cache the most safety-critical fields. Web UI
 * saves write both SD AND NVS, so the cache is kept fresh. */

static void load_from_nvs(anchor_config_t *out)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &h) != ESP_OK) {
        return;
    }

    int32_t i32;
    uint8_t u8;
    size_t  slen;

    /* device.name */
    slen = sizeof(out->device.name);
    nvs_get_str(h, NVS_KEY_DEV_NAME, out->device.name, &slen);

    /* device.metric */
    if (nvs_get_u8(h, NVS_KEY_METRIC, &u8) == ESP_OK) out->device.metric = (u8 != 0);

    /* display.rotation, brightness */
    if (nvs_get_i32(h, NVS_KEY_ROTATION, &i32) == ESP_OK) {
        if (i32 == 0 || i32 == 90 || i32 == 180 || i32 == 270) {
            out->display.rotation = (int) i32;
        }
    }
    if (nvs_get_i32(h, NVS_KEY_BRIGHT, &i32) == ESP_OK) {
        if (i32 >= BRIGHT_MIN && i32 <= BRIGHT_MAX) out->display.brightness = (int) i32;
    }

    /* anchor.options + selected (cached as meters + unit per slot) */
    bool all_ok = true;
    anchor_distance_t opts[3];
    for (int i = 0; i < 3; i++) {
        char key_m[16], key_u[16];
        snprintf(key_m, sizeof(key_m), NVS_KEY_DIST_M_FMT, i);
        snprintf(key_u, sizeof(key_u), NVS_KEY_DIST_U_FMT, i);
        int32_t m_x10;
        if (nvs_get_i32(h, key_m, &m_x10) != ESP_OK || nvs_get_u8(h, key_u, &u8) != ESP_OK) {
            all_ok = false; break;
        }
        opts[i].meters = m_x10 / 10.0;
        opts[i].unit   = (u8 == 0) ? UNIT_FT : UNIT_M;
        opts[i].value  = (opts[i].unit == UNIT_FT) ? (opts[i].meters / FT_TO_M) : opts[i].meters;
        opts[i].value  = round_1dp(opts[i].value);
    }
    if (all_ok) {
        memcpy(out->anchor.options, opts, sizeof(opts));
        if (nvs_get_i32(h, NVS_KEY_SELECTED, &i32) == ESP_OK && i32 >= 0 && i32 < 3) {
            out->anchor.selected_idx = (int) i32;
        }
    }

    if (nvs_get_i32(h, NVS_KEY_ARM_SEC, &i32) == ESP_OK &&
        i32 >= ARM_SEC_MIN && i32 <= ARM_SEC_MAX) {
        out->anchor.arming_seconds = (int) i32;
    }

    /* WiFi mode + AP creds */
    if (nvs_get_u8(h, NVS_KEY_WIFI_MODE, &u8) == ESP_OK) {
        if (u8 == WIFI_OFF || u8 == WIFI_AP || u8 == WIFI_STA) {
            out->wifi.mode = (anchor_wifi_mode_t) u8;
        }
    }
    slen = sizeof(out->wifi.ap_ssid_prefix);
    nvs_get_str(h, NVS_KEY_AP_SSID_PFX, out->wifi.ap_ssid_prefix, &slen);
    slen = sizeof(out->wifi.ap_password);
    nvs_get_str(h, NVS_KEY_AP_PASS,     out->wifi.ap_password,    &slen);

    /* WiFi STA networks blob. */
    nvs_wifi_blob_t blob;
    size_t blen = sizeof(blob);
    if (nvs_get_blob(h, NVS_KEY_WIFI_BLOB, &blob, &blen) == ESP_OK
        && blen == sizeof(blob)
        && blob.version == NVS_WIFI_BLOB_VERSION
        && blob.count >= 0 && blob.count <= ANCHOR_WIFI_MAX_NETWORKS) {
        out->wifi.sta_network_count = blob.count;
        memcpy(out->wifi.sta_networks, blob.networks, sizeof(blob.networks));
    }

    nvs_close(h);
}

/* Mirror the resolved config back to NVS so the cache always reflects the
 * authoritative SD config. Called after a successful SD parse. */
static void save_to_nvs(const anchor_config_t *in)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h) != ESP_OK) {
        ESP_LOGW(TAG, "NVS mirror: open failed; cache may be stale");
        return;
    }

    nvs_set_str(h, NVS_KEY_DEV_NAME,    in->device.name);
    nvs_set_u8 (h, NVS_KEY_METRIC,      in->device.metric ? 1 : 0);
    nvs_set_i32(h, NVS_KEY_ROTATION,    in->display.rotation);
    nvs_set_i32(h, NVS_KEY_BRIGHT,      in->display.brightness);

    for (int i = 0; i < 3; i++) {
        char key_m[16], key_u[16];
        snprintf(key_m, sizeof(key_m), NVS_KEY_DIST_M_FMT, i);
        snprintf(key_u, sizeof(key_u), NVS_KEY_DIST_U_FMT, i);
        int32_t m_x10 = (int32_t) round(in->anchor.options[i].meters * 10.0);
        nvs_set_i32(h, key_m, m_x10);
        nvs_set_u8 (h, key_u, (uint8_t) in->anchor.options[i].unit);
    }
    nvs_set_i32(h, NVS_KEY_SELECTED, in->anchor.selected_idx);
    nvs_set_i32(h, NVS_KEY_ARM_SEC,  in->anchor.arming_seconds);

    nvs_set_u8 (h, NVS_KEY_WIFI_MODE,    (uint8_t) in->wifi.mode);
    nvs_set_str(h, NVS_KEY_AP_SSID_PFX,  in->wifi.ap_ssid_prefix);
    nvs_set_str(h, NVS_KEY_AP_PASS,      in->wifi.ap_password);

    nvs_wifi_blob_t blob = {
        .version = NVS_WIFI_BLOB_VERSION,
        .count   = in->wifi.sta_network_count,
    };
    memcpy(blob.networks, in->wifi.sta_networks, sizeof(blob.networks));
    nvs_set_blob(h, NVS_KEY_WIFI_BLOB, &blob, sizeof(blob));

    esp_err_t cerr = nvs_commit(h);
    nvs_close(h);
    if (cerr != ESP_OK) {
        ESP_LOGW(TAG, "NVS mirror: commit failed (%s)", esp_err_to_name(cerr));
    } else {
        ESP_LOGI(TAG, "NVS mirror: cache refreshed from SD config");
    }
}

/* Public wrapper around save_to_nvs() so other modules (e.g. the
 * on-device WiFi editor) can persist a config change without going
 * through the SD writer (#63). */
esp_err_t anchor_config_save_nvs(const anchor_config_t *in)
{
    if (!in) return ESP_ERR_INVALID_ARG;
    save_to_nvs(in);
    return ESP_OK;
}

/* ---- Three-tier load ---------------------------------------------- */

esp_err_t anchor_config_load(anchor_config_t *out)
{
    if (!out) return ESP_ERR_INVALID_ARG;

    /* Tier 3: built-in defaults. */
    anchor_config_defaults(out);

    /* Tier 2: NVS cache overrides defaults. */
    load_from_nvs(out);

    /* Tier 1: SD config.toml overrides everything (per field). */
    char eb[512] = {0};
    esp_err_t err = load_from_sd(out, eb, sizeof(eb));
    if (err == ESP_OK) {
        if (eb[0]) {
            ESP_LOGW(TAG, "config.toml loaded with warnings:\n%s", eb);
        } else {
            ESP_LOGI(TAG, "config.toml loaded cleanly");
        }
        /* SD wins: mirror back to NVS so next boot without a card sees
         * the latest config. Customer behavior the user requested:
         * if SD differs from NVS, SD overwrites NVS. */
        save_to_nvs(out);
    } else if (err == ESP_ERR_NOT_FOUND) {
        ESP_LOGI(TAG, "no /sdcard/anchor/config.toml — using NVS cache + defaults");
    } else if (err == ESP_ERR_INVALID_STATE) {
        ESP_LOGI(TAG, "SD not mounted — using NVS cache + defaults");
    } else {
        ESP_LOGW(TAG, "config.toml load failed (%s): %s", esp_err_to_name(err), eb);
    }

    return ESP_OK;
}

/* ---- Diagnostic logging ------------------------------------------- */

static const char *unit_str(anchor_unit_t u) { return u == UNIT_M ? "m" : "ft"; }
static const char *vol_str(anchor_sound_volume_t v) {
    switch (v) { case SOUND_LOW: return "low"; case SOUND_HIGH: return "high"; default: return "medium"; }
}
static const char *wifi_mode_str(anchor_wifi_mode_t m) {
    switch (m) { case WIFI_OFF: return "off"; case WIFI_STA: return "sta"; default: return "ap"; }
}

void anchor_config_log(const anchor_config_t *c)
{
    if (!c) return;
    ESP_LOGI(TAG, "device.name           = \"%s\"", c->device.name);
    ESP_LOGI(TAG, "device.metric         = %s", c->device.metric ? "true (m)" : "false (ft)");
    ESP_LOGI(TAG, "display.rotation      = %d", c->display.rotation);
    ESP_LOGI(TAG, "display.brightness    = %d", c->display.brightness);
    ESP_LOGI(TAG, "anchor.distances      = [%.1f%s, %.1f%s, %.1f%s] (selected=%d)",
             c->anchor.options[0].value, unit_str(c->anchor.options[0].unit),
             c->anchor.options[1].value, unit_str(c->anchor.options[1].unit),
             c->anchor.options[2].value, unit_str(c->anchor.options[2].unit),
             c->anchor.selected_idx);
    ESP_LOGI(TAG, "anchor.arming_seconds = %d", c->anchor.arming_seconds);
    ESP_LOGI(TAG, "anchor.sound_volume   = %s", vol_str(c->anchor.sound_volume));
    ESP_LOGI(TAG, "wifi.mode             = %s", wifi_mode_str(c->wifi.mode));
    ESP_LOGI(TAG, "wifi.ap.ssid_prefix   = \"%s\"", c->wifi.ap_ssid_prefix);
    ESP_LOGI(TAG, "wifi.networks         = %d configured", c->wifi.sta_network_count);
    for (int i = 0; i < c->wifi.sta_network_count; i++) {
        /* Don't log passwords. Just an indicator of presence. */
        ESP_LOGI(TAG, "  [%d] ssid=\"%s\" password=%s",
                 i, c->wifi.sta_networks[i].ssid,
                 c->wifi.sta_networks[i].password[0] ? "(set)" : "(open)");
    }
}
