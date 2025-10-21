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

#ifndef INC_FX_COMM_H
#define INC_FX_COMM_H

#ifdef __cplusplus
extern "C" {
#endif

//****************************************************************************
// Include(s)
//****************************************************************************

//****************************************************************************
// Definition(s):
//****************************************************************************

#define DBUF_MAX_LEN	256

//****************************************************************************
// Structure(s):
//****************************************************************************

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
// Public Function Prototype(s):
//****************************************************************************

void fx_comm_process_ping_pong_buffers(CommPort *cp);

//****************************************************************************
// Shared variable(s)
//****************************************************************************

#ifdef __cplusplus
}
#endif

#endif	//INC_FX_COMM_H
