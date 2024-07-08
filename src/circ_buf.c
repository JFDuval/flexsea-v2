//****************************************************************************
// Include(s)
//****************************************************************************

#include "main.h"
#include "circ_buf.h"

//This code was adapted from https://github.com/charlesdobson/circular-buffer

//****************************************************************************
// Variable(s)
//****************************************************************************

//One circular buffer for the serial input
circ_buf_t circ_buf_serial_rx = {.buffer = {0},        \
                                .length = 0,          \
                                .write_index = 0,     \
                                .read_index = 0};

//****************************************************************************
// Private Function Prototype(s)
//****************************************************************************

//****************************************************************************
// Public Function(s)
//****************************************************************************

//Add a value to the circular buffer (single byte)
//Returns 0 if it's not full (normal operation)
//Returns 1 if the buffer is full (it will save the value by overwriting the
//oldest data)
uint8_t circ_buf_write(circ_buf_t *cb, uint8_t new_value)
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

    cb->length++;       //Increase buffer size after writing
    cb->write_index++;  //Increase write_index position to prepare for next write

    //If at last index in buffer, set writeIndex back to 0
    if(cb->write_index == CIRC_BUF_SIZE)
    {
        cb->write_index = 0;
    }

    return ret_val;
}

//Read a value from the circular buffer (single byte)
//Returns 0 if it's not empty, read_value is your data (normal operation)
//Returns 1 if the buffer is empty (read_value will be set to 0)
uint8_t circ_buf_read(circ_buf_t *cb, uint8_t* read_value)
{
    //Check if buffer is empty
    if(cb->length == 0)
    {
        *read_value = 0;
        return 1;
    }

    //Return value
    *read_value = cb->buffer[cb->read_index];

    cb->length--;       //Decrease buffer size after reading
    cb->read_index++;   //Increase read_index position to prepare for next read

    //If at last index in buffer, set read_index back to 0
    if(cb->read_index == CIRC_BUF_SIZE)
    {
        cb->read_index = 0;
    }

    return 0;
}

//Get the buffer size
uint16_t circ_buf_size(circ_buf_t *cb)
{
    return cb->length;
}

//Get average of all values
int16_t circ_buf_average(circ_buf_t *cb)
{
    int32_t sum = 0, average = 0;
    for(int i = 0; i < CIRC_BUF_SIZE; i++)
    {
        sum += cb->buffer[i];
    }
    average = sum / CIRC_BUF_SIZE;

    return (int16_t) average;
}

//ToDo:

/*

//Initialize circular buffer
void circ_buff_init(circularBuffer_t* cb)

//Look at a specific location, but do not remove the value from the buffer
uint8_t circ_buff_peek(circularBuffer_t* cb, uint16_t offset)

//Find the index of a given value
int32_t circ_buff_search(circularBuffer_t* cb, uint8_t value, uint16_t start)

uint8_t circ_buff_checksum(circularBuffer_t* cb, uint16_t start, uint16_t end)

int circ_buff_read_section(circularBuffer_t* cb, uint8_t* readInto, uint16_t start, uint16_t numBytes)

int circ_buff_get_size(circularBuffer_t* cb){return cb->size;}
int circ_buff_get_space(circularBuffer_t* cb){return (CB_BUF_LEN - cb->size);}

*/
