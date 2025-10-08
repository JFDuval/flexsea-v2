#include "main.h"

//We use this main to test the communication API and functionality outside of the test framework.
//If this works, we know we can integrate the communication stack into any C project.
//This should always use the highest level API available; the goal isn't to duplicate other
//unit tests, but to do a quick integration test on the full system.

//Test structure
typedef struct
{
	uint8_t field1;
	int16_t value1;
	uint8_t some_data_array[10];
	int32_t large_value;
	uint8_t another_array[6];
} __attribute__((__packed__)) FlexSEA_Main_Test_Command_s;

//This is a FlexSEA test command
uint8_t test_command_1a(uint8_t cmd_6bits, ReadWrite rw, AckNack ack, uint8_t *buf, uint8_t len)
{
	//We check a few parameters
	if((cmd_6bits == 1) && (rw == CmdWrite) && (len >= 1) &&
			(cmd_6bits == CMD_GET_6BITS(buf[CMD_CODE_INDEX])))
	{
		//Valid
		printf("If you see this, test_command_1a() has been successfully called!\n\n");
		return TEST_CMD_RETURN;
	}
	else
	{
		//Problem
		return 1;
	}
}

int main()
{
	int i = 0;

	printf("Hello world! This is a FlexSEA Comm v2.0 test!\n\n");

	//We create a structure
	FlexSEA_Main_Test_Command_s fx_test_struct1;
	fx_test_struct1.field1 = 123;
	fx_test_struct1.value1 = -1234;
	memset(fx_test_struct1.some_data_array, 0, 9);
	fx_test_struct1.some_data_array[0] = 237;
	fx_test_struct1.some_data_array[8] = 238;
	fx_test_struct1.large_value = 100000;
	memset(fx_test_struct1.another_array, 0, 6);
	fx_test_struct1.another_array[0] = 100;
	fx_test_struct1.another_array[5] = 200;
	uint8_t payload_in_len = sizeof(fx_test_struct1);
	uint8_t* payload_in = (uint8_t*)&fx_test_struct1;

	uint8_t bytestream[MAX_ENCODED_PAYLOAD_BYTES] = {0};
	uint8_t bytestream_len = 0;
	uint8_t cmd_6bits_in = 1;
	uint8_t ret_val = 0, ret_val_cmd = 0;
	ReadWrite rw = CmdWrite;
	AckNack ack = Nack;

	//Init stack & register test function:
	fx_rx_cmd_init();
	fx_register_rx_cmd_handler(cmd_6bits_in, &test_command_1a);

	ret_val = fx_create_bytestream_from_cmd(cmd_6bits_in, rw, ack, payload_in,
			payload_in_len, bytestream, &bytestream_len);

	//We prepare a new circular buffer
	circ_buf_t cb = {.buffer = {0}, .length = 0, .write_index = 0, .read_index =
			0};

	//Our payload makes it into the circular buffer
	ret_val = 0;
	for(i = 0; i < bytestream_len; i++)
	{
		//Write to circular buffer
		ret_val = circ_buf_write_byte(&cb, bytestream[i]);

		//circ_buf_write() should always return 0 if we are not overwriting
		if(ret_val){break;}
	}

	//At this point our encoded command is in the circular buffer
	uint8_t cmd_6bits_out = 0;
	ReadWrite rw_out = CmdInvalid;
	AckNack ack_out = Nack;
	uint8_t buf[MAX_ENCODED_PAYLOAD_BYTES] = {0};
	uint8_t buf_len = 0;
	ret_val = fx_get_cmd_handler_from_bytestream(&cb, &cmd_6bits_out, &rw_out,
			&ack_out, buf, &buf_len);

	//Call handler
	if(!ret_val)
	{
		ret_val_cmd = fx_call_rx_cmd_handler(cmd_6bits_out, rw_out, ack_out, buf, buf_len);
		if(ret_val_cmd == TEST_CMD_RETURN)
		{
			printf("Success!\n");
		}
		else
		{
			printf("Our return value doesn't match with the expected test function.\n");
		}
	}
	else
	{
		printf("payload_parse_str() did not return 0, it detected an invalid command.\n");
	}

	return 0;
}
