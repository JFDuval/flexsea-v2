#include "main.h"

//We use this main to test the communication API and functionality outside of the test framework.
//If this works, we know we can integrate the communication stack into any C project.
//This should always use the highest level API available; the goal isn't to duplicate other
//unit tests, but to do a quick integration test on the full system.
int main()
{
	printf("Hello world! This is a FlexSEA Comm v0.2 test.\n\n");

	//We start by creating and packing a payload
	uint8_t payload[] = "FlexSEA Comm v0.2";
	uint8_t payload_len = sizeof(payload);
	uint8_t packed_payload[48] = {0};
	uint8_t packed_payload_len = 0;

	printf("Original payload: '%s' (length: %i bytes)\n", payload, payload_len);
	uint8_t ret_val = comm_pack_payload(payload, payload_len, packed_payload,
			&packed_payload_len, MAX_PACKED_PAYLOAD_BYTES);

	if(!ret_val)
	{
		printf("ret_val = %i. A ret_val of 0 means that comm_pack_payload() "
				"worked as expected.\n",
				ret_val);
	}
	else
	{
		printf(
				"ret_val = %i. A ret_val that's not 0 means that comm_pack_payload() "
				"encountered an error.\n",
				ret_val);
	}

	printf("Packaged payload: '%s' (length: %i bytes, %i bytes of overhead)\r\n",
			packed_payload, packed_payload_len, (packed_payload_len - payload_len));

	//We then feed it to a new circular buffer
	circ_buf_t cb = {.buffer = {0}, .length = 0, .write_index = 0, .read_index =
			0};
	int i = 0;
	ret_val = 0;
	for(i = 0; i < packed_payload_len + 1; i++)
	{
		//Write to circular buffer
		ret_val = circ_buf_write_byte(&cb, packed_payload[i]);

		//circ_buf_write() should always return 0 if we are not overwriting
		if(ret_val)
		{
			printf("An error occurred while loading bytes in the circular buffer!\n");
			break;
		}
	}

	//At this point our packaged payload is in 'cb'. We unpack it.

	uint8_t extracted_packed_payload[MAX_PACKED_PAYLOAD_BYTES] = {0};
	uint8_t extracted_packed_payload_len = 0;
	uint8_t extracted_unpacked_payload[MAX_PACKED_PAYLOAD_BYTES] = {0};
	uint8_t extracted_unpacked_payload_len = 0;
	ret_val = comm_unpack_payload(&cb, extracted_packed_payload,
			&extracted_packed_payload_len, extracted_unpacked_payload,
			&extracted_unpacked_payload_len);
	if(ret_val)
	{
		printf("An error occurred while unpacking our payload from the circular buffer!\n");
	}
	printf("Payload after it traveled through our communication stack: '%s' (length: %i bytes)\n",
			extracted_unpacked_payload, extracted_unpacked_payload_len);

	//The stack is padding up to 48 bytes. Are the actual payload bytes identical?
	if(!strncmp((char *)payload, (char *)extracted_unpacked_payload, payload_len))
	{
		printf("The first %i bytes of 'extracted_unpacked_payload' are identical to 'payload'. "
				"Success!", payload_len);
	}
	else
	{
		printf("The first %i bytes of 'extracted_unpacked_payload' are NOT identical to 'payload'. "
				"Problem.", payload_len);
	}

	return 0;
}
