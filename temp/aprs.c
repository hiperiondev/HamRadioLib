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
#include "aprs.h"

// Utility function to convert latitude to APRS format
char* lat_to_aprs(double lat, int ambiguity) {
    char *result = malloc(10);  // 8 chars + null + extra
    if (!result) return NULL;

    int degrees = (int)fabs(lat);
    double minutes = (fabs(lat) - degrees) * 60.0;
    char hemisphere = (lat >= 0) ? 'N' : 'S';

    if (ambiguity > 0) {
        snprintf(result, 10, "%02d  %c", degrees, hemisphere);
    } else {
        snprintf(result, 10, "%02d%05.2f%c", degrees, minutes, hemisphere);
    }
    return result;
}

// Utility function to convert longitude to APRS format
char* lon_to_aprs(double lon, int ambiguity) {
    char *result = malloc(11);  // 9 chars + null + extra
    if (!result) return NULL;

    int degrees = (int)fabs(lon);
    double minutes = (fabs(lon) - degrees) * 60.0;
    char hemisphere = (lon >= 0) ? 'E' : 'W';

    if (ambiguity > 0) {
        snprintf(result, 11, "%03d  %c", degrees, hemisphere);
    } else {
        snprintf(result, 11, "%03d%05.2f%c", degrees, minutes, hemisphere);
    }
    return result;
}

int aprs_encode_position_no_ts(char *info, size_t len, const aprs_position_no_ts_t *data) {
    char *lat_str = lat_to_aprs(data->latitude, 0);
    char *lon_str = lon_to_aprs(data->longitude, 0);
    if (!lat_str || !lon_str) {
        free(lat_str);
        free(lon_str);
        return -1;
    }

    int ret = snprintf(info, len, "%c%s%c%s%c%s",
                       data->dti, lat_str, data->symbol_table,
                       lon_str, data->symbol_code, data->comment ? data->comment : "");

    free(lat_str);
    free(lon_str);

    if (ret < 0 || (size_t)ret >= len) return -1;
    return ret;
}

int aprs_decode_position_no_ts(const char *info, aprs_position_no_ts_t *data) {
    if (!info || info[0] != '!' && info[0] != '=') return -1;
    data->dti = info[0];

    // Parse latitude (8 chars: DDMM.MM[N/S])
    double lat_deg = atoi(info + 1);
    double lat_min = atof(info + 3) / 100.0;
    data->latitude = lat_deg + lat_min;
    if (info[8] == 'S') data->latitude = -data->latitude;

    // Symbol table
    data->symbol_table = info[9];

    // Parse longitude (9 chars: DDDMM.MM[E/W])
    double lon_deg = atoi(info + 10);
    double lon_min = atof(info + 13) / 100.0;
    data->longitude = lon_deg + lon_min;
    if (info[18] == 'W') data->longitude = -data->longitude;

    // Symbol code
    data->symbol_code = info[19];

    // Comment
    size_t comment_len = strlen(info + 20);
    data->comment = malloc(comment_len + 1);
    if (!data->comment) return -1;
    strcpy(data->comment, info + 20);

    data->has_course_speed = false;
    return 0;
}

int aprs_encode_message(char *info, size_t len, const aprs_message_t *data) {
    int ret = snprintf(info, len, ":%-9s:%s%s",
                       data->addressee, data->message,
                       data->message_number ? data->message_number : "");
    if (ret < 0 || (size_t)ret >= len) return -1;
    return ret;
}

int aprs_decode_message(const char *info, aprs_message_t *data) {
    if (info[0] != ':') return -1;
    strncpy(data->addressee, info + 1, 9);
    data->addressee[9] = '\0';

    const char *msg_start = info + 10;
    const char *msg_end = strchr(msg_start, '{');
    if (!msg_end) msg_end = msg_start + strlen(msg_start);

    size_t msg_len = msg_end - msg_start;
    data->message = malloc(msg_len + 1);
    if (!data->message) return -1;
    strncpy(data->message, msg_start, msg_len);
    data->message[msg_len] = '\0';

    if (msg_end[0] == '{') {
        size_t num_len = strlen(msg_end + 1);
        data->message_number = malloc(num_len + 1);
        if (!data->message_number) {
            free(data->message);
            return -1;
        }
        strcpy(data->message_number, msg_end + 1);
    } else {
        data->message_number = NULL;
    }
    return 0;
}

int aprs_encode_weather_report(char *info, size_t len, const aprs_weather_report_t *data) {
    int ret = snprintf(info, len, "_%s_c%03d_s%03d_t%03.0f",
                       data->timestamp, data->wind_direction,
                       data->wind_speed, data->temperature);
    if (ret < 0 || (size_t)ret >= len) return -1;
    return ret;
}

int aprs_decode_weather_report(const char *info, aprs_weather_report_t *data) {
    if (info[0] != '_') return -1;
    strncpy(data->timestamp, info + 1, 8);
    data->timestamp[8] = '\0';

    data->wind_direction = atoi(info + 10);
    data->wind_speed = atoi(info + 14);
    data->temperature = atof(info + 18);
    return 0;
}

int aprs_encode_object_report(char *info, size_t len, const aprs_object_report_t *data) {
    char *lat_str = lat_to_aprs(data->latitude, 0);
    char *lon_str = lon_to_aprs(data->longitude, 0);
    if (!lat_str || !lon_str) {
        free(lat_str);
        free(lon_str);
        return -1;
    }

    int ret = snprintf(info, len, ";%-9s*%s%s%s",
                       data->name, data->timestamp, lat_str, lon_str);
    free(lat_str);
    free(lon_str);

    if (ret < 0 || (size_t)ret >= len) return -1;
    return ret;
}

int aprs_decode_object_report(const char *info, aprs_object_report_t *data) {
    if (info[0] != ';') return -1;
    strncpy(data->name, info + 1, 9);
    data->name[9] = '\0';

    strncpy(data->timestamp, info + 11, 7);
    data->timestamp[7] = '\0';

    double lat_deg = atoi(info + 18);
    double lat_min = atof(info + 20) / 100.0;
    data->latitude = lat_deg + lat_min;
    if (info[25] == 'S') data->latitude = -data->latitude;

    double lon_deg = atoi(info + 26);
    double lon_min = atof(info + 29) / 100.0;
    data->longitude = lon_deg + lon_min;
    if (info[34] == 'W') data->longitude = -data->longitude;

    return 0;
}

// Placeholder implementations for remaining functions
int aprs_encode_position_with_ts(char *info, size_t len, const aprs_position_with_ts_t *data) {
    char *lat_str = lat_to_aprs(data->latitude, 0);
    char *lon_str = lon_to_aprs(data->longitude, 0);
    if (!lat_str || !lon_str) {
        free(lat_str);
        free(lon_str);
        return -1;
    }

    int ret = snprintf(info, len, "%c%s%s%s%s",
                       data->dti, data->timestamp, lat_str, lon_str,
                       data->comment ? data->comment : "");
    free(lat_str);
    free(lon_str);

    if (ret < 0 || (size_t)ret >= len) return -1;
    return ret;
}

int aprs_decode_position_with_ts(const char *info, aprs_position_with_ts_t *data) {
    if (info[0] != '/' && info[0] != '@') return -1;
    data->dti = info[0];
    strncpy(data->timestamp, info + 1, 7);
    data->timestamp[7] = '\0';

    double lat_deg = atoi(info + 8);
    double lat_min = atof(info + 10) / 100.0;
    data->latitude = lat_deg + lat_min;
    if (info[15] == 'S') data->latitude = -data->latitude;

    data->symbol_table = info[16];

    double lon_deg = atoi(info + 17);
    double lon_min = atof(info + 20) / 100.0;
    data->longitude = lon_deg + lon_min;
    if (info[25] == 'W') data->longitude = -data->longitude;

    data->symbol_code = info[26];

    size_t comment_len = strlen(info + 27);
    data->comment = malloc(comment_len + 1);
    if (!data->comment) return -1;
    strcpy(data->comment, info + 27);

    return 0;
}

int aprs_encode_telemetry(char *info, size_t len, const aprs_telemetry_t *data) {
    int ret = snprintf(info, len, "T#%03u,%.0f,%.0f,%.0f,%.0f,%.0f,%d",
                       data->sequence_number, data->analog[0], data->analog[1],
                       data->analog[2], data->analog[3], data->analog[4], data->digital);
    if (ret < 0 || (size_t)ret >= len) return -1;
    return ret;
}

int aprs_decode_telemetry(const char *info, aprs_telemetry_t *data) {
    if (info[0] != 'T' || info[1] != '#') return -1;
    data->sequence_number = atoi(info + 2);
    sscanf(info + 6, "%lf,%lf,%lf,%lf,%lf,%hhu",
           &data->analog[0], &data->analog[1], &data->analog[2],
           &data->analog[3], &data->analog[4], &data->digital);
    return 0;
}

int aprs_encode_status(char *info, size_t len, const aprs_status_t *data) {
    int ret;
    if (data->has_timestamp) {
        ret = snprintf(info, len, ">%s%s", data->timestamp, data->status_text);
    } else {
        ret = snprintf(info, len, ">%s", data->status_text);
    }
    if (ret < 0 || (size_t)ret >= len) return -1;
    return ret;
}

int aprs_decode_status(const char *info, aprs_status_t *data) {
    if (info[0] != '>') return -1;
    if (info[7] == 'z') {
        data->has_timestamp = true;
        strncpy(data->timestamp, info + 1, 7);
        data->timestamp[7] = '\0';
        strncpy(data->status_text, info + 8, 62);
    } else {
        data->has_timestamp = false;
        data->timestamp[0] = '\0';
        strncpy(data->status_text, info + 1, 62);
    }
    data->status_text[62] = '\0';
    return 0;
}

int aprs_encode_general_query(char *info, size_t len, const aprs_general_query_t *data) {
    int ret = snprintf(info, len, "?%s", data->query_type);
    if (ret < 0 || (size_t)ret >= len) return -1;
    return ret;
}

int aprs_decode_general_query(const char *info, aprs_general_query_t *data) {
    if (info[0] != '?') return -1;
    strncpy(data->query_type, info + 1, 10);
    data->query_type[10] = '\0';
    return 0;
}

int aprs_encode_station_capabilities(char *info, size_t len, const aprs_station_capabilities_t *data) {
    int ret = snprintf(info, len, "<%s", data->capabilities_text);
    if (ret < 0 || (size_t)ret >= len) return -1;
    return ret;
}

int aprs_decode_station_capabilities(const char *info, aprs_station_capabilities_t *data) {
    if (info[0] != '<') return -1;
    strncpy(data->capabilities_text, info + 1, 99);
    data->capabilities_text[99] = '\0';
    return 0;
}

int aprs_encode_mice_frame(char *buf, size_t buf_len, const aprs_mice_t *data, const ax25_address_t *source, const ax25_address_t *digipeaters, int num_digipeaters) {
    // Simplified Mic-E encoding (actual implementation would encode into AX.25 frame)
    char info[50];
    char *lat_str = lat_to_aprs(data->latitude, 0);
    char *lon_str = lon_to_aprs(data->longitude, 0);
    if (!lat_str || !lon_str) {
        free(lat_str);
        free(lon_str);
        return -1;
    }

    int ret = snprintf(info, sizeof(info), "%s%s", lat_str, lon_str);
    free(lat_str);
    free(lon_str);

    if (ret < 0 || (size_t)ret >= buf_len) return -1;
    strncpy(buf, info, buf_len);
    return ret;
}

int aprs_decode_mice_frame(const char *buf, size_t len, aprs_mice_t *data, ax25_address_t *source, ax25_address_t *digipeaters, int *num_digipeaters) {
    // Simplified Mic-E decoding (actual implementation would decode AX.25 frame)
    double lat_deg = atoi(buf);
    double lat_min = atof(buf + 2) / 100.0;
    data->latitude = lat_deg + lat_min;
    if (buf[7] == 'S') data->latitude = -data->latitude;

    double lon_deg = atoi(buf + 8);
    double lon_min = atof(buf + 11) / 100.0;
    data->longitude = lon_deg + lon_min;
    if (buf[16] == 'W') data->longitude = -data->longitude;

    return 0;
}
