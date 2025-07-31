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

#ifndef HDLC_H_
#define HDLC_H_

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Reverses the bits of a given byte.
 *
 * This function takes a single unsigned char (byte) and reverses the order of its bits.
 * For example, the byte 0xE1 (binary 11100001) becomes 0x87 (binary 10000111).
 * This operation is essential in HDLC encoding and decoding because the AX.25 protocol,
 * which uses HDLC framing, transmits data with the least significant bit (LSB) first.
 * The function employs bitwise operations to efficiently swap the bits within the byte.
 *
 * @param byte The input byte whose bits are to be reversed.
 * @return The byte with its bits reversed.
 */
unsigned char ReverseBits(unsigned char byte);

/**
 * @brief Encodes an AX.25 frame into an HDLC frame.
 *
 * This function transforms an AX.25 frame into an HDLC frame suitable for transmission.
 * The encoding process includes several steps:
 * - Reverses the bits of each byte in the input frame using ReverseBits() to match the LSB-first
 *   transmission order of AX.25.
 * - Calculates a 16-bit Frame Check Sequence (FCS) using the CRC function from common.h and appends
 *   it to the frame for error detection.
 * - Performs bit stuffing: after five consecutive 1 bits, a 0 bit is inserted to prevent the flag
 *   sequence (0x7E) from appearing within the data.
 * - Adds the HDLC flag byte (0x7E) at the beginning and end of the frame to delimit the frame boundaries.
 *
 * The encoded frame is written to the provided encodedFrame buffer, and its length is stored in encodedLen.
 * The input frame buffer is modified in-place to include the FCS before encoding.
 *
 * @param frame Pointer to the input AX.25 frame data. This buffer must have enough space to append
 *              two additional bytes for the FCS (frameLen + 2 bytes minimum).
 * @param frameLen Length of the input frame in bytes, excluding the FCS.
 * @param encodedFrame Pointer to the output buffer where the encoded HDLC frame will be stored.
 *                     Must be large enough to hold the encoded data, including flags and potential
 *                     bit stuffing (typically up to 1.25x frameLen + 2 bytes).
 * @param encodedLen Pointer to an integer where the length of the encoded frame will be stored,
 *                   in bytes, including the start and end flags.
 */
void hdlc_frame_encode(unsigned char *frame, int frameLen, unsigned char *encodedFrame, int *encodedLen);

/**
 * @brief Decodes an HDLC frame back into an AX.25 frame.
 *
 * This function reverses the HDLC encoding process to extract an AX.25 frame from an HDLC-encoded frame.
 * The decoding process involves:
 * - Detecting the start and end flags (0x7E) to identify the frame boundaries.
 * - Removing bit stuffing: when five consecutive 1 bits are followed by a 0 bit, the 0 bit is skipped.
 * - Extracting the frame data and the appended FCS (16-bit CRC).
 * - Verifying the FCS using the CRC function from common.h to ensure the frame is not corrupted.
 * - Reversing the bits of each byte in the decoded frame using ReverseBits() to restore the original
 *   byte order expected by AX.25.
 *
 * The decoded frame is written to the decodedFrame buffer, and its length (excluding FCS) is stored
 * in decodedLen. The function returns 0 on success or -1 if decoding fails due to invalid flags,
 * FCS mismatch, or insufficient data.
 *
 * @param encodedFrame Pointer to the input HDLC frame data, including start and end flags (0x7E).
 * @param encodedLen Length of the input HDLC frame in bytes.
 * @param decodedFrame Pointer to the output buffer where the decoded AX.25 frame will be stored.
 *                     Must be large enough to hold the decoded data (typically encodedLen or less).
 * @param decodedLen Pointer to an integer where the length of the decoded frame will be stored,
 *                   in bytes, excluding the FCS.
 * @return 0 on successful decoding, -1 on failure (e.g., missing flags, FCS mismatch, or frame too short).
 */
int hdlc_frame_decode(unsigned char *encodedFrame, int encodedLen, unsigned char *decodedFrame, int *decodedLen);

#endif /* HDLC_H_ */
