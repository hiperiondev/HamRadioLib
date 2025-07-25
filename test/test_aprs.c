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
#include <stdint.h>
#include <math.h>
#include <limits.h>

#include "common.h"
#include "test_common.h"
#include "utils.h"
#include "ax25.h"
#include "hdlc.h"
#include "aprs.h"

static uint32_t assert_count = 0;

int test_aprs_position_encoding_decoding() {
    printf("test_aprs_position_encoding_decoding\n");
    uint8_t err = 0;

    // Test 1: Position report (49.5N, -72.75W)
    {
        aprs_position_no_ts_t pos = { .latitude = 49.5, .longitude = -72.75, .symbol_table = '/', .symbol_code = '-', .comment = "Test" };
        char info[100];
        int len = aprs_encode_position_no_ts(info, 100, &pos);
        TEST_ASSERT(len == 24, "Position encoding length incorrect", err);
        TEST_ASSERT(strcmp(info, "!4930.00N/07245.00W-Test") == 0, "Encoded position incorrect", err);
        aprs_position_no_ts_t decoded;
        int ret = aprs_decode_position_no_ts(info, &decoded);
        TEST_ASSERT(ret == 0, "Position decoding failed", err);
        TEST_ASSERT(fabs(decoded.latitude - 49.5) < 0.001, "Decoded latitude incorrect", err);
        TEST_ASSERT(fabs(decoded.longitude + 72.75) < 0.001, "Decoded longitude incorrect", err);
        TEST_ASSERT(decoded.symbol_table == '/', "Symbol table incorrect", err);
        TEST_ASSERT(decoded.symbol_code == '-', "Symbol code incorrect", err);
        TEST_ASSERT(strcmp(decoded.comment, "Test") == 0, "Comment incorrect", err);
        free(decoded.comment);
    }

    // Test 2: Position with no comment
    {
        aprs_position_no_ts_t pos = { .latitude = -35.25, .longitude = 135.5, .symbol_table = '/', .symbol_code = '>', .comment = NULL };
        char info[100];
        int len = aprs_encode_position_no_ts(info, 100, &pos);
        TEST_ASSERT(len == 20, "Position encoding length incorrect", err);
        TEST_ASSERT(strcmp(info, "!3515.00S/13530.00E>") == 0, "Encoded position incorrect", err);
        aprs_position_no_ts_t decoded;
        int ret = aprs_decode_position_no_ts(info, &decoded);
        TEST_ASSERT(ret == 0, "Position decoding failed", err);
        TEST_ASSERT(fabs(decoded.latitude + 35.25) < 0.001, "Decoded latitude incorrect", err);
        TEST_ASSERT(fabs(decoded.longitude - 135.5) < 0.001, "Decoded longitude incorrect", err);
        TEST_ASSERT(decoded.comment != NULL && decoded.comment[0] == '\0', "Comment should be empty", err);
        free(decoded.comment);
    }

    // Test 3: Position with course 360
    {
        aprs_position_no_ts_t pos = { .latitude = 37.7749, .longitude = -122.4194, .symbol_table = '/', .symbol_code = '>', .comment = NULL, .dti = '!',
                .has_course_speed = true, .course = 360, .speed = 0 };
        char info[100];
        int len = aprs_encode_position_no_ts(info, 100, &pos);
        TEST_ASSERT(len == 27, "Position encoding length incorrect", err);
        TEST_ASSERT(strncmp(info, "!3746.49N/12225.16W>000/000", 27) == 0, "Encoded position with course 360 incorrect", err);
    }

    // Test 4: Position with negative course
    {
        aprs_position_no_ts_t pos = { .latitude = 37.7749, .longitude = -122.4194, .symbol_table = '/', .symbol_code = '>', .comment = NULL, .dti = '!',
                .has_course_speed = true, .course = -10, .speed = 0 };
        char info[100];
        int len = aprs_encode_position_no_ts(info, 100, &pos);
        TEST_ASSERT(len == 27, "Position encoding length incorrect", err);
        TEST_ASSERT(strncmp(info, "!3746.49N/12225.16W>350/000", 27) == 0, "Encoded position with negative course incorrect", err);
    }

    // Test 5: Decode position with invalid course
    {
        const char *info = "!3746.49N/12225.16W>999/000";
        aprs_position_no_ts_t pos;
        int ret = aprs_decode_position_no_ts(info, &pos);
        TEST_ASSERT(ret == -1, "Should fail to decode invalid course", err);
    }

    // Test 6: Decode position with invalid speed format
    {
        const char *info = "!3746.49N/12225.16W>180/-01";
        aprs_position_no_ts_t pos;
        int ret = aprs_decode_position_no_ts(info, &pos);
        TEST_ASSERT(ret == 0, "Should decode with invalid speed as comment", err);
        TEST_ASSERT(pos.has_course_speed == false, "has_course_speed should be false", err);
        TEST_ASSERT(strcmp(pos.comment, "180/-01") == 0, "Comment should include invalid extension", err);
        free(pos.comment);
    }

    // Test 7: Position with ambiguity level 3
    {
        aprs_position_no_ts_t pos = { .latitude = 49.5, .longitude = -72.75, .symbol_table = '/', .symbol_code = '-', .comment = "AMB3", .ambiguity = 3 };
        char info[100];
        int len = aprs_encode_position_no_ts(info, 100, &pos);
        TEST_ASSERT(len == 24, "Position encoding length with ambiguity incorrect", err);
        TEST_ASSERT(strcmp(info, "!493 .  N/0724 .  W-AMB3") == 0, "Encoded position with ambiguity incorrect", err);
        aprs_position_no_ts_t decoded;
        int ret = aprs_decode_position_no_ts(info, &decoded);
        TEST_ASSERT(ret == 0, "Position decoding with ambiguity failed", err);
        TEST_ASSERT(fabs(decoded.latitude - (49 + (35.0 / 60))) < 0.001, "Decoded latitude with ambiguity incorrect", err);
        TEST_ASSERT(fabs(decoded.longitude - (-72 - (45.0 / 60))) < 0.001, "Decoded longitude with ambiguity incorrect", err);
        TEST_ASSERT(decoded.symbol_table == '/', "Symbol table incorrect", err);
        TEST_ASSERT(decoded.symbol_code == '-', "Symbol code incorrect", err);
        TEST_ASSERT(strcmp(decoded.comment, "AMB3") == 0, "Ambiguity comment incorrect", err);
        free(decoded.comment);
    }

    // Test 8: Position with ambiguity level 4 and course/speed
    {
        aprs_position_no_ts_t pos = { .latitude = 37.7749, .longitude = -122.4194, .symbol_table = '/', .symbol_code = '>', .comment = "AMB4",
                .has_course_speed = true, .course = 180, .speed = 10, .ambiguity = 4 };
        char info[100];
        int len = aprs_encode_position_no_ts(info, 100, &pos);
        TEST_ASSERT(len == 31, "Position encoding length with ambiguity and course/speed incorrect", err);
        TEST_ASSERT(strcmp(info, "!37  .  N/122  .  W>180/010AMB4") == 0, "Encoded position with ambiguity and course/speed incorrect", err);
        aprs_position_no_ts_t decoded;
        int ret = aprs_decode_position_no_ts(info, &decoded);
        TEST_ASSERT(ret == 0, "Position decoding with ambiguity and course/speed failed", err);
        TEST_ASSERT(fabs(decoded.latitude - (37 + (30.0 / 60))) < 0.001, "Decoded latitude with ambiguity incorrect", err);
        TEST_ASSERT(fabs(decoded.longitude - (-122 - (30.0 / 60))) < 0.001, "Decoded longitude with ambiguity incorrect", err);
        TEST_ASSERT(decoded.has_course_speed == true, "has_course_speed should be true", err);
        TEST_ASSERT(decoded.course == 180, "Course mismatch", err);
        TEST_ASSERT(decoded.speed == 10, "Speed mismatch", err);
        TEST_ASSERT(strcmp(decoded.comment, "AMB4") == 0, "Ambiguity comment incorrect", err);
        free(decoded.comment);
    }

    return err;
}

int test_aprs_message_encoding_decoding() {
    printf("test_aprs_message_encoding_decoding\n");
    uint8_t err = 0;

    // Test 1: Message with number
    {
        aprs_message_t msg = { .addressee = "WB2OSZ-7", .message = "Hello", .message_number = "001" };
        char info[100];
        int len = aprs_encode_message(info, 100, &msg);
        TEST_ASSERT(len == 21, "Message encoding length incorrect", err);
        TEST_ASSERT(strcmp(info, ":WB2OSZ-7 :Hello{001}") == 0, "Encoded message incorrect", err);
        aprs_message_t decoded;
        int ret = aprs_decode_message(info, &decoded);
        TEST_ASSERT(ret == 0, "Message decoding failed", err);
        trim_trailing_spaces(decoded.addressee);
        TEST_ASSERT(strcmp(decoded.addressee, "WB2OSZ-7") == 0, "Decoded addressee incorrect", err);
        TEST_ASSERT(strcmp(decoded.message, "Hello") == 0, "Decoded message incorrect", err);
        TEST_ASSERT(strcmp(decoded.message_number, "001") == 0, "Decoded message number incorrect", err);
        free(decoded.message);
        free(decoded.message_number);
    }

    // Test 2: Message without number
    {
        aprs_message_t msg = { .addressee = "N2GH    ", .message = "Hi, Dave!", .message_number = NULL };
        char info[100];
        int len = aprs_encode_message(info, 100, &msg);
        TEST_ASSERT(len == 20, "Message encoding length incorrect", err);
        TEST_ASSERT(strcmp(info, ":N2GH     :Hi, Dave!") == 0, "Encoded message incorrect", err);
        aprs_message_t decoded;
        int ret = aprs_decode_message(info, &decoded);
        TEST_ASSERT(ret == 0, "Message decoding failed", err);
        trim_trailing_spaces(decoded.addressee);
        TEST_ASSERT(strcmp(decoded.addressee, "N2GH") == 0, "Decoded addressee incorrect", err);
        TEST_ASSERT(strcmp(decoded.message, "Hi, Dave!") == 0, "Decoded message incorrect", err);
        TEST_ASSERT(decoded.message_number == NULL, "Message number should be NULL", err);
        free(decoded.message);
    }

    return err;
}

int test_aprs_real_packets() {
    printf("test_aprs_real_packets\n");
    uint8_t err = 0;

    // Test 1: Real position report "!4903.50N/07201.75W-Test /A=001234"
    {
        const char *info = "!4903.50N/07201.75W-Test /A=001234";
        aprs_position_no_ts_t pos;
        int ret = aprs_decode_position_no_ts(info, &pos);
        TEST_ASSERT(ret == 0, "Real position decoding failed", err);
        TEST_ASSERT(fabs(pos.latitude - 49.058333) < 0.001, "Real position latitude incorrect", err);
        TEST_ASSERT(fabs(pos.longitude + 72.029167) < 0.001, "Real position longitude incorrect", err);
        TEST_ASSERT(pos.symbol_table == '/', "Real position symbol table incorrect", err);
        TEST_ASSERT(pos.symbol_code == '-', "Real position symbol code incorrect", err);
        TEST_ASSERT(strcmp(pos.comment, "Test /A=001234") == 0, "Real position comment incorrect", err);
        free(pos.comment);
    }

    // Test 2: Real message ":WB2OSZ-7 :Hello{001}"
    {
        const char *info = ":WB2OSZ-7 :Hello{001}";
        aprs_message_t msg;
        int ret = aprs_decode_message(info, &msg);
        TEST_ASSERT(ret == 0, "Real message decoding failed", err);
        trim_trailing_spaces(msg.addressee);
        TEST_ASSERT(strcmp(msg.addressee, "WB2OSZ-7") == 0, "Real message addressee incorrect", err);
        TEST_ASSERT(strcmp(msg.message, "Hello") == 0, "Real message text incorrect", err);
        TEST_ASSERT(strcmp(msg.message_number, "001") == 0, "Real message number incorrect", err);
        free(msg.message);
        free(msg.message_number);
    }

    return err;
}

int test_aprs_edge_cases() {
    printf("test_aprs_edge_cases\n");
    uint8_t err = 0;

    // Test 1: Invalid latitude
    {
        char *lat_str = lat_to_aprs(91.0, 0);
        TEST_ASSERT(lat_str == NULL, "Latitude > 90 should return NULL", err);
    }

    // Test 2: Invalid longitude
    {
        char *lon_str = lon_to_aprs(-181.0, 0);
        TEST_ASSERT(lon_str == NULL, "Longitude < -180 should return NULL", err);
    }

    // Test 3: Message with long addressee
    {
        aprs_message_t msg;
        memcpy(msg.addressee, "TOOLONGADD", 10); // 10 chars, no null terminator
        msg.message = "Test";
        msg.message_number = NULL;
        char info[100];
        int len = aprs_encode_message(info, 100, &msg);
        TEST_ASSERT(len == -1, "Encoding long addressee should fail", err);
    }

    return err;
}

int test_aprs_weather_object_position() {
    printf("test_aprs_weather_object_position\n");
    uint8_t err = 0;

    // Test: Weather report encoding and decoding
    {
        aprs_weather_report_t weather = { .timestamp = "12010000",
                .timestamp_format = "HMS",  // Explicitly set to HMS for 8-character timestamp
                .temperature = 25.0, .wind_speed = 10, .wind_direction = 180, .wind_gust = -1,
                .rainfall_last_hour = -1,  // Explicitly set to -1 to exclude from encoding
                .rainfall_24h = -1, .rainfall_since_midnight = -1, .barometric_pressure = -1, .humidity = -1, .luminosity = -1, .snowfall_24h = -999.9,
                .rain_rate = -1, .water_height_feet = -999.9, .water_height_meters = -999.9, .indoors_temperature = -999.9, .indoors_humidity = -1,
                .raw_rain_counter = -1 };
        char info[100];
        int len = aprs_encode_weather_report(info, 100, &weather);
        TEST_ASSERT(len == 21, "Weather report encoding length incorrect", err);
        TEST_ASSERT(strcmp(info, "_12010000c180s010t025") == 0, "Weather report encoding incorrect", err);
        aprs_weather_report_t decoded;
        int ret = aprs_decode_weather_report(info, &decoded);
        TEST_ASSERT(ret == 0, "Weather report decoding failed", err);
        TEST_ASSERT(fabs(decoded.temperature - 25.0) < 0.001, "Temperature mismatch", err);
        TEST_ASSERT(decoded.wind_speed == 10, "Wind speed mismatch", err);
        TEST_ASSERT(decoded.wind_direction == 180, "Wind direction mismatch", err);
        TEST_ASSERT(strcmp(decoded.timestamp, "12010000") == 0, "Timestamp mismatch", err);
    }

    // Test 2: Object report encoding and decoding
    {
        aprs_object_report_t obj = { .name = "TESTOBJ  ", .timestamp = "111111z", .latitude = 37.7749, .longitude = -122.4194, .symbol_table = '/',
                .symbol_code = '>' };
        char info[100];
        int len = aprs_encode_object_report(info, 100, &obj);
        TEST_ASSERT(len == 37, "Object report encoding length incorrect", err);
        TEST_ASSERT(strcmp(info, ";TESTOBJ  *111111z3746.49N/12225.16W>") == 0, "Object report encoding incorrect", err);
        aprs_object_report_t decoded;
        int ret = aprs_decode_object_report(info, &decoded);
        TEST_ASSERT(ret == 0, "Object report decoding failed", err);
        char trimmed_name[10];
        strncpy(trimmed_name, decoded.name, 9);
        trimmed_name[9] = '\0';
        trim_trailing_spaces(trimmed_name);
        TEST_ASSERT(strcmp(trimmed_name, "TESTOBJ") == 0, "Object name mismatch", err);
        TEST_ASSERT(fabs(decoded.latitude - 37.7749) < 0.001, "Object latitude mismatch", err);
        TEST_ASSERT(fabs(decoded.longitude + 122.4194) < 0.001, "Object longitude mismatch", err);
        TEST_ASSERT(decoded.symbol_table == '/', "Object symbol table mismatch", err);
        TEST_ASSERT(decoded.symbol_code == '>', "Object symbol code mismatch", err);
    }

    // Test 3: Timestamped position report encoding and decoding
    {
        aprs_position_with_ts_t pos = { .timestamp = "111111z", .latitude = 37.7749, .longitude = -122.4194, .symbol_table = '/', .symbol_code = '>', .comment =
                "Moving", .dti = '@' };
        char info[100];
        int len = aprs_encode_position_with_ts(info, 100, &pos);
        TEST_ASSERT(len == 33, "Timestamped position encoding length incorrect", err);
        TEST_ASSERT(strcmp(info, "@111111z3746.49N/12225.16W>Moving") == 0, "Timestamped position encoding incorrect", err);
        aprs_position_with_ts_t decoded;
        int ret = aprs_decode_position_with_ts(info, &decoded);
        TEST_ASSERT(ret == 0, "Timestamped position decoding failed", err);
        TEST_ASSERT(decoded.dti == '@', "DTI mismatch", err);
        TEST_ASSERT(strcmp(decoded.timestamp, "111111z") == 0, "Timestamp mismatch", err);
        TEST_ASSERT(fabs(decoded.latitude - 37.7749) < 0.001, "Latitude mismatch", err);
        TEST_ASSERT(fabs(decoded.longitude + 122.4194) < 0.001, "Longitude mismatch", err);
        TEST_ASSERT(decoded.symbol_table == '/', "Symbol table mismatch", err);
        TEST_ASSERT(decoded.symbol_code == '>', "Symbol code mismatch", err);
        TEST_ASSERT(strcmp(decoded.comment, "Moving") == 0, "Comment mismatch", err);
        free(decoded.comment);
    }

    // Test 4: Mic-E encoding and decoding
    {
        aprs_mice_t mice = { .latitude = 33.426667, // 33°25.60'N
                .longitude = -112.129, // 112°07.74'W
                .speed = 20, .course = 251, .symbol_table = '/', .symbol_code = '[', .message_code = "M3" // Returning
                };
        char dest_str[7];
        char info[100];
        int ret1 = aprs_encode_mice_destination(dest_str, &mice);
        int len = aprs_encode_mice_info(info, 100, &mice);
        TEST_ASSERT(ret1 == 0, "Mic-E destination encoding failed", err);
        TEST_ASSERT(len == 9, "Mic-E info encoding length incorrect", err);
        aprs_mice_t decoded;
        int message_bits;
        bool ns, long_offset, we;
        int ret2 = aprs_decode_mice_destination(dest_str, &decoded, &message_bits, &ns, &long_offset, &we);
        int ret3 = aprs_decode_mice_info(info, len, &decoded, long_offset, we);
        TEST_ASSERT(ret2 == 0, "Mic-E destination decoding failed", err);
        TEST_ASSERT(ret3 == 0, "Mic-E info decoding failed", err);
        TEST_ASSERT(fabs(decoded.latitude - 33.426667) < 0.001, "Mic-E latitude mismatch", err);
        TEST_ASSERT(fabs(decoded.longitude + 112.129) < 0.001, "Mic-E longitude mismatch", err);
        TEST_ASSERT(decoded.speed == 20, "Mic-E speed mismatch", err);
        TEST_ASSERT(decoded.course == 251, "Mic-E course mismatch", err);
        TEST_ASSERT(decoded.symbol_table == '/', "Mic-E symbol table mismatch", err);
        TEST_ASSERT(decoded.symbol_code == '[', "Mic-E symbol code mismatch", err);
        const char *standard_codes[8] = { "Emergency", "M6", "M5", "M4", "M3", "M2", "M1", "M0" };
        strcpy(decoded.message_code, standard_codes[message_bits]);
        TEST_ASSERT(strcmp(decoded.message_code, "M3") == 0, "Mic-E message code mismatch", err);
    }

    // Test 5: Telemetry encoding and decoding
    {
        aprs_telemetry_t telem = { .sequence_number = 123, .analog = { 100.0, 200.0, 150.0, 50.0, 255.0 }, .digital = 0xA5 // 10100101
                };
        char info[100];
        int len = aprs_encode_telemetry(info, 100, &telem);
        TEST_ASSERT(len == 34, "Telemetry encoding length incorrect", err);
        TEST_ASSERT(strcmp(info, "T#123,100,200,150,050,255,10100101") == 0, "Telemetry encoding incorrect", err);
        aprs_telemetry_t decoded;
        int ret = aprs_decode_telemetry(info, &decoded);
        TEST_ASSERT(ret == 0, "Telemetry decoding failed", err);
        TEST_ASSERT(decoded.sequence_number == 123, "Telemetry sequence number mismatch", err);
        for (int i = 0; i < 5; i++) {
            char msg[50];
            snprintf(msg, 50, "Telemetry analog %d mismatch", i);
            TEST_ASSERT(fabs(decoded.analog[i] - telem.analog[i]) < 0.001, msg, err);
        }
        TEST_ASSERT(decoded.digital == 0xA5, "Telemetry digital bits mismatch", err);
    }

    return err;
}

int test_aprs_position_with_ts() {
    printf("test_aprs_position_with_ts\n");
    uint8_t err = 0;
    const char *info = "@092345z4903.50N/07201.75W-Test";
    aprs_position_with_ts_t pos;
    int ret = aprs_decode_position_with_ts(info, &pos);
    TEST_ASSERT(ret == 0, "Failed to decode position with timestamp", err);
    TEST_ASSERT(pos.dti == '@', "DTI mismatch", err);
    TEST_ASSERT(strcmp(pos.timestamp, "092345z") == 0, "Timestamp mismatch", err);
    double expected_lat = 49.0 + 3.50 / 60.0;
    double expected_lon = -(72.0 + 1.75 / 60.0);
    TEST_ASSERT(fabs(pos.latitude - expected_lat) < 0.0001, "Latitude mismatch", err);
    TEST_ASSERT(fabs(pos.longitude - expected_lon) < 0.0001, "Longitude mismatch", err);
    TEST_ASSERT(pos.symbol_table == '/', "Symbol table mismatch", err);
    TEST_ASSERT(pos.symbol_code == '-', "Symbol code mismatch", err);
    TEST_ASSERT(strcmp(pos.comment, "Test") == 0, "Comment mismatch", err);
    free(pos.comment);
    return err;
}

int test_aprs_weather() {
    printf("test_aprs_weather\n");
    uint8_t err = 0;
    const char *info = "_10090556c220s004g005t077r000p000P000h50b09900wRSW";
    aprs_weather_report_t weather;
    int ret = aprs_decode_weather_report(info, &weather);
    TEST_ASSERT(ret == 0, "Failed to decode weather report", err);
    TEST_ASSERT(strcmp(weather.timestamp, "10090556") == 0, "Timestamp mismatch", err);
    TEST_ASSERT(weather.wind_direction == 220, "Wind direction mismatch", err);
    TEST_ASSERT(weather.wind_speed == 4, "Wind speed mismatch", err);
    TEST_ASSERT(fabs(weather.temperature - 77.0) < 0.1, "Temperature mismatch", err);
    return err;
}

int test_aprs_object() {
    printf("test_aprs_object\n");
    uint8_t err = 0;
    const char *info = ";LEADER   *092345z4903.50N/07201.75W>";
    aprs_object_report_t obj;
    int ret = aprs_decode_object_report(info, &obj);
    TEST_ASSERT(ret == 0, "Failed to decode object report", err);
    TEST_ASSERT(strcmp(obj.name, "LEADER") == 0, "Object name mismatch", err);
    TEST_ASSERT(strcmp(obj.timestamp, "092345z") == 0, "Timestamp mismatch", err);
    double expected_lat = 49.0 + 3.50 / 60.0;
    double expected_lon = -(72.0 + 1.75 / 60.0);
    TEST_ASSERT(fabs(obj.latitude - expected_lat) < 0.0001, "Latitude mismatch", err);
    TEST_ASSERT(fabs(obj.longitude - expected_lon) < 0.0001, "Longitude mismatch", err);
    TEST_ASSERT(obj.symbol_table == '/', "Symbol table mismatch", err);
    TEST_ASSERT(obj.symbol_code == '>', "Symbol code mismatch", err);
    return err;
}

int test_aprs_mice() {
    printf("test_aprs_mice\n");
    uint8_t err = 0;
    const char *dest_str = "SUSURB";
    const unsigned char info[] = { 0x60, 0x43, 0x46, 0x22, 0x1C, 0x1F, 0x21, 0x5B, 0x2F, 0x3A, 0x60, 0x22, 0x33, 0x7A, 0x7D, 0x5F, 0x20, 0x00 };
    aprs_mice_t mice;
    int message_bits;
    bool ns, long_offset, we;
    int ret = aprs_decode_mice_destination(dest_str, &mice, &message_bits, &ns, &long_offset, &we);
    TEST_ASSERT(ret == 0, "Failed to decode Mic-E destination", err);
    ret = aprs_decode_mice_info((const char*) info, sizeof(info) - 1, &mice, long_offset, we);
    TEST_ASSERT(ret == 0, "Failed to decode Mic-E info", err);

    // Set message_code based on message_bits and info type
    const char *standard_codes[8] = { "Emergency", "M6", "M5", "M4", "M3", "M2", "M1", "M0" };
    const char *custom_codes[8] = { "Emergency", "C6", "C5", "C4", "C3", "C2", "C1", "C0" };
    bool is_standard = (info[0] == '`');
    strcpy(mice.message_code, is_standard ? standard_codes[message_bits] : custom_codes[message_bits]);

    TEST_ASSERT(fabs(mice.latitude - 35.586833) < 0.0001, "Latitude mismatch", err);
    TEST_ASSERT(fabs(mice.longitude - 139.701) < 0.0001, "Longitude mismatch", err);
    TEST_ASSERT(mice.course == 305, "Course mismatch", err);
    TEST_ASSERT(mice.speed == 0, "Speed mismatch", err);
    TEST_ASSERT(mice.symbol_table == '/', "Symbol table mismatch", err);
    TEST_ASSERT(mice.symbol_code == '[', "Symbol code mismatch", err);
    TEST_ASSERT(strcmp(mice.message_code, "M0") == 0, "Message code mismatch", err);
    return err;
}

int test_aprs_telemetry() {
    printf("test_aprs_telemetry\n");
    uint8_t err = 0;
    const char *info = "T#001,123,045,067,089,100,00000000";
    aprs_telemetry_t telemetry;
    int ret = aprs_decode_telemetry(info, &telemetry);
    TEST_ASSERT(ret == 0, "Failed to decode telemetry", err);
    TEST_ASSERT(telemetry.sequence_number == 1, "Sequence number mismatch", err);
    TEST_ASSERT(fabs(telemetry.analog[0] - 123) < 0.1, "Analog 0 mismatch", err);
    TEST_ASSERT(fabs(telemetry.analog[1] - 45) < 0.1, "Analog 1 mismatch", err);
    TEST_ASSERT(fabs(telemetry.analog[2] - 67) < 0.1, "Analog 2 mismatch", err);
    TEST_ASSERT(fabs(telemetry.analog[3] - 89) < 0.1, "Analog 3 mismatch", err);
    TEST_ASSERT(fabs(telemetry.analog[4] - 100) < 0.1, "Analog 4 mismatch", err);
    TEST_ASSERT(telemetry.digital == 0, "Digital bits mismatch", err);
    return 0;
}

int test_aprs_status() {
    printf("test_aprs_status\n");
    uint8_t err = 0;
    char info[100] = { 0 };  // Zero-initialized
    aprs_status_t status = { .has_timestamp = false, .status_text = "Test status" };
    int len = aprs_encode_status(info, sizeof(info), &status);
    TEST_ASSERT(len == 12, "Status encoding length incorrect", err);
    TEST_ASSERT(strncmp(info, ">Test status", len) == 0, "Encoded status incorrect", err);
    aprs_status_t decoded;
    int ret = aprs_decode_status(info, &decoded);
    TEST_ASSERT(ret == 0, "Status decoding failed", err);
    TEST_ASSERT(decoded.has_timestamp == false, "Decoded has_timestamp incorrect", err);
    TEST_ASSERT(strcmp(decoded.status_text, "Test status") == 0, "Decoded status text incorrect", err);
    // With timestamp
    status.has_timestamp = true;
    strcpy(status.timestamp, "092345z");
    len = aprs_encode_status(info, sizeof(info), &status);
    TEST_ASSERT(len == 19, "Status with timestamp encoding length incorrect", err);
    TEST_ASSERT(strncmp(info, ">092345zTest status", len) == 0, "Encoded status with timestamp incorrect", err);
    ret = aprs_decode_status(info, &decoded);
    TEST_ASSERT(ret == 0, "Status with timestamp decoding failed", err);
    TEST_ASSERT(decoded.has_timestamp == true, "Decoded has_timestamp incorrect", err);
    TEST_ASSERT(strcmp(decoded.timestamp, "092345z") == 0, "Decoded timestamp incorrect", err);
    TEST_ASSERT(strcmp(decoded.status_text, "Test status") == 0, "Decoded status text incorrect", err);
    return err;
}

int test_aprs_general_query() {
    printf("test_aprs_general_query\n");
    uint8_t err = 0;
    char info[100] = { 0 };  // Zero-initialized
    aprs_general_query_t query = { .query_type = "APRS" };
    int len = aprs_encode_general_query(info, sizeof(info), &query);
    TEST_ASSERT(len == 6, "General query encoding length incorrect", err);
    TEST_ASSERT(strncmp(info, "?APRS?", len) == 0, "Encoded general query incorrect", err);
    aprs_general_query_t decoded;
    int ret = aprs_decode_general_query(info, &decoded);
    TEST_ASSERT(ret == 0, "General query decoding failed", err);
    TEST_ASSERT(strcmp(decoded.query_type, "APRS") == 0, "Decoded query type incorrect", err);
    // Another query type
    strcpy(query.query_type, "WX");
    len = aprs_encode_general_query(info, sizeof(info), &query);
    TEST_ASSERT(len == 4, "General query encoding length incorrect", err);
    TEST_ASSERT(strncmp(info, "?WX?", len) == 0, "Encoded general query incorrect", err);
    ret = aprs_decode_general_query(info, &decoded);
    TEST_ASSERT(ret == 0, "General query decoding failed", err);
    TEST_ASSERT(strcmp(decoded.query_type, "WX") == 0, "Decoded query type incorrect", err);
    return err;
}

int test_aprs_station_capabilities() {
    printf("test_aprs_station_capabilities\n");
    uint8_t err = 0;
    char info[100] = { 0 };  // Zero-initialized
    aprs_station_capabilities_t cap = { .capabilities_text = "IGATE,MSG_CNT=43,LOC_CNT=14" };
    int len = aprs_encode_station_capabilities(info, sizeof(info), &cap);
    TEST_ASSERT(len == 28, "Station capabilities encoding length incorrect", err);
    TEST_ASSERT(strncmp(info, "<IGATE,MSG_CNT=43,LOC_CNT=14", len) == 0, "Encoded station capabilities incorrect", err);
    aprs_station_capabilities_t decoded;
    int ret = aprs_decode_station_capabilities(info, &decoded);
    TEST_ASSERT(ret == 0, "Station capabilities decoding failed", err);
    TEST_ASSERT(strcmp(decoded.capabilities_text, "IGATE,MSG_CNT=43,LOC_CNT=14") == 0, "Decoded capabilities text incorrect", err);
    return err;
}

int test_aprs_packets() {
    printf("test_aprs_packets\n");
    uint8_t err = 0;

    // Test 1: Position Report without Timestamp
    {
        aprs_position_no_ts_t original = { .latitude = 37.7749, .longitude = -122.4194, .symbol_table = '/', .symbol_code = '>', .comment = "San Francisco",
                .dti = '!', .has_course_speed = true, .course = 180, .speed = 10 };

        char info[100];
        int len = aprs_encode_position_no_ts(info, 100, &original);
        TEST_ASSERT(len > 0, "Failed to encode position no ts", err);

        aprs_position_no_ts_t decoded;
        int ret = aprs_decode_position_no_ts(info, &decoded);
        TEST_ASSERT(ret == 0, "Failed to decode position no ts", err);

        TEST_ASSERT(fabs(decoded.latitude - original.latitude) < 0.0001, "Latitude mismatch", err);
        TEST_ASSERT(fabs(decoded.longitude - original.longitude) < 0.0001, "Longitude mismatch", err);
        TEST_ASSERT(decoded.symbol_table == original.symbol_table, "Symbol table mismatch", err);
        TEST_ASSERT(decoded.symbol_code == original.symbol_code, "Symbol code mismatch", err);
        TEST_ASSERT(strcmp(decoded.comment, original.comment) == 0, "Comment mismatch", err);
        TEST_ASSERT(decoded.dti == original.dti, "DTI mismatch", err);
        TEST_ASSERT(decoded.has_course_speed == original.has_course_speed, "has_course_speed mismatch", err);
        TEST_ASSERT(decoded.course == original.course, "Course mismatch", err);
        TEST_ASSERT(decoded.speed == original.speed, "Speed mismatch", err);

        aprs_frame_print((unsigned char*) info, len);
        free(decoded.comment);
    }

    // Test 2: Position Report with Timestamp
    {
        aprs_position_with_ts_t original = { .dti = '@', .timestamp = "111111z", .latitude = 37.7749, .longitude = -122.4194, .symbol_table = '/',
                .symbol_code = '>', .comment = "Moving" };

        char info[100];
        int len = aprs_encode_position_with_ts(info, 100, &original);
        TEST_ASSERT(len > 0, "Failed to encode position with ts", err);

        aprs_position_with_ts_t decoded;
        int ret = aprs_decode_position_with_ts(info, &decoded);
        TEST_ASSERT(ret == 0, "Failed to decode position with ts", err);

        TEST_ASSERT(decoded.dti == original.dti, "DTI mismatch", err);
        TEST_ASSERT(strcmp(decoded.timestamp, original.timestamp) == 0, "Timestamp mismatch", err);
        TEST_ASSERT(fabs(decoded.latitude - original.latitude) < 0.0001, "Latitude mismatch", err);
        TEST_ASSERT(fabs(decoded.longitude - original.longitude) < 0.0001, "Longitude mismatch", err);
        TEST_ASSERT(decoded.symbol_table == original.symbol_table, "Symbol table mismatch", err);
        TEST_ASSERT(decoded.symbol_code == original.symbol_code, "Symbol code mismatch", err);
        TEST_ASSERT(strcmp(decoded.comment, original.comment) == 0, "Comment mismatch", err);

        aprs_frame_print((unsigned char*) info, len);
        free(decoded.comment);
    }

    // Test 3: Message
    {
        aprs_message_t original = { .addressee = "WB2OSZ-7", .message = "Hello", .message_number = "001" };

        char info[100];
        int len = aprs_encode_message(info, 100, &original);
        TEST_ASSERT(len > 0, "Failed to encode message", err);

        aprs_message_t decoded;
        int ret = aprs_decode_message(info, &decoded);
        TEST_ASSERT(ret == 0, "Failed to decode message", err);

        trim_trailing_spaces(decoded.addressee);
        TEST_ASSERT(strcmp(decoded.addressee, original.addressee) == 0, "Addressee mismatch", err);
        TEST_ASSERT(strcmp(decoded.message, original.message) == 0, "Message mismatch", err);
        TEST_ASSERT(strcmp(decoded.message_number, original.message_number) == 0, "Message number mismatch", err);

        aprs_frame_print((unsigned char*) info, len);
        free(decoded.message);
        free(decoded.message_number);
    }

    // Test 4: Weather Report
    {
        aprs_weather_report_t original = { .timestamp = "12010000", .temperature = 25.0, .wind_speed = 10, .wind_direction = 180 };

        char info[100];
        int len = aprs_encode_weather_report(info, 100, &original);
        TEST_ASSERT(len > 0, "Failed to encode weather report", err);

        aprs_weather_report_t decoded;
        int ret = aprs_decode_weather_report(info, &decoded);
        TEST_ASSERT(ret == 0, "Failed to decode weather report", err);

        TEST_ASSERT(strcmp(decoded.timestamp, original.timestamp) == 0, "Timestamp mismatch", err);
        TEST_ASSERT(fabs(decoded.temperature - original.temperature) < 0.001, "Temperature mismatch", err);
        TEST_ASSERT(decoded.wind_speed == original.wind_speed, "Wind speed mismatch", err);
        TEST_ASSERT(decoded.wind_direction == original.wind_direction, "Wind direction mismatch", err);

        aprs_frame_print((unsigned char*) info, len);
    }

    // Test 5: Object Report
    {
        aprs_object_report_t original = { .name = "TESTOBJ  ", .timestamp = "111111z", .latitude = 37.7749, .longitude = -122.4194, .symbol_table = '/',
                .symbol_code = '>' };

        char info[100];
        int len = aprs_encode_object_report(info, 100, &original);
        TEST_ASSERT(len > 0, "Failed to encode object report", err);

        aprs_object_report_t decoded;
        int ret = aprs_decode_object_report(info, &decoded);
        TEST_ASSERT(ret == 0, "Failed to decode object report", err);

        char trimmed_name[10];
        strncpy(trimmed_name, decoded.name, 9);
        trimmed_name[9] = '\0';
        trim_trailing_spaces(trimmed_name);
        TEST_ASSERT(strcmp(trimmed_name, "TESTOBJ") == 0, "Object name mismatch", err);
        TEST_ASSERT(strcmp(decoded.timestamp, original.timestamp) == 0, "Timestamp mismatch", err);
        TEST_ASSERT(fabs(decoded.latitude - original.latitude) < 0.0001, "Latitude mismatch", err);
        TEST_ASSERT(fabs(decoded.longitude - original.longitude) < 0.0001, "Longitude mismatch", err);
        TEST_ASSERT(decoded.symbol_table == original.symbol_table, "Symbol table mismatch", err);
        TEST_ASSERT(decoded.symbol_code == original.symbol_code, "Symbol code mismatch", err);

        aprs_frame_print((unsigned char*) info, len);
    }

    // Test 6: Telemetry Report
    {
        aprs_telemetry_t original = { .sequence_number = 123, .analog = { 100.0, 200.0, 150.0, 50.0, 255.0 }, .digital = 0xA5 };

        char info[100];
        int len = aprs_encode_telemetry(info, 100, &original);
        TEST_ASSERT(len > 0, "Failed to encode telemetry", err);

        aprs_telemetry_t decoded;
        int ret = aprs_decode_telemetry(info, &decoded);
        TEST_ASSERT(ret == 0, "Failed to decode telemetry", err);

        TEST_ASSERT(decoded.sequence_number == original.sequence_number, "Sequence number mismatch", err);
        for (int i = 0; i < 5; i++) {
            char msg[50];
            snprintf(msg, 50, "Analog %d mismatch", i);
            TEST_ASSERT(fabs(decoded.analog[i] - original.analog[i]) < 0.001, msg, err);
        }
        TEST_ASSERT(decoded.digital == original.digital, "Digital bits mismatch", err);

        aprs_frame_print((unsigned char*) info, len);
    }

    // Test 7: Status Report
    {
        aprs_status_t original = { .has_timestamp = true, .timestamp = "092345z", .status_text = "Test status" };

        char info[100];
        int len = aprs_encode_status(info, 100, &original);
        TEST_ASSERT(len > 0, "Failed to encode status", err);

        aprs_status_t decoded;
        int ret = aprs_decode_status(info, &decoded);
        TEST_ASSERT(ret == 0, "Failed to decode status", err);

        TEST_ASSERT(decoded.has_timestamp == original.has_timestamp, "has_timestamp mismatch", err);
        TEST_ASSERT(strcmp(decoded.timestamp, original.timestamp) == 0, "Timestamp mismatch", err);
        TEST_ASSERT(strcmp(decoded.status_text, original.status_text) == 0, "Status text mismatch", err);

        aprs_frame_print((unsigned char*) info, len);
    }

    // Test 8: General Query
    {
        aprs_general_query_t original = { .query_type = "APRS" };

        char info[100];
        int len = aprs_encode_general_query(info, 100, &original);
        TEST_ASSERT(len > 0, "Failed to encode general query", err);

        aprs_general_query_t decoded;
        int ret = aprs_decode_general_query(info, &decoded);
        TEST_ASSERT(ret == 0, "Failed to decode general query", err);

        TEST_ASSERT(strcmp(decoded.query_type, original.query_type) == 0, "Query type mismatch", err);

        aprs_frame_print((unsigned char*) info, len);
    }

    // Test 9: Station Capabilities
    {
        aprs_station_capabilities_t original = { .capabilities_text = "IGATE,MSG_CNT=43,LOC_CNT=14" };

        char info[100];
        int len = aprs_encode_station_capabilities(info, 100, &original);
        TEST_ASSERT(len > 0, "Failed to encode station capabilities", err);

        aprs_station_capabilities_t decoded;
        int ret = aprs_decode_station_capabilities(info, &decoded);
        TEST_ASSERT(ret == 0, "Failed to decode station capabilities", err);

        TEST_ASSERT(strcmp(decoded.capabilities_text, original.capabilities_text) == 0, "Capabilities text mismatch", err);

        aprs_frame_print((unsigned char*) info, len);
    }

    // Test 10: Mic-E Compressed Position Report
    {
        aprs_mice_t original = { .latitude = 33.426667, .longitude = -112.129, .speed = 20, .course = 251, .symbol_table = '/', .symbol_code = '[',
                .message_code = "M3" };

        char dest_str[7];
        char info[100];
        int ret1 = aprs_encode_mice_destination(dest_str, &original);
        int len = aprs_encode_mice_info(info, 100, &original);
        TEST_ASSERT(ret1 == 0, "Failed to encode Mic-E destination", err);
        TEST_ASSERT(len > 0, "Failed to encode Mic-E info", err);

        aprs_mice_t decoded;
        int message_bits;
        bool ns, long_offset, we;
        int ret2 = aprs_decode_mice_destination(dest_str, &decoded, &message_bits, &ns, &long_offset, &we);
        int ret3 = aprs_decode_mice_info(info, len, &decoded, long_offset, we);
        TEST_ASSERT(ret2 == 0, "Failed to decode Mic-E destination", err);
        TEST_ASSERT(ret3 == 0, "Failed to decode Mic-E info", err);

        TEST_ASSERT(fabs(decoded.latitude - original.latitude) < 0.001, "Latitude mismatch", err);
        TEST_ASSERT(fabs(decoded.longitude - original.longitude) < 0.001, "Longitude mismatch", err);
        TEST_ASSERT(decoded.speed == original.speed, "Speed mismatch", err);
        TEST_ASSERT(decoded.course == original.course, "Course mismatch", err);
        TEST_ASSERT(decoded.symbol_table == original.symbol_table, "Symbol table mismatch", err);
        TEST_ASSERT(decoded.symbol_code == original.symbol_code, "Symbol code mismatch", err);
        const char *standard_codes[8] = { "Emergency", "M6", "M5", "M4", "M3", "M2", "M1", "M0" };
        strcpy(decoded.message_code, standard_codes[message_bits]);
        TEST_ASSERT(strcmp(decoded.message_code, original.message_code) == 0, "Message code mismatch", err);

        aprs_frame_print((unsigned char*) info, len);
    }

    return err;
}

int test_aprs_bulletin() {
    printf("test_aprs_bulletin\n");
    uint8_t err = 0;

    // Test 1: Bulletin with no message number
    {
        aprs_bulletin_t bulletin = { .bulletin_id = "BLN1", .message = "Test bulletin", .message_number = NULL };
        char info[100];
        int len = aprs_encode_bulletin(info, 100, &bulletin);
        TEST_ASSERT(len == 24, "Bulletin encoding length incorrect", err);
        TEST_ASSERT(strcmp(info, ":BLN1     :Test bulletin") == 0, "Encoded bulletin incorrect", err);
        aprs_message_t decoded;
        int ret = aprs_decode_message(info, &decoded);
        TEST_ASSERT(ret == 0, "Bulletin decoding failed", err);
        TEST_ASSERT(aprs_is_bulletin(&decoded), "Decoded message should be a bulletin", err);
        trim_trailing_spaces(decoded.addressee);
        TEST_ASSERT(strcmp(decoded.addressee, "BLN1") == 0, "Decoded addressee incorrect", err);
        TEST_ASSERT(strcmp(decoded.message, "Test bulletin") == 0, "Decoded message incorrect", err);
        TEST_ASSERT(decoded.message_number == NULL, "Message number should be NULL", err);
        free(decoded.message);
    }

    // Test 2: Bulletin with message number
    {
        aprs_bulletin_t bulletin = { .bulletin_id = "BLN2", .message = "Emergency alert", .message_number = "123" };
        char info[100];
        int len = aprs_encode_bulletin(info, 100, &bulletin);
        TEST_ASSERT(len == 31, "Bulletin encoding length incorrect", err);
        TEST_ASSERT(strcmp(info, ":BLN2     :Emergency alert{123}") == 0, "Encoded bulletin incorrect", err);
        aprs_message_t decoded;
        int ret = aprs_decode_message(info, &decoded);
        TEST_ASSERT(ret == 0, "Bulletin decoding failed", err);
        TEST_ASSERT(aprs_is_bulletin(&decoded), "Decoded message should be a bulletin", err);
        trim_trailing_spaces(decoded.addressee);
        TEST_ASSERT(strcmp(decoded.addressee, "BLN2") == 0, "Decoded addressee incorrect", err);
        TEST_ASSERT(strcmp(decoded.message, "Emergency alert") == 0, "Decoded message incorrect", err);
        TEST_ASSERT(strcmp(decoded.message_number, "123") == 0, "Decoded message number incorrect", err);
        free(decoded.message);
        free(decoded.message_number);
    }

    return err;
}

int test_aprs_item_report() {
    printf("test_aprs_item_report\n");
    uint8_t err = 0;

    // Test 1: Live item report with comment
    {
        aprs_item_report_t item = { .name = "ITEM1", .is_live = true, .latitude = 37.7749, .longitude = -122.4194, .symbol_table = '/', .symbol_code = '>',
                .comment = "Test item" };
        char info[100];
        int len = aprs_encode_item_report(info, 100, &item);
        TEST_ASSERT(len == 39, "Item report encoding length incorrect", err);
        TEST_ASSERT(strcmp(info, ")ITEM1    !3746.49N/12225.16W>Test item") == 0, "Encoded item report incorrect", err);
        aprs_item_report_t decoded;
        int ret = aprs_decode_item_report(info, &decoded);
        TEST_ASSERT(ret == 0, "Item report decoding failed", err);
        TEST_ASSERT(strcmp(decoded.name, "ITEM1") == 0, "Decoded item name incorrect", err);
        TEST_ASSERT(decoded.is_live == true, "Decoded live status incorrect", err);
        TEST_ASSERT(fabs(decoded.latitude - 37.7749) < 0.001, "Decoded latitude incorrect", err);
        TEST_ASSERT(fabs(decoded.longitude + 122.4194) < 0.001, "Decoded longitude incorrect", err);
        TEST_ASSERT(decoded.symbol_table == '/', "Decoded symbol table incorrect", err);
        TEST_ASSERT(decoded.symbol_code == '>', "Decoded symbol code incorrect", err);
        TEST_ASSERT(strcmp(decoded.comment, "Test item") == 0, "Decoded comment incorrect", err);
        free(decoded.comment);
    }

    // Test 2: Killed item report without comment
    {
        aprs_item_report_t item = { .name = "ITEM2", .is_live = false, .latitude = 37.7749, .longitude = -122.4194, .symbol_table = '/', .symbol_code = '>',
                .comment = NULL };
        char info[100];
        int len = aprs_encode_item_report(info, 100, &item);
        TEST_ASSERT(len == 30, "Killed item report encoding length incorrect", err);
        TEST_ASSERT(strcmp(info, ")ITEM2    =3746.49N/12225.16W>") == 0, "Encoded killed item report incorrect", err);
        aprs_item_report_t decoded;
        int ret = aprs_decode_item_report(info, &decoded);
        TEST_ASSERT(ret == 0, "Killed item report decoding failed", err);
        TEST_ASSERT(strcmp(decoded.name, "ITEM2") == 0, "Decoded item name incorrect", err);
        TEST_ASSERT(decoded.is_live == false, "Decoded live status incorrect", err);
        TEST_ASSERT(fabs(decoded.latitude - 37.7749) < 0.001, "Decoded latitude incorrect", err);
        TEST_ASSERT(fabs(decoded.longitude + 122.4194) < 0.001, "Decoded longitude incorrect", err);
        TEST_ASSERT(decoded.symbol_table == '/', "Decoded symbol table incorrect", err);
        TEST_ASSERT(decoded.symbol_code == '>', "Decoded symbol code incorrect", err);
        TEST_ASSERT(decoded.comment != NULL && decoded.comment[0] == '\0', "Comment should be empty", err);
        free(decoded.comment);
    }

    return err;
}

uint8_t test_other(void) {
    printf("test_other\n");
    uint8_t err = 0;

    // Test for raw GPS
    {
        const char *raw_gps_str = "GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47";
        aprs_raw_gps_t data = { .raw_data = (char*) raw_gps_str, .data_len = strlen(raw_gps_str) };
        char info[256];
        int len = aprs_encode_raw_gps(info, sizeof(info), &data);
        TEST_ASSERT(len == 1 + data.data_len, "Raw GPS encoding length incorrect", err);
        char expected[256];
        snprintf(expected, sizeof(expected), "$%s", raw_gps_str);
        COMPARE_FRAME(info, (size_t )len, expected, (size_t )strlen(expected), "Raw GPS encoding");

        aprs_raw_gps_t decoded;
        int ret = aprs_decode_raw_gps(info, &decoded);
        TEST_ASSERT(ret == 0, "Raw GPS decoding failed", err);
        TEST_ASSERT(strcmp(decoded.raw_data, data.raw_data) == 0, "Raw GPS data mismatch", err);
        TEST_ASSERT(decoded.data_len == data.data_len, "Raw GPS length mismatch", err);
        free(decoded.raw_data);
    }

    // Test for grid square
    {
        aprs_grid_square_t data = { .grid_square = "JJ00", .comment = "Test location" };
        char info[256];
        int len = aprs_encode_grid_square(info, sizeof(info), &data);
        char expected[256];
        snprintf(expected, sizeof(expected), "[%s %s", data.grid_square, data.comment); // Changed from ">" to "["
        TEST_ASSERT(len == strlen(expected), "Grid square encoding length incorrect", err);
        COMPARE_FRAME(info, (size_t )len, expected, (size_t )strlen(expected), "Grid square encoding");

        aprs_grid_square_t decoded;
        int ret = aprs_decode_grid_square(info, &decoded);
        TEST_ASSERT(ret == 0, "Grid square decoding failed", err);
        TEST_ASSERT(strcmp(decoded.grid_square, data.grid_square) == 0, "Grid square mismatch", err);
        TEST_ASSERT((data.comment && decoded.comment && strcmp(decoded.comment, data.comment) == 0) || (!data.comment && !decoded.comment),
                "Grid square comment mismatch", err);
        free(decoded.comment);
    }

    // Test for DF report
    {
        aprs_df_report_t data = { .bearing = 270, .signal_strength = 5, .comment = "Strong signal" };
        char info[256];
        int len = aprs_encode_df_report(info, sizeof(info), &data);
        char expected[256];
        snprintf(expected, sizeof(expected), "+%03d/%d%s", data.bearing, data.signal_strength, data.comment ? data.comment : "");
        TEST_ASSERT(len == strlen(expected), "DF report encoding length incorrect", err);
        COMPARE_FRAME(info, (size_t )len, expected, (size_t )strlen(expected), "DF report encoding");

        aprs_df_report_t decoded;
        int ret = aprs_decode_df_report(info, &decoded);
        TEST_ASSERT(ret == 0, "DF report decoding failed", err);
        TEST_ASSERT(decoded.bearing == data.bearing, "DF bearing mismatch", err);
        TEST_ASSERT(decoded.signal_strength == data.signal_strength, "DF signal strength mismatch", err);
        TEST_ASSERT((data.comment && decoded.comment && strcmp(decoded.comment, data.comment) == 0) || (!data.comment && !decoded.comment),
                "DF comment mismatch", err);
        free(decoded.comment);
    }

    // Test for test packet
    {
        const char *test_data = "TEST123";
        aprs_test_packet_t data = { .data = (char*) test_data, .data_len = strlen(test_data) };
        char info[256];
        int len = aprs_encode_test_packet(info, sizeof(info), &data);
        TEST_ASSERT(len == 1 + data.data_len, "Test packet encoding length incorrect", err);
        char expected[256];
        snprintf(expected, sizeof(expected), ",%s", test_data);
        COMPARE_FRAME(info, (size_t )len, expected, (size_t )strlen(expected), "Test packet encoding");

        aprs_test_packet_t decoded;
        int ret = aprs_decode_test_packet(info, &decoded);
        TEST_ASSERT(ret == 0, "Test packet decoding failed", err);
        TEST_ASSERT(strcmp(decoded.data, data.data) == 0, "Test packet data mismatch", err);
        TEST_ASSERT(decoded.data_len == data.data_len, "Test packet length mismatch", err);
        free(decoded.data);
    }

    // Error case for raw GPS: invalid length
    {
        aprs_raw_gps_t data = { .raw_data = "GP", .data_len = 2 };
        char info[256];
        int len = aprs_encode_raw_gps(info, sizeof(info), &data);
        TEST_ASSERT(len == -1, "Should fail to encode invalid raw GPS", err);
    }

    // Error case for grid square: invalid length
    {
        aprs_grid_square_t data = { .grid_square = "ABC", .comment = NULL };
        char info[256];
        int len = aprs_encode_grid_square(info, sizeof(info), &data);
        TEST_ASSERT(len == -1, "Should fail to encode invalid grid square", err);
    }

    // Error case for DF report: invalid bearing
    {
        aprs_df_report_t data = { .bearing = 400, .signal_strength = 5, .comment = NULL };
        char info[256];
        int len = aprs_encode_df_report(info, sizeof(info), &data);
        TEST_ASSERT(len == -1, "Should fail to encode invalid bearing", err);
    }

    // Error case for test packet: empty data
    {
        aprs_test_packet_t data = { .data = "", .data_len = 0 };
        char info[256];
        info[0] = '\0';
        int len = aprs_encode_test_packet(info, strlen(info), &data);
        TEST_ASSERT(len == -1, "Should fail to encode empty test packet", err);
    }

    return 0;
}

int test_aprs_raw_gps() {
    printf("test_aprs_raw_gps\n");
    int err = 0;
    // Test 1: Valid raw GPS data
    {
        const char *raw_data = "GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47";
        aprs_raw_gps_t data = { .raw_data = (char*) raw_data, .data_len = strlen(raw_data) };
        char info[256];
        int ret = aprs_encode_raw_gps(info, sizeof(info), &data);
        TEST_ASSERT(ret > 0, "Encoding failed", err);
        char expected[256];
        snprintf(expected, sizeof(expected), "$%s", raw_data);
        TEST_ASSERT(strcmp(info, expected) == 0, "Encoded string incorrect", err);
        aprs_raw_gps_t decoded;
        ret = aprs_decode_raw_gps(info, &decoded);
        TEST_ASSERT(ret == 0, "Decoding failed", err);
        TEST_ASSERT(decoded.data_len == strlen(raw_data), "Data length mismatch", err);
        TEST_ASSERT(strcmp(decoded.raw_data, raw_data) == 0, "Decoded data mismatch", err);
        free(decoded.raw_data);
    }
    // Test 2: Invalid raw GPS data (does not start with "GP")
    {
        const char *raw_data = "INVALID";
        aprs_raw_gps_t data = { .raw_data = (char*) raw_data, .data_len = strlen(raw_data) };
        char info[256];
        int ret = aprs_encode_raw_gps(info, sizeof(info), &data);
        TEST_ASSERT(ret == -1, "Encoding should fail for invalid data", err);
    }
    return err;
}

int test_aprs_grid_square() {
    printf("test_aprs_grid_square\n");
    int err = 0;
    // Test 1: 6-character grid square with comment
    {
        aprs_grid_square_t data = { .grid_square = "JN48AA", .comment = "Test comment" };
        char info[256];
        int ret = aprs_encode_grid_square(info, sizeof(info), &data);
        TEST_ASSERT(ret > 0, "Encoding failed", err);
        char expected[256];
        snprintf(expected, sizeof(expected), "[%s %s", data.grid_square, data.comment); // Changed from ">" to "["
        TEST_ASSERT(strcmp(info, expected) == 0, "Encoded string incorrect", err);
        aprs_grid_square_t decoded;
        ret = aprs_decode_grid_square(info, &decoded);
        TEST_ASSERT(ret == 0, "Decoding failed", err);
        TEST_ASSERT(strcmp(decoded.grid_square, "JN48AA") == 0, "Grid square mismatch", err);
        TEST_ASSERT(strcmp(decoded.comment, "Test comment") == 0, "Comment mismatch", err);
        free(decoded.comment);
    }
    // Test 2: 4-character grid square without comment
    {
        aprs_grid_square_t data = { .grid_square = "JN48", .comment = NULL };
        char info[256];
        int ret = aprs_encode_grid_square(info, sizeof(info), &data);
        TEST_ASSERT(ret > 0, "Encoding failed", err);
        char expected[256];
        snprintf(expected, sizeof(expected), "[%s ", data.grid_square); // Changed from ">" to "["
        TEST_ASSERT(strcmp(info, expected) == 0, "Encoded string incorrect", err);
        aprs_grid_square_t decoded;
        ret = aprs_decode_grid_square(info, &decoded);
        TEST_ASSERT(ret == 0, "Decoding failed", err);
        TEST_ASSERT(strcmp(decoded.grid_square, "JN48") == 0, "Grid square mismatch", err);
        TEST_ASSERT(decoded.comment == NULL || decoded.comment[0] == '\0', "Comment should be empty", err);
        if (decoded.comment)
            free(decoded.comment);
    }
    // Test 3: Invalid grid square length
    {
        aprs_grid_square_t data = { .grid_square = "JN4", .comment = NULL };
        char info[256];
        info[0] = '\0';
        int ret = aprs_encode_grid_square(info, strlen(info), &data);
        TEST_ASSERT(ret == -1, "Encoding should fail for invalid grid square", err);
    }
    return err;
}

int test_aprs_df_report() {
    printf("test_aprs_df_report\n");
    int err = 0;
    // Test 1: Valid DF report with comment
    {
        aprs_df_report_t data = { .bearing = 45, .signal_strength = 5, .comment = "DF test" };
        char info[256];
        int ret = aprs_encode_df_report(info, sizeof(info), &data);
        TEST_ASSERT(ret > 0, "Encoding failed", err);
        char expected[256];
        snprintf(expected, sizeof(expected), "+%03d/%d%s", data.bearing, data.signal_strength, data.comment);
        TEST_ASSERT(strcmp(info, expected) == 0, "Encoded string incorrect", err);
        aprs_df_report_t decoded;
        ret = aprs_decode_df_report(info, &decoded);
        TEST_ASSERT(ret == 0, "Decoding failed", err);
        TEST_ASSERT(decoded.bearing == 45, "Bearing mismatch", err);
        TEST_ASSERT(decoded.signal_strength == 5, "Signal strength mismatch", err);
        TEST_ASSERT(strcmp(decoded.comment, "DF test") == 0, "Comment mismatch", err);
        free(decoded.comment);
    }
    // Test 2: Valid DF report without comment
    {
        aprs_df_report_t data = { .bearing = 0, .signal_strength = 0, .comment = NULL };
        char info[256];
        int ret = aprs_encode_df_report(info, sizeof(info), &data);
        TEST_ASSERT(ret > 0, "Encoding failed", err);
        char expected[256];
        snprintf(expected, sizeof(expected), "+%03d/%d", data.bearing, data.signal_strength);
        TEST_ASSERT(strcmp(info, expected) == 0, "Encoded string incorrect", err);
        aprs_df_report_t decoded;
        ret = aprs_decode_df_report(info, &decoded);
        TEST_ASSERT(ret == 0, "Decoding failed", err);
        TEST_ASSERT(decoded.bearing == 0, "Bearing mismatch", err);
        TEST_ASSERT(decoded.signal_strength == 0, "Signal strength mismatch", err);
        TEST_ASSERT(decoded.comment == NULL || decoded.comment[0] == '\0', "Comment should be empty", err);
        if (decoded.comment)
            free(decoded.comment);
    }
    // Test 3: Invalid bearing
    {
        aprs_df_report_t data = { .bearing = 360, .signal_strength = 5, .comment = NULL };
        char info[256];
        int ret = aprs_encode_df_report(info, sizeof(info), &data);
        TEST_ASSERT(ret == -1, "Encoding should fail for invalid bearing", err);
    }
    // Test 4: Invalid signal strength
    {
        aprs_df_report_t data = { .bearing = 45, .signal_strength = 10, .comment = NULL };
        char info[256];
        int ret = aprs_encode_df_report(info, sizeof(info), &data);
        TEST_ASSERT(ret == -1, "Encoding should fail for invalid signal strength", err);
    }
    return err;
}

int test_aprs_test_packet() {
    printf("test_aprs_test_packet\n");
    int err = 0;
    // Test 1: Simple test packet
    {
        const char *test_data = "TestData123";
        aprs_test_packet_t data = { .data = (char*) test_data, .data_len = strlen(test_data) };
        char info[256];
        int ret = aprs_encode_test_packet(info, sizeof(info), &data);
        TEST_ASSERT(ret > 0, "Encoding failed", err);
        char expected[256];
        snprintf(expected, sizeof(expected), ",%s", test_data);
        TEST_ASSERT(strcmp(info, expected) == 0, "Encoded string incorrect", err);
        aprs_test_packet_t decoded;
        ret = aprs_decode_test_packet(info, &decoded);
        TEST_ASSERT(ret == 0, "Decoding failed", err);
        TEST_ASSERT(decoded.data_len == strlen(test_data), "Data length mismatch", err);
        TEST_ASSERT(strncmp(decoded.data, test_data, decoded.data_len) == 0, "Decoded data mismatch", err);
        free(decoded.data);
    }
    // Test 2: Test case for empty test packet
    {
        aprs_test_packet_t data = { .data = "", .data_len = 0 };
        char info[256];
        int len = aprs_encode_test_packet(info, sizeof(info), &data);
        TEST_ASSERT(len == 1, "Encoding of empty test packet should succeed with length 1", err);
        char expected[256] = ",";
        COMPARE_FRAME(info, (size_t )len, expected, (size_t )1, "Empty test packet encoding");
    }
    return err;
}

int test_aprs_compressed_position() {
    printf("test_aprs_compressed_position\n");
    uint8_t err = 0;

    // Test 1: Basic position (NYC)
    {
        aprs_compressed_position_t pos = { .latitude = 40.7128, .longitude = -74.0060, .symbol_table = '/', .symbol_code = '-', .comment = NULL, .dti =
        APRS_DTI_POSITION_NO_TS_NO_MSG, .has_course_speed = false, .has_altitude = false, .course = -1, .speed = -1, .altitude = INT_MIN };

        char info[100];
        int len = aprs_encode_compressed_position(info, sizeof(info), &pos);
        TEST_ASSERT(len > 0, "Compressed position encoding failed", err);
        TEST_ASSERT(len == 14, "Compressed position length incorrect", err);

        aprs_compressed_position_t decoded;
        int ret = aprs_decode_compressed_position(info, &decoded);
        TEST_ASSERT(ret == 0, "Compressed position decoding failed", err);
        TEST_ASSERT(fabs(decoded.latitude - 40.7128) < 0.01, "Decoded latitude incorrect", err);
        TEST_ASSERT(fabs(decoded.longitude - (-74.0060)) < 0.01, "Decoded longitude incorrect", err);

        aprs_free_compressed_position(&decoded);
    }

    // Test 2: Position with course and speed
    {
        aprs_compressed_position_t pos = { .latitude = 34.0522, .longitude = -118.2437, .symbol_table = '/', .symbol_code = '>', .comment = my_strdup(
                "Moving west"), .dti = APRS_DTI_POSITION_NO_TS_NO_MSG, .has_course_speed = true, .has_altitude = false, .course = 268,  // multiple of 4
                .speed = 63,    // use 63 knots for exact round-trip
                .altitude = INT_MIN };

        char info[100];
        int len = aprs_encode_compressed_position(info, sizeof(info), &pos);
        TEST_ASSERT(len > 0, "Compressed position with course/speed encoding failed", err);

        aprs_compressed_position_t decoded;
        int ret = aprs_decode_compressed_position(info, &decoded);
        TEST_ASSERT(ret == 0, "Compressed position with course/speed decoding failed", err);
        TEST_ASSERT(decoded.has_course_speed, "Course/speed flag not set", err);

        // Allow small tolerance for 4-degree quantization on course
        int course_diff = abs(decoded.course - 268);
        TEST_ASSERT(course_diff <= 4, "Decoded course incorrect", err);

        // With speed=63, the decode matches exactly (tolerance still <=1)
        TEST_ASSERT(abs(decoded.speed - 63) <= 1, "Decoded speed incorrect", err);

        free(pos.comment);
        aprs_free_compressed_position(&decoded);
    }

    // Test 3: Position with altitude (exact round-trip value)
    {
        aprs_compressed_position_t pos = { .latitude = 39.7392, .longitude = -104.9903, .symbol_table = '\\', .symbol_code = '^', .comment = my_strdup(
                "Altitude test"), .dti = APRS_DTI_POSITION_NO_TS_NO_MSG, .has_course_speed = false, .has_altitude = true, .course = -1, .speed = -1, .altitude =
                1999  // chosen value that encodes/decodes exactly
                };

        char info[100];
        int len = aprs_encode_compressed_position(info, sizeof(info), &pos);
        TEST_ASSERT(len > 0, "Compressed position with altitude encoding failed", err);

        aprs_compressed_position_t decoded;
        int ret = aprs_decode_compressed_position(info, &decoded);
        TEST_ASSERT(ret == 0, "Compressed position with altitude decoding failed", err);
        TEST_ASSERT(decoded.has_altitude, "Altitude flag not set", err);
        TEST_ASSERT(decoded.altitude == 1999, "Decoded altitude incorrect", err);

        free(pos.comment);
        aprs_free_compressed_position(&decoded);
    }

    return err;
}

int test_aprs_weather_extensions(void) {
    printf("test_aprs_weather_extensions\n");
    aprs_weather_report_t input = { .wind_direction = 360, .wind_speed = 4, .wind_gust = 15, .temperature = 71, .rain_1h = 0, .rain_24h = 33,
            .rain_midnight = 2, .humidity = 54, .barometric_pressure = 10001 };

    char encoded[128];
    int encoded_len = aprs_encode_peet1(encoded, sizeof(encoded), &input);
    TEST_ASSERT(encoded_len > 0, "Encoding Peet Bros #W1", 1);

    const char expected[] = "#W1c360s004g015t071r000p033P002h54b10001";
    COMPARE_FRAME(encoded, (size_t )encoded_len, expected, (size_t )strlen(expected), "Encoded Peet #W1 matches expected");

    aprs_weather_report_t decoded = { 0 };
    int r = aprs_decode_peet1(encoded, &decoded);
    TEST_ASSERT(r == 0, "Decoding Peet Bros #W1", 2);

    TEST_ASSERT(decoded.wind_direction == 360, "Wind direction == 360", 3);
    TEST_ASSERT(decoded.wind_speed == 4, "Wind speed == 4", 4);
    TEST_ASSERT(decoded.wind_gust == 15, "Wind gust == 15", 5);
    TEST_ASSERT(decoded.temperature == 71, "Temperature == 71", 6);
    TEST_ASSERT(decoded.rain_1h == 0, "Rain 1h == 0", 7);
    TEST_ASSERT(decoded.rain_24h == 33, "Rain 24h == 33", 8);
    TEST_ASSERT(decoded.rain_midnight == 2, "Rain since midnight == 2", 9);
    TEST_ASSERT(decoded.humidity == 54, "Humidity == 54", 10);
    TEST_ASSERT(decoded.barometric_pressure == 10001, "Pressure == 10001", 11);

    // Test decode from position-carrying weather
    aprs_position_no_ts_t pos = { .latitude = 42.0, .longitude = -71.0, .symbol_table = '/', .symbol_code = '_', .has_course_speed = 1, .course = 180, .speed =
            5, };

    static char comment_buf[100];
    snprintf(comment_buf, sizeof(comment_buf), "c360s004t071g015r000p033P002h54b10001");
    pos.comment = comment_buf;

    aprs_weather_report_t extracted = { 0 };
    TEST_ASSERT(aprs_decode_position_weather(&pos, &extracted) == 0, "Decode position-carrying weather", 12);

    TEST_ASSERT(extracted.wind_direction == 360, "Extracted wind dir == 360", 13);
    TEST_ASSERT(extracted.wind_speed == 4, "Extracted wind speed == 4", 14);
    TEST_ASSERT(extracted.wind_gust == 15, "Extracted wind gust == 15", 15);
    TEST_ASSERT(extracted.temperature == 71, "Extracted temp == 71", 16);
    TEST_ASSERT(extracted.rain_1h == 0, "Extracted rain 1h == 0", 17);
    TEST_ASSERT(extracted.rain_24h == 33, "Extracted rain 24h == 33", 18);
    TEST_ASSERT(extracted.rain_midnight == 2, "Extracted rain midnight == 2", 19);
    TEST_ASSERT(extracted.humidity == 54, "Extracted humidity == 54", 20);
    TEST_ASSERT(extracted.barometric_pressure == 10001, "Extracted pressure == 10001", 21);

    return 0;
}

int test_aprs_directed_query() {
    printf("test_aprs_directed_query\n");
    uint8_t err = 0;
    aprs_station_info_t local_station = { .callsign = "MYCALL", .software_version = "TestStation 1.0", .status_text = "Station operational", .latitude = 34.0,
            .longitude = -117.0, .symbol_table = '/', .symbol_code = '>', .has_dest = true, .dest_lat = 34.1, .dest_lon = -116.9, .has_altitude = false,
            .altitude = 0, .timestamp = "061230z" };

    // Simulate incoming APRS message ":MYCALL   :?APRS?"
    char info_in[100] = { 0 };
    strcpy(info_in, ":MYCALL   :?APRS?");
    aprs_message_t msg;
    int ret = aprs_decode_message(info_in, &msg);
    TEST_ASSERT(ret == 0, "Failed to decode incoming message", err);

    // Handle the directed query
    char response[100] = { 0 };
    int rlen = aprs_handle_directed_query(&msg, response, sizeof(response), local_station);
    TEST_ASSERT(rlen > 0, "No response generated for directed query", err);

    // Verify response content matches the configured version string
    TEST_ASSERT(strcmp(response, "TestStation 1.0") == 0, "Incorrect response to ?APRS? query", err);

    free(msg.message);
    return err;
}

int test_aprs_main() {
    int result = 0;
    printf("\n----------------------------------------------------------------------------------\n");
    printf("Starting APRS Tests\n");
    printf("----------------------------------------------------------------------------------\n\n");
    result |= test_aprs_position_encoding_decoding();
    result |= test_aprs_message_encoding_decoding();
    result |= test_aprs_real_packets();
    result |= test_aprs_edge_cases();
    result |= test_aprs_weather_object_position();
    result |= test_aprs_position_with_ts();
    result |= test_aprs_weather();
    result |= test_aprs_object();
    result |= test_aprs_mice();
    result |= test_aprs_telemetry();
    result |= test_aprs_status();
    result |= test_aprs_general_query();
    result |= test_aprs_station_capabilities();
    result |= test_aprs_packets();
    result |= test_aprs_item_report();
    result |= test_aprs_bulletin();
    result |= test_other();
    result |= test_aprs_grid_square();
    result |= test_aprs_df_report();
    result |= test_aprs_test_packet();
    result |= test_aprs_compressed_position();
    result |= test_aprs_weather_extensions();
    result |= test_aprs_directed_query();
    printf("\n----------------------------------------------------------------------------------\n");
    printf("Tests APRS Completed. %s\n", result == 0 ? "All tests passed" : "Some tests failed");
    printf("----------------------------------------------------------------------------------\n\n");
    return result;
}
