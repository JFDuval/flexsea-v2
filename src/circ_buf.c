//****************************************************************************
// Include(s)
//****************************************************************************

#include "main.h"
#include "circ_buf.h"

//This code was adapted from https://github.com/charlesdobson/circular-buffer

//Return value convention: 0 is good, 1 means an error happened
//Any numerical data that needs to be returned is passed via pointers

//****************************************************************************
// Variable(s)
//****************************************************************************

//One circular buffer for the serial input
circ_buf_t circ_buf_serial_rx = {.buffer = {0}, .length = 0, .write_index = 0,
		.read_index = 0};

//****************************************************************************
// Private Function Prototype(s)
//****************************************************************************

//****************************************************************************
// Public Function(s)
//****************************************************************************

//Inits or re-inits a circular buffer
uint8_t circ_buf_init(circ_buf_t *cb)
{
	memset(cb->buffer, 0, CIRC_BUF_SIZE);
	cb->length = 0;
	cb->write_index = 0;
	cb->read_index = 0;

	return 0;
}

//Add a value to the circular buffer (single byte)
//Returns 0 if it's not full (normal operation)
//Returns 1 if the buffer is full (it will save the value by overwriting the
//oldest data)
uint8_t circ_buf_write_byte(circ_buf_t *cb, uint8_t new_value)
{
	uint8_t ret_val = 0;

	//Check if buffer is full
	if(cb->length >= CIRC_BUF_SIZE)
	{
		cb->length = CIRC_BUF_SIZE;
		ret_val = 1;
	}

	//Save new value to buffer
	cb->buffer[cb->write_index] = new_value;

	cb->length++;		//Increase buffer size after writing
	cb->write_index++;	//Increase write_index position to prepare for next write

	//If at last index in buffer, set write_index back to 0
	if(cb->write_index == CIRC_BUF_SIZE)
	{
		cb->write_index = 0;
	}

	return ret_val;
}

//Read a value from the circular buffer (single byte)
//Returns 0 if it's not empty, read_value is your data (normal operation)
//Returns 1 if the buffer is empty (read_value will be set to 0)
uint8_t circ_buf_read_byte(circ_buf_t *cb, uint8_t *read_value)
{
	//Check if buffer is empty
	if(cb->length == 0)
	{
		*read_value = 0;
		return 1;
	}

	//Return value
	*read_value = cb->buffer[cb->read_index];

	cb->length--;		//Decrease buffer size after reading
	cb->read_index++;	//Increase read_index position to prepare for next read

	//If at last index in buffer, set read_index back to 0
	if(cb->read_index == CIRC_BUF_SIZE)
	{
		cb->read_index = 0;
	}

	return 0;
}

//Look at a specific location, but do not remove the value from the buffer
//The location is an offset from the read pointer
//Returns 0 the offset is within the size (normal operation)
//Returns 1 if the buffer if the offset is outside the range of data available to read
uint8_t circ_buf_peek(circ_buf_t *cb, uint8_t *read_value, uint16_t offset)
{
	if(offset >= cb->length)
	{
		read_value = 0;
		return 1;
	}

	*read_value = cb->buffer[((cb->read_index + offset) % CIRC_BUF_SIZE)];
	return 0;
}

//Find the index of a given value
uint8_t circ_buf_search(circ_buf_t *cb, uint16_t *search_result, uint8_t value,
		uint16_t start_offset)
{
	if(start_offset >= cb->length)
	{
		//Invalid search
		*search_result = 0;
		return 1;
	}

	int i = 0; //Keeps track of how many values we looked at
	int index = cb->read_index + start_offset; //Keeps track of the index

	//Search from start offset to end of the linear buffer
	while((i < cb->length) && (index < CIRC_BUF_SIZE))
	{
		if(cb->buffer[index] == value)
		{
			//We found our value, we return
			*search_result = index;
			return 0;
		}
		i++;
		index++;
	}

	//Start from the beginning (aka "circularize" the buffer)
	index %= CIRC_BUF_SIZE;
	while(i < cb->length)
	{
		if(cb->buffer[index] == value)
		{
			//We found our value, we return
			*search_result = index;
			return 0;
		}
		i++;
		index++;
	}

	//Value not found
	*search_result = 0;
	return 1;
}

//Calculate a checksum for a given section, without removing the values
uint8_t circ_buf_checksum(circ_buf_t *cb, uint8_t *checksum, uint16_t start,
		uint16_t end)
{
	if((start >= cb->length) || (end > cb->length))
	{
		//Start or stop are out of the range
		checksum = 0;
		return 1;
	}

	if((end - start) < 1)
	{
		//We need some bytes if we want to calculate a checksum
		checksum = 0;
		return 1;
	}

	uint8_t temp_checksum = 0;

	int i = (cb->read_index + start); //No modulo on purpose as it would have no ultimate effect
	int j = (cb->read_index + end);

	//Calculate from start offset to end of the linear buffer
	while((i < j) && (i < CIRC_BUF_SIZE))
	{
		temp_checksum += cb->buffer[i++];
	}

	//Start from the beginning (aka "circularize" the buffer)
	i %= CIRC_BUF_SIZE;
	j %= CIRC_BUF_SIZE;
	while(i < j)
	{
		temp_checksum += cb->buffer[i++];
	}

	//Success
	*checksum = temp_checksum;
	return 0;
}

//Get the buffer size
uint8_t circ_buf_get_size(circ_buf_t *cb, uint16_t *cb_size)
{
	*cb_size = cb->length;
	return 0;
}
