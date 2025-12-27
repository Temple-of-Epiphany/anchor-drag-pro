/**
 * UTC Timezone Offsets - With Locations
 * Author: Colin Bitterfield
 * Email: colin@bitterfield.com
 * Date Created: 2025-12-26
 * Date Updated: 2025-12-26
 * 
 * All valid UTC offsets worldwide (39 total)
 * Includes representative locations for user reference
 * Offset stored as 15-minute increments from UTC
 */

#ifndef UTC_OFFSETS_LOCATIONS_H
#define UTC_OFFSETS_LOCATIONS_H

#include <stdint.h>

#define TZ_COUNT 39

typedef struct {
    const char* offset_name;
    const char* locations;
    int8_t offset_15m;  // Multiply by 15 for minutes from UTC
} TZEntryLoc;

const TZEntryLoc timezones[TZ_COUNT] = {
    {"UTC-12:00", "Baker Island, Howland Island",                    -48},
    {"UTC-11:00", "American Samoa, Niue",                            -44},
    {"UTC-10:00", "Hawaii, Cook Islands, Tahiti",                    -40},
    {"UTC-09:30", "Marquesas Islands",                               -38},
    {"UTC-09:00", "Alaska, Gambier Islands",                         -36},
    {"UTC-08:00", "US Pacific, Baja California",                     -32},
    {"UTC-07:30", "Historical - no current use",                     -30},
    {"UTC-07:00", "US Mountain, Arizona, Yukon",                     -28},
    {"UTC-06:00", "US Central, Mexico City, Guatemala",              -24},
    {"UTC-05:00", "US Eastern, Peru, Colombia, Cuba",                -20},
    {"UTC-04:30", "Venezuela",                                       -18},
    {"UTC-04:00", "Atlantic Canada, Caribbean, Chile",               -16},
    {"UTC-03:30", "Newfoundland",                                    -14},
    {"UTC-03:00", "Argentina, Brazil East, Greenland",               -12},
    {"UTC-02:00", "Mid-Atlantic, South Georgia",                      -8},
    {"UTC-01:00", "Azores, Cape Verde",                               -4},
    {"UTC+00:00", "UK, Portugal, Iceland, Ghana",                      0},
    {"UTC+01:00", "Central Europe, West Africa, Algeria",              4},
    {"UTC+02:00", "Eastern Europe, Egypt, South Africa",               8},
    {"UTC+03:00", "Moscow, Saudi Arabia, East Africa",                12},
    {"UTC+03:30", "Iran",                                             14},
    {"UTC+04:00", "UAE, Oman, Mauritius, Seychelles",                 16},
    {"UTC+04:30", "Afghanistan",                                      18},
    {"UTC+05:00", "Pakistan, Maldives, Uzbekistan",                   20},
    {"UTC+05:30", "India, Sri Lanka",                                 22},
    {"UTC+05:45", "Nepal",                                            23},
    {"UTC+06:00", "Bangladesh, Bhutan, Kazakhstan",                   24},
    {"UTC+06:30", "Myanmar, Cocos Islands",                           26},
    {"UTC+07:00", "Thailand, Vietnam, Indonesia West",                28},
    {"UTC+08:00", "China, Singapore, Philippines, Perth",             32},
    {"UTC+08:45", "Australia Central Western",                        35},
    {"UTC+09:00", "Japan, Korea, Indonesia East",                     36},
    {"UTC+09:30", "Australia Central (Darwin, Adelaide)",             38},
    {"UTC+10:00", "Australia East, Papua New Guinea, Guam",           40},
    {"UTC+10:30", "Lord Howe Island",                                 42},
    {"UTC+11:00", "Solomon Islands, New Caledonia, Vanuatu",          44},
    {"UTC+12:00", "New Zealand, Fiji, Marshall Islands",              48},
    {"UTC+12:45", "Chatham Islands",                                  51},
    {"UTC+13:00", "Tonga, Phoenix Islands, Samoa",                    52},
    {"UTC+14:00", "Line Islands (Kiritimati)",                        56}
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

// Helper: Find timezone by offset_15m value, returns index or -1
int8_t findTimezoneByOffset(int8_t off15) {
    for (int8_t i = 0; i < TZ_COUNT; i++) {
        if (timezones[i].offset_15m == off15) {
            return i;
        }
    }
    return -1;
}

#endif // UTC_OFFSETS_LOCATIONS_H
