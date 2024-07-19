/****************************************************************************
 [Project] FlexSEA: Flexible & Scalable Electronics Architecture
 [Sub-project] 'flexsea-comm' Communication stack
 Copyright (C) 2016 Dephy, Inc. <http://dephy.com/>

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
 [Lead developper] Jean-Francois (JF) Duval, jfduval at dephy dot com.
 [Origin] Based on Jean-Francois Duval's work at the MIT Media Lab
 Biomechatronics research group <http://biomech.media.mit.edu/>
 [Contributors]
 *****************************************************************************
 [This file] flexsea_comm: Data-Link layer of the FlexSEA protocol
 *****************************************************************************
 [Change log] (Convention: YYYY-MM-DD | author | comment)
 * 2016-09-09 | jfduval | Initial GPL-3.0 release
 *
 ****************************************************************************/

#ifndef INC_FX_COMM_H
#define INC_FX_COMM_H

#ifdef __cplusplus
extern "C" {
#endif

//****************************************************************************
// Include(s)
//****************************************************************************

#include "circ_buf.h"

//****************************************************************************
// Definition(s):
//****************************************************************************

//Framing:
#define HEADER  						0xED	//237d
#define FOOTER  						0xEE	//238d
#define ESCAPE  						0xE9	//233d

//Buffers and packets:
#define MIN_OVERHEAD					4		//Header + Footer + Checksum + # bytes
#define MAX_PACKED_PAYLOAD_BYTES		48		//Max number of bytes in a packed payload

//****************************************************************************
// Public Function Prototype(s):
//****************************************************************************

uint8_t comm_pack_payload(uint8_t *payload, uint8_t payload_len,
		uint8_t *packed_payload, uint8_t *packed_payload_len,
				uint8_t max_packed_payload_len);
uint8_t comm_unpack_payload(circ_buf_t *cb, uint8_t *packed, uint8_t *packed_len,
		uint8_t *unpacked, uint8_t *unpacked_len);

//****************************************************************************
// Structure(s):
//****************************************************************************

//****************************************************************************
// Shared variable(s)
//****************************************************************************

#ifdef __cplusplus
}
#endif

#endif	//INC_FX_COMM_H
