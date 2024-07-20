#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "tests.h"
#include "flexsea_codec.h"

void test_codec_pack_payload_simple(void)
{
	//Simple test: if we pack a short payload (text string), do we get the
	//correct length (including the \0) + 3 bytes for the header/footer/checksum?
	uint8_t payload[] = "flexsea_v2";
	uint8_t payload_len = sizeof(payload);
	uint8_t packed_payload_len = 0;
	uint8_t packed_payload[MAX_PACKED_PAYLOAD_BYTES] = {0};
	uint8_t ret_val = comm_pack_payload(payload, payload_len, packed_payload,
			&packed_payload_len, MAX_PACKED_PAYLOAD_BYTES);
	TEST_ASSERT_EQUAL_MESSAGE(0, ret_val,
			"comm_pack_payload() is reporting an error");
	TEST_ASSERT_EQUAL_MESSAGE(payload_len + MIN_OVERHEAD, packed_payload_len,
			"How many bytes does generating a string add?");

	//Check if our header and footer make sense, as well as our payload bytes
	TEST_ASSERT_EQUAL(HEADER, packed_payload[0]);
	TEST_ASSERT_EQUAL(payload_len, packed_payload[1]);
	TEST_ASSERT_EQUAL(FOOTER, packed_payload[packed_payload_len - 1]);

	//Manual checksum test:
	uint16_t i = 0;
	uint8_t manual_checksum = 0;
	for(i = 0; i < payload_len; i++)
	{
		manual_checksum += payload[i];
	}
	TEST_ASSERT_EQUAL(manual_checksum, packed_payload[packed_payload_len - 2]);
}

void test_codec_pack_payload_too_long(void)
{
	//We pack 48 chars, knowing that with overhead it will be too long. It should
	//return  an error, and no packed payload.
	uint8_t payload[48] = {[0 ... 47] = 'x'};
	uint8_t payload_len = sizeof(payload);
	uint8_t packed_payload_len = 0;
	uint8_t packed_payload[MAX_PACKED_PAYLOAD_BYTES] = {0};
	uint8_t ret_val = comm_pack_payload(payload, payload_len, packed_payload,
			&packed_payload_len, MAX_PACKED_PAYLOAD_BYTES);
	TEST_ASSERT_NOT_EQUAL_MESSAGE(0, ret_val,
			"comm_pack_payload() is not reporting an error when it should");
	TEST_ASSERT_EQUAL(0, packed_payload_len);
}

void test_codec_pack_payload_escape(void)
{
	//We make sure that escape chars are added where they should be, and that the
	//string length is correct
	uint8_t payload[36] = {[0 ... 35] = 'x'};
	payload[10] = HEADER; //Replace one 'x' by a char that will need to be escaped
	uint8_t payload_len = sizeof(payload);
	uint8_t packed_payload_len = 0;
	uint8_t packed_payload[MAX_PACKED_PAYLOAD_BYTES] = {0};
	uint8_t ret_val = comm_pack_payload(payload, payload_len, packed_payload,
			&packed_payload_len, MAX_PACKED_PAYLOAD_BYTES);
	TEST_ASSERT_EQUAL_MESSAGE(0, ret_val,
			"comm_pack_payload() is reporting an error");
	TEST_ASSERT_EQUAL_MESSAGE(payload_len + MIN_OVERHEAD + 1,
			packed_payload_len, "How many bytes does generating a string add?");

	//Check if our header and footer make sense, as well as our payload bytes
	//(including a shift for the escape char)
	TEST_ASSERT_EQUAL(HEADER, packed_payload[0]);
	TEST_ASSERT_EQUAL(payload_len + 1, packed_payload[1]);
	TEST_ASSERT_EQUAL(FOOTER, packed_payload[packed_payload_len - 1]);
	TEST_ASSERT_EQUAL(ESCAPE, packed_payload[12]); //Is the ESCAPE where it should be?

	//Manual checksum test:
	uint16_t i = 0;
	uint8_t manual_checksum = 0;
	for(i = 0; i < payload_len; i++)
	{
		manual_checksum += payload[i];
	}
	manual_checksum += ESCAPE;	//Adding one ESCAPE char
	TEST_ASSERT_EQUAL(manual_checksum, packed_payload[packed_payload_len - 2]);
}

//Simple payload (no escape, short, etc.) located right at the start of our
//circular buffer
void test_codec_unpack_payload_simple(void)
{
	//We start by packing a simple payload
	uint8_t payload[] = "flexsea_v2";
	uint8_t payload_len = sizeof(payload);
	uint8_t packed_payload_len = 0;
	uint8_t packed_payload[MAX_PACKED_PAYLOAD_BYTES] = {0};
	uint8_t ret_val = comm_pack_payload(payload, payload_len, packed_payload,
			&packed_payload_len, MAX_PACKED_PAYLOAD_BYTES);
	TEST_ASSERT_EQUAL_MESSAGE(0, ret_val,
			"comm_pack_payload() is reporting an error");
	TEST_ASSERT_EQUAL_MESSAGE(payload_len + MIN_OVERHEAD, packed_payload_len,
			"How many bytes does generating a string add?");

	//We then feed it to a new circular buffer
	circ_buf_t cb = {.buffer = {0}, .length = 0, .write_index = 0, .read_index =
			0};
	int i = 0;
	ret_val = 0;
	for(i = 0; i < packed_payload_len; i++)
	{
		//Write to circular buffer
		ret_val = circ_buf_write_byte(&cb, packed_payload[i]);

		//circ_buf_write() should always return 0 if we are not overwriting
		if(ret_val)
		{
			TEST_FAIL_MESSAGE("CB indicates it's full while it shouldn't.");
			break;
		}
	}

	//At this point our packaged payload is in 'cb'. We unpack it.
	uint8_t extracted_packed_payload[MAX_PACKED_PAYLOAD_BYTES] = {0};
	uint8_t extracted_packed_payload_len = 0;
	uint8_t extracted_unpacked_payload[MAX_PACKED_PAYLOAD_BYTES] = {0};
	uint8_t extracted_unpacked_payload_len = 0;
	ret_val = comm_unpack_payload(&cb, extracted_packed_payload,
			&extracted_packed_payload_len, extracted_unpacked_payload,
			&extracted_unpacked_payload_len);
	if(ret_val)
	{
		TEST_FAIL_MESSAGE("comm_unpack_payload encountered an error");
	}

	//Compare lengths in & out
	TEST_ASSERT_EQUAL(payload_len, extracted_unpacked_payload_len);
	TEST_ASSERT_EQUAL(packed_payload_len, extracted_packed_payload_len);

	//Compare strings in & out
	TEST_ASSERT_EQUAL_UINT8_ARRAY(packed_payload, extracted_packed_payload,
			packed_payload_len);
	TEST_ASSERT_EQUAL_UINT8_ARRAY(payload, extracted_unpacked_payload,
			payload_len);
}

//Simple payload (no escape, short, etc.) preceded by some garbage in our
//circular buffer
void test_codec_unpack_payload_with_garbage_before(void)
{
	//We start by packing a simple payload
	uint8_t payload[] = "flexsea_v2";
	uint8_t payload_len = sizeof(payload);
	uint8_t packed_payload_len = 0;
	uint8_t packed_payload[MAX_PACKED_PAYLOAD_BYTES] = {0};
	uint8_t ret_val = comm_pack_payload(payload, payload_len, packed_payload,
			&packed_payload_len, MAX_PACKED_PAYLOAD_BYTES);
	TEST_ASSERT_EQUAL_MESSAGE(0, ret_val,
			"comm_pack_payload() is reporting an error");
	TEST_ASSERT_EQUAL_MESSAGE(payload_len + MIN_OVERHEAD, packed_payload_len,
			"How many bytes does generating a string add?");

	//We prepare a new circular buffer
	circ_buf_t cb = {.buffer = {0}, .length = 0, .write_index = 0, .read_index =
			0};

	//Oh, there is noise on our bus! We get some bytes in, and some are
	//key values used by our communication...
	circ_buf_write_byte(&cb, 0);
	circ_buf_write_byte(&cb, 0);
	circ_buf_write_byte(&cb, HEADER);
	circ_buf_write_byte(&cb, FOOTER);
	circ_buf_write_byte(&cb, ESCAPE);
	circ_buf_write_byte(&cb, 123);
	circ_buf_write_byte(&cb, FOOTER);

	//Our payload makes it into the circular buffer
	int i = 0;
	ret_val = 0;
	for(i = 0; i < packed_payload_len; i++)
	{
		//Write to circular buffer
		ret_val = circ_buf_write_byte(&cb, packed_payload[i]);

		//circ_buf_write() should always return 0 if we are not overwriting
		if(ret_val)
		{
			TEST_FAIL_MESSAGE("CB indicates it's full while it shouldn't.");
			break;
		}
	}

	//At this point our packaged payload is in 'cb'. We unpack it.
	uint8_t extracted_packed_payload[MAX_PACKED_PAYLOAD_BYTES] = {0};
	uint8_t extracted_packed_payload_len = 0;
	uint8_t extracted_unpacked_payload[MAX_PACKED_PAYLOAD_BYTES] = {0};
	uint8_t extracted_unpacked_payload_len = 0;
	ret_val = comm_unpack_payload(&cb, extracted_packed_payload,
			&extracted_packed_payload_len, extracted_unpacked_payload,
			&extracted_unpacked_payload_len);
	if(ret_val)
	{
		TEST_FAIL_MESSAGE("comm_unpack_payload encountered an error");
	}

	//Compare lengths in & out
	TEST_ASSERT_EQUAL(payload_len, extracted_unpacked_payload_len);
	TEST_ASSERT_EQUAL(packed_payload_len, extracted_packed_payload_len);

	//Compare strings in & out
	TEST_ASSERT_EQUAL_UINT8_ARRAY(packed_payload, extracted_packed_payload,
			packed_payload_len);
	TEST_ASSERT_EQUAL_UINT8_ARRAY(payload, extracted_unpacked_payload,
			payload_len);
}

//Long payload (no escape, short, etc.) preceded by some garbage in our
//circular buffer, and with a corrupted byte in the middle
void test_codec_unpack_payload_with_garbage_before_and_corruption(void)
{
	//We start by packing a long payload
	uint8_t payload[] = "the flexsea_v2 comm protocol is awesome!";
	uint8_t payload_len = sizeof(payload);
	uint8_t packed_payload_len = 0;
	uint8_t packed_payload[MAX_PACKED_PAYLOAD_BYTES] = {0};
	uint8_t ret_val = comm_pack_payload(payload, payload_len, packed_payload,
			&packed_payload_len, MAX_PACKED_PAYLOAD_BYTES);
	TEST_ASSERT_EQUAL_MESSAGE(0, ret_val,
			"comm_pack_payload() is reporting an error");
	TEST_ASSERT_EQUAL_MESSAGE(payload_len + MIN_OVERHEAD, packed_payload_len,
			"How many bytes does generating a string add?");

	//We prepare a new circular buffer
	circ_buf_t cb = {.buffer = {0}, .length = 0, .write_index = 0, .read_index =
			0};

	//Oh, there is noise on our bus! We get some bytes in, and some are
	//key values used by our communication...
	circ_buf_write_byte(&cb, FOOTER);
	circ_buf_write_byte(&cb, FOOTER);
	circ_buf_write_byte(&cb, FOOTER);
	circ_buf_write_byte(&cb, 0);
	circ_buf_write_byte(&cb, 0);
	circ_buf_write_byte(&cb, HEADER);
	circ_buf_write_byte(&cb, FOOTER);
	circ_buf_write_byte(&cb, ESCAPE);
	circ_buf_write_byte(&cb, 123);
	circ_buf_write_byte(&cb, FOOTER);

	//Our payload makes it into the circular buffer
	int i = 0;
	ret_val = 0;
	for(i = 0; i < packed_payload_len; i++)
	{
		//Write to circular buffer
		ret_val = circ_buf_write_byte(&cb, packed_payload[i]);

		//circ_buf_write() should always return 0 if we are not overwriting
		if(ret_val)
		{
			TEST_FAIL_MESSAGE("CB indicates it's full while it shouldn't.");
			break;
		}
	}

	//While in transit, a byte gets corrupted
	cb.buffer[cb.read_index + (cb.length / 2)] = (cb.buffer[cb.read_index
			+ (cb.length / 2)] + 1);

	//At this point our packaged payload is in 'cb'. Due to the byte corruption, we should not be able to unpack it.
	uint8_t extracted_packed_payload[MAX_PACKED_PAYLOAD_BYTES] = {0};
	uint8_t extracted_packed_payload_len = 0;
	uint8_t extracted_unpacked_payload[MAX_PACKED_PAYLOAD_BYTES] = {0};
	uint8_t extracted_unpacked_payload_len = 0;
	ret_val = comm_unpack_payload(&cb, extracted_packed_payload,
			&extracted_packed_payload_len, extracted_unpacked_payload,
			&extracted_unpacked_payload_len);
	if(!ret_val)
	{
		TEST_FAIL_MESSAGE("comm_unpack_payload should have thrown an error!");
	}

	//Make sure our reported lengths are 0
	TEST_ASSERT_EQUAL(0, extracted_unpacked_payload_len);
	TEST_ASSERT_EQUAL(0, extracted_packed_payload_len);

	//Make sure return payloads are empty
	uint8_t empty_payload[MAX_PACKED_PAYLOAD_BYTES] = {0};
	TEST_ASSERT_EQUAL_UINT8_ARRAY(empty_payload, extracted_packed_payload,
			MAX_PACKED_PAYLOAD_BYTES);
	TEST_ASSERT_EQUAL_UINT8_ARRAY(empty_payload, extracted_unpacked_payload,
			MAX_PACKED_PAYLOAD_BYTES);
}

//Long payload (no escape, short, etc.) received periodically
//We decode as soon as we receive
void test_codec_continuous_receive_unpack(void)
{
	//We start by packing a long payload
	int i = 0, loop = 0;
	uint8_t payload[] = "the flexsea_v2 comm protocol is reliable!";
	uint8_t payload_len = sizeof(payload);
	uint8_t packed_payload_len = 0;
	uint8_t packed_payload[MAX_PACKED_PAYLOAD_BYTES] = {0};
	uint8_t ret_val = comm_pack_payload(payload, payload_len, packed_payload,
			&packed_payload_len, MAX_PACKED_PAYLOAD_BYTES);
	TEST_ASSERT_EQUAL_MESSAGE(0, ret_val,
			"comm_pack_payload() is reporting an error");
	TEST_ASSERT_EQUAL_MESSAGE(payload_len + MIN_OVERHEAD, packed_payload_len,
			"How many bytes does generating a string add?");

	//We prepare a new circular buffer
	circ_buf_t cb = {.buffer = {0}, .length = 0, .write_index = 0, .read_index =
			0};

	//We want to make sure we go around our circular buffer a few times.
	int iterations = (10 * CIRC_BUF_SIZE) / MAX_PACKED_PAYLOAD_BYTES;
	uint8_t extracted_packed_payload[MAX_PACKED_PAYLOAD_BYTES] = {0};
	uint8_t extracted_packed_payload_len = 0;
	uint8_t extracted_unpacked_payload[MAX_PACKED_PAYLOAD_BYTES] = {0};
	uint8_t extracted_unpacked_payload_len = 0;
	for(loop = 0; loop < iterations; loop++)
	{
		//Our payload makes it into the circular buffer
		ret_val = 0;
		for(i = 0; i < packed_payload_len; i++)
		{
			//Write to circular buffer
			ret_val = circ_buf_write_byte(&cb, packed_payload[i]);

			//circ_buf_write() should always return 0 if we are not overwriting
			if(ret_val)
			{
				TEST_FAIL_MESSAGE("CB indicates it's full while it shouldn't.");
				break;
			}
		}

		//At this point our packaged payload is in 'cb'. We unpack it.
		extracted_packed_payload_len = 0;
		extracted_unpacked_payload_len = 0;
		memset(extracted_packed_payload, 0, MAX_PACKED_PAYLOAD_BYTES);
		memset(extracted_unpacked_payload, 0, MAX_PACKED_PAYLOAD_BYTES);
		ret_val = comm_unpack_payload(&cb, extracted_packed_payload,
				&extracted_packed_payload_len, extracted_unpacked_payload,
				&extracted_unpacked_payload_len);
		if(ret_val)
		{
			TEST_FAIL_MESSAGE("comm_unpack_payload encountered an error");
		}

		//Compare lengths in & out
		TEST_ASSERT_EQUAL(payload_len, extracted_unpacked_payload_len);
		TEST_ASSERT_EQUAL(packed_payload_len, extracted_packed_payload_len);

		//Compare strings in & out
		TEST_ASSERT_EQUAL_UINT8_ARRAY(packed_payload, extracted_packed_payload,
				packed_payload_len);
		TEST_ASSERT_EQUAL_UINT8_ARRAY(payload, extracted_unpacked_payload,
				payload_len);
	}
}

void test_flexsea_codec(void)
{
	//Packing:
	RUN_TEST(test_codec_pack_payload_simple);
	RUN_TEST(test_codec_pack_payload_too_long);
	RUN_TEST(test_codec_pack_payload_escape);

	//Unpacking:
	RUN_TEST(test_codec_unpack_payload_simple);
	RUN_TEST(test_codec_unpack_payload_with_garbage_before);
	RUN_TEST(test_codec_unpack_payload_with_garbage_before_and_corruption);

	//Continuous data stream:
	RUN_TEST(test_codec_continuous_receive_unpack);

	fflush(stdout);
}

#ifdef __cplusplus
}
#endif
