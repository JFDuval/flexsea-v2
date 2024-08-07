import serial
import time
import sys
#from ctypes import *

# Add the FlexSEA path to this project
sys.path.append('../../')
from flexsea_python.flexsea_python import FlexSEAPython

dll_filename = '../../projects/eclipse_pc/DynamicLib/libflexsea-v2.dll'
com_port = 'COM6'
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


# Custom command handler used by the serial demo code
def fx_rx_cmd_handler_23(cmd_6bits, rw, buf):
    print(f'Handler #23 received: cmd={cmd_6bits}, rw={rw}, buf={buf}.')
    print(f'This confirms the reception of our command, and the success of our demo code.')
    battery_mv = int.from_bytes(buf[1:4], 'little')
    uvlo = int(buf[5])
    last_uvlo = int(buf[6])
    lim_sw_1 = int(buf[7])
    lim_sw_2 = int(buf[8])
    print(f'battery_mv = {battery_mv}, uvlo = {uvlo}, last_uvlo = {last_uvlo}, lim_sw_1 = {lim_sw_1},'
          f'lim_sw_2 = {lim_sw_2}')


# Loopback demo: we create a bytestream, shuffle it around, then decode it
# No serial port required, no interaction with any other system: pure software loopback
def flexsea_demo_local_loopback():

    print('Demo code - Python project with FlexSEA v2.0 DLL')
    print('Local Loopback - No external interaction\n')

    # Initialize FlexSEA comm
    fx = FlexSEAPython(dll_filename)

    # Generate bytestream from text string (payload):
    ret_val, bytestream, bytestream_len = fx.create_bytestream_from_cmd(cmd=23, payload_string="FlexSEA")

    if not ret_val:
        print("We successfully created a bytestream.")
        print(f'This will be a HEADER: {bytestream[0]}')
        print(f'This is our input payload: {bytestream[3:10]}')
    else:
        print("We did not successfully create a bytestream. Quit.")
        exit()

    # Feed bytestream to circular buffer
    fx.write_to_circular_buffer(bytestream, bytestream_len)

    # At this point our encoded command is in the circular buffer. Can we decode it?
    ret_val, cmd_6bits_out, rw_out, buf, buf_len = fx.get_cmd_handler_from_bytestream()
    if not ret_val:
        print("We successfully got a command handler from a bytestream.")
        print(f'Command handler: {cmd_6bits_out}')
        print(f'R/W type: {rw_out}')
    else:
        print("We did not successfully get a command handler. Quit.")
        exit()

    # Call handler:
    fx.call_cmd_handler(cmd_6bits_out, rw_out, buf)


# Serial demo: we create and send commands to a serial peripheral
# (typically an STM32). Our peripheral will send a reply.
# This code will run until you stop it.
def flexsea_demo_serial():

    print('Demo code - Python project with FlexSEA v2.0 DLL')
    print('Serial TX - Connect a Nucleo first!\n')

    # Initialize FlexSEA comm
    fx = FlexSEAPython(dll_filename)

    # Configure serial port
    ret_val = serial_open(com_port)
    if ret_val:
        print(f"Problem opening the {com_port} serial port - quit.")
        exit()

    # Prepare for reception:
    fx.register_cmd_handler(23, fx_rx_cmd_handler_23)

    # Generate bytestream from text string (payload):
    ret_val, bytestream, bytestream_len = fx.create_bytestream_from_cmd(cmd=1, payload_string="FlexSEA")

    if not ret_val:
        print("We successfully created a bytestream.")
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
                print('We got out of sync... figure out why.')
                # Send bytestream to serial port
                serial_write(bytestream)

            # Feed any received bytes into the circular buffer
            bytes_to_read = serial_port.in_waiting
            if bytes_to_read > 0:
                print(f'Bytes to read: {bytes_to_read}.')
                for i in range(bytes_to_read):
                    new_rx_byte = serial_port.read(1)
                    ret_val = fx.write_to_circular_buffer(new_rx_byte[0], 1)
                    if ret_val:
                        print("circ_buf_write_byte() problem!")
                        exit()

            # At this point received commands are in the circular buffer. Can we decode them?
            ret_val, cmd_6bits_out, rw_out, buf, buf_len = fx.get_cmd_handler_from_bytestream()
            if not ret_val:
                print("We successfully got a command handler from a bytestream.")
                print(f'Command handler: {cmd_6bits_out}')
                print(f'R/W type: {rw_out}')
                # Call handler:
                fx.call_cmd_handler(cmd_6bits_out, rw_out, buf)
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
