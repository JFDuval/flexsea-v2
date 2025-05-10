#ifndef INC_FX_TRANSMIT_H_
#define INC_FX_TRANSMIT_H_

//****************************************************************************
// Include(s)
//****************************************************************************

#include "main.h"

//****************************************************************************
// Definition(s):
//****************************************************************************

//Demo structure. The order of variables stresses memory alignment to highlight
//any potential problems.
typedef struct DemoStructure
{
	int8_t var0_int8;
	uint32_t var1_uint32;
	uint8_t var2_uint8;
	int32_t var3_int32;
	int8_t var4_int8;
	uint16_t var5_uint16;
	uint8_t var6_uint8;
	int16_t var7_int16;
	float var8_float;

}__attribute__((__packed__))DemoStructure;

typedef struct StressTestStructure
{
	int32_t packet_number;
	int16_t ramp_value;
	uint8_t reset;

}__attribute__((__packed__))StressTestStructure;

//****************************************************************************
// Public Function Prototype(s):
//****************************************************************************

uint8_t fx_transmit(uint8_t send_reply, uint8_t cmd_reply);
void fx_init_stress_test(void);

//****************************************************************************
// Shared variable(s)
//****************************************************************************

extern DemoStructure my_demo_structure;

#endif //INC_FX_TRANSMIT_H_
