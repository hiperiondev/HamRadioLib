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

#ifndef APRS_H_
#define APRS_H_

#ifndef APRS_H
#define APRS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * Structure to represent an AX.25 address (callsign and SSID).
 */
typedef struct {
    char callsign[7]; // null-terminated
    uint8_t ssid; // 0-15
} aprs_address_t;

/**
 * Structure to represent an AX.25 frame for APRS.
 */
typedef struct {
    aprs_address_t source;
    aprs_address_t destination;
    aprs_address_t digipeaters[8];
    int num_digipeaters;
    char *info;
    size_t info_len;
} aprs_frame_t;

/**
 * Structure for APRS position report without timestamp.
 */
typedef struct {
    double latitude;
    double longitude;
    char symbol_table;
    char symbol_code;
    char *comment; // optional
    char dti; // '!' or '='
    bool has_course_speed;
    int course; // 0-360 degrees
    int speed; // knots, >=0
} aprs_position_no_ts_t;

/**
 * Structure for APRS message packet.
 */
typedef struct {
    char addressee[10]; // null-terminated, 9 chars max
    char *message; // up to 67 chars
    char *message_number; // optional, up to 5 chars
} aprs_message_t;

/**
 * Structure for APRS weather report.
 */
typedef struct {
    char timestamp[9]; // e.g., "12010000" (MMDDHHMM)
    float temperature;
    int wind_speed;
    int wind_direction;
} aprs_weather_report_t;

/**
 * Structure for APRS object report.
 */
typedef struct {
    char name[10]; // null-terminated, up to 9 chars
    char timestamp[8]; // e.g., "111111z" (DDHHMMz)
    double latitude;
    double longitude;
    char symbol_table;
    char symbol_code;
} aprs_object_report_t;

/**
 * Structure for APRS timestamped position report.
 */
typedef struct {
    char dti; // '/' or '@'
    char timestamp[8]; // e.g., "111111z" (DDHHMMz)
    double latitude;
    double longitude;
    char symbol_table;
    char symbol_code;
    char *comment; // optional
} aprs_position_with_ts_t;

// Mic-E structure
typedef struct {
    double latitude;      // Latitude in degrees (-90 to 90)
    double longitude;     // Longitude in degrees (-180 to 180)
    int speed;            // Speed in knots (0-799)
    int course;           // Course in degrees (0-360)
    char symbol_table;    // Symbol table identifier ('/' or '\')
    char symbol_code;     // Symbol code (printable ASCII)
    char message_code[10]; // Message code (e.g., "M0", "C1", "Emergency")
} aprs_mice_t;

// telemetry structure
typedef struct {
    char callsign[7];     // Callsign (6 chars + null terminator)
    uint8_t ssid;         // SSID (0-15)
    unsigned int sequence_number; // Sequence number (0-999)
    double analog[5];     // Five analog values (0-255)
    uint8_t digital;      // Eight digital bits (0 or 1)
} aprs_telemetry_t;

// Function prototypes
/**
 * Encode a single AX.25 address into a buffer.
 * @param buf Output buffer
 * @param addr Address to encode
 * @param is_last True if this is the last address in the sequence
 * @param is_digipeater True if this is a digipeater address
 * @return Number of bytes written (7)
 */
int aprs_encode_address(char *buf, const aprs_address_t *addr, bool is_last, bool is_digipeater);

/**
 * Encode all addresses in an AX.25 frame.
 * @param buf Output buffer
 * @param frame Frame containing addresses
 * @return Number of bytes written
 */
int aprs_encode_addresses(char *buf, const aprs_frame_t *frame);

/**
 * Encode a complete AX.25 frame.
 * @param buf Output buffer
 * @param buf_len Buffer length
 * @param frame Frame to encode
 * @return Number of bytes written or -1 on error
 */
int aprs_encode_frame(char *buf, size_t buf_len, const aprs_frame_t *frame);

/**
 * Convert latitude to APRS format (ddmm.mmN/S).
 * @param lat Latitude in degrees
 * @return Static string or NULL on error
 */
char* lat_to_aprs(double lat);

/**
 * Convert longitude to APRS format (dddmm.mmE/W).
 * @param lon Longitude in degrees
 * @return Static string or NULL on error
 */
char* lon_to_aprs(double lon);

/**
 * Encode an APRS position report without timestamp.
 * @param info Output buffer for information field
 * @param len Buffer length
 * @param data Position data
 * @return Number of bytes written or -1 on error
 */
int aprs_encode_position_no_ts(char *info, size_t len, const aprs_position_no_ts_t *data);

/**
 * Encode an APRS message packet.
 * @param info Output buffer for information field
 * @param len Buffer length
 * @param data Message data
 * @return Number of bytes written or -1 on error
 */
int aprs_encode_message(char *info, size_t len, const aprs_message_t *data);

/**
 * Decode an AX.25 address from a buffer.
 * @param buf Input buffer
 * @param addr Output address structure
 * @param is_last Output flag indicating if this is the last address
 * @return Number of bytes read (7) or -1 on error
 */
int aprs_decode_address(const char *buf, aprs_address_t *addr, bool *is_last);

/**
 * Decode an AX.25 frame.
 * @param buf Input buffer
 * @param len Buffer length
 * @param frame Output frame structure
 * @return Number of bytes read or -1 on error
 */
int aprs_decode_frame(const char *buf, size_t len, aprs_frame_t *frame);

/**
 * Decode an APRS position report without timestamp.
 * @param info Input information field
 * @param data Output position data
 * @return 0 on success, -1 on error
 */
int aprs_decode_position_no_ts(const char *info, aprs_position_no_ts_t *data);

/**
 * Decode an APRS message packet.
 * @param info Input information field
 * @param data Output message data
 * @return 0 on success, -1 on error
 */
int aprs_decode_message(const char *info, aprs_message_t *data);

double aprs_parse_lat(const char *str);
double aprs_parse_lon(const char *str);
int aprs_encode_weather_report(char *info, size_t len, const aprs_weather_report_t *data);
int aprs_decode_weather_report(const char *info, aprs_weather_report_t *data);
int aprs_encode_object_report(char *info, size_t len, const aprs_object_report_t *data);
int aprs_decode_object_report(const char *info, aprs_object_report_t *data);
int aprs_encode_position_with_ts(char *info, size_t len, const aprs_position_with_ts_t *data);
int aprs_decode_position_with_ts(const char *info, aprs_position_with_ts_t *data);
bool aprs_validate_timestamp(const char *timestamp, bool zulu);
int aprs_parse_weather_field(const char *data, char field_id, char *value, size_t value_len);
int aprs_encode_mice_destination(char *dest_str, const aprs_mice_t *data);
int aprs_encode_mice_info(char *info, size_t len, const aprs_mice_t *data);
int aprs_encode_mice_frame(char *buf, size_t buf_len, const aprs_mice_t *data, const aprs_address_t *source, const aprs_address_t *digipeaters,
        int num_digipeaters);
int aprs_encode_telemetry(char *info, size_t len, const aprs_telemetry_t *data);
int aprs_decode_telemetry(const char *info, aprs_telemetry_t *data);
int aprs_decode_mice_destination(const char *dest_str, aprs_mice_t *data, int *message_bits, bool *ns, bool *long_offset, bool *we);
int aprs_decode_mice_info(const char *info, size_t len, aprs_mice_t *data, bool long_offset, bool we);
int aprs_decode_mice_frame(const char *buf, size_t len, aprs_mice_t *data, aprs_address_t *source, aprs_address_t *digipeaters, int *num_digipeaters);

#endif

#endif /* APRS_H_ */
