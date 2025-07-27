/*
 * Copyright 2025 Emiliano Augusto Gonzalez (egonzalez . hiperion @ gmail . com))
 * * Project Site: https://github.com/hiperiondev/HamRadioLib *
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stddef.h>
#include <ctype.h>
#include <limits.h>

#include "common.h"
#include "aprs.h"

// Constants for Base91 compression
#define BASE91_SIZE 91
#define LAT_SCALE 380926.0    // 91^4 / 2 for latitude scaling
#define LON_SCALE 190463.0    // 91^4 / 4 for longitude scaling
#define ALTITUDE_OFFSET 10000 // Offset for altitude encoding

// Base91 character set for APRS compression
static const char BASE91_CHARSET[] = "!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";

/**
 * Custom strndup implementation for C99.
 * @param s String to duplicate
 * @param n Maximum number of characters to copy
 * @return Pointer to duplicated string or NULL on failure
 */
static char* my_strndup(const char *s, size_t n) {
    size_t len;
    for (len = 0; len < n && s[len] != '\0'; len++)
        ;
    char *dup = malloc(len + 1);
    if (dup) {
        memcpy(dup, s, len);
        dup[len] = '\0';
    }
    return dup;
}

double aprs_parse_lat(const char *str, int *ambiguity) {
    if (strlen(str) != 8) {
        return NAN;
    }
    char deg_str[3] = { str[0], str[1], '\0' };
    char min_str[3] = { str[2], str[3], '\0' };
    char frac_str[3] = { str[5], str[6], '\0' };
    char dir = str[7];

    if (str[4] != '.') {
        return NAN;
    }

    // Determine ambiguity level
    *ambiguity = 0;
    if (min_str[0] == ' ' && min_str[1] == ' ' && frac_str[0] == ' ' && frac_str[1] == ' ') {
        *ambiguity = 4;
    } else if (frac_str[0] == ' ' && frac_str[1] == ' ') {
        *ambiguity = 3;
    } else if (frac_str[1] == ' ') {
        *ambiguity = 2;
    } else if (min_str[1] == ' ') {
        *ambiguity = 1;
    }

    // Parse degrees
    double degrees = atof(deg_str);

    // Parse minutes
    double minutes;
    if (*ambiguity == 4) {
        minutes = 30.0;  // Center of 0-59 minutes
    } else if (*ambiguity == 3) {
        if (min_str[0] == ' ') {
            return NAN;  // Invalid: tens of minutes should not be space for ambiguity 3
        }
        char tens = min_str[0];
        if (!isdigit(tens)) {
            return NAN;
        }
        minutes = (tens - '0') * 10 + 5;  // Center of units digit
    } else {
        // Replace spaces with '0' for parsing
        for (int i = 0; i < 2; i++) {
            if (min_str[i] == ' ')
                min_str[i] = '0';
        }
        minutes = atof(min_str);
    }

    // Parse hundredths
    double frac_min = 0.0;
    if (*ambiguity <= 1) {
        // Replace spaces with '0'
        for (int i = 0; i < 2; i++) {
            if (frac_str[i] == ' ')
                frac_str[i] = '0';
        }
        frac_min = atof(frac_str) / 100.0;
    } else if (*ambiguity == 2) {
        frac_min = 0.5;  // Center of hundredths
    }

    double lat = degrees + (minutes + frac_min) / 60.0;
    if (dir == 'S') {
        lat = -lat;
    } else if (dir != 'N') {
        return NAN;
    }
    return lat;
}

double aprs_parse_lon(const char *str, int *ambiguity) {
    if (strlen(str) != 9) {
        return NAN;
    }
    char deg_str[4] = { str[0], str[1], str[2], '\0' };
    char min_str[3] = { str[3], str[4], '\0' };
    char frac_str[3] = { str[6], str[7], '\0' };
    char dir = str[8];

    if (str[5] != '.') {
        return NAN;
    }

    // Determine ambiguity level
    *ambiguity = 0;
    if (min_str[0] == ' ' && min_str[1] == ' ' && frac_str[0] == ' ' && frac_str[1] == ' ') {
        *ambiguity = 4;
    } else if (frac_str[0] == ' ' && frac_str[1] == ' ') {
        *ambiguity = 3;
    } else if (frac_str[1] == ' ') {
        *ambiguity = 2;
    } else if (min_str[1] == ' ') {
        *ambiguity = 1;
    }

    // Parse degrees
    double degrees = atof(deg_str);

    // Parse minutes
    double minutes;
    if (*ambiguity == 4) {
        minutes = 30.0;  // Center of 0-59 minutes
    } else if (*ambiguity == 3) {
        if (min_str[0] == ' ') {
            return NAN;  // Invalid: tens of minutes should not be space for ambiguity 3
        }
        char tens = min_str[0];
        if (!isdigit(tens)) {
            return NAN;
        }
        minutes = (tens - '0') * 10 + 5;  // Center of units digit
    } else {
        // Replace spaces with '0' for parsing
        for (int i = 0; i < 2; i++) {
            if (min_str[i] == ' ')
                min_str[i] = '0';
        }
        minutes = atof(min_str);
    }

    // Parse hundredths
    double frac_min = 0.0;
    if (*ambiguity <= 1) {
        // Replace spaces with '0'
        for (int i = 0; i < 2; i++) {
            if (frac_str[i] == ' ')
                frac_str[i] = '0';
        }
        frac_min = atof(frac_str) / 100.0;
    } else if (*ambiguity == 2) {
        frac_min = 0.5;  // Center of hundredths
    }

    double lon = degrees + (minutes + frac_min) / 60.0;
    if (dir == 'W') {
        lon = -lon;
    } else if (dir != 'E') {
        return NAN;
    }
    return lon;
}

char* lat_to_aprs(double lat, int ambiguity) {
    static char buf[9];  // DDMM.hhN + null terminator
    if (lat < -90 || lat > 90 || ambiguity < 0 || ambiguity > 4) {
        return NULL;
    }
    char dir = (lat >= 0) ? 'N' : 'S';
    lat = fabs(lat);
    int deg = (int) lat;
    double min = (lat - deg) * 60;
    int min_int = (int) min;
    int min_frac = (int) ((min - min_int) * 100);
    sprintf(buf, "%02d%02d.%02d%c", deg, min_int, min_frac, dir);

    if (ambiguity > 0) {
        // Corrected digit positions: 0,1 (deg), 2,3 (min), 5,6 (hundredths)
        int digit_positions[] = { 5, 6, 3, 2 };  // From right to left: hundredths, minutes
        for (int i = 0; i < ambiguity && i < 4; i++) {
            buf[digit_positions[i]] = ' ';
        }
    }
    return buf;
}

char* lon_to_aprs(double lon, int ambiguity) {
    static char buf[10];  // DDDMM.hhE/W + null terminator
    if (lon < -180 || lon > 180 || ambiguity < 0 || ambiguity > 4) {
        return NULL;
    }
    char dir = (lon >= 0) ? 'E' : 'W';
    lon = fabs(lon);
    int deg = (int) lon;
    double min = (lon - deg) * 60;
    int min_int = (int) min;
    int min_frac = (int) ((min - min_int) * 100);
    sprintf(buf, "%03d%02d.%02d%c", deg, min_int, min_frac, dir);

    if (ambiguity > 0) {
        // Corrected digit positions: 6,7 (hundredths), 4,3 (minutes)
        int digit_positions[] = { 6, 7, 4, 3 };  // From right to left
        for (int i = 0; i < ambiguity && i < 4; i++) {
            buf[digit_positions[i]] = ' ';
        }
    }
    return buf;
}

int aprs_encode_message(char *info, size_t len, const aprs_message_t *data) {
    // Validate addressee length and null termination
    bool null_found = false;
    for (int i = 0; i < 9; i++) {
        if (data->addressee[i] == '\0') {
            null_found = true;
            break;
        }
    }
    if (!null_found && data->addressee[9] != '\0') {
        return -1; // Addressee exceeds 9 characters or not null-terminated
    }

    // Ensure addressee is null-terminated
    char addressee[10];
    strncpy(addressee, data->addressee, 9);
    addressee[9] = '\0';

    // Check message length
    if (data->message && strlen(data->message) > 67) {
        return -1;
    }

    // Check message_number length
    if (data->message_number && strlen(data->message_number) > 5) {
        return -1;
    }

    // Encode the message
    int ret = snprintf(info, len, ":%-9s:%s", addressee, data->message ? data->message : "");
    if (ret < 0 || (size_t) ret >= len) {
        return -1;
    }

    if (data->message_number) {
        int ret2 = snprintf(info + ret, len - ret, "{%s}", data->message_number);
        if (ret2 < 0 || (size_t) ret2 >= len - ret) {
            return -1;
        }
        ret += ret2;
    }

    return ret;
}

int aprs_encode_position_no_ts(char *info, size_t len, const aprs_position_no_ts_t *data) {
    // Validate inputs
    char dti = data->dti;
    if (dti == '\0') {
        dti = '!'; // Default to '!' if DTI is uninitialized
    } else if (dti != '!' && dti != '=') {
        printf("Error: Invalid DTI '%c'\n", dti);
        return -1;
    }
    if (data->symbol_table != '/' && data->symbol_table != '\\') {
        printf("Error: Invalid symbol table '%c'\n", data->symbol_table);
        return -1;
    }
    if (!isprint(data->symbol_code)) {
        printf("Error: Invalid symbol code '%c'\n", data->symbol_code);
        return -1;
    }
    if (fabs(data->latitude) > 90.0f) {
        printf("Error: Invalid latitude '%f'\n", data->latitude);
        return -1;
    }
    if (fabs(data->longitude) > 180.0f) {
        printf("Error: Invalid longitude '%f'\n", data->longitude);
        return -1;
    }

    // Handle course and speed
    int course = 0;
    if (data->has_course_speed) {
        course = (data->course % 360 + 360) % 360; // Normalize course to 0-359
        if (data->speed < 0) {
            printf("Error: Invalid speed '%d'\n", data->speed);
            return -1;
        }
    }

    // Encode latitude and longitude with fixed lengths
    char *lat_tmp = lat_to_aprs(data->latitude, data->ambiguity);
    char *lon_tmp = lon_to_aprs(data->longitude, data->ambiguity);
    if (lat_tmp == NULL || lon_tmp == NULL) {
        printf("Error: Failed to convert latitude or longitude to APRS format\n");
        return -1;
    }

    // Ensure latitude is 8 chars, longitude is 9 chars
    char latitude[9];  // 8 + null
    char longitude[10]; // 9 + null
    memset(latitude, ' ', 8); // Fill with spaces
    memset(longitude, ' ', 9);
    latitude[8] = '\0';
    longitude[9] = '\0';

    // Copy lat_tmp, ensuring exactly 8 characters
    size_t lat_len = my_strnlen(lat_tmp, 8);
    memcpy(latitude, lat_tmp, lat_len);
    for (size_t i = lat_len; i < 8; i++) {
        latitude[i] = ' ';
    }

    // Copy lon_tmp, ensuring exactly 9 characters
    size_t lon_len = my_strnlen(lon_tmp, 9);
    memcpy(longitude, lon_tmp, lon_len);
    for (size_t i = lon_len; i < 9; i++) {
        longitude[i] = ' ';
    }

    // Encode base position string (20 chars: 1+8+1+9+1)
    int ret = snprintf(info, len, "%c%s%c%s%c", dti, latitude, data->symbol_table, longitude, data->symbol_code);
    if (ret != 20 || (size_t) ret >= len) {
        printf("Error: Buffer too small or overflow for base position string\n");
        return -1;
    }

    // Add course and speed if present
    if (data->has_course_speed) {
        int speed = data->speed;
        int ret2 = snprintf(info + ret, len - ret, "%03d/%03d", course, speed);
        if (ret2 < 0 || (size_t) ret2 >= len - ret) {
            printf("Error: Buffer overflow for course/speed\n");
            return -1;
        }
        ret += ret2;
    }

    // Add comment if present
    if (data->comment && strlen(data->comment) > 0) {
        int ret2 = snprintf(info + ret, len - ret, "%s", data->comment);
        if (ret2 < 0 || (size_t) ret2 >= len - ret) {
            printf("Error: Buffer overflow for comment\n");
            return -1;
        }
        ret += ret2;
    }

    return ret; // Return length of encoded string
}

int aprs_decode_position_no_ts(const char *info, aprs_position_no_ts_t *data) {
    if (!info || !data || strlen(info) < 19) {
        fprintf(stderr, "Error: Invalid input or insufficient length (%zu)\n", info ? strlen(info) : 0);
        return -1;
    }
    memset(data, 0, sizeof(aprs_position_no_ts_t));
    if (info[0] != '!' && info[0] != '=') {
        fprintf(stderr, "Error: Invalid DTI '%c'\n", info[0]);
        return -1;
    }
    data->dti = info[0];
    char lat_str[9];
    strncpy(lat_str, info + 1, 8);
    lat_str[8] = '\0';
    // Handle ambiguity for latitude
    if (lat_str[2] == ' ' && lat_str[3] == ' ') {
        lat_str[2] = '3';
        lat_str[3] = '0';
    } else if (lat_str[3] == ' ') {
        lat_str[3] = '5';
    } else if (lat_str[2] == ' ') {
        lat_str[2] = '2';
    }
    if (lat_str[5] == ' ' && lat_str[6] == ' ') {
        lat_str[5] = '0';
        lat_str[6] = '0';
    } else if (lat_str[6] == ' ') {
        lat_str[6] = '5';
    } else if (lat_str[5] == ' ') {
        lat_str[5] = '5';
    }
    // Now validate and parse latitude
    if (!isdigit(lat_str[0]) || !isdigit(lat_str[1]) || !isdigit(lat_str[2]) || !isdigit(lat_str[3]) || lat_str[4] != '.' || !isdigit(lat_str[5])
            || !isdigit(lat_str[6]) || (lat_str[7] != 'N' && lat_str[7] != 'S')) {
        fprintf(stderr, "Error: Invalid latitude format '%s'\n", lat_str);
        return -1;
    }
    char deg_str[3] = { lat_str[0], lat_str[1], '\0' };
    char min_str[6] = { lat_str[2], lat_str[3], lat_str[4], lat_str[5], lat_str[6], '\0' };
    int lat_deg = atoi(deg_str);
    double lat_min = atof(min_str);
    data->latitude = lat_deg + lat_min / 60.0;
    if (lat_str[7] == 'S') {
        data->latitude = -data->latitude;
    }
    data->symbol_table = info[9];
    if (data->symbol_table != '/' && data->symbol_table != '\\') {
        fprintf(stderr, "Error: Invalid symbol table '%c'\n", data->symbol_table);
        return -1;
    }
    char lon_str[10];
    strncpy(lon_str, info + 10, 9);
    lon_str[9] = '\0';
    // Handle ambiguity for longitude
    if (lon_str[3] == ' ' && lon_str[4] == ' ') {
        lon_str[3] = '3';
        lon_str[4] = '0';
    } else if (lon_str[4] == ' ') {
        lon_str[4] = '5';
    } else if (lon_str[3] == ' ') {
        lon_str[3] = '2';
    }
    if (lon_str[6] == ' ' && lon_str[7] == ' ') {
        lon_str[6] = '0';
        lon_str[7] = '0';
    } else if (lon_str[7] == ' ') {
        lon_str[7] = '5';
    } else if (lon_str[6] == ' ') {
        lon_str[6] = '5';
    }
    // Now validate and parse longitude
    if (!isdigit(lon_str[0]) || !isdigit(lon_str[1]) || !isdigit(lon_str[2]) || !isdigit(lon_str[3]) || !isdigit(lon_str[4]) || lon_str[5] != '.'
            || !isdigit(lon_str[6]) || !isdigit(lon_str[7]) || (lon_str[8] != 'E' && lon_str[8] != 'W')) {
        fprintf(stderr, "Error: Invalid longitude format '%s'\n", lon_str);
        return -1;
    }
    char lon_deg_str[4] = { lon_str[0], lon_str[1], lon_str[2], '\0' };
    char lon_min_str[6] = { lon_str[3], lon_str[4], lon_str[5], lon_str[6], lon_str[7], '\0' };
    int lon_deg = atoi(lon_deg_str);
    double lon_min = atof(lon_min_str);
    data->longitude = lon_deg + lon_min / 60.0;
    if (lon_str[8] == 'W') {
        data->longitude = -data->longitude;
    }
    data->symbol_code = info[19];
    if (!isprint(data->symbol_code)) {
        fprintf(stderr, "Error: Invalid symbol code '%c'\n", data->symbol_code);
        return -1;
    }
    const char *p = info + 20;
    if (strlen(p) >= 7 && p[3] == '/') {
        char course_str[4], speed_str[4];
        strncpy(course_str, p, 3);
        course_str[3] = '\0';
        strncpy(speed_str, p + 4, 3);
        speed_str[3] = '\0';
        // Validate course
        for (int i = 0; i < 3; i++) {
            if (!isdigit(course_str[i])) {
                fprintf(stderr, "Error: Invalid course format '%s'\n", course_str);
                return -1;
            }
        }
        int course = atoi(course_str);
        if (course < 0 || course > 359) {
            fprintf(stderr, "Error: Course '%d' out of range\n", course);
            return -1;
        }
        // Validate speed
        int valid_speed = 1;
        for (int i = 0; i < 3; i++) {
            if (!isdigit(speed_str[i])) {
                valid_speed = 0;
                break;
            }
        }
        int speed = valid_speed ? atoi(speed_str) : -1;
        if (speed < 0) {
            valid_speed = 0;
        }
        if (valid_speed) {
            data->course = course;
            data->speed = speed;
            data->has_course_speed = 1;
            p += 7;
        } else {
            data->has_course_speed = 0;
        }
    } else {
        data->has_course_speed = 0;
    }
    if (*p) {
        data->comment = my_strdup(p);
        if (!data->comment) {
            fprintf(stderr, "Error: Memory allocation failed for comment\n");
            return -1;
        }
    } else {
        data->comment = my_strdup("");
        if (!data->comment) {
            fprintf(stderr, "Error: Memory allocation failed for empty comment\n");
            return -1;
        }
    }
    return 0;
}

int aprs_decode_message(const char *info, aprs_message_t *data) {
    if (info[0] != ':')
        return -1;
    char addressee[10];
    int ret = sscanf(info + 1, "%9[^:]:", addressee);
    if (ret != 1)
        return -1;
    strncpy(data->addressee, addressee, 9);
    data->addressee[9] = '\0'; // Ensure null-termination
    const char *message_start = info + 11; // : + 9 chars + :
    const char *msg_num = strrchr(message_start, '{');
    size_t msg_len = msg_num ? (size_t) (msg_num - message_start) : strlen(message_start);
    if (msg_len > 67)
        return -1;
    data->message = my_strndup(message_start, msg_len);
    if (!data->message && msg_len > 0)
        return -1;
    if (msg_num) {
        const char *msg_num_end = strchr(msg_num + 1, '}');
        if (msg_num_end) {
            size_t num_len = (size_t) (msg_num_end - (msg_num + 1));
            data->message_number = my_strndup(msg_num + 1, num_len);
            if (!data->message_number || num_len > 5) {
                free(data->message_number);
                free(data->message);
                return -1;
            }
        } else {
            data->message_number = NULL;
        }
    } else {
        data->message_number = NULL;
    }
    return 0;
}

int aprs_encode_weather_report(char *info, size_t len, const aprs_weather_report_t *data) {
    if (!info || !data || len < 21) { // Minimum length for _DDHHMMz cDDD sDDD tDDD
        fprintf(stderr, "Error: Invalid input or insufficient buffer size (%zu)\n", len);
        return -1;
    }

    size_t ts_len = strlen(data->timestamp);
    // Validate timestamp based on format
    if (strcmp(data->timestamp_format, "DHM") == 0 && ts_len != 7) {
        fprintf(stderr, "Error: Invalid DHM timestamp length (%zu): '%s'\n", ts_len, data->timestamp);
        return -1;
    } else if (strcmp(data->timestamp_format, "HMS") == 0 && ts_len != 8) {
        fprintf(stderr, "Error: Invalid HMS timestamp length (%zu): '%s'\n", ts_len, data->timestamp);
        return -1;
    }

    int written = 0;
    // Start with weather symbol and timestamp
    written += snprintf(info + written, len - written, "_%s", data->timestamp);
    if (written >= len) {
        fprintf(stderr, "Error: Buffer overflow after timestamp\n");
        return -1;
    }

    // Wind direction (cDDD)
    if (data->wind_direction >= 0 && data->wind_direction <= 360) {
        written += snprintf(info + written, len - written, "c%03d", data->wind_direction % 360);
        if (written >= len) {
            fprintf(stderr, "Error: Buffer overflow after wind direction\n");
            return -1;
        }
    } else {
        fprintf(stderr, "Error: Invalid wind direction (%d)\n", data->wind_direction);
        return -1; // Required field invalid
    }

    // Wind speed (sDDD)
    if (data->wind_speed >= 0) {
        written += snprintf(info + written, len - written, "s%03d", data->wind_speed);
        if (written >= len) {
            fprintf(stderr, "Error: Buffer overflow after wind speed\n");
            return -1;
        }
    } else {
        fprintf(stderr, "Error: Invalid wind speed (%d)\n", data->wind_speed);
        return -1; // Required field invalid
    }

    // Temperature (tDDD or t-DD)
    if (data->temperature >= -99.9 && data->temperature <= 999.9) {
        int temp_f = (int) roundf(data->temperature);
        if (temp_f >= 0) {
            written += snprintf(info + written, len - written, "t%03d", temp_f);
        } else {
            written += snprintf(info + written, len - written, "t-%02d", -temp_f);
        }
        if (written >= len) {
            fprintf(stderr, "Error: Buffer overflow after temperature\n");
            return -1;
        }
    } else {
        fprintf(stderr, "Error: Invalid temperature (%f)\n", data->temperature);
        return -1; // Required field invalid
    }

    // Optional fields: only include if valid
    if (data->wind_gust >= 0) {
        written += snprintf(info + written, len - written, "g%03d", data->wind_gust);
        if (written >= len) {
            fprintf(stderr, "Error: Buffer overflow after wind gust\n");
            return -1;
        }
    }
    if (data->rainfall_last_hour >= 0) {
        written += snprintf(info + written, len - written, "r%03d", data->rainfall_last_hour);
        if (written >= len) {
            fprintf(stderr, "Error: Buffer overflow after rainfall last hour\n");
            return -1;
        }
    }
    if (data->rainfall_24h >= 0) {
        written += snprintf(info + written, len - written, "p%03d", data->rainfall_24h);
        if (written >= len) {
            fprintf(stderr, "Error: Buffer overflow after rainfall 24h\n");
            return -1;
        }
    }
    if (data->rainfall_since_midnight >= 0) {
        written += snprintf(info + written, len - written, "P%03d", data->rainfall_since_midnight);
        if (written >= len) {
            fprintf(stderr, "Error: Buffer overflow after rainfall since midnight\n");
            return -1;
        }
    }
    if (data->humidity >= 0 && data->humidity <= 100) {
        written += snprintf(info + written, len - written, "h%02d", data->humidity);
        if (written >= len) {
            fprintf(stderr, "Error: Buffer overflow after humidity\n");
            return -1;
        }
    }
    if (data->barometric_pressure >= 0) {
        written += snprintf(info + written, len - written, "b%05d", data->barometric_pressure);
        if (written >= len) {
            fprintf(stderr, "Error: Buffer overflow after barometric pressure\n");
            return -1;
        }
    }
    if (data->luminosity >= 0) {
        if (data->luminosity < 1000) {
            written += snprintf(info + written, len - written, "L%03d", data->luminosity);
        } else {
            written += snprintf(info + written, len - written, "l%03d", data->luminosity - 1000);
        }
        if (written >= len) {
            fprintf(stderr, "Error: Buffer overflow after luminosity\n");
            return -1;
        }
    }
    if (data->snowfall_24h >= 0.0f) {
        int snow_int = (int) roundf(data->snowfall_24h * 10.0f);
        written += snprintf(info + written, len - written, "S%03d", snow_int);
        if (written >= len) {
            fprintf(stderr, "Error: Buffer overflow after snowfall\n");
            return -1;
        }
    }
    if (data->rain_rate >= 0) {
        written += snprintf(info + written, len - written, "R%03d", data->rain_rate);
        if (written >= len) {
            fprintf(stderr, "Error: Buffer overflow after rain rate\n");
            return -1;
        }
    }
    if (data->water_height_feet >= 0.0f) {
        written += snprintf(info + written, len - written, "F%.1f", data->water_height_feet);
        if (written >= len) {
            fprintf(stderr, "Error: Buffer overflow after water height feet\n");
            return -1;
        }
    }
    if (data->water_height_meters >= 0.0f) {
        written += snprintf(info + written, len - written, "f%.1f", data->water_height_meters);
        if (written >= len) {
            fprintf(stderr, "Error: Buffer overflow after water height meters\n");
            return -1;
        }
    }
    if (data->indoors_temperature >= -99.9f && data->indoors_temperature <= 999.9f) {
        int temp_f = (int) roundf(data->indoors_temperature);
        if (temp_f >= 0) {
            written += snprintf(info + written, len - written, "i%02d", temp_f);
        } else {
            written += snprintf(info + written, len - written, "i-%02d", -temp_f);
        }
        if (written >= len) {
            fprintf(stderr, "Error: Buffer overflow after indoors temperature\n");
            return -1;
        }
    }
    if (data->indoors_humidity >= 0 && data->indoors_humidity <= 100) {
        written += snprintf(info + written, len - written, "I%02d", data->indoors_humidity);
        if (written >= len) {
            fprintf(stderr, "Error: Buffer overflow after indoors humidity\n");
            return -1;
        }
    }
    if (data->raw_rain_counter >= 0) {
        written += snprintf(info + written, len - written, "#%05d", data->raw_rain_counter);
        if (written >= len) {
            fprintf(stderr, "Error: Buffer overflow after raw rain counter\n");
            return -1;
        }
    }

    return written;
}

int aprs_decode_weather_report(const char *info, aprs_weather_report_t *data) {
    if (info[0] != '_') {
        return -1;
    }
    const char *p = info + 1;

    // Parse timestamp
    if (strlen(p) < 8) {
        return -1;
    }
    strncpy(data->timestamp, p, 8);
    data->timestamp[8] = '\0';
    p += 8;

    // Determine timestamp format and zulu flag
    if (*p == 'z') {
        strcpy(data->timestamp_format, "HMS");
        data->is_zulu = 1;
        p++;
    } else if (*p == 'l') {
        strcpy(data->timestamp_format, "HMS");
        data->is_zulu = 0;
        p++;
    } else {
        strcpy(data->timestamp_format, "DHM");
        data->is_zulu = 1;
    }

    // Initialize data fields
    data->temperature = -999.9;
    data->wind_speed = -1;
    data->wind_direction = -1;
    data->wind_gust = -1;
    data->rainfall_last_hour = -1;
    data->rainfall_24h = -1;
    data->rainfall_since_midnight = -1;
    data->barometric_pressure = -1;
    data->humidity = -1;
    data->luminosity = -1;
    data->snowfall_24h = -999.9;
    data->rain_rate = -1;
    data->water_height_feet = -999.9;
    data->water_height_meters = -999.9;
    data->indoors_temperature = -999.9;
    data->indoors_humidity = -1;
    data->raw_rain_counter = -1;

    // Parse weather fields
    while (*p) {
        if (*p == 't') {
            p++;
            if (*p == '-') {
                p++;
                char buf[3];
                strncpy(buf, p, 2);
                buf[2] = '\0';
                p += 2;
                data->temperature = -atof(buf); // Temperature in Fahrenheit
            } else {
                char buf[4];
                strncpy(buf, p, 3);
                buf[3] = '\0';
                p += 3;
                data->temperature = atof(buf); // Temperature in Fahrenheit
            }
        } else if (*p == 's') {
            p++;
            char buf[4];
            strncpy(buf, p, 3);
            buf[3] = '\0';
            p += 3;
            data->wind_speed = atoi(buf);
        } else if (*p == 'c') {
            p++;
            char buf[4];
            strncpy(buf, p, 3);
            buf[3] = '\0';
            p += 3;
            data->wind_direction = atoi(buf);
        } else if (*p == 'g') {
            p++;
            char buf[4];
            strncpy(buf, p, 3);
            buf[3] = '\0';
            p += 3;
            data->wind_gust = atoi(buf);
        } else if (*p == 'r') {
            p++;
            char buf[4];
            strncpy(buf, p, 3);
            buf[3] = '\0';
            p += 3;
            data->rainfall_last_hour = atoi(buf);
        } else if (*p == 'p') {
            p++;
            char buf[4];
            strncpy(buf, p, 3);
            buf[3] = '\0';
            p += 3;
            data->rainfall_24h = atoi(buf);
        } else if (*p == 'P') {
            p++;
            char buf[4];
            strncpy(buf, p, 3);
            buf[3] = '\0';
            p += 3;
            data->rainfall_since_midnight = atoi(buf);
        } else if (*p == 'b') {
            p++;
            char buf[6];
            strncpy(buf, p, 5);
            buf[5] = '\0';
            p += 5;
            data->barometric_pressure = atoi(buf);
        } else if (*p == 'h') {
            p++;
            char buf[3];
            strncpy(buf, p, 2);
            buf[2] = '\0';
            p += 2;
            data->humidity = atoi(buf);
        } else if (*p == 'L' || *p == 'l') {
            char type = *p;
            p++;
            char buf[4];
            strncpy(buf, p, 3);
            buf[3] = '\0';
            p += 3;
            int lum = atoi(buf);
            if (type == 'L') {
                data->luminosity = lum;
            } else {
                data->luminosity = lum + 1000;
            }
        } else if (*p == 'S') {
            p++;
            char buf[4];
            strncpy(buf, p, 3);
            buf[3] = '\0';
            p += 3;
            data->snowfall_24h = atof(buf) / 10.0; // Assuming tenths of inches
        } else if (*p == 'R') {
            p++;
            char buf[4];
            strncpy(buf, p, 3);
            buf[3] = '\0';
            p += 3;
            data->rain_rate = atoi(buf);
        } else if (*p == 'F') {
            p++;
            char *end;
            data->water_height_feet = strtof(p, &end);
            p = end;
        } else if (*p == 'f') {
            p++;
            char *end;
            data->water_height_meters = strtof(p, &end);
            p = end;
        } else if (*p == 'i') {
            p++;
            if (*p == '-') {
                p++;
                char *end;
                data->indoors_temperature = -strtof(p, &end);
                p = end;
            } else {
                char *end;
                data->indoors_temperature = strtof(p, &end);
                p = end;
            }
        } else if (*p == 'I') {
            p++;
            char buf[3];
            strncpy(buf, p, 2);
            buf[2] = '\0';
            p += 2;
            data->indoors_humidity = atoi(buf);
        } else if (*p == '#') {
            p++;
            char buf[6];
            strncpy(buf, p, 5);
            buf[5] = '\0';
            p += 5;
            data->raw_rain_counter = atoi(buf);
        } else {
            p++;
        }
        while (*p && !isalpha(*p))
            p++;
    }
    return 0;
}

int aprs_encode_object_report(char *info, size_t len, const aprs_object_report_t *data) {
    if (data->symbol_table != '/' && data->symbol_table != '\\') {
        return -1; // Invalid symbol table
    }
    if (!isprint(data->symbol_code)) {
        return -1; // Invalid symbol code
    }
    char *lat_str = lat_to_aprs(data->latitude, 0);
    char *lon_str = lon_to_aprs(data->longitude, 0);
    if (!lat_str || !lon_str) {
        return -1;
    }

    int ret = snprintf(info, len, ";%-9s*%s%s%c%s%c", data->name, data->timestamp, lat_str, data->symbol_table, lon_str, data->symbol_code);
    if (ret < 0 || (size_t) ret >= len) {
        return -1;
    }

    return ret;
}

int aprs_decode_object_report(const char *info, aprs_object_report_t *data) {
    size_t info_len = strlen(info);
    if (info[0] != ';' || info_len < 37) {
        return -1;
    }

    // Extract name (positions 1-9)
    strncpy(data->name, info + 1, 9);
    data->name[9] = '\0';
    // Trim trailing spaces
    for (int i = 8; i >= 0; i--) {
        if (data->name[i] != ' ') {
            data->name[i + 1] = '\0';
            break;
        }
        if (i == 0) {
            data->name[0] = '\0';
        }
    }

    // Extract timestamp (positions 11-17)
    if (info[10] != '*') {
        return -1;
    }
    strncpy(data->timestamp, info + 11, 7);
    data->timestamp[7] = '\0';
    if (data->timestamp[6] != 'z') {
        return -1;
    }

    // Parse latitude (positions 18-25)
    char lat_str[9];
    strncpy(lat_str, info + 18, 8);
    lat_str[8] = '\0';
    int lat_ambiguity; // Dummy variable for ambiguity (not used in object reports)
    data->latitude = aprs_parse_lat(lat_str, &lat_ambiguity);
    if (isnan(data->latitude)) {
        return -1;
    }

    // Symbol table (position 26)
    data->symbol_table = info[26];

    // Parse longitude (positions 27-35)
    char lon_str[10];
    strncpy(lon_str, info + 27, 9);
    lon_str[9] = '\0';
    int lon_ambiguity; // Dummy variable for ambiguity (not used in object reports)
    data->longitude = aprs_parse_lon(lon_str, &lon_ambiguity);
    if (isnan(data->longitude)) {
        return -1;
    }

    // Symbol code (position 36)
    data->symbol_code = info[36];

    return 0;
}

int aprs_encode_position_with_ts(char *info, size_t len, const aprs_position_with_ts_t *data) {
    // Validate inputs
    if (data->dti != '/' && data->dti != '@') {
        printf("Error: Invalid DTI '%c'\n", data->dti);
        return -1;
    }
    if (strlen(data->timestamp) != 7 || (data->timestamp[6] != 'z' && data->timestamp[6] != 'l')) {
        printf("Error: Invalid timestamp '%s'\n", data->timestamp);
        return -1;
    }
    if (data->symbol_table != '/' && data->symbol_table != '\\') {
        printf("Error: Invalid symbol table '%c'\n", data->symbol_table);
        return -1;
    }
    if (!isprint(data->symbol_code)) {
        printf("Error: Invalid symbol code '%c'\n", data->symbol_code);
        return -1;
    }
    if (fabs(data->latitude) > 90.0) {
        printf("Error: Invalid latitude '%f'\n", data->latitude);
        return -1;
    }
    if (fabs(data->longitude) > 180.0) {
        printf("Error: Invalid longitude '%f'\n", data->longitude);
        return -1;
    }

    // Convert latitude to DDMM.MM{N|S}
    int lat_deg = (int) fabs(data->latitude);
    double lat_min = (fabs(data->latitude) - lat_deg) * 60.0;
    char lat_dir = data->latitude >= 0 ? 'N' : 'S';
    char lat_str[10];
    snprintf(lat_str, sizeof(lat_str), "%02d%05.2f%c", lat_deg, lat_min, lat_dir);

    // Convert longitude to DDDMM.MM{E|W}
    int lon_deg = (int) fabs(data->longitude);
    double lon_min = (fabs(data->longitude) - lon_deg) * 60.0;
    char lon_dir = data->longitude >= 0 ? 'E' : 'W';
    char lon_str[11];
    snprintf(lon_str, sizeof(lon_str), "%03d%05.2f%c", lon_deg, lon_min, lon_dir);

    // Encode the string
    int ret = snprintf(info, len, "%c%s%s%c%s%c", data->dti, data->timestamp, lat_str, data->symbol_table, lon_str, data->symbol_code);
    if (ret < 0 || (size_t) ret >= len) {
        printf("Error: Buffer too small (required %d, available %zu)\n", ret, len);
        return -1;
    }

    // Add comment if present
    if (data->comment) {
        int ret2 = snprintf(info + ret, len - ret, "%s", data->comment);
        if (ret2 < 0 || (size_t) ret2 >= len - ret) {
            printf("Error: Buffer overflow for comment\n");
            return -1;
        }
        ret += ret2;
    }

    return ret; // Return length of encoded string
}

int aprs_decode_position_with_ts(const char *info, aprs_position_with_ts_t *data) {
    if (!info || !data)
        return -1;

    // Check DTI and minimum length
    if (info[0] != '@' && info[0] != '/')
        return -1;
    if (strlen(info) < 26)
        return -1; // Minimum length for DTI+TS+LAT+TABLE+LON+CODE

    // Initialize data
    *data = (aprs_position_with_ts_t ) { 0 };

    // Extract DTI
    data->dti = info[0];

    // Extract timestamp (DDHHMMz)
    if (strlen(info) < 8) {
        return -1;
    }
    memcpy(data->timestamp, info + 1, 7);
    data->timestamp[7] = '\0'; // Null-terminate the timestamp
    if (data->timestamp[6] != 'z') {
        return -1;
    }

    // Extract latitude (8 chars, e.g., 4903.50N)
    char lat_str[9] = { 0 };
    strncpy(lat_str, info + 8, 8);
    if (lat_str[7] != 'N' && lat_str[7] != 'S')
        return -1;

    // Parse latitude with improved precision
    char deg_str[3] = { lat_str[0], lat_str[1], '\0' };
    char min_str[3] = { lat_str[2], lat_str[3], '\0' };
    char frac_str[3] = { lat_str[5], lat_str[6], '\0' };
    double deg = atof(deg_str);
    double min = atof(min_str);
    double frac = atof(frac_str) / 100.0;
    data->latitude = deg + (min + frac) / 60.0;
    if (lat_str[7] == 'S')
        data->latitude = -data->latitude;

    // Extract symbol table
    data->symbol_table = info[16];
    if (data->symbol_table != '/' && data->symbol_table != '\\')
        return -1;

    // Extract longitude (9 chars, e.g., 07201.75W)
    char lon_str[10] = { 0 };
    strncpy(lon_str, info + 17, 9);
    if (lon_str[8] != 'E' && lon_str[8] != 'W')
        return -1;

    // Parse longitude with improved precision
    char lon_deg_str[4] = { lon_str[0], lon_str[1], lon_str[2], '\0' };
    char lon_min_str[3] = { lon_str[3], lon_str[4], '\0' };
    char lon_frac_str[3] = { lon_str[6], lon_str[7], '\0' };
    double lon_deg = atof(lon_deg_str);
    double lon_min = atof(lon_min_str);
    double lon_frac = atof(lon_frac_str) / 100.0;
    data->longitude = lon_deg + (lon_min + lon_frac) / 60.0;
    if (lon_str[8] == 'W')
        data->longitude = -data->longitude;

    // Extract symbol code
    data->symbol_code = info[26];
    if (!isprint(data->symbol_code))
        return -1;

    // Extract course/speed if present (e.g., 123/456)
    data->has_course_speed = false;
    if (strlen(info) >= 34&& info[27] == '/' && isdigit(info[28]) && isdigit(info[29]) &&
    isdigit(info[30]) && info[31] == '/' && isdigit(info[32]) && isdigit(info[33]) && isdigit(info[34])) {
        char course_str[4] = { info[28], info[29], info[30], '\0' };
        char speed_str[4] = { info[32], info[33], info[34], '\0' };
        data->course = atoi(course_str);
        data->speed = atoi(speed_str);
        data->has_course_speed = true;
    }

    // Extract comment (if any)
    const char *comment_start = info + (data->has_course_speed ? 35 : 27);
    if (*comment_start) {
        data->comment = my_strdup(comment_start);
        if (!data->comment)
            return -1;
    } else {
        data->comment = NULL;
    }

    return 0;
}

bool aprs_validate_timestamp(const char *timestamp, bool zulu) {
    if (strlen(timestamp) != (zulu ? 7 : 8))
        return false;
    for (int i = 0; i < (zulu ? 6 : 8); i++) {
        if (!isdigit(timestamp[i]))
            return false;
    }
    if (zulu && timestamp[6] != 'z')
        return false;
    return true;
}

int aprs_parse_weather_field(const char *data, char field_id, char *value, size_t value_len) {
    const char *p = data;
    while (*p) {
        if (*p == field_id) {
            strncpy(value, p + 1, value_len - 1);
            value[value_len - 1] = '\0';
            return p - data + 4;
        }
        p++;
    }
    return -1;
}

int aprs_encode_mice_destination(char *dest_str, const aprs_mice_t *data) {
    // Compute latitude digits
    double lat = fabs(data->latitude);
    int deg = (int) lat;
    double min_frac = (lat - deg) * 60.0;
    int min = (int) min_frac;
    double hun = (min_frac - min) * 100.0;
    int hun_int = (int) (hun + 0.5); // Round to nearest

    // Extract digits: deg_tens, deg_units, min_tens, min_units, hun_tens, hun_units
    int digits[6] = { deg / 10, deg % 10, min / 10, min % 10, hun_int / 10, hun_int % 10 };

    // Determine message bits
    const char *message_bits;
    if (!strcmp(data->message_code, "M0")) {
        message_bits = "111";
    } else if (!strcmp(data->message_code, "M1")) {
        message_bits = "110";
    } else if (!strcmp(data->message_code, "M2")) {
        message_bits = "101";
    } else if (!strcmp(data->message_code, "M3")) {
        message_bits = "100";
    } else if (!strcmp(data->message_code, "M4")) {
        message_bits = "011";
    } else if (!strcmp(data->message_code, "M5")) {
        message_bits = "010";
    } else if (!strcmp(data->message_code, "M6")) {
        message_bits = "001";
    } else if (!strcmp(data->message_code, "C0")) {
        message_bits = "111";
    } else if (!strcmp(data->message_code, "C1")) {
        message_bits = "110";
    } else if (!strcmp(data->message_code, "C2")) {
        message_bits = "101";
    } else if (!strcmp(data->message_code, "C3")) {
        message_bits = "100";
    } else if (!strcmp(data->message_code, "C4")) {
        message_bits = "011";
    } else if (!strcmp(data->message_code, "C5")) {
        message_bits = "010";
    } else if (!strcmp(data->message_code, "C6")) {
        message_bits = "001";
    } else if (!strcmp(data->message_code, "Emergency")) {
        message_bits = "000";
    } else {
        return -1; // Invalid message code
    }

    // Determine additional bits: N/S, long_offset, W/E
    int bits[6];
    bits[0] = message_bits[0] - '0'; // Message bit A
    bits[1] = message_bits[1] - '0'; // Message bit B
    bits[2] = message_bits[2] - '0'; // Message bit C
    bits[3] = (data->latitude >= 0) ? 1 : 0; // N/S: 1 North, 0 South
    double abs_longitude = fabs(data->longitude);
    int long_deg = (int) abs_longitude;
    bits[4] = (long_deg >= 100) ? 1 : 0; // Long_offset: 1 if >=100
    bits[5] = (data->longitude < 0) ? 1 : 0; // W/E: 1 West, 0 East

    // Encode each byte
    for (int i = 0; i < 6; i++) {
        int digit = digits[i];
        if (digit < 0 || digit > 9) {
            return -1; // Invalid digit
        }
        int bit = bits[i];
        char c;
        if (i < 3) { // Positions 0-2: message bits
            c = bit ? 'P' + digit : '0' + digit;
        } else if (i == 3) { // Position 3: N/S
            c = bit ? 'P' + digit : 'A' + digit;
        } else if (i == 4) { // Position 4: long_offset
            c = bit ? 'P' + digit : '0' + digit;
        } else { // Position 5: W/E
            c = bit ? 'P' + digit : 'A' + digit;
        }
        dest_str[i] = c;
    }
    dest_str[6] = '\0'; // Null-terminate
    return 0;
}

int aprs_decode_mice_destination(const char *dest_str, aprs_mice_t *data, int *message_bits, bool *ns, bool *long_offset, bool *we) {
    if (strlen(dest_str) != 6) {
        return -1; // Invalid length
    }

    int digits[6];
    bool bits[6];

    // Decode each character
    for (int i = 0; i < 6; i++) {
        char c = dest_str[i];
        if (i == 3 || i == 5) { // N/S (pos 3) and W/E (pos 5)
            if (c >= 'A' && c <= 'J') {
                digits[i] = c - 'A';
                bits[i] = false;
            } else if (c >= 'P' && c <= 'Y') {
                digits[i] = c - 'P';
                bits[i] = true;
            } else {
                return -1; // Invalid character
            }
        } else { // Positions 0-2 (message bits) and 4 (long offset)
            if (c >= '0' && c <= '9') {
                digits[i] = c - '0';
                bits[i] = false;
            } else if (c >= 'P' && c <= 'Y') {
                digits[i] = c - 'P';
                bits[i] = true;
            } else {
                return -1; // Invalid character
            }
        }
    }

    // Extract message bits (ABC from positions 0-2)
    *message_bits = (bits[0] << 2) | (bits[1] << 1) | bits[2];
    *ns = bits[3];          // North/South: true=North, false=South
    *long_offset = bits[4]; // Longitude offset: true=add 100 degrees
    *we = bits[5];          // West/East: true=West, false=East

    // Compute latitude
    int deg = digits[0] * 10 + digits[1];
    double min = digits[2] * 10 + digits[3] + (digits[4] * 10.0 + digits[5]) / 100.0;
    data->latitude = deg + min / 60.0;
    if (!*ns) { // South
        data->latitude = -data->latitude;
    }

    return 0;
}

int aprs_encode_mice_info(char *info, size_t len, const aprs_mice_t *data) {
    if (len < 9) {
        return -1; // Not enough space
    }
    // Validate inputs
    if (data->speed < 0 || data->speed > 799 || data->course < 0 || data->course > 360) {
        return -1;
    }
    if (!isprint(data->symbol_code) || (data->symbol_table != '/' && data->symbol_table != '\\')) {
        return -1;
    }

    // Data type identifier: '`' for current GPS fix
    info[0] = '`';

    // Encode longitude
    double abs_longitude = fabs(data->longitude);
    int long_deg = (int) abs_longitude;
    double min_frac = (abs_longitude - long_deg) * 60.0;
    int min = (int) min_frac;
    double hun = (min_frac - min) * 100.0;
    int hun_int = (int) (hun + 0.5);

    // Adjust long_deg if >=100
    int offset = (long_deg >= 100) ? 1 : 0;
    if (offset) {
        long_deg -= 100;
    }

    // Encode degrees
    int d = long_deg;
    if (d < 0 || d > 179) {
        return -1;
    }
    int encoded_d = (d < 60) ? d + 28 : d + 88;
    info[1] = (char) encoded_d;

    // Encode minutes
    int m = min % 60;
    info[2] = (char) (m + 28);

    // Encode hundredths
    int h = hun_int % 100;
    info[3] = (char) (h + 28);

    // Encode speed and course
    int speed_knots = data->speed;
    int course_deg = data->course;
    int SP = speed_knots / 10;
    int DC = (speed_knots % 10) * 10 + (course_deg / 100);
    int SE = course_deg % 100;
    info[4] = (char) (SP + 28);
    info[5] = (char) (DC + 28);
    info[6] = (char) (SE + 28);

    // Symbol code and table
    info[7] = data->symbol_code;
    info[8] = data->symbol_table;

    return 9;
}

int aprs_decode_mice_info(const char *info, size_t len, aprs_mice_t *data, bool long_offset, bool we) {
    // Check if info field is long enough (minimum 9 bytes for Mic-E)
    if (len < 9) {
        return -1; // Invalid length
    }

    // Validate data type indicator (byte 0)
    char dti = info[0];
    if (dti != '`' && dti != '\'') {
        return -1; // Invalid data type
    }

    // Decode longitude from bytes 1-3
    int d = info[1] - 28; // Degrees
    if (d >= 88) {
        d -= 60; // Adjust for degrees >= 60
    }
    int m = info[2] - 28; // Minutes
    int h = info[3] - 28; // Hundredths of minutes

    // Validate ranges
    if (d < 0 || d > 179 || m < 0 || m > 59 || h < 0 || h > 99) {
        return -1; // Invalid values
    }

    // Apply longitude offset (from destination field)
    if (long_offset) {
        d += 100; // Add 100 if longitude >= 100°
    }

    // Correct longitude calculation
    double min = m + h / 100.0; // Combine minutes and hundredths
    data->longitude = d + min / 60.0; // Convert to degrees

    // Adjust for West longitude
    if (we) {
        data->longitude = -data->longitude;
    }

    // Decode speed and course (bytes 4-6)
    int sp = info[4] - 28;
    int dc = info[5] - 28;
    int se = info[6] - 28;
    data->speed = sp * 10 + (dc / 10);
    data->course = (dc % 10) * 100 + se;

    // Decode symbols (bytes 7-8)
    data->symbol_code = info[7];
    data->symbol_table = info[8];

    return 0; // Success
}

int aprs_encode_telemetry(char *info, size_t len, const aprs_telemetry_t *data) {
    if (len < 30) {
        return -1;
    }
    // Validate analog values
    for (int i = 0; i < 5; i++) {
        if (data->analog[i] < 0 || data->analog[i] > 255) {
            return -1;
        }
    }
    char bits_str[9];
    for (int i = 7; i >= 0; i--) {
        bits_str[7 - i] = ((data->digital >> i) & 1) ? '1' : '0';
    }
    bits_str[8] = '\0';
    int ret = snprintf(info, len, "T#%03u,%03u,%03u,%03u,%03u,%03u,%s", data->sequence_number % 1000, (unsigned int) data->analog[0],
            (unsigned int) data->analog[1], (unsigned int) data->analog[2], (unsigned int) data->analog[3], (unsigned int) data->analog[4], bits_str);
    if (ret < 0 || (size_t) ret >= len) {
        return -1;
    }
    return ret;
}

int aprs_decode_telemetry(const char *info, aprs_telemetry_t *data) {
    if (info[0] != 'T' || info[1] != '#') {
        return -1;
    }
    char *p = (char*) info + 2;
    char *end;
    data->sequence_number = strtoul(p, &end, 10);
    if (*end != ',') {
        return -1;
    }
    p = end + 1;
    for (int i = 0; i < 5; i++) {
        data->analog[i] = strtoul(p, &end, 10);
        if (*end != ',' && i < 4) {
            return -1;
        }
        p = end + 1;
    }
    char bits_str[9];
    strncpy(bits_str, p, 8);
    bits_str[8] = '\0';
    data->digital = strtoul(bits_str, NULL, 2);
    return 0;
}

int aprs_encode_status(char *info, size_t len, const aprs_status_t *data) {
    if (!info || !data || len < 1)
        return -1;
    size_t pos = 0;
    info[pos++] = '>';
    if (data->has_timestamp) {
        if (pos + 7 > len)
            return -1;
        memcpy(info + pos, data->timestamp, 7);
        pos += 7;
    }
    size_t text_len = strlen(data->status_text);
    if (text_len > 62)
        text_len = 62;
    if (pos + text_len > len)
        return -1;
    memcpy(info + pos, data->status_text, text_len);
    pos += text_len;
    if (pos < len)
        info[pos] = '\0'; // Null-terminate if space allows
    return pos;
}

int aprs_decode_status(const char *info, aprs_status_t *data) {
    if (!info || !data)
        return -1;
    if (info[0] != '>')
        return -1;
    size_t len = strlen(info);
    if (len < 1)
        return -1;
    size_t pos = 1;
    if (len >= 8 && isdigit(info[1]) && isdigit(info[2]) && isdigit(info[3]) && isdigit(info[4]) && isdigit(info[5]) && isdigit(info[6]) && info[7] == 'z') {
        data->has_timestamp = true;
        memcpy(data->timestamp, info + 1, 7);
        data->timestamp[7] = '\0';
        pos += 7;
    } else {
        data->has_timestamp = false;
        data->timestamp[0] = '\0';
    }
    size_t text_len = len - pos;
    if (text_len > 62)
        text_len = 62;
    memcpy(data->status_text, info + pos, text_len);
    data->status_text[text_len] = '\0';
    return 0;
}

int aprs_encode_general_query(char *info, size_t len, const aprs_general_query_t *data) {
    if (!info || !data || len < 3)
        return -1;
    size_t type_len = strlen(data->query_type);
    if (type_len == 0 || type_len > 10)
        return -1;
    size_t total_len = 2 + type_len;
    if (total_len > len)
        return -1;
    info[0] = '?';
    memcpy(info + 1, data->query_type, type_len);
    info[1 + type_len] = '?';
    if (total_len < len)
        info[total_len] = '\0'; // Null-terminate if space allows
    return total_len;
}

int aprs_decode_general_query(const char *info, aprs_general_query_t *data) {
    if (!info || !data)
        return -1;
    size_t len = strlen(info);
    if (len < 3 || info[0] != '?' || info[len - 1] != '?')
        return -1;
    size_t type_len = len - 2;
    if (type_len > 10)
        return -1;
    memcpy(data->query_type, info + 1, type_len);
    data->query_type[type_len] = '\0';
    return 0;
}

int aprs_encode_station_capabilities(char *info, size_t len, const aprs_station_capabilities_t *data) {
    if (!info || !data || len < 1)
        return -1;
    size_t text_len = strlen(data->capabilities_text);
    if (text_len > 99)
        text_len = 99;
    size_t total_len = 1 + text_len;
    if (total_len > len)
        return -1;
    info[0] = '<';
    memcpy(info + 1, data->capabilities_text, text_len);
    if (total_len < len)
        info[total_len] = '\0'; // Null-terminate if space allows
    return total_len;
}

int aprs_decode_station_capabilities(const char *info, aprs_station_capabilities_t *data) {
    if (!info || !data)
        return -1;
    if (info[0] != '<')
        return -1;
    size_t len = strlen(info);
    if (len > 100)
        len = 100;
    size_t text_len = len - 1;
    memcpy(data->capabilities_text, info + 1, text_len);
    data->capabilities_text[text_len] = '\0';
    return 0;
}

int aprs_encode_bulletin(char *info, size_t len, const aprs_bulletin_t *data) {
    if (strlen(data->bulletin_id) > 4) {
        return -1; // Bulletin ID too long
    }
    char addressee[10];
    snprintf(addressee, 10, "%-9s", data->bulletin_id);
    aprs_message_t msg;
    strcpy(msg.addressee, addressee);
    msg.addressee[9] = '\0';
    msg.message = data->message;
    msg.message_number = data->message_number;
    return aprs_encode_message(info, len, &msg);
}

int aprs_encode_item_report(char *info, size_t len, const aprs_item_report_t *data) {
    if (strlen(data->name) > 9) {
        return -1; // Item name too long
    }
    if (data->symbol_table != '/' && data->symbol_table != '\\') {
        return -1; // Invalid symbol table
    }
    if (!isprint(data->symbol_code)) {
        return -1; // Invalid symbol code
    }

    char name_padded[10];
    snprintf(name_padded, 10, "%-9s", data->name);

    char *lat_str = lat_to_aprs(data->latitude, 0);
    char *lon_str = lon_to_aprs(data->longitude, 0);
    if (!lat_str || !lon_str) {
        return -1;
    }

    char status_char = data->is_live ? '!' : '=';
    int ret = snprintf(info, len, ")%s%c%s%c%s%c", name_padded, status_char, lat_str, data->symbol_table, lon_str, data->symbol_code);
    if (ret < 0 || (size_t) ret >= len) {
        return -1;
    }

    if (data->comment) {
        int ret2 = snprintf(info + ret, len - ret, "%s", data->comment);
        if (ret2 < 0 || (size_t) ret2 >= len - ret) {
            return -1;
        }
        ret += ret2;
    }

    return ret;
}

bool aprs_is_bulletin(const aprs_message_t *msg) {
    if (strncmp(msg->addressee, "BLN", 3) == 0 && isdigit(msg->addressee[3])) {
        return true;
    }
    return false;
}

int aprs_decode_item_report(const char *info, aprs_item_report_t *data) {
    size_t info_len = strlen(info);
    if (info[0] != ')' || info_len < 30) {
        return -1; // Minimum length for item report without comment
    }

    // Extract name (positions 1-9)
    strncpy(data->name, info + 1, 9);
    data->name[9] = '\0';
    trim_trailing_spaces(data->name);

    // Status character (position 10)
    char status_char = info[10];
    if (status_char == '!') {
        data->is_live = true;
    } else if (status_char == '=') {
        data->is_live = false;
    } else {
        return -1; // Invalid status character
    }

    // Parse latitude (positions 11-18)
    char lat_str[9];
    strncpy(lat_str, info + 11, 8);
    lat_str[8] = '\0';
    int lat_ambiguity;
    data->latitude = aprs_parse_lat(lat_str, &lat_ambiguity);
    if (isnan(data->latitude)) {
        return -1;
    }

    // Symbol table (position 19)
    if (info[19] != '/' && info[19] != '\\') {
        return -1; // Invalid symbol table
    }
    data->symbol_table = info[19];

    // Parse longitude (positions 20-28)
    char lon_str[10];
    strncpy(lon_str, info + 20, 9);
    lon_str[9] = '\0';
    int lon_ambiguity;
    data->longitude = aprs_parse_lon(lon_str, &lon_ambiguity);
    if (isnan(data->longitude)) {
        return -1;
    }

    // Symbol code (position 29)
    if (!isprint(info[29])) {
        return -1; // Invalid symbol code
    }
    data->symbol_code = info[29];

    // Comment (position 30 onwards)
    if (info_len > 30) {
        data->comment = my_strdup(info + 30);
        if (!data->comment) {
            return -1; // Memory allocation failure
        }
    } else {
        data->comment = my_strdup("");
        if (!data->comment) {
            return -1; // Memory allocation failure
        }
    }

    return 0;
}

int aprs_encode_raw_gps(char *info, size_t len, const aprs_raw_gps_t *data) {
    if (data == NULL || data->raw_data == NULL || data->data_len < 3 || strncmp(data->raw_data, "GP", 2) != 0) {
        return -1;  // Invalid raw GPS data
    }
    if (len < data->data_len + 2) {  // +1 for DTI, +1 for null terminator
        return -1;
    }
    info[0] = APRS_DTI_RAW_GPS;   // '$'
    memcpy(info + 1, data->raw_data, data->data_len);
    info[data->data_len + 1] = '\0';
    return data->data_len + 1;
}

int aprs_encode_test_packet(char *info, size_t len, const aprs_test_packet_t *data) {
    if (len < data->data_len + 2) {  // +1 for DTI, +1 for null terminator
        return -1;
    }
    info[0] = APRS_DTI_TEST_PACKET;
    memcpy(info + 1, data->data, data->data_len);
    info[data->data_len + 1] = '\0';
    return data->data_len + 1;
}

int aprs_decode_raw_gps(const char *info, aprs_raw_gps_t *data) {
    if (info[0] != APRS_DTI_RAW_GPS) {
        return -1;
    }
    size_t total_len = strlen(info);
    if (total_len <= 1) {
        return -1;
    }
    size_t data_len = total_len - 1;
    data->raw_data = my_strndup(info + 1, data_len);
    if (!data->raw_data) {
        return -1;
    }
    data->data_len = data_len;
    return 0;
}

int aprs_encode_grid_square(char *info, size_t len, const aprs_grid_square_t *data) {
    if (data == NULL || len < 1) {
        return -1;
    }
    size_t grid_len = strlen(data->grid_square);
    if (grid_len != 4 && grid_len != 6) {
        return -1;
    }
    size_t comment_len = data->comment ? strlen(data->comment) : 0;
    size_t total_len = 1 + grid_len + 1 + comment_len;
    if (len < total_len + 1) {
        return -1;
    }
    info[0] = APRS_DTI_GRID_SQUARE;
    memcpy(info + 1, data->grid_square, grid_len);
    size_t pos = 1 + grid_len;
    info[pos++] = ' ';
    if (comment_len > 0) {
        memcpy(info + pos, data->comment, comment_len);
        pos += comment_len;
    }
    info[pos] = '\0';
    return total_len;
}

int aprs_decode_grid_square(const char *info, aprs_grid_square_t *data) {
    if (info == NULL || info[0] != APRS_DTI_GRID_SQUARE) {
        return -1;
    }
    size_t len = strlen(info);
    if (len < 6) {
        return -1;
    }
    const char *space_pos = strchr(info + 1, ' ');
    if (space_pos == NULL) {
        return -1;
    }
    size_t grid_len = space_pos - (info + 1);
    if (grid_len != 4 && grid_len != 6) {
        return -1;
    }
    strncpy(data->grid_square, info + 1, grid_len);
    data->grid_square[grid_len] = '\0';
    size_t comment_start = space_pos - info + 1;
    size_t comment_len = len - comment_start;
    if (comment_len > 0) {
        data->comment = malloc(comment_len + 1);
        if (!data->comment) {
            return -1;
        }
        strncpy(data->comment, info + comment_start, comment_len);
        data->comment[comment_len] = '\0';
    } else {
        data->comment = NULL;
    }
    return 0;
}

int aprs_encode_df_report(char *info, size_t len, const aprs_df_report_t *data) {
    if (data == NULL || data->bearing < 0 || data->bearing > 359 || data->signal_strength < 0 || data->signal_strength > 9) {
        return -1;
    }
    size_t comment_len = data->comment ? strlen(data->comment) : 0;
    size_t total_len = 6 + comment_len;
    if (len < total_len + 1) {
        return -1;
    }
    info[0] = APRS_DTI_DF_REPORT;
    snprintf(info + 1, 6, "%03d/%d", data->bearing, data->signal_strength);
    size_t pos = 6;
    if (comment_len > 0) {
        memcpy(info + pos, data->comment, comment_len);
        pos += comment_len;
    }
    info[pos] = '\0';
    printf("Encoded DF report: '%s' (length: %zu)\n", info, total_len);
    return total_len;
}

int aprs_decode_df_report(const char *info, aprs_df_report_t *data) {
    size_t len = strlen(info);
    if (len < 6 || info[0] != APRS_DTI_DF_REPORT) {
        return -1;
    }
    if (!isdigit(info[1]) || !isdigit(info[2]) || !isdigit(info[3]) || info[4] != '/' || !isdigit(info[5])) {
        return -1;
    }
    char bearing_str[4] = { info[1], info[2], info[3], '\0' };
    data->bearing = atoi(bearing_str);
    if (data->bearing < 0 || data->bearing > 359) {
        return -1;
    }
    data->signal_strength = info[5] - '0';
    if (data->signal_strength < 0 || data->signal_strength > 9) {
        return -1;
    }
    if (len > 6) {
        size_t comment_len = len - 6;
        if (comment_len > 0) {
            data->comment = my_strdup(info + 6);
            if (!data->comment) {
                return -1;
            }
        } else {
            data->comment = NULL;
        }
    } else {
        data->comment = NULL;
    }
    return 0;
}

int aprs_decode_test_packet(const char *info, aprs_test_packet_t *data) {
    if (info[0] != APRS_DTI_TEST_PACKET) {
        return -1;
    }
    size_t len = strlen(info + 1);
    data->data = my_strndup(info + 1, len);
    if (!data->data) {
        return -1;
    }
    data->data_len = len;
    return 0;
}

static void encode_base91(uint32_t value, char *output, int length) {
    for (int i = length - 1; i >= 0; i--) {
        output[i] = BASE91_CHARSET[value % BASE91_SIZE];
        value /= BASE91_SIZE;
    }
}

static uint32_t decode_base91(const char *input, int length) {
    uint32_t value = 0;
    for (int i = 0; i < length; i++) {
        const char *pos = strchr(BASE91_CHARSET, input[i]);
        if (!pos)
            return 0;  // invalid character returns 0
        value = value * BASE91_SIZE + (pos - BASE91_CHARSET);
    }
    return value;
}

static void encode_latitude(double lat, char *output) {
    if (lat < -90.0 || lat > 90.0) {
        memset(output, BASE91_CHARSET[0], 4);
        return;
    }
    // shift from [-90,90] to [0,180]
    double scaled_double = (lat + 90.0) * 91.0 * 91.0 * 91.0 * 91.0 / 180.0;
    uint32_t scaled = (uint32_t) (scaled_double + 0.5);
    if (scaled > 91 * 91 * 91 * 91 - 1)
        scaled = 91 * 91 * 91 * 91 - 1;
    encode_base91(scaled, output, 4);
}

static double decode_latitude(const char *input) {
    uint32_t decoded = decode_base91(input, 4);
    return ((double) decoded * 180.0 / (double) (91 * 91 * 91 * 91 - 1)) - 90.0;
}

static void encode_longitude(double lon, char *output) {
    if (lon < -180.0 || lon > 180.0) {
        memset(output, BASE91_CHARSET[0], 4);
        return;
    }
    // shift from [-180,180] to [0,360]
    double scaled_double = (lon + 180.0) * 91.0 * 91.0 * 91.0 * 91.0 / 360.0;
    uint32_t scaled = (uint32_t) (scaled_double + 0.5);
    if (scaled > 91 * 91 * 91 * 91 - 1)
        scaled = 91 * 91 * 91 * 91 - 1;
    encode_base91(scaled, output, 4);
}

static double decode_longitude(const char *input) {
    uint32_t decoded = decode_base91(input, 4);
    return ((double) decoded * 360.0 / (double) (91 * 91 * 91 * 91 - 1)) - 180.0;
}

static void encode_course_speed(int course, int speed, char *output) {
    if (course < 0 || course > 360 || speed < 0) {
        // Invalid course or speed -> two spaces as per spec
        output[0] = ' ';
        output[1] = ' ';
        return;
    }
    // Normalize course (360 is treated as 0)
    if (course == 360) {
        course = 0;
    }
    // Course in 4-degree increments
    int c = course / 4;
    if (c > 89) {
        c = 89;
    }
    // Compute s from speed: s = round(log(speed+1) / log(1.08))
    double s_val = log((double) (speed + 1)) / log(1.08);
    int s = (int) (s_val + 0.5);
    if (s > 89) {
        s = 89;
    }
    // Map to APRS Base91 characters (ASCII offset 33): BASE91_CHARSET[0] == '!'
    output[0] = BASE91_CHARSET[c];
    output[1] = BASE91_CHARSET[s];
}

static void decode_course_speed(const char *input, int *course, int *speed) {
    // Check for blank (no data) or invalid
    if (!input || input[0] == ' ' || input[1] == ' ') {
        *course = -1;
        *speed = -1;
        return;
    }
    // Look up values c and s from the APRS Base91 alphabet
    const char *p0 = strchr(BASE91_CHARSET, input[0]);
    const char *p1 = strchr(BASE91_CHARSET, input[1]);
    if (!p0 || !p1) {
        *course = -1;
        *speed = -1;
        return;
    }
    int c = (int) (p0 - BASE91_CHARSET);
    int s = (int) (p1 - BASE91_CHARSET);
    // Validate ranges (spec allows c,s = 0..89)
    if (c < 0 || c > 89 || s < 0 || s > 89) {
        *course = -1;
        *speed = -1;
        return;
    }
    // Compute course = c * 4
    *course = c * 4;
    // Compute speed = round(1.08^s - 1)
    double spd = pow(1.08, (double) s) - 1.0;
    *speed = (int) (spd + 0.5);
    // Normalize 360->0 (although c<=89 should not produce exactly 360)
    if (*course == 360) {
        *course = 0;
    }
}

static void encode_altitude(int alt, char *output) {
    if (alt <= 0) {
        output[0] = output[1] = ' ';
        return;
    }
    double cs = log(alt) / log(1.002);
    uint32_t val = (uint32_t) (cs + 0.5);  // nearest integer exponent
    if (val >= BASE91_SIZE * BASE91_SIZE) {
        output[0] = output[1] = ' ';
        return;
    }
    encode_base91(val, output, 2);
}

static int decode_altitude(const char *input) {
    uint32_t cs = decode_base91(input, 2);
    double altd = pow(1.002, (double) cs);
    return (int) (altd + 0.5);
}

/**
 * Create compression type byte
 */
static char create_compression_type(bool has_data, bool is_altitude, bool is_current) {
    uint8_t byte = 0;

    if (is_current) {
        byte |= 0x20; // GPS fix is current
    }

    if (has_data) {
        if (is_altitude) {
            byte |= 0x02; // Altitude data format
        } else {
            byte |= 0x01; // Course/speed data format
        }
    }
    // else: no additional data (0x00)

    // Add offset to get into Base91 printable range
    return BASE91_CHARSET[byte + 33];
}

/**
 * Parse compression type byte
 */
static void parse_compression_type(char type_char, bool *has_data, bool *is_altitude, bool *is_current) {
    const char *pos = strchr(BASE91_CHARSET, type_char);
    if (pos == NULL) {
        *has_data = false;
        *is_altitude = false;
        *is_current = false;
        return;
    }

    uint8_t byte = (pos - BASE91_CHARSET) - 33;

    *is_current = (byte & 0x20) != 0;
    *has_data = (byte & 0x03) != 0;
    *is_altitude = (byte & 0x03) == 0x02;
}

int aprs_encode_compressed_position(char *info, size_t len, const aprs_compressed_position_t *data) {
    if (!info || !data || len < 15) {
        return -1;
    }

    // Validate input
    if (data->latitude < -90.0 || data->latitude > 90.0 || data->longitude < -180.0 || data->longitude > 180.0) {
        return -1;
    }

    char compressed[14]; // 13 chars + null terminator
    int pos = 0;

    // Encode latitude (4 characters)
    encode_latitude(data->latitude, &compressed[pos]);
    pos += 4;

    // Encode longitude (4 characters)
    encode_longitude(data->longitude, &compressed[pos]);
    pos += 4;

    // Symbol table
    compressed[pos++] = data->symbol_table;

    // Encode additional data (2 characters)
    bool has_data = false;
    bool is_altitude = false;

    if (data->has_altitude && data->altitude != INT_MIN) {
        encode_altitude(data->altitude, &compressed[pos]);
        has_data = true;
        is_altitude = true;
    } else if (data->has_course_speed && data->course >= 0 && data->speed >= 0) {
        encode_course_speed(data->course, data->speed, &compressed[pos]);
        has_data = true;
        is_altitude = false;
    } else {
        // No additional data
        compressed[pos] = ' ';
        compressed[pos + 1] = ' ';
    }
    pos += 2;

    // Symbol code
    compressed[pos++] = data->symbol_code;

    // Compression type
    compressed[pos++] = create_compression_type(has_data, is_altitude, true);

    compressed[pos] = '\0';

    // Assemble final packet
    int written = snprintf(info, len, "%c%s%s", data->dti ? data->dti : APRS_DTI_POSITION_NO_TS_NO_MSG, compressed, data->comment ? data->comment : "");

    if (written >= (int) len) {
        return -1; // Buffer too small
    }

    return written;
}

int aprs_decode_compressed_position(const char *info, aprs_compressed_position_t *data) {
    if (!info || !data || strlen(info) < 14) {
        return -1;
    }

    // Initialize structure
    memset(data, 0, sizeof(aprs_compressed_position_t));
    data->speed = -1;
    data->course = -1;
    data->altitude = INT_MIN;

    // Extract DTI
    data->dti = info[0];

    // Validate DTI
    if (data->dti != APRS_DTI_POSITION_NO_TS_NO_MSG && data->dti != APRS_DTI_POSITION_NO_TS_WITH_MSG && data->dti != APRS_DTI_POSITION_WITH_TS_NO_MSG
            && data->dti != APRS_DTI_POSITION_WITH_TS_WITH_MSG) {
        return -1;
    }

    // Extract compressed position (13 characters after DTI)
    const char *compressed = &info[1];

    if (strlen(compressed) < 13) {
        return -1;
    }

    // Decode latitude (characters 0-3)
    data->latitude = decode_latitude(&compressed[0]);
    if (data->latitude < -90.0 || data->latitude > 90.0) {
        return -1;
    }

    // Decode longitude (characters 4-7)
    data->longitude = decode_longitude(&compressed[4]);
    if (data->longitude < -180.0 || data->longitude > 180.0) {
        return -1;
    }

    // Extract symbol table (character 8)
    data->symbol_table = compressed[8];

    // Extract symbol code (character 11)
    data->symbol_code = compressed[11];

    // Parse compression type (character 12)
    bool has_data, is_altitude, is_current;
    parse_compression_type(compressed[12], &has_data, &is_altitude, &is_current);

    // Decode additional data if present (characters 9-10)
    if (has_data) {
        if (is_altitude) {
            data->altitude = decode_altitude(&compressed[9]);
            data->has_altitude = true;
        } else {
            decode_course_speed(&compressed[9], &data->course, &data->speed);
            if (data->course >= 0 && data->speed >= 0) {
                data->has_course_speed = true;
            }
        }
    }

    // Extract comment (everything after the 13-character compressed position)
    if (strlen(info) > 14) {
        size_t comment_len = strlen(info) - 14;
        data->comment = malloc(comment_len + 1);
        if (data->comment) {
            strcpy(data->comment, &info[14]);
        }
    }

    return 0;
}

bool aprs_is_compressed_position(const char *info) {
    if (!info || strlen(info) < 14) {
        return false;
    }

    char dti = info[0];
    if (dti != APRS_DTI_POSITION_NO_TS_NO_MSG && dti != APRS_DTI_POSITION_NO_TS_WITH_MSG && dti != APRS_DTI_POSITION_WITH_TS_NO_MSG
            && dti != APRS_DTI_POSITION_WITH_TS_WITH_MSG) {
        return false;
    }

    // Try to decode and see if it succeeds
    aprs_compressed_position_t temp;
    return aprs_decode_compressed_position(info, &temp) == 0;
}

void aprs_free_compressed_position(aprs_compressed_position_t *data) {
    if (data && data->comment) {
        free(data->comment);
        data->comment = NULL;
    }
}

/* Internal helper for parsing fixed-width numeric field */
static int parse_fixed_int(const char *s, int len) {
    char buf[8];
    if (len >= (int)sizeof(buf)) return -1;
    memcpy(buf, s, len);
    buf[len] = '\0';
    return atoi(buf);
}

int aprs_decode_peet1(const char *info, aprs_weather_report_t *data) {
    if (strncmp(info, "#W1", 3) != 0) return -1;
    info += 3;
    memset(data, 0, sizeof(*data));

    while (*info) {
        if (info[0] == 'c') data->wind_direction = parse_fixed_int(info + 1, 3);
        else if (info[0] == 's') data->wind_speed = parse_fixed_int(info + 1, 3);
        else if (info[0] == 'g') data->wind_gust = parse_fixed_int(info + 1, 3);
        else if (info[0] == 't') data->temperature = (float)parse_fixed_int(info + 1, 3);
        else if (info[0] == 'r') data->rain_1h = parse_fixed_int(info + 1, 3);
        else if (info[0] == 'p') data->rain_24h = parse_fixed_int(info + 1, 3);
        else if (info[0] == 'P') data->rain_midnight = parse_fixed_int(info + 1, 3);
        else if (info[0] == 'h') data->humidity = parse_fixed_int(info + 1, 2);
        else if (info[0] == 'b') data->barometric_pressure = parse_fixed_int(info + 1, 5);
        info += (info[0] == 'h') ? 3 : (info[0] == 'b') ? 6 : 4;
    }
    return 0;
}

int aprs_decode_peet2(const char *info, aprs_weather_report_t *data) {
    if (strncmp(info, "*W2", 3) != 0) return -1;
    return aprs_decode_peet1(info + 1, data);
}

int aprs_encode_peet1(char *dst, int len, const aprs_weather_report_t *data) {
    return snprintf(dst, len, "#W1c%03ds%03dg%03dt%03dr%03dp%03dP%03dh%02db%05d",
        data->wind_direction,
        data->wind_speed,
        data->wind_gust,
        (int)data->temperature,
        data->rain_1h,
        data->rain_24h,
        data->rain_midnight,
        data->humidity,
        data->barometric_pressure);
}


int aprs_encode_peet2(char *dst, int len, const aprs_weather_report_t *data) {
    int r = aprs_encode_peet1(dst + 1, len - 1, data);
    if (r <= 0) return -1;
    dst[0] = '*';
    return r + 1;
}

int aprs_decode_position_weather(const aprs_position_no_ts_t *pos,
                                 aprs_weather_report_t *w) {
    if (pos->symbol_code != '_') {
        return -1;  // Not a weather-bearing position report
    }
    if (!pos->comment) {
        return -1;
    }
    // Prepend the "#W1" header (Peet Bros format 1) to parse the fields
    char buf[APRS_COMMENT_LEN + 4];
    int n = snprintf(buf, sizeof(buf), "#W1%s", pos->comment);
    if (n < 0 || (size_t)n >= sizeof(buf)) {
        return -1;  // Encoding error or buffer overflow
    }
    return aprs_decode_peet1(buf, w);
}
