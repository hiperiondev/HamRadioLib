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

#define APRS_COMMENT_LEN 100

// Data Type Identifiers (DTIs) for APRS packets
#define APRS_DTI_POSITION_NO_TS_NO_MSG '!'     // Position without timestamp (no messaging)
#define APRS_DTI_POSITION_NO_TS_WITH_MSG '='   // Position without timestamp (with messaging)
#define APRS_DTI_POSITION_WITH_TS_NO_MSG '/'   // Position with timestamp (no messaging)
#define APRS_DTI_POSITION_WITH_TS_WITH_MSG '@' // Position with timestamp (with messaging)
#define APRS_DTI_OBJECT_REPORT ';'             // Object report
#define APRS_DTI_ITEM_REPORT ')'               // Item report
#define APRS_DTI_MESSAGE ':'                   // Message, includes bulletins and announcements
#define APRS_DTI_STATUS '>'                    // Status report
#define APRS_DTI_QUERY '?'                     // Query
#define APRS_DTI_STATION_CAPABILITIES '<'      // Station capabilities
#define APRS_DTI_TELEMETRY 'T'                 // Telemetry data
#define APRS_DTI_WEATHER_REPORT '_'            // Weather report
#define APRS_DTI_PEET_BROS_RAW_1 '#'           // Peet Bros U-II Weather Station (raw, format 1)
#define APRS_DTI_PEET_BROS_RAW_2 '*'           // Peet Bros U-II Weather Station (raw, format 2)
#define APRS_DTI_RAW_GPS '$'                   // Raw GPS data (e.g., NMEA sentences)
#define APRS_DTI_USER_DEFINED '{'              // User-defined format
#define APRS_DTI_THIRD_PARTY '}'               // Third-party format
#define APRS_DTI_TEST_PACKET ','               // Test packet (invalid or test data)
#define APRS_DTI_GRID_SQUARE '['               // Maidenhead grid locator beacon
#define APRS_DTI_DF_REPORT '+'                 // Direction Finding report
#define APRS_DTI_AGRELO '%'                    // Agrelo format (proprietary)
#define APRS_DTI_RESERVED_1 '&'                // Reserved for future use
#define APRS_DTI_MIC_E_CURRENT '`'             // Current Mic-E data
#define APRS_DTI_MIC_E_OLD '\''                // Old Mic-E data
#define APRS_DTI_RESERVED_2 '"'                // Reserved (non-standard)

// Structure holding local station data for query responses
typedef struct {
    char callsign[10];          // Local station callsign (null-terminated, max 9 chars)
    char software_version[50];  // Version/identifier string for ?APRS?
    char status_text[63];       // Status text for ?INFO?
    double latitude;            // Latitude for ?LOC?
    double longitude;           // Longitude for ?LOC?
    char symbol_table;          // Symbol table for position
    char symbol_code;           // Symbol code for position
    // Optionally include altitude or other fields if needed
    bool has_dest;              // If true, use dest_lat/lon for ?DST?
    double dest_lat, dest_lon;  // Destination coordinates for ?DST?
    bool has_altitude;          // If true, include altitude in responses
    int altitude;               // Altitude (feet) if has_altitude is true
    // Timestamp of last beacon for ?TIME? (format "DDHHMMz")
    char timestamp[8];          // e.g. "061230z"
} aprs_station_info_t;

/**
 * @brief Structure for APRS Base91 compressed position report.
 *
 * This structure holds data for a Base91 compressed position report, including position, speed, course, altitude, and symbol.
 */
typedef struct {
    double latitude;      ///< Latitude in decimal degrees (-90 to 90).
    double longitude;     ///< Longitude in decimal degrees (-180 to 180).
    int speed;            ///< Speed in knots (0-1943), or -1 if not available.
    int course;           ///< Course in degrees (0-360), or -1 if not available.
    int altitude;         ///< Altitude in feet, or INT_MIN if not available.
    char symbol_table;    ///< Symbol table identifier ('/' or '\').
    char symbol_code;     ///< Symbol code (printable ASCII).
    char *comment;        ///< Optional comment field, null-terminated.
    char dti;             ///< Data Type Indicator ('!' or '=' for compressed reports).
    bool has_course_speed; ///< Flag indicating if course and speed are included.
    bool has_altitude;    ///< Flag indicating if altitude is included.
} aprs_compressed_position_t;

/**
 * @brief Structure to represent an AX.25 frame for APRS packets.
 *
 * This structure contains the source, destination, digipeater path, and information field of an APRS packet.
 */
typedef struct {
    char *info;                   ///< Information field of the APRS packet.
    size_t info_len;              ///< Length of the information field.
} aprs_frame_t;

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
    int ambiguity;        ///< Ambiguity level (0-4) for position precision.
} aprs_position_no_ts_t;

typedef struct {
    double latitude;      ///< Latitude in decimal degrees (-90 to 90).
    double longitude;     ///< Longitude in decimal degrees (-180 to 180).
    char symbol_table;    ///< Symbol table identifier ('/' or '\').
    char symbol_code;     ///< Symbol code (printable ASCII).
    char *comment;        ///< Optional comment field, null-terminated.
    char timestamp[8];    ///< Timestamp in UTC (DDHHMMz for day/hour/minute).
    char dti;             ///< Data Type Indicator ('@' or '/' for timestamped reports).
    bool has_course_speed; ///< Flag indicating if course and speed are included.
    int course;           ///< Course in degrees (0-360), if has_course_speed is true.
    int speed;            ///< Speed in knots (>=0), if has_course_speed is true.
    int ambiguity;        ///< Ambiguity level (0-4) for position precision.
} aprs_position_with_ts_t;

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
    char timestamp[9];
    char timestamp_format[4];
    bool is_zulu;
    float temperature;
    int wind_speed;
    int wind_direction;
    int wind_gust;
    int rainfall_last_hour;
    int rainfall_24h;
    int rainfall_since_midnight;
    int barometric_pressure;
    int humidity;
    int luminosity;
    float snowfall_24h;
    int rain_rate;
    float water_height_feet;
    float water_height_meters;
    float indoors_temperature;
    int indoors_humidity;
    int raw_rain_counter;
    int rain_1h;          // Rain in last hour
    int rain_24h;         // Rain in last 24 hours
    int rain_midnight;    // Rain since midnight
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
    bool killed;           // false = live (*), true = killed (_)
} aprs_object_report_t;

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
    bool killed;           // false = live (*), true = killed (_)
    char timestamp[8];     // DDHHMM[z|l] + NULL
} aprs_item_report_t;

typedef struct {
    char *raw_data;       // The raw NMEA sentence or data
    size_t data_len;      // Length of the raw data
} aprs_raw_gps_t;

typedef struct {
    char grid_square[7];  // Grid square code, up to 6 characters + null terminator
    char *comment;        // Optional comment
} aprs_grid_square_t;

typedef struct {
    int bearing;          // Bearing in degrees (0-359)
    int signal_strength;  // Signal strength (e.g., 0-9)
    char *comment;        // Optional comment
} aprs_df_report_t;

typedef struct {
    char *data;           // The packet data
    size_t data_len;      // Length of the data
} aprs_test_packet_t;

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

/**
 * @brief Encode an APRS raw GPS data packet.
 *
 * This function encodes raw GPS data, such as NMEA sentences, into the APRS information field. The data is prefixed
 * with the raw GPS Data Type Indicator (DTI) '$'. The input data must start with "GP" (indicating a GPS-related
 * sentence) and be at least 3 characters long.
 *
 * @param info Output buffer for the information field where the encoded packet will be written.
 * @param len Size of the output buffer, which must be at least `data->data_len + 2` to accommodate the DTI, data,
 *            and null terminator.
 * @param data Pointer to the raw GPS data structure containing the NMEA sentence or similar data and its length.
 * @return Number of bytes written to the buffer (including the DTI), or -1 on error. Errors occur if:
 *         - `data` or `data->raw_data` is NULL,
 *         - `data->data_len` is less than 3,
 *         - `data->raw_data` does not start with "GP",
 *         - The buffer size `len` is insufficient.
 * @note The caller must ensure the output buffer is large enough to hold the DTI, the raw data, and a null terminator.
 * @see aprs_decode_raw_gps
 */
int aprs_encode_raw_gps(char *info, size_t len, const aprs_raw_gps_t *data);

/**
 * @brief Encode an APRS grid square report.
 *
 * This function encodes a Maidenhead grid square report into the APRS information field, prefixed with the '>'
 * character (used as a DTI in this implementation). The grid square code must be 4 or 6 characters long, followed
 * by a space and an optional comment. The total encoded length includes the DTI, grid square, space, and comment.
 *
 * @param info Output buffer for the information field where the encoded packet will be written.
 * @param len Size of the output buffer, which must be at least the length of the grid square plus comment, space,
 *            DTI, and null terminator.
 * @param data Pointer to the grid square data structure containing the grid square code and optional comment.
 * @return Number of bytes written to the buffer, or -1 on error. Errors occur if:
 *         - `data` is NULL,
 *         - `len` is too small,
 *         - `data->grid_square` is not 4 or 6 characters long.
 * @note The grid square length is strictly validated to be either 4 or 6 characters as per Maidenhead conventions.
 * @see aprs_decode_grid_square
 */
int aprs_encode_grid_square(char *info, size_t len, const aprs_grid_square_t *data);

/**
 * @brief Encode an APRS direction finding (DF) report.
 *
 * This function encodes a direction finding report into the APRS information field, prefixed with the '@' character
 * (used as a DTI in this implementation). The report includes a 3-digit bearing (000-359), a slash, a single-digit
 * signal strength (0-9) padded to 3 digits (e.g., "S00"), and an optional comment.
 *
 * @param info Output buffer for the information field where the encoded packet will be written.
 * @param len Size of the output buffer, which must be at least 8 (DTI + bearing + slash + signal strength) plus
 *            the comment length and null terminator.
 * @param data Pointer to the DF report data structure containing bearing, signal strength, and optional comment.
 * @return Number of bytes written to the buffer, or -1 on error. Errors occur if:
 *         - `data` is NULL,
 *         - `data->bearing` is not between 0 and 359,
 *         - `data->signal_strength` is not between 0 and 9,
 *         - `len` is insufficient.
 * @note The signal strength is encoded as a single digit followed by "00" (e.g., "500" for strength 5).
 * @see aprs_decode_df_report
 */
int aprs_encode_df_report(char *info, size_t len, const aprs_df_report_t *data);

/**
 * @brief Encode an APRS test packet.
 *
 * This function encodes a test packet with arbitrary data into the APRS information field, prefixed with the test
 * packet Data Type Indicator (DTI) ','. This is useful for testing APRS systems with custom or experimental data.
 *
 * @param info Output buffer for the information field where the encoded packet will be written.
 * @param len Size of the output buffer, which must be at least `data->data_len + 2` to accommodate the DTI, data,
 *            and null terminator.
 * @param data Pointer to the test packet data structure containing the arbitrary data and its length.
 * @return Number of bytes written to the buffer (including the DTI), or -1 on error. Errors occur if:
 *         - `len` is insufficient to hold the DTI, data, and null terminator.
 * @note No specific validation is performed on the content of `data->data`, allowing flexibility for testing purposes.
 * @see aprs_decode_test_packet
 */
int aprs_encode_test_packet(char *info, size_t len, const aprs_test_packet_t *data);

/**
 * @brief Decode an APRS raw GPS data packet.
 *
 * This function decodes raw GPS data from the APRS information field into a structure. The data must be prefixed
 * with the raw GPS Data Type Indicator (DTI) '$'. The decoded data is dynamically allocated and stored in the
 * provided structure.
 *
 * @param info Input buffer containing the information field with the encoded raw GPS data.
 * @param data Pointer to the raw GPS data structure to fill with the decoded NMEA sentence or data and its length.
 * @return 0 on success, -1 on error. Errors occur if:
 *         - The first character is not '$',
 *         - The input is empty after the DTI,
 *         - Memory allocation for `data->raw_data` fails.
 * @note The caller is responsible for freeing `data->raw_data` to prevent memory leaks.
 * @see aprs_encode_raw_gps
 */
int aprs_decode_raw_gps(const char *info, aprs_raw_gps_t *data);

/**
 * @brief Decode an APRS grid square report.
 *
 * This function decodes a grid square report from the APRS information field into a structure. The data must be
 * prefixed with '>' (used as a DTI in this implementation), followed by a 4- or 6-character grid square, a space,
 * and an optional comment. The decoded comment, if present, is dynamically allocated.
 *
 * @param info Input buffer containing the information field with the encoded grid square report.
 * @param data Pointer to the grid square data structure to fill with the grid square code and optional comment.
 * @return 0 on success, -1 on error. Errors occur if:
 *         - The first character is not '>',
 *         - The input length is less than 6 (DTI + 4 chars + space),
 *         - No space separator is found,
 *         - The grid square length is not 4 or 6,
 *         - Memory allocation for `data->comment` fails (if a comment is present).
 * @note The caller must free `data->comment` if it is non-NULL to prevent memory leaks.
 * @see aprs_encode_grid_square
 */
int aprs_decode_grid_square(const char *info, aprs_grid_square_t *data);

/**
 * @brief Decode an APRS direction finding (DF) report.
 *
 * This function decodes a direction finding report from the APRS information field into a structure. The data must
 * be prefixed with '@' (used as a DTI in this implementation), followed by a 3-digit bearing (000-359), a slash,
 * a 3-digit signal strength field (e.g., "S00" where S is 0-9), and an optional comment. The decoded comment, if
 * present, is dynamically allocated.
 *
 * @param info Input buffer containing the information field with the encoded DF report.
 * @param data Pointer to the DF report data structure to fill with bearing, signal strength, and optional comment.
 * @return 0 on success, -1 on error. Errors occur if:
 *         - The first character is not '@',
 *         - The input length is less than 8 (DTI + bearing + slash + signal strength),
 *         - The separator is not '/',
 *         - `bearing` is not between 0 and 359,
 *         - `signal_strength` is not between 0 and 9,
 *         - Memory allocation for `data->comment` fails (if a comment is present).
 * @note The signal strength is extracted from the first digit of the 3-digit field (e.g., "500" yields 5).
 * @note The caller must free `data->comment` if it is non-NULL to prevent memory leaks.
 * @see aprs_encode_df_report
 */
int aprs_decode_df_report(const char *info, aprs_df_report_t *data);

/**
 * @brief Decode an APRS test packet.
 *
 * This function decodes a test packet from the APRS information field into a structure. The data must be prefixed
 * with the test packet Data Type Indicator (DTI) ','. The decoded data is dynamically allocated and stored in the
 * provided structure.
 *
 * @param info Input buffer containing the information field with the encoded test packet.
 * @param data Pointer to the test packet data structure to fill with the arbitrary data and its length.
 * @return 0 on success, -1 on error. Errors occur if:
 *         - The first character is not ',',
 *         - Memory allocation for `data->data` fails.
 * @note The caller is responsible for freeing `data->data` to prevent memory leaks.
 * @see aprs_encode_test_packet
 */
int aprs_decode_test_packet(const char *info, aprs_test_packet_t *data);

/**
 * @brief Encode an APRS Base91 compressed position report.
 *
 * This function encodes a compressed position report using Base91 encoding into the information field.
 * The compressed format is: DTI + 4-char lat + 4-char lon + symbol + 2-char data + compression_type + comment
 *
 * @param info Output buffer for the information field.
 * @param len Size of the output buffer.
 * @param data Pointer to the compressed position data structure.
 * @return Number of bytes written to the buffer, or -1 on error.
 */
int aprs_encode_compressed_position(char *info, size_t len, const aprs_compressed_position_t *data);

/**
 * @brief Decode an APRS Base91 compressed position report.
 *
 * This function decodes a Base91 compressed position report from the information field into a structure.
 *
 * @param info Input buffer containing the information field.
 * @param data Pointer to the compressed position data structure to fill.
 * @return 0 on success, -1 on error.
 */
int aprs_decode_compressed_position(const char *info, aprs_compressed_position_t *data);

/**
 * @brief Check if an APRS packet contains a compressed position.
 *
 * This function checks if the given APRS information field contains a Base91 compressed position.
 *
 * @param info Input buffer containing the information field.
 * @return true if the packet contains a compressed position, false otherwise.
 */
bool aprs_is_compressed_position(const char *info);

/**
 * @brief Free memory allocated for compressed position structure.
 *
 * This function frees the dynamically allocated comment field in the compressed position structure.
 *
 * @param data Pointer to the compressed position data structure.
 */
void aprs_free_compressed_position(aprs_compressed_position_t *data);

int aprs_decode_peet1(const char *info, aprs_weather_report_t *data);
int aprs_decode_peet2(const char *info, aprs_weather_report_t *data);
int aprs_encode_peet1(char *dst, int len, const aprs_weather_report_t *data);
int aprs_encode_peet2(char *dst, int len, const aprs_weather_report_t *data);
int aprs_decode_position_weather(const aprs_position_no_ts_t *pos, aprs_weather_report_t *w);
int aprs_handle_directed_query(const aprs_message_t *msg, char *info, size_t len, aprs_station_info_t local_station);

#endif /* APRS_H_ */
