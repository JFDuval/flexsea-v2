#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "tests.h"
#include "flexsea_command.h"

//Make sure we can code/decode the 6-bit command and 2-bit RW in one byte when they are valid
//Commands are created manually (not using tx_cmd)
void test_command_cmd_rw_byte_valid(void)
{
	uint8_t payload[10] = "\0payload"; //The first char will be replaced by our cmd/rw code
	uint16_t payload_len = sizeof(payload);

	//Test #1: Write
	uint8_t cmd_6bits_in = 22;
	uint8_t cmd_8bits = CMD_SET_W(cmd_6bits_in); //Write
	TEST_ASSERT_EQUAL(CMD_GET_6BITS(cmd_8bits), cmd_6bits_in);	//Quick macro test
	payload[CMD_CODE_INDEX] = cmd_8bits;
	uint8_t ret_val = 0, cmd_6bits_out = 0;
	ReadWrite rw = CmdInvalid;
	ret_val = payload_parse_str(payload, payload_len, &cmd_6bits_out, &rw);
	TEST_ASSERT_EQUAL(0, ret_val);	//ret_val = 0 when valid
	TEST_ASSERT_EQUAL(cmd_6bits_in, cmd_6bits_out);
	TEST_ASSERT_EQUAL(CmdWrite, rw);

	//Test #2: Read
	cmd_6bits_in = 33;
	cmd_8bits = CMD_SET_R(cmd_6bits_in); //Read
	TEST_ASSERT_EQUAL(CMD_GET_6BITS(cmd_8bits), cmd_6bits_in);	//Quick macro test
	payload[CMD_CODE_INDEX] = cmd_8bits;
	ret_val = 0;
	cmd_6bits_out = 0;
	rw = CmdInvalid;
	ret_val = payload_parse_str(payload, payload_len, &cmd_6bits_out, &rw);
	TEST_ASSERT_EQUAL(0, ret_val);	//ret_val = 0 when valid
	TEST_ASSERT_EQUAL(cmd_6bits_in, cmd_6bits_out);
	TEST_ASSERT_EQUAL(CmdRead, rw);

	//Test #3: Read Write
	cmd_6bits_in = 44;
	cmd_8bits = CMD_SET_RW(cmd_6bits_in); //Read Write
	TEST_ASSERT_EQUAL(CMD_GET_6BITS(cmd_8bits), cmd_6bits_in);	//Quick macro test
	payload[CMD_CODE_INDEX] = cmd_8bits;
	ret_val = 0;
	cmd_6bits_out = 0;
	rw = CmdInvalid;
	ret_val = payload_parse_str(payload, payload_len, &cmd_6bits_out, &rw);
	TEST_ASSERT_EQUAL(0, ret_val);	//ret_val = 0 when valid
	TEST_ASSERT_EQUAL(cmd_6bits_in, cmd_6bits_out);
	TEST_ASSERT_EQUAL(CmdReadWrite, rw);
}

//Make sure we can detect errors when coding/decoding the 6-bit command and 2-bit RW in one byte
//Commands are created manually (not using tx_cmd)
void test_command_cmd_rw_byte_invalid(void)
{
	uint8_t payload[10] = "\0payload"; //The first char will be replaced by our cmd/rw code
	uint16_t payload_len = sizeof(payload);

	//Test #1: command code of 0 (smaller than MIN_CMD_CODE)
	uint8_t cmd_6bits_in = 0;
	uint8_t cmd_8bits = CMD_SET_W(cmd_6bits_in); //Write
	payload[CMD_CODE_INDEX] = cmd_8bits;
	uint8_t ret_val = 0, cmd_6bits_out = 0;
	ReadWrite rw = CmdInvalid;
	ret_val = payload_parse_str(payload, payload_len, &cmd_6bits_out, &rw);
	TEST_ASSERT_EQUAL(1, ret_val);	//1 means it detected the problem
	TEST_ASSERT_EQUAL(0, cmd_6bits_out); //Invalid returns 0
	TEST_ASSERT_EQUAL(0, rw); //Invalid returns 0

	//Test #2: RW = CmdInvalid
	cmd_6bits_in = MAX_CMD_CODE;
	cmd_8bits = ((cmd_6bits_in << 2) & 0xFC);	//Force the 2 LSB to 0
	payload[CMD_CODE_INDEX] = cmd_8bits;
	ret_val = 0, cmd_6bits_out = 0;
	rw = CmdInvalid;
	ret_val = payload_parse_str(payload, payload_len, &cmd_6bits_out, &rw);
	TEST_ASSERT_EQUAL(1, ret_val);	//1 means it detected the problem
	TEST_ASSERT_EQUAL(0, cmd_6bits_out); //Invalid returns 0
	TEST_ASSERT_EQUAL(0, rw); //Invalid returns 0

	//Note: testing a command code above MAX_CMD_CODE doesn't work, as the bits just
	//get shifted (ex.: 64 becomes 0, 65 becomes 1). That check will be done when
	//transmitting, not here at reception.
}

//We pass a valid command code and RW to the function. Is it happy?
void test_command_tx_cmd_basic_good_cmd_rw(void)
{
	uint8_t payload_in[10] = "payload";
	uint16_t payload_in_len = 7;	//Just the chars we want
	uint8_t payload_out[10] = {0};
	uint16_t payload_out_len = 0;
	uint8_t cmd_6bits_in = 33;
	uint8_t ret_val = 0;
	ReadWrite rw = CmdWrite;

	ret_val = tx_cmd(cmd_6bits_in, rw, payload_in, payload_in_len,
			payload_out, &payload_out_len);
	TEST_ASSERT_EQUAL(0, ret_val);
	TEST_ASSERT_EQUAL(payload_in_len + 1, payload_out_len);
	TEST_ASSERT_EQUAL(CMD_SET_W(cmd_6bits_in), payload_out[CMD_CODE_INDEX]);
}

//We pass invalid command codes and RW to the function. Does it notice?
void test_command_tx_cmd_basic_bad_cmd_rw(void)
{
	//Test #1: command code too low
	uint8_t payload_in[10] = "payload";
	uint16_t payload_in_len = 7;	//Just the chars we want
	uint8_t payload_out[10] = {0};
	uint16_t payload_out_len = 0;
	uint8_t cmd_6bits_in = 0;
	uint8_t ret_val = 0;
	ReadWrite rw = CmdWrite;

	ret_val = tx_cmd(cmd_6bits_in, rw, payload_in, payload_in_len,
			payload_out, &payload_out_len);
	TEST_ASSERT_EQUAL(1, ret_val);
	TEST_ASSERT_EQUAL(0, payload_out_len);
	TEST_ASSERT_EQUAL(0, payload_out[CMD_CODE_INDEX]);

	//Test #2: command code too high
	payload_in_len = 7;
	memcpy(payload_in, "payload", payload_in_len);
	memset(payload_out, 0, 10);
	payload_out_len = 0;
	cmd_6bits_in = MAX_CMD_CODE + 1;
	ret_val = 0;
	rw = CmdWrite;

	ret_val = tx_cmd(cmd_6bits_in, rw, payload_in, payload_in_len,
			payload_out, &payload_out_len);
	TEST_ASSERT_EQUAL(1, ret_val);
	TEST_ASSERT_EQUAL(0, payload_out_len);
	TEST_ASSERT_EQUAL(0, payload_out[CMD_CODE_INDEX]);

	//Test #3: valid command code but invalid RW
	payload_in_len = 7;
	memcpy(payload_in, "payload", payload_in_len);
	memset(payload_out, 0, 10);
	payload_out_len = 0;
	cmd_6bits_in = MAX_CMD_CODE - 1;
	ret_val = 0;
	rw = CmdInvalid;

	ret_val = tx_cmd(cmd_6bits_in, rw, payload_in, payload_in_len,
			payload_out, &payload_out_len);
	TEST_ASSERT_EQUAL(1, ret_val);
	TEST_ASSERT_EQUAL(0, payload_out_len);
	TEST_ASSERT_EQUAL(0, payload_out[CMD_CODE_INDEX]);
}

//Can the parse command identify a valid string? Send it to the catch-all?
void test_command_parse_catchall(void)
{
	init_flexsea_payload_ptr();

	uint8_t payload[10] = "payload"; //The first char will be replaced by our cmd/rw code
	uint16_t payload_len = sizeof(payload);
	uint8_t payload_out[10] = {0};
	uint16_t payload_out_len = 0;
	ReadWrite rw = CmdWrite;
	uint8_t ret_val = 0;
	uint8_t cmd = 22;

	ret_val = tx_cmd(cmd, rw, payload, payload_len,
				payload_out, &payload_out_len);

	uint8_t cmd_6bits = 0;
	rw = CmdInvalid;
	uint8_t ret_val_cmd = 0;
	ret_val = payload_parse_str(payload_out, payload_out_len, &cmd_6bits, &rw);
	if(!ret_val)
	{
		ret_val_cmd = flexsea_payload(cmd_6bits, rw, payload_out, payload_out_len);
		if(ret_val_cmd == CATCHALL_RETURN)
		{
			TEST_PASS(); //As expected, we reached our catch-all
		}
		else
		{
			TEST_FAIL_MESSAGE("Our return value doesn't match with the expected catch-all.");
		}
	}
	else
	{
		TEST_FAIL_MESSAGE("payload_parse_str() did not return 0, it detected an invalid command.");
	}
}

//This is a FlexSEA test command
uint8_t test_command_22a(uint8_t cmd_6bits, ReadWrite rw, uint8_t *buf, uint16_t len)
{
	//We check a few parameters
	if((cmd_6bits == 22) && (rw == CmdWrite) && (len >= 1) &&
			(cmd_6bits == CMD_GET_6BITS(buf[CMD_CODE_INDEX])))
	{
			//Valid
			return TEST_CMD_RETURN;
	}
	else
	{
		//Problem
		return 1;
	}
}

//Can the parse command identify a valid string? Send it to the registered command?
void test_command_parse_registered_command(void)
{
	init_flexsea_payload_ptr();

	uint8_t payload[10] = "\0payload"; //The first char will be replaced by our cmd/rw code
	uint16_t payload_len = sizeof(payload);

	uint8_t cmd = 22;
	payload[CMD_CODE_INDEX] = CMD_SET_W(cmd);	//Write
	uint8_t ret_val = 0, cmd_6bits = 0;
	ReadWrite rw = CmdInvalid;
	uint8_t ret_val_cmd = 0;

	//Register test function:
	register_command(cmd, &test_command_22a);

	ret_val = payload_parse_str(payload, payload_len, &cmd_6bits, &rw);
	if(!ret_val)
	{
		ret_val_cmd = flexsea_payload(cmd_6bits, rw, payload, payload_len);
		if(ret_val_cmd == TEST_CMD_RETURN)
		{
			TEST_PASS(); //As expected, we reached our test function
		}
		else
		{
			TEST_FAIL_MESSAGE("Our return value doesn't match with the expected test function.");
		}
	}
	else
	{
		TEST_FAIL_MESSAGE("payload_parse_str() did not return 0, it detected an invalid command.");
	}
}

void test_flexsea_command(void)
{
	RUN_TEST(test_command_cmd_rw_byte_valid);
	RUN_TEST(test_command_cmd_rw_byte_invalid);
	RUN_TEST(test_command_tx_cmd_basic_good_cmd_rw);
	RUN_TEST(test_command_tx_cmd_basic_bad_cmd_rw);
	RUN_TEST(test_command_parse_catchall);
	RUN_TEST(test_command_parse_registered_command);

	fflush(stdout);
}

#ifdef __cplusplus
}
#endif
