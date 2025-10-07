import struct
import ctypes
from ctypes import *
import serial
from serial import SerialException
import time
import platform
from flexsea_tools import *

# The variables found before the FlexSEAPython class need to match the C code.
# Only edit if you have changed the code used to generate the DLL!

MAX_ENCODED_PAYLOAD_BYTES = 200
MIN_CMD_CODE = 0
MAX_CMD_CODE = 63
CMD_WHO_AM_I = 0
CMD_ACK = 1
MIN_OVERHEAD = 4
CMD_OVERHEAD = 3

# This structure holds all the info about a given circular buffer
# This needs to match circ_buf.h!
CIRC_BUF_SIZE = 1000


class CircularBuffer(Structure):
    _fields_ = [("buffer", c_uint8 * CIRC_BUF_SIZE),
                ("read_index", c_uint16),
                ("write_index", c_uint16),
                ("length", c_uint16)]


class WhoAmIStruct(Structure):
    _pack_ = 1
    _fields_ = [("uuid", c_uint32 * 3),
                ("serial_number", c_uint32),
                ("board", c_int8 * 12)]


class FlexSEASerial:

    def __init__(self, com_port_name=None, channel=-1, hardware=None):
        self.com_port_name = com_port_name
        self.serial_port = 0 # Holds the serial port object
        if self.com_port_name:
            self.open(self.com_port_name)

    def open(self, com_port_name, baudrate=115200):
        """
        Open serial port
        :param com_port_name: 'COM5', 'dev/ttyUSB0'
        :param baudrate: communication speed (default = 115200)
        :return: 0 if success, 1 otherwise
        """
        try:
            self.serial_port = serial.Serial(com_port_name, baudrate=baudrate)
        except SerialException:
            print(f"Could not open {com_port_name}! Either it is already open, either it doesn't exist.")

        if self.serial_port:
            print(f'Successfully opened {com_port_name}.')
            # Flush any old RX and TX bytes
            self.serial_port.reset_input_buffer()
            self.serial_port.reset_output_buffer()
            return 0
        else:
            return 1

    def valid_port(self):
        if self.serial_port:
            return 1
        else:
            return 0

    def write(self, bytestream, bytestream_length):
        """
        Write to a previously opened serial port
        :param bytestream: data to write
        :param bytestream_length: length of data to write
        :return:
        """
        timeout_counter = 10
        if self.serial_port:
            self.serial_port.write(bytestream[0:bytestream_length])
            while self.serial_port.out_waiting and timeout_counter:
                timeout_counter = timeout_counter - 1
                time.sleep(0.01)
        else:
            print("No serial port object, can't write!")

    def read_byte(self):
        """
        Read a byte from the serial port
        :return:
        """
        b = 0
        if self.serial_port:
            b = self.serial_port.read(1)
        return b

    def bytes_available(self):
        """
        Check for available bytes (rx)
        :return:
        """
        b = 0
        if self.serial_port:
            b = self.serial_port.in_waiting
        return b

    def reset_buffers(self):
        """
        Reset (empty) the RX and TX buffers
        :return:
        """
        if self.serial_port:
            self.serial_port.reset_input_buffer()
            self.serial_port.reset_output_buffer()


class FlexSEAPython:

    def __init__(self, dll_filename, open_new_port=True, com_port_name=None, channel=-1, existing_port=None):
        self.pf = self.identify_platform()
        self.com_port_name = com_port_name
        if open_new_port:
            # Create serial object, open new port
            self.serial = FlexSEASerial(com_port_name, channel)  # Holds the serial port object
        else:
            # Re-use a port
            self.serial = existing_port
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
        self.ack_dict = {
            "Nack": 0,
            "Ack": 1,
        }
        self.register_cmd_handler(CMD_ACK, self.fx_rx_cmd_handler_1)

    def create_bytestream_from_cmd(self, cmd, rw, ack, payload_string):
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
        ack_c = c_uint8(self.ack_dict[ack])

        ret_val = self.fx.fx_create_bytestream_from_cmd(cmd_6bits_in, rw_c, ack_c, payload_in, payload_in_len,
                                                        bytestream_ba, byref(bytestream_len))

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
                    print("circ_buf_write_byte(): buffer is full! Overwriting...")
                    return 1    # Problem
            else:
                # Write array
                for i in range(bytestream_len):
                    ret_val = self.fx.circ_buf_write_byte(byref(self.cb), bytestream[i])
                    if ret_val:
                        print("circ_buf_write_byte(): buffer is full! Overwriting...")
                        return 1    # Problem
            return 0    # Success
        else:
            return 1    # Problem

    def read_byte_from_circular_buffer(self):
        """
        This should only be used for debugging! This takes bytes from the CB, bypassing the stack
        :return: one byte
        """
        byte_read = c_uint8(0)
        self.fx.circ_buf_read_byte(byref(self.cb), byref(byte_read))
        return byte_read.value

    def get_circular_buffer_length(self):
        """
        Returns the length of the circular buffer
        :return: length in bytes
        """
        length = c_uint16(0)
        ret_val = self.fx.circ_buf_get_size(byref(self.cb), byref(length))
        return length.value

    def reinit_circular_buffer(self):
        """
        Buffer grew too big? Use this to start from scratch
        :return: 0 if it succeeded
        """
        ret_val = self.fx.circ_buf_init(byref(self.cb))
        return ret_val

    def get_cmd_handler_from_bytestream(self):
        # At this point our encoded command is in the circular buffer
        cmd_6bits_out = c_uint8(0)
        rw_out = c_uint8(0)
        ack_out = c_uint8(0)
        buf = (c_uint8 * MAX_ENCODED_PAYLOAD_BYTES)()
        buf_len = c_uint8(0)
        # Can we decode it?
        ret_val = self.fx.fx_get_cmd_handler_from_bytestream(byref(self.cb), byref(cmd_6bits_out), byref(rw_out),
                                                             byref(ack_out), buf, byref(buf_len))
        return ret_val, cmd_6bits_out.value, rw_out.value, ack_out.value, bytes(buf), buf_len.value

    def cmd_handler_catchall(self, cmd_6bits, rw, ack, buf):
        print(f'Handler Catch-All received: cmd={cmd_6bits}, rw={rw}, ack={ack}, buf={buf}.')
        print(f'This is a sign that you are missing a callback!')

    def init_cmd_handler(self):
        index = 0
        for i in range(MAX_CMD_CODE):
            self.cmd_handler_dict[index] = {index, self.cmd_handler_catchall}
            index += 1

    def register_cmd_handler(self, cmd, handler):
        self.cmd_handler_dict.update({cmd: handler})

    def call_cmd_handler(self, cmd_6bits, rw, ack, buf):
        my_cmd = self.cmd_handler_dict[cmd_6bits]
        # Member functions come as a set, but user callbacks do not
        if isinstance(my_cmd, set):
            # If it's a set, we call the element that's a method (the one that's not an int...)
            for item in my_cmd:
                if not isinstance(item, int):
                    item(cmd_6bits, rw, ack, buf)
        else:
            # Call without popping
            my_cmd(cmd_6bits, rw, ack, buf)

    def grab_new_bytes(self):
        """
        Feed any received bytes into the circular buffer
        :return:
        """
        bytes_to_read = self.serial.bytes_available()
        if bytes_to_read > 0:
            # print(f'Bytes to read: {bytes_to_read}.')
            for i in range(bytes_to_read):
                new_rx_byte = self.serial.read_byte()
                ret_val = self.write_to_circular_buffer(new_rx_byte[0], 1)


    def receive(self):
        """
        This replicates the embedded system's fx_receive command
        :return:
        """
        send_reply = 0
        cmd_reply = 0
        new_data = 0
        if self.get_circular_buffer_length() > MIN_OVERHEAD:
            ret_val, cmd_6bits_out, rw_out, ack_out, buf, buf_len = self.get_cmd_handler_from_bytestream()
            if not ret_val:
                # Call handler:
                self.call_cmd_handler(cmd_6bits_out, rw_out, ack_out, buf)
                new_data = 1
                # Reply if requested
                if (rw_out == self.rw_dict['CmdRead']) or (rw_out == self.rw_dict['CmdReadWrite']):
                    reply_cmd = cmd_6bits_out
                    send_reply = 1
                self.cleanup()
        else:
            self.cleanup()

        return send_reply, cmd_reply, new_data

    def rw_one_packet(self, bytestream, bytestream_len, start_time, callback=None, comm_wait=100):
        # Send bytestream to serial port
        self.serial.write(bytestream, bytestream_len)
        self.grab_new_bytes()  # Grab what could be already there (ex.: transceiver switching noise)
        time.sleep(0.002)  # Minimum round trip (2ms)
        current_time = round(time.time() * 1000)
        max_wait_time = current_time + comm_wait

        # Try reading for a while
        try:
            while current_time < max_wait_time:

                current_time = round(time.time() * 1000)

                # Feed any received bytes into the circular buffer
                self.grab_new_bytes()
                # Look for a packet in the new bytes
                send_reply, cmd_reply, new_data = self.receive()

                # We print & save after both new data has been received:
                if new_data:
                    timestamp = round(time.time() * 1000) - start_time
                    if callback:
                        callback(timestamp)
                    # Adding a return here causes comm issues (missed packets)

        except KeyboardInterrupt:
            print('Interrupted! End of rw_one_flexsea_packet routine.')
            exit()

    def cleanup(self):
        self.fx.fx_cleanup(byref(self.cb))

    def who_am_i(self):
        # Prepare for reception:
        self.register_cmd_handler(CMD_WHO_AM_I, self.fx_rx_cmd_handler_0)
        start_time = round(time.time() * 1000)

        # Who am I?
        ret_val, bytestream, bytestream_len = self.create_bytestream_from_cmd(cmd=CMD_WHO_AM_I, rw="CmdReadWrite",
                                                                              ack="Nack", payload_string='')
        self.rw_one_packet(bytestream, bytestream_len, start_time, None)

    @staticmethod
    def identify_platform():
        # What computer hardware is this code running on?
        pf_string = platform.platform()  # This returns a long string with OS and CPU info
        pf = 'Unknown'
        if pf_string.find('rpi') != -1:  # Ex: "Linux-6.12.25+rpt-rpi-2712-aarch64-with-glibc2.36"
            pf = "RPI"
        elif pf_string.find('Windows') != -1:  # Ex.: "Windows-11-10.0.26100-SP0"
            pf = "WIN"
        elif pf_string.find('MacOS') != -1:  # Ex.: "macOS-15.3.2-arm64-arm-64bit"
            pf = "MAC"
        else:
            pf = "LINUX"
            # By elimination, this is a Linux desktop... but it could be BSD or anything else!

        return pf

    def get_pf(self):
        # Accessor for the 'pf' variable (platform name, as short string)
        return self.pf

    def get_serial_port(self):
        # Return serial port object
        return self.serial

    @staticmethod
    def get_max_cb_length():
        global CIRC_BUF_SIZE
        return CIRC_BUF_SIZE

    def fx_rx_cmd_handler_0(self, cmd_6bits, rw, ack, buf):
        """
        # Who am I? Handler. ToDo document. Static?
        :param cmd_6bits:
        :param rw:
        :param ack:
        :param buf:
        :return:
        """
        rx_data = WhoAmIStruct()
        ctypes.memmove(pointer(rx_data), buf[CMD_OVERHEAD:], sizeof(rx_data))
        uuid = rx_data.uuid[0] * 2 ** 64 + rx_data.uuid[1] * 2 ** 32 + rx_data.uuid[2]  # Rebuild UUID C-style

        board_str_as_bytes = [chr(rx_data.board[i]) for i in range(len(rx_data.board))]
        board = ''.join(board_str_as_bytes)
        board = board.split('\x00', 1)[0]  # Remove trailing zeros
        print(f'Who am I? UUID = 0x{uuid:02X}, Serial number = {rx_data.serial_number}, Board = {board}')

    def fx_rx_cmd_handler_1(self, cmd_6bits, rw, ack, buf):
        """
        Command ACK handler. ToDo document. Static?
        :param rw:
        :param ack:
        :param buf:
        :return:
        """
        ack_cmd = byte_to_int8(buf[CMD_OVERHEAD])
        ack_packet_num = bytes_to_uint16(buf[CMD_OVERHEAD + 1:CMD_OVERHEAD + 3]) & 0x7FFF
        print(f'FlexSEA command acknowledged: cmd = {ack_cmd}, packet #{ack_packet_num}. The last TX # was '
              f'{self.fx.get_last_tx_packet_num()}')


class CommHardware:
    """
    Some applications have more than one channel per serial port.
    This class, while not very generic, is a first implementation.
    This supports three RS-485 ports on one RPi5, using one UART
    and some GPIOs to manage transceivers.
    """
    def __init__(self):
        self.rs485_re = []
        self.rs485_de = []
        # Transceiver states. TX is DE, RX is !RE.
        self.TX_ON = 1
        self.TX_OFF = 0
        self.RX_ON = 0
        self.RX_OFF = 1
        self.configured = False
        self.configure_rpi_rs485()

    def configure_rpi_rs485(self):
        # Crude integration of the pi485.py test code
        try:
            import gpiod
        except ImportError:
            print("Error! 'gpiod' doesn't exist for this platform, or the Python package is missing. Exiting.")
            return

        # RPi hat pins
        RS485_1_DE = 9
        RS485_1_RE = 11
        RS485_2_DE = 8
        RS485_2_RE = 7
        RS485_3_DE = 5
        RS485_3_RE = 6

        # Configure as outputs
        chip = gpiod.Chip('gpiochip4')

        # ch1
        self.rs485_re.append(chip.get_line(RS485_1_RE))
        self.rs485_de.append(chip.get_line(RS485_1_DE))
        self.rs485_re[0].request(consumer="RS485_1_DE", type=gpiod.LINE_REQ_DIR_OUT)
        self.rs485_de[0].request(consumer="RS485_1_RE", type=gpiod.LINE_REQ_DIR_OUT)
        # ch2
        self.rs485_re.append(chip.get_line(RS485_2_RE))
        self.rs485_de.append(chip.get_line(RS485_2_DE))
        self.rs485_re[1].request(consumer="RS485_2_DE", type=gpiod.LINE_REQ_DIR_OUT)
        self.rs485_de[1].request(consumer="RS485_2_RE", type=gpiod.LINE_REQ_DIR_OUT)
        # ch3
        self.rs485_re.append(chip.get_line(RS485_3_RE))
        self.rs485_de.append(chip.get_line(RS485_3_DE))
        self.rs485_re[2].request(consumer="RS485_3_DE", type=gpiod.LINE_REQ_DIR_OUT)
        self.rs485_de[2].request(consumer="RS485_3_RE", type=gpiod.LINE_REQ_DIR_OUT)

        # By default, we don't transmit and don't receive:
        self.rs485_de[0].set_value(self.TX_OFF)
        self.rs485_re[0].set_value(self.RX_OFF)
        self.rs485_de[1].set_value(self.TX_OFF)
        self.rs485_re[1].set_value(self.RX_OFF)
        self.rs485_de[2].set_value(self.TX_OFF)
        self.rs485_re[2].set_value(self.RX_OFF)

        self.configured = True

    def use_channel(self, desired_ch):
        if self.configured:
            if desired_ch == 0:
                self.rs485_de[0].set_value(self.TX_ON)
                self.rs485_re[0].set_value(self.RX_ON)
                self.rs485_de[1].set_value(self.TX_OFF)
                self.rs485_re[1].set_value(self.RX_OFF)
                self.rs485_de[2].set_value(self.TX_OFF)
                self.rs485_re[2].set_value(self.RX_OFF)
            elif desired_ch == 1:
                self.rs485_de[0].set_value(self.TX_OFF)
                self.rs485_re[0].set_value(self.RX_OFF)
                self.rs485_de[1].set_value(self.TX_ON)
                self.rs485_re[1].set_value(self.RX_ON)
                self.rs485_de[2].set_value(self.TX_OFF)
                self.rs485_re[2].set_value(self.RX_OFF)
            elif desired_ch == 2:
                self.rs485_de[0].set_value(self.TX_OFF)
                self.rs485_re[0].set_value(self.RX_OFF)
                self.rs485_de[1].set_value(self.TX_OFF)
                self.rs485_re[1].set_value(self.RX_OFF)
                self.rs485_de[2].set_value(self.TX_ON)
                self.rs485_re[2].set_value(self.RX_ON)
