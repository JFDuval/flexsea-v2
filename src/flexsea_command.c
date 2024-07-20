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
 [This file] flexsea_command: OSI layers 4-7 of the FlexSEA protocol v2.0
 ****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

//FlexSEA Command Prototype:  WWWWWIIIIIPPPP
//==========================
//[CMD][R/W][DATA...]
//=> CMD is a 6-bit command code
//=> R/W is a 2-bit code to determine if this command is a Read, Write, or
//   Read/Write
//=> CMD and R/W share a byte. CMD has the 6 MSBs, and RW has the 2 LSBs
//=> Data is a byte array

//This file is all TODO TODO TODO

//****************************************************************************
// Include(s)
//****************************************************************************

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <flexsea_command.h>

//****************************************************************************
// Variable(s)
//****************************************************************************

//Function pointer array:
void (*flexsea_payload_ptr[MAX_CMD_CODE])(uint8_t cmd_6bits, ReadWrite rw,
		uint8_t *buf, uint16_t len);

//****************************************************************************
// Private Function Prototype(s):
//****************************************************************************

static void flexsea_payload_catchall(uint8_t cmd_6bits, ReadWrite rw,
		uint8_t *buf, uint16_t len);

//****************************************************************************
// Public Function(s)
//****************************************************************************

//Initialize function pointer array
void init_flexsea_payload_ptr(void)
{
	int i = 0;

	//By default, they all point to 'flexsea_payload_catchall()'
	for(i = 0; i < MAX_CMD_CODE; i++)
	{
		flexsea_payload_ptr[i] = &flexsea_payload_catchall;
	}

	//Attach pointers to your project-specific functions:
	//(index = command code)
	//======================================================

	//ToDo
}

/*
//Generic TX command
//ToDo: this function crashes if len is too long! (35 is too much...)
uint16_t tx_cmd(uint8_t *payloadData, uint8_t cmdCode, uint8_t cmd_type, \
				uint32_t len, uint8_t receiver, uint8_t *buf)
{
	uint16_t bytes = 0;
	uint16_t index = 0;
	uint32_t length = 0;

	//Protection against long len:
	length = (len > PAYLOAD_BYTES) ? PAYLOAD_BYTES : len;

	prepare_empty_payload(getBoardID(), receiver, buf, sizeof(buf));
	buf[P_CMDS] = 1;	//Fixed, 1 command

	if(cmd_type == CMD_READ)
	{
		buf[P_CMD1] = CMD_R(cmdCode);
	}
	else if(cmd_type == CMD_WRITE)
	{
		buf[P_CMD1] = CMD_W(cmdCode);
	}
	else
	{
		flexsea_error(SE_INVALID_READ_TYPE);
		return 0;
	}

	index = P_DATA1;
	memcpy(&buf[index], payloadData, length);
	bytes = index+length;

	return bytes;
}
*/

//This function takes an unpacked payload as an input, and determines what the command code and R/W is
//It doesn't do much at this point, but it is ready to be expanded (addressing, etc.)
//'uint8_t *unpacked': serialized data
//'uint8_t unpacked_len': serialized data length
//'uint8_t *cmd_6bits': 6-bit command code (if valid, 0 otherwise)
//'uint8_t *rw': 2-bit R/W (if valid, 0 otherwise)
uint8_t payload_parse_str(uint8_t* unpacked, uint16_t unpacked_len, uint8_t *cmd_6bits, ReadWrite *rw)
{
	uint8_t _cmd = 0, _cmd_6bits = 0, valid = 0;
	ReadWrite _rw = CmdInvalid;
	//Command
	_cmd = unpacked[CMD_CODE_INDEX];		//CMD w/ R/W bit
	_cmd_6bits = CMD_GET_6BITS(_cmd);		//CMD code, no R/W information
	_rw = CMD_GET_RW(_cmd);
	valid = CMD_IS_VALID(_cmd);

	if((_cmd_6bits <= MAX_CMD_CODE) && valid)
	{
		//Valid function!
		*cmd_6bits = _cmd_6bits;
		*rw = _rw;

		//At this point we are ready to use the function pointer array to call a
		//specific command function. We do not call it here as it would prevent us
		//from unit testing this function. The use needs to do that next.
		//Ex.: (*flexsea_payload_ptr[cmd_6bits]) (cmd_6bits, rw, unpacked, unpacked_len);

		return 0;
	}
	else
	{
		//Invalid function!
		*cmd_6bits = 0;
		*rw = 0;
		return 1;
	}
}

//To avoid exposing flexsea_payload_ptr we wrap it in this function
void flexsea_payload(uint8_t cmd_6bits, ReadWrite rw,
		uint8_t *buf, uint16_t len)
{
	(*flexsea_payload_ptr[cmd_6bits]) (cmd_6bits, rw, buf, len);
}

//****************************************************************************
// Private Function(s)
//****************************************************************************

//Catch all function, which is also our prototype
//'uint8_t cmd': 6-bit command code
//'uint8_t rw': 2-bit Read / Write / Read-Write code
//'uint8_t *buf': serialized data
//'uint16_t len': length of the serialized data
static void flexsea_payload_catchall(uint8_t cmd_6bits, ReadWrite rw,
		uint8_t *buf, uint16_t len)
{
	(void)cmd_6bits;
	(void)rw;
	(void)buf;
	(void)len;
	return;
}

#ifdef __cplusplus
}
#endif
