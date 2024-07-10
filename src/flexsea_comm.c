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
 [Lead developer] Jean-Francois (JF) Duval, jfduval at dephy dot com.
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

//ToDo: update this documentation

//FlexSEA comm. prototype:
//=======================
//[HEADER][# of BYTES][PAYLOAD (DATA)...][CHECKSUM][FOOTER]
//=> Number of bytes includes the ESCAPE bytes
//=> Checksum is done on the payload (data + ESCAPEs) and on the BYTES byte.

//****************************************************************************
// Include(s)
//****************************************************************************

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <flexsea_comm.h>
#include "circ_buf.h"

//****************************************************************************
// Variable(s)
//****************************************************************************

//****************************************************************************
// Private Function Prototype(s):
//****************************************************************************

//****************************************************************************
// Public Function(s)
//****************************************************************************

//Takes a payload (raw data) and packs it so it can be sent on the wire
//Packing means adding a header, a footer, escape chars (if needed), and a checksum
//'uint8_t *payload': the data you want to send, stored in an array
//'uint8_t payload_bytes': number of bytes in 'payload' (could be shorter than sizeof())
//'uint8_t *packed_ payload': array used to return a packed payload (this is your output)
//'uint8_t *packed_payload_bytes': number of bytes in 'packed_payload'
//Returns 0 if it was able to pack it, 1 otherwise

//ToDo remove COMM_STR_BUF_LEN
//ToDo optional byte filling, of a given length

uint8_t comm_pack_payload(uint8_t *payload, uint8_t payload_bytes,
		uint8_t *packed_payload, uint8_t *packed_payload_bytes)
{
	uint16_t i = 0, escapes = 0, idx = 0, total_bytes = 0;
	uint8_t checksum = 0;

	//Fill packaged_payload with known values ('a')
	memset(packed_payload, 0xAA, COMM_STR_BUF_LEN);

	//Fill packaged_payload with payload and add ESCAPE characters when necessary
	escapes = 0;
	idx = 2;
	for(i = 0; i < payload_bytes && idx < COMM_STR_BUF_LEN; i++)
	{
		if((payload[i] == HEADER) || (payload[i] == FOOTER)
				|| (payload[i] == ESCAPE))
		{
			escapes = escapes + 1;
			packed_payload[idx] = ESCAPE;
			packed_payload[idx + 1] = payload[i];
			checksum += packed_payload[idx];
			checksum += packed_payload[idx + 1];
			idx = idx + 1;
		}
		else
		{
			packed_payload[idx] = payload[i];
			checksum += packed_payload[idx];
		}
		idx++;
	}

	if((idx + 2) >= COMM_STR_BUF_LEN)
	{
		//Packaged payload too long, abort
		memset(packed_payload, 0, COMM_STR_BUF_LEN);	//Clear string
		return 1;
	}

	total_bytes = payload_bytes + escapes;

	//String length?
	if(total_bytes >= COMM_STR_BUF_LEN)
	{
		//Too long, abort:
		memset(packed_payload, 0, COMM_STR_BUF_LEN);	//Clear string
		return 1;
	}

	//Build comm_str:
	packed_payload[0] = HEADER;
	packed_payload[1] = total_bytes;
	packed_payload[2 + total_bytes] = checksum;
	packed_payload[3 + total_bytes] = FOOTER;

	//Return the last index of the valid data
	*packed_payload_bytes = (3 + total_bytes);
	return 0;
}

//Takes data from the wire (stored in a circular buffer) and extracts the first valid payload

//ToDo: return number of bytes (pointers), and error code
//ToDo: is it useful to return the packed version?

uint8_t comm_unpack_payload(circ_buf_t *cb, uint8_t *packed,
		uint8_t unpacked[PACKAGED_PAYLOAD_LEN])
{
	uint16_t cb_size = circ_buf_get_size(cb);
	uint16_t found_packed_payload = 0, found_footer = 0,
			possible_footer_pos = 0;
	uint16_t last_possible_header_index = cb_size - 4;
	uint16_t header_pos = 0, last_header_pos = 0;
	uint8_t first_time = 1;
	uint8_t checksum = 0;
	uint8_t ret_val = 0;
	uint8_t bytes_in_packed_payload = 0;
	uint8_t byte_peek = 0;

	//We look for a packed payload, starting by searching for a header
	uint16_t headers = 0, footers = 0;
	while(!found_packed_payload
			&& (last_header_pos < last_possible_header_index))
	{
		//We start from index 0 (first time), then from the last header position
		if(first_time)
		{
			ret_val = circ_buf_search(cb, &header_pos, HEADER, last_header_pos);
			first_time = 0;
		}
		else
		{
			ret_val = circ_buf_search(cb, &header_pos, HEADER,
					last_header_pos + 1);
		}

		//If we can't find a header, we quit searching for packed payloads
		if(ret_val == 1)
		{
			return 1;
		}

		//We found a header! Can we find a footer in the right location?
		headers++;
		found_footer = 0;
		if(header_pos <= last_possible_header_index)
		{
			//How many bytes in this potential packed payload?
			ret_val = circ_buf_peek(cb, &bytes_in_packed_payload,
					header_pos + 1);
			if(!ret_val)
			{
				//Is there a footer?
				possible_footer_pos = header_pos + 3 + bytes_in_packed_payload;
				ret_val = circ_buf_peek(cb, &byte_peek, possible_footer_pos);
				if(!ret_val)
				{
					//We make sure that what we think is our footer is actually a footer
					found_footer = ((possible_footer_pos < cb_size)
							&& (byte_peek == FOOTER));
				}
			}
		}

		//Now that we found a packed payload, let's make sure it's valid
		if(found_footer)
		{
			footers++;
			ret_val = circ_buf_checksum(cb, &checksum, (header_pos + 2),
					(possible_footer_pos - 1));
			if(!ret_val)
			{
				//If checksum is valid than we found a valid string
				ret_val = circ_buf_peek(cb, &byte_peek,
						(possible_footer_pos - 1));
				if(!ret_val)
				{
					//We make sure that the two checksums match
					found_packed_payload = (checksum == byte_peek);
				}
			}
		}

		//Either we found a packed payload and it had a valid checksum, or we want to launch a new search
		last_header_pos = header_pos;
	}

	//A correct packaged payload was found in the circular buffer, and we can now extract it
	int numBytesInPackedString = 0;
	uint16_t i = 0;
	if(found_packed_payload)
	{
		numBytesInPackedString = header_pos + bytes_in_packed_payload + 4;

		//Our circular buffer is first in first out. If our header wasn't at index = 0 we need to dump some bytes to clear any build-up.
		if(header_pos)
		{
			uint8_t dump = 0;
			for(i = 0; i < header_pos; i++)
			{
				ret_val = circ_buf_read_byte(cb, &dump);
			}
		}

		//At this point our header is at index 0. We grab the following bytes and save them.
		for(i = 0; i < (bytes_in_packed_payload + 4); i++)
		{
			ret_val = circ_buf_read_byte(cb, &packed[i]);
		}

		//Final step, we remove any ESCAPE chars
		uint16_t k, skip = 0, unpacked_idx = 0;
		for(k = 0; k < bytes_in_packed_payload; k++)
		{
			uint16_t index = k + 2; //First value is header, next value is bytes, next value is first data
			if((packed[index] == ESCAPE) && (skip == 0))
			{
				skip = 1;
			}
			else
			{
				skip = 0;
				unpacked[unpacked_idx++] = packed[index];
			}
		}

		//Success, we are done! The user will be able to access the data in 'unpacked'
		return 0;
	}

	//We didn't extract what we wanted
	return 1;
}

#ifdef __cplusplus
}
#endif
