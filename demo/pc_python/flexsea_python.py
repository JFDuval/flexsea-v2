from ctypes import *


def fx_rx_cmd_handler_catchall(cmd_6bits, rw, buf):
    print(f'Handler Catch-All received: cmd={cmd_6bits}, rw={rw}, buf={buf}.')


def fx_rx_cmd_handler_23(cmd_6bits, rw, buf):
    print(f'Handler #23 received: cmd={cmd_6bits}, rw={rw}, buf={buf}.')
    print(f'This confirms the reception of our command, and the success of our demo code.')


def python_flexsea_cmd_handler(cmd_6bits, rw, buf):
    if cmd_6bits == 23:
        fx_rx_cmd_handler_23(cmd_6bits, rw, buf)
    else:
        fx_rx_cmd_handler_catchall(cmd_6bits, rw, buf)


# This structure holds all the info about a given circular buffer
# This needs to match circ_buf.h!
CIRC_BUF_SIZE = 1000


class CircularBuffer(Structure):
    _fields_ = [("buffer", c_uint8 * CIRC_BUF_SIZE),
                ("read_index", c_uint16),
                ("write_index", c_uint16),
                ("length", c_uint16)]


MAX_ENCODED_PAYLOAD_BYTES = 48
