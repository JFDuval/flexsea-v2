#ifdef __cplusplus
extern "C" {
#endif

#include "tests.h"
#include "flexsea_comm.h"
#include "flexsea.h"
#include <time.h>

//We prepare a new circular buffer for our tests
circ_buf_t cb_test = {.buffer = {0}, .length = 0, .write_index = 0,
		.read_index = 0};
//CommPort
CommPort comm_port;

//This emulates USB reception. Use this to test the ping pong buffer reception.
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

//This emulates basic serial reception. Use this to test reception without ping pong buffers.
void uart_callback(uint8_t new_byte)
{
	//Original code: save one byte per ISR
	circ_buf_write_byte(&cb_test, new_byte);
}

void comm_port_init(CommPort *cp)
{
	//FlexSEA Comm Port:
	cp->id = 0;
	cp->send_reply = 0;
	cp->reply_cmd = 0;
	cp->cb = &cb_test;
	//cp->tx_fct_prt = (void);
	cp->use_ping_pong = 1;	//Enable ping pong buffers
	memset(cp->dbuf_ping, 0x00, DBUF_MAX_LEN);
	memset(cp->dbuf_pong, 0x00, DBUF_MAX_LEN);
	cp->dbuf_lock_ping = 0;
	cp->dbuf_lock_pong = 0;
	cp->dbuf_ping_len = 0;
	cp->dbuf_pong_len = 0;
	cp->dbuf_selected = 0;
}

void test_comm_flexsea_ping_pong_buffer(void)
{
	circ_buf_init(&cb_test);
	comm_port_init(&comm_port);

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

//This is a FlexSEA test command
uint8_t test_command_10a(uint8_t cmd_6bits, ReadWrite rw, AckNack ack, uint8_t *buf, uint8_t len)
{
	//We check a few parameters
	if((cmd_6bits == 10) && (rw == CmdRead) && (len >= 1) &&
			(cmd_6bits == CMD_GET_6BITS(buf[CMD_CODE_INDEX])))
	{
			//Valid
			return FX_SUCCESS;
	}
	else
	{
		//Problem
		return FX_PROBLEM;
	}
}

//Receive one packet using fx_receive()
void test_comm_flexsea_receive_full_packet(void)
{
	circ_buf_init(&cb_test);
	comm_port_init(&comm_port);

	//We create a payload
	uint8_t text_payload[55] = "This is a test: can we receive data using fx_receive? ";	//54 bytes
	uint8_t* payload_in = (uint8_t*)&text_payload;
	uint8_t payload_in_len = 54;

	uint8_t bytestream[MAX_ENCODED_PAYLOAD_BYTES] = {0};
	uint8_t bytestream_len = 0;
	uint8_t cmd_6bits_in = 10;
	uint8_t ret_val = 0, ret_val_cmd = 0;
	ReadWrite rw = CmdRead;
	AckNack ack = Nack;

	//Register test function:
	fx_register_rx_cmd_handler(cmd_6bits_in, &test_command_10a);

	//Encode test string
	ret_val = fx_create_bytestream_from_cmd(cmd_6bits_in, rw, ack, payload_in,
			payload_in_len, bytestream, &bytestream_len);
	TEST_ASSERT_EQUAL(0, ret_val);

	//Receive bytes via serial peripheral
	Comm_RxHandler(bytestream, bytestream_len);

	//Receive command?
	ret_val = fx_receive(&comm_port);
	TEST_ASSERT_EQUAL(0, ret_val);
	//If it got decoded, we will notify fx_transmit using these flags
	TEST_ASSERT_EQUAL(cmd_6bits_in, comm_port.reply_cmd);
	TEST_ASSERT_EQUAL(1, comm_port.send_reply);
}

//Receive 1000 packets using fx_receive()
void test_comm_flexsea_receive_many_full_packets(void)
{
	circ_buf_init(&cb_test);
	comm_port_init(&comm_port);

	//We create a payload
	uint8_t text_payload[55] = "This is a test: can we receive data using fx_receive? ";	//54 bytes
	uint8_t* payload_in = (uint8_t*)&text_payload;
	uint8_t payload_in_len = 54;

	uint8_t bytestream[MAX_ENCODED_PAYLOAD_BYTES] = {0};
	uint8_t bytestream_len = 0;
	uint8_t cmd_6bits_in = 10;
	uint8_t ret_val = 0;
	ReadWrite rw = CmdRead;
	AckNack ack = Nack;

	//Register test function:
	fx_register_rx_cmd_handler(cmd_6bits_in, &test_command_10a);

	//Encode test string
	ret_val = fx_create_bytestream_from_cmd(cmd_6bits_in, rw, ack, payload_in,
			payload_in_len, bytestream, &bytestream_len);
	TEST_ASSERT_EQUAL(0, ret_val);

	for(int i = 0; i < 1000; i++)
	{
		//Receive bytes via serial peripheral
		Comm_RxHandler(bytestream, bytestream_len);

		//Receive command?
		ret_val = fx_receive(&comm_port);
		TEST_ASSERT_EQUAL(0, ret_val);
		//If it got decoded, we will notify fx_transmit using these flags
		TEST_ASSERT_EQUAL(cmd_6bits_in, comm_port.reply_cmd);
		TEST_ASSERT_EQUAL(1, comm_port.send_reply);
	}
}

void generate_pseudo_random_bytes(uint8_t *buffer, uint8_t length)
{
    srand(time(NULL)); // Seed the random number generator with current time
    for(int i = 0; i < length; i++)
    {
    	buffer[i] = (uint8_t)(rand() % 256); // Generate a random byte (0-255)
	}
}

//Receive 1000 packets using fx_receive(). Noise in between each packet.
void test_comm_flexsea_receive_many_full_packets_with_noise_in_between(void)
{
	circ_buf_init(&cb_test);
	comm_port_init(&comm_port);

	//We create a payload
	uint8_t text_payload[55] = "This is a test: can we receive data using fx_receive? ";	//54 bytes
	uint8_t* payload_in = (uint8_t*)&text_payload;
	uint8_t payload_in_len = 54;

	uint8_t bytestream[MAX_ENCODED_PAYLOAD_BYTES] = {0};
	uint8_t bytestream_len = 0;
	uint8_t cmd_6bits_in = 10;
	uint8_t ret_val = 0, ret_val_cmd = 0;
	ReadWrite rw = CmdRead;
	AckNack ack = Nack;

	//Register test function:
	fx_register_rx_cmd_handler(cmd_6bits_in, &test_command_10a);

	//Encode test string
	ret_val = fx_create_bytestream_from_cmd(cmd_6bits_in, rw, ack, payload_in,
			payload_in_len, bytestream, &bytestream_len);
	TEST_ASSERT_EQUAL(0, ret_val);

	int noise_bytes = 0;
	uint8_t noisy_array[21] = {0};
	uint16_t good_packets = 0;
	for(int i = 0; i < 1000; i++)
	{
		//Receive bytes via serial peripheral
		Comm_RxHandler(bytestream, bytestream_len);

		//Receive command?
		for(int j = 0; j < 20; j++)
		{
			ret_val = fx_receive(&comm_port);
			if(ret_val == FX_SUCCESS)
			{
				//If it got decoded, we will notify fx_transmit using these flags
				TEST_ASSERT_EQUAL(cmd_6bits_in, comm_port.reply_cmd);
				TEST_ASSERT_EQUAL(1, comm_port.send_reply);
				good_packets++;
				break;
			}

			if(j == 19)
			{
				TEST_FAIL();
			}
		}

		//Add some noise
		noise_bytes++;
		if(noise_bytes > 20){noise_bytes = 0;}
		generate_pseudo_random_bytes(noisy_array, noise_bytes);
		Comm_RxHandler(noisy_array, noise_bytes);

		//Note: calling two RX before parsing breaks this code...

		//Receive command?
		for(int j = 0; j < 20; j++)
		{
			ret_val = fx_receive(&comm_port);
			if(ret_val == FX_SUCCESS)
			{
				//If it got decoded, we will notify fx_transmit using these flags
				TEST_ASSERT_EQUAL(cmd_6bits_in, comm_port.reply_cmd);
				TEST_ASSERT_EQUAL(1, comm_port.send_reply);
				TEST_FAIL();	//Just noise, not supposed to detect a packet
				break;
			}
		}
	}

	//Did we receive all the expected packets?
	TEST_ASSERT_EQUAL(1000, good_packets++);
}

//Receive 1000 packets using fx_receive(). Decode slowly.
void test_comm_flexsea_receive_slowly(void)
{
	circ_buf_init(&cb_test);
	comm_port_init(&comm_port);

	//We create a payload
	uint8_t text_payload[55] = "This is a test: can we receive data using fx_receive? ";	//54 bytes
	uint8_t* payload_in = (uint8_t*)&text_payload;
	uint8_t payload_in_len = 54;

	uint8_t bytestream[MAX_ENCODED_PAYLOAD_BYTES] = {0};
	uint8_t bytestream_len = 0;
	uint8_t cmd_6bits_in = 10;
	uint8_t ret_val = 0;
	ReadWrite rw = CmdRead;
	AckNack ack = Nack;

	//Register test function:
	fx_register_rx_cmd_handler(cmd_6bits_in, &test_command_10a);

	//Encode test string
	ret_val = fx_create_bytestream_from_cmd(cmd_6bits_in, rw, ack, payload_in,
			payload_in_len, bytestream, &bytestream_len);
	TEST_ASSERT_EQUAL(0, ret_val);

	uint16_t good_packets = 0;
	for(int i = 0; i < 1000; i++)
	{
		//Receive bytes via serial peripheral
		Comm_RxHandler(bytestream, bytestream_len);
		//Oh, receive another packet right away!
		Comm_RxHandler(bytestream, bytestream_len);

		//Receive command?
		uint8_t received = 0;
		for(int j = 0; j < 20; j++)
		{
			ret_val = fx_receive(&comm_port);
			if(ret_val == FX_SUCCESS)
			{
				//If it got decoded, we will notify fx_transmit using these flags
				TEST_ASSERT_EQUAL(cmd_6bits_in, comm_port.reply_cmd);
				TEST_ASSERT_EQUAL(1, comm_port.send_reply);
				good_packets++;
				received++;
			}

			if((j == 19) & !received)
			{
				TEST_FAIL();
			}
		}
	}

	//Did we receive all the expected packets?
	TEST_ASSERT_EQUAL(2000, good_packets++);
}

//Receive one packet using fx_receive(), one byte at the time
void test_comm_flexsea_receive_full_packet_byte_by_byte_ping_pong(void)
{
	circ_buf_init(&cb_test);
	comm_port_init(&comm_port);
	comm_port.use_ping_pong = 1;

	//We create a payload
	uint8_t text_payload[55] = "This is a test: can we receive data using fx_receive? ";	//54 bytes
	uint8_t* payload_in = (uint8_t*)&text_payload;
	uint8_t payload_in_len = 54;

	uint8_t bytestream[MAX_ENCODED_PAYLOAD_BYTES] = {0};
	uint8_t bytestream_len = 0;
	uint8_t cmd_6bits_in = 10;
	uint8_t ret_val = 0, ret_val_cmd = 0;
	ReadWrite rw = CmdRead;
	AckNack ack = Nack;

	//Register test function:
	fx_register_rx_cmd_handler(cmd_6bits_in, &test_command_10a);

	//Encode test string
	ret_val = fx_create_bytestream_from_cmd(cmd_6bits_in, rw, ack, payload_in,
			payload_in_len, bytestream, &bytestream_len);
	TEST_ASSERT_EQUAL(0, ret_val);

	uint8_t tiny_buffer[2] = {0};
	for(int i = 0; i < bytestream_len-1; i++)
	{
		//Receive one byte via serial peripheral
		tiny_buffer[0] = bytestream[i];
		Comm_RxHandler(tiny_buffer, 1);

		//Receive command?
		ret_val = fx_receive(&comm_port);
		TEST_ASSERT_EQUAL(1, ret_val);	//1 means not received
	}

	//Last byte
	tiny_buffer[0] = bytestream[bytestream_len - 1];
	Comm_RxHandler(tiny_buffer, 1);

	//Receive command?
	ret_val = fx_receive(&comm_port);
	TEST_ASSERT_EQUAL(0, ret_val);
	//If it got decoded, we will notify fx_transmit using these flags
	TEST_ASSERT_EQUAL(cmd_6bits_in, comm_port.reply_cmd);
	TEST_ASSERT_EQUAL(1, comm_port.send_reply);
}

//Receive one packet using fx_receive(), one byte at the time
void test_comm_flexsea_receive_full_packet_byte_by_byte_no_ping_pong(void)
{
	circ_buf_init(&cb_test);
	comm_port_init(&comm_port);
	comm_port.use_ping_pong = 0;

	//We create a payload
	uint8_t text_payload[55] = "This is a test: can we receive data using fx_receive? ";	//54 bytes
	uint8_t* payload_in = (uint8_t*)&text_payload;
	uint8_t payload_in_len = 54;

	uint8_t bytestream[MAX_ENCODED_PAYLOAD_BYTES] = {0};
	uint8_t bytestream_len = 0;
	uint8_t cmd_6bits_in = 10;
	uint8_t ret_val = 0, ret_val_cmd = 0;
	ReadWrite rw = CmdRead;
	AckNack ack = Nack;

	//Register test function:
	fx_register_rx_cmd_handler(cmd_6bits_in, &test_command_10a);

	//Encode test string
	ret_val = fx_create_bytestream_from_cmd(cmd_6bits_in, rw, ack, payload_in,
			payload_in_len, bytestream, &bytestream_len);
	TEST_ASSERT_EQUAL(0, ret_val);

	for(int i = 0; i < bytestream_len-1; i++)
	{
		//Receive one byte via serial peripheral
		uart_callback(bytestream[i]);

		//Receive command?
		ret_val = fx_receive(&comm_port);
		TEST_ASSERT_EQUAL(1, ret_val);	//1 means not received
	}

	//Last byte
	uart_callback(bytestream[bytestream_len - 1]);

	//Receive command?
	ret_val = fx_receive(&comm_port);
	TEST_ASSERT_EQUAL(0, ret_val);
	//If it got decoded, we will notify fx_transmit using these flags
	TEST_ASSERT_EQUAL(cmd_6bits_in, comm_port.reply_cmd);
	TEST_ASSERT_EQUAL(1, comm_port.send_reply);
}

void test_flexsea_comm(void)
{
	RUN_TEST(test_comm_flexsea_ping_pong_buffer);
	RUN_TEST(test_comm_flexsea_receive_full_packet);
	RUN_TEST(test_comm_flexsea_receive_many_full_packets);
	RUN_TEST(test_comm_flexsea_receive_many_full_packets_with_noise_in_between);
	RUN_TEST(test_comm_flexsea_receive_slowly);
	RUN_TEST(test_comm_flexsea_receive_full_packet_byte_by_byte_ping_pong);
	RUN_TEST(test_comm_flexsea_receive_full_packet_byte_by_byte_no_ping_pong);

	fflush(stdout);
}

#ifdef __cplusplus
}
#endif
