/**
 * UTC Timezone Offsets - Minimal Version
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2025-12-26
 * Date Updated: 2025-12-26
 * 
 * All valid UTC offsets worldwide (39 total)
 * Offset stored as 15-minute increments from UTC
 */

#ifndef UTC_OFFSETS_MINIMAL_H
#define UTC_OFFSETS_MINIMAL_H

#include <stdint.h>

#define TZ_COUNT 39

typedef struct {
    const char* name;
    int8_t offset_15m;  // Multiply by 15 for minutes from UTC
} TZEntry;

const TZEntry timezones[TZ_COUNT] = {
    {"UTC-12:00", -48},
    {"UTC-11:00", -44},
    {"UTC-10:00", -40},
    {"UTC-09:30", -38},
    {"UTC-09:00", -36},
    {"UTC-08:00", -32},
    {"UTC-07:30", -30},
    {"UTC-07:00", -28},
    {"UTC-06:00", -24},
    {"UTC-05:00", -20},
    {"UTC-04:30", -18},
    {"UTC-04:00", -16},
    {"UTC-03:30", -14},
    {"UTC-03:00", -12},
    {"UTC-02:00",  -8},
    {"UTC-01:00",  -4},
    {"UTC+00:00",   0},
    {"UTC+01:00",   4},
    {"UTC+02:00",   8},
    {"UTC+03:00",  12},
    {"UTC+03:30",  14},
    {"UTC+04:00",  16},
    {"UTC+04:30",  18},
    {"UTC+05:00",  20},
    {"UTC+05:30",  22},
    {"UTC+05:45",  23},
    {"UTC+06:00",  24},
    {"UTC+06:30",  26},
    {"UTC+07:00",  28},
    {"UTC+08:00",  32},
    {"UTC+08:45",  35},
    {"UTC+09:00",  36},
    {"UTC+09:30",  38},
    {"UTC+10:00",  40},
    {"UTC+10:30",  42},
    {"UTC+11:00",  44},
    {"UTC+12:00",  48},
    {"UTC+12:45",  51},
    {"UTC+13:00",  52},
    {"UTC+14:00",  56}
};

// Helper: Convert offset_15m to display string
// buf must be at least 10 chars
void formatUTCOffset(int8_t off15, char* buf) {
    int totalMins = off15 * 15;
    int hours = totalMins / 60;
    int mins = (totalMins < 0 ? -totalMins : totalMins) % 60;
    sprintf(buf, "UTC%+03d:%02d", hours, mins);
}

// Helper: Get minutes from UTC
int16_t getOffsetMinutes(int8_t off15) {
    return (int16_t)off15 * 15;
}

#endif // UTC_OFFSETS_MINIMAL_H
