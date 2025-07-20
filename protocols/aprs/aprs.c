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
    if (buf_len < 256)
        return -1;
    char *p = buf;
    *p++ = 0x7E; // flag
    int addr_len = aprs_encode_addresses(p, frame);
    p += addr_len;
    *p++ = 0x03; // control: UI frame
    *p++ = 0xF0; // PID: no layer 3
    if (frame->info_len > 256)
        return -1;
    memcpy(p, frame->info, frame->info_len);
    p += frame->info_len;
    unsigned char *fcs_start = (unsigned char*) buf + 1;
    size_t fcs_len = addr_len + 1 + 1 + frame->info_len;
    uint16_t fcs = CRC(fcs_start, fcs_len);
    *p++ = fcs & 0xFF;
    *p++ = (fcs >> 8) & 0xFF;
    *p++ = 0x7E; // flag
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
    char *lat_str = lat_to_aprs(data->latitude);
    char *lon_str = lon_to_aprs(data->longitude);
    if (!lat_str || !lon_str)
        return -1;
    int ret = snprintf(info, len, "!%s%c%s%c%s", lat_str, data->symbol_table, lon_str, data->symbol_code, data->comment ? data->comment : "");
    if (ret < 0 || (size_t) ret >= len)
        return -1;
    return ret;
}

int aprs_encode_message(char *info, size_t len, const aprs_message_t *data) {
    // Check if addressee is too long
    bool too_long = true;
    for (int i = 0; i < 9; i++) {
        if (data->addressee[i] == '\0') {
            too_long = false;
            break;
        }
    }
    if (too_long && data->addressee[9] != '\0') {
        return -1; // addressee is longer than 9 characters
    }

    // Check message length
    if (data->message && strlen(data->message) > 67)
        return -1;

    // Check message_number length
    if (data->message_number && strlen(data->message_number) > 5)
        return -1;

    // Encode the message
    int ret = snprintf(info, len, ":%-9s:%s", data->addressee, data->message ? data->message : "");
    if (ret < 0 || (size_t) ret >= len)
        return -1;

    if (data->message_number) {
        int ret2 = snprintf(info + ret, len - ret, "{%s}", data->message_number);
        if (ret2 < 0 || (size_t) ret2 >= len - ret)
            return -1;
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
    if (info[0] != '!')
        return -1;
    char lat_str[9], lon_str[10], symbol_table, symbol_code;
    int ret = sscanf(info + 1, "%8s%c%9s%c", lat_str, &symbol_table, lon_str, &symbol_code);
    if (ret != 4)
        return -1;
    int deg, min_int, min_frac;
    char dir;
    ret = sscanf(lat_str, "%2d%2d.%2d%c", &deg, &min_int, &min_frac, &dir);
    if (ret != 4 || (dir != 'N' && dir != 'S'))
        return -1;
    double min = min_int + min_frac / 100.0;
    data->latitude = deg + min / 60.0;
    if (dir == 'S')
        data->latitude = -data->latitude;
    ret = sscanf(lon_str, "%3d%2d.%2d%c", &deg, &min_int, &min_frac, &dir);
    if (ret != 4 || (dir != 'E' && dir != 'W'))
        return -1;
    min = min_int + min_frac / 100.0;
    data->longitude = deg + min / 60.0;
    if (dir == 'W')
        data->longitude = -data->longitude;
    data->symbol_table = symbol_table;
    data->symbol_code = symbol_code;
    const char *comment_start = info + 1 + 8 + 1 + 9 + 1;
    data->comment = my_strdup(comment_start);
    if (!data->comment && *comment_start != '\0')
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
