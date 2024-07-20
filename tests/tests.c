#include "tests.h"

int main(void)
{
	printf("Welcome to the FlexSEA v2.0 Test Suite\n");
	printf("======================================\n\n");

	UNITY_BEGIN();

	RUN_TEST(test_circ_buf);
	RUN_TEST(test_flexsea_codec);
	RUN_TEST(test_flexsea_command);

	return UNITY_END();
}
