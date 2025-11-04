//****************************************************************************
// Include(s)
//****************************************************************************

#include <fx_transmit.h>
#include "main.h"

//****************************************************************************
// Variable(s)
//****************************************************************************

//UART transmission
uint8_t bytestream[MAX_ENCODED_PAYLOAD_BYTES] = {0};
uint8_t bytestream_len = 0;

DemoStructure my_demo_structure = {.var0_int8 = -1, .var1_uint32 = 123456,
		.var2_uint8 = 150, .var3_int32 = -1234567, .var4_int8 = -125,
		.var5_uint16 = 4567, .var6_uint8 = 123, .var7_int16 = -4567,
		.var8_float = 12.37};

//Stress test:
#define RAMP_MAX	1000
StressTestStructure stress_test;

//****************************************************************************
// Private Function Prototype(s)
//****************************************************************************

static uint8_t fx_tx_demo(uint8_t (*tx_fct_prt) (uint8_t *, uint16_t));
static uint8_t fx_tx_stress_test(uint8_t (*tx_fct_prt) (uint8_t *, uint16_t));
static uint8_t fx_tx_ack(CommPort *cp);

//****************************************************************************
// Public Function(s)
//****************************************************************************

uint8_t fx_transmit(CommPort *cp)
{
	if(cp->send_reply)
	{
		//Note: we might use a function pointer array here, just like in the
		//reception

		switch(cp->reply_cmd)
		{
			//case FX_CMD_WHO_AM_I:
			//	fx_tx_who_am_i(cp->tx_fct_prt);
			//	break;

			case FX_CMD_DEMO:
				fx_tx_demo(cp->tx_fct_prt);
				break;

			case FX_CMD_STRESS_TEST:
				fx_tx_stress_test(cp->tx_fct_prt);
				break;
		}

		cp->send_reply = 0;
	}
	else
	{
		if(cp->send_ack)
		{
			fx_tx_ack(cp);
		}
	}

	return FX_SUCCESS;
}

void fx_init_stress_test(void)
{
	stress_test.packet_number = -1;
	stress_test.ramp_value = -1;
}

//****************************************************************************
// Private Function(s)
//****************************************************************************

//This is the default FlexSEA stack test command.
static uint8_t fx_tx_demo(uint8_t (*tx_fct_prt) (uint8_t *, uint16_t))
{
	uint8_t ret_val = 0;

	//Send known, fixed values
	uint8_t payload_len = sizeof(my_demo_structure);
	uint8_t* payload = (uint8_t*)&my_demo_structure;

	ret_val = fx_create_bytestream_from_cmd(FX_CMD_DEMO, CmdWrite,
			Nack, payload, payload_len, bytestream, &bytestream_len);

	tx_fct_prt(bytestream, bytestream_len);

	return ret_val;
}


//Increment and ceil counter and ramp
void counter_and_ramp(void)
{
	stress_test.packet_number++;
	stress_test.ramp_value++;

    if(stress_test.ramp_value > RAMP_MAX)
    {
    	stress_test.ramp_value = 0;
    }
}

//FlexSEA Stress Test Command
static uint8_t fx_tx_stress_test(uint8_t (*tx_fct_prt) (uint8_t *, uint16_t))
{
	uint8_t ret_val = 0;

	//Send incrementing values
	counter_and_ramp();

	uint8_t payload_len = sizeof(stress_test);
	uint8_t* payload = (uint8_t*)&stress_test;

	ret_val = fx_create_bytestream_from_cmd(FX_CMD_STRESS_TEST, CmdWrite,
			Nack, payload, payload_len, bytestream, &bytestream_len);

	tx_fct_prt(bytestream, bytestream_len);

	return ret_val;
}

//FlexSEA Write Acknowledge. This is used for pure Writes that require an Ack.
//Reads and ReadWrite will Ack as part of the existing reply.
static uint8_t fx_tx_ack(CommPort *cp)
{
	uint8_t ret_val = 0;

	uint8_t payload_len = 3;
	uint8_t payload[3] = {0};
	payload[0] = cp->ack_cmd;
	payload[1] = CMD_ACK_PNUM_LSB(cp->ack_packet_num);
	payload[2] = CMD_ACK_PNUM_MSB(1, cp->ack_packet_num);

	ret_val = fx_create_bytestream_from_cmd(FX_CMD_ACK, CmdWrite,
			Ack, payload, payload_len, bytestream, &bytestream_len);

	cp->tx_fct_prt(bytestream, bytestream_len);

	return ret_val;
}
