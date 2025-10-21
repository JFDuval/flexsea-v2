#ifdef __cplusplus
extern "C" {
#endif

#include "tests.h"
#include "flexsea_comm.h"
#include "flexsea.h"

//We prepare a new circular buffer for our tests
circ_buf_t cb_test = {.buffer = {0}, .length = 0, .write_index = 0,
		.read_index = 0};
//CommPort
CommPort comm_port;

//This emulates USB reception
void Comm_RxHandler(uint8_t* Buf, uint32_t Len)
{
	//Basic lock & double buffer
	if(comm_port.dbuf_selected == 0)
	{
		//Write to the Ping buffer
		comm_port.dbuf_lock_ping = 1;
		comm_port.dbuf_ping_len = Len;
		memcpy(comm_port.dbuf_ping, Buf, Len);
		comm_port.dbuf_lock_ping = 0;
		comm_port.dbuf_selected = 1;
	}
	else
	{
		//Write to the Pong buffer
		comm_port.dbuf_lock_pong = 1;
		comm_port.dbuf_pong_len = Len;
		memcpy(comm_port.dbuf_pong, Buf, Len);
		comm_port.dbuf_lock_pong = 0;
		comm_port.dbuf_selected = 0;
	}
}

void test_flexsea_ping_pong_buffer(void)
{
	circ_buf_init(&cb_test);

	//FlexSEA Comm Port:
	comm_port.id = 0;
	comm_port.send_reply = 0;
	comm_port.reply_cmd = 0;
	comm_port.cb = &cb_test;
	//comm_port.tx_fct_prt = (void);
	memset(comm_port.dbuf_ping, 0x00, DBUF_MAX_LEN);
	memset(comm_port.dbuf_pong, 0x00, DBUF_MAX_LEN);
	comm_port.dbuf_lock_ping = 0;
	comm_port.dbuf_lock_pong = 0;
	comm_port.dbuf_ping_len = 0;
	comm_port.dbuf_pong_len = 0;

	//We receive a few chunks of information and make sure it gets saved in order
	uint8_t wbuf1[10] = "Test #1.";
	Comm_RxHandler(wbuf1, 8);
	fx_comm_process_ping_pong_buffers(&comm_port);
	memcpy(wbuf1, "Worked?", 7);
	Comm_RxHandler(wbuf1, 7);
	fx_comm_process_ping_pong_buffers(&comm_port);
	memcpy(wbuf1, "For sure!", 9);
	Comm_RxHandler(wbuf1, 9);
	fx_comm_process_ping_pong_buffers(&comm_port);
	TEST_ASSERT_EQUAL(cb_test.length, 8+7+9);
	uint8_t read_back[8+7+9] = {0};
	for(int i = 0; i < (8+7+9+1); i++)
	{
		circ_buf_read_byte(&cb_test, &read_back[i]);
	}
	TEST_ASSERT_EQUAL_STRING("Test #1.Worked?For sure!", read_back);
}

void test_flexsea_comm(void)
{
	RUN_TEST(test_flexsea_ping_pong_buffer);

	fflush(stdout);
}

#ifdef __cplusplus
}
#endif
