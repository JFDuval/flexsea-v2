#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "flexsea_comm.h"
#include "flexsea-comm_test-all.h"
#include "unity.h"

int main(void)
{
    printf("Welcome to the FlexSEA v2.0 Test Suite\n");
    printf("======================================\n\n");

	UNITY_BEGIN();

	RUN_TEST(test_circ_buf);
	RUN_TEST(test_flexsea_comm);

	return UNITY_END();
}
