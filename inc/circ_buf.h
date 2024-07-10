#ifndef INC_CIRC_BUF_H_
#define INC_CIRC_BUF_H_

//****************************************************************************
// Include(s)
//****************************************************************************

#include "main.h"

//****************************************************************************
// Definition(s):
//****************************************************************************

//Number of uint8_t we can hold in a circular buffer
#define CIRC_BUF_SIZE       1000

//This structure holds all the info about a given circular buffer
typedef struct circ_buf
{
	volatile uint8_t buffer[CIRC_BUF_SIZE];	//Empty circular buffer
	volatile uint16_t read_index;			//Index of the read pointer
	volatile uint16_t write_index;			//Index of the write pointer
	volatile uint16_t length;			//Number of values in circular buffer
}circ_buf_t;

//****************************************************************************
// Public Function Prototype(s):
//****************************************************************************

uint8_t circ_buf_init(circ_buf_t *cb);
uint8_t circ_buf_write_byte(circ_buf_t *cb, uint8_t new_value);
uint8_t circ_buf_read_byte(circ_buf_t *cb, uint8_t *read_value);
uint8_t circ_buf_peek(circ_buf_t *cb, uint8_t *read_value, uint16_t offset);
uint8_t circ_buf_search(circ_buf_t *cb, uint16_t *search_result, uint8_t value,
		uint16_t start_offset);
uint8_t circ_buf_checksum(circ_buf_t *cb, uint8_t *checksum, uint16_t start,
		uint16_t end);
uint16_t circ_buf_get_size(circ_buf_t *cb);

//****************************************************************************
// Shared variable(s)
//****************************************************************************

extern circ_buf_t circ_buf_serial_rx;

#endif // INC_CIRC_BUF_H_
