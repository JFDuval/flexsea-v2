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

//Return codes:
#define UNPACK_ERR_HEADER				-1
#define UNPACK_ERR_FOOTER				-2
#define UNPACK_ERR_LEN					-3
#define UNPACK_ERR_CHECKSUM				-4

//Buffers and packets:
#define RX_BUF_LEN						150		//Reception buffer (flexsea_comm)
#define PAYLOAD_BUF_LEN					36		//Number of bytes in a payload string
#define PAYLOAD_BYTES					(PAYLOAD_BUF_LEN - 4)
#define COMM_STR_BUF_LEN				48		//Number of bytes in a comm. string
#define PACKAGED_PAYLOAD_LEN			48		//Temporary
#define PACKET_WRAPPER_LEN				RX_BUF_LEN
#define COMM_PERIPH_ARR_LEN				RX_BUF_LEN

//****************************************************************************
// Public Function Prototype(s):
//****************************************************************************

uint8_t comm_pack_payload(uint8_t *payload, uint8_t payload_bytes,
		uint8_t *packed_payload, uint8_t *packed_payload_bytes);
uint8_t comm_unpack_payload(circ_buf_t *cb, uint8_t *packed,
		uint8_t unpacked[PACKAGED_PAYLOAD_LEN]);

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
