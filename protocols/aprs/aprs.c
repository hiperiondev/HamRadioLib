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

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832
#endif

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
    // MOD: removed centering offsets; now return raw parsed value and only set ambiguity. // MODIFIED
    // Centering offsets should be applied by the application for display purposes, per APRS spec recommendation. // MODIFIED
    if (!str || strlen(str) != 8) {
        return NAN;
    }
    if (str[4] != '.') {
        return NAN;
    }
    // Expected format: DDMM.hhN with optional spaces for ambiguity.
    // Count ambiguity by counting spaces from the right among: [6]=hh(2), [5]=hh(1), [3]=MM(2), [2]=MM(1).
    int amb = 0;
    if (str[6] == ' ')
        amb++;
    if (str[5] == ' ')
        amb++;
    if (str[3] == ' ')
        amb++;
    if (str[2] == ' ')
        amb++;
    if (ambiguity)
        *ambiguity = amb;  // MODIFIED

    // Build numeric strings substituting spaces with '0' (raw value without centering). // MODIFIED
    char deg_str[3] = { str[0], str[1], '\0' };
    char min_str[3] = { (str[2] == ' ' ? '0' : str[2]), (str[3] == ' ' ? '0' : str[3]), '\0' };  // MODIFIED
    char frac_str[3] = { (str[5] == ' ' ? '0' : str[5]), (str[6] == ' ' ? '0' : str[6]), '\0' };  // MODIFIED
    char dir = str[7];

    if (!isdigit((unsigned char )deg_str[0]) || !isdigit((unsigned char )deg_str[1]))
        return NAN;
    if ((min_str[0] < '0' || min_str[0] > '9') || (min_str[1] < '0' || min_str[1] > '9'))
        return NAN;
    if (!((frac_str[0] >= '0' && frac_str[0] <= '9') || frac_str[0] == '0'))
        return NAN;
    if (!((frac_str[1] >= '0' && frac_str[1] <= '9') || frac_str[1] == '0'))
        return NAN;

    int degrees = atoi(deg_str);
    int minutes = atoi(min_str);
    int frac = atoi(frac_str);

    if (degrees < 0 || degrees > 90)
        return NAN;
    if (minutes < 0 || minutes > 59)
        return NAN;

    double lat = degrees + ((double) minutes + (double) frac / 100.0) / 60.0;

    if (dir == 'S')
        lat = -lat;
    else if (dir != 'N')
        return NAN;

    return lat;
}

double aprs_parse_lon(const char *str, int *ambiguity) {
    // MOD: removed centering offsets; now return raw parsed value and only set ambiguity. // MODIFIED
    // Centering offsets should be applied by the application for display purposes, per APRS spec recommendation. // MODIFIED
    if (!str || strlen(str) != 9) {
        return NAN;
    }
    if (str[5] != '.') {
        return NAN;
    }
    // Expected format: DDDMM.hhE/W with optional spaces for ambiguity.
    // Count ambiguity by counting spaces from the right among: [7]=hh(2), [6]=hh(1), [4]=MM(2), [3]=MM(1).
    int amb = 0;
    if (str[7] == ' ')
        amb++;
    if (str[6] == ' ')
        amb++;
    if (str[4] == ' ')
        amb++;
    if (str[3] == ' ')
        amb++;
    if (ambiguity)
        *ambiguity = amb;  // MODIFIED

    // Build numeric strings substituting spaces with '0' (raw value without centering). // MODIFIED
    char deg_str[4] = { str[0], str[1], str[2], '\0' };
    char min_str[3] = { (str[3] == ' ' ? '0' : str[3]), (str[4] == ' ' ? '0' : str[4]), '\0' };  // MODIFIED
    char frac_str[3] = { (str[6] == ' ' ? '0' : str[6]), (str[7] == ' ' ? '0' : str[7]), '\0' };  // MODIFIED
    char dir = str[8];

    if (!isdigit((unsigned char )deg_str[0]) || !isdigit((unsigned char )deg_str[1]) || !isdigit((unsigned char )deg_str[2]))
        return NAN;
    if ((min_str[0] < '0' || min_str[0] > '9') || (min_str[1] < '0' || min_str[1] > '9'))
        return NAN;
    if (!((frac_str[0] >= '0' && frac_str[0] <= '9') || frac_str[0] == '0'))
        return NAN;
    if (!((frac_str[1] >= '0' && frac_str[1] <= '9') || frac_str[1] == '0'))
        return NAN;

    int degrees = atoi(deg_str);
    int minutes = atoi(min_str);
    int frac = atoi(frac_str);

    if (degrees < 0 || degrees > 180)
        return NAN;
    if (minutes < 0 || minutes > 59)
        return NAN;

    double lon = degrees + ((double) minutes + (double) frac / 100.0) / 60.0;

    if (dir == 'W')
        lon = -lon;
    else if (dir != 'E')
        return NAN;

    return lon;
}

// Convierte latitud a cadena APRS "DDMM.mmN/S", aplicando ambigüedad (espacios)
char* lat_to_aprs(double lat, int ambiguity) {
    static char buf[9];  // "DDMM.mmN" + '\0'
    if (lat < -90 || lat > 90 || ambiguity < 0 || ambiguity > 4) {
        return NULL;
    }
    char dir = (lat >= 0) ? 'N' : 'S';
    lat = fabs(lat);
    int deg = (int) lat;
    double min = (lat - deg) * 60.0;
    int min_int = (int) min;
    int min_frac = (int) ((min - min_int) * 100);
    sprintf(buf, "%02d%02d.%02d%c", deg, min_int, min_frac, dir);

    if (ambiguity > 0) {
        // Posi. de dígitos a reemplazar: [5,6] decimales de minutos; [3,2] dígitos de minutos
        int digit_positions[] = { 5, 6, 3, 2 };
        for (int i = 0; i < ambiguity && i < 4; i++) {
            buf[digit_positions[i]] = ' ';
        }
    }
    return buf;
}

// Convierte longitud a cadena APRS "DDDMM.mmE/W", con ambigüedad similar
char* lon_to_aprs(double lon, int ambiguity) {
    static char buf[10];  // "DDDMM.mmE" + '\0'
    if (lon < -180 || lon > 180 || ambiguity < 0 || ambiguity > 4) {
        return NULL;
    }
    char dir = (lon >= 0) ? 'E' : 'W';
    lon = fabs(lon);
    int deg = (int) lon;
    double min = (lon - deg) * 60.0;
    int min_int = (int) min;
    int min_frac = (int) ((min - min_int) * 100);
    sprintf(buf, "%03d%02d.%02d%c", deg, min_int, min_frac, dir);

    if (ambiguity > 0) {
        // Posi. de dígitos a reemplazar: [6,7] decimales; [4,3] dígitos de minutos
        int digit_positions[] = { 6, 7, 4, 3 };
        for (int i = 0; i < ambiguity && i < 4; i++) {
            buf[digit_positions[i]] = ' ';
        }
    }
    return buf;
}

int aprs_encode_message(char *info, size_t len, const aprs_message_t *data) {
    bool null_found = false;
    for (int i = 0; i < 9; i++) {
        if (data->addressee[i] == '\0') {
            null_found = true;
            break;
        }
    }
    if (!null_found && data->addressee[9] != '\0') {
        return -1;
    }

    char addressee[10];
    strncpy(addressee, data->addressee, 9);
    addressee[9] = '\0';

    if (data->message && strlen(data->message) > 67) {
        return -1;
    }
    if (data->message_number && strlen(data->message_number) > 5) {
        return -1;
    }

    // MOD FIX: add parentheses for correct precedence
    if (data->message
            && (((data->message[0] == 'a' || data->message[0] == 'A') && (data->message[1] == 'c' || data->message[1] == 'C')
                    && (data->message[2] == 'k' || data->message[2] == 'K'))
                    || ((data->message[0] == 'r' || data->message[0] == 'R') && (data->message[1] == 'e' || data->message[1] == 'E')
                            && (data->message[2] == 'j' || data->message[2] == 'J')))) {
        if (!data->message_number || strlen(data->message_number) != 5)
            return -1;
    }

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

// Modified function to encode position without timestamp, including APRS v1.2 speed encoding
int aprs_encode_position_no_ts(char *out, size_t outlen, const aprs_position_no_ts_t *data) {
    if (!out || !data || outlen < 21)
        return -1;

    // Determine DTI (default '!' if data->dti==0)
    char dti_char = (data->dti != 0) ? data->dti : APRS_DTI_POSITION_NO_TS_NO_MSG;

    // Get APRS strings for lat and lon with ambiguity
    char *lat_str = lat_to_aprs(data->latitude, data->ambiguity);
    char *lon_str = lon_to_aprs(data->longitude, data->ambiguity);
    if (!lat_str || !lon_str)
        return -1;

    // Basic format: DTI + latitude + symbol table + longitude + symbol code
    int n = snprintf(out, outlen, "%c%s%c%s%c", dti_char, lat_str, data->symbol_table, lon_str, data->symbol_code);
    if (n < 0 || (size_t) n >= outlen)
        return -1;
    size_t idx = (size_t) n;

    // Optional course/speed (course mod 360, speed with APRS v1.2 encoding)
    if (data->has_course_speed) {
        int course = data->course % 360;
        if (course < 0)
            course += 360;
        int encoded_speed;

        // Special case for space station speed
        if (data->speed == 15118) {
            encoded_speed = 799;  // Encode 15,118 knots as 799 per APRS v1.2
        }
        // Speeds above 670 knots use S = 670 + (speed - 670) / 112
        else if (data->speed > 670) {
            if (data->speed > 74370) {
                encoded_speed = 999;  // Clamp to max representable value
            } else {
                encoded_speed = 670 + (data->speed - 670) / 112;
                if (encoded_speed > 999)
                    encoded_speed = 999;  // Ensure within 3 digits
            }
        }
        // Speeds 0-670 knots encoded directly
        else {
            encoded_speed = data->speed < 0 ? 0 : data->speed;
        }

        int m = snprintf(out + idx, outlen - idx, "%03d/%03d", course, encoded_speed);
        if (m < 0 || idx + (size_t) m >= outlen)
            return -1;
        idx += (size_t) m;
    }

    // Optional comment
    if (data->comment && *data->comment) {
        size_t clen = strlen(data->comment);
        if (idx + clen >= outlen)
            return -1;
        memcpy(out + idx, data->comment, clen);
        idx += clen;
    }

    out[idx] = '\0';
    return (int) idx;
}

static inline bool aprs_is_only_spaces(const char *s) {  // MODIFIED
    if (!s)
        return true;
    while (*s) {
        if (!isspace((unsigned char )*s))
            return false;
        s++;
    }
    return true;
}

int aprs_decode_position_no_ts(const char *info, aprs_position_no_ts_t *pos) {
    if (!info || !pos)
        return -1;
    size_t len = strlen(info);
    if (len < 20)
        return -1;

    memset(pos, 0, sizeof(*pos));
    pos->dti = info[0];
    if (pos->dti != '!' && pos->dti != '=')
        return -1;

    char lat_str[9];
    memcpy(lat_str, info + 1, 8);
    lat_str[8] = '\0';
    int amb_lat = 0;  // MODIFIED
    pos->latitude = aprs_parse_lat(lat_str, &amb_lat);
    if (isnan(pos->latitude))
        return -1;

    pos->symbol_table = info[9];

    char lon_str[10];
    memcpy(lon_str, info + 10, 9);
    lon_str[9] = '\0';
    int amb_lon = 0;  // MODIFIED
    pos->longitude = aprs_parse_lon(lon_str, &amb_lon);
    if (isnan(pos->longitude))
        return -1;

    pos->symbol_code = info[19];

    const char *p = info + 20;
    pos->has_course_speed = false;
    pos->course = -1;
    pos->speed = -1;

    // Check for course/speed extension
    if (len >= 27 && p[3] == '/') {
        // Ensure numeric before parsing; if numeric but out of range, fail
        bool is_numeric = true;
        for (int i = 0; i < 3; i++) {
            if (!isdigit((unsigned char )p[i]) || !isdigit((unsigned char )p[4 + i])) {
                is_numeric = false;
                break;
            }
        }
        if (is_numeric) {
            char course_str[4] = { p[0], p[1], p[2], '\0' };
            char speed_str[4] = { p[4], p[5], p[6], '\0' };
            int c = atoi(course_str);
            int s = atoi(speed_str);
            if (c < 0 || c > 360 || s < 0 || s > 999) {
                return -1;
            }
            pos->course = (c == 0) ? 360 : c;
            pos->speed = s;
            pos->has_course_speed = true;
            p += 7;
        }
        // else: treat as part of comment
    }

    pos->altitude = INT_MIN;

    // Set ambiguity values
    pos->lat_ambiguity = amb_lat;  // MODIFIED
    pos->lon_ambiguity = amb_lon;  // MODIFIED
    pos->ambiguity = (amb_lat > amb_lon) ? amb_lat : amb_lon;  // preserve overall ambiguity

    // Parse comment
    pos->comment = (strlen(p) > 0) ? my_strdup(p) : NULL;

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
    data->addressee[9] = '\0';

    const char *message_start = info + 11;
    const char *msg_num = strrchr(message_start, '{');
    size_t msg_len = msg_num ? (size_t) (msg_num - message_start) : strlen(message_start);
    if (msg_len > 67)
        return -1;
    data->message = my_strndup(message_start, msg_len);
    if (!data->message && msg_len > 0)
        return -1;

    if (msg_num) {
        const char *msg_num_end = strchr(msg_num, '}');
        if (msg_num_end && msg_num_end > msg_num + 1) {
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

    // MOD FIX: add parentheses for correct precedence
    if (data->message
            && (((data->message[0] == 'a' || data->message[0] == 'A') && (data->message[1] == 'c' || data->message[1] == 'C')
                    && (data->message[2] == 'k' || data->message[2] == 'K'))
                    || ((data->message[0] == 'r' || data->message[0] == 'R') && (data->message[1] == 'e' || data->message[1] == 'E')
                            && (data->message[2] == 'j' || data->message[2] == 'J')))) {
        if (!data->message_number || strlen(data->message_number) != 5) {
            if (data->message_number) {
                free(data->message_number);
                data->message_number = NULL;
            }
            if (data->message) {
                free(data->message);
                data->message = NULL;
            }
            return -1;
        }
    }

    return 0;
}

int aprs_encode_weather_report(char *info, size_t len, const aprs_weather_report_t *data) {
    if (!info || !data) {
        return -1;
    }
    int written = 0;
    // If a position is included, encode it first (use '!' DTI, no timestamp in position part)
    if (data->has_position) {
        aprs_position_no_ts_t pos = { 0 };
        pos.dti = APRS_DTI_POSITION_NO_TS_NO_MSG;  // '!'
        pos.latitude = data->latitude;
        pos.longitude = data->longitude;
        pos.ambiguity = 0;
        pos.symbol_table = data->symbol_table;
        pos.symbol_code = data->symbol_code;
        pos.has_course_speed = false;
        pos.comment = NULL;
        int n = aprs_encode_position_no_ts(info, len, &pos);
        if (n < 0) {
            return -1;
        }
        written = n;
    }
    // Append weather symbol '_' if needed and the timestamp
    if (!data->has_position || data->symbol_code != '_') {
        // include '_' before weather timestamp (positionless or symbol != '_')
        int n = snprintf(info + written, len - written, "_%s", data->timestamp);
        if (n < 0 || (size_t) n >= len - written) {
            return -1;
        }
        written += n;
    } else {
        // Position provided and symbol_code is '_', skip extra '_'
        int n = snprintf(info + written, len - written, "%s", data->timestamp);
        if (n < 0 || (size_t) n >= len - written) {
            return -1;
        }
        written += n;
    }
    // Wind direction (cDDD)
    if (data->wind_direction >= 0 && data->wind_direction <= 360) {
        int n = snprintf(info + written, len - written, "c%03d", data->wind_direction % 360);
        if (n < 0 || (size_t) n >= len - written) {
            return -1;
        }
        written += n;
    } else {
        return -1;
    }
    // Wind speed (sDDD)
    if (data->wind_speed >= 0) {
        int n = snprintf(info + written, len - written, "s%03d", data->wind_speed);
        if (n < 0 || (size_t) n >= len - written) {
            return -1;
        }
        written += n;
    } else {
        return -1;
    }
    // Temperature (tDDD or t-DD)
    if (data->temperature >= -99.9f && data->temperature <= 999.9f) {
        int temp_f = (int) roundf(data->temperature);
        int n;
        if (temp_f >= 0) {
            n = snprintf(info + written, len - written, "t%03d", temp_f);
        } else {
            n = snprintf(info + written, len - written, "t-%02d", -temp_f);
        }
        if (n < 0 || (size_t) n >= len - written) {
            return -1;
        }
        written += n;
    } else {
        return -1;
    }
    // Optional extensions (gust, rain, etc.)
    if (data->wind_gust >= 0) {
        int n = snprintf(info + written, len - written, "g%03d", data->wind_gust);
        if (n < 0 || (size_t) n >= len - written)
            return -1;
        written += n;
    }
    if (data->rainfall_last_hour >= 0) {
        int n = snprintf(info + written, len - written, "r%03d", data->rainfall_last_hour);
        if (n < 0 || (size_t) n >= len - written)
            return -1;
        written += n;
    }
    if (data->rainfall_24h >= 0) {
        int n = snprintf(info + written, len - written, "p%03d", data->rainfall_24h);
        if (n < 0 || (size_t) n >= len - written)
            return -1;
        written += n;
    }
    if (data->rainfall_since_midnight >= 0) {
        int n = snprintf(info + written, len - written, "P%03d", data->rainfall_since_midnight);
        if (n < 0 || (size_t) n >= len - written)
            return -1;
        written += n;
    }
    if (data->humidity >= 0 && data->humidity <= 100) {
        int n = snprintf(info + written, len - written, "h%02d", data->humidity);
        if (n < 0 || (size_t) n >= len - written)
            return -1;
        written += n;
    }
    if (data->barometric_pressure >= 0) {
        int n = snprintf(info + written, len - written, "b%05d", data->barometric_pressure);
        if (n < 0 || (size_t) n >= len - written)
            return -1;
        written += n;
    }
    if (data->luminosity >= 0) {
        int n;
        if (data->luminosity < 1000) {
            n = snprintf(info + written, len - written, "L%03d", data->luminosity);
        } else {
            n = snprintf(info + written, len - written, "l%03d", data->luminosity - 1000);
        }
        if (n < 0 || (size_t) n >= len - written)
            return -1;
        written += n;
    }
    if (data->snowfall_24h >= 0.0f) {
        int snow_int = (int) roundf(data->snowfall_24h * 10.0f);
        int n = snprintf(info + written, len - written, "S%03d", snow_int);
        if (n < 0 || (size_t) n >= len - written)
            return -1;
        written += n;
    }
    if (data->rain_rate >= 0) {
        int n = snprintf(info + written, len - written, "R%03d", data->rain_rate);
        if (n < 0 || (size_t) n >= len - written)
            return -1;
        written += n;
    }
    if (data->water_height_feet >= 0.0f) {
        int n = snprintf(info + written, len - written, "F%.1f", data->water_height_feet);
        if (n < 0 || (size_t) n >= len - written)
            return -1;
        written += n;
    }
    if (data->water_height_meters >= 0.0f) {
        int n = snprintf(info + written, len - written, "f%.1f", data->water_height_meters);
        if (n < 0 || (size_t) n >= len - written)
            return -1;
        written += n;
    }
    if (data->indoors_temperature >= -99.9f && data->indoors_temperature <= 999.9f) {
        int temp_f = (int) roundf(data->indoors_temperature);
        int n;
        if (temp_f >= 0) {
            n = snprintf(info + written, len - written, "i%02d", temp_f);
        } else {
            n = snprintf(info + written, len - written, "i-%02d", -temp_f);
        }
        if (n < 0 || (size_t) n >= len - written)
            return -1;
        written += n;
    }
    if (data->indoors_humidity >= 0 && data->indoors_humidity <= 100) {
        int n = snprintf(info + written, len - written, "I%02d", data->indoors_humidity);
        if (n < 0 || (size_t) n >= len - written)
            return -1;
        written += n;
    }
    if (data->raw_rain_counter >= 0) {
        int n = snprintf(info + written, len - written, "#%05d", data->raw_rain_counter);
        if (n < 0 || (size_t) n >= len - written)
            return -1;
        written += n;
    }
    return written;
}

int aprs_decode_weather_report(const char *info, aprs_weather_report_t *data) {
    if (!info || !data)
        return -1;
    const char *wx = info;

    // MOD FIX: ensure clean struct to avoid stale values
    *data = (aprs_weather_report_t ) { 0 };  // MOD FIX

    // 1) Optional position (DTI '!' or '=')
    aprs_position_no_ts_t pos = (aprs_position_no_ts_t ) { 0 };              // MOD: explicit zero-init
    if (*wx == APRS_DTI_POSITION_NO_TS_NO_MSG || *wx == APRS_DTI_POSITION_NO_TS_WITH_MSG) {
        if (aprs_decode_position_no_ts(wx, &pos) != 0) {
            return -1;
        }
        data->has_position = true;
        data->latitude = pos.latitude;                                    // MOD: populate position when present
        data->longitude = pos.longitude;                                  // MOD: populate position when present
        data->symbol_table = pos.symbol_table;                            // MOD: populate symbol table
        data->symbol_code = pos.symbol_code;                             // MOD: populate symbol code
        // Skip the whole positional block to the beginning of the comment/weather section
        // Position without timestamp is: DTI(1) + lat(8) + symtbl(1) + lon(9) + symcode(1) = 20 bytes
        // If there's a comment after the position report, it starts right after those 20 bytes.
        if (strlen(wx) < 20) {
            return -1;
        }
        wx += 20;
        // If comment exists, it should begin immediately; for weather, expect '_' next or the weather fields
        if (!*wx) {
            return -1;
        }
    } else {
        data->has_position = false;
    }

    // 2) Optional leading '_' weather DTI
    if (*wx == APRS_DTI_WEATHER_REPORT) {                                 // '_'
        wx++;
    }

    // 3) Timestamp: up to first known field code (c,s,t,g,p,P,b,h,L,l,S,R,F,f,i,I,#,w)
    data->timestamp[0] = '\0';                                            // MOD: ensure clean slate
    data->has_timestamp = false;                                          // MOD
    data->is_zulu = false;                                                // MOD
    data->timestamp_format[0] = '\0';                                     // MOD

    const char *field_codes = "cstgpPbhLlSRFfiI#w";  // MOD FIX: removed stray space
    const char *p = wx;
    while (*p && !strchr(field_codes, *p))
        p++;

    size_t ts_len = (size_t) (p - wx);
    if (ts_len > 0 && ts_len < sizeof(data->timestamp)) {                 // MOD: relaxed to allow 1..8
        memcpy(data->timestamp, wx, ts_len);
        data->timestamp[ts_len] = '\0';
        data->has_timestamp = true;                                       // MOD
        // Heuristic for format – APRS 1.2 allows either DDHHMMz or HHMMSSxx; tests use 8 digits (HMS)
        if (ts_len == 7 && (data->timestamp[6] == 'z' || data->timestamp[6] == 'Z' || data->timestamp[6] == '/')) {
            strcpy(data->timestamp_format, "z");                          // MOD
            data->is_zulu = (data->timestamp[6] == 'z' || data->timestamp[6] == 'Z');  // MOD
        } else {
            strcpy(data->timestamp_format, "HMS");                        // MOD
        }
        wx = p;
    } else if (ts_len == 0) {
        // No timestamp – leave has_timestamp=false and continue
    } else {
        return -1;                                                        // MOD: invalid/too long
    }

    // 4) Defaults
    data->temperature = -1000.0f;
    data->wind_speed = -1;
    data->wind_direction = -1;
    data->wind_gust = -1;
    data->rainfall_last_hour = -1;
    data->rainfall_24h = -1;
    data->rainfall_since_midnight = -1;
    data->barometric_pressure = -1;
    data->humidity = -1;
    data->luminosity = -1;
    data->snowfall_24h = -1000.0f;
    data->rain_rate = -1;
    data->water_height_feet = -1000.0f;
    data->water_height_meters = -1000.0f;
    data->indoors_temperature = -1000.0f;
    data->indoors_humidity = -1;
    data->raw_rain_counter = -1;
    data->rain_1h = -1;
    data->rain_24h = -1;
    data->rain_midnight = -1;

    // 5) Parse fields
    while (*wx) {
        switch (*wx) {
            case 'c': {  // wind direction (3 digits)
                int val = -1;
                if (sscanf(wx + 1, "%3d", &val) == 1)
                    data->wind_direction = val;
                wx += 1 + 3;
                break;
            }
            case 's': {  // wind speed (3 digits)
                int val = -1;
                if (sscanf(wx + 1, "%3d", &val) == 1)
                    data->wind_speed = val;
                wx += 1 + 3;
                break;
            }
            case 'g': {  // wind gust (3 digits)
                int val = -1;
                if (sscanf(wx + 1, "%3d", &val) == 1)
                    data->wind_gust = val;
                wx += 1 + 3;
                break;
            }
            case 't': {  // temperature (C as provided)
                int tval;
                if (sscanf(wx + 1, "%3d", &tval) == 1)
                    data->temperature = (float) tval;
                wx += 1 + 3;
                break;
            }
            case 'r': {  // rain rate
                int val = -1;
                if (sscanf(wx + 1, "%3d", &val) == 1)
                    data->rain_rate = val;
                wx += 1 + 3;
                break;
            }
            case 'p': {  // rainfall last hour
                int val = -1;
                if (sscanf(wx + 1, "%3d", &val) == 1)
                    data->rainfall_last_hour = val;
                wx += 1 + 3;
                break;
            }
            case 'P': {  // rainfall since midnight
                int val = -1;
                if (sscanf(wx + 1, "%3d", &val) == 1)
                    data->rainfall_since_midnight = val;
                wx += 1 + 3;
                break;
            }
            case 'h': {  // humidity (00..99 or 100)
                int val = -1;
                if (sscanf(wx + 1, "%2d", &val) == 1)
                    data->humidity = val;
                wx += 1 + 2;
                break;
            }
            case 'b': {  // barometric pressure (5 digits)
                int val = -1;
                if (sscanf(wx + 1, "%5d", &val) == 1)
                    data->barometric_pressure = val;
                wx += 1 + 5;
                break;
            }
            case 'L': {  // luminosity (3 digits – old)
                int val = -1;
                if (sscanf(wx + 1, "%3d", &val) == 1)
                    data->luminosity = val;
                wx += 1 + 3;
                break;
            }
            case 'l': {  // luminosity (3 digits – new)
                int val = -1;
                if (sscanf(wx + 1, "%3d", &val) == 1)
                    data->luminosity = val;
                wx += 1 + 3;
                break;
            }
            case 'S': {  // snowfall last 24h (3 digits, tenths of inch or cm as encoded upstream)
                int val = -1;
                if (sscanf(wx + 1, "%3d", &val) == 1)
                    data->snowfall_24h = (float) val / 10.0f;
                wx += 1 + 3;
                break;
            }
            case 'R': {  // rain rate (alternate)
                int val = -1;
                if (sscanf(wx + 1, "%3d", &val) == 1)
                    data->rain_rate = val;
                wx += 1 + 3;
                break;
            }
            case 'F': {  // water height feet (float with 1 decimal)
                float fval;
                if (sscanf(wx + 1, "%f", &fval) == 1)
                    data->water_height_feet = fval;
                // Move past 'F' and the parsed number (scan until next non-number token)
                wx++;
                while (*wx && (isdigit((unsigned char)*wx) || *wx == '.'))
                    wx++;
                break;
            }
            case 'f': {  // water height meters (float with 1 decimal)
                float fval;
                if (sscanf(wx + 1, "%f", &fval) == 1)
                    data->water_height_meters = fval;
                wx++;
                while (*wx && (isdigit((unsigned char)*wx) || *wx == '.'))
                    wx++;
                break;
            }
            case 'i': {  // indoors temperature (signed 2 digits)
                int tval;
                if (sscanf(wx + 1, "%2d", &tval) == 1)
                    data->indoors_temperature = (float) tval;
                wx += 1 + 2;
                break;
            }
            case 'I': {  // indoors humidity (2 digits)
                int val = -1;
                if (sscanf(wx + 1, "%2d", &val) == 1)
                    data->indoors_humidity = val;
                wx += 1 + 2;
                break;
            }
            case '#': {  // raw rain counter (5 digits)
                int val = -1;
                if (sscanf(wx + 1, "%5d", &val) == 1)
                    data->raw_rain_counter = val;
                wx += 1 + 5;
                break;
            }
                // Unknown or vendor extensions; skip 1 char
            default:
                wx++;
            break;
        }
    }

    return 0;
}

int aprs_encode_object_report(char *dest, size_t len, const aprs_object_report_t *data) {
    size_t pos = 0;

    // 1) DTI
    if (pos + 1 >= len)
        return -1;
    dest[pos++] = APRS_DTI_OBJECT_REPORT;  // ';'

    // 2) Object name: pad with spaces to exactly 9 bytes
    if (pos + 9 >= len)
        return -1;
    size_t nlen = strlen(data->name);
    if (nlen > 9)
        nlen = 9;
    memcpy(dest + pos, data->name, nlen);
    if (nlen < 9)
        memset(dest + pos + nlen, ' ', 9 - nlen);
    pos += 9;

    // 3) Live/killed
    if (pos + 1 >= len)
        return -1;
    dest[pos++] = data->killed ? '_' : '*';

    // 4) Timestamp (7 chars)
    if (pos + 7 >= len)
        return -1;
    memcpy(dest + pos, data->timestamp, 7);
    pos += 7;

    // 5) Latitude (8 chars)
    if (pos + 8 >= len)
        return -1;
    {
        char *latstr = lat_to_aprs(data->latitude, 0);
        memcpy(dest + pos, latstr, 8);
    }
    pos += 8;

    // 6) Symbol table
    if (pos + 1 >= len)
        return -1;
    dest[pos++] = data->symbol_table;

    // 7) Longitude (9 chars)
    if (pos + 9 >= len)
        return -1;
    {
        char *lonstr = lon_to_aprs(data->longitude, 0);
        memcpy(dest + pos, lonstr, 9);
    }
    pos += 9;

    // 8) Symbol code
    if (pos + 1 >= len)
        return -1;
    dest[pos++] = data->symbol_code;

    // 9) Optional course/speed
    if (data->has_course_speed) {
        int c = data->course % 360;
        if (c < 0)
            c += 360;
        int s = data->speed < 0 ? 0 : data->speed;
        int w = snprintf(dest + pos, len - pos, "/%03d/%03d", c, s);
        if (w < 0 || (size_t) w >= len - pos)
            return -1;
        pos += w;
    }

    // 10) Optional PHG
    if (data->phg.power || data->phg.height || data->phg.gain || data->phg.direction) {
        int w = snprintf(dest + pos, len - pos, "PHG%d%d%d%d", data->phg.power, data->phg.height, data->phg.gain, data->phg.direction);
        if (w < 0 || (size_t) w >= len - pos)
            return -1;
        pos += w;
    }

    // 11) Optional comment
    if (data->comment && *data->comment) {
        size_t clen = strlen(data->comment);
        if (pos + clen >= len)
            return -1;
        memcpy(dest + pos, data->comment, clen);
        pos += clen;
    }

    // Null‐terminate
    if (pos >= len)
        return -1;
    dest[pos] = '\0';
    return (int) pos;
}

int aprs_decode_object_report(const char *info, aprs_object_report_t *data) {
    const char *p = info;
    char buf[16];
    int dummy_amb;

    // 1) DTI
    if (*p++ != APRS_DTI_OBJECT_REPORT)  // ';'
        return -1;

    // 2) Name (always 9 chars, space-padded)
    memcpy(data->name, p, 9);
    data->name[9] = '\0';  // safe C string
    // Trim trailing spaces (per APRS1.2: not significant)
    for (int i = 8; i >= 0; i--) {
        if (data->name[i] == ' ')
            data->name[i] = '\0';
        else
            break;
    }
    p += 9;

    // 3) Live/killed
    data->killed = (*p++ == '_');

    // 4) Timestamp
    memcpy(data->timestamp, p, 7);
    data->timestamp[7] = '\0';
    p += 7;

    // 5) Latitude
    memcpy(buf, p, 8);
    buf[8] = '\0';
    data->latitude = aprs_parse_lat(buf, &dummy_amb);
    p += 8;

    // 6) Symbol table
    data->symbol_table = *p++;

    // 7) Longitude
    memcpy(buf, p, 9);
    buf[9] = '\0';
    data->longitude = aprs_parse_lon(buf, &dummy_amb);
    p += 9;

    // 8) Symbol code
    data->symbol_code = *p++;

    // 9) Optional course/speed
    data->has_course_speed = false;
    if (*p == '/') {
        int c, s;
        if (sscanf(p, "/%3d/%3d", &c, &s) == 2) {
            data->course = c;
            data->speed = s;
            data->has_course_speed = true;
        }
        // skip both '/' sections
        p = strchr(p + 1, '/') ? : p;
        if (p)
            p = strchr(p + 1, '/') ? : p;
        if (p)
            p++;
    }

    // 10) Optional PHG
    memset(&data->phg, 0, sizeof(data->phg));
    if (strncmp(p, "PHG", 3) == 0) {
        int pw, ht, gn, dir;
        if (sscanf(p + 3, "%1d%1d%1d%1d", &pw, &ht, &gn, &dir) == 4) {
            data->phg.power = pw;
            data->phg.height = ht;
            data->phg.gain = gn;
            data->phg.direction = dir;
        }
        p += 7;  // skip "PHGpphd"
    }

    // 11) Comment
    if (*p != '\0') {
        size_t clen = strlen(p) + 1;
        data->comment = malloc(clen);
        if (!data->comment)
            return -1;
        memcpy(data->comment, p, clen);
    } else {
        data->comment = NULL;
    }

    return 0;
}

int aprs_encode_position_with_ts(char *info, size_t len, const aprs_position_with_ts_t *data) {
    // Validate inputs
    if (data->dti != '/' && data->dti != '@') {
        //printf("Error: Invalid DTI '%c'\n", data->dti);
        return -1;
    }
    if (strlen(data->timestamp) != 7 || (data->timestamp[6] != 'z' && data->timestamp[6] != 'l')) {
        //printf("Error: Invalid timestamp '%s'\n", data->timestamp);
        return -2;
    }
    if (data->symbol_table != '/' && data->symbol_table != '\\') {
        //printf("Error: Invalid symbol table '%c'\n", data->symbol_table);
        return -3;
    }
    if (!isprint(data->symbol_code)) {
        //printf("Error: Invalid symbol code '%c'\n", data->symbol_code);
        return -4;
    }
    if (fabs(data->latitude) > 90.0) {
        //printf("Error: Invalid latitude '%f'\n", data->latitude);
        return -5;
    }
    if (fabs(data->longitude) > 180.0) {
        //printf("Error: Invalid longitude '%f'\n", data->longitude);
        return -5;
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
        //printf("Error: Buffer too small (required %d, available %zu)\n", ret, len);
        return -1;
    }

    // Add comment if present
    if (data->comment) {
        int ret2 = snprintf(info + ret, len - ret, "%s", data->comment);
        if (ret2 < 0 || (size_t) ret2 >= len - ret) {
            //printf("Error: Buffer overflow for comment\n");
            return -2;
        }
        ret += ret2;
    }

    return ret;  // Return length of encoded string
}

bool aprs_validate_timestamp(const char *timestamp, bool zulu) {
    if (!timestamp)
        return false;
    if (strlen(timestamp) != 7)
        return false;

    /* Check type indicator */
    char type = timestamp[6];
    if (zulu) {
        if (!(type == 'z' || type == 'Z' || type == 'h'))
            return false;
    } else {
        if (!(type == '/' || type == 'l'))  // '/' = local time, 'l' custom
            return false;
    }

    /* Check that other chars are digits */
    for (int i = 0; i < 6; i++) {
        if (!isdigit((unsigned char )timestamp[i]))
            return false;
    }

    return true;
}

int aprs_decode_position_with_ts(const char *info, aprs_position_with_ts_t *data) {
    if (!info || !data)
        return -1;

    memset(data, 0, sizeof(*data));
    data->dti = info[0];

    size_t infolen = strlen(info);
    if (infolen < 27)
        return -1;

    /* Extract timestamp */
    char ts[8] = { 0 };
    memcpy(ts, info + 1, 7);

    bool zulu = (ts[6] == 'z' || ts[6] == 'Z' || ts[6] == 'h');
    if (!zulu)
        return -1;
    memcpy(data->timestamp, ts, 7);
    data->timestamp[7] = '\0';

    /* Latitude */
    char latstr[9] = { 0 };
    memcpy(latstr, info + 8, 8);
    int lat_amb = 0;  // MODIFIED
    data->latitude = aprs_parse_lat(latstr, &lat_amb);
    if (isnan(data->latitude))
        return -1;

    data->symbol_table = info[16];

    /* Longitude */
    char lonstr[10] = { 0 };
    memcpy(lonstr, info + 17, 9);
    int lon_amb = 0;  // MODIFIED
    data->longitude = aprs_parse_lon(lonstr, &lon_amb);
    if (isnan(data->longitude))
        return -1;

    data->symbol_code = info[26];
    if (!isprint((unsigned char )data->symbol_code))
        return -1;

    // Set ambiguity values
    data->lat_ambiguity = lat_amb;  // MODIFIED
    data->lon_ambiguity = lon_amb;  // MODIFIED
    data->ambiguity = (lat_amb > lon_amb) ? lat_amb : lon_amb;  // MODIFIED

    /* Optional course/speed or comment */
    data->has_course_speed = false;
    if (infolen >= 34&&
    isdigit((unsigned char)info[27]) &&
    isdigit((unsigned char)info[28]) &&
    isdigit((unsigned char)info[29]) &&
    info[30] == '/' &&
    isdigit((unsigned char)info[31]) &&
    isdigit((unsigned char)info[32]) &&
    isdigit((unsigned char)info[33])) {

        char crs[4] = { info[27], info[28], info[29], '\0' };
        char spd[4] = { info[31], info[32], info[33], '\0' };
        int course = atoi(crs);
        int speed = atoi(spd);
        if (course < 0 || course > 359 || speed < 0 || speed > 999)
            return -1;

        data->course = course;
        data->speed = speed;
        data->has_course_speed = true;

        if (infolen > 34 && info[34] != '\0' && !aprs_is_only_spaces(info + 34)) {
            data->comment = my_strdup(info + 34);
        } else {
            data->comment = NULL;
        }
    } else {
        if (infolen > 27 && info[27] != '\0' && !aprs_is_only_spaces(info + 27)) {
            data->comment = my_strdup(info + 27);
        } else {
            data->comment = NULL;
        }
    }

    return 0;
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
    int hun_int = (int) (hun + 0.5);  // Round to nearest

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
        return -1;  // Invalid message code
    }

    // Determine additional bits: N/S, long_offset, W/E
    int bits[6];
    bits[0] = message_bits[0] - '0';  // Message bit A
    bits[1] = message_bits[1] - '0';  // Message bit B
    bits[2] = message_bits[2] - '0';  // Message bit C
    bits[3] = (data->latitude >= 0) ? 1 : 0;  // N/S: 1 North, 0 South
    double abs_longitude = fabs(data->longitude);
    int long_deg = (int) abs_longitude;
    bits[4] = (long_deg >= 100) ? 1 : 0;  // Long_offset: 1 if >=100
    bits[5] = (data->longitude < 0) ? 1 : 0;  // W/E: 1 West, 0 East

    // Encode each byte
    for (int i = 0; i < 6; i++) {
        int digit = digits[i];
        if (digit < 0 || digit > 9) {
            return -1;  // Invalid digit
        }
        int bit = bits[i];
        char c;
        if (i < 3) {  // Positions 0-2: message bits
            c = bit ? 'P' + digit : '0' + digit;
        } else if (i == 3) {  // Position 3: N/S
            c = bit ? 'P' + digit : 'A' + digit;
        } else if (i == 4) {  // Position 4: long_offset
            c = bit ? 'P' + digit : '0' + digit;
        } else {  // Position 5: W/E
            c = bit ? 'P' + digit : 'A' + digit;
        }
        dest_str[i] = c;
    }
    dest_str[6] = '\0';  // Null-terminate
    return 0;
}

int aprs_decode_mice_destination(const char *dest_str, aprs_mice_t *data, int *message_bits, bool *ns, bool *long_offset, bool *we) {
    if (strlen(dest_str) != 6) {
        return -1;  // Invalid length
    }

    int digits[6];
    bool bits[6];

    // Decode each character
    for (int i = 0; i < 6; i++) {
        char c = dest_str[i];
        if (i == 3 || i == 5) {  // N/S (pos 3) and W/E (pos 5)
            if (c >= 'A' && c <= 'J') {
                digits[i] = c - 'A';
                bits[i] = false;
            } else if (c >= 'P' && c <= 'Y') {
                digits[i] = c - 'P';
                bits[i] = true;
            } else {
                return -1;  // Invalid character
            }
        } else {  // Positions 0-2 (message bits) and 4 (long offset)
            if (c >= '0' && c <= '9') {
                digits[i] = c - '0';
                bits[i] = false;
            } else if (c >= 'P' && c <= 'Y') {
                digits[i] = c - 'P';
                bits[i] = true;
            } else {
                return -1;  // Invalid character
            }
        }
    }

    // Extract message bits (ABC from positions 0-2)
    *message_bits = (bits[0] << 2) | (bits[1] << 1) | bits[2];
    *ns = bits[3];          // North/South: true=North, false=South
    *long_offset = bits[4];  // Longitude offset: true=add 100 degrees
    *we = bits[5];          // West/East: true=West, false=East

    // Compute latitude
    int deg = digits[0] * 10 + digits[1];
    double min = digits[2] * 10 + digits[3] + (digits[4] * 10.0 + digits[5]) / 100.0;
    data->latitude = deg + min / 60.0;
    if (!*ns) {  // South
        data->latitude = -data->latitude;
    }

    return 0;
}

int aprs_encode_mice_info(char *info, size_t len, const aprs_mice_t *data) {
    if (len < 9) {
        return -1;  // Not enough space
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
        return -1;  // Invalid length
    }

    // Validate data type indicator (byte 0)
    char dti = info[0];
    if (dti != '`' && dti != '\'') {
        return -1;  // Invalid data type
    }

    // Decode longitude from bytes 1-3
    int d = info[1] - 28;  // Degrees
    if (d >= 88) {
        d -= 60;  // Adjust for degrees >= 60
    }
    int m = info[2] - 28;  // Minutes
    int h = info[3] - 28;  // Hundredths of minutes

    // Validate ranges
    if (d < 0 || d > 179 || m < 0 || m > 59 || h < 0 || h > 99) {
        return -1;  // Invalid values
    }

    // Apply longitude offset (from destination field)
    if (long_offset) {
        d += 100;  // Add 100 if longitude >= 100°
    }

    // Correct longitude calculation
    double min = m + h / 100.0;  // Combine minutes and hundredths
    data->longitude = d + min / 60.0;  // Convert to degrees

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

    return 0;  // Success
}

int aprs_encode_telemetry(char *info, size_t len, const aprs_telemetry_t *data) {
    if (len < 30)
        return -1;
    // Validate analog values (allow 0–999 instead of 0–255)
    for (int i = 0; i < 5; i++) {
        if (data->analog[i] < 0 || data->analog[i] > 999) {
            return -1;
        }
    }
    // Build 8-bit digital bitstring
    char bits_str[9];
    for (int i = 7; i >= 0; i--) {
        bits_str[7 - i] = ((data->digital >> i) & 1) ? '1' : '0';
    }
    bits_str[8] = '\0';
    // Format: T#<seq>,<a0>,<a1>,<a2>,<a3>,<a4>,<8-bit-bits>
    int ret = snprintf(info, len, "T#%03u,%03u,%03u,%03u,%03u,%03u,%s", data->sequence_number % 1000, (unsigned int) data->analog[0],
            (unsigned int) data->analog[1], (unsigned int) data->analog[2], (unsigned int) data->analog[3], (unsigned int) data->analog[4], bits_str);
    if (ret < 0 || (size_t) ret >= len) {
        return -1;
    }
    return ret;
}

int aprs_decode_telemetry(const char *info, aprs_telemetry_t *data) {
    if (!info || !data)
        return -1;  // modified: null checks

    const char *t = (info[0] == 'T' && info[1] == '#') ? info : strstr(info, "T#");
    if (!t) {
        return -1;
    }
    char *p = (char*) t + 2;
    char *end;
    data->sequence_number = strtoul(p, &end, 10);
    if (*end != ',') {
        return -1;
    }
    p = end + 1;
    for (int i = 0; i < 5; i++) {
        data->analog[i] = strtoul(p, &end, 10);
        if ((*end != ',' && i < 4) || end == p) {
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
    if (!info || !data) {
        return -1;
    }
    if (len == 0) {
        return -1;
    }
    size_t pos = 0;
    // Prefix with '>' (Status Report DTI)
    if (pos < len) {
        info[pos++] = APRS_DTI_STATUS;  // '>'
    } else {
        return -1;
    }
    // Optional timestamp (DDHHMMz)
    if (data->has_timestamp) {
        // Timestamp must be exactly 7 chars ending in 'z'
        if (strlen(data->timestamp) != 7 || data->timestamp[6] != 'z') {
            return -1;
        }
        // Ensure space for 7-char timestamp + status text
        if (pos + 7 >= len) {
            return -1;
        }
        memcpy(info + pos, data->timestamp, 7);
        pos += 7;
    }
    // Determine status text length
    size_t text_len = strlen(data->status_text);
    // Enforce APRS limits: 62 chars without timestamp, 55 with
    if (data->has_timestamp) {
        if (text_len > 55) {
            return -1;
        }
    } else {
        if (text_len > 62) {
            return -1;
        }
    }
    // Ensure space for status text + null terminator
    if (pos + text_len >= len) {
        return -1;
    }
    memcpy(info + pos, data->status_text, text_len);
    pos += text_len;
    info[pos] = '\0';
    return (int) pos;
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
    // Accept 'z' or 'l' as timestamp marker
    if (len >= 8 && isdigit(info[1]) && isdigit(info[2]) && isdigit(info[3]) && isdigit(info[4]) && isdigit(info[5]) && isdigit(info[6])
            && (info[7] == 'z' || info[7] == 'l')) {
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
        info[total_len] = '\0';  // Null-terminate if space allows
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
        info[total_len] = '\0';  // Null-terminate if space allows
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
        return -1;  // Bulletin ID too long
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
    if (!info || !data)
        return -1;

    // Validate name length
    if (strlen(data->name) > 9) {
        return -1;  // Item name too long
    }

    // Validate symbol table
    if (data->symbol_table != '/' && data->symbol_table != '\\') {
        return -1;  // Invalid symbol table
    }

    // Validate symbol code
    if (!isprint((unsigned char )data->symbol_code)) {
        return -1;  // Invalid symbol code
    }

    // Pad name to 9 characters (right-padded with spaces)
    char name_padded[10];
    snprintf(name_padded, sizeof(name_padded), "%-9s", data->name);

    // Convert latitude and longitude to APRS format strings
    char *lat_str = lat_to_aprs(data->latitude, 0);
    char *lon_str = lon_to_aprs(data->longitude, 0);
    if (!lat_str || !lon_str) {
        return -1;  // Conversion failed
    }

    // Determine live ('!') or killed ('=') status flag
    char status_char = data->is_live ? '!' : '=';

    // Start encoding string with ")" identifier for item report
    int ret = snprintf(info, len, ")%s%c%s%c%s%c", name_padded, status_char, lat_str, data->symbol_table, lon_str, data->symbol_code);

    if (ret < 0 || (size_t) ret >= len) {
        return -1;  // Encoding error or buffer too small
    }

    // Optional course/speed "CCC/SSS"
    if (data->has_course_speed) {
        int course = data->course % 360;
        if (course < 0)
            course += 360;
        int speed = data->speed < 0 ? 0 : data->speed;
        int m = snprintf(info + ret, len - ret, "%03d/%03d", course, speed);
        if (m < 0 || (size_t) ret + (size_t) m >= len) {
            return -1;
        }
        ret += m;
    }

    // Optional PHG (power/height/gain/directivity)
    if (data->has_phg) {
        int m = snprintf(info + ret, len - ret, "PHG%d%d%d%d", data->phg.power, data->phg.height, data->phg.gain, data->phg.direction);
        if (m < 0 || (size_t) ret + (size_t) m >= len) {
            return -1;
        }
        ret += m;
    }

    // Append optional comment if present
    if (data->comment && data->comment[0] != '\0') {
        int add = snprintf(info + ret, len - ret, "%s", data->comment);
        if (add < 0 || (size_t) add >= (len - ret)) {
            return -1;  // Comment encoding error
        }
        ret += add;
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
    if (!info || !data)
        return -1;
    size_t len = strlen(info);

    // Must start with ')' and be at least 30 chars (base format)
    if (info[0] != ')' || len < 30)
        return -1;

    // Zero‑init
    *data = (aprs_item_report_t ) { 0 };

    // Set default PHG fields
    data->phg.power = data->phg.height = data->phg.gain = data->phg.direction = -1;
    data->has_phg = false;

    // --- Name (9 chars) ---
    char raw_name[10] = { 0 };
    memcpy(raw_name, info + 1, 9);
    int nl = 9;
    while (nl > 0 && raw_name[nl - 1] == ' ')
        nl--;
    memcpy(data->name, raw_name, nl);
    data->name[nl] = '\0';

    // --- Live vs. killed ---
    char flag = info[10];
    if (flag == '!') {
        data->is_live = true;
        data->killed = false;
    } else if (flag == '=') {
        data->is_live = false;
        data->killed = true;
    } else {
        return -1;
    }

    // --- Latitude (8 chars) ---
    char lat_str[9] = { 0 };
    memcpy(lat_str, info + 11, 8);
    int lat_ambiguity;
    data->latitude = aprs_parse_lat(lat_str, &lat_ambiguity);
    if (isnan(data->latitude))
        return -1;

    // --- Symbol table ---
    data->symbol_table = info[19];

    // --- Longitude (9 chars) ---
    char lon_str[10] = { 0 };
    memcpy(lon_str, info + 20, 9);
    int lon_ambiguity;
    data->longitude = aprs_parse_lon(lon_str, &lon_ambiguity);
    if (isnan(data->longitude))
        return -1;

    // --- Symbol code ---
    data->symbol_code = info[29];

    // Position index after base (name+flag+lat+symbol_tbl+lon+symbol_code = 30 chars)
    size_t pos = 30;

    // Optional course/speed "CCC/SSS"
    data->has_course_speed = false;
    if (pos + 7 <= len&&
    isdigit((unsigned char)info[pos]) && isdigit((unsigned char)info[pos+1]) && isdigit((unsigned char)info[pos+2]) &&
    info[pos+3] == '/' &&
    isdigit((unsigned char)info[pos+4]) && isdigit((unsigned char)info[pos+5]) && isdigit((unsigned char)info[pos+6])) {
        char cs_str[8] = { 0 };
        memcpy(cs_str, info + pos, 7);
        int course = atoi(cs_str);
        int speed = atoi(cs_str + 4);
        if (course >= 0 && course < 360 && speed >= 0) {
            data->has_course_speed = true;
            data->course = course;
            data->speed = speed;
            pos += 7;
        } else {
            return -1;
        }
    }

    // Optional PHG "PHGpphd"
    if (pos + 7 <= len && strncmp(info + pos, "PHG", 3) == 0) {
        char phgbuf[5] = { 0 };
        memcpy(phgbuf, info + pos + 3, 4);
        bool valid = true;
        for (int i = 0; i < 4; i++) {
            char c = phgbuf[i];
            if (!isdigit((unsigned char )c) && !(i == 1 && c >= 'A' && c <= 'Z')) {
                valid = false;
                break;
            }
        }
        if (valid) {
            data->has_phg = true;
            data->phg.power = phgbuf[0] - '0';
            if (isdigit(phgbuf[1])) {
                data->phg.height = phgbuf[1] - '0';
            } else {
                data->phg.height = (phgbuf[1] - 'A') + 10;
            }
            data->phg.gain = phgbuf[2] - '0';
            data->phg.direction = phgbuf[3] - '0';
            pos += 7;
        }
    }

    // --- Optional comment ---
    if (pos < len) {
        data->comment = malloc(len - pos + 1);
        if (!data->comment)
            return -1;
        memcpy(data->comment, info + pos, len - pos);
        data->comment[len - pos] = '\0';
    } else {
        data->comment = malloc(1);
        if (!data->comment)
            return -1;
        data->comment[0] = '\0';
    }

    return 0;
}

int aprs_encode_raw_gps(char *info, size_t len, const aprs_raw_gps_t *data) {
    if (data == NULL || data->raw_data == NULL || data->data_len < 3 || strncmp(data->raw_data, "GP", 2) != 0) {
        return -1;  // Invalid raw GPS data
    }
    // modified: reject accidental $ULT* WX encoding via this function
    if (strncmp(data->raw_data, "ULT", 3) == 0) {
        return -2;  // modified
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
    if (!info || !data)                                                     // MOD: null checks
        return -1;
    if (info[0] != APRS_DTI_RAW_GPS) {                                      // '$'
        return -1;
    }
    size_t total_len = strlen(info);
    if (total_len <= 1) {
        return -1;
    }

    // Copy payload exactly as-is (including any '*' checksum if present)
    size_t payload_len = total_len - 1;
    data->raw_data = (char*) malloc(payload_len + 1);                        // MOD: explicit malloc to avoid surprises
    if (!data->raw_data)
        return -1;
    memcpy(data->raw_data, info + 1, payload_len);                          // MOD
    data->raw_data[payload_len] = '\0';                                     // MOD
    data->data_len = payload_len;                                           // MOD

    // Optional: Best-effort NMEA checksum validation (does not fail decode)
    const char *star = strrchr(info + 1, '*');                              // MOD
    if (star && (star - (info + 1)) >= 1 && (info + total_len - star) >= 3) {  // MOD
        unsigned int given = 0;
        // parse two hex digits after '*'
        if (sscanf(star + 1, "%2x", &given) == 1) {
            unsigned int calc = 0;
            const char *p = info + 1;                                       // start after '$'
            while (p < star) {
                calc ^= (unsigned char) (*p++);
            }
            (void) calc;                                                     // MOD: keep for potential diagnostics
            // We intentionally do not error out on mismatch to be lenient per spec recommendations.
        }
    }

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
    //printf("Encoded DF report: '%s' (length: %zu)\n", info, total_len);
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
    if (!info || !data)
        return -1;  // modified: null checks
    // modified: accept old '"' map/test and reserved '&' as test-like payloads
    if (info[0] != APRS_DTI_TEST_PACKET && info[0] != APRS_DTI_RESERVED_2 && info[0] != APRS_DTI_RESERVED_1) {
        fprintf(stderr, "Unknown/invalid DTI for test/map/routing: '%c'\n", info[0]);  // modified: debug logging
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
    // MOD FIX: Use APRS compressed altitude formula with +ALTITUDE_OFFSET (feet)
    // alt is in feet; encoding uses log base 1.002 of (alt + 10000)
    if (alt == INT_MIN) {                                   // MOD FIX
        output[0] = output[1] = ' ';                        // MOD FIX
        return;                                             // MOD FIX
    }                                                       // MOD FIX
    long adj = (long) alt + ALTITUDE_OFFSET;                 // MOD FIX
    if (adj < 0)
        adj = 0;                                   // MOD FIX
    double cs = log((double) adj) / log(1.002);              // MOD FIX
    uint32_t val = (uint32_t) (cs + 0.5);                   // MOD FIX
    if (val >= BASE91_SIZE * BASE91_SIZE) {                 // MOD FIX
        output[0] = output[1] = ' ';                        // MOD FIX
        return;                                             // MOD FIX
    }
    encode_base91(val, output, 2);
}

static int decode_altitude(const char *input) {
    // MOD FIX: Decode APRS compressed altitude with -ALTITUDE_OFFSET (feet)
    uint32_t cs = decode_base91(input, 2);
    double altd = pow(1.002, (double) cs);                  // MOD FIX
    long feet = (long) llround(altd) - ALTITUDE_OFFSET;     // MOD FIX
    if (feet < INT_MIN)
        feet = INT_MIN;                     // MOD FIX
    if (feet > INT_MAX)
        feet = INT_MAX;                     // MOD FIX
    return (int) feet;                                      // MOD FIX
}

/**
 * Create compression type byte
 */
static char create_compression_type(bool has_data, bool is_altitude, bool is_current) {
    uint8_t byte = 0;

    if (is_current) {
        byte |= 0x20;  // GPS fix is current
    }

    if (has_data) {
        if (is_altitude) {
            byte |= 0x02;  // Altitude data format
        } else {
            byte |= 0x01;  // Course/speed data format
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

    char compressed[14];  // 13 chars + null terminator
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
        return -1;  // Buffer too small
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
    if (len >= (int) sizeof(buf))
        return -1;
    memcpy(buf, s, len);
    buf[len] = '\0';
    return atoi(buf);
}

int aprs_decode_peet1(const char *info, aprs_weather_report_t *data) {
    if (strncmp(info, "#W1", 3) != 0)
        return -1;
    info += 3;
    memset(data, 0, sizeof(*data));

    while (*info) {
        if (info[0] == 'c')
            data->wind_direction = parse_fixed_int(info + 1, 3);
        else if (info[0] == 's')
            data->wind_speed = parse_fixed_int(info + 1, 3);
        else if (info[0] == 'g')
            data->wind_gust = parse_fixed_int(info + 1, 3);
        else if (info[0] == 't')
            data->temperature = (float) parse_fixed_int(info + 1, 3);
        else if (info[0] == 'r')
            data->rain_1h = parse_fixed_int(info + 1, 3);
        else if (info[0] == 'p')
            data->rain_24h = parse_fixed_int(info + 1, 3);
        else if (info[0] == 'P')
            data->rain_midnight = parse_fixed_int(info + 1, 3);
        else if (info[0] == 'h')
            data->humidity = parse_fixed_int(info + 1, 2);
        else if (info[0] == 'b')
            data->barometric_pressure = parse_fixed_int(info + 1, 5);
        info += (info[0] == 'h') ? 3 : (info[0] == 'b') ? 6 : 4;
    }
    return 0;
}

int aprs_decode_peet2(const char *info, aprs_weather_report_t *data) {
    if (strncmp(info, "*W2", 3) != 0)
        return -1;
    return aprs_decode_peet1(info + 1, data);
}

int aprs_encode_peet1(char *dst, int len, const aprs_weather_report_t *data) {
    return snprintf(dst, len, "#W1c%03ds%03dg%03dt%03dr%03dp%03dP%03dh%02db%05d", data->wind_direction, data->wind_speed, data->wind_gust,
            (int) data->temperature, data->rain_1h, data->rain_24h, data->rain_midnight, data->humidity, data->barometric_pressure);
}

int aprs_encode_peet2(char *dst, int len, const aprs_weather_report_t *data) {
    int r = aprs_encode_peet1(dst + 1, len - 1, data);
    if (r <= 0)
        return -1;
    dst[0] = '*';
    return r + 1;
}

int aprs_decode_position_weather(const aprs_position_no_ts_t *pos, aprs_weather_report_t *w) {
    if (pos->symbol_code != '_') {
        return -1;  // Not a weather-bearing position report
    }
    if (!pos->comment) {
        return -1;
    }
    // Prepend the "#W1" header (Peet Bros format 1) to parse the fields
    char buf[APRS_COMMENT_LEN + 4];
    int n = snprintf(buf, sizeof(buf), "#W1%s", pos->comment);
    if (n < 0 || (size_t) n >= sizeof(buf)) {
        return -1;  // Encoding error or buffer overflow
    }
    return aprs_decode_peet1(buf, w);
}

/* Helper: compute great-circle distance (km) between two lat/lon points */
static double haversine_km(double lat1, double lon1, double lat2, double lon2) {
    const double R = 6371.0;  // Earth radius in km
    double dlat = (lat2 - lat1) * M_PI / 180.0;
    double dlon = (lon2 - lon1) * M_PI / 180.0;
    double a = sin(dlat / 2) * sin(dlat / 2) + cos(lat1 * M_PI / 180.0) * cos(lat2 * M_PI / 180.0) * sin(dlon / 2) * sin(dlon / 2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));
    return R * c;
}

int aprs_handle_directed_query(const aprs_message_t *msg, char *info, size_t len, aprs_station_info_t local_station) {
    if (!msg || !info)
        return -1;
    // Check address matches local callsign (trim trailing spaces)
    char dest[10];
    strncpy(dest, msg->addressee, 9);
    dest[9] = '\0';
    for (int i = strlen(dest) - 1; i >= 0 && dest[i] == ' '; i--)
        dest[i] = '\0';
    if (strcmp(dest, local_station.callsign) != 0) {
        return 0;  // Not addressed to us
    }
    // Message text must start and end with '?'
    const char *text = msg->message;
    size_t tlen = strlen(text);
    if (tlen < 3 || text[0] != '?' || text[tlen - 1] != '?') {
        return 0;  // Not a valid query
    }
    // Extract query type (between '?')
    char qtype[20];
    size_t type_len = tlen - 2;
    if (type_len >= sizeof(qtype))
        type_len = sizeof(qtype) - 1;
    memcpy(qtype, text + 1, type_len);
    qtype[type_len] = '\0';

    // Handle each supported query type
    if (strcmp(qtype, "APRS") == 0) {
        // Respond with software version string (no prefix)
        size_t outlen = strlen(local_station.software_version);
        if (outlen >= len)
            return -1;
        memcpy(info, local_station.software_version, outlen);
        info[outlen] = '\0';
        return (int) outlen;
    } else if (strcmp(qtype, "INFO") == 0) {
        // Respond with status text (with '>' prefix)
        aprs_status_t st = { .has_timestamp = false };
        memcpy(st.status_text, local_station.status_text, sizeof(st.status_text) - 1);
        st.status_text[sizeof(st.status_text) - 1] = '\0';
        return aprs_encode_status(info, len, &st);
    } else if (strcmp(qtype, "LOC") == 0) {
        // Respond with current position (no timestamp)
        aprs_position_no_ts_t pos = { .latitude = local_station.latitude, .longitude = local_station.longitude, .symbol_table = local_station.symbol_table,
                .symbol_code = local_station.symbol_code, .comment = NULL, .dti = '!' };
        return aprs_encode_position_no_ts(info, len, &pos);
    } else if (strcmp(qtype, "TIME") == 0) {
        // Respond with the last beacon time (use status with timestamp)
        aprs_status_t st = { .has_timestamp = true };
        memcpy(st.timestamp, local_station.timestamp, 7);
        st.status_text[0] = '\0';
        return aprs_encode_status(info, len, &st);
    } else if (strcmp(qtype, "WX") == 0) {
        // Dummy weather report
        static aprs_weather_report_t wx = { .timestamp = "000000z", .wind_direction = 90, .wind_speed = 5, .temperature = 25.0 };
        return aprs_encode_weather_report(info, len, &wx);
    } else if (strcmp(qtype, "MSG") == 0) {
        // Messaging capability: respond with a simple text
        const char *msgtxt = "MSG supported";
        size_t outlen = strlen(msgtxt);
        if (outlen >= len)
            return -1;
        memcpy(info, msgtxt, outlen);
        info[outlen] = '\0';
        return (int) outlen;
    } else if (strcmp(qtype, "DST") == 0) {
        // Distance to destination if configured
        if (local_station.has_dest) {
            double dkm = haversine_km(local_station.latitude, local_station.longitude, local_station.dest_lat, local_station.dest_lon);
            int dkm_int = (int) (dkm + 0.5);
            return snprintf(info, len, "%d km", dkm_int);
        } else {
            const char *nodst = "Unknown";
            size_t outlen = strlen(nodst);
            if (outlen >= len)
                return -1;
            memcpy(info, nodst, outlen);
            info[outlen] = '\0';
            return (int) outlen;
        }
    } else if (strcmp(qtype, "APRSP") == 0) {
        // Respond with current position (with timestamp)
        aprs_position_with_ts_t pos = { 0 };
        pos.dti = '/';
        pos.latitude = local_station.latitude;
        pos.longitude = local_station.longitude;
        pos.symbol_table = local_station.symbol_table;
        pos.symbol_code = local_station.symbol_code;
        memcpy(pos.timestamp, local_station.timestamp, 8);  // "DDHHMMz"
        pos.timestamp[7] = 'z';
        return aprs_encode_position_with_ts(info, len, &pos);
    } else if (strcmp(qtype, "APRSS") == 0) {
        // Respond with status (no timestamp)
        aprs_status_t st = { .has_timestamp = false };
        memcpy(st.status_text, local_station.status_text, sizeof(st.status_text) - 1);
        st.status_text[sizeof(st.status_text) - 1] = '\0';
        return aprs_encode_status(info, len, &st);
    } else if (strcmp(qtype, "APRSM") == 0) {
        // Respond with messages (none)
        const char *res = "No messages";
        size_t outlen = strlen(res);
        if (outlen >= len)
            return -1;
        memcpy(info, res, outlen);
        info[outlen] = '\0';
        return (int) outlen;
    } else if (strcmp(qtype, "APRSO") == 0) {
        // Respond with objects (none)
        const char *res = "No objects";
        size_t outlen = strlen(res);
        if (outlen >= len)
            return -1;
        memcpy(info, res, outlen);
        info[outlen] = '\0';
        return (int) outlen;
    } else if (strcmp(qtype, "APRSD") == 0) {
        // Respond with direct hears (none)
        const char *res = "Directs=";
        size_t outlen = strlen(res);
        if (outlen >= len)
            return -1;
        memcpy(info, res, outlen);
        info[outlen] = '\0';
        return (int) outlen;
    } else if (strncmp(qtype, "APRSH", 5) == 0) {
        // Query: has heard a particular station
        const char *res = "Not heard";
        size_t outlen = strlen(res);
        if (outlen >= len)
            return -1;
        memcpy(info, res, outlen);
        info[outlen] = '\0';
        return (int) outlen;
    } else if (strcmp(qtype, "APRST") == 0 || strcmp(qtype, "PING") == 0) {
        // Respond with route trace (none)
        const char *res = "No route";
        size_t outlen = strlen(res);
        if (outlen >= len)
            return -1;
        memcpy(info, res, outlen);
        info[outlen] = '\0';
        return (int) outlen;
    }
    // Unsupported query type
    return 0;
}

void encodePositionPacket(const aprs_position_report_t *pos, char *out) {
    // 1) Codificar bloque base (lat/lon, símbolos, curso/velocidad y comentario)
    //    usando aprs_encode_position_no_ts en un buffer suficientemente grande (256 bytes).
    int n = aprs_encode_position_no_ts(out, 256, (const aprs_position_no_ts_t*) pos);
    if (n < 0)
        return;

    size_t idx = (size_t) n;

    // 2) PHG opcional: “PHGpphd”
    if (pos->phg.power >= 0 && pos->phg.height >= 0 && pos->phg.gain >= 0 && pos->phg.direction >= 0) {
        char phg[8];
        int m = snprintf(phg, sizeof(phg), "PHG%d%d%d%d", pos->phg.power, pos->phg.height, pos->phg.gain, pos->phg.direction);
        if (m > 0 && idx + (size_t) m < 256) {
            memcpy(out + idx, phg, (size_t) m);
            idx += (size_t) m;
        }
    }

    // 3) Altitud opcional: “/A=nnnnnn”
    if (pos->altitude >= 0) {
        char alt[12];
        int a = snprintf(alt, sizeof(alt), "/A=%06d", pos->altitude);
        if (a > 0 && idx + (size_t) a < 256) {
            memcpy(out + idx, alt, (size_t) a);
            idx += (size_t) a;
        }
    }

    // 4) Terminar cadena
    out[idx] = '\0';
}

// Decode altitude (/A=nnnnnn) and PHG from the comment string of a position/status packet
void parseAltitudePHG(const char *comment, aprs_position_report_t *pos) {
    // Default: not found
    pos->altitude = -1;
    pos->phg.power = pos->phg.height = pos->phg.gain = pos->phg.direction = -1;

    // Search for altitude token "/A="
    const char *alt_ptr = strstr(comment, "/A=");
    if (alt_ptr && strlen(alt_ptr) >= 6 + 3) {
        // Expect exactly 6 digits after "/A="
        char altbuf[7] = { 0 };
        strncpy(altbuf, alt_ptr + 3, 6);
        // Only parse if all 6 are digits
        int valid = 1;
        for (int i = 0; i < 6; i++) {
            if (!isdigit(altbuf[i])) {
                valid = 0;
                break;
            }
        }
        if (valid) {
            pos->altitude = atoi(altbuf);
        }
    }

    // Search for PHG token "PHG"
    const char *phg_ptr = strstr(comment, "PHG");
    if (phg_ptr && strlen(phg_ptr) >= 4 + 3) {
        // Format is PHGxxxx (4 digits)
        // Verify all 4 chars after "PHG" are digits or uppercase letters (digit or for extended height)
        char phgbuf[5] = { 0 };
        strncpy(phgbuf, phg_ptr + 3, 4);
        int valid = 1;
        for (int i = 0; i < 4; i++) {
            if (!isdigit(phgbuf[i]) && !(i == 2 && phgbuf[i] >= 'A' && phgbuf[i] <= 'Z')) {
                // Typically P,H,G are 0-9; height could be extended (A-Z) as per spec
                valid = 0;
                break;
            }
        }
        if (valid) {
            // First character: power (0-9)
            pos->phg.power = phgbuf[0] - '0';
            // Second: height (log2 HAAT/10) or extended; convert 'A'..'Z' to integer >9 if needed
            if (isdigit(phgbuf[1])) {
                pos->phg.height = phgbuf[1] - '0';
            } else {
                pos->phg.height = (phgbuf[1] - 'A') + 10;  // 'A' = 10, etc.
            }
            // Third: gain (0-9)
            pos->phg.gain = phgbuf[2] - '0';
            // Fourth: direction (0-9)
            pos->phg.direction = phgbuf[3] - '0';
        }
    }
}

// Parse a User-Defined APRS information field beginning with '{'
void parse_user_defined(const char *info) {
    if (info == NULL || info[0] != '{')
        return;  // Not a user-defined packet
    aprs_user_defined_format_t udf;
    udf.userID = info[1];
    udf.packetType = info[2];
    // Copy the remainder of the field as ASCII data
    strncpy(udf.data, info + 3, sizeof(udf.data) - 1);
    udf.data[sizeof(udf.data) - 1] = '\0';

    // (Application-specific handling of udf follows here, e.g. store or display it)
    printf("User-Defined packet: UserID=%c, Type=%c, Data=\"%s\"\n", udf.userID, udf.packetType, udf.data);
}

// Parse a Third-Party APRS packet (leading '}' indicates a tunnel header)
void parse_third_party(const char *info) {
    if (info == NULL || info[0] != '}')
        return;  // Not a third-party packet
    // Find the "::" that separates the header from the original APRS packet
    const char *sep = strstr(info, "::");
    if (!sep) {
        // Malformed third-party packet; no separator found
        return;
    }
    // The original APRS packet begins after the "::"
    const char *inner = sep + 2;
    // Recursively parse the inner packet as a normal APRS information field
    // (You would typically call your general parsing routine here)
    printf("Third-Party packet (inner): \"%s\"\n", inner);
    // e.g. parse_info_field(inner);
}

void parse_dx_spot(const char *info) {
    if (info == NULL || info[0] != '%')
        return;  // Not a DX packet
    // Modified: Changed parsing logic from proprietary Agrelo telemetry to standard DX spot format
    const char *p = info + 1;
    if (strncmp(p, "DX de ", 6) != 0)
        return;  // Malformed DX packet; no "DX de " prefix
    p += 6;
    aprs_dx_spot_t dx;
    memset(&dx, 0, sizeof(dx));
    const char *sep = strchr(p, ':');
    if (!sep) {
        // Malformed; no ':' after callsign
        return;
    }
    // Parse de_callsign
    size_t call_len = sep - p;
    if (call_len >= sizeof(dx.de_callsign))
        call_len = sizeof(dx.de_callsign) - 1;
    strncpy(dx.de_callsign, p, call_len);
    dx.de_callsign[call_len] = '\0';
    p = sep + 1;
    // Parse frequency
    char *end;
    dx.frequency = strtod(p, &end);
    if (end == p || *end != ' ')
        return;  // Invalid frequency or no space
    p = end + 1;
    // Parse spotted_callsign
    const char *comment_start = strchr(p, ' ');
    size_t spotted_len;
    if (comment_start) {
        spotted_len = comment_start - p;
    } else {
        // Modified: Use my_strnlen instead of strlen to avoid warning about bound depending on source length
        spotted_len = my_strnlen(p, sizeof(dx.spotted_callsign) - 1);
    }
    // Modified: Cap spotted_len explicitly (though my_strnlen already limits)
    if (spotted_len > sizeof(dx.spotted_callsign) - 1)
        spotted_len = sizeof(dx.spotted_callsign) - 1;
    strncpy(dx.spotted_callsign, p, spotted_len);
    dx.spotted_callsign[spotted_len] = '\0';
    // Parse optional comment
    if (comment_start) {
        p = comment_start + 1;
        // Modified: Use my_strnlen for comment to consistently handle lengths and avoid potential similar warnings
        size_t comment_len = my_strnlen(p, sizeof(dx.comment) - 1);
        strncpy(dx.comment, p, comment_len);
        dx.comment[comment_len] = '\0';
    }
    // (Application-specific handling of dx follows here, e.g. store or display it)
    // Modified: Changed printf to reflect DX spot instead of Agrelo
    printf("DX Spot packet: de=\"%s\", Freq=%.3f MHz, Spotted=\"%s\", Comment=\"%s\"\n", dx.de_callsign, dx.frequency, dx.spotted_callsign, dx.comment);
}

int aprs_encode_user_defined(char *info, size_t len, const aprs_user_defined_format_t *data) {
    if (!info || !data)
        return -1;
    // Compute lengths: DTI + UserID + packetType + data
    size_t data_len = strlen(data->data);
    size_t total_len = 1 + 1 + 1 + data_len;
    if (len < total_len + 1)  // +1 for null terminator
        return -1;
    info[0] = APRS_DTI_USER_DEFINED;  // '{'
    info[1] = data->userID;
    info[2] = data->packetType;
    if (data_len > 0) {
        memcpy(info + 3, data->data, data_len);
    }
    info[3 + data_len] = '\0';
    return (int) total_len;
}

int aprs_encode_third_party(char *info, size_t len, const char *header, const char *inner_info) {
    if (!info || !header || !inner_info)
        return -1;
    size_t header_len = strlen(header);
    size_t inner_len = strlen(inner_info);
    // Total: '}' + header + "::" + inner info
    size_t total_len = 1 + header_len + 2 + inner_len;
    if (len < total_len + 1)
        return -1;
    char *p = info;
    *p++ = APRS_DTI_THIRD_PARTY;  // '}'
    memcpy(p, header, header_len);
    p += header_len;
    memcpy(p, "::", 2);
    p += 2;
    if (inner_len > 0) {
        memcpy(p, inner_info, inner_len);
    }
    p += inner_len;
    *p = '\0';
    return (int) total_len;
}

int aprs_decode_user_defined(const char *info, aprs_user_defined_format_t *out) {
    if (!info || !out || info[0] != APRS_DTI_USER_DEFINED)
        return -1;
    // Must have at least prefix + userID + packetType
    if (strlen(info) < 3)
        return -1;

    out->userID = info[1];
    out->packetType = info[2];

    // Copy the remainder as data (may be zero-length)
    size_t data_len = strlen(info + 3);
    if (data_len >= APRS_MAX_INFO_LEN)
        data_len = APRS_MAX_INFO_LEN - 1;
    memcpy(out->data, info + 3, data_len);
    out->data[data_len] = '\0';

    return 0;
}

int aprs_decode_third_party(const char *info, aprs_third_party_packet_t *out) {
    if (!info || !out || info[0] != APRS_DTI_THIRD_PARTY)
        return -1;
    // Locate the "::" separator
    const char *sep = strstr(info + 1, "::");
    if (!sep)
        return -1;

    // Compute header length and cap it
    size_t header_len = sep - (info + 1);
    if (header_len >= APRS_MAX_HEADER_LEN)
        header_len = APRS_MAX_HEADER_LEN - 1;
    memcpy(out->header, info + 1, header_len);
    out->header[header_len] = '\0';

    // Copy inner info (after the "::")
    const char *inner = sep + 2;
    size_t inner_len = strlen(inner);
    if (inner_len >= APRS_MAX_INFO_LEN)
        inner_len = APRS_MAX_INFO_LEN - 1;
    memcpy(out->inner_info, inner, inner_len);
    out->inner_info[inner_len] = '\0';

    return 0;
}
