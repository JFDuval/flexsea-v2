#ifdef __cplusplus
extern "C" {
#endif

#include "tests.h"
#include "flexsea.h"

//Can we do UINT32 => bytes => UINT32?
void test_fx_split_rebuild_uint32(void)
{
	uint8_t buf[4] = {0};
	uint16_t index = 0;

	//Large number, 4 bytes required
	uint32_t start = 1234567890, end = 0;
	SPLIT_32(start, buf, &index);
	index = 0;
	end = REBUILD_UINT32(buf, &index);
	TEST_ASSERT_EQUAL(start, end);
	//Is the index tracking?
	TEST_ASSERT_EQUAL(4, index);

	//Tiny number, single byte
	index = 0;
	start = 1;
	end = 0;
	SPLIT_32(start, buf, &index);
	index = 0;
	end = REBUILD_UINT32(buf, &index);
	TEST_ASSERT_EQUAL(start, end);
}

//Can we do INT32 => bytes => INT32?
void test_fx_split_rebuild_int32(void)
{
	uint8_t buf[4] = {0};
	uint16_t index = 0;

	//Large number, 4 bytes required
	int32_t start = -1234567890, end = 0;
	SPLIT_32((uint32_t)start, buf, &index);
	index = 0;
	end = (int32_t)REBUILD_UINT32(buf, &index);
	TEST_ASSERT_EQUAL(start, end);
	//Is the index tracking?
	TEST_ASSERT_EQUAL(4, index);

	//Tiny number, single byte
	index = 0;
	start = -1;
	end = 0;
	SPLIT_32((uint32_t)start, buf, &index);
	index = 0;
	end = (int32_t)REBUILD_UINT32(buf, &index);
	TEST_ASSERT_EQUAL(start, end);
}

//Can we do UINT16 => bytes => UINT16?
void test_fx_split_rebuild_uint16(void)
{
	uint8_t buf[2] = {0};
	uint16_t index = 0;

	//Large number, 2 bytes required
	uint16_t start = 12345, end = 0;
	SPLIT_16(start, buf, &index);
	index = 0;
	end = REBUILD_UINT16(buf, &index);
	TEST_ASSERT_EQUAL(start, end);
	//Is the index tracking?
	TEST_ASSERT_EQUAL(2, index);

	//Tiny number, single byte
	index = 0;
	start = 1;
	end = 0;
	SPLIT_16(start, buf, &index);
	index = 0;
	end = REBUILD_UINT16(buf, &index);
	TEST_ASSERT_EQUAL(start, end);
}

//Can we do INT16 => bytes => INT16?
void test_fx_split_rebuild_int16(void)
{
	uint8_t buf[2] = {0};
	uint16_t index = 0;

	//Large number, 2 bytes required
	int16_t start = -12345, end = 0;
	SPLIT_16((uint16_t)start, buf, &index);
	index = 0;
	end = (int16_t)REBUILD_UINT16(buf, &index);
	TEST_ASSERT_EQUAL(start, end);
	//Is the index tracking?
	TEST_ASSERT_EQUAL(2, index);

	//Tiny number, single byte
	index = 0;
	start = -1;
	end = 0;
	SPLIT_16((uint16_t)start, buf, &index);
	index = 0;
	end = (int16_t)REBUILD_UINT16(buf, &index);
	TEST_ASSERT_EQUAL(start, end);
}

//Can we do float => bytes => float?
void test_fx_split_rebuild_float(void)
{
	uint8_t buf[4] = {0};
	uint16_t index = 0;

	//Random number, anything will use 4 bytes so not critical
	float start = -123.456, end = 0;
	SPLIT_FLOAT(start, buf, &index);
	index = 0;
	end = REBUILD_FLOAT(buf, &index);
	TEST_ASSERT_EQUAL(start, end);
	//Is the index tracking?
	TEST_ASSERT_EQUAL(4, index);
}

void test_flexsea_tools(void)
{
	RUN_TEST(test_fx_split_rebuild_uint32);
	RUN_TEST(test_fx_split_rebuild_int32);
	RUN_TEST(test_fx_split_rebuild_uint16);
	RUN_TEST(test_fx_split_rebuild_int16);
	RUN_TEST(test_fx_split_rebuild_float);

	fflush(stdout);
}

#ifdef __cplusplus
}
#endif
