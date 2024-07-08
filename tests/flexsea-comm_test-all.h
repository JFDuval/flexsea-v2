#ifdef __cplusplus
extern "C" {
#endif

#ifndef TEST_ALL_FX_COMM_H
#define TEST_ALL_FX_COMM_H

#include "unity.h"
#include "../inc/flexsea_buffers.h"
#include "../inc/flexsea_comm.h"
#include "../inc/flexsea_comm_def.h"

int flexsea_comm_test(void);

//Prototypes for public functions defined in individual test files:
//void test_flexsea(void);
void test_flexsea_buffers(void);
void test_flexsea_comm(void);
//void test_flexsea_payload(void);
void test_circ_buf(void);

#endif	//TEST_ALL_FX_COMM_H

#ifdef __cplusplus
}
#endif
