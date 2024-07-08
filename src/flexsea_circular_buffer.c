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
	[This file] flexsea_circular_buffer.c: simple circular buffer implementation
*****************************************************************************
	[Change log] (Convention: YYYY-MM-DD | author | comment)
	* 2017-03-21 | dudds4 | Initial GPL-3.0 release
****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#include "flexsea_circular_buffer.h"
#include <string.h>

//Initialize circular buffer
void circ_buff_init(circularBuffer_t* cb)
{
	cb->head = -1;
	cb->tail = 0;
	cb->size = 0;

	//FIll with zeros
	int i = 0;
	uint8_t* b = cb->bytes;
	for(i = 0; i < CB_BUF_LEN; i++)
	{
		b[i] = 0;
	}
}

//Write new data to circular buffer
int circ_buff_write(circularBuffer_t* cb, uint8_t *writeFrom, uint16_t numBytes)
{
    const int MSG_BIGGER_THAN_BUFFER = 1;
	const int OVERWROTE = 2;
    const int SUCCESS = 0;

	if(numBytes > CB_BUF_LEN){return MSG_BIGGER_THAN_BUFFER;}

	if((cb->tail + numBytes) <= CB_BUF_LEN)
	{
		memcpy((cb->bytes + cb->tail), writeFrom, numBytes);
	}
	else
	{
		uint16_t bytesUntilEnd = CB_BUF_LEN - cb->tail;
		memcpy(cb->bytes + cb->tail, writeFrom, bytesUntilEnd);
		memcpy(cb->bytes, writeFrom + bytesUntilEnd, numBytes - bytesUntilEnd);
	}

	if(cb->head < 0){cb->head = 0;}
	cb->tail = (cb->tail + numBytes) % CB_BUF_LEN;
	cb->size += numBytes;

	if(cb->size > CB_BUF_LEN)
	{
		//CB has been overwritten
		cb->size = CB_BUF_LEN;
		cb->head = cb->tail;
		return OVERWROTE;
	}
	return SUCCESS;
}

//Look at a specific location, but do not remove the value from the buffer
uint8_t circ_buff_peek(circularBuffer_t* cb, uint16_t offset)
{
    if(offset >= cb->size){return 0;}
    return cb->bytes[((cb->head + offset) % CB_BUF_LEN)];
}

//Find the index of a given value
int32_t circ_buff_search(circularBuffer_t* cb, uint8_t value, uint16_t start)
{
    if(start >= cb->size){return -1;}
    int i = start;
    int index = cb->head + start;

    while((i < cb->size) && (index < CB_BUF_LEN))
    {
        if(cb->bytes[index] == value) return i;
        i++;
        index++;
    }

    index %= CB_BUF_LEN;

    while(i < cb->size)
    {
        if(cb->bytes[index] == value){return i;}
        i++;
        index++;
    }

    return -1;
}

/* Not used
int32_t circ_buff_search_not(circularBuffer_t* cb, uint8_t value, uint16_t start)
{
    if(start > cb->size){return -1;}

    uint16_t i = start;

    uint16_t idx = (cb->head + start);
    while((idx < CB_BUF_LEN) && (i < cb->size) && (cb->bytes[idx] == value))
    {
        idx++;
        i++;
    }

    idx = idx % CB_BUF_LEN;
    while((idx < CB_BUF_LEN) && (i < cb->size) && (cb->bytes[idx] == value))
    {
        idx++;
        i++;
    }

    return i;
}
*/

uint8_t circ_buff_checksum(circularBuffer_t* cb, uint16_t start, uint16_t end)
{
    if((start >= cb->size) || (end > cb->size)){return 0;}
	if(end - start < 1){return 0;}

    uint8_t checksum = 0;

    int i = (cb->head + start); //no modulo on purpose. (it would have no ultimate effect)
	int j = (cb->head + end);

	while((i < j) && (i < CB_BUF_LEN))
	{
		checksum += cb->bytes[i++];
	}

    i %= CB_BUF_LEN;
	j %= CB_BUF_LEN;

    while(i < j)
    {
        checksum += cb->bytes[i++];
    }

    return checksum;
}

int circ_buff_read(circularBuffer_t* cb, uint8_t* readInto, uint16_t numBytes)
{
    const int SUCCESS = 0;
    const int NOT_ENOUGH_BUFFERED = 1;

    if(numBytes > cb->size){return NOT_ENOUGH_BUFFERED;}

    if((cb->head + numBytes) < CB_BUF_LEN)
    {
    	//Copy a continuous stretch of data to the readInto array
        memcpy(readInto, cb->bytes + cb->head, numBytes);
    }
    else
    {
    	//Copy data while handling the discontinuity at the end of the circular buffer
		uint16_t bytesUntilEnd = CB_BUF_LEN - cb->head;
        memcpy(readInto, (cb->bytes + cb->head), bytesUntilEnd);
        memcpy((readInto + bytesUntilEnd), cb->bytes, (numBytes - bytesUntilEnd));
    }

    //ToDo recompute size?

    return SUCCESS;
}

int circ_buff_read_section(circularBuffer_t* cb, uint8_t* readInto, uint16_t start, uint16_t numBytes)
{
    const int SUCCESS = 0;
    const int INVALID_ARGS = 1;

	if(!cb || !readInto || ((start + numBytes) > cb->size)){return INVALID_ARGS;}

	uint16_t s = (cb->head + start) % CB_BUF_LEN;
	if((s + numBytes) > CB_BUF_LEN)
	{
		uint16_t bytesUntilEnd = CB_BUF_LEN - s;
		memcpy(readInto, (cb->bytes + s), bytesUntilEnd);
		memcpy((readInto + bytesUntilEnd), cb->bytes, (numBytes - bytesUntilEnd));
	}
	else
	{
		memcpy(readInto, (cb->bytes + s), numBytes);
	}
	return SUCCESS;
}

//That seems wrong... ToDo double check
int circ_buff_move_head(circularBuffer_t* cb, uint16_t numBytes)
{
    const int SUCCESS = 0;
    const int MOVED_MORE_THAN_BUFFERED = 1;
	const int MOVED_MORE_THAN_MAX = 2;

	int result = SUCCESS;
	if(numBytes > CB_BUF_LEN)
	{
		result = MOVED_MORE_THAN_MAX;
	}
	else if(numBytes > cb->size)
	{
		result = MOVED_MORE_THAN_BUFFERED;
	}

	numBytes = (numBytes < CB_BUF_LEN) ? numBytes : CB_BUF_LEN;

	if(cb->head < 0) //buffer is already empty
	{
		return result;
	}

	if((cb->head + numBytes) > CB_BUF_LEN)
	{
		uint16_t bytesUntilEnd = CB_BUF_LEN - cb->head;
		memset((cb->bytes + cb->head), 0, bytesUntilEnd);
		memset(cb->bytes, 0, (numBytes - bytesUntilEnd));
	}
	else
	{
		memset((cb->bytes + cb->head), 0, numBytes);
	}

	cb->size -= numBytes;
	if(cb->size < 1)
	{
		cb->head = -1;
		cb->tail = 0;
		cb->size = 0;
	}
	else
	{
		cb->head = (cb->head + numBytes) % CB_BUF_LEN;
	}

	return result;
}

int circ_buff_get_size(circularBuffer_t* cb){return cb->size;}
int circ_buff_get_space(circularBuffer_t* cb){return (CB_BUF_LEN - cb->size);}

#ifdef __cplusplus
}
#endif
