#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "flexsea_comm.h"
#include "flexsea-comm_test-all.h"
#include "unity.h"

void test_test(void)
{
    uint8_t my_test_payload[] = "jfduval";
    uint8_t my_test_packed_payload[48] = {0};
    uint8_t retval = comm_gen_str(my_test_payload, my_test_packed_payload, 7);
	TEST_ASSERT_EQUAL_MESSAGE(10, retval, "How many bytes");

    //Feed this into a circular buffer

    //Call unpack_payload_cb(), make sure to get the string back
}


int main(void)
{
    printf("Welcome to the FlexSEA v2.0 Test Suite\n");
    printf("======================================\n\n");

	UNITY_BEGIN();

	RUN_TEST(test_test);
	RUN_TEST(test_flexsea_buffers);
	RUN_TEST(test_circ_buf);

	return UNITY_END();
}
