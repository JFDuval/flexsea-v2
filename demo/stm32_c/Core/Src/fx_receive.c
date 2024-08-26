//****************************************************************************
// Include(s)
//****************************************************************************

#include <fx_receive.h>
#include "main.h"

//****************************************************************************
// Variable(s)
//****************************************************************************

//****************************************************************************
// Private Function Prototype(s)
//****************************************************************************


//****************************************************************************
// Public Function(s)
//****************************************************************************

//Strong redefinition of flexsea_command.c/fx_register_rx_cmd_handlers()
//Register all your reception handlers here
uint8_t fx_register_rx_cmd_handlers(void)
{
	fx_register_rx_cmd_handler(1, &fx_rx_cmd_demo);

	return 0;
}

uint8_t fx_receive(uint8_t *send_reply, uint8_t *reply_cmd)
{
	uint8_t cmd_6bits_out = 0;
	ReadWrite rw_out = CmdInvalid;
	uint8_t buf[MAX_ENCODED_PAYLOAD_BYTES] = {0};
	uint8_t buf_len = 0;
	uint8_t ret_val = 0, ret_val_cmd = 0;
	*send_reply = 0;

	//Receive commands
	if(new_bytes)
	{
		//At this point our encoded command is in the circular buffer
		ret_val = fx_get_cmd_handler_from_bytestream(&cb, &cmd_6bits_out, &rw_out,
				buf, &buf_len);

		//Call handler
		if(!ret_val)
		{
			ret_val_cmd = fx_call_rx_cmd_handler(cmd_6bits_out, rw_out, buf, buf_len);

			//Reply if requested
			if((rw_out == CmdRead) || (rw_out == CmdReadWrite))
			{
				*reply_cmd = cmd_6bits_out;
				*send_reply = 1;
			}
		}
	}

	return 0;
}

//This is the default FlexSEA stack test command. Every other command needs to
//match this prototype.
uint8_t fx_rx_cmd_demo(uint8_t cmd_6bits, ReadWrite rw, uint8_t *buf,
		uint8_t len)
{
	//We check a few parameters
	if((cmd_6bits == 1) && (rw == CmdReadWrite) && (len >= 1) &&
			(cmd_6bits == CMD_GET_6BITS(buf[CMD_CODE_INDEX])))
	{
		//Valid
		printf("If you see this, fx_rx_cmd_demo() has been successfully "
				"called!\n\n");

		//Decode serial data
		DemoStructure my_rx_demo_structure = {0};
		memcpy(&my_rx_demo_structure, &buf[1], sizeof(my_rx_demo_structure));
		//This should be identical to what we send:
		if(!memcmp(&my_rx_demo_structure, &my_demo_structure, sizeof(my_rx_demo_structure)))
		{
			printf("Identical structures, perfect!\n");
			return TEST_CMD_RETURN;
		}
		else
		{
			//Problem
			return 1;
		}
	}
	else
	{
		//Problem
		return 1;
	}
}

//****************************************************************************
// Private Function(s)
//****************************************************************************
