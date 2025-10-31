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

//FlexSEA Communication String Prototype:
//=======================================
//[HEADER][# of BYTES][PAYLOAD (DATA)...][CHECKSUM][FOOTER]
//=> Number of bytes includes the ESCAPE bytes
//=> Checksum is done on the payload (data + ESCAPEs) and on the BYTES byte.
//=> Payload/data: string of bytes. It can be a command (see flexsea_command)
//   or raw data.

//This file is all about encoding and decoding (CODEC) the data you want to
//exchange between two systems. Encode your data using fx_encode(),
//send it on a serial bus, receive it and decode it with fx_decode().

//The data you encode can be anything you desire, but it is typically a
//FlexSEA Command Packet. See flexsea_command for more info.

//****************************************************************************
// Include(s)
//****************************************************************************

#include "flexsea.h"
#include <flexsea_codec.h>

//****************************************************************************
// Variable(s)
//****************************************************************************

//****************************************************************************
// Private Function Prototype(s):
//****************************************************************************

//****************************************************************************
// Public Function(s)
//****************************************************************************

//Takes a payload (raw data) and encodes it so it can be sent on the wire
//Packing means adding a header, a footer, escape chars (if needed), and a checksum
//'uint8_t *payload': the data you want to send, stored in an array
//'uint8_t payload_len': number of bytes in 'payload' (could be shorter than sizeof())
//'uint8_t *encoded payload': array used to return an encoded payload (this is your output)
//'uint8_t *encoded_payload_len': number of bytes in 'encoded_payload'
//'uint8_t max_packed_payload_len' maximum allowed length of 'encoded_payload'
//Returns 0 if it was able to encode it, 1 otherwise
uint8_t fx_encode(uint8_t *payload, uint8_t payload_len,
		uint8_t *encoded_payload, uint8_t *encoded_payload_len,
		uint8_t max_encoded_payload_len)
{
	uint16_t i = 0, escapes = 0, idx = 0, total_bytes = 0;
	uint8_t checksum = 0;

	//Fill encoded_payload with known values ('a'), up to 'max_encoded_payload_len'
	memset(encoded_payload, 0xAA, max_encoded_payload_len);

	//Fill encoded_payload with payload and add ESCAPE characters when necessary
	escapes = 0;
	idx = 2;
	for(i = 0; i < payload_len && idx < max_encoded_payload_len; i++)
	{
		if((payload[i] == HEADER) || (payload[i] == FOOTER)
				|| (payload[i] == ESCAPE))
		{
			escapes = escapes + 1;
			encoded_payload[idx] = ESCAPE;
			encoded_payload[idx + 1] = payload[i];
			checksum += encoded_payload[idx];
			checksum += encoded_payload[idx + 1];
			idx = idx + 1;
		}
		else
		{
			encoded_payload[idx] = payload[i];
			checksum += encoded_payload[idx];
		}
		idx++;
	}

	if((idx + 2) >= max_encoded_payload_len)
	{
		//Packaged payload too long, abort
		memset(encoded_payload, 0, max_encoded_payload_len);	//Clear string
		return 1;
	}

	total_bytes = payload_len + escapes;

	//String length?
	if(total_bytes >= max_encoded_payload_len)
	{
		//Too long, abort:
		memset(encoded_payload, 0, max_encoded_payload_len);	//Clear string
		return 1;
	}

	//Build comm_str:
	encoded_payload[0] = HEADER;
	encoded_payload[1] = total_bytes;
	encoded_payload[2 + total_bytes] = checksum;
	encoded_payload[3 + total_bytes] = FOOTER;

	//Return the length of the valid data
	*encoded_payload_len = (MIN_OVERHEAD + total_bytes);
	return 0;
}

//Takes data from the wire (stored in a circular buffer) and extracts the first valid payload
//'circ_buf_t *cb': circular buffer than contains the bytes received over a given physical interface
//'uint8_t *encoded': array that will store the extracted encoded payload.
//'uint8_t *encoded_len': length of the encoded array.
//'uint8_t *decoded': array that will store the extracted decoded payload (no header/footer/...)
//'uint8_t *decoded_len': length of the decoded array.

//ToDo: how do we handle a corrupted payload? Do we remove it from the buffer or not?

uint8_t fx_decode(circ_buf_t *cb, uint8_t *encoded, uint8_t *encoded_len,
		uint8_t *decoded, uint8_t *decoded_len)
{
	uint8_t ret_val = 0;
	uint16_t cb_size = 0;
	ret_val = circ_buf_get_size(cb, &cb_size);
	uint16_t found_encoded_payload = 0, found_footer = 0,
			possible_footer_pos = 0;
	uint16_t last_possible_header_index = cb_size - 4;
	uint16_t header_pos = 0, last_header_pos = 0;
	uint8_t first_time = 1;
	uint8_t checksum = 0;
	uint8_t bytes_in_encoded_payload = 0;
	uint8_t byte_peek = 0;

	*encoded_len = 0;
	*decoded_len = 0;

	//We look for an encoded payload, starting by searching for a header
	uint16_t headers = 0, footers = 0;
	while(!found_encoded_payload
			&& (last_header_pos < last_possible_header_index))
	{
		//We start from index 0 (first time), then from the last header position
		if(first_time)
		{
			ret_val = circ_buf_search(cb, &header_pos, HEADER, last_header_pos);
			first_time = 0;

			if(ret_val == 1)
			{
				//We just looked at every possible byte and didn't find anything, so
				//let's delete them to avoid looking at the same ones again and again
				uint8_t dump = 0;
				for(int j = 0; j < last_header_pos; j++)
				{
					ret_val = circ_buf_read_byte(cb, &dump);
				}

				return 1;
			}
		}
		else
		{
			ret_val = circ_buf_search(cb, &header_pos, HEADER,
					last_header_pos + 1);
		}

		//This means it's not the first time => we found at least one header => don't delete
		if(ret_val == 1)
		{
			return 1;
		}

		//We found a header! Can we find a footer in the right location?
		headers++;
		found_footer = 0;
		if(header_pos <= last_possible_header_index)
		{
			//How many bytes in this potential encoded payload?
			ret_val = circ_buf_peek(cb, &bytes_in_encoded_payload,
					header_pos + 1);
			if(!ret_val)
			{
				//Is there a footer?
				possible_footer_pos = header_pos + bytes_in_encoded_payload + MIN_OVERHEAD - 1;
				ret_val = circ_buf_peek(cb, &byte_peek, possible_footer_pos);
				if(!ret_val)
				{
					//We make sure that what we think is our footer is actually a footer
					found_footer = ((possible_footer_pos < cb_size)
							&& (byte_peek == FOOTER));
				}
			}
		}

		//Now that we found an encoded payload, let's make sure it's valid
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
					found_encoded_payload = (checksum == byte_peek);
				}
			}
		}

		//Either we found an encoded payload and it had a valid checksum, or we want to launch a new search
		last_header_pos = header_pos;
	}

	//A correct encoded payload was found in the circular buffer, and we can now extract it
	uint16_t i = 0;
	if(found_encoded_payload)
	{
		*encoded_len = bytes_in_encoded_payload + MIN_OVERHEAD;

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
		for(i = 0; i < (bytes_in_encoded_payload + MIN_OVERHEAD); i++)
		{
			ret_val = circ_buf_read_byte(cb, &encoded[i]);
		}

		//Final step, we remove any ESCAPE chars
		uint16_t k, skip = 0, decoded_idx = 0;
		for(k = 0; k < bytes_in_encoded_payload; k++)
		{
			uint16_t index = k + 2; //First value is header, next value is bytes, next value is first data
			if((encoded[index] == ESCAPE) && (skip == 0))
			{
				skip = 1;
			}
			else
			{
				skip = 0;
				decoded[decoded_idx++] = encoded[index];
			}
		}

		//Success, we are done! The user will be able to access the data in 'unpacked'
		*decoded_len = decoded_idx;
		return 0;
	}

	//We didn't extract what we wanted
	return 1;
}

//Anything that it's in the buffer and that's before a header is useless
//It could be padding, noise, etc. In any case, we don't want that.
uint8_t fx_cleanup(circ_buf_t *cb)
{
	uint16_t latch_byte_cnt = cb->length;
	uint8_t ret_val = 0, current_byte = 0;

	if(latch_byte_cnt)
	{
		//Bytes left in the buffer
		for(int i = 0; i < latch_byte_cnt; i++)
		{
			//Look at one byte in the buffer
			ret_val = circ_buf_peek(cb, &current_byte, i);
			//Is it a header?
			if(current_byte == HEADER)
			{
				//Flush what was before the header, and stop here
				uint8_t dump = 0;
				for(int j = 0; j < i; j++)
				{
					ret_val = circ_buf_read_byte(cb, &dump);
				}
				break;
			}
			//Did we scan the entire buffer?
			if(i == (latch_byte_cnt-1))
			{
				//Flush everything, get to cb_len = 0
				uint8_t dump = 0;
				for(int j = 0; j <= i; j++)
				{
					ret_val = circ_buf_read_byte(cb, &dump);
				}
				break;
			}
		}
	}

	return 0;	//Always OK
}

#ifdef __cplusplus
}
#endif
