#ifdef __cplusplus
extern "C" {
#endif

#include "tests.h"
#include "flexsea.h"

//Can do UINT32 => bytes => UINT32?
void test_fx_split_rebuild_uint32(void)
{
	uint8_t buf[4] = {0};
	uint16_t index = 0;
	uint32_t start_uint32 = 12345678, end_uint32 = 0;
	SPLIT_32(start_uint32, buf, &index);
	index = 0;
	end_uint32 = REBUILD_UINT32(buf, &index);

	TEST_ASSERT_EQUAL(start_uint32, end_uint32);
}

void test_flexsea_tools(void)
{
	RUN_TEST(test_fx_split_rebuild_uint32);

	fflush(stdout);
}

#ifdef __cplusplus
}
#endif
