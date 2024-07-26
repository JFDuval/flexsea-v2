from ctypes import *
print('Demo code - Python project with FlexSEA v2.0 DLL\n')
flexsea = cdll.LoadLibrary('../../DynamicLib/libflexsea-v2.dll')
ret_val = flexsea.fx_rx_cmd_init()
if ret_val:
    print("Problem initializing the FlexSEA stack - quit.")
    exit()

# Generate bytestream from text string (payload):
MAX_ENCODED_PAYLOAD_BYTES = 48
payload_string = "FlexSEA"
payload_in = c_char_p(payload_string.encode())
payload_in_len = c_uint8(7)
bytestream_ba = (c_uint8 * MAX_ENCODED_PAYLOAD_BYTES)()
bytestream_len = c_uint8(0)
cmd_6bits_in = c_uint8(23)
ret_val = 0
rw = c_uint8(2)

ret_val = flexsea.fx_create_bytestream_from_cmd(cmd_6bits_in, rw, payload_in, payload_in_len, bytestream_ba,
                                                byref(bytestream_len))
if not ret_val:
    print("We successfully created a bytestream.")
    bytestream = bytes(bytestream_ba)
    # print(bytestream)
    print(f'This will be a HEADER: {bytestream[0]}')
    print(f'This is our input payload: {bytestream[3:10]}')
else:
    print("We did not successfully create a bytestream. Quit.")
    exit()

# This structure holds all the info about a given circular buffer
CIRC_BUF_SIZE = 1000


class CircularBuffer(Structure):
    _fields_ = [("buffer", c_uint8 * CIRC_BUF_SIZE),
                ("read_index", c_uint16),
                ("write_index", c_uint16),
                ("length", c_uint16)]


cb = CircularBuffer()
for i in range(int(bytestream_len.value)):
    ret_val = flexsea.circ_buf_write_byte(byref(cb), bytestream[i])
    if ret_val:
        print("circ_buf_write_byte() problem!")
        exit()

# At this point our encoded command is in the circular buffer
cmd_6bits_out = c_uint8(0)
rw_out = c_uint8(0)
buf = (c_uint8 * MAX_ENCODED_PAYLOAD_BYTES)()
buf_len = c_uint8(0)
# Can we decode it?
ret_val = flexsea.fx_get_cmd_handler_from_bytestream(byref(cb), byref(cmd_6bits_out), byref(rw_out), buf,
                                                     byref(buf_len))
if not ret_val:
    print("We successfully got a command handler from a bytestream.")
    print(f'Command handler: {cmd_6bits_out.value}')
    print(f'R/W type: {rw_out.value}')
else:
    print("We did not successfully get a command handler. Quit.")
    exit()
