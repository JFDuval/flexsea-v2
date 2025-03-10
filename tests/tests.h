#ifdef __cplusplus
extern "C" {
#endif

#ifndef INC_TEST_H
#define INC_TEST_H

#include "../../unity/unity.h"
#include "flexsea.h"

int flexsea_comm_test(void);

//Prototypes for public functions defined in individual test files:
void test_flexsea_tools(void);
void test_circ_buf(void);
void test_flexsea_codec(void);
void test_flexsea_command(void);
void test_flexsea(void);


#endif	//INC_TEST_H

#ifdef __cplusplus
}
#endif
