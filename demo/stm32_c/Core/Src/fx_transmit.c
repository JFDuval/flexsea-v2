//****************************************************************************
// Include(s)
//****************************************************************************

#include <fx_transmit.h>
#include "main.h"

//****************************************************************************
// Variable(s)
//****************************************************************************

//UART transmission
uint8_t bytestream[MAX_ENCODED_PAYLOAD_BYTES] = {0};
uint8_t bytestream_len = 0;

DemoStructure my_demo_structure = {.var1_uint32 = 123456, .var2_uint8 = 150,
		.var3_int32 = -1234567, .var4_int8 = -125, .var5_uint16 = 4567,
		.var6_uint8 = 123, .var7_int16 = -4567, .var8_float = 12.37};

//****************************************************************************
// Private Function Prototype(s)
//****************************************************************************

uint8_t fx_tx_demo(void);
uint8_t fx_tx_sensor_status(void);

//****************************************************************************
// Public Function(s)
//****************************************************************************

uint8_t fx_transmit(uint8_t send_reply, uint8_t cmd_reply)
{
	if(send_reply)
	{
		//Note: we might use a function pointer array here, just like in the
		//reception

		switch(cmd_reply)
		{
			case 1:
				fx_tx_demo();
				break;
		}

		send_reply = 0;
	}

	return 0;
}

//This is the default FlexSEA stack test command.
uint8_t fx_tx_demo(void)
{
	uint8_t ret_val = 0;

	//Send known, fixed values
	uint8_t payload_len = sizeof(my_demo_structure);
	uint8_t* payload = (uint8_t*)&my_demo_structure;

	ret_val = fx_create_bytestream_from_cmd(1, CmdWrite, payload,
			  payload_len, bytestream, &bytestream_len);

	//CDC_Transmit_FS(bytestream, bytestream_len);
	HAL_UART_Transmit_IT(&huart2, &bytestream, bytestream_len);

	return 0;
}

//****************************************************************************
// Private Function(s)
//****************************************************************************
