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

#ifdef __cplusplus
extern "C" {
#endif

//FlexSEA comm. prototype:
//=======================
//[HEADER][# of BYTES][DATA...][CHECKSUM][FOOTER]
//=> Number of bytes includes the ESCAPE bytes
//=> Checksum is done on the payload (data + ESCAPEs) and on the BYTES byte.

//To transmit a message:
//======================
// 1) Place the payload in an array (no header, no footer: pure data)
// 2) Call comm_gen_str(your_data_array, number_of_bytes)
// 2b) It will return the index of the last byte of the message (add 1 for the length)
// 2c) The message is in comm_str[]
// 3) Send comm_str[] (x_puts(comm_str, msg_length));

//To receive a message:
//=====================
// 1) Assuming that you have dealt with all the previous messages, call comm_str_payload();
//    to fill the buffer with zeros
// 2) Every time you receive a byte update the buffer: comm_update_rx_buffer(your_new_byte);
// 3) Call payload_str_available_in_buffer = comm_decode_str(). If you get >= 1, read the
//    comm_str_payload buffer and do something with the data!
// 4) At this point you might want to flush the read payload from rx_buf

//****************************************************************************
// Include(s)
//****************************************************************************

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <flexsea_comm.h>
//#include "log.h"
//#include "flexsea_user_structs.h"

//****************************************************************************
// Variable(s)
//****************************************************************************

uint8_t comm_str[2][COMM_PERIPH_ARR_LEN];
uint8_t rx_command[2][COMM_PERIPH_ARR_LEN];

//PacketWrapper packet[NUMBER_OF_PORTS][2];
//CommPeriph commPeriph[NUMBER_OF_PORTS];

uint32_t cmd_valid = 0;
uint32_t cmd_bad_checksum = 0;

//struct commSpy_s commSpy1 = {0,0,0,0,0,0,0};

//****************************************************************************
// Private Function Prototype(s):
//****************************************************************************


//****************************************************************************
// Public Function(s)
//****************************************************************************

//Takes payload, adds ESCAPES, checksum, header, ...
//ToDo: this needs to return an error code, not a number of bytes
uint8_t comm_gen_str(uint8_t payload[], uint8_t *cstr, uint8_t bytes)
{
	unsigned int i = 0, escapes = 0, idx = 0, total_bytes = 0;
	uint8_t checksum = 0;

	//Fill comm_str with known values ('a')
	memset(cstr, 0xAA, COMM_STR_BUF_LEN);

	//Fill comm_str with payload and add ESCAPE characters
	escapes = 0;
	idx = 2;
	for(i = 0; i < bytes && idx < COMM_STR_BUF_LEN; i++)
	{
		if((payload[i] == HEADER) || (payload[i] == FOOTER) || (payload[i] == ESCAPE))
		{
			escapes = escapes + 1;
			cstr[idx] = ESCAPE;
			cstr[idx+1] = payload[i];
			checksum += cstr[idx];
			checksum += cstr[idx+1];
			idx = idx + 1;
		}
		else
		{
			cstr[idx] = payload[i];
			checksum += cstr[idx];
		}
		idx++;
	}

	if((idx + 2) >= COMM_STR_BUF_LEN)
	{
		//Comm string too long, abort
		memset(cstr, 0, COMM_STR_BUF_LEN);	//Clear string
		return 0;
	}

	total_bytes = bytes + escapes;

	//String length?
	if(total_bytes >= COMM_STR_BUF_LEN)
	{
		//Too long, abort:
		memset(cstr, 0, COMM_STR_BUF_LEN);	//Clear string
		return 0;
	}

	//Build comm_str:
	cstr[0] = HEADER;
	cstr[1] = total_bytes;
	cstr[2 + total_bytes] = checksum;
	cstr[3 + total_bytes] = FOOTER;

	//Return the last index of the valid data
	return (3 + total_bytes);
}

//Original FlexSEA code - for reference only
uint16_t unpack_payload_cb(circularBuffer_t *cb, uint8_t *packed, uint8_t unpacked[PACKAGED_PAYLOAD_LEN])
{
	//LOG(linfo,"unpack_payload_cb called");
	int bufSize = circ_buff_get_size(cb);

	int foundString = 0, foundFrame = 0, bytes, possibleFooterPos;
	int lastPossibleHeaderIndex = bufSize - 4;
	int headerPos = -1, lastHeaderPos = -1;
	uint8_t checksum = 0;

	int headers = 0, footers = 0;
	while(!foundString && lastHeaderPos < lastPossibleHeaderIndex)
	{
		headerPos = circ_buff_search(cb, HEADER, lastHeaderPos+1);
		//if we can't find a header, we quit searching for strings
		if(headerPos == -1) break;

		headers++;
		foundFrame = 0;
		if(headerPos <= lastPossibleHeaderIndex)
		{
			//LOG(ldebug2,"Last possible header");
			bytes = circ_buff_peek(cb, headerPos + 1);
			possibleFooterPos = headerPos + 3 + bytes;
			foundFrame = (possibleFooterPos < bufSize && circ_buff_peek(cb, possibleFooterPos) == FOOTER);
		}

		if(foundFrame)
		{
			//LOG(ldebug2,"Frame found");
			footers++;
			checksum = circ_buff_checksum(cb, headerPos+2, possibleFooterPos-1);

			//if checksum is valid than we found a valid string
			foundString = (checksum == circ_buff_peek(cb, possibleFooterPos-1));
		}

		//either we found a frame and it had a valid checksum, or we want to try the next value of i
		lastHeaderPos = headerPos;
	}

	int numBytesInPackedString = 0;
	if(foundString)
	{
		//LOG(ldebug2,"String found");
		numBytesInPackedString = headerPos + bytes + 4;

		circ_buff_read_section(cb, packed, headerPos, bytes + 4);

		int k, skip = 0, unpacked_idx = 0;
		for(k = 0; k < bytes; k++)
		{
			int index = k+2; //first value is header, next value is bytes, next value is first data
			if(packed[index] == ESCAPE && skip == 0)
			{
				skip = 1;
			}
			else
			{
				skip = 0;
				unpacked[unpacked_idx++] = packed[index];
			}
		}
	}

	return numBytesInPackedString;
}

//Moving this to the new circ buf code - WIP
uint16_t unpack_payload_cb2(circularBuffer_t *cb, uint8_t *packed, uint8_t unpacked[PACKAGED_PAYLOAD_LEN])
{
	//LOG(linfo,"unpack_payload_cb called");
	int bufSize = circ_buff_get_size(cb);

	int foundString = 0, foundFrame = 0, bytes, possibleFooterPos;
	int lastPossibleHeaderIndex = bufSize - 4;
	int headerPos = -1, lastHeaderPos = -1;
	uint8_t checksum = 0;

	int headers = 0, footers = 0;
	while(!foundString && lastHeaderPos < lastPossibleHeaderIndex)
	{
		headerPos = circ_buff_search(cb, HEADER, lastHeaderPos+1);
		//if we can't find a header, we quit searching for strings
		if(headerPos == -1) break;

		headers++;
		foundFrame = 0;
		if(headerPos <= lastPossibleHeaderIndex)
		{
			//LOG(ldebug2,"Last possible header");
			bytes = circ_buff_peek(cb, headerPos + 1);
			possibleFooterPos = headerPos + 3 + bytes;
			foundFrame = (possibleFooterPos < bufSize && circ_buff_peek(cb, possibleFooterPos) == FOOTER);
		}

		if(foundFrame)
		{
			//LOG(ldebug2,"Frame found");
			footers++;
			checksum = circ_buff_checksum(cb, headerPos+2, possibleFooterPos-1);

			//if checksum is valid than we found a valid string
			foundString = (checksum == circ_buff_peek(cb, possibleFooterPos-1));
		}

		//either we found a frame and it had a valid checksum, or we want to try the next value of i
		lastHeaderPos = headerPos;
	}

	int numBytesInPackedString = 0;
	if(foundString)
	{
		//LOG(ldebug2,"String found");
		numBytesInPackedString = headerPos + bytes + 4;

		circ_buff_read_section(cb, packed, headerPos, bytes + 4);

		int k, skip = 0, unpacked_idx = 0;
		for(k = 0; k < bytes; k++)
		{
			int index = k+2; //first value is header, next value is bytes, next value is first data
			if(packed[index] == ESCAPE && skip == 0)
			{
				skip = 1;
			}
			else
			{
				skip = 0;
				unpacked[unpacked_idx++] = packed[index];
			}
		}
	}

	return numBytesInPackedString;
}

#ifdef __cplusplus
}
#endif
