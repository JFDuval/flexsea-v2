import struct
from ctypes import *

# The variables found before the FlexSEAPython class need to match the C code.
# Only edit if you have changed the code used to generate the DLL!

MAX_ENCODED_PAYLOAD_BYTES = 200
MIN_CMD_CODE = 1
MAX_CMD_CODE = 63

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
        self.cmd_handler_dict = {}
        self.init_cmd_handler()
        self.rw_dict = {
            "CmdInvalid": 0,
            "CmdRead": 1,
            "CmdWrite": 2,
            "CmdReadWrite": 3,
        }

    def create_bytestream_from_cmd(self, cmd, rw, payload_string):
        """
        Create a new bytestream (data ready to be sent via USB) from a command, and a payload
        :param cmd: command code, between MIN_CMD_CODE and MAX_CMD_CODE
        :param rw: string from 'rw_dict' that describes the Read/Write command type
        :param payload_string: data to send, either as a string or a bytearray
        :return: ret_val (0 if success), bytestream and its length in bytes
        """
        if cmd < MIN_CMD_CODE or cmd > MAX_CMD_CODE:
            # Invalid command code
            return 1, [], 0

        if isinstance(payload_string, str):
            payload_in = c_char_p(payload_string.encode())
        else:
            # Data is already in the form of a byte array
            payload_in = payload_string
        payload_in_len = c_uint8(len(payload_string))
        bytestream_ba = (c_uint8 * MAX_ENCODED_PAYLOAD_BYTES)()
        bytestream_len = c_uint8(0)
        cmd_6bits_in = c_uint8(cmd)

        rw_c = c_uint8(self.rw_dict[rw])

        ret_val = self.fx.fx_create_bytestream_from_cmd(cmd_6bits_in, rw_c, payload_in, payload_in_len, bytestream_ba,
                                                        byref(bytestream_len))

        return ret_val, bytes(bytestream_ba), int(bytestream_len.value)

    def write_to_circular_buffer(self, bytestream, bytestream_len):
        """
        The C code living in the DLL takes care of everything, all it needs is some input data
        That data has to be in a circular buffer, and that's how we fill it.
        :param bytestream: serial data received, either a byte or an array
        :param bytestream_len: length of serial data
        :return: 0 if it succeeded
        """
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
            return 0    # Success
        else:
            return 1    # Problem

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
        print(f'If you are running a demo, you should not be seeing this!')

    def init_cmd_handler(self):
        index = 0
        for i in range(MAX_CMD_CODE):
            self.cmd_handler_dict[index] = {index, self.cmd_handler_catchall}
            index += 1

    def register_cmd_handler(self, cmd, handler):
        self.cmd_handler_dict.update({cmd: handler})

    def call_cmd_handler(self, cmd_6bits, rw, buf):
        my_cmd = self.cmd_handler_dict[cmd_6bits]
        # Member functions come as a set, but user callbacks do not
        if isinstance(my_cmd, set):
            # If it's a set, we get the first member
            my_cmd.pop()(cmd_6bits, rw, buf)
        else:
            # Call without popping
            my_cmd(cmd_6bits, rw, buf)
