//****************************************************************************
// Include(s)
//****************************************************************************

#include <comm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "flexsea.h"
#include "main.h"

//****************************************************************************
// Variable(s)
//****************************************************************************

//Temporary buffer for our UART reception
volatile uint8_t pc_rx_data[10] = {0};

//We prepare a new circular buffer for USB Serial
circ_buf_t cb = {.buffer = {0}, .length = 0, .write_index = 0,
		.read_index = 0};

//Communication Ports
CommPort comm_port[2];

//****************************************************************************
// Private Function Prototype(s)
//****************************************************************************


//****************************************************************************
// Public Function(s)
//****************************************************************************

void comm_init(void)
{
	//FlexSEA Comm Ports:
	//USB Serial
	comm_port[CP_USB].id = 0;
	comm_port[CP_USB].send_reply = 0;
	comm_port[CP_USB].reply_cmd = 0;
	comm_port[CP_USB].cb = &cb;
	comm_port[CP_USB].tx_fct_prt = &usb_serial_tx_string;
	comm_port[CP_USB].use_dbuf = 0;
	memset(comm_port[CP_USB].dbuf[0], 0x00, DBUF_MAX_LEN);
	memset(comm_port[CP_USB].dbuf[1], 0x00, DBUF_MAX_LEN);
	comm_port[CP_USB].dbuf_lock[0] = 0;
	comm_port[CP_USB].dbuf_lock[1] = 0;
	comm_port[CP_USB].dbuf_len[0] = 0;
	comm_port[CP_USB].dbuf_len[1] = 0;
	comm_port[CP_USB].dbuf_selected = 0;
}

//Send a string
uint8_t usb_serial_tx_string(uint8_t *bytes_to_send, uint16_t length)
{
	HAL_UART_Transmit_IT(&huart2, bytes_to_send, length);
	return 0;
}

void usb_serial_rx(void)
{
	HAL_UART_Receive_IT(&huart2, &pc_rx_data, 1);
}

//****************************************************************************
// Private Function(s)
//****************************************************************************

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if(huart->Instance == USART2)
	{
		//Minimalist code. We deal with one byte at a time (let's hope
		//we service this ISR quickly!) and we feed it to the circular
		//buffer.
		circ_buf_write_byte(&cb, pc_rx_data[0]);
		HAL_UART_Receive_IT(&huart2, &pc_rx_data, 1);
	}
}
