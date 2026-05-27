/*
 * NMEA 0183 parser — pure C, no I/O.
 *
 * Parses the sentences the device cares about for anchor watch:
 *   GGA — time + lat/lon + fix quality + satellites + HDOP + altitude
 *   RMC — time/date + status + lat/lon + SOG + COG
 *   HDM / HDT — magnetic / true heading
 *   VTG — track + ground speed
 *
 * Checksum-validated. Any sentence with a bad checksum is rejected.
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    NMEA_UNKNOWN = 0,
    NMEA_BAD_CHECKSUM,
    NMEA_GGA,
    NMEA_RMC,
    NMEA_HDM,        /* magnetic heading */
    NMEA_HDT,        /* true heading */
    NMEA_VTG,
} nmea_sentence_t;

/* Subset of fields the parser will populate. Each field has a
 * dedicated "_valid" flag; the caller merges into a longer-lived
 * fix struct (gps_source). */
typedef struct {
    nmea_sentence_t type;

    bool   pos_valid;       /* GGA / RMC */
    double latitude;        /* signed decimal degrees */
    double longitude;

    bool   fix_valid;       /* GGA quality > 0 OR RMC status == 'A' */
    int    fix_quality;     /* GGA quality code (0..) */
    int    satellites;      /* GGA */
    double hdop;            /* GGA */
    double altitude_m;      /* GGA */

    bool   sog_valid;       /* RMC / VTG */
    double sog_kts;
    bool   cog_valid;
    double cog_deg;         /* true track */

    bool   heading_valid;   /* HDM / HDT */
    double heading_deg;
    bool   heading_is_true; /* false → magnetic */
} nmea_fields_t;

/* Parse one NMEA sentence. `line` is the raw text — leading $ is
 * optional, trailing \r\n is tolerated. Returns the sentence type
 * (or NMEA_UNKNOWN / NMEA_BAD_CHECKSUM). On success, populates the
 * relevant fields of `out`; untouched fields keep their input value
 * (callers typically zero `out` before passing). */
nmea_sentence_t nmea0183_parse(const char *line, nmea_fields_t *out);

#ifdef __cplusplus
}
#endif
