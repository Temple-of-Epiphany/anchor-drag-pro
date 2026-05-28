/*
 * Anchor geometry — distance + centroid + sample buffer.
 *
 * Pure C, no I/O. Lives here for v0.2; moves to core/ when Workstream
 * 1 (portable C library extraction, #50) lands.
 *
 * Distance helpers use the equirectangular projection — accurate at
 * the scales an anchor swings (≤ a few hundred metres), much cheaper
 * than haversine. Anchor centroid is the simple arithmetic mean of
 * the collected samples; the swing radius is the max distance from
 * centroid to any sample plus a user-configurable buffer.
 *
 * Author:  Colin Bitterfield <colin@bitterfield.com>
 * License: Proprietary
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    double lat;
    double lon;
} geo_point_t;

/* Approximate distance in metres between two lat/lon points using the
 * equirectangular projection at the average latitude. Accurate to
 * better than 0.1% for distances < 1 km at temperate latitudes. */
double anchor_geo_distance_m(geo_point_t a, geo_point_t b);

/* Same, but in feet (sailing customers often prefer feet for anchor
 * swing). */
double anchor_geo_distance_ft(geo_point_t a, geo_point_t b);

/* Compute the arithmetic centroid of `n` samples. Returns false if
 * n == 0. */
bool   anchor_geo_centroid(const geo_point_t *samples, size_t n,
                            geo_point_t *out);

/* Max distance from `centre` to any of the `n` samples (in metres). */
double anchor_geo_max_radius_m(const geo_point_t *samples, size_t n,
                                geo_point_t centre);

/* Signed local-tangent-plane offset from `from` to `to`, in metres.
 * Positive `dx_east_m` means `to` is east of `from`; positive
 * `dy_north_m` means `to` is north of `from`. Uses the same
 * equirectangular projection as anchor_geo_distance_m. */
void anchor_geo_offset_m(geo_point_t from, geo_point_t to,
                          double *dx_east_m, double *dy_north_m);

/* ---- Bounded sample ring buffer ----
 *
 * The state-machine collects up to ANCHOR_GEO_BUF_MAX samples during
 * ARMING; older samples slide out the back. */

#define ANCHOR_GEO_BUF_MAX 256

typedef struct {
    geo_point_t  samples[ANCHOR_GEO_BUF_MAX];
    size_t       count;     /* number of valid samples (≤ MAX) */
    size_t       head;      /* index where the next sample writes */
} anchor_geo_buf_t;

void  anchor_geo_buf_reset(anchor_geo_buf_t *b);
void  anchor_geo_buf_push (anchor_geo_buf_t *b, geo_point_t p);
size_t anchor_geo_buf_size(const anchor_geo_buf_t *b);

/* Snapshot — copies up to `max_out` samples to `out` in oldest-first
 * order. Returns the number copied. */
size_t anchor_geo_buf_snapshot(const anchor_geo_buf_t *b,
                                geo_point_t *out, size_t max_out);

#ifdef __cplusplus
}
#endif
