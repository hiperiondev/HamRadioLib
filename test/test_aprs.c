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
#include <stdint.h>
#include <math.h>

#include "test_common.h"
#include "aprs.h"

static uint32_t assert_count = 0;

// Helper function to trim trailing spaces from a string
static void trim_trailing_spaces(char *str) {
    size_t len = strlen(str);
    while (len > 0 && str[len - 1] == ' ') {
        str[--len] = '\0';
    }
}

int test_aprs_address_encoding_decoding() {
    uint8_t err = 0;

    // Test 1: Destination address "APRS" (no SSID, not last)
    {
        aprs_address_t addr = { .callsign = "APRS  ", .ssid = 0 };
        char buf[7];
        aprs_encode_address(buf, &addr, false, false);
        uint8_t expected[7] = { ('A' << 1), ('P' << 1), ('R' << 1), ('S' << 1), (' ' << 1), (' ' << 1), 0 };
        for (int i = 0; i < 7; i++) {
            TEST_ASSERT((uint8_t )buf[i] == expected[i], "Destination address encoding incorrect", err);
        }
        aprs_address_t decoded;
        bool is_last;
        aprs_decode_address(buf, &decoded, &is_last);
        TEST_ASSERT(strcmp(decoded.callsign, "APRS") == 0, "Decoded destination callsign incorrect", err);
        TEST_ASSERT(decoded.ssid == 0, "Decoded destination SSID incorrect", err);
        TEST_ASSERT(is_last == false, "is_last should be false for destination", err);
    }

    // Test 2: Source address "N0CALL-7" (last, no digipeaters)
    {
        aprs_address_t addr = { .callsign = "N0CALL", .ssid = 7 };
        char buf[7];
        aprs_encode_address(buf, &addr, true, false);
        uint8_t expected[7] = { ('N' << 1), ('0' << 1), ('C' << 1), ('A' << 1), ('L' << 1), ('L' << 1), (7 << 1) | 0x01 };
        for (int i = 0; i < 7; i++) {
            TEST_ASSERT((uint8_t )buf[i] == expected[i], "Source address encoding incorrect", err);
        }
        aprs_address_t decoded;
        bool is_last;
        aprs_decode_address(buf, &decoded, &is_last);
        char trimmed[7];
        strncpy(trimmed, decoded.callsign, 6);
        trimmed[6] = '\0';
        trim_trailing_spaces(trimmed);
        TEST_ASSERT(strcmp(trimmed, "N0CALL") == 0, "Decoded source callsign incorrect", err);
        TEST_ASSERT(decoded.ssid == 7, "Decoded source SSID incorrect", err);
        TEST_ASSERT(is_last == true, "is_last should be true for source", err);
    }

    // Test 3: Digipeater "WIDE1-1" (not last)
    {
        aprs_address_t addr = { .callsign = "WIDE1 ", .ssid = 1 };
        char buf[7];
        aprs_encode_address(buf, &addr, false, true);
        uint8_t expected[7] = { ('W' << 1), ('I' << 1), ('D' << 1), ('E' << 1), ('1' << 1), (' ' << 1), (1 << 1) };
        for (int i = 0; i < 7; i++) {
            TEST_ASSERT((uint8_t )buf[i] == expected[i], "Digipeater address encoding incorrect", err);
        }
        aprs_address_t decoded;
        bool is_last;
        aprs_decode_address(buf, &decoded, &is_last);
        char trimmed[7];
        strncpy(trimmed, decoded.callsign, 6);
        trimmed[6] = '\0';
        trim_trailing_spaces(trimmed);
        TEST_ASSERT(strcmp(trimmed, "WIDE1") == 0, "Decoded digipeater callsign incorrect", err);
        TEST_ASSERT(decoded.ssid == 1, "Decoded digipeater SSID incorrect", err);
        TEST_ASSERT(is_last == false, "is_last should be false for digipeater", err);
    }

    return err;
}

int test_aprs_frame_encoding_decoding() {
    uint8_t err = 0;

    // Test 4: Frame with no digipeaters (APRS > N0CALL-7, info="!4903.50N/07201.75W-")
    {
        aprs_frame_t frame = { .destination = { .callsign = "APRS  ", .ssid = 0 }, .source = { .callsign = "N0CALL", .ssid = 7 }, .num_digipeaters = 0, .info =
                "!4903.50N/07201.75W-", .info_len = 20 };
        char buf[256];
        int len = aprs_encode_frame(buf, 256, &frame);
        TEST_ASSERT(len == 40, "Frame encoding length incorrect", err);
        aprs_frame_t decoded;
        int decode_len = aprs_decode_frame(buf, len, &decoded);
        TEST_ASSERT(decode_len == len, "Frame decoding length mismatch", err);
        char trimmed_dest[7];
        strncpy(trimmed_dest, decoded.destination.callsign, 6);
        trimmed_dest[6] = '\0';
        trim_trailing_spaces(trimmed_dest);
        TEST_ASSERT(strcmp(trimmed_dest, "APRS") == 0, "Decoded destination callsign mismatch", err);
        TEST_ASSERT(decoded.source.ssid == 7, "Decoded source SSID mismatch", err);
        TEST_ASSERT(decoded.num_digipeaters == 0, "Number of digipeaters mismatch", err);
        TEST_ASSERT(decoded.info_len == 20, "Info length mismatch", err);
        TEST_ASSERT(strncmp(decoded.info, "!4903.50N/07201.75W-", 20) == 0, "Info content mismatch", err);
        free(decoded.info);
    }

    // Test 5: Frame with one digipeater (APRS > N0CALL-7 via WIDE1-1)
    {
        aprs_frame_t frame = { .destination = { .callsign = "APRS  ", .ssid = 0 }, .source = { .callsign = "N0CALL", .ssid = 7 }, .digipeaters = { { .callsign =
                "WIDE1 ", .ssid = 1 } }, .num_digipeaters = 1, .info = ":WB2OSZ-7 :Hello", .info_len = 16 };
        char buf[256];
        int len = aprs_encode_frame(buf, 256, &frame);
        TEST_ASSERT(len == 43, "Frame with digipeater encoding length incorrect", err);
        aprs_frame_t decoded;
        int decode_len = aprs_decode_frame(buf, len, &decoded);
        TEST_ASSERT(decode_len == len, "Frame decoding length mismatch", err);
        TEST_ASSERT(decoded.num_digipeaters == 1, "Number of digipeaters mismatch", err);
        char trimmed_digi[7];
        strncpy(trimmed_digi, decoded.digipeaters[0].callsign, 6);
        trimmed_digi[6] = '\0';
        trim_trailing_spaces(trimmed_digi);
        TEST_ASSERT(strcmp(trimmed_digi, "WIDE1") == 0, "Digipeater callsign mismatch", err);
        TEST_ASSERT(decoded.info_len == 16, "Info length mismatch", err);
        free(decoded.info);
    }

    return err;
}

int test_aprs_position_encoding_decoding() {
    uint8_t err = 0;

    // Test 6: Position report (49.5N, -72.75W)
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

    // Test 7: Position with no comment
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

    return err;
}

int test_aprs_message_encoding_decoding() {
    uint8_t err = 0;

    // Test 8: Message with number
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

    // Test 9: Message without number
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
    uint8_t err = 0;

    // Test 11: Real position report "!4903.50N/07201.75W-Test /A=001234"
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

    // Test 12: Real message ":WB2OSZ-7 :Hello{001}"
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

    // Test 13: Real frame encoding (APRS > KX4O-7, "!3821.31N/07742.02W>")
    {
        aprs_frame_t frame = { .destination = { .callsign = "APRS  ", .ssid = 0 }, .source = { .callsign = "KX4O  ", .ssid = 7 }, .num_digipeaters = 0, .info =
                "!3821.31N/07742.02W>", .info_len = 20 };
        char buf[256];
        int len = aprs_encode_frame(buf, 256, &frame);
        TEST_ASSERT(len == 40, "Real frame encoding length incorrect", err);
        aprs_frame_t decoded;
        int decode_len = aprs_decode_frame(buf, len, &decoded);
        TEST_ASSERT(decode_len == len, "Real frame decoding length mismatch", err);
        char trimmed_source[7];
        strncpy(trimmed_source, decoded.source.callsign, 6);
        trimmed_source[6] = '\0';
        trim_trailing_spaces(trimmed_source);
        TEST_ASSERT(strcmp(trimmed_source, "KX4O") == 0, "Real frame source callsign mismatch", err);
        TEST_ASSERT(decoded.info_len == 20, "Real frame info length mismatch", err);
        TEST_ASSERT(strncmp(decoded.info, "!3821.31N/07742.02W>", 20) == 0, "Real frame info mismatch", err);
        free(decoded.info);
    }

    return err;
}

int test_aprs_edge_cases() {
    uint8_t err = 0;

    // Test 14: Invalid latitude
    {
        char *lat_str = lat_to_aprs(91.0);
        TEST_ASSERT(lat_str == NULL, "Latitude > 90 should return NULL", err);
    }

    // Test 15: Invalid longitude
    {
        char *lon_str = lon_to_aprs(-181.0);
        TEST_ASSERT(lon_str == NULL, "Longitude < -180 should return NULL", err);
    }

    // Test 16: Message with long addressee
    {
        aprs_message_t msg;
        memcpy(msg.addressee, "TOOLONGADD", 10); // 10 chars, no null terminator
        msg.message = "Test";
        msg.message_number = NULL;
        char info[100];
        int len = aprs_encode_message(info, 100, &msg);
        TEST_ASSERT(len == -1, "Encoding long addressee should fail", err);
    }

    // Test 17: Frame with max digipeaters
    {
        aprs_frame_t frame = { .destination = { .callsign = "APRS  ", .ssid = 0 }, .source = { .callsign = "N0CALL", .ssid = 0 }, .digipeaters =
                { { .callsign = "WIDE1 ", .ssid = 1 }, { .callsign = "WIDE2 ", .ssid = 2 }, { .callsign = "WIDE3 ", .ssid = 3 }, { .callsign = "WIDE4 ", .ssid =
                        4 }, { .callsign = "WIDE5 ", .ssid = 5 }, { .callsign = "WIDE6 ", .ssid = 6 }, { .callsign = "WIDE7 ", .ssid = 7 }, { .callsign =
                        "WIDE8 ", .ssid = 8 } }, .num_digipeaters = 8, .info = "!4900.00N/07200.00W/", .info_len = 20 };
        char buf[256];
        int len = aprs_encode_frame(buf, 256, &frame);
        TEST_ASSERT(len == 96, "Max digipeaters frame length incorrect", err);
        aprs_frame_t decoded;
        int decode_len = aprs_decode_frame(buf, len, &decoded);
        TEST_ASSERT(decode_len == len, "Max digipeaters decoding length mismatch", err);
        TEST_ASSERT(decoded.num_digipeaters == 8, "Max digipeaters count mismatch", err);
        free(decoded.info);
    }

    return err;
}

int test_aprs_weather_object_position() {
    uint8_t err = 0;

    // Test: Weather report encoding and decoding
    {
        aprs_weather_report_t weather = { .timestamp = "12010000", .temperature = 25.0, .wind_speed = 10, .wind_direction = 180 };
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

    // Test: Object report encoding and decoding
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

    // Test: Timestamped position report encoding and decoding
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

    // Test: Mic-E encoding and decoding
    {
        aprs_mice_t mice = { .latitude = 33.426667, // 33°25.60'N
                .longitude = -112.129, // 112°07.74'W
                .speed = 20, .course = 251, .symbol_table = '/', .symbol_code = '[', .message_code = "M3" // Returning
                };
        aprs_address_t source = { .callsign = "N0CALL", .ssid = 0 };
        aprs_address_t digipeaters[1] = { { .callsign = "WIDE1 ", .ssid = 1 } };
        char buf[256];
        int len = aprs_encode_mice_frame(buf, 256, &mice, &source, digipeaters, 1);
        TEST_ASSERT(len >= 25, "Mic-E frame encoding length incorrect", err);
        aprs_mice_t decoded;
        aprs_address_t decoded_source;
        aprs_address_t decoded_digipeaters[8];
        int decoded_num_digipeaters;
        int ret = aprs_decode_mice_frame(buf, len, &decoded, &decoded_source, decoded_digipeaters, &decoded_num_digipeaters);
        TEST_ASSERT(ret == 0, "Mic-E frame decoding failed", err);
        TEST_ASSERT(fabs(decoded.latitude - 33.426667) < 0.001, "Mic-E latitude mismatch", err);
        TEST_ASSERT(fabs(decoded.longitude + 112.129) < 0.001, "Mic-E longitude mismatch", err);
        TEST_ASSERT(decoded.speed == 20, "Mic-E speed mismatch", err);
        TEST_ASSERT(decoded.course == 251, "Mic-E course mismatch", err);
        TEST_ASSERT(decoded.symbol_table == '/', "Mic-E symbol table mismatch", err);
        TEST_ASSERT(decoded.symbol_code == '[', "Mic-E symbol code mismatch", err);
        TEST_ASSERT(strcmp(decoded.message_code, "M3") == 0, "Mic-E message code mismatch", err);
        TEST_ASSERT(strcmp(decoded_source.callsign, "N0CALL") == 0, "Mic-E source callsign mismatch", err);
        TEST_ASSERT(decoded_num_digipeaters == 1, "Mic-E digipeater count mismatch", err);
    }

    // Test: Telemetry encoding and decoding
    {
        aprs_telemetry_t telem = { .callsign = "N0CALL", .ssid = 0, .sequence_number = 123, .analog = { 100.0, 200.0, 150.0, 50.0, 255.0 }, .digital = 0xA5 // 10100101
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
    uint8_t err = 0;
    const char *dest_str = "SUSURB";
    const unsigned char info[] = { 0x60, 0x43, 0x46, 0x22, 0x1C, 0x1F, 0x21, 0x5B, 0x2F, 0x3A, 0x60, 0x22, 0x33, 0x7A, 0x7D, 0x5F, 0x20, 0x00 };
    aprs_mice_t mice;
    int message_bits;
    bool ns, long_offset, we;
    int ret = aprs_decode_mice_destination(dest_str, &mice, &message_bits, &ns, &long_offset, &we);
    TEST_ASSERT(ret == 0, "Failed to decode Mic-E destination", err);
    ret = aprs_decode_mice_info((const char *)info, sizeof(info) - 1, &mice, long_offset, we);
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

int test_aprs_main() {
    int result = 0;
    printf("\n----------------------------------------------------------------------------------\n");
    printf("Starting APRS Tests\n");
    printf("----------------------------------------------------------------------------------\n\n");
    result |= test_aprs_address_encoding_decoding();
    result |= test_aprs_frame_encoding_decoding();
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
    printf("\n----------------------------------------------------------------------------------\n");
    printf("Tests APRS Completed. %s\n", result == 0 ? "All tests passed" : "Some tests failed");
    printf("----------------------------------------------------------------------------------\n\n");
    return result;
}
