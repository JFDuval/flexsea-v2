from ctypes import *

# The variables found before the FlexSEAPython class need to match the C code.
# Only edit if you have changed the code used to generate the DLL!

MAX_ENCODED_PAYLOAD_BYTES = 48

# This structure holds all the info about a given circular buffer
# This needs to match circ_buf.h!
CIRC_BUF_SIZE = 1000


class CircularBuffer(Structure):
    _fields_ = [("buffer", c_uint8 * CIRC_BUF_SIZE),
                ("read_index", c_uint16),
                ("write_index", c_uint16),
                ("length", c_uint16)]


class FlexSEAPython:

    def __init__(self, dll_filename):
        self.cb = CircularBuffer()
        self.fx = cdll.LoadLibrary(dll_filename)
        ret_val = self.fx.fx_rx_cmd_init()
        if ret_val:
            print("Problem initializing the FlexSEA stack - quit.")
            exit()

    def create_bytestream_from_cmd(self, cmd):
        payload_string = "FlexSEA"
        payload_in = c_char_p(payload_string.encode())
        payload_in_len = c_uint8(7)
        bytestream_ba = (c_uint8 * MAX_ENCODED_PAYLOAD_BYTES)()
        bytestream_len = c_uint8(0)
        cmd_6bits_in = c_uint8(cmd)
        rw = c_uint8(2)

        ret_val = self.fx.fx_create_bytestream_from_cmd(cmd_6bits_in, rw, payload_in, payload_in_len, bytestream_ba,
                                                        byref(bytestream_len))

        return ret_val, bytes(bytestream_ba), int(bytestream_len.value)

    def write_to_circular_buffer(self, bytestream, bytestream_len):
        if bytestream_len > 0:
            if bytestream_len == 1:
                # One byte at the time
                ret_val = self.fx.circ_buf_write_byte(byref(self.cb), bytestream)
                if ret_val:
                    print("circ_buf_write_byte() problem!")
                    exit()
            else:
                # Write array
                for i in range(bytestream_len):
                    ret_val = self.fx.circ_buf_write_byte(byref(self.cb), bytestream[i])
                    if ret_val:
                        print("circ_buf_write_byte() problem!")
                        exit()

    def get_cmd_handler_from_bytestream(self):
        # At this point our encoded command is in the circular buffer
        cmd_6bits_out = c_uint8(0)
        rw_out = c_uint8(0)
        buf = (c_uint8 * MAX_ENCODED_PAYLOAD_BYTES)()
        buf_len = c_uint8(0)
        # Can we decode it?
        ret_val = self.fx.fx_get_cmd_handler_from_bytestream(byref(self.cb), byref(cmd_6bits_out), byref(rw_out), buf,
                                                             byref(buf_len))
        return ret_val, cmd_6bits_out.value, rw_out.value,bytes(buf), buf_len.value

    def cmd_handler_catchall(self, cmd_6bits, rw, buf):
        print(f'Handler Catch-All received: cmd={cmd_6bits}, rw={rw}, buf={buf}.')

    def call_cmd_handler(self, cmd_6bits, rw, buf):
        if cmd_6bits == 23:
            fx_rx_cmd_handler_23(cmd_6bits, rw, buf)
        else:
            self.cmd_handler_catchall(cmd_6bits, rw, buf)






def fx_rx_cmd_handler_23(cmd_6bits, rw, buf):
    print(f'Handler #23 received: cmd={cmd_6bits}, rw={rw}, buf={buf}.')
    print(f'This confirms the reception of our command, and the success of our demo code.')


def python_flexsea_cmd_handler(cmd_6bits, rw, buf):
    if cmd_6bits == 23:
        fx_rx_cmd_handler_23(cmd_6bits, rw, buf)
    else:
        fx_rx_cmd_handler_catchall(cmd_6bits, rw, buf)



