/*
 * GPS source manager — single in-memory fix struct + freshness check
 * + ingest hooks from any of the configured transports (URL gateway
 * now, N2K + serial + internal later).
 *
 * Spec context: docs/config-schema.md § [gps] — "source priority
 * list" + "freshness_seconds". This v0.2 module implements the URL
 * ingest path only; the priority-list logic lands when more sources
 * exist to choose between.
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#pragma once

#include "nmea0183.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GPS_SRC_NONE = 0,
    GPS_SRC_URL,         /* TCP NMEA-0183 gateway */
    GPS_SRC_N2K,         /* PGN 129029 — future */
    GPS_SRC_SERIAL,      /* RS485 NMEA-0183 — future */
    GPS_SRC_INTERNAL,    /* on-board u-blox — IMU/GPS variant only */
} gps_source_t;

/* The live navigational state. All "_valid" flags are independent —
 * a source may give position without heading and vice-versa. */
typedef struct {
    bool          pos_valid;
    double        latitude;       /* signed decimal degrees */
    double        longitude;
    int           fix_quality;    /* 0 = none */
    int           satellites;
    double        hdop;
    double        altitude_m;

    bool          sog_valid;
    double        sog_kts;
    bool          cog_valid;
    double        cog_deg;

    bool          heading_valid;
    double        heading_deg;
    bool          heading_is_true;

    gps_source_t  source;
    uint64_t      last_update_us; /* esp_timer_get_time() of the last write */
} gps_fix_t;

/* Initialise the singleton state. Idempotent. */
void gps_source_init(void);

/* Merge a freshly-parsed NMEA sentence into the current fix. Any
 * "_valid" flags set in `fields` overwrite the corresponding fields
 * of the live state; flags left false in `fields` are not cleared
 * (sentences arrive interleaved — GGA only carries pos, HDM only
 * carries heading, etc.). Updates last_update_us. Thread-safe. */
void gps_source_ingest_nmea(gps_source_t source, const nmea_fields_t *fields);

/* Snapshot the current fix. Caller-allocated. Thread-safe. */
void gps_source_get(gps_fix_t *out);

/* True if last_update_us was within `max_age_ms` of now and any
 * "_valid" flag is set. */
bool gps_source_is_fresh(uint32_t max_age_ms);

#ifdef __cplusplus
}
#endif
