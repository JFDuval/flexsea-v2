from ctypes import *
import serial


dll_filename = '../../projects/eclipse_pc/DynamicLib/libflexsea-v2.dll'
com_port = 'COM5'
serial_port = 0  # Holds the serial port object


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


# Open serial port
def serial_open(com_port_name):
    global serial_port
    serial_port = serial.Serial(com_port_name, baudrate=115200)
    if serial_port:
        print(f'Successfully opened {com_port_name}.')
        return 0
    else:
        print(f'Could not open {com_port_name}!')
        return 1


# Write to a previously opened serial port
def serial_write(bytestream):
    if serial_port:
        serial_port.write(bytestream)
    else:
        print("No serial port object, can't write!")


# Loopback demo: we create a bytestream, shuffle it around then decode it
# No serial port required, no interaction with any other system
def flexsea_demo_local_loopback():

    print('Demo code - Python project with FlexSEA v2.0 DLL')
    print('Local Loopback - No external interaction\n')
    flexsea = cdll.LoadLibrary(dll_filename)
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

    # Call handler:
    python_flexsea_cmd_handler(cmd_6bits_out.value, rw.value, bytes(buf))


# Serial demo: we create and send commands to a serial peripheral
# (typically an STM32)
def flexsea_demo_serial():

    print('Demo code - Python project with FlexSEA v2.0 DLL')
    print('Serial TX - Connect a Nucleo first!\n')

    # Configure FlexSEA stack
    flexsea = cdll.LoadLibrary(dll_filename)
    ret_val = flexsea.fx_rx_cmd_init()
    if ret_val:
        print("Problem initializing the FlexSEA stack - quit.")
        exit()

    # Configure serial port
    ret_val = serial_open(com_port)
    if ret_val:
        print(f"Problem opening the {com_port} serial port - quit.")
        exit()

    # Generate bytestream from text string (payload):
    MAX_ENCODED_PAYLOAD_BYTES = 48
    payload_string = "FlexSEA"
    payload_in = c_char_p(payload_string.encode())
    payload_in_len = c_uint8(7)
    bytestream_ba = (c_uint8 * MAX_ENCODED_PAYLOAD_BYTES)()
    bytestream_len = c_uint8(0)
    cmd_6bits_in = c_uint8(1)  # Our STM32 demo code expects command #1
    rw = c_uint8(2)  # Write

    ret_val = flexsea.fx_create_bytestream_from_cmd(cmd_6bits_in, rw, payload_in, payload_in_len, bytestream_ba,
                                                    byref(bytestream_len))
    if not ret_val:
        print("We successfully created a bytestream.")
        bytestream = bytes(bytestream_ba)
    else:
        print("We did not successfully create a bytestream. Quit.")
        exit()

    # Send bytestream to serial port
    serial_write(bytestream)


if __name__ == "__main__":
    # Available demos, select one or more:
    flexsea_demo_local_loopback()
    print('\n=-=-=-=-=\n')
    flexsea_demo_serial()
