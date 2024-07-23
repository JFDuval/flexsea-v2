#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "tests.h"
#include "flexsea.h"

//Can we create a valid bytestream from a command?
void test_fx_create_bytestream_from_cmd(void)
{
	uint8_t payload_in[MAX_ENCODED_PAYLOAD_BYTES] = "FlexSEA v2 Full System Test";
	uint8_t payload_in_len = 27;	//Just the chars we want
	uint8_t bytestream[MAX_ENCODED_PAYLOAD_BYTES] = {0};
	uint8_t bytestream_len = 0;
	uint8_t cmd_6bits = 45;
	uint8_t ret_val = 0;
	ReadWrite rw = CmdWrite;

	ret_val = fx_create_bytestream_from_cmd(cmd_6bits, rw, payload_in,
			payload_in_len, bytestream, &bytestream_len);

	//Make sure our function works by checking it's return value, as
	//well as encode and command specific bytes
	TEST_ASSERT_EQUAL(0, ret_val);
	TEST_ASSERT_EQUAL(payload_in_len + MIN_OVERHEAD + CMD_OVERHEAD, bytestream_len);
	TEST_ASSERT_EQUAL(HEADER, bytestream[0]);
	TEST_ASSERT_EQUAL(CMD_SET_W(cmd_6bits), bytestream[2]);
	TEST_ASSERT_EQUAL(FOOTER, bytestream[bytestream_len - 1]);
}

//Can we get a valid command handler?
void test_fx_get_cmd_handler_from_bytestream(void)
{
	//This unit test is very close to being a full integration test...
	//We need to encode a command and feed it to a CB before we can
	//test the decoder.

	//The order is simple payload => command => encoded bytestream => CB
	uint8_t payload[] = "A very integrated unit test.";
	uint8_t payload_len = sizeof(payload);
	uint8_t bytestream[MAX_ENCODED_PAYLOAD_BYTES] = {0};
	uint8_t bytestream_len = 0;
	uint8_t cmd_6bits_in = 5;
	uint8_t ret_val = 0;
	ReadWrite rw = CmdWrite;
	ret_val = fx_create_bytestream_from_cmd(cmd_6bits_in, rw, payload,
			payload_len, bytestream, &bytestream_len);
	//We then feed it to a new circular buffer
	circ_buf_t cb = {.buffer = {0}, .length = 0, .write_index = 0, .read_index =
			0};
	int i = 0;
	ret_val = 0;
	for(i = 0; i < bytestream_len; i++)
	{
		//Write to circular buffer
		ret_val = circ_buf_write_byte(&cb, bytestream[i]);

		//circ_buf_write() should always return 0 if we are not overwriting
		if(ret_val)
		{
			TEST_FAIL_MESSAGE("CB indicates it's full while it shouldn't.");
			break;
		}
	}

	//At this point our encoded command is in the circular buffer
	uint8_t cmd_6bits_out = 0;
	ReadWrite rw_out = CmdInvalid;
	uint8_t buf[MAX_ENCODED_PAYLOAD_BYTES] = {0};
	uint8_t buf_len = 0;
	ret_val = fx_get_cmd_handler_from_bytestream(&cb, &cmd_6bits_out, &rw_out,
			buf, &buf_len);

	TEST_ASSERT_EQUAL(0, ret_val);
	TEST_ASSERT_EQUAL(cmd_6bits_in, cmd_6bits_out);
	TEST_ASSERT_EQUAL(rw, rw_out);
	TEST_ASSERT_EQUAL(CMD_SET_W(cmd_6bits_out), buf[0]);
	TEST_ASSERT_EQUAL_UINT8_ARRAY(payload, &buf[1], payload_len);
}

//Test structure
typedef struct
{
	uint8_t field1;
	int16_t field2;
	uint8_t field3[9];
	int32_t field4;
} __attribute__((__packed__)) FlexSEA_Cmd_Test_s;

//Can we serialize a structure, send it over a serial interface, then reconstruct it?
void test_structure_serialize_deserialize(void)
{
	//We create a structure
	FlexSEA_Cmd_Test_s fx_test_struct1;
	fx_test_struct1.field1 = 123;
	fx_test_struct1.field2 = -1234;
	memset(fx_test_struct1.field3, 0, 9);
	fx_test_struct1.field3[0] = 237;
	fx_test_struct1.field3[8] = 238;
	fx_test_struct1.field4 = 100000;

	uint8_t s = sizeof(fx_test_struct1);
	TEST_ASSERT_EQUAL(16, s);

	//We serialize it and feed it to a CB
	circ_buf_t cb = {.buffer = {0}, .length = 0, .write_index = 0, .read_index =
			0};
	uint8_t ret_val = 0;
	uint8_t* ptr= (uint8_t*)&fx_test_struct1;

	int i = 0;
	for(i = 0; i < s; i++)
	{
		//Write to circular buffer
		ret_val = circ_buf_write_byte(&cb, ptr[i]);

		//circ_buf_write() should always return 0 if we are not overwriting
		if(ret_val)
		{
			TEST_FAIL_MESSAGE("CB indicates it's full while it shouldn't.");
			break;
		}
	}

	uint16_t cb_size = 0;
	ret_val = circ_buf_get_size(&cb, &cb_size);
	TEST_ASSERT_EQUAL(0, ret_val);
	TEST_ASSERT_EQUAL(s, cb_size);

	//We then read it back from the CB, and re-create a structure from it.
	uint8_t r_array[CIRC_BUF_SIZE] = {0};
	uint8_t r_byte = 0;
	for(i = 0; i < cb_size; i++)
	{
		//Read from circular buffer
		ret_val = circ_buf_read_byte(&cb, &r_byte);
		//Save the same info in a regular buffer
		r_array[i] = r_byte;

		//circ_buf_read() should always return 0 if we are not reading from an empty buffer
		if(ret_val)
		{
			TEST_FAIL_MESSAGE("CB read indicates it's empty while it shouldn't.");
			break;
		}
	}

	//We create a new structure
	FlexSEA_Cmd_Test_s fx_test_struct2;
	memcpy(&fx_test_struct2, (FlexSEA_Cmd_Test_s*)&r_array, sizeof(fx_test_struct2));
	TEST_ASSERT_EQUAL(fx_test_struct1.field1, fx_test_struct2.field1);
	TEST_ASSERT_EQUAL(fx_test_struct1.field2, fx_test_struct2.field2);
	TEST_ASSERT_EQUAL(fx_test_struct1.field4, fx_test_struct2.field4);
}

void test_flexsea(void)
{
	RUN_TEST(test_fx_create_bytestream_from_cmd);
	RUN_TEST(test_fx_get_cmd_handler_from_bytestream);
	RUN_TEST(test_structure_serialize_deserialize);

	fflush(stdout);
}

#ifdef __cplusplus
}
#endif
