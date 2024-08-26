import serial
import time
import sys

# Add the FlexSEA path to this project
sys.path.append('../../')
from flexsea_python import FlexSEAPython
from flexsea_tools import *
# Note: with PyCharm you must add this folder and mark is as a Sources Folder to avoid an Unresolved Reference issue

dll_filename = '../../projects/eclipse_pc/DynamicLib/libflexsea-v2.dll'
com_port = 'COM7'
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
# Note: this has to match the embedded code for the serial demo to work!
def fx_rx_cmd_handler_1(cmd_6bits, rw, buf):
    print(f'Handler #1 received: cmd={cmd_6bits}, rw={rw}, buf={buf}.')
    print(f'This confirms the reception of our command, and the success of our demo code.')
    var1_uint32 = bytes_to_uint32(buf[1:5])
    var2_uint8 = byte_to_uint8(buf[5])
    var3_int32 = bytes_to_int32(buf[6:10])
    var4_int8 = byte_to_int8(buf[10])
    var5_uint16 = bytes_to_uint16(buf[11:13])
    var6_uint8 = byte_to_uint8(buf[13])
    var7_int16 = bytes_to_int16(buf[14:16])
    var8_float = bytes_to_float(buf[16:20])
    print(f'var1_uint32 = {var1_uint32}, var2_uint8 = {var2_uint8}, var3_int32 = {var3_int32}, var4_int8 = '
          f'{var4_int8}, var5_uint16 = {var5_uint16}, var6_uint8 = {var6_uint8}, var7_int16 = {var7_int16}, '
          f'var8_float = {var8_float}')
    # We know what we are supposed to decode:
    if (var1_uint32 == 123456 and var2_uint8 == 150 and var3_int32 == -1234567 and var4_int8 == -125 and
            var5_uint16 == 4567 and var6_uint8 == 123 and var7_int16 == -4567 and round(var8_float, 2) == 12.37):
        print('\nAll decoded values match what our demo code is sending!\n')
    else:
        print('\nSome of the decoded values do not match what our demo code is sending!\n')


# We create and serialize the same payload the embedded system sends
def gen_test_code_payload():
    var1_uint32 = 123456
    var2_uint8 = 150
    var3_int32 = -1234567
    var4_int8 = -125
    var5_uint16 = 4567
    var6_uint8 = 123
    var7_int16 = -4567
    var8_float = 12.37

    payload_string = (uint32_to_bytes(var1_uint32) + uint8_to_byte(var2_uint8) + int32_to_bytes(var3_int32)
                      + int8_to_byte(var4_int8) + uint16_to_bytes(var5_uint16) + uint8_to_byte(var6_uint8)
                      + int16_to_bytes(var7_int16) + float_to_bytes(var8_float))

    return payload_string


# Loopback demo: we create a bytestream, shuffle it around, then decode it
# No serial port required, no interaction with any other system: pure software loopback
def flexsea_demo_local_loopback():

    print('Demo code - Python project with FlexSEA v2.0 DLL')
    print('Local Loopback - No external interaction\n')

    # Initialize FlexSEA comm
    fx = FlexSEAPython(dll_filename)

    # Prepare for reception:
    fx.register_cmd_handler(1, fx_rx_cmd_handler_1)

    # Generate bytestream from text string (payload):
    ret_val, bytestream, bytestream_len = fx.create_bytestream_from_cmd(cmd=1, rw="CmdReadWrite",
                                                                        payload_string=gen_test_code_payload())

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
    fx.register_cmd_handler(1, fx_rx_cmd_handler_1)

    # Generate bytestream from text string (payload):
    ret_val, bytestream, bytestream_len = fx.create_bytestream_from_cmd(cmd=1, rw="CmdReadWrite",
                                                                        payload_string=gen_test_code_payload())

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
