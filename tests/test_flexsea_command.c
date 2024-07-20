#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "tests.h"
#include "flexsea_command.h"

//Make sure we can code/decode the 6-bit command and 2-bit RW in one byte when they are valid
void test_command_cmd_rw_byte_valid(void)
{
	init_flexsea_payload_ptr();

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
void test_command_cmd_rw_byte_invalid(void)
{
	init_flexsea_payload_ptr();

	uint8_t payload[10] = "\0payload"; //The first char will be replaced by our cmd/rw code
	uint16_t payload_len = sizeof(payload);

	//Test #1: command higher than maximum allowed
	uint8_t cmd_6bits_in = MAX_CMD_CODE + 1;
	uint8_t cmd_8bits = CMD_SET_W(cmd_6bits_in); //Write
	payload[CMD_CODE_INDEX] = cmd_8bits;
	uint8_t ret_val = 0, cmd_6bits_out = 0;
	ReadWrite rw = CmdInvalid;
	ret_val = payload_parse_str(payload, payload_len, &cmd_6bits_out, &rw);
	TEST_ASSERT_EQUAL(0, ret_val);	//ret_val = 0 when valid ToDo when sending 64, it extracts 0 and calls it valid... how do we handle this?
	TEST_ASSERT_EQUAL(cmd_6bits_in, cmd_6bits_out);
	TEST_ASSERT_EQUAL(CmdWrite, rw);

	/*
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
	*/
}

//Can the parse command identify a valid string?
void test_command_parse_valid(void)
{
	init_flexsea_payload_ptr();

	uint8_t payload[10] = "\0payload"; //The first char will be replaced by our cmd/rw code
	uint16_t payload_len = sizeof(payload);

	uint8_t cmd = 22;
	payload[CMD_CODE_INDEX] = CMD_SET_W(cmd);	//Write
	uint8_t ret_val = 0, cmd_6bits = 0;
	ReadWrite rw = CmdInvalid;

	ret_val = payload_parse_str(payload, payload_len, &cmd_6bits, &rw);
	if(!ret_val)
	{
		TEST_PASS(); //payload_parse_str() returned 0, it detected a valid command.
		flexsea_payload(cmd_6bits, rw, payload, payload_len);
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
	RUN_TEST(test_command_parse_valid);

	fflush(stdout);
}

#ifdef __cplusplus
}
#endif
