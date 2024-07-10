#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "tests.h"
#include "circ_buf.h"

//Test writing (and overwriting) a full buffer
void test_circ_buf_w(void)
{
	//Initialize new cir_buf
	circ_buf_t cb = {.buffer = {0}, .length = 0, .write_index = 0, .read_index =
			0};

	//Write sequential values to buffer, filling it
	int i = 0;
	uint8_t w_byte = 0, ret_val = 0;
	for(i = 0; i < CIRC_BUF_SIZE; i++)
	{
		//Write to circular buffer
		ret_val = circ_buf_write_byte(&cb, w_byte);
		w_byte++;

		//circ_buf_write() should always return 0 if we are not overwriting
		if(ret_val)
		{
			TEST_FAIL_MESSAGE("CB indicates it's full while it shouldn't.");
			break;
		}
	}

	//We add one more value. It should overwrite the first byte.
	w_byte = 123;
	ret_val = circ_buf_write_byte(&cb, w_byte);
	//circ_buf_write() should always return 0 if we are not overwriting => expecting 1
	TEST_ASSERT_EQUAL(1, ret_val);
	//Did that value get written?
	TEST_ASSERT_EQUAL(cb.buffer[0], w_byte);
}

//Test writing and reading a full buffer
void test_circ_buf_rw(void)
{
	//Initialize new cir_buf
	circ_buf_t cb = {.buffer = {0}, .length = 0, .write_index = 0, .read_index =
			0};
	//Create two arrays of the same size:
	uint8_t w_array[CIRC_BUF_SIZE] = {0};
	uint8_t r_array[CIRC_BUF_SIZE] = {0};

	//Write sequential values to buffer, filling it
	int i = 0;
	uint8_t w_byte = 0, ret_val = 0;
	for(i = 0; i < CIRC_BUF_SIZE; i++)
	{
		//Write to circular buffer
		ret_val = circ_buf_write_byte(&cb, w_byte);
		//Save the same info in a regular buffer
		w_array[i] = w_byte;
		w_byte++;

		//circ_buf_write() should always return 0 if we are not overwriting
		if(ret_val)
		{
			TEST_FAIL_MESSAGE("CB indicates it's full while it shouldn't.");
			break;
		}
	}

	//Read entire buffer
	uint8_t r_byte = 0;
	for(i = 0; i < CIRC_BUF_SIZE; i++)
	{
		//Read from circular buffer
		ret_val = circ_buf_read_byte(&cb, &r_byte);
		//Save the same info in a regular buffer
		r_array[i] = r_byte;

		//circ_buf_read() should always return 0 if we are not overwriting
		if(ret_val)
		{
			TEST_FAIL_MESSAGE("CB indicates it's full while it shouldn't.");
			break;
		}
	}

	//Compare what we wrote and read
	int same = 0;
	for(i = 0; i < CIRC_BUF_SIZE; i++)
	{
		if(w_array[i] == r_array[i])
		{
			same++;
		}
	}

	//We should be getting identical values for every index
	TEST_ASSERT_EQUAL(CIRC_BUF_SIZE, same);
}

//Test the buffer size function
void test_circ_buf_size(void)
{
	//Initialize new cir_buf
	circ_buf_t cb = {.buffer = {0}, .length = 0, .write_index = 0, .read_index =
			0};

	//New buffer should report a size of 0
	TEST_ASSERT_EQUAL(0, circ_buf_get_size(&cb));

	//Write sequential values to buffer
	int i = 0;
	uint8_t w_byte = 0, ret_val = 0;
	int16_t manual_counter = 0;
	int16_t bytes_to_write = CIRC_BUF_SIZE / 2;
	for(i = 0; i < bytes_to_write; i++)
	{
		//Write to circular buffer
		ret_val = circ_buf_write_byte(&cb, w_byte);
		w_byte++;

		//circ_buf_write() should always return 0 if we are not overwriting
		if(ret_val)
		{
			TEST_FAIL_MESSAGE("CB indicates it's full while it shouldn't.");
			break;
		}
	}
	manual_counter = bytes_to_write;
	TEST_ASSERT_EQUAL(manual_counter, circ_buf_get_size(&cb));

	//Read some bytes out
	int16_t bytes_to_read = bytes_to_write / 2;
	int16_t remaining_bytes_to_read = bytes_to_write - bytes_to_read; //(just in case CIRC_BUF_SIZE is odd)
	uint8_t r_byte = 0;
	for(i = 0; i < bytes_to_read; i++)
	{
		//Read from circular buffer
		ret_val = circ_buf_read_byte(&cb, &r_byte);

		//circ_buf_read() should always return 0 if we are not overwriting
		if(ret_val)
		{
			TEST_FAIL_MESSAGE("CB indicates it's full while it shouldn't.");
			break;
		}
	}
	manual_counter -= bytes_to_read;
	TEST_ASSERT_EQUAL(manual_counter, circ_buf_get_size(&cb));

	//Finish reading the buffer
	for(i = 0; i < remaining_bytes_to_read; i++)
	{
		//Read from circular buffer
		ret_val = circ_buf_read_byte(&cb, &r_byte);

		//circ_buf_read() should always return 0 if we are not overwriting
		if(ret_val)
		{
			TEST_FAIL_MESSAGE("CB indicates it's full while it shouldn't.");
			break;
		}
	}
	manual_counter -= remaining_bytes_to_read;
	TEST_ASSERT_EQUAL_MESSAGE(0, manual_counter,
			"Test writer error, this should be zero.");
	TEST_ASSERT_EQUAL(manual_counter, circ_buf_get_size(&cb));

	//And now we try to read from an empty buffer
	ret_val = circ_buf_read_byte(&cb, &r_byte);
	//circ_buf_read() should always return 0 if we are not overreading => expecting 1, with r_byte = 0
	TEST_ASSERT_EQUAL(1, ret_val);
	TEST_ASSERT_EQUAL(0, r_byte);
	//Size should still be 0
	TEST_ASSERT_EQUAL(0, circ_buf_get_size(&cb));
}

//Test the peek function
void test_circ_buf_peek(void)
{
	//Initialize new cir_buf
	circ_buf_t cb = {.buffer = {0}, .length = 0, .write_index = 0, .read_index =
			0};

	//Write sequential values to buffer, filling it
	int i = 0;
	uint8_t w_byte = 0, ret_val = 0;
	for(i = 0; i < CIRC_BUF_SIZE; i++)
	{
		//Write to circular buffer
		ret_val = circ_buf_write_byte(&cb, w_byte);
		w_byte++;

		//circ_buf_write() should always return 0 if we are not overwriting
		if(ret_val)
		{
			TEST_FAIL_MESSAGE("CB indicates it's full while it shouldn't.");
			break;
		}
	}

	//Confirm that our size is of a full buffer
	TEST_ASSERT_EQUAL(CIRC_BUF_SIZE, circ_buf_get_size(&cb));
	//Peek and confirm that we are seeing the correct value
	uint8_t p_byte = 0, offset = 10;
	ret_val = circ_buf_peek(&cb, &p_byte, offset);
	TEST_ASSERT_EQUAL(0, ret_val);	//Should be 0, we are within the size
	TEST_ASSERT_EQUAL(offset, p_byte);//Sequential values, offset should equal p_byte
	//Make sure we didn't change the size
	TEST_ASSERT_EQUAL(CIRC_BUF_SIZE, circ_buf_get_size(&cb));
}

//Test the search function
void test_circ_buf_search(void)
{
	//Initialize new cir_buf
	circ_buf_t cb = {.buffer = {0}, .length = 0, .write_index = 0, .read_index =
			0};

	//Search an empty buffer. We should get an error.
	uint8_t ret_val = 0;
	uint8_t value = 255;
	uint16_t search_result = 0, start_index = 0;
	ret_val = circ_buf_search(&cb, &search_result, value, start_index);
	//circ_buf_search() should always return 0 if it contains our value, 1 otherwise
	TEST_ASSERT_EQUAL(1, ret_val);
	//DDid we get search_result = 0?
	TEST_ASSERT_EQUAL(0, search_result);

	//Write sequential values to buffer, filling it
	int i = 0;
	uint8_t w_byte = 0;
	for(i = 0; i < CIRC_BUF_SIZE; i++)
	{
		//Write to circular buffer
		ret_val = circ_buf_write_byte(&cb, w_byte);
		w_byte++;

		//circ_buf_write() should always return 0 if we are not overwriting
		if(ret_val)
		{
			TEST_FAIL_MESSAGE("CB indicates it's full while it shouldn't.");
			break;
		}
	}

	//We should find '255' at index 255
	ret_val = 0;
	value = 255;
	search_result = 0;
	start_index = 0;
	ret_val = circ_buf_search(&cb, &search_result, value, start_index);
	//circ_buf_search() should always return 0 if it contains our value
	TEST_ASSERT_EQUAL(0, ret_val);
	//Is our value where it should be based on our sequential writing?
	TEST_ASSERT_EQUAL(value, search_result);

	//If we start searching further into the buffer we should find our value again at index 511
	ret_val = 0;
	value = 255;
	search_result = 0;
	start_index = 256;
	ret_val = circ_buf_search(&cb, &search_result, value, start_index);
	//circ_buf_search() should always return 0 if it contains our value
	TEST_ASSERT_EQUAL(0, ret_val);
	//Is our value where it should be based on our sequential writing?
	TEST_ASSERT_EQUAL(511, search_result);

	//Start searching at the very end to make sure our search function wraps around correctly
	ret_val = 0;
	value = 100;
	search_result = 0;
	start_index = CIRC_BUF_SIZE - 1;
	ret_val = circ_buf_search(&cb, &search_result, value, start_index);
	//circ_buf_search() should always return 0 if it contains our value
	TEST_ASSERT_EQUAL(0, ret_val);
	//Is our value where it should be based on our sequential writing?
	TEST_ASSERT_EQUAL(value, search_result);

	//We re-init our circular buffer, and half-fill it with constants
	circ_buf_init(&cb);
	w_byte = 123;
	for(i = 0; i < CIRC_BUF_SIZE / 10; i++)
	{
		//Write to circular buffer
		ret_val = circ_buf_write_byte(&cb, w_byte);
	}
	//And now we search for a value that isn't there
	ret_val = 0;
	value = 234;
	search_result = 0;
	start_index = 0;
	ret_val = circ_buf_search(&cb, &search_result, value, start_index);
	//circ_buf_search() should always return 0 if it contains our value => 1 in this case
	TEST_ASSERT_EQUAL(1, ret_val);
	//Is our value where it should be based on our sequential writing? 0 when not found
	TEST_ASSERT_EQUAL(0, search_result);
}

//Test the checksum calculation
void test_circ_buf_checksum(void)
{
	//Initialize new cir_buf
	circ_buf_t cb = {.buffer = {0}, .length = 0, .write_index = 0, .read_index =
			0};

	//Create one array of the same size:
	uint8_t w_array[CIRC_BUF_SIZE] = {0};

	//Write sequential values to buffer, filling it
	int i = 0;
	uint8_t w_byte = 0, ret_val = 0;
	for(i = 0; i < CIRC_BUF_SIZE; i++)
	{
		//Write to circular buffer
		ret_val = circ_buf_write_byte(&cb, w_byte);
		//Save the same info in a regular buffer
		w_array[i] = w_byte;
		w_byte++;

		//circ_buf_write() should always return 0 if we are not overwriting
		if(ret_val)
		{
			TEST_FAIL_MESSAGE("CB indicates it's full while it shouldn't.");
			break;
		}
	}

	//Manual checksum calculation, first 'len' values starting at offset = 0
	uint16_t len = 10;
	uint8_t manual_checksum = 0;
	for(i = 0; i < len; i++)
	{
		manual_checksum += w_array[i];
	}

	//We compare to our function
	ret_val = 0;
	uint8_t checksum = 0;
	uint16_t start_index = 0;
	uint16_t stop_index = len;
	ret_val = circ_buf_checksum(&cb, &checksum, start_index, stop_index);
	//circ_buf_checksum() should always return 0 if there are no errors
	TEST_ASSERT_EQUAL(0, ret_val);
	//We expect the same value as our manual test
	TEST_ASSERT_EQUAL(manual_checksum, checksum);

	//Calculate at the rollover point
	len = 10;
	manual_checksum = 0;
	for(i = CIRC_BUF_SIZE - (len / 2); i < CIRC_BUF_SIZE; i++)
	{
		manual_checksum += w_array[i];
	}
	for(i = 0; i < (len / 2); i++)
	{
		manual_checksum += w_array[i];
	}

	//We compare to our function
	ret_val = 0;
	checksum = 0;
	cb.read_index = CIRC_BUF_SIZE - (len / 2);//We manually change the read pointer
	start_index = 0;
	stop_index = len;
	ret_val = circ_buf_checksum(&cb, &checksum, start_index, stop_index);
	//circ_buf_checksum() should always return 0 if there are no errors
	TEST_ASSERT_EQUAL(0, ret_val);
	//We expect the same value as our manual test
	TEST_ASSERT_EQUAL(manual_checksum, checksum);

	//Re-init buffer. Fill with a handful of values.
	circ_buf_init(&cb);
	w_byte = 123;
	for(i = 0; i < CIRC_BUF_SIZE / 10; i++)
	{
		//Write to circular buffer
		ret_val = circ_buf_write_byte(&cb, w_byte);
		w_byte++;
	}
	//We attempt to calculate a checksum for more values than what's available
	ret_val = 0;
	checksum = 0;
	start_index = 0;
	stop_index = CIRC_BUF_SIZE / 2;
	ret_val = circ_buf_checksum(&cb, &checksum, start_index, stop_index);
	//circ_buf_checksum() should always return 0 if there are no errors => 1 in this case
	TEST_ASSERT_EQUAL(1, ret_val);
}

void test_circ_buf(void)
{
	RUN_TEST(test_circ_buf_w);
	RUN_TEST(test_circ_buf_rw);
	RUN_TEST(test_circ_buf_size);
	RUN_TEST(test_circ_buf_peek);
	RUN_TEST(test_circ_buf_search);
	RUN_TEST(test_circ_buf_checksum);

	fflush(stdout);
}

#ifdef __cplusplus
}
#endif
