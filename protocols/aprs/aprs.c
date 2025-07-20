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

// Function to parse latitude from APRS format
double aprs_parse_lat(const char *str) {
    if (strlen(str) != 8)
        return NAN;
    char deg[3], min[3], frac[3], dir;
    if (sscanf(str, "%2s%2s.%2s%c", deg, min, frac, &dir) != 4)
        return NAN;
    double degrees = atof(deg);
    double minutes = atof(min) + atof(frac) / 100.0;
    double lat = degrees + minutes / 60.0;
    if (dir == 'S')
        lat = -lat;
    else if (dir != 'N')
        return NAN;
    return lat;
}

// Function to parse longitude from APRS format
double aprs_parse_lon(const char *str) {
    if (strlen(str) != 9)
        return NAN;
    char deg[4], min[3], frac[3], dir;
    if (sscanf(str, "%3s%2s.%2s%c", deg, min, frac, &dir) != 4)
        return NAN;
    double degrees = atof(deg);
    double minutes = atof(min) + atof(frac) / 100.0;
    double lon = degrees + minutes / 60.0;
    if (dir == 'W')
        lon = -lon;
    else if (dir != 'E')
        return NAN;
    return lon;
}

int aprs_encode_address(char *buf, const aprs_address_t *addr, bool is_last, bool is_digipeater) {
    for (int i = 0; i < 6; i++) {
        char c = addr->callsign[i];
        if (c == 0)
            c = ' ';
        buf[i] = (c << 1) & 0xFE;
    }
    uint8_t ssid_byte = (addr->ssid & 0x0F) << 1;
    buf[6] = ssid_byte;
    if (is_last) {
        buf[6] |= 0x01; // set end-of-address bit
    }
    return 7;
}

int aprs_encode_addresses(char *buf, const aprs_frame_t *frame) {
    int offset = 0;
    offset += aprs_encode_address(buf + offset, &frame->destination, false, false);
    // Set C-bit for destination
    buf[6] |= 0x80;
    bool source_is_last = (frame->num_digipeaters == 0);
    offset += aprs_encode_address(buf + offset, &frame->source, source_is_last, false);
    for (int i = 0; i < frame->num_digipeaters; i++) {
        bool is_last = (i == frame->num_digipeaters - 1);
        offset += aprs_encode_address(buf + offset, &frame->digipeaters[i], is_last, true);
    }
    return offset;
}

int aprs_encode_frame(char *buf, size_t buf_len, const aprs_frame_t *frame) {
    if (frame->num_digipeaters > 8 || frame->info_len > 256) {
        return -1;
    }
    size_t addr_len = 7 * (2 + frame->num_digipeaters);
    size_t total_len = 1 + addr_len + 1 + 1 + frame->info_len + 2 + 1; // start flag, addresses, control, PID, info, FCS, end flag
    if (buf_len < total_len) {
        return -1;
    }
    char *p = buf;
    *p++ = 0x7E; // start flag
    int written = aprs_encode_addresses(p, frame);
    p += written;
    *p++ = 0x03; // control: UI frame
    *p++ = 0xF0; // PID: no layer 3
    memcpy(p, frame->info, frame->info_len);
    p += frame->info_len;
    unsigned char *fcs_start = (unsigned char*) buf + 1;
    size_t fcs_len = addr_len + 1 + 1 + frame->info_len;
    uint16_t fcs = CRC(fcs_start, fcs_len);
    *p++ = fcs & 0xFF;
    *p++ = (fcs >> 8) & 0xFF;
    *p++ = 0x7E; // end flag
    return p - buf;
}

char* lat_to_aprs(double lat) {
    static char buf[9];
    if (lat < -90 || lat > 90)
        return NULL;
    char dir = (lat >= 0) ? 'N' : 'S';
    lat = fabs(lat);
    int deg = (int) lat;
    double min = (lat - deg) * 60;
    int min_int = (int) min;
    int min_frac = (int) ((min - min_int) * 100);
    sprintf(buf, "%02d%02d.%02d%c", deg, min_int, min_frac, dir);
    return buf;
}

char* lon_to_aprs(double lon) {
    static char buf[10];
    if (lon < -180 || lon > 180)
        return NULL;
    char dir = (lon >= 0) ? 'E' : 'W';
    lon = fabs(lon);
    int deg = (int) lon;
    double min = (lon - deg) * 60;
    int min_int = (int) min;
    int min_frac = (int) ((min - min_int) * 100);
    sprintf(buf, "%03d%02d.%02d%c", deg, min_int, min_frac, dir);
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

    char *lat_str = lat_to_aprs(data->latitude);
    char *lon_str = lon_to_aprs(data->longitude);
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

    if (data->comment) {
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

int aprs_decode_address(const char *buf, aprs_address_t *addr, bool *is_last) {
    for (int i = 0; i < 6; i++) {
        unsigned char byte = (unsigned char) buf[i];
        char c = byte >> 1;
        addr->callsign[i] = (c == ' ') ? '\0' : c;
    }
    addr->callsign[6] = '\0';
    unsigned char ssid_byte = (unsigned char) buf[6];
    addr->ssid = (ssid_byte >> 1) & 0x0F;
    *is_last = (ssid_byte & 0x01) != 0;
    return 7;
}

int aprs_decode_frame(const char *buf, size_t len, aprs_frame_t *frame) {
    if (len < 17 || buf[0] != 0x7E || buf[len - 1] != 0x7E)
        return -1;
    int offset = 1;
    bool is_last;
    offset += aprs_decode_address(buf + offset, &frame->destination, &is_last);
    offset += aprs_decode_address(buf + offset, &frame->source, &is_last);
    frame->num_digipeaters = 0;
    while (!is_last && frame->num_digipeaters < 8 && offset < len - 4) {
        offset += aprs_decode_address(buf + offset, &frame->digipeaters[frame->num_digipeaters], &is_last);
        frame->num_digipeaters++;
    }
    if ((unsigned char) buf[offset] != 0x03 || (unsigned char) buf[offset + 1] != 0xF0)
        return -1;
    offset += 2;
    size_t info_len = len - offset - 3;
    frame->info = malloc(info_len + 1);
    memcpy(frame->info, buf + offset, info_len);
    frame->info[info_len] = '\0';
    frame->info_len = info_len;
    uint16_t fcs = CRC((unsigned char*) buf + 1, len - 4);
    if (((unsigned char) buf[len - 3] != (fcs & 0xFF)) || ((unsigned char) buf[len - 2] != (fcs >> 8)))
        return -1;
    return len;
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
    data->latitude = aprs_parse_lat(lat_str);
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
    data->longitude = aprs_parse_lon(lon_str);
    if (isnan(data->longitude)) {
        return -1; // Invalid longitude
    }

    // Symbol code: info[19]
    if (!isprint(info[19])) {
        return -1; // Invalid symbol code
    }
    data->symbol_code = info[19];

    // Check for course/speed extension (ddd/ddd or ddd/-dd)
    data->has_course_speed = false;
    data->course = 0;
    data->speed = 0;
    if (info_len >= 27) {
        // Check format: three digits, a slash, then three characters (digits or '-dd')
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
        // Speed part can be three digits or '-dd'
        if (!(isdigit(info[24]) && isdigit(info[25]) && isdigit(info[26])) && !(info[24] == '-' && isdigit(info[25]) && isdigit(info[26]))) {
            is_extension = false;
        }
        if (is_extension) {
            char course_str[4] = { info[20], info[21], info[22], '\0' };
            char speed_str[4];
            if (info[24] == '-') {
                speed_str[0] = '-';
                speed_str[1] = info[25];
                speed_str[2] = info[26];
                speed_str[3] = '\0';
            } else {
                speed_str[0] = info[24];
                speed_str[1] = info[25];
                speed_str[2] = info[26];
                speed_str[3] = '\0';
            }
            data->course = atoi(course_str);
            data->speed = atoi(speed_str);
            if (data->course > 359 || data->course < 0) { // Course must be 0–359
                return -1;
            }
            if (data->speed < 0) { // Speed must be non-negative
                return -1;
            }
            data->has_course_speed = true;
            data->comment = my_strdup(info + 27); // Comment starts after course/speed
        } else {
            data->comment = my_strdup(info + 20); // Treat invalid extension as comment
        }
    } else {
        data->comment = my_strdup(info + 20); // Comment starts after symbol code
    }

    if (!data->comment) {
        return -1; // Memory allocation failure
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
    // Validate inputs
    if (strlen(data->timestamp) != 8) {
        return -1;
    }
    if (data->wind_direction < 0 || data->wind_direction > 360 || data->wind_speed < 0) {
        return -1;
    }
    // Explicitly cast to int to avoid floating-point issues
    int temp = (int) (data->temperature + 0.5); // Round 25.0 to 25
    if (temp < -99 || temp > 999) { // APRS temperature range constraint
        return -1;
    }

    // Format temperature: "tDDD" for positive, "t-DD" for negative
    char temp_str[5];
    if (temp >= 0) {
        snprintf(temp_str, sizeof(temp_str), "t%03d", temp); // e.g., "t025"
    } else {
        snprintf(temp_str, sizeof(temp_str), "t-%02d", -temp); // e.g., "t-05"
    }

    // Encode weather report without spaces
    int ret = snprintf(info, len, "_%sc%03ds%03d%s", data->timestamp, data->wind_direction, data->wind_speed, temp_str);

    // Check for encoding errors or buffer overflow
    if (ret < 0 || (size_t) ret >= len) {
        return -1;
    }

    return ret;
}

int aprs_decode_weather_report(const char *info, aprs_weather_report_t *data) {
    size_t info_len = strlen(info);
    if (info[0] != '_' || info_len < 9) {
        return -1;
    }

    // Extract timestamp (positions 1-8)
    strncpy(data->timestamp, info + 1, 8);
    data->timestamp[8] = '\0';

    // Parse weather data
    const char *p = info + 9;
    data->wind_direction = -1;
    data->wind_speed = -1;
    data->temperature = NAN;

    while (*p) {
        if (*p == 'c') {
            char dir_str[4];
            strncpy(dir_str, p + 1, 3);
            dir_str[3] = '\0';
            data->wind_direction = atoi(dir_str);
            p += 4;
        } else if (*p == 's') {
            char speed_str[4];
            strncpy(speed_str, p + 1, 3);
            speed_str[3] = '\0';
            data->wind_speed = atoi(speed_str);
            p += 4;
        } else if (*p == 't') {
            char temp_str[4];
            strncpy(temp_str, p + 1, 3);
            temp_str[3] = '\0';
            data->temperature = atof(temp_str);
            p += 4;
        } else {
            p++;
        }
    }

    // Check if required fields are set
    if (data->wind_direction == -1 || data->wind_speed == -1 || isnan(data->temperature)) {
        return -1;
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
    char *lat_str = lat_to_aprs(data->latitude);
    char *lon_str = lon_to_aprs(data->longitude);
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
    data->latitude = aprs_parse_lat(lat_str);
    if (isnan(data->latitude)) {
        return -1;
    }

    // Symbol table (position 26)
    data->symbol_table = info[26];

    // Parse longitude (positions 27-35)
    char lon_str[10];
    strncpy(lon_str, info + 27, 9);
    lon_str[9] = '\0';
    data->longitude = aprs_parse_lon(lon_str);
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

    char *lat_str = lat_to_aprs(data->latitude);
    char *lon_str = lon_to_aprs(data->longitude);
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
    data->latitude = aprs_parse_lat(lat_str);
    if (isnan(data->latitude)) {
        return -1;
    }

    // Symbol table (position 16)
    data->symbol_table = info[16];

    // Parse longitude (positions 17-25)
    char lon_str[10];
    strncpy(lon_str, info + 17, 9);
    lon_str[9] = '\0';
    data->longitude = aprs_parse_lon(lon_str);
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

// Function to encode the Mic-E destination address
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

// Function to encode the Mic-E information field
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

// Function to encode a complete Mic-E frame
int aprs_encode_mice_frame(char *buf, size_t buf_len, const aprs_mice_t *data, const aprs_address_t *source, const aprs_address_t *digipeaters,
        int num_digipeaters) {
    char dest_str[7];
    if (aprs_encode_mice_destination(dest_str, data) != 0) {
        return -1;
    }

    char info[256];
    int info_len = aprs_encode_mice_info(info, sizeof(info), data);
    if (info_len < 0) {
        return -1;
    }

    aprs_frame_t frame;
    // Set destination address
    strncpy(frame.destination.callsign, dest_str, 6);
    frame.destination.callsign[6] = '\0';
    frame.destination.ssid = 0; // Default SSID

    // Set source address
    frame.source = *source;

    // Set digipeaters
    if (num_digipeaters > 8) {
        return -1;
    }
    memcpy(frame.digipeaters, digipeaters, num_digipeaters * sizeof(aprs_address_t));
    frame.num_digipeaters = num_digipeaters;

    // Set info
    frame.info = info;
    frame.info_len = info_len;

    // Encode the frame
    return aprs_encode_frame(buf, buf_len, &frame);
}

int aprs_decode_mice_frame(const char *buf, size_t len, aprs_mice_t *data, aprs_address_t *source, aprs_address_t *digipeaters, int *num_digipeaters) {
    aprs_frame_t frame;
    if (aprs_decode_frame(buf, len, &frame) < 0) {
        return -1; // Failed to decode AX.25 frame
    }

    // Extract destination address
    char dest_str[7];
    strncpy(dest_str, frame.destination.callsign, 6);
    dest_str[6] = '\0';

    // Decode destination
    int message_bits;
    bool ns, long_offset, we;
    if (aprs_decode_mice_destination(dest_str, data, &message_bits, &ns, &long_offset, &we) < 0) {
        free(frame.info);
        return -1;
    }

    // Decode information field
    if (aprs_decode_mice_info(frame.info, frame.info_len, data, long_offset, we) < 0) {
        free(frame.info);
        return -1;
    }

    // Determine message code
    const char *standard_codes[8] = { "Emergency", "M6", "M5", "M4", "M3", "M2", "M1", "M0" };
    const char *custom_codes[8] = { "Emergency", "C6", "C5", "C4", "C3", "C2", "C1", "C0" };
    bool is_standard = (frame.info[0] == '`');
    strcpy(data->message_code, is_standard ? standard_codes[message_bits] : custom_codes[message_bits]);

    // Set source and digipeaters
    *source = frame.source;
    *num_digipeaters = frame.num_digipeaters;
    for (int i = 0; i < frame.num_digipeaters; i++) {
        digipeaters[i] = frame.digipeaters[i];
    }

    free(frame.info);
    return 0;
}

// Function to encode a telemetry packet
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

// Function to decode a telemetry packet
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
