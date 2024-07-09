#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "flexsea_comm.h"
#include "unity.h"

void test_comm_gen_str_simple(void)
{
	//Simple test: if we pack a short text string, do we get the correct length (including the \0) + 3 bytes for the header/footer/checksum?
	uint8_t my_test_payload[] = "flexsea_v2";
	uint8_t my_test_payload_len = sizeof(my_test_payload);
	uint8_t my_test_packed_payload[PACKAGED_PAYLOAD_LEN] = {0};
	uint8_t packed_bytes = comm_gen_str(my_test_payload, my_test_packed_payload, my_test_payload_len);
	TEST_ASSERT_EQUAL_MESSAGE(my_test_payload_len + 3, packed_bytes, "How many bytes does generating a string add?");

	//Check if our header and footer make sense, as well as our payload bytes
	TEST_ASSERT_EQUAL(HEADER, my_test_packed_payload[0]);
	TEST_ASSERT_EQUAL(my_test_payload_len, my_test_packed_payload[1]);
	TEST_ASSERT_EQUAL(FOOTER, my_test_packed_payload[packed_bytes]);

	//Manual checksum test:
	uint16_t i = 0;
	uint8_t manual_checksum = 0;
	for(i = 0; i < my_test_payload_len; i++)
	{
		manual_checksum += my_test_payload[i];
	}
	TEST_ASSERT_EQUAL(manual_checksum, my_test_packed_payload[packed_bytes - 1]);
}

void test_comm_gen_str_too_long(void)
{
	//We pack 48 chars, knowing that with overhead it will be too long. It should return 0
	uint8_t my_test_payload[48] = {[0 ... 47] = 'x'};
	uint8_t my_test_payload_len = sizeof(my_test_payload);
	uint8_t my_test_packed_payload[PACKAGED_PAYLOAD_LEN] = {0};
	uint8_t packed_bytes = comm_gen_str(my_test_payload, my_test_packed_payload, my_test_payload_len);
	TEST_ASSERT_EQUAL(0, packed_bytes);
}

void test_comm_gen_str_escape(void)
{
	//We make sure that escape chars are added where they should be, and that the string length is correct
	uint8_t my_test_payload[36] = {[0 ... 35] = 'x'};
	my_test_payload[10] = HEADER;	//Replace one 'x' by a char that will need to be escaped
	uint8_t my_test_payload_len = sizeof(my_test_payload);
	uint8_t my_test_packed_payload[PACKAGED_PAYLOAD_LEN] = {0};
	uint8_t packed_bytes = comm_gen_str(my_test_payload, my_test_packed_payload, my_test_payload_len);
	TEST_ASSERT_EQUAL_MESSAGE(my_test_payload_len + 4, packed_bytes, "How many bytes does generating a string add?");

	//Check if our header and footer make sense, as well as our payload bytes (including a shift for the escape char)
	TEST_ASSERT_EQUAL(HEADER, my_test_packed_payload[0]);
	TEST_ASSERT_EQUAL(my_test_payload_len + 1, my_test_packed_payload[1]);
	TEST_ASSERT_EQUAL(FOOTER, my_test_packed_payload[packed_bytes]);
	TEST_ASSERT_EQUAL(ESCAPE, my_test_packed_payload[12]);	//Is the ESCAPE where it should be?

	//Manual checksum test:
	uint16_t i = 0;
	uint8_t manual_checksum = 0;
	for(i = 0; i < my_test_payload_len; i++)
	{
		manual_checksum += my_test_payload[i];
	}
	manual_checksum += ESCAPE;	//Adding one ESCAPE char
	TEST_ASSERT_EQUAL(manual_checksum, my_test_packed_payload[packed_bytes - 1]);
}

//Simple payload (no escape, short, etc) located right at the start of our circular buffer
void test_comm_unpack_1(void)
{
	//We start by packing a simple payload
	uint8_t my_test_payload[] = "flexsea_v2";
	uint8_t my_test_payload_len = sizeof(my_test_payload);
	uint8_t my_test_packed_payload[PACKAGED_PAYLOAD_LEN] = {0};
	uint8_t packed_bytes = comm_gen_str(my_test_payload, my_test_packed_payload, my_test_payload_len);
	TEST_ASSERT_EQUAL_MESSAGE(my_test_payload_len + 3, packed_bytes, "How many bytes does generating a string add?");

	//We then feed it to a new circular buffer
	circ_buf_t cb = {.buffer = {0},        \
					 .length = 0,          \
					 .write_index = 0,     \
					 .read_index = 0};
	int i = 0;
	uint8_t ret_val = 0;
	for(i = 0; i < packed_bytes + 1; i++)
	{
		//Write to circular buffer
		ret_val = circ_buf_write_byte(&cb, my_test_packed_payload[i]);

		//circ_buf_write() should always return 0 if we are not overwriting
		if(ret_val)
		{
			TEST_FAIL_MESSAGE("CB indicates it's full while it shouldn't.");
			break;
		}
	}

	//At this point our packaged payload is in 'cb'. We unpack it.
	uint8_t extracted_packed_payload[PACKAGED_PAYLOAD_LEN] = {0};
	uint8_t extracted_unpacked_payload[PACKAGED_PAYLOAD_LEN] = {0};
	ret_val = unpack_payload_cb2(&cb, extracted_packed_payload,extracted_unpacked_payload);
	if(ret_val)
	{
		TEST_FAIL_MESSAGE("unpack_payload_cb2 encountered an error");
	}
}

/*

//Helper function for test_circ_unpack
int testCircPackUnpack(circularBuffer_t* cb, uint8_t* comm_str, uint8_t* packed, uint8_t* unpacked,
						uint8_t preoffset, uint8_t postOffset)
{
	int i;
	uint8_t value;
	for(i=0; i < preoffset; i++)
	{
		value = rand();
		circ_buff_write(cb, &value, 1);
	}

	circ_buff_write(cb, comm_str, COMM_STR_BUF_LEN);

	for(i=0; i < postOffset; i++)
	{
		value = rand();
		circ_buff_write(cb, &value, 1);
	}

	LOCK_MUTEX(&(cp->data_guard));
	uint16_t retCode = unpack_payload_cb(cb, packed, unpacked);
	UNLOCK_MUTEX(&(cp->data_guard));
	return retCode;
}

int fillFakeReadAll(uint8_t* fakePayload)
{
	int index = 0;
	fakePayload[index++] = FLEXSEA_PLAN_1;
	fakePayload[index++] = FLEXSEA_MANAGE_1;
	fakePayload[index++] = 1;
	fakePayload[index++] = CMD_R(CMD_READ_ALL);

	uint8_t value;
	while(index < 44)
	{
		do { value = rand(); }
		while(value == HEADER || value == FOOTER || value == ESCAPE);

		fakePayload[index++] = value;
	}

	return index;
}

void test_circ_unpack(void)
{
	//First, we generate a comm_str:
	//==============================
	srand(time(NULL));

	//Empty strings:
	memset(fakePayload, 0, PAYLOAD_BUF_LEN);
	memset(fakeCommStr, 0, COMM_STR_BUF_LEN);

	circularBuffer_t cb;
	circ_buff_init(&cb);

	//Positive tests, it should find a comm str
	int preOffsetMax;
	uint8_t tPacked[RX_BUF_LEN];
	uint8_t tUnpacked[RX_BUF_LEN];
	int i, j, preoffset, postoffset, expected, result, lengthUnpacked, comm_str_last_ind;
	for(i = 0; i < 100; i++)
	{
		lengthUnpacked = fillFakeReadAll(fakePayload);
		resetCommStats();
		comm_str_last_ind = comm_gen_str(fakePayload, fakeCommStr, lengthUnpacked);
		preOffsetMax = CB_BUF_LEN - comm_str_last_ind - 1;
		preoffset = rand() % preOffsetMax;
		postoffset = CB_BUF_LEN - COMM_STR_BUF_LEN - preoffset;
		result = testCircPackUnpack(&cb, fakeCommStr, tPacked, tUnpacked, preoffset, postoffset);
		expected = preoffset + comm_str_last_ind + 1;
		TEST_ASSERT_EQUAL(expected, result);
		for(j = 0; j < lengthUnpacked; j++)
		{
			TEST_ASSERT_EQUAL_MESSAGE(fakePayload[j], tUnpacked[j], "Positive case, unpacked strings mismatch");
		}
		for(j = 0; j <= comm_str_last_ind; j++)
		{
			TEST_ASSERT_EQUAL_MESSAGE(fakeCommStr[j], tPacked[j], "Positive case, packed strings mismatch");
		}
	}

	//Negative tests, invalid comm str
	//numbytes is wrong
	fakeCommStr[1] = fakeCommStr[1] * 2;
	expected = 0;
	for(i = 0; i < 10; i++)
	{
		preoffset = rand() % preOffsetMax;
		postoffset = CB_BUF_LEN - COMM_STR_BUF_LEN - preoffset;
		result = testCircPackUnpack(&cb, fakeCommStr, tPacked, tUnpacked, preoffset, postoffset);
		TEST_ASSERT_EQUAL(expected, result);
	}
	fakeCommStr[1] = fakeCommStr[1] / 2;
	//footer is wrong
	fakeCommStr[comm_str_last_ind] = 0;
	expected = 0;
	for(i = 0; i < 10; i++)
	{
		preoffset = rand() % preOffsetMax;
		postoffset = CB_BUF_LEN - COMM_STR_BUF_LEN - preoffset;
		result = testCircPackUnpack(&cb, fakeCommStr, tPacked, tUnpacked, preoffset, postoffset);
		TEST_ASSERT_EQUAL(expected, result);
	}
	//checksum is wrong
	fakeCommStr[comm_str_last_ind] = FOOTER;
	fakeCommStr[comm_str_last_ind-1] *= 2;
	expected = 0;
	for(i = 0; i < 10; i++)
	{
		preoffset = rand() % preOffsetMax;
		postoffset = CB_BUF_LEN - COMM_STR_BUF_LEN - preoffset;
		result = testCircPackUnpack(&cb, fakeCommStr, tPacked, tUnpacked, preoffset, postoffset);
		TEST_ASSERT_EQUAL(expected, result);
	}
}
*/

void test_flexsea_comm(void)
{
	RUN_TEST(test_comm_gen_str_simple);
	RUN_TEST(test_comm_gen_str_too_long);
	RUN_TEST(test_comm_gen_str_escape);

	RUN_TEST(test_comm_unpack_1);

	//RUN_TEST(test_circ_unpack);

	fflush(stdout);
}

#ifdef __cplusplus
}
#endif
