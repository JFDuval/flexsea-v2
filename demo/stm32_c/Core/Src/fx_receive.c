//****************************************************************************
// Include(s)
//****************************************************************************

#include <fx_receive.h>
#include "main.h"
#include "circ_buf.h"

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
	fx_register_rx_cmd_handler(FX_CMD_DEMO, &fx_rx_cmd_demo);
	fx_register_rx_cmd_handler(FX_CMD_STRESS_TEST, &fx_rx_cmd_stress_test);

	return FX_SUCCESS;
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
	if(cb.length > MIN_OVERHEAD)
	{
		//At this point our encoded command is in the circular buffer
		ret_val = fx_get_cmd_handler_from_bytestream(&cb, &cmd_6bits_out, &rw_out,
				buf, &buf_len);

		new_bytes -= buf_len; //ToDo this is probably useless

		//Call handler
		if(!ret_val)
		{
			ret_val_cmd = fx_call_rx_cmd_handler(cmd_6bits_out, rw_out, buf, buf_len);

			if(!ret_val_cmd)
			{
				//Reply if requested
				if((rw_out == CmdRead) || (rw_out == CmdReadWrite))
				{
					*reply_cmd = cmd_6bits_out;
					*send_reply = 1;
				}

				//Proceed with clean-up procedure
				//flush_useless_bytes();
				//cleanup_flag = 1;
				fx_cleanup(&cb);
			}
		}
	}
	else
	{
		//flush_useless_bytes();
		fx_cleanup(&cb);
	}

	/*
	if(cleanup_flag)
	{
		cleanup_flag = 0;
	}
	*/

	return FX_SUCCESS;	//Always true
}

//This is the default FlexSEA stack test command. Every other command needs to
//match this prototype.
uint8_t fx_rx_cmd_demo(uint8_t cmd_6bits, ReadWrite rw, uint8_t *buf,
		uint8_t len)
{
	//We check a few parameters
	if((cmd_6bits == FX_CMD_DEMO) && (rw == CmdReadWrite) && (len >= 1) &&
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

			//This is great, but are we also able to decode it manually? In some applications
			//our data won't be neatly packed in a structure.

			DemoStructure my_rx_demo_manual = {0};
			uint16_t index = 1;
			my_rx_demo_manual.var0_int8 = buf[index++];
			my_rx_demo_manual.var1_uint32 = REBUILD_UINT32(buf, &index);
			my_rx_demo_manual.var2_uint8 = buf[index++];
			my_rx_demo_manual.var3_int32 = (int32_t) REBUILD_UINT32(buf, &index);
			my_rx_demo_manual.var4_int8 = (int8_t) buf[index++];
			my_rx_demo_manual.var5_uint16 = REBUILD_UINT16(buf, &index);
			my_rx_demo_manual.var6_uint8 = buf[index++];
			my_rx_demo_manual.var7_int16 = (int16_t) REBUILD_UINT16(buf, &index);
			my_rx_demo_manual.var8_float = REBUILD_FLOAT(buf, &index);

			//This should be identical to what we send:
			if(!memcmp(&my_rx_demo_manual, &my_demo_structure, sizeof(my_rx_demo_manual)))
			{
				return FX_SUCCESS;
			}
			else
			{
				//Problem
				return FX_PROBLEM;
			}
		}
		else
		{
			//Problem
			return FX_PROBLEM;
		}
	}
	else
	{
		//Problem
		return FX_PROBLEM;
	}
}

//FlexSEA Stress Test Command
uint8_t fx_rx_cmd_stress_test(uint8_t cmd_6bits, ReadWrite rw, uint8_t *buf,
		uint8_t len)
{
	//We check a few parameters
	if((cmd_6bits == FX_CMD_STRESS_TEST) && (rw == CmdReadWrite) && (len >= 1) &&
			(cmd_6bits == CMD_GET_6BITS(buf[CMD_CODE_INDEX])))
	{
		//Valid

		//Decode serial data

		return FX_SUCCESS;
	}
	else
	{
		//Problem
		return FX_PROBLEM;
	}
}

//****************************************************************************
// Private Function(s)
//****************************************************************************
