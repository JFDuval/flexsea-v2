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
 [This file] flexsea_command: OSI layers 4-7 of the FlexSEA protocol v2.0
 ****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

//FlexSEA Command Prototype:
//==========================
//[CMD + R/W][DATA...]
//=> CMD is a 6-bit command code
//=> R/W is a 2-bit code to determine if this command is a Read, Write, or
//   Read/Write
//=> CMD and R/W share a byte. CMD has the 6 MSBs, and RW has the 2 LSBs.
//=> Data is a byte array

//This file is all about sending and receiving commands. The command code and
//R/W coding is determined, but the data structure is left to the user.

//****************************************************************************
// Include(s)
//****************************************************************************

#include "flexsea.h"
#include <flexsea_command.h>

//****************************************************************************
// Variable(s)
//****************************************************************************

//Function pointer array that points to the command handlers
uint8_t (*fx_rx_cmd_handler_ptr[MAX_CMD_CODE])(uint8_t cmd_6bits, ReadWrite rw,
		uint8_t *buf, uint8_t buf_len);

//****************************************************************************
// Private Function Prototype(s):
//****************************************************************************

__attribute__((weak)) uint8_t fx_register_rx_cmd_handlers(void);
static uint8_t fx_rx_cmd_handler_catchall(uint8_t cmd_6bits, ReadWrite rw,
		uint8_t *buf, uint8_t buf_len);

//****************************************************************************
// Public Function(s)
//****************************************************************************

//Initialize function pointer array. Without this we hard-fault.
uint8_t fx_rx_cmd_init(void)
{
	int i = 0;

	//By default, they all point to 'flexsea_payload_catchall()'
	for(i = 0; i < MAX_CMD_CODE; i++)
	{
		fx_rx_cmd_handler_ptr[i] = &fx_rx_cmd_handler_catchall;
	}

	//In the user-space, pair command codes and functions by
	//using register_command()
	fx_register_rx_cmd_handlers();


	return 0;
}

//Creates a TX command by adding a command code and RW to a data string
//'uint8_t cmd_6bits': 6-bit command code
//'ReadWrite rw': 2-bit R/W message type
//'uint8_t *buf_in': input data
//'uint8_t buf_in_len': input data length
//'uint8_t *buf_out': output data
//'uint8_t buf_out_len': output data length
uint8_t fx_create_tx_cmd(uint8_t cmd_6bits, ReadWrite rw, uint8_t *buf_in,
		uint8_t buf_in_len, uint8_t *buf_out, uint8_t *buf_out_len)
{
	uint8_t cmd_rw = 0;

	if((cmd_6bits >= MIN_CMD_CODE) && (cmd_6bits <= MAX_CMD_CODE) &&
			(rw != CmdInvalid))
	{
		//Create the CMD + R/W byte
		switch(rw)
		{
			case CmdRead:
				cmd_rw = CMD_SET_R(cmd_6bits);
				break;
			case CmdWrite:
				cmd_rw = CMD_SET_W(cmd_6bits);
				break;
			case CmdReadWrite:
				cmd_rw = CMD_SET_RW(cmd_6bits);
				break;
			default:
				//Invalid!
				return 1;
		}

		//Create the output data string
		buf_out[CMD_CODE_INDEX] = cmd_rw;
		memcpy(&buf_out[CMD_CODE_INDEX + 1], buf_in, buf_in_len);
		*buf_out_len = buf_in_len + 1;

		return 0;
	}
	else
	{
		*buf_out_len = 0;
		return 1;
	}
}

//This function takes a decoded payload as an input, and determines what the command code and R/W is
//It doesn't do much at this point, but it is ready to be expanded (addressing, etc.)
//'uint8_t *decoded': serialized data, typically obtained from fx_decode()
//'uint8_t decoded_len': serialized data length
//'uint8_t *cmd_6bits': 6-bit command code (if valid, 0 otherwise)
//'ReadWrite *rw': 2-bit R/W (if valid, 0 otherwise)
uint8_t fx_parse_rx_cmd(uint8_t* decoded, uint8_t decoded_len, uint8_t *cmd_6bits, ReadWrite *rw)
{
	uint8_t _cmd = 0, _cmd_6bits = 0, valid = 0;
	ReadWrite _rw = CmdInvalid;
	//Command
	_cmd = decoded[CMD_CODE_INDEX];		//CMD w/ R/W bit
	_cmd_6bits = CMD_GET_6BITS(_cmd);	//CMD code, no R/W information
	_rw = CMD_GET_RW(_cmd);
	valid = CMD_IS_VALID(_cmd);

	if((_cmd_6bits >= MIN_CMD_CODE) && (_cmd_6bits <= MAX_CMD_CODE) && valid)
	{
		//Valid function!
		*cmd_6bits = _cmd_6bits;
		*rw = _rw;

		//At this point we are ready to use the function pointer array to call a
		//specific command function. We do not call it here as it would prevent us
		//from unit testing this function. The use needs to do that next.
		//Usage:
		//  If calling from this file:
		//    (*fx_rx_cmd_handler_ptr[cmd_6bits]) (cmd_6bits, rw, unpacked, unpacked_len);
		//  If calling from another file:
		//    ToDo

		return 0;
	}
	else
	{
		//Invalid function!
		*cmd_6bits = 0;
		*rw = CmdInvalid;
		return 1;
	}
}

//To avoid exposing fx_rx_cmd_handler_ptr we wrap it in this function
uint8_t fx_call_rx_cmd_handler(uint8_t cmd_6bits, ReadWrite rw,
		uint8_t *buf, uint8_t len)
{
	return (*fx_rx_cmd_handler_ptr[cmd_6bits]) (cmd_6bits, rw, buf, len);
}

//Pair a function and a command code together
uint8_t fx_register_rx_cmd_handler(uint8_t cmd, uint8_t (*fct_prt) (uint8_t, ReadWrite, uint8_t *, uint8_t))
{
	if((cmd >= MIN_CMD_CODE) && (cmd < MAX_CMD_CODE))
	{
		fx_rx_cmd_handler_ptr[cmd] = fct_prt;
		return 0;
	}
	else
	{
		return 1;
	}
}

__attribute__((weak)) uint8_t fx_register_rx_cmd_handlers(void)
{
	//Implement in user space, and register your handlers
	return 0;
}

//****************************************************************************
// Private Function(s)
//****************************************************************************

//Catch all function, which is also our prototype
//'uint8_t cmd': 6-bit command code
//'uint8_t rw': 2-bit Read / Write / Read-Write code
//'uint8_t *buf': serialized data
//'uint8_t len': length of the serialized data
static uint8_t fx_rx_cmd_handler_catchall(uint8_t cmd_6bits, ReadWrite rw,
		uint8_t *buf, uint8_t len)
{
	(void)cmd_6bits;
	(void)rw;
	(void)buf;
	(void)len;

	return CATCHALL_RETURN;
}

#ifdef __cplusplus
}
#endif
