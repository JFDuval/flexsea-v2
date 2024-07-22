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

void test_flexsea(void)
{
	RUN_TEST(test_fx_create_bytestream_from_cmd);

	fflush(stdout);
}

#ifdef __cplusplus
}
#endif
