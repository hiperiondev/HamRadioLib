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

/** @defgroup aprs_dti Data Type Identifiers (DTIs)
 *  @brief Single-character identifiers that select the APRS information format.
 *  @{
 */

/** Position w/o timestamp, no messaging. */
#define APRS_DTI_POSITION_NO_TS_NO_MSG      '!'
/** Position w/o timestamp, with messaging. */
#define APRS_DTI_POSITION_NO_TS_WITH_MSG    '='
/** Position with timestamp, no messaging. */
#define APRS_DTI_POSITION_WITH_TS_NO_MSG    '/'
/** Position with timestamp, with messaging. */
#define APRS_DTI_POSITION_WITH_TS_WITH_MSG  '@'
/** Object report. */
#define APRS_DTI_OBJECT_REPORT              ';'
/** Item report. */
#define APRS_DTI_ITEM_REPORT                ')'
/** Message (also bulletins/announcements). */
#define APRS_DTI_MESSAGE                    ':'
/** Status report. */
#define APRS_DTI_STATUS                     '>'
/** Query. */
#define APRS_DTI_QUERY                      '?'
/** Station capabilities. */
#define APRS_DTI_STATION_CAPABILITIES       '<'
/** Telemetry. */
#define APRS_DTI_TELEMETRY                  'T'
/** Weather report. */
#define APRS_DTI_WEATHER_REPORT             '_'
/** Peet Bros U-II raw weather, format 1. */
#define APRS_DTI_PEET_BROS_RAW_1            '#'
/** Peet Bros U-II raw weather, format 2. */
#define APRS_DTI_PEET_BROS_RAW_2            '*'
/** Raw GPS (e.g., NMEA). */
#define APRS_DTI_RAW_GPS                    '$'
/** User-defined format. */
#define APRS_DTI_USER_DEFINED               '{'
/** Third-party format. */
#define APRS_DTI_THIRD_PARTY                '}'
/** Test/invalid packet. */
#define APRS_DTI_TEST_PACKET                ','
/** Maidenhead grid locator beacon. */
#define APRS_DTI_GRID_SQUARE                '['
/** Direction Finding report. */
#define APRS_DTI_DF_REPORT                  '+'
/** Reserved (future use). */
#define APRS_DTI_RESERVED_1                 '&'
/** Mic-E (current). */
#define APRS_DTI_MIC_E_CURRENT              '`'
/** Mic-E (old). */
#define APRS_DTI_MIC_E_OLD                  '\''
/** Test data (old). */
#define APRS_DTI_RESERVED_2                 '"'
/** Ultimeter 2000 raw weather. */
#define APRS_DTI_ULTIMETER                  '$'
// Define DTI for DX Spot
#define APRS_DTI_AGRELO                     '%'
/** @} */

/** @name Common buffer size limits
 *  @{
 */
/** Maximum length of an APRS header (AX.25 path and station fields), characters. */
#define APRS_MAX_HEADER_LEN   128
/** Maximum length of an APRS information field, characters. */
#define APRS_MAX_INFO_LEN     512
/** Maximum recommended free-form comment length used in some structures. */
#define APRS_COMMENT_LEN      200
/** @} */

/** Limits for Ultimeter sanity checks (tenths of °F). */  // MOD
#define APRS_ULT_TEMPF_TENTHS_MIN   (-900)           // MOD
#define APRS_ULT_TEMPF_TENTHS_MAX   (1500)           // MOD

/* =========================
 *        Structures
 * ========================= */

/**
 * @brief Third-party packet wrapper (DTI '}').
 *
 * The APRS "third-party" format encapsulates another APRS packet as text.
 */
typedef struct {
    char header[APRS_MAX_HEADER_LEN]; /**< Outer packet header/path (NUL-terminated). */
    char inner_info[APRS_MAX_INFO_LEN]; /**< Encapsulated APRS info field (NUL-terminated). */
} aprs_third_party_packet_t;

/**
 * @brief PHG (Power-Height-Gain) descriptor.
 *
 * Use -1 in members that are not present.
 */
typedef struct {
    int power; /**< Transmitter power (0–9), or -1 if absent. */
    int height; /**< Antenna height (0–9 or A–Z in APRS text), or -1. */
    int gain; /**< Antenna gain (0–9), or -1. */
    int direction; /**< Antenna directivity (0–9), or -1. */
} aprs_phg_t;

/**
 * @brief Minimal position-without-timestamp report used by helpers.
 */
typedef struct {
    double latitude; /**< Latitude in decimal degrees (-90..90). */
    double longitude; /**< Longitude in decimal degrees (-180..180). */
    char symbol; /**< Symbol code character. */
    int altitude; /**< Altitude in feet, or -1 if absent. */
    aprs_phg_t phg; /**< Optional PHG (set members to -1 if absent). */
    char comment[100]; /**< Optional trailing comment (NUL-terminated). */
} aprs_position_report_t;

/**
 * @brief Local station information used to answer directed queries.
 *
 * All string fields are NUL-terminated; booleans toggle inclusion of optional values.
 */
typedef struct {
    char callsign[10]; /**< Local station callsign (up to 9 chars + NUL). */
    char software_version[50]; /**< Identifier/version string for ?APRS?. */
    char status_text[63]; /**< Status used for ?INFO? (up to 62 chars + NUL). */
    double latitude; /**< Lat for ?LOC?. */
    double longitude; /**< Lon for ?LOC?. */
    char symbol_table; /**< Symbol table for position responses. */
    char symbol_code; /**< Symbol code for position responses. */
    bool has_dest; /**< If true, include destination coordinates. */
    double dest_lat; /**< Destination latitude for ?DST?. */
    double dest_lon; /**< Destination longitude for ?DST?. */
    bool has_altitude; /**< If true, include altitude. */
    int altitude; /**< Altitude (feet), valid if has_altitude. */
    char timestamp[8]; /**< Last beacon timestamp "DDHHMMz" for ?TIME?. */
} aprs_station_info_t;

/**
 * @brief User-defined format (DTI '{').
 */
typedef struct {
    char userID; /**< One-character user ID (after '{'). */
    char packetType; /**< One-character user-defined packet type. */
    char data[256]; /**< Arbitrary printable ASCII payload (NUL-terminated). */
} aprs_user_defined_format_t;

/**
 * @brief Compressed position (Base-91) representation (DTIs '!'/'=').
 *
 * Optional course/speed/altitude are controlled by flags.
 * Some members (e.g., comment) may be dynamically allocated by decoders.
 */
typedef struct {
    double latitude; /**< Latitude in decimal degrees (-90..90). */
    double longitude; /**< Longitude in decimal degrees (-180..180). */
    int speed; /**< Speed in knots (0..1943), or -1. */
    int course; /**< Course in degrees (0..360), or -1. */
    int altitude; /**< Altitude in feet, or INT_MIN if not available. */
    char symbol_table; /**< Symbol table ('/' or '\\'). */
    char symbol_code; /**< Symbol code (printable ASCII). */
    char *comment; /**< Optional comment (NUL-terminated, malloc'd if decoded). */
    char dti; /**< Data Type Indicator ('!' or '='). */
    bool has_course_speed; /**< True if course/speed were present. */
    bool has_altitude; /**< True if altitude was present. */
} aprs_compressed_position_t;

/**
 * @brief Generic APRS frame slice exposing only the info field and its length.
 */
typedef struct {
    char *info; /**< Pointer to information field buffer. */
    size_t info_len; /**< Length (bytes) of the information field. */
} aprs_frame_t;

/**
 * @brief Position report without timestamp (DTIs '!' or '=').
 *
 * Optional course/speed, altitude, PHG and DAO extension are supported.
 */
typedef struct {
    double latitude; /**< Latitude in decimal degrees. */
    double longitude; /**< Longitude in decimal degrees. */
    char symbol_table; /**< Symbol table ('/' or '\\'). */
    char symbol_code; /**< Symbol code. */
    char *comment; /**< Optional free-form comment (malloc'd by decoders). */
    char dti; /**< DTI used to encode the position ('!' or '='). */
    bool has_course_speed; /**< True if course/speed are set. */
    int course; /**< Course in degrees, valid if has_course_speed. */
    int speed; /**< Speed in knots, valid if has_course_speed. */
    int ambiguity; /**< Position ambiguity (0..4). */
    int altitude; /**< Altitude in feet, or -1 if absent. */
    aprs_phg_t phg; /**< Optional PHG (members -1 if absent). */
    bool has_dao; /**< True if DAO extra digits are present. */
    char dao_datum; /**< DAO datum ID (uppercase letter), valid if has_dao. */
    char dao_lat_extra; /**< Extra digit for latitude precision (0..9 base-91 char). */
    char dao_lon_extra; /**< Extra digit for longitude precision (0..9 base-91 char). */
    int lat_ambiguity; /**< per-axis ambiguity for latitude (0–4) */
    int lon_ambiguity; /**< per-axis ambiguity for longitude (0–4) */
} aprs_position_no_ts_t;

/**
 * @brief Position report with timestamp (DTIs '/' or '@').
 */
typedef struct {
    double latitude; /**< Latitude in decimal degrees. */
    double longitude; /**< Longitude in decimal degrees. */
    char symbol_table; /**< Symbol table ('/' or '\\'). */
    char symbol_code; /**< Symbol code. */
    char *comment; /**< Optional comment (malloc'd by decoders). */
    char timestamp[8]; /**< "DDHHMMz" (UTC) or "DDHHMMl" (local). */
    char dti; /**< DTI used ('/' or '@'). */
    bool has_course_speed; /**< True if course/speed are set. */
    int course; /**< Course in degrees, valid if has_course_speed. */
    int speed; /**< Speed in knots, valid if has_course_speed. */
    int ambiguity; /**< Position ambiguity (0..4). */
    int lat_ambiguity; /**< per-axis ambiguity for latitude (0–4) */
    int lon_ambiguity; /**< per-axis ambiguity for longitude (0–4) */
} aprs_position_with_ts_t;

/**
 * @brief APRS text message (also used for bulletins/announcements).
 */
typedef struct {
    char addressee[10]; /**< Addressee callsign (up to 9 chars + NUL). */
    char *message; /**< Message text (up to 67 chars, malloc'd by decoder). */
    char *message_number; /**< Optional message number (up to 5 chars), malloc'd. */
} aprs_message_t;

/**
 * @brief APRS weather report (canonical and vendor/raw variants).
 *
 * Some fields are optional and may be left unset; boolean flags indicate
 * presence where necessary. Raw Peet Bros and other vendor formats are
 * supported via dedicated encode/decode helpers.
 */
typedef struct {
    bool has_position; /**< True if latitude/longitude/symbols are set. */
    double latitude; /**< Latitude in decimal degrees. */
    double longitude; /**< Longitude in decimal degrees. */
    char symbol_table; /**< Symbol table for weather icon. */
    char symbol_code; /**< Symbol code for weather icon. */
    bool has_timestamp; /**< True if timestamp is set. */
    char timestamp[9]; /**< "DDHHMMz" or "DDHHMMSS" (NUL-terminated). */
    char timestamp_format[4]; /**< Small tag string to indicate parsed format. */
    bool is_zulu; /**< True if timestamp is UTC ('z'), else local. */
    float temperature; /**< Ambient temperature (°C or °F per context). */
    int wind_speed; /**< Wind speed (knots). */
    int wind_direction; /**< Wind direction (degrees). */
    int wind_gust; /**< Wind gust (knots). */
    int rainfall_last_hour; /**< Rain last hour (0.01" units or context-specific). */
    int rainfall_24h; /**< Rain last 24h. */
    int rainfall_since_midnight;/**< Rain since midnight. */
    int barometric_pressure; /**< Barometric pressure (tenths of mbar or vendor units). */
    int humidity; /**< Relative humidity (%). */
    int luminosity; /**< Luminosity/solar radiation (vendor-specific). */
    float snowfall_24h; /**< Snowfall in last 24h. */
    int rain_rate; /**< Instantaneous rain rate. */
    float water_height_feet; /**< Water height (feet). */
    float water_height_meters; /**< Water height (meters). */
    float indoors_temperature; /**< Indoor temperature. */
    int indoors_humidity; /**< Indoor relative humidity. */
    int raw_rain_counter; /**< Raw rain counter (vendor). */
    int rain_1h; /**< Rain in last hour (duplicate convenience). */
    int rain_24h; /**< Rain in last 24 hours (duplicate convenience). */
    int rain_midnight; /**< Rain since midnight (duplicate convenience). */
} aprs_weather_report_t;

/**
 * @brief APRS object report (DTI ';').
 */
typedef struct {
    char name[10]; /**< Object name (up to 9 chars + NUL). */
    char timestamp[8]; /**< "DDHHMMz". */
    double latitude; /**< Latitude. */
    double longitude; /**< Longitude. */
    char symbol_table; /**< Symbol table. */
    char symbol_code; /**< Symbol code. */
    bool killed; /**< True if killed ('_'), false if live ('*'). */
    bool has_course_speed; /**< True if course/speed are present. */
    int course; /**< Course (degrees). */
    int speed; /**< Speed (knots). */
    aprs_phg_t phg; /**< Optional PHG. */
    char *comment; /**< Optional comment (malloc'd by decoder). */
} aprs_object_report_t;

/**
 * @brief Mic-E data representation (destination + info portions).
 */
typedef struct {
    double latitude; /**< Latitude. */
    double longitude; /**< Longitude. */
    int speed; /**< Speed (knots, 0..799). */
    int course; /**< Course (degrees). */
    char symbol_table; /**< Symbol table. */
    char symbol_code; /**< Symbol code. */
    char message_code[10]; /**< Message code / status (NUL-terminated). */
} aprs_mice_t;

/**
 * @brief APRS telemetry frame (sequence, 5 analog, 8 digital).
 */
typedef struct {
    unsigned int sequence_number; /**< Sequence number (0..999). */
    double analog[5]; /**< Analog channels (normalized or raw per usage). */
    uint8_t digital; /**< 8-bit digital bitmap (bit0 = channel 1). */
} aprs_telemetry_t;

/**
 * @brief Status report (DTI '>').
 */
typedef struct {
    bool has_timestamp; /**< True if timestamp is included. */
    char timestamp[8]; /**< "DDHHMMz" if present. */
    char status_text[63]; /**< Status text (up to 62 chars + NUL). */
} aprs_status_t;

/**
 * @brief General query (DTI '?').
 */
typedef struct {
    char query_type[11]; /**< Query key (e.g., "APRS"), up to 10 chars + NUL. */
} aprs_general_query_t;

/**
 * @brief Station capabilities (DTI '<').
 */
typedef struct {
    char capabilities_text[100];/**< Capabilities summary (NUL-terminated). */
} aprs_station_capabilities_t;

/**
 * @brief Bulletin/announcement wrapper using the message format.
 */
typedef struct {
    char bulletin_id[5]; /**< Bulletin ID (e.g., "BLN1"). */
    char *message; /**< Message text (malloc'd by decoder). */
    char *message_number; /**< Optional message number (malloc'd). */
} aprs_bulletin_t;

/**
 * @brief APRS item report (DTI ')').
 */
typedef struct {
    char name[10]; /**< Item name (up to 9 chars + NUL). */
    bool is_live; /**< True for live item, false for killed. */
    double latitude; /**< Latitude. */
    double longitude; /**< Longitude. */
    char symbol_table; /**< Symbol table. */
    char symbol_code; /**< Symbol code. */
    bool has_course_speed; /**< True if course/speed present. */
    int course; /**< Course (degrees). */
    int speed; /**< Speed (knots). */
    bool has_phg; /**< True if PHG is present. */
    aprs_phg_t phg; /**< Optional PHG. */
    char *comment; /**< Optional comment (malloc'd). */
    bool killed; /**< False = live ('*'), true = killed ('_'). */
    char timestamp[8]; /**< "DDHHMMz" or local variant. */
} aprs_item_report_t;

/**
 * @brief Kind of RAW ($) payload.                  // MOD: added enum definition
 */
typedef enum {                                       // MOD
    APRS_RAW_KIND_NMEA = 0,                          // MOD
    APRS_RAW_KIND_ULTIMETER = 1                      // MOD
} aprs_raw_kind_t;                                   // MOD

/**
 * @brief Raw GPS/NMEA or Ultimeter payload (DTI '$'). // MOD updated doc
 */
typedef struct {                                      // MOD
    aprs_raw_kind_t kind; /**< Payload kind. */   // MOD
    char *raw_data; /**< Raw payload after DTI ('$'). For NMEA, exclude leading '$' if present. malloc'd. */  // MOD
    size_t data_len; /**< Number of bytes in @ref raw_data. */  // MOD

    /* Parsed Ultimeter fields (valid when kind==APRS_RAW_KIND_ULTIMETER). Values are raw units noted. */  // MOD
    struct {                                          // MOD
        int has_field12;                         // MOD
        int has_field13;                         // MOD
        uint16_t wind_peak_0_1kph;                   // MOD field #1
        uint16_t wind_dir_peak;                       // MOD field #2 (0..255)
        int16_t temp_out_0_1F;                       // MOD field #3 signed
        uint16_t rain_total_0_01in;                   // MOD field #4
        uint16_t barometer_0_1mbar;                   // MOD field #5
        int16_t barometer_delta_0_1mbar;             // MOD field #6 signed
        uint16_t baro_corr_lsw;                       // MOD field #7
        uint16_t baro_corr_msw;                       // MOD field #8
        uint16_t humidity_out_0_1pct;                 // MOD field #9
        uint16_t day_of_year;                         // MOD field #10
        uint16_t minute_of_day;                       // MOD field #11
        uint16_t rain_today_0_01in;                   // MOD field #12 (optional)
        uint16_t wind_avg_1min_0_1kph;                // MOD field #13 (optional)
    } ult;                                            // MOD
} aprs_raw_gps_t;

/**
 * @brief Maidenhead grid locator beacon (DTI '[').
 */
typedef struct {
    char grid_square[7]; /**< 2–6 character grid (plus NUL). */
    char *comment; /**< Optional comment (malloc'd). */
} aprs_grid_square_t;

/**
 * @brief Direction Finding (DF) report (DTI '+').
 */
typedef struct {
    /* --- Position & symbol --- */
    double latitude;             // MODIFIED: Latitude in decimal degrees (N positive, S negative)
    double longitude;            // MODIFIED: Longitude in decimal degrees (E positive, W negative)
    char symbol_table;           // MODIFIED: Symbol table ID ('/' primary or '\\' alternate)
    char symbol_code;            // MODIFIED: Symbol code character

    /* --- Core DF data --- */
    int course;                  // MODIFIED: Course 0–360; 0 means fixed DF per spec; -1 if unknown
    int speed;                   // MODIFIED: Speed in knots 0–999; -1 if unknown
    int bearing;                 // MODIFIED: DF bearing 0–359 degrees
    int n_hits;                  // MODIFIED: NRQ 'N' digit 0–9 (0 means NRQ meaningless)
    int range;                   // MODIFIED: NRQ 'R' digit 0–9 (range ~ 2^R miles)
    int quality;                 // MODIFIED: NRQ 'Q' digit 0–9 (beamwidth quality)

    /* --- Optional timestamp --- */
    unsigned int timestamp;      // MODIFIED: If >0, encode '@' with HMS HHMMSSz (timestamp % 86400)

    /* --- Optional extensions --- */
    int dfs_strength;            // MODIFIED: DFS 's' 0–9, or -1 if absent (DFSshgd)
    aprs_phg_t phg;              // MODIFIED: PHG data; set members to -1 if absent

    /* --- Optional human comment --- */
    char df_comment[100];        // MODIFIED: Optional free-text comment (NUL-terminated)
} aprs_df_report_t;

/**
 * @brief Arbitrary test/invalid payload (DTI ',').
 */
typedef struct {
    char *data; /**< Opaque data (malloc'd). */
    size_t data_len; /**< Length in bytes. */
} aprs_test_packet_t;

typedef struct {
    char call[10];        // Call sign (e.g., "CALL")
    float frequency;      // Frequency (e.g., 145.500000)
    char message[100];    // Message content (e.g., "Test DX Spot")
} aprs_dx_spot_t;

/** @brief Agrelo DFJr / MicroFinder DF report (DTI '%'). // MODIFIED: Changed from aprs_dx_spot_t for DX spot (not in spec) to Agrelo DF format
 */
typedef struct {
    int bearing;  // MODIFIED: Changed from char call[10]; to int bearing; for Agrelo bearing (0-359 degrees)
    int quality;  // MODIFIED: Changed from float frequency and char message[100]; to int quality; for Agrelo quality (0-9)
} aprs_agrelo_df_t;  // MODIFIED: Changed struct name from aprs_dx_spot_t to aprs_agrelo_df_t for spec compliance

int aprs_encode_agrelo_df(char *info, size_t len, const aprs_agrelo_df_t *data);
int aprs_decode_agrelo_df(const char *info, aprs_agrelo_df_t *data);

int aprs_encode_df_report(char *buffer, int buf_size, aprs_df_report_t *report);     // MODIFIED: implemented per spec
int aprs_decode_df_report(const char *buffer, aprs_df_report_t *report);             // MODIFIED: decoder updated per spec

/** @name Bulletins / Items / Messages
 *  @{
 */
/**
 * @brief Encode a bulletin message into an APRS information field (DTI ':').
 * @param info Output buffer for the information field (NUL-terminated on success).
 * @param len  Size of @p info in bytes.
 * @param data Pointer to bulletin descriptor.
 * @return Number of characters written on success; negative on error.
 */
int aprs_encode_bulletin(char *info, size_t len, const aprs_bulletin_t *data);

/**
 * @brief Encode an item report (DTI ')').
 * @param info Output buffer (NUL-terminated on success).
 * @param len  Size of @p info in bytes.
 * @param data Item data to encode.
 * @return Number of characters written; negative on error.
 */
int aprs_encode_item_report(char *info, size_t len, const aprs_item_report_t *data);

/**
 * @brief Determine if a decoded message is a bulletin/announcement.
 * @param msg Decoded message structure.
 * @retval true  Message matches bulletin format.
 * @retval false Regular message.
 */
bool aprs_is_bulletin(const aprs_message_t *msg);

/**
 * @brief Decode an item report from an info field.
 * @param info Input NUL-terminated info string (without header/AX.25).
 * @param data Output structure to fill. Pointers may be allocated.
 * @return 0 on success; negative on error.
 */
int aprs_decode_item_report(const char *info, aprs_item_report_t *data);
/** @} */

/** @name Position helpers
 *  @{
 */
/**
 * @brief Convert latitude to APRS "DDMM.mmN/S" text with ambiguity.
 * @param lat        Latitude in decimal degrees.
 * @param ambiguity  Ambiguity level (0..4).
 * @return Pointer to an internal/static buffer containing the formatted string.
 */
char* lat_to_aprs(double lat, int ambiguity);

/**
 * @brief Convert longitude to APRS "DDDMM.mmE/W" text with ambiguity.
 * @param lon        Longitude in decimal degrees.
 * @param ambiguity  Ambiguity level (0..4).
 * @return Pointer to an internal/static buffer containing the formatted string.
 */
char* lon_to_aprs(double lon, int ambiguity);

/**
 * @brief Encode a position report without timestamp (DTIs '!' or '=').
 * @param info Output buffer.
 * @param len  Buffer size in bytes.
 * @param data Position to encode.
 * @return Characters written; negative on error.
 */
int aprs_encode_position_no_ts(char *info, size_t len, const aprs_position_no_ts_t *data);

/**
 * @brief Decode a position report without timestamp.
 * @param info Input NUL-terminated info field string.
 * @param data Output structure (pointers may be allocated).
 * @return 0 on success; negative on error.
 */
int aprs_decode_position_no_ts(const char *info, aprs_position_no_ts_t *data);

/**
 * @brief Encode a position report with timestamp (DTIs '/' or '@').
 * @param info Output buffer.
 * @param len  Buffer size in bytes.
 * @param data Position to encode (timestamp field must be filled).
 * @return Characters written; negative on error.
 */
int aprs_encode_position_with_ts(char *info, size_t len, const aprs_position_with_ts_t *data);

/**
 * @brief Decode a position report with timestamp.
 * @param info Input NUL-terminated info field string.
 * @param data Output structure.
 * @return 0 on success; negative on error.
 */
int aprs_decode_position_with_ts(const char *info, aprs_position_with_ts_t *data);

/**
 * @brief Parse latitude from "DDMM.mmN/S" text and report ambiguity.
 * @param str        Pointer to latitude string.
 * @param ambiguity  Output ambiguity (0..4) or NULL to ignore.
 * @return Decimal degrees; NaN on error.
 */
double aprs_parse_lat(const char *str, int *ambiguity);

/**
 * @brief Parse longitude from "DDDMM.mmE/W" text and report ambiguity.
 * @param str        Pointer to longitude string.
 * @param ambiguity  Output ambiguity (0..4) or NULL to ignore.
 * @return Decimal degrees; NaN on error.
 */
double aprs_parse_lon(const char *str, int *ambiguity);
/** @} */

/** @name Messages
 *  @{
 */
/**
 * @brief Encode a message (DTI ':').
 * @param info Output buffer.
 * @param len  Buffer size in bytes.
 * @param data Message (addressee/text/optional number).
 * @return Characters written; negative on error.
 */
int aprs_encode_message(char *info, size_t len, const aprs_message_t *data);

/**
 * @brief Decode a message (DTI ':').
 * @param info Input NUL-terminated info field.
 * @param data Output message structure (strings may be allocated).
 * @return 0 on success; negative on error.
 */
int aprs_decode_message(const char *info, aprs_message_t *data);
/** @} */

/** @name Weather
 *  @{
 */
/**
 * @brief Encode a canonical APRS weather report (DTI '_').
 * @param info Output buffer.
 * @param len  Buffer size in bytes.
 * @param data Weather values to encode.
 * @return Characters written; negative on error.
 */
int aprs_encode_weather_report(char *info, size_t len, const aprs_weather_report_t *data);

/**
 * @brief Decode a canonical APRS weather report (DTI '_').
 * @param info Input NUL-terminated info field.
 * @param data Output weather structure.
 * @return 0 on success; negative on error.
 */
int aprs_decode_weather_report(const char *info, aprs_weather_report_t *data);

/**
 * @brief Decode Peet Bros raw weather format #1 (DTI '#').
 * @param info Input NUL-terminated info field.
 * @param data Output weather values.
 * @return 0 on success; negative on error.
 */
int aprs_decode_peet1(const char *info, aprs_weather_report_t *data);

/**
 * @brief Decode Peet Bros raw weather format #2 (DTI '*').
 * @param info Input NUL-terminated info field.
 * @param data Output weather values.
 * @return 0 on success; negative on error.
 */
int aprs_decode_peet2(const char *info, aprs_weather_report_t *data);

/**
 * @brief Encode Peet Bros raw weather format #1 (DTI '#').
 * @param dst  Output buffer.
 * @param len  Buffer size in bytes.
 * @param data Weather values to encode.
 * @return Characters written; negative on error.
 */
int aprs_encode_peet1(char *dst, int len, const aprs_weather_report_t *data);

/**
 * @brief Encode Peet Bros raw weather format #2 (DTI '*').
 * @param dst  Output buffer.
 * @param len  Buffer size in bytes.
 * @param data Weather values to encode.
 * @return Characters written; negative on error.
 */
int aprs_encode_peet2(char *dst, int len, const aprs_weather_report_t *data);

/**
 * @brief Merge a decoded position (no-ts) into a weather report.
 * @param pos Decoded position (no timestamp).
 * @param w   Output/updated weather report that gains lat/lon/symbols.
 * @return 0 on success; negative on error.
 */
int aprs_decode_position_weather(const aprs_position_no_ts_t *pos, aprs_weather_report_t *w);
/** @} */

/** @name Objects / Items
 *  @{
 */
/**
 * @brief Encode an object report (DTI ';').
 * @param info Output buffer.
 * @param len  Buffer size in bytes.
 * @param data Object fields to encode.
 * @return Characters written; negative on error.
 */
int aprs_encode_object_report(char *info, size_t len, const aprs_object_report_t *data);

/**
 * @brief Decode an object report (DTI ';').
 * @param info Input NUL-terminated info field.
 * @param data Output object structure.
 * @return 0 on success; negative on error.
 */
int aprs_decode_object_report(const char *info, aprs_object_report_t *data);
/** @} */

/** @name Validation & helpers
 *  @{
 */

int aprs_validate_timestamp(const char *timestamp);
/**
 * @brief Extract a weather subfield by field ID (e.g., 't' for temperature).
 * @param data       Info field text to scan.
 * @param field_id   Field identifier character.
 * @param value      Output buffer for the extracted value as text.
 * @param value_len  Size of @p value buffer.
 * @return 0 if found and copied; negative if not found or invalid.
 */
int aprs_parse_weather_field(const char *data, char field_id, char *value, size_t value_len);
/** @} */

/** @name Mic-E
 *  @{
 */
/**
 * @brief Encode Mic-E destination field from logical values.
 * @param dest_str Output buffer to receive destination callsign field text.
 * @param data     Mic-E data (lat/lon/course/speed/symbol/message code).
 * @return Characters written; negative on error.
 */
int aprs_encode_mice_destination(char *dest_str, const aprs_mice_t *data);

/**
 * @brief Encode Mic-E information field from logical values.
 * @param info Output buffer.
 * @param len  Buffer size in bytes.
 * @param data Mic-E data.
 * @return Characters written; negative on error.
 */
int aprs_encode_mice_info(char *info, size_t len, const aprs_mice_t *data);

/**
 * @brief Decode Mic-E destination field into logical values.
 * @param dest_str    Source destination callsign field text.
 * @param data        Output Mic-E data.
 * @param message_bits Output Mic-E message bits.
 * @param ns          Output hemisphere flag (true = North).
 * @param long_offset Output long/lat offset flag (Mic-E encoding detail).
 * @param we          Output hemisphere flag (true = West).
 * @return 0 on success; negative on error.
 */
int aprs_decode_mice_destination(const char *dest_str, aprs_mice_t *data, int *message_bits, bool *ns, bool *long_offset, bool *we);

/**
 * @brief Decode Mic-E info field into logical values.
 * @param info       Input info field.
 * @param len        Length in bytes of @p info.
 * @param data       Output Mic-E data.
 * @param long_offset Flag from destination decode (encoding detail).
 * @param we         Hemisphere flag from destination decode.
 * @return 0 on success; negative on error.
 */
int aprs_decode_mice_info(const char *info, size_t len, aprs_mice_t *data, bool long_offset, bool we);
/** @} */

/** @name Telemetry / Status / Queries / Capabilities
 *  @{
 */
/**
 * @brief Encode telemetry report (DTI 'T').
 * @param info Output buffer.
 * @param len  Buffer size in bytes.
 * @param data Telemetry values (sequence/analog[5]/digital).
 * @return Characters written; negative on error.
 */
int aprs_encode_telemetry(char *info, size_t len, const aprs_telemetry_t *data);

/**
 * @brief Decode telemetry report (DTI 'T').
 * @param info Input NUL-terminated info field.
 * @param data Output telemetry structure.
 * @return 0 on success; negative on error.
 */
int aprs_decode_telemetry(const char *info, aprs_telemetry_t *data);

/**
 * @brief Encode status report (DTI '>').
 * @param info Output buffer.
 * @param len  Buffer size in bytes.
 * @param data Status (timestamp optional).
 * @return Characters written; negative on error.
 */
int aprs_encode_status(char *info, size_t len, const aprs_status_t *data);

/**
 * @brief Decode status report (DTI '>').
 * @param info Input NUL-terminated info field.
 * @param data Output status structure.
 * @return 0 on success; negative on error.
 */
int aprs_decode_status(const char *info, aprs_status_t *data);

/**
 * @brief Encode a general query (DTI '?').
 * @param info Output buffer.
 * @param len  Buffer size in bytes.
 * @param data Query descriptor.
 * @return Characters written; negative on error.
 */
int aprs_encode_general_query(char *info, size_t len, const aprs_general_query_t *data);

/**
 * @brief Decode a general query (DTI '?').
 * @param info Input NUL-terminated info field.
 * @param data Output query structure.
 * @return 0 on success; negative on error.
 */
int aprs_decode_general_query(const char *info, aprs_general_query_t *data);

/**
 * @brief Encode station capabilities (DTI '<').
 * @param info Output buffer.
 * @param len  Buffer size in bytes.
 * @param data Capabilities text.
 * @return Characters written; negative on error.
 */
int aprs_encode_station_capabilities(char *info, size_t len, const aprs_station_capabilities_t *data);

/**
 * @brief Decode station capabilities (DTI '<').
 * @param info Input NUL-terminated info field.
 * @param data Output capabilities structure.
 * @return 0 on success; negative on error.
 */
int aprs_decode_station_capabilities(const char *info, aprs_station_capabilities_t *data);

/**
 * @brief Handle a directed query and synthesize an APRS-compliant reply.
 * @param msg           Parsed incoming message (addressee/query text).
 * @param info          Output buffer to hold the reply info field.
 * @param len           Size of @p info in bytes.
 * @param local_station Local station information used to build the reply.
 * @return Characters written into @p info; negative on error or unhandled query.
 */
int aprs_handle_directed_query(const aprs_message_t *msg, char *info, size_t len, aprs_station_info_t local_station);
/** @} */

/** @name Raw GPS / Grid / DF / Test
 *  @{
 */
/**
 * @brief Encode raw GPS/NMEA (DTI '$').
 * @param info Output buffer.
 * @param len  Buffer size.
 * @param data Raw GPS payload.
 * @return Characters written; negative on error.
 */
int aprs_encode_raw_gps(char *info, size_t len, const aprs_raw_gps_t *data);

/**
 * @brief Encode Maidenhead grid beacon (DTI '[').
 * @param info Output buffer.
 * @param len  Buffer size.
 * @param data Grid square payload.
 * @return Characters written; negative on error.
 */
int aprs_encode_grid_square(char *info, size_t len, const aprs_grid_square_t *data);

/**
 * @brief Encode DF report (DTI '+').
 * @param info Output buffer.
 * @param len  Buffer size.
 * @param data DF values.
 * @return Characters written; negative on error.
 */
int aprs_encode_df_report(char *buffer, int buf_size, aprs_df_report_t *report);

int aprs_decode_df_report(const char *buffer, aprs_df_report_t *report);

/**
 * @brief Encode test/invalid packet (DTI ',').
 * @param info Output buffer.
 * @param len  Buffer size.
 * @param data Opaque test payload.
 * @return Characters written; negative on error.
 */
int aprs_encode_test_packet(char *info, size_t len, const aprs_test_packet_t *data);

/**
 * @brief Decode raw GPS/NMEA.
 * @param info Input NUL-terminated info field.
 * @param data Output raw GPS payload (strings may be allocated).
 * @return 0 on success; negative on error.
 */
int aprs_decode_raw_gps(const char *info, aprs_raw_gps_t *data);

/**
 * @brief Decode Maidenhead grid beacon.
 * @param info Input NUL-terminated info field.
 * @param data Output grid payload (comment may be allocated).
 * @return 0 on success; negative on error.
 */
int aprs_decode_grid_square(const char *info, aprs_grid_square_t *data);

/**
 * @brief Decode DF report.
 * @param info Input NUL-terminated info field.
 * @param data Output DF values.
 * @return 0 on success; negative on error.
 */
int aprs_decode_df_report(const char *info, aprs_df_report_t *data);

/**
 * @brief Decode test/invalid packet.
 * @param info Input NUL-terminated info field.
 * @param data Output payload.
 * @return 0 on success; negative on error.
 */
int aprs_decode_test_packet(const char *info, aprs_test_packet_t *data);
/** @} */

/** @name Compressed positions
 *  @{
 */
/**
 * @brief Encode a compressed position (Base-91).
 * @param info Output buffer.
 * @param len  Buffer size.
 * @param data Compressed position parameters.
 * @return Characters written; negative on error.
 */
int aprs_encode_compressed_position(char *info, size_t len, const aprs_compressed_position_t *data);

/**
 * @brief Decode a compressed position (Base-91).
 * @param info Input NUL-terminated info field.
 * @param data Output structure (comment may be malloc'd).
 * @return 0 on success; negative on error.
 */
int aprs_decode_compressed_position(const char *info, aprs_compressed_position_t *data);

/**
 * @brief Check whether an info field is a compressed position.
 * @param info Input NUL-terminated info field.
 * @retval true  Looks like a compressed position.
 * @retval false Otherwise.
 */
bool aprs_is_compressed_position(const char *info);

/**
 * @brief Free any dynamic memory owned by a decoded compressed position.
 * @param data Structure previously filled by a decoder.
 */
void aprs_free_compressed_position(aprs_compressed_position_t *data);
/** @} */

/** @name Misc parsing/encoding helpers used by higher-level code
 *  @{
 */
/**
 * @brief Compose a classic (uncompressed) position packet string.
 * @param pos Position report fields.
 * @param out Output buffer (sized by caller).
 */
void encodePositionPacket(const aprs_position_report_t *pos, char *out);

/**
 * @brief Parse altitude/PHG hints from a free-form comment into a position.
 * @param comment Source comment text.
 * @param pos     Position object updated in-place.
 */
void parseAltitudePHG(const char *comment, aprs_position_report_t *pos);

/**
 * @brief Parse the user-defined format payload (DTI '{') for diagnostics.
 * @param info Pointer to text after the '{' character.
 */
aprs_user_defined_format_t parse_user_defined(const char *info);

char* parse_third_party(const char *info);

/**
 * @brief Encode a user-defined format payload (DTI '{').
 * @param info Output buffer.
 * @param len  Buffer size.
 * @param data User-defined payload.
 * @return Characters written; negative on error.
 */
int aprs_encode_user_defined(char *info, size_t len, const aprs_user_defined_format_t *data);

/**
 * @brief Encode a third-party packet (DTI '}') with header + inner info.
 * @param info       Output buffer.
 * @param len        Buffer size.
 * @param header     Outer header/path text.
 * @param inner_info Encapsulated info field text.
 * @return Characters written; negative on error.
 */
int aprs_encode_third_party(char *info, size_t len, const char *header, const char *inner_info);

/**
 * @brief Decode a user-defined format payload.
 * @param info Input NUL-terminated text following '{'.
 * @param out  Output structure.
 * @return 0 on success; negative on error.
 */
int aprs_decode_user_defined(const char *info, aprs_user_defined_format_t *out);

/**
 * @brief Decode a third-party packet wrapper (extract header/inner info).
 * @param info Input NUL-terminated info field (starting after '}').
 * @param out  Output wrapper (strings copied).
 * @return 0 on success; negative on error.
 */
int aprs_decode_third_party(const char *info, aprs_third_party_packet_t *out);
/** @} */

#endif /* APRS_H_ */

