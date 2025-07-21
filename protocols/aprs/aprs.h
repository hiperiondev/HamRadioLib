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

#ifndef APRS_H_
#define APRS_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Structure to represent an AX.25 frame for APRS packets.
 *
 * This structure contains the source, destination, digipeater path, and information field of an APRS packet.
 */
typedef struct {
    char *info;                   ///< Information field of the APRS packet.
    size_t info_len;              ///< Length of the information field.
} aprs_frame_t;

/**
 * @brief Structure for APRS position report without timestamp.
 *
 * This structure holds position data, including latitude, longitude, symbol, comment, and optional course and speed.
 */
typedef struct {
    double latitude;      ///< Latitude in decimal degrees (-90 to 90).
    double longitude;     ///< Longitude in decimal degrees (-180 to 180).
    char symbol_table;    ///< Symbol table identifier ('/' or '\').
    char symbol_code;     ///< Symbol code (printable ASCII).
    char *comment;        ///< Optional comment field, null-terminated.
    char dti;             ///< Data Type Indicator ('!' or '=' for non-messaging/messaging capable).
    bool has_course_speed; ///< Flag indicating if course and speed are included.
    int course;           ///< Course in degrees (0-360), if has_course_speed is true.
    int speed;            ///< Speed in knots (>=0), if has_course_speed is true.
} aprs_position_no_ts_t;

/**
 * @brief Structure for APRS message packet.
 *
 * This structure contains the addressee, message text, and optional message number for acknowledgment.
 */
typedef struct {
    char addressee[10];   ///< Addressee callsign, null-terminated, up to 9 characters.
    char *message;        ///< Message text, up to 67 characters, null-terminated.
    char *message_number; ///< Optional message number for acknowledgment, up to 5 characters, null-terminated.
} aprs_message_t;

/**
 * @brief Structure for APRS weather report.
 *
 * This structure holds weather data, including timestamp, temperature, wind speed, and wind direction.
 * Units follow APRS protocol conventions: temperature in Fahrenheit, wind speed in mph.
 */
typedef struct {
    char timestamp[9];    ///< Timestamp in MMDDHHMM format (e.g., "12010000").
    float temperature;    ///< Temperature in degrees Fahrenheit.
    int wind_speed;       ///< Wind speed in miles per hour.
    int wind_direction;   ///< Wind direction in degrees (0-360).
} aprs_weather_report_t;

/**
 * @brief Structure for APRS object report.
 *
 * This structure holds data for an APRS object, including name, timestamp, position, and symbol.
 */
typedef struct {
    char name[10];        ///< Object name, null-terminated, up to 9 characters.
    char timestamp[8];    ///< Timestamp in DDHHMMz format (e.g., "111111z").
    double latitude;      ///< Latitude in decimal degrees (-90 to 90).
    double longitude;     ///< Longitude in decimal degrees (-180 to 180).
    char symbol_table;    ///< Symbol table identifier ('/' or '\').
    char symbol_code;     ///< Symbol code (printable ASCII).
} aprs_object_report_t;

/**
 * @brief Structure for APRS timestamped position report.
 *
 * This structure holds position data with a timestamp, symbol, and optional comment.
 */
typedef struct {
    char dti;             ///< Data Type Indicator ('/' or '@' for non-messaging/messaging capable).
    char timestamp[8];    ///< Timestamp in DDHHMMz format (e.g., "111111z").
    double latitude;      ///< Latitude in decimal degrees (-90 to 90).
    double longitude;     ///< Longitude in decimal degrees (-180 to 180).
    char symbol_table;    ///< Symbol table identifier ('/' or '\').
    char symbol_code;     ///< Symbol code (printable ASCII).
    char *comment;        ///< Optional comment field, null-terminated.
} aprs_position_with_ts_t;

/**
 * @brief Structure for Mic-E encoded position report.
 *
 * This structure holds data for a Mic-E compressed position report, including position, speed, course, symbol, and message code.
 */
typedef struct {
    double latitude;      ///< Latitude in degrees (-90 to 90).
    double longitude;     ///< Longitude in degrees (-180 to 180).
    int speed;            ///< Speed in knots (0-799).
    int course;           ///< Course in degrees (0-360).
    char symbol_table;    ///< Symbol table identifier ('/' or '\').
    char symbol_code;     ///< Symbol code (printable ASCII).
    char message_code[10]; ///< Message code (e.g., "M0", "C1", "Emergency"), null-terminated.
} aprs_mice_t;

/**
 * @brief Structure for APRS telemetry report.
 *
 * This structure holds telemetry data, including sequence number, analog values, and digital bits.
 */
typedef struct {
    unsigned int sequence_number; ///< Sequence number (0-999).
    double analog[5];     ///< Five analog values (typically 0-255).
    uint8_t digital;      ///< Eight digital bits (0 or 1).
} aprs_telemetry_t;

/**
 * @brief Structure for APRS status report.
 *
 * This structure holds a status message, optionally with a timestamp.
 */
typedef struct {
    bool has_timestamp;   ///< Flag indicating if timestamp is present.
    char timestamp[8];    ///< Timestamp in DDHHMMz format (e.g., "111111z"), if present.
    char status_text[63]; ///< Status message text, up to 62 characters + null terminator.
} aprs_status_t;

/**
 * @brief Structure for APRS general query.
 *
 * This structure holds the query type for general queries in APRS.
 */
typedef struct {
    char query_type[11];  ///< Query type (e.g., "APRS"), up to 10 characters + null terminator.
} aprs_general_query_t;

/**
 * @brief Structure for APRS station capabilities.
 *
 * This structure holds the capabilities text for a station.
 */
typedef struct {
    char capabilities_text[100]; ///< Capabilities text, up to 99 characters + null terminator.
} aprs_station_capabilities_t;

/**
 * @brief Structure for APRS bulletin.
 *
 * This structure holds the bulletin ID and message text.
 */
typedef struct {
    char bulletin_id[5];  ///< Bulletin ID, e.g., "BLN1", null-terminated.
    char *message;        ///< Message text, up to 67 characters, null-terminated.
    char *message_number; ///< Optional message number for acknowledgment, up to 5 characters, null-terminated.
} aprs_bulletin_t;

/**
 * @brief Structure for APRS item report.
 *
 * This structure holds the item name, live/killed status, position, symbol, and optional comment.
 */
typedef struct {
    char name[10];        ///< Item name, null-terminated, up to 9 characters.
    bool is_live;         ///< True if the item is live, false if killed.
    double latitude;      ///< Latitude in decimal degrees (-90 to 90).
    double longitude;     ///< Longitude in decimal degrees (-180 to 180).
    char symbol_table;    ///< Symbol table identifier ('/' or '\').
    char symbol_code;     ///< Symbol code (printable ASCII).
    char *comment;        ///< Optional comment field, null-terminated.
} aprs_item_report_t;

/**
 * @brief Encode an APRS bulletin.
 *
 * This function encodes a bulletin into the information field, using the bulletin ID as the addressee.
 *
 * @param info Output buffer for the information field.
 * @param len Size of the output buffer.
 * @param data Pointer to the bulletin data structure.
 * @return Number of bytes written to the buffer, or -1 on error.
 */
int aprs_encode_bulletin(char *info, size_t len, const aprs_bulletin_t *data);

/**
 * @brief Encode an APRS item report.
 *
 * This function encodes an item report into the information field.
 *
 * @param info Output buffer for the information field.
 * @param len Size of the output buffer.
 * @param data Pointer to the item report data structure.
 * @return Number of bytes written to the buffer, or -1 on error.
 */
int aprs_encode_item_report(char *info, size_t len, const aprs_item_report_t *data);

/**
 * @brief Check if a decoded message is a bulletin.
 *
 * This function checks if the addressee starts with "BLN" followed by a digit.
 *
 * @param msg Pointer to the decoded message structure.
 * @return True if the message is a bulletin, false otherwise.
 */
bool aprs_is_bulletin(const aprs_message_t *msg);

/**
 * @brief Decode an APRS item report.
 *
 * This function decodes an item report from the information field into a structure.
 *
 * @param info Input buffer containing the information field.
 * @param data Pointer to the item report data structure to fill.
 * @return 0 on success, -1 on error.
 */
int aprs_decode_item_report(const char *info, aprs_item_report_t *data);

/**
 * @brief Convert latitude to APRS format.
 *
 * This function converts a latitude value in decimal degrees to the APRS format string (ddmm.mmN/S), with an optional ambiguity level.
 *
 * @param lat Latitude in decimal degrees (-90 to 90).
 * @param ambiguity Level of ambiguity (0-4), where 0 is no ambiguity.
 * @return Pointer to a static string containing the formatted latitude, or NULL on error.
 * @note The returned string is stored in a static buffer and will be overwritten on subsequent calls.
 */
char* lat_to_aprs(double lat, int ambiguity);

/**
 * @brief Convert longitude to APRS format.
 *
 * This function converts a longitude value in decimal degrees to the APRS format string (dddmm.mmE/W), with an optional ambiguity level.
 *
 * @param lon Longitude in decimal degrees (-180 to 180).
 * @param ambiguity Level of ambiguity (0-4), where 0 is no ambiguity.
 * @return Pointer to a static string containing the formatted longitude, or NULL on error.
 * @note The returned string is stored in a static buffer and will be overwritten on subsequent calls.
 */
char* lon_to_aprs(double lon, int ambiguity);

/**
 * @brief Encode an APRS position report without timestamp.
 *
 * This function encodes a position report (with or without messaging capability) into the information field, including optional course and speed.
 *
 * @param info Output buffer for the information field.
 * @param len Size of the output buffer.
 * @param data Pointer to the position data structure.
 * @return Number of bytes written to the buffer, or -1 on error.
 */
int aprs_encode_position_no_ts(char *info, size_t len, const aprs_position_no_ts_t *data);

/**
 * @brief Encode an APRS message packet.
 *
 * This function encodes a message packet into the information field, including addressee, message text, and optional message number.
 *
 * @param info Output buffer for the information field.
 * @param len Size of the output buffer.
 * @param data Pointer to the message data structure.
 * @return Number of bytes written to the buffer, or -1 on error.
 */
int aprs_encode_message(char *info, size_t len, const aprs_message_t *data);

/**
 * @brief Decode an APRS position report without timestamp.
 *
 * This function decodes a position report from the information field into a structure.
 *
 * @param info Input buffer containing the information field.
 * @param data Pointer to the position data structure to fill.
 * @return 0 on success, -1 on error.
 */
int aprs_decode_position_no_ts(const char *info, aprs_position_no_ts_t *data);

/**
 * @brief Decode an APRS message packet.
 *
 * This function decodes a message packet from the information field into a structure.
 *
 * @param info Input buffer containing the information field.
 * @param data Pointer to the message data structure to fill.
 * @return 0 on success, -1 on error.
 */
int aprs_decode_message(const char *info, aprs_message_t *data);

/**
 * @brief Parse latitude from APRS format string.
 *
 * This function parses a latitude string in APRS format (ddmm.mmN/S) and returns it in decimal degrees.
 *
 * @param str Input string in APRS latitude format.
 * @param ambiguity Pointer to store the ambiguity level (0-4), if needed.
 * @return Latitude in decimal degrees, or NAN on error.
 */
double aprs_parse_lat(const char *str, int *ambiguity);

/**
 * @brief Parse longitude from APRS format string.
 *
 * This function parses a longitude string in APRS format (dddmm.mmE/W) and returns it in decimal degrees.
 *
 * @param str Input string in APRS longitude format.
 * @param ambiguity Pointer to store the ambiguity level (0-4), if needed.
 * @return Longitude in decimal degrees, or NAN on error.
 */
double aprs_parse_lon(const char *str, int *ambiguity);

/**
 * @brief Encode an APRS weather report.
 *
 * This function encodes a weather report packet into the information field, including timestamp and weather data.
 *
 * @param info Output buffer for the information field.
 * @param len Size of the output buffer.
 * @param data Pointer to the weather report structure.
 * @return Number of bytes written to the buffer, or -1 on error.
 */
int aprs_encode_weather_report(char *info, size_t len, const aprs_weather_report_t *data);

/**
 * @brief Decode an APRS weather report.
 *
 * This function decodes a weather report packet from the information field into a structure.
 *
 * @param info Input buffer containing the information field.
 * @param data Pointer to the weather report structure to fill.
 * @return 0 on success, -1 on error.
 */
int aprs_decode_weather_report(const char *info, aprs_weather_report_t *data);

/**
 * @brief Encode an APRS object report.
 *
 * This function encodes an object report packet into the information field, including name, timestamp, and position.
 *
 * @param info Output buffer for the information field.
 * @param len Size of the output buffer.
 * @param data Pointer to the object report structure.
 * @return Number of bytes written to the buffer, or -1 on error.
 */
int aprs_encode_object_report(char *info, size_t len, const aprs_object_report_t *data);

/**
 * @brief Decode an APRS object report.
 *
 * This function decodes an object report packet from the information field into a structure.
 *
 * @param info Input buffer containing the information field.
 * @param data Pointer to the object report structure to fill.
 * @return 0 on success, -1 on error.
 */
int aprs_decode_object_report(const char *info, aprs_object_report_t *data);

/**
 * @brief Encode an APRS position report with timestamp.
 *
 * This function encodes a timestamped position report into the information field.
 *
 * @param info Output buffer for the information field.
 * @param len Size of the output buffer.
 * @param data Pointer to the position data structure.
 * @return Number of bytes written to the buffer, or -1 on error.
 */
int aprs_encode_position_with_ts(char *info, size_t len, const aprs_position_with_ts_t *data);

/**
 * @brief Decode an APRS position report with timestamp.
 *
 * This function decodes a timestamped position report from the information field into a structure.
 *
 * @param info Input buffer containing the information field.
 * @param data Pointer to the position data structure to fill.
 * @return 0 on success, -1 on error.
 */
int aprs_decode_position_with_ts(const char *info, aprs_position_with_ts_t *data);

/**
 * @brief Validate an APRS timestamp.
 *
 * This function checks if a timestamp is valid in DDHHMMz or other supported formats.
 *
 * @param timestamp Input timestamp string (e.g., "111111z").
 * @param zulu Flag indicating if the timestamp is in zulu (UTC) format.
 * @return true if the timestamp is valid, false otherwise.
 */
bool aprs_validate_timestamp(const char *timestamp, bool zulu);

/**
 * @brief Parse a specific weather field from APRS data.
 *
 * This function extracts a specific weather field (e.g., temperature, wind speed) from the input data.
 *
 * @param data Input buffer containing the weather data.
 * @param field_id Identifier of the field to parse (e.g., 't' for temperature).
 * @param value Output buffer to store the parsed value.
 * @param value_len Size of the output buffer.
 * @return Number of bytes written to the value buffer, or -1 on error.
 */
int aprs_parse_weather_field(const char *data, char field_id, char *value, size_t value_len);

/**
 * @brief Encode Mic-E destination address.
 *
 * This function encodes the latitude and message bits into the destination address for Mic-E packets.
 *
 * @param dest_str Output buffer for the encoded destination address (7 characters).
 * @param data Pointer to the Mic-E data structure.
 * @return 0 on success, -1 on error.
 */
int aprs_encode_mice_destination(char *dest_str, const aprs_mice_t *data);

/**
 * @brief Encode Mic-E information field.
 *
 * This function encodes the longitude, speed, course, and symbol into the information field for Mic-E packets.
 *
 * @param info Output buffer for the information field.
 * @param len Size of the output buffer.
 * @param data Pointer to the Mic-E data structure.
 * @return Number of bytes written to the buffer, or -1 on error.
 */
int aprs_encode_mice_info(char *info, size_t len, const aprs_mice_t *data);

/**
 * @brief Decode Mic-E destination address.
 *
 * This function decodes the latitude and message bits from the destination address of a Mic-E packet.
 *
 * @param dest_str Input destination address string.
 * @param data Pointer to the Mic-E data structure to fill.
 * @param message_bits Pointer to store the message bits.
 * @param ns Pointer to store the north/south indicator.
 * @param long_offset Pointer to store the longitude offset indicator.
 * @param we Pointer to store the west/east indicator.
 * @return 0 on success, -1 on error.
 */
int aprs_decode_mice_destination(const char *dest_str, aprs_mice_t *data, int *message_bits, bool *ns, bool *long_offset, bool *we);

/**
 * @brief Decode Mic-E information field.
 *
 * This function decodes the longitude, speed, course, and symbol from the information field of a Mic-E packet.
 *
 * @param info Input buffer containing the information field.
 * @param len Length of the input buffer.
 * @param data Pointer to the Mic-E data structure to fill.
 * @param long_offset Longitude offset indicator.
 * @param we West/east indicator.
 * @return 0 on success, -1 on error.
 */
int aprs_decode_mice_info(const char *info, size_t len, aprs_mice_t *data, bool long_offset, bool we);

/**
 * @brief Encode an APRS telemetry report.
 *
 * This function encodes a telemetry report packet into the information field.
 *
 * @param info Output buffer for the information field.
 * @param len Size of the output buffer.
 * @param data Pointer to the telemetry data structure.
 * @return Number of bytes written to the buffer, or -1 on error.
 */
int aprs_encode_telemetry(char *info, size_t len, const aprs_telemetry_t *data);

/**
 * @brief Decode an APRS telemetry report.
 *
 * This function decodes a telemetry report packet from the information field into a structure.
 *
 * @param info Input buffer containing the information field.
 * @param data Pointer to the telemetry data structure to fill.
 * @return 0 on success, -1 on error.
 */
int aprs_decode_telemetry(const char *info, aprs_telemetry_t *data);

/**
 * @brief Encode an APRS status report.
 *
 * This function encodes a status report packet into the information field, with optional timestamp.
 *
 * @param info Output buffer for the information field.
 * @param len Size of the output buffer.
 * @param data Pointer to the status data structure.
 * @return Number of bytes written to the buffer, or -1 on error.
 */
int aprs_encode_status(char *info, size_t len, const aprs_status_t *data);

/**
 * @brief Decode an APRS status report.
 *
 * This function decodes a status report packet from the information field into a structure.
 *
 * @param info Input buffer containing the information field.
 * @param data Pointer to the status data structure to fill.
 * @return 0 on success, -1 on error.
 */
int aprs_decode_status(const char *info, aprs_status_t *data);

/**
 * @brief Encode an APRS general query.
 *
 * This function encodes a general query packet into the information field.
 *
 * @param info Output buffer for the information field.
 * @param len Size of the output buffer.
 * @param data Pointer to the general query data structure.
 * @return Number of bytes written to the buffer, or -1 on error.
 */
int aprs_encode_general_query(char *info, size_t len, const aprs_general_query_t *data);

/**
 * @brief Decode an APRS general query.
 *
 * This function decodes a general query packet from the information field into a structure.
 *
 * @param info Input buffer containing the information field.
 * @param data Pointer to the general query data structure to fill.
 * @return 0 on success, -1 on error.
 */
int aprs_decode_general_query(const char *info, aprs_general_query_t *data);

/**
 * @brief Encode APRS station capabilities.
 *
 * This function encodes a station capabilities packet into the information field.
 *
 * @param info Output buffer for the information field.
 * @param len Size of the output buffer.
 * @param data Pointer to the station capabilities data structure.
 * @return Number of bytes written to the buffer, or -1 on error.
 */
int aprs_encode_station_capabilities(char *info, size_t len, const aprs_station_capabilities_t *data);

/**
 * @brief Decode APRS station capabilities.
 *
 * This function decodes a station capabilities packet from the information field into a structure.
 *
 * @param info Input buffer containing the information field.
 * @param data Pointer to the station capabilities data structure to fill.
 * @return 0 on success, -1 on error.
 */
int aprs_decode_station_capabilities(const char *info, aprs_station_capabilities_t *data);

#endif /* APRS_H_ */
