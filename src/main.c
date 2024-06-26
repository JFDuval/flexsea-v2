#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "flexsea_comm_def.h"
#include <flexsea_circular_buffer.h>

//Takes payload, adds ESCAPES, checksum, header, ...
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
		if ((payload[i] == HEADER) || (payload[i] == FOOTER) || (payload[i] == ESCAPE))
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
		//LOG(lwarning,"Comm string too long, abort");
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
			bytes = circ_buff_peak(cb, headerPos + 1);
			possibleFooterPos = headerPos + 3 + bytes;
			foundFrame = (possibleFooterPos < bufSize && circ_buff_peak(cb, possibleFooterPos) == FOOTER);
		}

		if(foundFrame)
		{
			//LOG(ldebug2,"Frame found");
			footers++;
			checksum = circ_buff_checksum(cb, headerPos+2, possibleFooterPos-1);

			//if checksum is valid than we found a valid string
			foundString = (checksum == circ_buff_peak(cb, possibleFooterPos-1));
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


int main()
{
    printf("Hello world!\n");

    uint8_t my_test_payload[] = "jfduval";
    uint8_t my_test_packed_payload[48] = {0};

    printf("Original payload: %s\n", my_test_payload);

    uint8_t retval = comm_gen_str(my_test_payload, my_test_packed_payload, 7);
    printf("retval = %i\n", retval);
    printf("Packaged payload: %s\r\n", my_test_packed_payload);

    //Feed this into a circular buffer

    //Call unpack_payload_cb(), make sure to get the string back

    return 0;
}
