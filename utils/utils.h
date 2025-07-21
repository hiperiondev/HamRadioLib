/*
 * Copyright 2025 Emiliano Augusto Gonzalez (egonzalez . hiperion @ gmail . com))
 * * Project https://github.com/hiperiondev/HamRadioLib *
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

#ifndef UTILS_H_
#define UTILS_H_

void hdlc_frame_print(unsigned char *hdlc_frame, int hdlc_len);
void ax25_frame_print(unsigned char *ax25_frame, int ax25_frame_len);
void aprs_frame_print(unsigned char *aprs_frame, int aprs_len);

unsigned char ReverseBits(unsigned char byte);

#endif /* UTILS_H_ */
