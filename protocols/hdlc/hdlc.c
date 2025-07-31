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
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "common.h"

unsigned char ReverseBits(unsigned char byte) {
    byte = ((byte >> 1) & 0x55) | ((byte & 0x55) << 1);
    byte = ((byte >> 2) & 0x33) | ((byte & 0x33) << 2);
    byte = ((byte >> 4) & 0x0F) | ((byte & 0x0F) << 4);
    return byte;
}

void hdlc_frame_encode(unsigned char *frame, int frameLen, unsigned char *encodedFrame, int *encodedLen) {
    for (int i = 0; i < frameLen; i++) {
        frame[i] = ReverseBits(frame[i]);
    }

    uint16_t crc = CRC(frame, frameLen);
    frame[frameLen++] = (crc >> 8) & 0xFF;
    frame[frameLen++] = crc & 0xFF;

    int cnt = 0;
    int bitIndex = 7;
    unsigned char byte = 0;
    int encodedIndex = 0;

    encodedFrame[encodedIndex++] = 0x7E;

    for (int i = 0; i < frameLen; i++) {
        for (int mask = 128; mask > 0; mask >>= 1) {
            if (frame[i] & mask) {
                byte |= (1 << bitIndex);
                bitIndex--;
                if (bitIndex < 0) {
                    encodedFrame[encodedIndex++] = byte;
                    byte = 0;
                    bitIndex = 7;
                }
                cnt++;
                if (cnt == 5) {
                    bitIndex--;
                    if (bitIndex < 0) {
                        encodedFrame[encodedIndex++] = byte;
                        byte = 0;
                        bitIndex = 7;
                    }
                    cnt = 0;
                }
            } else {
                bitIndex--;
                if (bitIndex < 0) {
                    encodedFrame[encodedIndex++] = byte;
                    byte = 0;
                    bitIndex = 7;
                }
                cnt = 0;
            }
        }
    }

    bitIndex--;
    if (bitIndex < 0) {
        encodedFrame[encodedIndex++] = byte;
        byte = 0;
        bitIndex = 7;
    }
    for (int i = 0; i < 6; i++) {
        byte |= (1 << bitIndex);
        bitIndex--;
        if (bitIndex < 0) {
            encodedFrame[encodedIndex++] = byte;
            byte = 0;
            bitIndex = 7;
        }
    }
    bitIndex--;
    if (bitIndex >= 0) {
        encodedFrame[encodedIndex++] = byte;
    } else {
        encodedFrame[encodedIndex++] = byte;
    }

    *encodedLen = encodedIndex;
}

int hdlc_frame_decode(unsigned char *encodedFrame, int encodedLen, unsigned char *decodedFrame, int *decodedLen) {
    int startFlagFound = 0;
    int endFlagFound = 0;
    int cnt = 0;
    int bitIndex = 0;
    unsigned char byte = 0;
    unsigned char shiftRegister = 0;
    int decodedIndex = 0;

    for (int i = 0; i < encodedLen; i++) {
        for (int k = 7; k >= 0; k--) {
            unsigned char bit = (encodedFrame[i] >> k) & 0x01;
            shiftRegister = ((shiftRegister << 1) | bit) & 0xFF;

            if (!startFlagFound && shiftRegister == 0x7E) {
                startFlagFound = 1;
                cnt = 0;
                bitIndex = 0;
                byte = 0;
                continue;
            }

            if (startFlagFound) {
                if (shiftRegister == 0x7E) {
                    endFlagFound = 1;
                    break;
                } else {
                    if (bit == 1) {
                        cnt++;
                        if (cnt > 6) {
                            return -1;
                        }
                        byte = (byte << 1) | bit;
                        bitIndex++;
                    } else if (cnt == 5) {
                        cnt = 0;
                    } else {
                        cnt = 0;
                        byte = (byte << 1) | bit;
                        bitIndex++;
                    }

                    if (bitIndex == 8) {
                        decodedFrame[decodedIndex++] = byte;
                        byte = 0;
                        bitIndex = 0;
                    }
                }
            }
        }
        if (endFlagFound)
            break;
    }

    if (!endFlagFound)
        return -1;

    if (decodedIndex < 2)
        return -1;
    uint16_t frameCRC = (decodedFrame[decodedIndex - 2] << 8) | decodedFrame[decodedIndex - 1];
    decodedIndex -= 2;

    uint16_t crc = CRC(decodedFrame, decodedIndex);
    if (crc != frameCRC)
        return -1;

    for (int i = 0; i < decodedIndex; i++) {
        decodedFrame[i] = ReverseBits(decodedFrame[i]);
    }

    *decodedLen = decodedIndex;
    return 0;
}
