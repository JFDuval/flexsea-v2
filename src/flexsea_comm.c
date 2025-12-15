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
 [Lead developer] Jean-Francois (JF) Duval, jf at jfduvaleng dot com.
 [Origin] Based on Jean-Francois Duval's work at the MIT Media Lab
 Biomechatronics research group <http://biomech.media.mit.edu/> (2013-2015)
 [Contributors to v1] Work maintained and expended by Dephy, Inc. (2015-20xx)
 [v2.0] Complete re-write based on the original idea. (2024)
 *****************************************************************************
 [This file] flexsea_codec: Data-Link layer of the FlexSEA protocol v2.0
 ****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

//****************************************************************************
// Include(s)
//****************************************************************************

#include "flexsea.h"
#include <flexsea_comm.h>

//****************************************************************************
// Variable(s)
//****************************************************************************

//****************************************************************************
// Private Function Prototype(s):
//****************************************************************************

//****************************************************************************
// Public Function(s)
//****************************************************************************

//We write to the Circular Buffer here, when new data has been received.
void fx_comm_process_ping_pong_buffers(CommPort *cp)
{
	if(cp->use_dbuf)
	{
		//Read from the non-selected buffer
		if(cp->dbuf_len[!cp->dbuf_selected] > 0 && !cp->dbuf_lock[!cp->dbuf_selected])
		{
			for(int i = 0; i < cp->dbuf_len[!cp->dbuf_selected]; i++)
			{
				circ_buf_write_byte(cp->cb, cp->dbuf[!cp->dbuf_selected][i]);
			}
			cp->dbuf_len[!cp->dbuf_selected] = 0;
		}

		//If we are reading too slow, pong might also be full...
		if(cp->dbuf_len[cp->dbuf_selected] > 0 && !cp->dbuf_lock[cp->dbuf_selected])
		{
			for(int i = 0; i < cp->dbuf_len[cp->dbuf_selected]; i++)
			{
				circ_buf_write_byte(cp->cb, cp->dbuf[cp->dbuf_selected][i]);
			}
			cp->dbuf_len[cp->dbuf_selected] = 0;
		}
	}
}

uint8_t fx_receive(CommPort *cp)
{
	uint8_t cmd_6bits_out = 0;
	ReadWrite rw_out = CmdInvalid;
	AckNack ack_out = Nack;
	uint8_t buf[MAX_ENCODED_PAYLOAD_BYTES] = {0};
	uint8_t buf_len = 0;
	uint8_t ret_val = 0, ret_val_cmd = 0;
	cp->send_reply = 0;
	cp->send_ack = 0;
	cp->ack_cmd = 0;
	cp->ack_packet_num = 0;

	fx_comm_process_ping_pong_buffers(cp);

	//Receive commands
	if(cp->cb->length > MIN_OVERHEAD)
	{
		//At this point our encoded command is in the circular buffer
		ret_val = fx_get_cmd_handler_from_bytestream(cp->cb, &cmd_6bits_out, &rw_out,
				&ack_out, buf, &buf_len);

		//Call handler
		if(!ret_val)
		{
			ret_val_cmd = fx_call_rx_cmd_handler(cmd_6bits_out, rw_out, ack_out, buf, buf_len);

			if(!ret_val_cmd)
			{
				//Reply if requested
				if((rw_out == CmdRead) || (rw_out == CmdReadWrite))
				{
					cp->reply_cmd = cmd_6bits_out;
					cp->send_reply = 1;
				}

				//Write with Ack request?
				if((rw_out == CmdWrite) && (ack_out == Ack))
				{
					cp->send_ack = 1;
					cp->ack_cmd = cmd_6bits_out;
					cp->ack_packet_num = get_last_rx_packet_num();
				}

				//Proceed with clean-up procedure
				fx_cleanup(cp->cb);

				return FX_SUCCESS;	//Success = we decoded something
			}
		}
	}
	else
	{
		fx_cleanup(cp->cb);
	}

	return FX_PROBLEM;	//Not really a problem, but we didn't decode anything.
}

#ifdef __cplusplus
}
#endif
