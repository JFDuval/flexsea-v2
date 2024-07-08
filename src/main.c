#include "main.h"

int main()
{
    printf("Hello world! This is a test\n");

    uint8_t my_test_payload[] = "jfduval";
    uint8_t my_test_packed_payload[48] = {0};

    printf("Original payload: %s\n", my_test_payload);

    uint8_t retval = comm_gen_str(my_test_payload, my_test_packed_payload, 7);
    printf("retval = %i\n", retval);
    printf("Packaged payload: %s\r\n", my_test_packed_payload);

    //Feed this into a circular buffer

    //Call unpack_payload_cb(), make sure to get the string back

    return 0;
}
