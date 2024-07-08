#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "circ_buf.h"
#include "unity.h"

//Test writing (and overwriting) a full buffer
void test_circ_buf_w(void)
{
	//Initialize new cir_buf
	circ_buf_t cb = {.buffer = {0},        \
	                 .length = 0,          \
	                 .write_index = 0,     \
	                 .read_index = 0};

	//Write sequential values to buffer, filling it
	int i = 0;
	uint8_t w_byte = 0, ret_val = 0;
	for(i = 0; i < CIRC_BUF_SIZE; i++)
	{
		//Write to circular buffer
		ret_val = circ_buf_write(&cb, w_byte);
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
	ret_val = circ_buf_write(&cb, w_byte);
	//circ_buf_write() should always return 0 if we are not overwriting => expecting 1
	TEST_ASSERT_EQUAL(1, ret_val);
	//Did that value get written?
	TEST_ASSERT_EQUAL(cb.buffer[0], w_byte);
}

//Test writing and reading a full buffer
void test_circ_buf_rw(void)
{
	//Initialize new cir_buf
	circ_buf_t cb = {.buffer = {0},        \
	                 .length = 0,          \
	                 .write_index = 0,     \
	                 .read_index = 0};
	//Create two arrays of the same size:
	uint8_t w_array[CIRC_BUF_SIZE] = {0};
	uint8_t r_array[CIRC_BUF_SIZE] = {0};

	//Write sequential values to buffer, filling it
	int i = 0;
	uint8_t w_byte = 0, ret_val = 0;
	for(i = 0; i < CIRC_BUF_SIZE; i++)
	{
		//Write to circular buffer
		ret_val = circ_buf_write(&cb, w_byte);
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
		ret_val = circ_buf_read(&cb, &r_byte);
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
	circ_buf_t cb = {.buffer = {0},        \
	                 .length = 0,          \
	                 .write_index = 0,     \
	                 .read_index = 0};

	//New buffer should report a size of 0
	TEST_ASSERT_EQUAL(0, circ_buf_size(&cb));

	//Write sequential values to buffer
	int i = 0;
	uint8_t w_byte = 0, ret_val = 0;
	int16_t manual_counter = 0;
	int16_t bytes_to_write = CIRC_BUF_SIZE / 2;
	for(i = 0; i < bytes_to_write; i++)
	{
		//Write to circular buffer
		ret_val = circ_buf_write(&cb, w_byte);
		w_byte++;

		//circ_buf_write() should always return 0 if we are not overwriting
		if(ret_val)
		{
			TEST_FAIL_MESSAGE("CB indicates it's full while it shouldn't.");
			break;
		}
	}
	manual_counter = bytes_to_write;
	TEST_ASSERT_EQUAL(manual_counter, circ_buf_size(&cb));

	//Read some bytes out
	int16_t bytes_to_read = bytes_to_write / 2;
	int16_t remaining_bytes_to_read = bytes_to_write - bytes_to_read; //(just in case CIRC_BUF_SIZE is odd)
	uint8_t r_byte = 0;
	for(i = 0; i < bytes_to_read; i++)
	{
		//Read from circular buffer
		ret_val = circ_buf_read(&cb, &r_byte);

		//circ_buf_read() should always return 0 if we are not overwriting
		if(ret_val)
		{
			TEST_FAIL_MESSAGE("CB indicates it's full while it shouldn't.");
			break;
		}
	}
	manual_counter -= bytes_to_read;
	TEST_ASSERT_EQUAL(manual_counter, circ_buf_size(&cb));

	//Finish reading the buffer
	for(i = 0; i < remaining_bytes_to_read; i++)
	{
		//Read from circular buffer
		ret_val = circ_buf_read(&cb, &r_byte);

		//circ_buf_read() should always return 0 if we are not overwriting
		if(ret_val)
		{
			TEST_FAIL_MESSAGE("CB indicates it's full while it shouldn't.");
			break;
		}
	}
	manual_counter -= remaining_bytes_to_read;
	TEST_ASSERT_EQUAL_MESSAGE(0, manual_counter, "Test writer error, this should be zero.");
	TEST_ASSERT_EQUAL(manual_counter, circ_buf_size(&cb));

	//And now we try to read from an empty buffer
	ret_val = circ_buf_read(&cb, &r_byte);
	//circ_buf_read() should always return 0 if we are not overreading => expecting 1, with r_byte = 0
	TEST_ASSERT_EQUAL(1, ret_val);
	TEST_ASSERT_EQUAL(0, r_byte);
	//Size should still be 0
	TEST_ASSERT_EQUAL(0, circ_buf_size(&cb));
}

/*
void test_buffer_circular_write_erase(void)
{
	circularBuffer_t buf;
	circularBuffer_t* cb = &buf;
	circ_buff_init(cb);

	srand(time(NULL));
	const int ALPHABET_LEN = 31;

	int i;
	uint8_t alphabet[ALPHABET_LEN];
	for(i = 0; i < ALPHABET_LEN; i++)
	{
		alphabet[i] = rand() % 256;
	}

	int shouldWrite, lengthToErase;
	int bufSize = circ_buff_get_size(cb);
	for(i = 0; i < 5000 || bufSize < (CB_BUF_LEN / 2); i++)
	{
		bufSize = circ_buff_get_size(cb);
		shouldWrite = rand() % 2;
		if(shouldWrite || bufSize < 2)
		{
			circ_buff_write(cb, alphabet, ALPHABET_LEN);
		}
		else
		{
			lengthToErase = rand() % (circ_buff_get_size(cb)/2);
			circ_buff_move_head(cb, lengthToErase);
		}
	}

	bufSize = circ_buff_get_size(cb);
	uint8_t outputBuf[CB_BUF_LEN];
	circ_buff_read(cb, outputBuf, bufSize);
	int j = ALPHABET_LEN-1;
	for(i = bufSize - 1; i >= 0; i--)
	{
		TEST_ASSERT_EQUAL(alphabet[j], outputBuf[i]);
		j--;
		if(j < 0)
			j+=ALPHABET_LEN;
	}
}

int getIndexOf(uint8_t value, uint8_t* buf, uint16_t start, uint16_t length)
{
	int i;
	for(i = start; i < length; i++)
	{
		if(buf[i] == value) return i;
	}
	return -1;
}

void test_buffer_circular_search(void)
{
	circularBuffer_t circBuf;
	circularBuffer_t* cb = &circBuf;
	circ_buff_init(cb);
	srand(time(NULL));

	uint8_t buf[CB_BUF_LEN];
	int i;
	for(i = 0; i < CB_BUF_LEN; i++)
		buf[i] = rand();

	//Perform a bunch of random writes to mess up the tail/head positions
	int length;
	for(i = 0; i < 100; i++)
	{
		length = rand() % CB_BUF_LEN;
		circ_buff_write(cb, buf, length);
	}

	//Write the actual values we will compare to
	circ_buff_write(cb, buf, CB_BUF_LEN);

	int expectedIndex, actualIndex, lastIndex;
	uint8_t value = 0, lastValue = 0;
	do {
		expectedIndex = 1;
		actualIndex = 1;
		lastIndex = -1;
		while(expectedIndex > 0)
		{
			expectedIndex = getIndexOf(value, buf, lastIndex+1, CB_BUF_LEN);
			actualIndex = circ_buff_search(cb, value, lastIndex+1);
			TEST_ASSERT_EQUAL(expectedIndex, actualIndex);
			lastIndex = expectedIndex;
		}

		lastValue = value;
		value++;
	} while(value > lastValue);
}

void test_buffer_circular_checksum(void)
{
	circularBuffer_t circBuf;
	circularBuffer_t* cb = &circBuf;
	circ_buff_init(cb);
	srand(time(NULL));

	uint8_t buf[CB_BUF_LEN];
	int i;
	for(i = 0; i < CB_BUF_LEN; i++)
		buf[i] = rand();


	int testI;
	for(testI = 0; testI < 100; testI++)
	{
		//Perform a bunch of random writes to mess up the tail/head positions
		int start, length;
		for(i = 0; i < 100; i++)
		{
			start = rand() % CB_BUF_LEN;
			length = rand() % (CB_BUF_LEN - start);
			circ_buff_write(cb, buf, length);
		}

		uint8_t *d = cb->bytes;
		uint8_t expectedSum = 0;
		for(i = 0; i < CB_BUF_LEN; i++)
		{
			expectedSum += d[i];
		}
		uint8_t actualSum = circ_buff_checksum(cb, 0, CB_BUF_LEN);
		TEST_ASSERT_EQUAL(expectedSum, actualSum);
	}
}

*/

void test_circ_buf(void)
{
	RUN_TEST(test_circ_buf_w);
	RUN_TEST(test_circ_buf_rw);
	RUN_TEST(test_circ_buf_size);
	//RUN_TEST(test_buffer_circular_write_erase);
	//RUN_TEST(test_buffer_circular_search);
	//RUN_TEST(test_buffer_circular_checksum);

	fflush(stdout);
}


#ifdef __cplusplus
}
#endif
