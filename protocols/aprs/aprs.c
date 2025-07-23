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

#include "common.h"
#include "aprs.h"

/**
 * Custom strdup implementation for C99.
 * @param s String to duplicate
 * @return Pointer to duplicated string or NULL on failure
 */
static char* my_strdup(const char *s) {
    size_t len = strlen(s) + 1;
    char *dup = malloc(len);
    if (dup) {
        memcpy(dup, s, len);
    }
    return dup;
}

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
        int digit_positions[] = { 0, 1, 2, 3, 5, 6 };  // positions of digits in buf
        int num_digits = 6;
        int start = num_digits - ambiguity;
        if (start < 0)
            start = 0;
        for (int i = start; i < num_digits; i++) {
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
        int digit_positions[] = { 0, 1, 2, 3, 4, 6, 7 };  // positions of digits in buf
        int num_digits = 7;
        int start = num_digits - ambiguity;
        if (start < 0)
            start = 0;
        for (int i = start; i < num_digits; i++) {
            buf[digit_positions[i]] = ' ';
        }
    }
    return buf;
}

int aprs_encode_position_no_ts(char *info, size_t len, const aprs_position_no_ts_t *data) {
    if (data->symbol_table != '/' && data->symbol_table != '\\') {
        return -1; // Invalid symbol table
    }
    if (!isprint(data->symbol_code)) {
        return -1; // Invalid symbol code
    }
    char dti = (data->dti == '!' || data->dti == '=') ? data->dti : '!';

    int ambiguity = 0;
    if (data->comment && strncmp(data->comment, "AMB", 3) == 0 && isdigit(data->comment[3]) && data->comment[4] == '\0') {
        ambiguity = data->comment[3] - '0';
        if (ambiguity < 0 || ambiguity > 4) {
            return -1; // Invalid ambiguity level
        }
    }

    char *lat_str = lat_to_aprs(data->latitude, ambiguity);
    char *lon_str = lon_to_aprs(data->longitude, ambiguity);
    if (!lat_str || !lon_str) {
        return -1;
    }

    int ret = snprintf(info, len, "%c%s%c%s%c", dti, lat_str, data->symbol_table, lon_str, data->symbol_code);
    if (ret < 0 || (size_t) ret >= len) {
        return -1;
    }

    if (data->has_course_speed) {
        int course = data->course % 360;
        if (course < 0)
            course += 360;
        int speed = data->speed;
        if (speed < 0)
            speed = 0;
        int ret2 = snprintf(info + ret, len - ret, "%03d/%03d", course, speed);
        if (ret2 < 0 || (size_t) ret2 >= len - ret) {
            return -1;
        }
        ret += ret2;
    }

    if (data->comment && strncmp(data->comment, "AMB", 3) != 0) {
        int ret2 = snprintf(info + ret, len - ret, "%s", data->comment);
        if (ret2 < 0 || (size_t) ret2 >= len - ret) {
            return -1;
        }
        ret += ret2;
    }

    return ret;
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

int aprs_decode_position_no_ts(const char *info, aprs_position_no_ts_t *data) {
    size_t info_len = strlen(info);
    if (info_len < 20) {
        return -1; // Minimum length for position without extension
    }

    if (info[0] != '!' && info[0] != '=') {
        return -1; // Invalid data type identifier
    }
    data->dti = info[0];

    // Parse latitude: info[1] to info[8] (ddmm.mmN/S)
    char lat_str[9];
    strncpy(lat_str, info + 1, 8);
    lat_str[8] = '\0';
    int lat_ambiguity;
    data->latitude = aprs_parse_lat(lat_str, &lat_ambiguity);
    if (isnan(data->latitude)) {
        return -1; // Invalid latitude
    }

    // Symbol table: info[9]
    if (info[9] != '/' && info[9] != '\\') {
        return -1; // Invalid symbol table
    }
    data->symbol_table = info[9];

    // Parse longitude: info[10] to info[18] (dddmm.mmE/W)
    char lon_str[10];
    strncpy(lon_str, info + 10, 9);
    lon_str[9] = '\0';
    int lon_ambiguity;
    data->longitude = aprs_parse_lon(lon_str, &lon_ambiguity);
    if (isnan(data->longitude)) {
        return -1; // Invalid longitude
    }

    // Symbol code: info[19]
    if (!isprint(info[19])) {
        return -1; // Invalid symbol code
    }
    data->symbol_code = info[19];

    // Store ambiguity level in comment if present
    data->comment = NULL;
    if (lat_ambiguity > 0 || lon_ambiguity > 0) {
        char amb_str[5];
        snprintf(amb_str, sizeof(amb_str), "AMB%d", lat_ambiguity > lon_ambiguity ? lat_ambiguity : lon_ambiguity);
        data->comment = my_strdup(amb_str);
        if (!data->comment) {
            return -1; // Memory allocation failure
        }
    }

    // Check for course/speed extension (ddd/ddd)
    data->has_course_speed = false;
    data->course = 0;
    data->speed = 0;
    if (info_len >= 27) {
        // Check format: three digits, a slash, three digits
        bool is_extension = true;
        for (int i = 0; i < 3; i++) {
            if (!isdigit(info[20 + i])) {
                is_extension = false;
                break;
            }
        }
        if (info[23] != '/') {
            is_extension = false;
        }
        for (int i = 0; i < 3; i++) {
            if (!isdigit(info[24 + i])) {
                is_extension = false;
                break;
            }
        }
        if (is_extension) {
            char course_str[4] = { info[20], info[21], info[22], '\0' };
            char speed_str[4] = { info[24], info[25], info[26], '\0' };
            data->course = atoi(course_str);
            data->speed = atoi(speed_str);
            if (data->course > 359 || data->course < 0) { // Course must be 0–359
                return -1;
            }
            if (data->speed < 0) { // Speed must be non-negative
                return -1;
            }
            data->has_course_speed = true;
            if (info_len > 27) {
                if (!data->comment) { // Only set comment if not already set by ambiguity
                    data->comment = my_strdup(info + 27); // Comment starts after course/speed
                    if (!data->comment) {
                        return -1; // Memory allocation failure
                    }
                }
            } else {
                if (!data->comment) {
                    data->comment = my_strdup(""); // Empty comment
                    if (!data->comment) {
                        return -1; // Memory allocation failure
                    }
                }
            }
        } else {
            if (!data->comment) { // Only set comment if not already set by ambiguity
                data->comment = my_strdup(info + 20); // Treat invalid extension as comment
                if (!data->comment) {
                    return -1; // Memory allocation failure
                }
            }
        }
    } else {
        if (!data->comment) { // Only set comment if not already set by ambiguity
            data->comment = my_strdup(info + 20); // Comment starts after symbol code
            if (!data->comment) {
                return -1; // Memory allocation failure
            }
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
    int written = 0;

    // Ensure buffer is initialized
    if (!info || len == 0)
        return 0;

    // Add underscore and timestamp
    written += snprintf(info + written, len - written, "_%s", data->timestamp);

    // Wind Direction
    if (data->wind_direction >= 0 && data->wind_direction <= 360) {
        written += snprintf(info + written, len - written, "c%03d", data->wind_direction);
    }

    // Wind Speed
    if (data->wind_speed >= 0) {
        written += snprintf(info + written, len - written, "s%03d", data->wind_speed);
    }

    // Temperature
    if (data->temperature >= -99.9 && data->temperature <= 999.9) {
        int temp_int = (int) round(data->temperature);
        if (temp_int >= 0) {
            written += snprintf(info + written, len - written, "t%03d", temp_int);
        } else {
            written += snprintf(info + written, len - written, "t-%02d", -temp_int);
        }
    }

    // Other fields (skipped if invalid to match test expectations)
    if (data->wind_gust >= 0) {
        written += snprintf(info + written, len - written, "g%03d", data->wind_gust);
    }
    if (data->rainfall_24h >= 0) {
        written += snprintf(info + written, len - written, "p%03d", data->rainfall_24h);
    }
    if (data->rainfall_since_midnight >= 0) {
        written += snprintf(info + written, len - written, "P%03d", data->rainfall_since_midnight);
    }
    if (data->humidity >= 0 && data->humidity <= 100) {
        written += snprintf(info + written, len - written, "h%02d", data->humidity);
    }
    if (data->barometric_pressure >= 0) {
        written += snprintf(info + written, len - written, "b%05d", data->barometric_pressure);
    }
    if (data->luminosity >= 0) {
        if (data->luminosity < 1000) {
            written += snprintf(info + written, len - written, "L%03d", data->luminosity);
        } else {
            written += snprintf(info + written, len - written, "l%03d", data->luminosity - 1000);
        }
    }
    if (data->snowfall_24h >= 0.0) {
        int snow_int = (int) round(data->snowfall_24h);
        written += snprintf(info + written, len - written, "S%03d", snow_int);
    }
    if (data->rain_rate >= 0) {
        written += snprintf(info + written, len - written, "R%03d", data->rain_rate);
    }
    if (data->water_height_feet >= 0.0) {
        written += snprintf(info + written, len - written, "F%.1f", data->water_height_feet);
    }
    if (data->water_height_meters >= 0.0) {
        written += snprintf(info + written, len - written, "f%.1f", data->water_height_meters);
    }
    if (data->indoors_temperature >= -99.9 && data->indoors_temperature <= 999.9) {
        int temp_int = (int) round(data->indoors_temperature);
        if (temp_int >= 0) {
            written += snprintf(info + written, len - written, "i%02d", temp_int);
        } else {
            written += snprintf(info + written, len - written, "i-%02d", -temp_int);
        }
    }
    if (data->indoors_humidity >= 0 && data->indoors_humidity <= 100) {
        written += snprintf(info + written, len - written, "I%02d", data->indoors_humidity);
    }
    if (data->raw_rain_counter >= 0) {
        written += snprintf(info + written, len - written, "#%05d", data->raw_rain_counter);
    }

    return written;
}

int aprs_decode_weather_report(const char *info, aprs_weather_report_t *data) {
    // Initialize all fields to absence values
    memset(data, 0, sizeof(*data));
    data->temperature = -999.9;
    data->wind_speed = -1;
    data->wind_direction = -1;
    data->wind_gust = -1;
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

    const char *p = info;

    // Check for positionless weather report starting with '_'
    if (*p == '_') {
        p++;
        // Extract timestamp (8 characters: MMDDHHMM)
        if (strlen(p) >= 8) {
            strncpy(data->timestamp, p, 8);
            data->timestamp[8] = '\0'; // Ensure null-termination
            p += 8;
        } else {
            return -1; // Invalid timestamp
        }
    } else {
        return -1; // Not a positionless weather report
    }

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
                data->temperature = -atof(buf);
            } else {
                char buf[4];
                strncpy(buf, p, 3);
                buf[3] = '\0';
                p += 3;
                data->temperature = atof(buf);
            }
        } else if (*p == 's') {
            p++;
            char buf[4];
            strncpy(buf, p, 3);
            buf[3] = '\0';
            p += 3;
            data->wind_speed = atoi(buf);
        } else if (*p == 'g') {
            p++;
            char buf[4];
            strncpy(buf, p, 3);
            buf[3] = '\0';
            p += 3;
            data->wind_gust = atoi(buf);
        } else if (*p == 'c') {
            p++;
            char buf[4];
            strncpy(buf, p, 3);
            buf[3] = '\0';
            p += 3;
            data->wind_direction = atoi(buf);
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
            p++;
            char buf[5];
            strncpy(buf, p, 4);
            buf[4] = '\0';
            p += 4;
            data->luminosity = atoi(buf);
        } else if (*p == 'S') {
            p++;
            char buf[4];
            strncpy(buf, p, 3);
            buf[3] = '\0';
            p += 3;
            data->snowfall_24h = atof(buf);
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
    if (data->symbol_table != '/' && data->symbol_table != '\\') {
        return -1; // Invalid symbol table
    }
    if (!isprint(data->symbol_code)) {
        return -1; // Invalid symbol code
    }
    char dti = (data->dti == '/' || data->dti == '@') ? data->dti : '@';

    // Use ambiguity 0 (full precision) since timestamped positions don’t support ambiguity
    char *lat_str = lat_to_aprs(data->latitude, 0);
    char *lon_str = lon_to_aprs(data->longitude, 0);
    if (!lat_str || !lon_str) {
        return -1;
    }

    int ret = snprintf(info, len, "%c%s%s%c%s%c", dti, data->timestamp, lat_str, data->symbol_table, lon_str, data->symbol_code);
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

int aprs_decode_position_with_ts(const char *info, aprs_position_with_ts_t *data) {
    if (strlen(info) < 26 || (info[0] != '/' && info[0] != '@')) {
        return -1;
    }

    data->dti = info[0];

    // Extract timestamp (positions 1-7)
    strncpy(data->timestamp, info + 1, 7);
    data->timestamp[7] = '\0';
    for (int i = 0; i < 7; i++) {
        if (!isdigit(data->timestamp[i]) && data->timestamp[i] != 'z') {
            return -1;
        }
    }
    if (data->timestamp[6] != 'z') {
        return -1; // Must end with 'z' for simplified format
    }

    // Parse latitude (positions 8-15)
    char lat_str[9];
    strncpy(lat_str, info + 8, 8);
    lat_str[8] = '\0';
    int lat_ambiguity; // Dummy variable for ambiguity (not used in timestamped positions)
    data->latitude = aprs_parse_lat(lat_str, &lat_ambiguity);
    if (isnan(data->latitude)) {
        return -1;
    }

    // Symbol table (position 16)
    data->symbol_table = info[16];

    // Parse longitude (positions 17-25)
    char lon_str[10];
    strncpy(lon_str, info + 17, 9);
    lon_str[9] = '\0';
    int lon_ambiguity; // Dummy variable for ambiguity (not used in timestamped positions)
    data->longitude = aprs_parse_lon(lon_str, &lon_ambiguity);
    if (isnan(data->longitude)) {
        return -1;
    }

    // Symbol code (position 26)
    data->symbol_code = info[26];

    // Comment (position 27 onwards)
    data->comment = my_strdup(info + 27);
    if (!data->comment && info[27] != '\0') {
        return -1;
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
        return -1;  // Invalid grid square length
    }
    size_t comment_len = data->comment ? strlen(data->comment) : 0;
    size_t total_len = 1 + grid_len + 1 + comment_len;  // '>' + grid + ' ' + comment
    if (len < total_len + 1) {  // +1 for null terminator
        return -1;
    }
    info[0] = '>';  // Use '>' as DTI
    memcpy(info + 1, data->grid_square, grid_len);
    size_t pos = 1 + grid_len;
    info[pos++] = ' ';  // Add space separator
    if (comment_len > 0) {
        memcpy(info + pos, data->comment, comment_len);
        pos += comment_len;
    }
    info[pos] = '\0';
    return total_len;
}

int aprs_decode_grid_square(const char *info, aprs_grid_square_t *data) {
    if (info == NULL || info[0] != '>') {
        return -1;
    }
    size_t len = strlen(info);
    if (len < 6) {  // At least '>' + 4 chars + ' '
        return -1;
    }
    // Find the space separator
    const char *space_pos = strchr(info + 1, ' ');
    if (space_pos == NULL) {
        return -1;  // No space separator found
    }
    size_t grid_len = space_pos - (info + 1);
    if (grid_len != 4 && grid_len != 6) {
        return -1;  // Invalid grid square length
    }
    strncpy(data->grid_square, info + 1, grid_len);
    data->grid_square[grid_len] = '\0';
    // Comment starts after the space
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
        return -1;  // Invalid bearing or signal strength
    }
    size_t comment_len = data->comment ? strlen(data->comment) : 0;
    size_t total_len = 1 + 3 + 1 + 3 + comment_len;  // '@' + BRG (3) + '/' + NRQ (3) + comment
    if (len < total_len + 1) {  // +1 for null terminator
        return -1;
    }
    info[0] = '@';  // Use '@' as DTI for position report
    snprintf(info + 1, 8, "%03d/%d00", data->bearing, data->signal_strength);  // BRG/NRQ format
    size_t pos = 8;  // after DTI + BRG + '/' + NRQ
    if (comment_len > 0) {
        memcpy(info + pos, data->comment, comment_len);
        pos += comment_len;
    }
    info[pos] = '\0';
    return total_len;
}

int aprs_decode_df_report(const char *info, aprs_df_report_t *data) {
    if (info[0] != '@' || strlen(info) < 8) {  // '@' + 3 digits + '/' + 3 digits
        return -1;
    }
    char bearing_str[4];
    strncpy(bearing_str, info + 1, 3);
    bearing_str[3] = '\0';
    data->bearing = atoi(bearing_str);
    if (data->bearing < 0 || data->bearing > 359) {
        return -1;
    }
    if (info[4] != '/') {
        return -1;
    }
    char nrq_str[4];
    strncpy(nrq_str, info + 5, 3);
    nrq_str[3] = '\0';
    data->signal_strength = nrq_str[0] - '0';  // Assuming NRQ is "S00" where S is signal strength
    if (data->signal_strength < 0 || data->signal_strength > 9) {
        return -1;
    }
    size_t len = strlen(info);
    if (len > 8) {
        data->comment = malloc(strlen(info + 8) + 1);
        if (!data->comment) {
            return -1;
        }
        strcpy(data->comment, info + 8);
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
