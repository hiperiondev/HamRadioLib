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
#include "ax25.h"  // For ax25_address_t used in Mic-E functions

// Structures for APRS data types

typedef struct {
    double latitude;
    double longitude;
    char symbol_table;
    char symbol_code;
    char *comment;
    char dti;  // Data Type Indicator
    bool has_course_speed;
    int course;
    int speed;
} aprs_position_no_ts_t;

typedef struct {
    char addressee[10];
    char *message;
    char *message_number;
} aprs_message_t;

typedef struct {
    char timestamp[9];  // MMDDHHMM format
    float temperature;
    int wind_speed;
    int wind_direction;
} aprs_weather_report_t;

typedef struct {
    char name[10];
    char timestamp[8];  // DDHHMMz format
    double latitude;
    double longitude;
    char symbol_table;
    char symbol_code;
} aprs_object_report_t;

typedef struct {
    char dti;
    char timestamp[8];  // DDHHMMz format
    double latitude;
    double longitude;
    char symbol_table;
    char symbol_code;
    char *comment;
} aprs_position_with_ts_t;

typedef struct {
    double latitude;
    double longitude;
    int speed;
    int course;
    char symbol_table;
    char symbol_code;
    char message_code[10];
} aprs_mice_t;

typedef struct {
    char callsign[7];
    uint8_t ssid;
    unsigned int sequence_number;
    double analog[5];
    uint8_t digital;
} aprs_telemetry_t;

typedef struct {
    bool has_timestamp;
    char timestamp[8];  // DDHHMMz format
    char status_text[63];
} aprs_status_t;

typedef struct {
    char query_type[11];
} aprs_general_query_t;

typedef struct {
    char capabilities_text[100];
} aprs_station_capabilities_t;

// Function declarations

// Encoding functions for standard APRS data types
int aprs_encode_position_no_ts(char *info, size_t len, const aprs_position_no_ts_t *data);
int aprs_encode_message(char *info, size_t len, const aprs_message_t *data);
int aprs_encode_weather_report(char *info, size_t len, const aprs_weather_report_t *data);
int aprs_encode_object_report(char *info, size_t len, const aprs_object_report_t *data);
int aprs_encode_position_with_ts(char *info, size_t len, const aprs_position_with_ts_t *data);
int aprs_encode_telemetry(char *info, size_t len, const aprs_telemetry_t *data);
int aprs_encode_status(char *info, size_t len, const aprs_status_t *data);
int aprs_encode_general_query(char *info, size_t len, const aprs_general_query_t *data);
int aprs_encode_station_capabilities(char *info, size_t len, const aprs_station_capabilities_t *data);

// Decoding functions for standard APRS data types
int aprs_decode_position_no_ts(const char *info, aprs_position_no_ts_t *data);
int aprs_decode_message(const char *info, aprs_message_t *data);
int aprs_decode_weather_report(const char *info, aprs_weather_report_t *data);
int aprs_decode_object_report(const char *info, aprs_object_report_t *data);
int aprs_decode_position_with_ts(const char *info, aprs_position_with_ts_t *data);
int aprs_decode_telemetry(const char *info, aprs_telemetry_t *data);
int aprs_decode_status(const char *info, aprs_status_t *data);
int aprs_decode_general_query(const char *info, aprs_general_query_t *data);
int aprs_decode_station_capabilities(const char *info, aprs_station_capabilities_t *data);

// Mic-E specific functions
int aprs_encode_mice_frame(char *buf, size_t buf_len, const aprs_mice_t *data, const ax25_address_t *source, const ax25_address_t *digipeaters, int num_digipeaters);
int aprs_decode_mice_frame(const char *buf, size_t len, aprs_mice_t *data, ax25_address_t *source, ax25_address_t *digipeaters, int *num_digipeaters);

// Utility functions
char* lat_to_aprs(double lat, int ambiguity);
char* lon_to_aprs(double lon, int ambiguity);

#endif /* APRS_H_ */
