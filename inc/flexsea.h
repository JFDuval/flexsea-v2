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
 [This file] flexsea: top-level FlexSEA protocol v2.0
 ****************************************************************************/

#ifndef INC_FX_FLEXSEA_H
#define INC_FX_FLEXSEA_H

#ifdef __cplusplus
extern "C" {
#endif

//****************************************************************************
// Include(s)
//****************************************************************************

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "circ_buf.h"
#include <flexsea_codec.h>
#include <flexsea_command.h>
#include <flexsea_tools.h>

//****************************************************************************
// Definition(s):
//****************************************************************************

//Use these for return calls:
#define FX_SUCCESS		0
#define FX_PROBLEM		1

//****************************************************************************
// Public Function Prototype(s):
//****************************************************************************

uint8_t fx_create_bytestream_from_cmd(uint8_t cmd_6bits, ReadWrite rw,
		AckNack ack, uint8_t *buf_in, uint8_t buf_in_len, uint8_t* bytestream,
		uint8_t *bytestream_len);
uint8_t fx_get_cmd_handler_from_bytestream(circ_buf_t *cb,
		uint8_t *cmd_6bits, ReadWrite *rw, AckNack *ack, uint8_t *buf,
		uint8_t *buf_len);

//****************************************************************************
// Structure(s):
//****************************************************************************

#define DBUF_MAX_LEN	256

//This structure holds all the info about a communication port
typedef struct CommPort
{
	uint8_t id;					//Port identification
	uint8_t send_reply;			//Do we have a reply to send?
	uint8_t reply_cmd;			//What is it?
	uint8_t send_ack;			//Do we need to acknowledge a Write?
	uint8_t ack_cmd;			//Command code we are acknowledging
	uint16_t ack_packet_num;	//Packet number we are acknowledging
	circ_buf_t *cb;				//Reception circular buffer
	uint8_t (*tx_fct_prt) (uint8_t *, uint16_t);	//TX function
	//Double buffering with ping-pong buffers (ToDo)
	volatile uint8_t dbuf_ping[DBUF_MAX_LEN];
	volatile uint8_t dbuf_pong[DBUF_MAX_LEN];
	volatile uint8_t dbuf_lock_ping;
	volatile uint8_t dbuf_lock_pong;
	volatile uint32_t dbuf_ping_len;
	volatile uint32_t dbuf_pong_len;
	volatile uint8_t dbuf_selected;
}CommPort;

//****************************************************************************
// Shared variable(s)
//****************************************************************************

#ifdef __cplusplus
}
#endif

#endif	//INC_FX_FLEXSEA_H
