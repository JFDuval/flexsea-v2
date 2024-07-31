from ctypes import *
import serial
import time
import flexsea_python as fx

dll_filename = '../../projects/eclipse_pc/DynamicLib/libflexsea-v2.dll'
com_port = 'COM5'
serial_port = 0  # Holds the serial port object
new_tx_delay_ms = 2000


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
    payload_string = "FlexSEA"
    payload_in = c_char_p(payload_string.encode())
    payload_in_len = c_uint8(7)
    bytestream_ba = (c_uint8 * fx.MAX_ENCODED_PAYLOAD_BYTES)()
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

    cb = fx.CircularBuffer()
    for i in range(int(bytestream_len.value)):
        ret_val = flexsea.circ_buf_write_byte(byref(cb), bytestream[i])
        if ret_val:
            print("circ_buf_write_byte() problem!")
            exit()

    # At this point our encoded command is in the circular buffer
    cmd_6bits_out = c_uint8(0)
    rw_out = c_uint8(0)
    buf = (c_uint8 * fx.MAX_ENCODED_PAYLOAD_BYTES)()
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
    fx.python_flexsea_cmd_handler(cmd_6bits_out.value, rw.value, bytes(buf))


# Serial demo: we create and send commands to a serial peripheral
# (typically an STM32). Our peripheral will send a reply.
def flexsea_demo_serial():

    print('Demo code - Python project with FlexSEA v2.0 DLL')
    print('Serial TX - Connect a Nucleo first!\n')

    # Configure FlexSEA stack
    flexsea = cdll.LoadLibrary(dll_filename)
    ret_val = flexsea.fx_rx_cmd_init()
    if ret_val:
        print("Problem initializing the FlexSEA stack - quit.")
        exit()

    # Prepare for reception:
    cb = fx.CircularBuffer()
    cmd_6bits_out = c_uint8(0)
    rw_out = c_uint8(0)
    buf = (c_uint8 * fx.MAX_ENCODED_PAYLOAD_BYTES)()
    buf_len = c_uint8(0)

    # Configure serial port
    ret_val = serial_open(com_port)
    if ret_val:
        print(f"Problem opening the {com_port} serial port - quit.")
        exit()

    # Generate bytestream from text string (payload):
    payload_string = "FlexSEA"
    payload_in = c_char_p(payload_string.encode())
    payload_in_len = c_uint8(7)
    bytestream_ba = (c_uint8 * fx.MAX_ENCODED_PAYLOAD_BYTES)(0)
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

    # Flush any old RX bytes
    serial_port.reset_input_buffer()

    # Send bytestream to serial port
    serial_write(bytestream)
    send_new_tx_cmd = False
    send_new_tx_cmd_timestamp = round(time.time() * 1000)

    # Send a packet, and wait for a reception
    try:
        while True:

            # Send a new packet after a brief delay
            current_time = round(time.time() * 1000)
            if send_new_tx_cmd and current_time > send_new_tx_cmd_timestamp + new_tx_delay_ms:
                # Send bytestream to serial port
                serial_write(bytestream)
                send_new_tx_cmd = False

            # If we got out of sync we kick it off here
            if current_time > send_new_tx_cmd_timestamp + 3 * new_tx_delay_ms:
                print('We got our of sync... figure out why.')
                # Send bytestream to serial port
                serial_write(bytestream)

            # Feed any received bytes into the circular buffer
            bytes_to_read = serial_port.in_waiting
            if bytes_to_read > 0:
                print(f'Bytes to read: {bytes_to_read}.')
                for i in range(bytes_to_read):
                    new_rx_byte = serial_port.read(1)
                    ret_val = flexsea.circ_buf_write_byte(byref(cb), new_rx_byte[0])
                    if ret_val:
                        print("circ_buf_write_byte() problem!")
                        exit()

            # Can we decode it?
            ret_val = flexsea.fx_get_cmd_handler_from_bytestream(byref(cb), byref(cmd_6bits_out), byref(rw_out), buf,
                                                                 byref(buf_len))
            if not ret_val:
                print("We successfully got a command handler from a bytestream.")
                print(f'Command handler: {cmd_6bits_out.value}')
                print(f'R/W type: {rw_out.value}')
                # Call handler:
                fx.python_flexsea_cmd_handler(cmd_6bits_out.value, rw.value, bytes(buf))
                send_new_tx_cmd = True
                send_new_tx_cmd_timestamp = round(time.time() * 1000)

            time.sleep(0.01)
    except KeyboardInterrupt:
        print('Interrupted! End or demo code.')


if __name__ == "__main__":
    # Available demos, select one or more:
    flexsea_demo_local_loopback()
    print('\n=-=-=-=-=\n')
    flexsea_demo_serial()
