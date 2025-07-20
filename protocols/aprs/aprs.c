/*
 * Copyright 2025 Emiliano Augusto Gonzalez (egonzalez . hiperion @ gmail . com))
 * * Project Site: https://github.com/hiperiondev/HamRadioLib *
 *
 * This is based on other projects:
 *    Asynchronous AX.25 library using asyncio: https://github.com/sjlongland/aioax25/
 *
 *    please contact their authors for more information.
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
        buf[i] = (c << 1) & 0xFE; // shift left and clear bit 0
    }
    uint8_t ssid_byte = (addr->ssid & 0x0F) << 1;
    if (!is_digipeater && !is_last) { // destination has C-bit=1
        ssid_byte |= 0x80;
    }
    buf[6] = ssid_byte;
    if (is_last) {
        buf[0] |= 0x01;
    }
    return 7;
}

int aprs_encode_addresses(char *buf, const aprs_frame_t *frame) {
    int offset = 0;
    offset += aprs_encode_address(buf + offset, &frame->destination, false, false);
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
        if (data->course < 0 || data->course > 360 || data->speed < 0) {
            return -1;
        }
        int ret2 = snprintf(info + ret, len - ret, "%03d/%03d", data->course, data->speed);
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
    *is_last = ((unsigned char) buf[0] & 0x01) != 0;
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
        return -1;
    }

    if (info[0] != '!' && info[0] != '=') {
        return -1;
    }
    data->dti = info[0];

    // Parse latitude: info[1] to info[8]
    char lat_str[9];
    strncpy(lat_str, info + 1, 8);
    lat_str[8] = '\0';
    data->latitude = aprs_parse_lat(lat_str);
    if (isnan(data->latitude)) {
        return -1;
    }

    // Symbol table: info[9]
    data->symbol_table = info[9];

    // Parse longitude: info[10] to info[18]
    char lon_str[10];
    strncpy(lon_str, info + 10, 9);
    lon_str[9] = '\0';
    data->longitude = aprs_parse_lon(lon_str);
    if (isnan(data->longitude)) {
        return -1;
    }

    // Symbol code: info[19]
    data->symbol_code = info[19];

    // Check for course/speed extension
    if (info_len >= 27) {
        // Check if info[20] to info[26] is ddd/ddd
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
            data->has_course_speed = true;
            // Comment starts at info + 27
            data->comment = my_strdup(info + 27);
        } else {
            data->has_course_speed = false;
            data->comment = my_strdup(info + 20);
        }
    } else {
        data->has_course_speed = false;
        data->comment = my_strdup(info + 20);
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
    // Validate ranges
    if (data->wind_speed < 0 || data->wind_direction < 0 || data->wind_direction > 360) {
        return -1;
    }

    // Format: _MMDDHHMMcDDD/SSS (simplified weather report)
    int ret = snprintf(info, len, "_12010000c%03d/%03d", data->wind_direction, data->wind_speed);
    if (ret < 0 || (size_t) ret >= len) {
        return -1;
    }

    return ret;
}

int aprs_decode_weather_report(const char *info, aprs_weather_report_t *data) {
    if (info[0] != '_' || strlen(info) < 17) {
        return -1;
    }

    // Check for 'c' at position 9
    if (info[9] != 'c') {
        return -1;
    }

    // Check for '/' at position 13
    if (info[13] != '/') {
        return -1;
    }

    // Extract wind direction: positions 10-12
    char dir_str[4] = { info[10], info[11], info[12], '\0' };

    // Extract wind speed: positions 14-16
    char speed_str[4] = { info[14], info[15], info[16], '\0' };

    // Check if all characters are digits
    for (int i = 0; i < 3; i++) {
        if (!isdigit(dir_str[i]) || !isdigit(speed_str[i])) {
            return -1;
        }
    }

    data->wind_direction = atoi(dir_str);
    data->wind_speed = atoi(speed_str);
    data->temperature = 0.0; // Not parsed in this simplified version

    if (data->wind_direction > 360 || data->wind_speed < 0) {
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

    int ret = snprintf(info, len, ";%-9s*111111z%s%c%s%c", data->name, lat_str, data->symbol_table, lon_str, data->symbol_code);
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

    // Check status and timestamp (simplified: assume '*111111z')
    if (info[10] != '*' || info[17] != 'z') {
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
