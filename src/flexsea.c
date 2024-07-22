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

#ifdef __cplusplus
extern "C" {
#endif

//****************************************************************************
// Include(s)
//****************************************************************************

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <flexsea.h>

//****************************************************************************
// Variable(s)
//****************************************************************************

//****************************************************************************
// Private Function Prototype(s):
//****************************************************************************

//****************************************************************************
// Public Function(s)
//****************************************************************************

//From command to bytestream
uint8_t fx_create_bytestream_from_cmd(uint8_t cmd_6bits, ReadWrite rw, uint8_t *buf_in,
		uint8_t buf_in_len, uint8_t* bytestream, uint8_t *bytestream_len)
{
	//Temporary variables to hold data between command creation
	//and encoding
	uint8_t payload_out[MAX_ENCODED_PAYLOAD_BYTES] = {0};
	uint8_t payload_out_len = 0;

	//Create a valid command (command code and RW bits)
	if(!fx_create_tx_cmd(cmd_6bits, rw, buf_in, buf_in_len,
			payload_out, &payload_out_len))
	{
		//Encode it
		if(!fx_encode(payload_out, payload_out_len, bytestream,
							bytestream_len, MAX_ENCODED_PAYLOAD_BYTES))
		{
			//Success!
			return 0;
		}
		else
		{
			*bytestream_len = 0;
			return 1;
		}
	}

	*bytestream_len = 0;
	return 1;
}

#ifdef __cplusplus
}
#endif
