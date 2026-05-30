/*
 * NMEA 0183 parser — implementation.
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#include "nmea0183.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

/* ---- Internal helpers ---- */

/* Validate the *HH checksum at the end of an NMEA sentence (between
 * the leading $ and the trailing *). Returns the start of the
 * payload (just after $) and writes the payload length up to *. */
static bool checksum_ok(const char *line, const char **payload, size_t *payload_len)
{
    if (!line) return false;
    /* Skip optional leading $. */
    if (line[0] == '$') line++;

    /* Find the * separator. */
    const char *star = strchr(line, '*');
    if (!star || star == line) return false;
    if (star[1] == '\0' || star[2] == '\0') return false;

    /* Two hex digits after *. */
    char hex[3] = { star[1], star[2], 0 };
    char *end;
    long expected = strtol(hex, &end, 16);
    if (end != hex + 2) return false;

    /* XOR everything between $ (excl.) and * (excl.). */
    uint8_t got = 0;
    for (const char *p = line; p < star; p++) got ^= (uint8_t) *p;

    if ((uint8_t) expected != got) return false;
    *payload     = line;
    *payload_len = star - line;
    return true;
}

/* Split a comma-delimited payload into up to max_fields slices. Returns
 * the count actually found. Writes pointers into a working buffer that
 * the caller owns; original text is destructively modified (commas
 * replaced with NULs). */
static int split_fields(char *payload, char *fields[], int max_fields)
{
    int count = 0;
    char *p = payload;
    if (max_fields > 0) fields[count++] = p;
    while (*p && count < max_fields) {
        if (*p == ',') {
            *p = '\0';
            fields[count++] = p + 1;
        }
        p++;
    }
    return count;
}

/* Convert "DDMM.MMMM" + 'N/S/E/W' indicator to signed decimal degrees.
 * Returns NaN on malformed input. */
static double parse_ddmm(const char *s, char hemi)
{
    if (!s || !*s) return NAN;
    /* Decimal point can be anywhere; minutes are everything to the right
     * of the dot, plus the two digits to the left of the dot.
     * Degrees = integer part above the minutes. */
    const char *dot = strchr(s, '.');
    if (!dot) {
        /* No fractional minutes — still valid (rare). Assume MM is the
         * last two digits before end-of-string. */
        size_t len = strlen(s);
        if (len < 3) return NAN;
        dot = s + len;   /* virtual dot after the last char */
    }
    /* "DDMM" or "DDDMM" — minutes are the two digits before the dot. */
    int min_start = (int)(dot - s) - 2;
    if (min_start < 1) return NAN;

    char deg_buf[8] = { 0 };
    int  deg_len = min_start;
    if (deg_len > (int) sizeof(deg_buf) - 1) deg_len = sizeof(deg_buf) - 1;
    memcpy(deg_buf, s, deg_len);

    double degrees = atof(deg_buf);
    double minutes = atof(s + min_start);
    double value   = degrees + minutes / 60.0;
    if (hemi == 'S' || hemi == 'W') value = -value;
    return value;
}

/* ---- Per-sentence parsers ----
 *
 * All take the comma-split fields (talker+type as fields[0], e.g. "GPGGA").
 * Field indices follow the standard NMEA 0183 numbering for that sentence.
 */

static void parse_gga(char *fields[], int count, nmea_fields_t *out)
{
    /* $GPGGA,hhmmss.ss,DDMM.MMMM,N,DDDMM.MMMM,W,Q,SS,HDOP,ALT,M,GEOID,M,,*CC */
    if (count < 10) return;
    if (fields[2][0] && fields[3][0] && fields[4][0] && fields[5][0]) {
        out->latitude  = parse_ddmm(fields[2], fields[3][0]);
        out->longitude = parse_ddmm(fields[4], fields[5][0]);
        if (!isnan(out->latitude) && !isnan(out->longitude)) {
            out->pos_valid = true;
        }
    }
    if (fields[6][0]) {
        out->fix_quality = atoi(fields[6]);
        out->fix_valid   = (out->fix_quality > 0);
    }
    if (fields[7][0]) out->satellites = atoi(fields[7]);
    if (fields[8][0]) out->hdop       = atof(fields[8]);
    if (count > 9 && fields[9][0]) out->altitude_m = atof(fields[9]);
}

static void parse_rmc(char *fields[], int count, nmea_fields_t *out)
{
    /* $GPRMC,hhmmss.ss,A,DDMM.MMMM,N,DDDMM.MMMM,W,SOG_kts,COG_deg,ddmmyy,MAG_VAR,MV_DIR,FAA*CC */
    if (count < 9) return;
    bool active = (fields[2][0] == 'A');
    if (active && fields[3][0] && fields[4][0] && fields[5][0] && fields[6][0]) {
        out->latitude  = parse_ddmm(fields[3], fields[4][0]);
        out->longitude = parse_ddmm(fields[5], fields[6][0]);
        if (!isnan(out->latitude) && !isnan(out->longitude)) {
            out->pos_valid = true;
            out->fix_valid = true;
            if (out->fix_quality == 0) out->fix_quality = 1;
        }
    }
    if (fields[7][0]) {
        out->sog_kts   = atof(fields[7]);
        out->sog_valid = true;
    }
    if (fields[8][0]) {
        out->cog_deg   = atof(fields[8]);
        out->cog_valid = true;
    }
}

static void parse_hdm(char *fields[], int count, nmea_fields_t *out)
{
    /* $HDM,hhh.h,M*CC  OR  $GPHDM,...,M*CC */
    if (count < 2) return;
    if (fields[1][0]) {
        out->heading_deg     = atof(fields[1]);
        out->heading_valid   = true;
        out->heading_is_true = false;
    }
}

static void parse_hdt(char *fields[], int count, nmea_fields_t *out)
{
    /* $HDT,hhh.h,T*CC */
    if (count < 2) return;
    if (fields[1][0]) {
        out->heading_deg     = atof(fields[1]);
        out->heading_valid   = true;
        out->heading_is_true = true;
    }
}

static void parse_vtg(char *fields[], int count, nmea_fields_t *out)
{
    /* $GPVTG,track_true,T,track_mag,M,speed_kts,N,speed_kmh,K,FAA*CC */
    if (count < 6) return;
    if (fields[1][0]) {
        out->cog_deg   = atof(fields[1]);
        out->cog_valid = true;
    }
    if (count > 5 && fields[5][0]) {
        out->sog_kts   = atof(fields[5]);
        out->sog_valid = true;
    }
}

/* ---- Public entry ---- */

nmea_sentence_t nmea0183_parse(const char *line, nmea_fields_t *out)
{
    if (!line || !out) return NMEA_UNKNOWN;
    out->type = NMEA_UNKNOWN;

    const char *payload;
    size_t plen;
    if (!checksum_ok(line, &payload, &plen)) {
        out->type = NMEA_BAD_CHECKSUM;
        return NMEA_BAD_CHECKSUM;
    }

    /* Work on a mutable copy — bounded to the payload length so we
     * don't read past *CC. */
    char buf[120];
    if (plen >= sizeof(buf)) plen = sizeof(buf) - 1;
    memcpy(buf, payload, plen);
    buf[plen] = '\0';

    char *fields[24];
    int   n = split_fields(buf, fields, 24);
    if (n < 1) return NMEA_UNKNOWN;

    /* fields[0] is e.g. "GPGGA" / "GPRMC" / "HDM" / "HDT" / "GPVTG".
     * Match on the last 3 chars (sentence type), ignoring talker. */
    const char *tag = fields[0];
    size_t tl = strlen(tag);
    const char *sent = (tl >= 3) ? tag + (tl - 3) : tag;

    if (strcmp(sent, "GGA") == 0) {
        parse_gga(fields, n, out);
        out->type = NMEA_GGA;
    } else if (strcmp(sent, "RMC") == 0) {
        parse_rmc(fields, n, out);
        out->type = NMEA_RMC;
    } else if (strcmp(sent, "HDM") == 0) {
        parse_hdm(fields, n, out);
        out->type = NMEA_HDM;
    } else if (strcmp(sent, "HDT") == 0) {
        parse_hdt(fields, n, out);
        out->type = NMEA_HDT;
    } else if (strcmp(sent, "VTG") == 0) {
        parse_vtg(fields, n, out);
        out->type = NMEA_VTG;
    } else {
        out->type = NMEA_UNKNOWN;
    }
    return out->type;
}
