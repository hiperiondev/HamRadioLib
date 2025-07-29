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

int aprs_encode_position_no_ts(char *out, size_t outlen, const aprs_position_no_ts_t *data) {
    if (!out || !data || outlen < 21)
        return -1;

    // Determinar DTI (por defecto '!' si data->dti==0)
    char dti_char = (data->dti != 0) ? data->dti : APRS_DTI_POSITION_NO_TS_NO_MSG;

    // Obtener cadenas APRS de lat y lon con ambigüedad
    char *lat_str = lat_to_aprs(data->latitude, data->ambiguity);
    char *lon_str = lon_to_aprs(data->longitude, data->ambiguity);
    if (!lat_str || !lon_str)
        return -1;

    // Formato básico: DTI + latitud + símbolo de tabla + longitud + símbolo de código
    int n = snprintf(out, outlen, "%c%s%c%s%c", dti_char, lat_str, data->symbol_table, lon_str, data->symbol_code);
    if (n < 0 || (size_t) n >= outlen)
        return -1;
    size_t idx = (size_t) n;

    // Curso/velocidad opcionales (curso mod 360, velocidad >=0)
    if (data->has_course_speed) {
        int course = data->course % 360;
        if (course < 0)
            course += 360;
        int speed = data->speed < 0 ? 0 : data->speed;
        int m = snprintf(out + idx, outlen - idx, "%03d/%03d", course, speed);
        if (m < 0 || idx + (size_t) m >= outlen)
            return -1;
        idx += (size_t) m;
    }

    // Comentario opcional
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

int aprs_decode_position_no_ts(const char *info, aprs_position_no_ts_t *data) {
    if (!info || !data)
        return -1;
    size_t len = strlen(info);
    // Base "no-TS" packet is exactly 20 chars: DTI(1)+LAT(8)+SYM_TBL(1)+LON(9)+SYM_CODE(1)
    if ((info[0] != '!' && info[0] != '=') || len < 20)
        return -1;

    // Zero-init everything
    *data = (aprs_position_no_ts_t ) { 0 };
    data->dti = info[0];

    // Latitude
    char lat_str[9] = { 0 };
    memcpy(lat_str, info + 1, 8);
    int lat_amb;
    data->latitude = aprs_parse_lat(lat_str, &lat_amb);
    if (isnan(data->latitude))
        return -1;
    data->ambiguity = lat_amb;

    // Symbol table
    data->symbol_table = info[9];

    // Longitude
    char lon_str[10] = { 0 };
    memcpy(lon_str, info + 10, 9);
    int lon_amb;
    data->longitude = aprs_parse_lon(lon_str, &lon_amb);
    if (isnan(data->longitude))
        return -1;

    // Symbol code
    data->symbol_code = info[19];
    if (!isprint((unsigned char )data->symbol_code))
        return -1;

    size_t idx = 20;

    // Optional course/speed "ccc/sss"
    if (len >= idx + 7&& isdigit((unsigned char)info[idx])
    && isdigit((unsigned char)info[idx+1])
    && isdigit((unsigned char)info[idx+2])
    && info[idx+3] == '/'
    && isdigit((unsigned char)info[idx+4])
    && isdigit((unsigned char)info[idx+5])
    && isdigit((unsigned char)info[idx+6])) {
        char cs_str[8] = { 0 };
        memcpy(cs_str, info + idx, 7);
        int course = atoi(cs_str);
        int speed = atoi(cs_str + 4);
        // invalid if course >=360 or speed <0
        if (course < 0 || course >= 360 || speed < 0)
            return -1;

        data->has_course_speed = true;
        data->course = course;
        data->speed = speed;
        idx += 7;
    }

    // Comment (always allocate at least an empty string)
    if (len > idx) {
        data->comment = my_strdup(info + idx);
    } else {
        data->comment = my_strdup("");
    }
    if (!data->comment)
        return -1;

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
    if (!info || !data) {
        fprintf(stderr, "Error: Invalid input to aprs_encode_weather_report\n");
        return -1;
    }
    int written = 0;
    // If a position is included, encode it first (use '!' DTI, no timestamp in position part)
    if (data->has_position) {
        aprs_position_no_ts_t pos = { 0 };
        pos.dti = APRS_DTI_POSITION_NO_TS_NO_MSG; // '!'
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
            fprintf(stderr, "Error: Buffer overflow writing weather timestamp\n");
            return -1;
        }
        written += n;
    } else {
        // Position provided and symbol_code is '_', skip extra '_'
        int n = snprintf(info + written, len - written, "%s", data->timestamp);
        if (n < 0 || (size_t) n >= len - written) {
            fprintf(stderr, "Error: Buffer overflow writing weather timestamp\n");
            return -1;
        }
        written += n;
    }
    // Wind direction (cDDD)
    if (data->wind_direction >= 0 && data->wind_direction <= 360) {
        int n = snprintf(info + written, len - written, "c%03d", data->wind_direction % 360);
        if (n < 0 || (size_t) n >= len - written) {
            fprintf(stderr, "Error: Buffer overflow after wind direction\n");
            return -1;
        }
        written += n;
    } else {
        fprintf(stderr, "Error: Invalid wind direction (%d)\n", data->wind_direction);
        return -1;
    }
    // Wind speed (sDDD)
    if (data->wind_speed >= 0) {
        int n = snprintf(info + written, len - written, "s%03d", data->wind_speed);
        if (n < 0 || (size_t) n >= len - written) {
            fprintf(stderr, "Error: Buffer overflow after wind speed\n");
            return -1;
        }
        written += n;
    } else {
        fprintf(stderr, "Error: Invalid wind speed (%d)\n", data->wind_speed);
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
            fprintf(stderr, "Error: Buffer overflow after temperature\n");
            return -1;
        }
        written += n;
    } else {
        fprintf(stderr, "Error: Invalid temperature (%f)\n", data->temperature);
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

    // 1) Optional position (DTI '!' or '=')
    aprs_position_no_ts_t pos = { 0 };
    char *pos_comment = NULL;
    if (*wx == APRS_DTI_POSITION_NO_TS_NO_MSG || *wx == APRS_DTI_POSITION_NO_TS_WITH_MSG) {
        if (aprs_decode_position_no_ts(wx, &pos) != 0) {
            return -1;
        }
        data->has_position = true;
        data->latitude = pos.latitude;
        data->longitude = pos.longitude;
        data->symbol_table = pos.symbol_table;
        data->symbol_code = pos.symbol_code;
        pos_comment = pos.comment;
        wx = pos_comment ? pos_comment : "";
    } else {
        data->has_position = false;
    }

    // 2) Skip leading '_' weather symbol if present
    if (*wx == APRS_DTI_WEATHER_REPORT) {
        wx++;
    }

    // 3) Extract timestamp as “up to first field code”
    const char *p = wx;
    const char *field_codes = "cstgpPb hLlSRFfiI#";
    while (*p && !strchr(field_codes, *p)) {
        p++;
    }
    size_t ts_len = p - wx;
    if (ts_len == 0 || ts_len >= sizeof(data->timestamp)) {
        if (pos_comment)
            free(pos_comment);
        return -1;
    }
    memcpy(data->timestamp, wx, ts_len);
    data->timestamp[ts_len] = '\0';
    wx = p;

    // 4) Initialize all weather fields to “not present”
    data->temperature = -999.9f;
    data->wind_speed = -1;
    data->wind_direction = -1;
    data->wind_gust = -1;
    data->rainfall_last_hour = -1;
    data->rainfall_24h = -1;
    data->rainfall_since_midnight = -1;
    data->barometric_pressure = -1;
    data->humidity = -1;
    data->luminosity = -1;
    data->snowfall_24h = -999.9f;
    data->rain_rate = -1;
    data->water_height_feet = -999.9f;
    data->water_height_meters = -999.9f;
    data->indoors_temperature = -999.9f;
    data->indoors_humidity = -1;
    data->raw_rain_counter = -1;

    // 5) Parse remaining weather fields in any order
    while (*wx) {
        switch (*wx) {
            case 'c': {
                char buf[4] = { 0 };
                strncpy(buf, wx + 1, 3);
                data->wind_direction = atoi(buf);
                wx += 4;
            }
            break;
            case 's': {
                char buf[4] = { 0 };
                strncpy(buf, wx + 1, 3);
                data->wind_speed = atoi(buf);
                wx += 4;
            }
            break;
            case 't': {
                wx++;
                bool neg = (*wx == '-');
                if (neg)
                    wx++;
                int n = neg ? 2 : 3;
                char buf[4] = { 0 };
                strncpy(buf, wx, n);
                data->temperature = neg ? -atoi(buf) : (float) atoi(buf);
                wx += n;
            }
            break;
            case 'g': {
                char buf[4] = { 0 };
                strncpy(buf, wx + 1, 3);
                data->wind_gust = atoi(buf);
                wx += 4;
            }
            break;
            case 'r': {
                char buf[4] = { 0 };
                strncpy(buf, wx + 1, 3);
                data->rainfall_last_hour = atoi(buf);
                wx += 4;
            }
            break;
            case 'p': {
                char buf[4] = { 0 };
                strncpy(buf, wx + 1, 3);
                data->rainfall_24h = atoi(buf);
                wx += 4;
            }
            break;
            case 'P': {
                char buf[4] = { 0 };
                strncpy(buf, wx + 1, 3);
                data->rainfall_since_midnight = atoi(buf);
                wx += 4;
            }
            break;
            case 'b': {
                char buf[6] = { 0 };
                strncpy(buf, wx + 1, 5);
                data->barometric_pressure = atoi(buf);
                wx += 6;
            }
            break;
            case 'h': {
                char buf[3] = { 0 };
                strncpy(buf, wx + 1, 2);
                data->humidity = atoi(buf);
                wx += 3;
            }
            break;
            case 'L':
            case 'l': {
                char type = *wx;
                char buf[4] = { 0 };
                strncpy(buf, wx + 1, 3);
                data->luminosity = (type == 'L') ? atoi(buf) : (atoi(buf) + 1000);
                wx += 4;
            }
            break;
            case 'S': {
                char buf[4] = { 0 };
                strncpy(buf, wx + 1, 3);
                data->snowfall_24h = atoi(buf) / 10.0f;
                wx += 4;
            }
            break;
            case 'R': {
                char buf[4] = { 0 };
                strncpy(buf, wx + 1, 3);
                data->rain_rate = atoi(buf);
                wx += 4;
            }
            break;
            case 'F': {
                wx++;
                data->water_height_feet = strtof(wx, (char**) &wx);
            }
            break;
            case 'f': {
                wx++;
                data->water_height_meters = strtof(wx, (char**) &wx);
            }
            break;
            case 'i': {
                wx++;
                bool neg = (*wx == '-');
                if (neg)
                    wx++;
                data->indoors_temperature = strtof(wx, (char**) &wx) * (neg ? -1.0f : 1.0f);
            }
            break;
            case 'I': {
                char buf[3] = { 0 };
                strncpy(buf, wx + 1, 2);
                data->indoors_humidity = atoi(buf);
                wx += 3;
            }
            break;
            case '#': {
                char buf[6] = { 0 };
                strncpy(buf, wx + 1, 5);
                data->raw_rain_counter = atoi(buf);
                wx += 6;
            }
            break;
            default:
                // Skip unknown characters
                wx++;
        }
    }

    if (pos_comment)
        free(pos_comment);
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
        p += 7; // skip "PHGpphd"
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
    size_t len = strlen(info);

    /* Must start with '@' or '/' and be long enough for TS+LAT+TABLE+LON+CODE */
    if ((info[0] != '@' && info[0] != '/') || len < 27) {
        return -1;
    }

    *data = (aprs_position_with_ts_t ) { 0 };
    data->dti = info[0];

    /* Timestamp DDHHMM[z|l] at positions 1–7 */
    memcpy(data->timestamp, info + 1, 7);
    data->timestamp[7] = '\0';
    if (data->timestamp[6] != 'z' && data->timestamp[6] != 'l') {
        return -1;
    }

    /* Latitude (positions 8–15) */
    char latstr[9] = { 0 };
    memcpy(latstr, info + 8, 8);
    int lat_amb;
    data->latitude = aprs_parse_lat(latstr, &lat_amb);
    if (isnan(data->latitude)) {
        return -1;
    }

    /* Symbol table at pos 16 */
    data->symbol_table = info[16];
    if (data->symbol_table != '/' && data->symbol_table != '\\') {
        return -1;
    }

    /* Longitude (positions 17–25) */
    char lonstr[10] = { 0 };
    memcpy(lonstr, info + 17, 9);
    int lon_amb;
    data->longitude = aprs_parse_lon(lonstr, &lon_amb);
    if (isnan(data->longitude)) {
        return -1;
    }

    /* Symbol code at pos 26 */
    data->symbol_code = info[26];
    if (!isprint((unsigned char )data->symbol_code)) {
        return -1;
    }

    /* Optional course/speed (positions 27–34: "/CCC/SSS") */
    data->has_course_speed = false;
    if (len >= 35&&
    info[27] == '/' &&
    isdigit((unsigned char)info[28]) &&
    isdigit((unsigned char)info[29]) &&
    isdigit((unsigned char)info[30]) &&
    info[31] == '/' &&
    isdigit((unsigned char)info[32]) &&
    isdigit((unsigned char)info[33]) &&
    isdigit((unsigned char)info[34])) {
        /* Parse numeric fields */
        int course = (info[28] - '0') * 100 + (info[29] - '0') * 10 + (info[30] - '0');
        int speed = (info[32] - '0') * 100 + (info[33] - '0') * 10 + (info[34] - '0');

        /* Validate ranges: course 0–359, speed 0–999 */
        if (course < 0 || course > 359 || speed < 0 || speed > 999) {
            return -1;
        }

        data->course = course;
        data->speed = speed;
        data->has_course_speed = true;
    }

    /* Comment (from pos 35 if course/speed, else pos 27) */
    const char *cstart = info + (data->has_course_speed ? 35 : 27);
    data->comment = *cstart ? my_strdup(cstart) : NULL;

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
    if (!info || !data)
        return -1;

    // Validate name length
    if (strlen(data->name) > 9) {
        return -1; // Item name too long
    }

    // Validate symbol table
    if (data->symbol_table != '/' && data->symbol_table != '\\') {
        return -1; // Invalid symbol table
    }

    // Validate symbol code
    if (!isprint((unsigned char )data->symbol_code)) {
        return -1; // Invalid symbol code
    }

    // Pad name to 9 characters (right-padded with spaces)
    char name_padded[10];
    snprintf(name_padded, sizeof(name_padded), "%-9s", data->name);

    // Convert latitude and longitude to APRS format strings
    char *lat_str = lat_to_aprs(data->latitude, 0);
    char *lon_str = lon_to_aprs(data->longitude, 0);
    if (!lat_str || !lon_str) {
        return -1; // Conversion failed
    }

    // Determine live ('!') or killed ('=') status flag
    char status_char = data->is_live ? '!' : '=';

    // Start encoding string with ")" identifier for item report
    int ret = snprintf(info, len, ")%s%c%s%c%s%c", name_padded, status_char, lat_str, data->symbol_table, lon_str, data->symbol_code);

    if (ret < 0 || (size_t) ret >= len) {
        return -1; // Encoding error or buffer too small
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
            return -1; // Comment encoding error
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
    const double R = 6371.0; // Earth radius in km
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
        return 0; // Not addressed to us
    }
    // Message text must start and end with '?'
    const char *text = msg->message;
    size_t tlen = strlen(text);
    if (tlen < 3 || text[0] != '?' || text[tlen - 1] != '?') {
        return 0; // Not a valid query
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

void encodePositionPacket(const PositionReport *pos, char *out) {
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
void parseAltitudePHG(const char *comment, PositionReport *pos) {
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
    UserDefinedFormat udf;
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

// Parse an Agrelo DFJr telemetry APRS field beginning with '%'
void parse_agrelo(const char *info) {
    if (info == NULL || info[0] != '%')
        return;  // Not an Agrelo packet
    AgreloData adf = { { 0 }, { 0 }, 0 };
    const char *p = info + 1;

    // Extract a (up to 3-char) ID before the first comma
    int i = 0;
    while (*p != '\0' && *p != ',' && i < 3) {
        adf.id[i++] = *p++;
    }
    adf.id[i] = '\0';
    if (*p == ',')
        p++;

    // Parse five analog values separated by commas
    for (int j = 0; j < 5; ++j) {
        if (*p == '\0')
            break;
        adf.analog[j] = atoi(p);
        p = strchr(p, ',');
        if (!p)
            break;
        ++p;
    }

    // Parse the remaining 8-bit digital status in binary form
    if (*p != '\0') {
        // Assume the rest is a binary string like "11001010"
        adf.digital = strtol(p, NULL, 2);
    }

    // (Application-specific handling of adf follows here)
    printf("Agrelo DFJr packet: ID=\"%s\", Analogs=%d,%d,%d,%d,%d, Digital=0x%02X\n", adf.id, adf.analog[0], adf.analog[1], adf.analog[2], adf.analog[3],
            adf.analog[4], adf.digital);
}

int aprs_encode_user_defined(char *info, size_t len, const UserDefinedFormat *data) {
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

int aprs_decode_user_defined(const char *info, UserDefinedFormat *out) {
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

int aprs_decode_third_party(const char *info, ThirdPartyPacket *out) {
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
