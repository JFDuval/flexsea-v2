/****************************************************************************
 [Project] FlexSEA: Flexible & Scalable Electronics Architecture v2
 Copyright (C) 2024 JFDuval Engineering LLC

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *****************************************************************************
 [Lead developer] Jean-Francois (JF) Duval, jfduval at jfduvaleng dot com.
 [Origin] Based on Jean-Francois Duval's work at the MIT Media Lab
 Biomechatronics research group <http://biomech.media.mit.edu/> (2013-2015)
 [Contributors to v1] Work maintained and expended by Dephy, Inc. (2015-20xx)
 [v2.0] Complete re-write based on the original idea. (2024)
 *****************************************************************************
 [This file] flexsea_codec: Data-Link layer of the FlexSEA protocol v2.0
 ****************************************************************************/

#ifndef INC_FX_CODEC_H
#define INC_FX_CODEC_H

#ifdef __cplusplus
extern "C" {
#endif

//****************************************************************************
// Include(s)
//****************************************************************************

//****************************************************************************
// Definition(s):
//****************************************************************************

//Framing:
#define HEADER  						0xED	//237d
#define FOOTER  						0xEE	//238d
#define ESCAPE  						0xE9	//233d

//Buffers and packets:
#define MIN_OVERHEAD					4		//Header + Footer + Checksum + # bytes
#define MAX_ENCODED_PAYLOAD_BYTES		200		//Max number of bytes in a packed payload
//Note: if you want to send 50 bytes, set MAX_ENCODED_PAYLOAD_BYTES to at least
//54 to support the minimum overhead. Any escapes bytes will prevent it from working,
//so we recommend keeping some margin.
//Some variables are uint8, do not exceed 256 bytes!

//****************************************************************************
// Public Function Prototype(s):
//****************************************************************************

uint8_t fx_encode(uint8_t *payload, uint8_t payload_len,
		uint8_t *encoded_payload, uint8_t *encoded_payload_len,
				uint8_t max_encoded_payload_len);
uint8_t fx_decode(circ_buf_t *cb, uint8_t *encoded, uint8_t *encoded_len,
		uint8_t *decoded, uint8_t *decoded_len);
uint8_t fx_cleanup(circ_buf_t *cb);

//****************************************************************************
// Structure(s):
//****************************************************************************

//****************************************************************************
// Shared variable(s)
//****************************************************************************

#ifdef __cplusplus
}
#endif

#endif	//INC_FX_CODEC_H
