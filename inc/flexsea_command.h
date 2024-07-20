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

#ifndef INC_FX_COMMAND_H
#define INC_FX_COMMAND_H

#ifdef __cplusplus
extern "C" {
#endif

//****************************************************************************
// Include(s)
//****************************************************************************


//****************************************************************************
// Definition(s):
//**********************************************************************1******

//6 bits for command codes, 2 bits for the R/W bits
#define MIN_CMD_CODE	1	//We use CMD=0 as a way to detect an incorrect cmd
#define MAX_CMD_CODE	63

typedef enum{
  CmdInvalid,		//00b: Invalid
  CmdRead,			//01b: Read
  CmdWrite,			//10b: Write
  CmdReadWrite		//11b: Read/Write
}ReadWrite;

//Macros to deal with the R/W bits
#define CMD_SET_R(x)		((x << 2) | CmdRead)
#define CMD_SET_W(x)		((x << 2) | CmdWrite)
#define CMD_SET_RW(x)		((x << 2) | CmdReadWrite)
#define CMD_GET_6BITS(x)	((x & 0xFF) >> 2)
#define CMD_GET_RW(x)		(x & 3)
#define CMD_IS_VALID(x)		(x & 3) ? 1 : 0

#define CMD_CODE_INDEX		0

//****************************************************************************
// Public Function Prototype(s):
//****************************************************************************

void init_flexsea_payload_ptr(void);
uint8_t payload_parse_str(uint8_t* unpacked, uint16_t unpacked_len,
		uint8_t *cmd_6bits, ReadWrite *rw);
void flexsea_payload(uint8_t cmd_6bits, ReadWrite rw,
		uint8_t *buf, uint16_t len);

//****************************************************************************
// Structure(s):
//****************************************************************************

//****************************************************************************
// Shared variable(s)
//****************************************************************************

#ifdef __cplusplus
}
#endif

#endif	//INC_FX_COMMAND_H
