/*
 * Anchor geometry — implementation.
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#include "anchor_geo.h"
#include <math.h>
#include <string.h>

#define EARTH_RADIUS_M  6371000.0
#define DEG_TO_RAD      (M_PI / 180.0)
#define M_TO_FT         3.28084

double anchor_geo_distance_m(geo_point_t a, geo_point_t b)
{
    double mid_lat = ((a.lat + b.lat) * 0.5) * DEG_TO_RAD;
    double dlat = (b.lat - a.lat) * DEG_TO_RAD;
    double dlon = (b.lon - a.lon) * DEG_TO_RAD * cos(mid_lat);
    return EARTH_RADIUS_M * sqrt(dlat * dlat + dlon * dlon);
}

double anchor_geo_distance_ft(geo_point_t a, geo_point_t b)
{
    return anchor_geo_distance_m(a, b) * M_TO_FT;
}

bool anchor_geo_centroid(const geo_point_t *samples, size_t n,
                          geo_point_t *out)
{
    if (!samples || !out || n == 0) return false;
    double slat = 0, slon = 0;
    for (size_t i = 0; i < n; i++) {
        slat += samples[i].lat;
        slon += samples[i].lon;
    }
    out->lat = slat / (double) n;
    out->lon = slon / (double) n;
    return true;
}

double anchor_geo_max_radius_m(const geo_point_t *samples, size_t n,
                                geo_point_t centre)
{
    double max_d = 0;
    for (size_t i = 0; i < n; i++) {
        double d = anchor_geo_distance_m(centre, samples[i]);
        if (d > max_d) max_d = d;
    }
    return max_d;
}

/* ---- Sample buffer ---- */

void anchor_geo_buf_reset(anchor_geo_buf_t *b)
{
    if (!b) return;
    b->count = 0;
    b->head  = 0;
}

void anchor_geo_buf_push(anchor_geo_buf_t *b, geo_point_t p)
{
    if (!b) return;
    b->samples[b->head] = p;
    b->head = (b->head + 1) % ANCHOR_GEO_BUF_MAX;
    if (b->count < ANCHOR_GEO_BUF_MAX) b->count++;
}

size_t anchor_geo_buf_size(const anchor_geo_buf_t *b)
{
    return b ? b->count : 0;
}

size_t anchor_geo_buf_snapshot(const anchor_geo_buf_t *b,
                                geo_point_t *out, size_t max_out)
{
    if (!b || !out || max_out == 0 || b->count == 0) return 0;
    size_t n = (b->count < max_out) ? b->count : max_out;
    /* Walk oldest → newest in the ring. When count < MAX, the oldest
     * sample is at index 0; when full, the oldest is at head. */
    size_t start = (b->count < ANCHOR_GEO_BUF_MAX) ? 0 : b->head;
    for (size_t i = 0; i < n; i++) {
        out[i] = b->samples[(start + i) % ANCHOR_GEO_BUF_MAX];
    }
    return n;
}
